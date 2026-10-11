#include "Property/NPCKnowledgeComponent.h"
#include "Property/CrimeWorldSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Property/PropertyLifeSubsystem.h"
#include "Property/PropertyParticipantComponent.h"

UNPCKnowledgeComponent::UNPCKnowledgeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UNPCKnowledgeComponent::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetSubsystem<UCrimeWorldSubsystem>()->RegisterWitness(this);
}

void UNPCKnowledgeComponent::EndPlay(const EEndPlayReason::Type Reason)
{
	if (auto* Registry = GetWorld()->GetSubsystem<UCrimeWorldSubsystem>()) Registry->UnregisterWitness(this);
	Super::EndPlay(Reason);
}

bool UNPCKnowledgeComponent::CanWitness(AActor* Offender, AActor* Evidence) const
{
	if (!bAlert || !GetWorld() || !IsValid(GetOwner()) || !IsValid(Offender) || !IsValid(Evidence) || Offender == GetOwner()) return false;
	if (const auto* Participant = GetOwner()->FindComponentByClass<UPropertyParticipantComponent>(); Participant && Participant->Health <= 0.f) return false;
	FVector EyeLocation;
	FRotator ViewRotation;
	GetOwner()->GetActorEyesViewPoint(EyeLocation, ViewRotation);
	const float MinimumViewDot = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(HalfVisionAngle, 0.f, 180.f)));
	const auto IsVisible = [&](AActor* Target, AActor* Other) -> bool
	{
		const FVector DirectionToTarget = Target->GetActorLocation() - EyeLocation;
		const auto* Character = Cast<ACharacter>(Target);
		if (DirectionToTarget.SizeSquared() > FMath::Square(FMath::Max(0.f, SightRange) * (Character && Character->bIsCrouched ? .65f : 1.f))) return false;
		if (!DirectionToTarget.IsNearlyZero() && FVector::DotProduct(ViewRotation.Vector(), DirectionToTarget.GetSafeNormal()) < MinimumViewDot) return false;
		FCollisionQueryParams SightTraceParams(SCENE_QUERY_STAT(TheftWitness), false, GetOwner());

		SightTraceParams.AddIgnoredActor(Other);
		FHitResult SightHit;
		return !GetWorld()->LineTraceSingleByChannel(SightHit, EyeLocation, Target->GetActorLocation(), ECC_Visibility, SightTraceParams)
			|| SightHit.GetActor() == Target;
	};
	return IsVisible(Offender, Evidence) && IsVisible(Evidence, Offender);
}

bool UNPCKnowledgeComponent::CanSeeActor(AActor* Actor) const
{
	if (!bAlert || !GetWorld() || !IsValid(GetOwner()) || !IsValid(Actor) || Actor == GetOwner()) return false;
	if (const auto* Participant = GetOwner()->FindComponentByClass<UPropertyParticipantComponent>(); Participant && Participant->Health <= 0.f) return false;
	FVector EyeLocation;
	FRotator ViewRotation;
	GetOwner()->GetActorEyesViewPoint(EyeLocation, ViewRotation);
	const FVector DirectionToTarget = Actor->GetActorLocation() - EyeLocation;
	const auto* Character = Cast<ACharacter>(Actor);
	if (DirectionToTarget.SizeSquared() > FMath::Square(FMath::Max(0.f, SightRange) * (Character && Character->bIsCrouched ? .65f : 1.f))) return false;
	const float MinimumViewDot = FMath::Cos(FMath::DegreesToRadians(FMath::Clamp(HalfVisionAngle, 0.f, 180.f)));
	if (!DirectionToTarget.IsNearlyZero() && FVector::DotProduct(ViewRotation.Vector(), DirectionToTarget.GetSafeNormal()) < MinimumViewDot) return false;
	FCollisionQueryParams SightTraceParams(SCENE_QUERY_STAT(PropertyAwareness), false, GetOwner());
	FHitResult SightHit;
	return !GetWorld()->LineTraceSingleByChannel(SightHit, EyeLocation, Actor->GetActorLocation(), ECC_Visibility, SightTraceParams) || SightHit.GetActor() == Actor;
}

AActor* UNPCKnowledgeComponent::GetObservedOffender(FGuid IncidentId) const
{
	const auto* ObservedActor = ObservedOffenders.Find(IncidentId);
	return ObservedActor ? ObservedActor->Get() : nullptr;
}

void UNPCKnowledgeComponent::Remember(const FTheftIncident& Incident, AActor* Offender)
{
	if (KnownThefts.ContainsByPredicate([&](const FTheftIncident& Known) { return Known.IncidentId == Incident.IncidentId; })) return;
	KnownThefts.Add(Incident);
	ObservedOffenders.Add(Incident.IncidentId, Offender);
	OnTheftWitnessed.Broadcast(Incident);
}

TArray<FTheftIncident> UNPCKnowledgeComponent::GetUnsettledCases(FName PersonId) const
{
	const auto* LifeSubsystem = GetWorld()->GetSubsystem<UPropertyLifeSubsystem>();
	return KnownThefts.FilterByPredicate([&](const FTheftIncident& Incident)
	{
		return !PersonId.IsNone() && Incident.Offender.PersonId == PersonId && !LifeSubsystem->IsSettled(Incident.IncidentId);
	});
}

int32 UNPCKnowledgeComponent::CountKnownThefts(FName PersonId) const
{
	int32 TheftCount = 0;
	for (const auto& Incident : KnownThefts)
		if (Incident.Offender.PersonId == PersonId) ++TheftCount;
	return TheftCount;
}

FName UNPCKnowledgeComponent::GetEvidenceSource(FGuid IncidentId) const
{
	const FName* Source = EvidenceSources.Find(IncidentId);
	return Source ? *Source : NAME_None;
}
