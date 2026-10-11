#include "Property/PropertyHearingComponent.h"
#include "Property/Demo/PropertyWitness.h"
#include "Property/AI/PropertyAIController.h"
#include "Property/NPCKnowledgeComponent.h"
#include "Property/PropertyParticipantComponent.h"
#include "Engine/World.h"

UPropertyHearingComponent::UPropertyHearingComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = .25f;
}

void UPropertyHearingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	Suspicion = FMath::Max(0.f, Suspicion - FMath::Max(0.f, DecayPerSecond) * DeltaTime);
}

void UPropertyHearingComponent::Hear(FVector Location, float Strength, EPropertyNoiseKind Kind)
{
	auto* Witness = Cast<APropertyWitness>(GetOwner());
	auto* WitnessController = Witness ? Cast<APropertyAIController>(Witness->GetController()) : nullptr;
	if (!WitnessController || !Witness->Knowledge->bAlert || Witness->Participant->Health <= 0.f || !WitnessController->CanEnforceAt(Location)) return;
	if (Location.ContainsNaN() || !FMath::IsFinite(Strength) || Strength <= 0.f) return;
	LastHeardLocation = Location;
	LastHeardKind = Kind;
	const bool bUrgentNoise = Kind == EPropertyNoiseKind::Alarm || Kind == EPropertyNoiseKind::Distraction;
	Suspicion = bUrgentNoise ? 1.f : FMath::Clamp(Suspicion + Strength * (Kind == EPropertyNoiseKind::Footstep ? FootstepSensitivity : ImpactSensitivity), 0.f, 1.f);
	OnSoundHeard.Broadcast(Location, Kind, Suspicion);
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if ((Kind != EPropertyNoiseKind::Alarm && CurrentTime < NextReactionAt) || Suspicion < FMath::Clamp(InvestigationThreshold, .01f, 1.f)) return;
	if (!WitnessController->Investigate(Location, Kind == EPropertyNoiseKind::Alarm)) return;
	NextReactionAt = CurrentTime + FMath::Max(.1f, ReactionCooldown);
	Witness->SayCue(Witness->SelectNoiseCue(Kind));
}

void UPropertyHearingComponent::ResetHearing()
{
	Suspicion = 0.f;
	LastHeardLocation = FVector::ZeroVector;
	NextReactionAt = 0.f;
}
