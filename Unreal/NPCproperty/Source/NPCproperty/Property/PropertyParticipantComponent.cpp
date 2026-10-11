#include "Property/PropertyParticipantComponent.h"
#include "Property/PropertyLifeSubsystem.h"
#include "Property/PropertyIdentityComponent.h"
#include "Property/AI/PropertyAIController.h"
#include "Property/Demo/PropertyWitness.h"
#include "NPCpropertyCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"

UPropertyParticipantComponent::UPropertyParticipantComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPropertyParticipantComponent::BeginPlay()
{
	Super::BeginPlay();
	GetOwner()->OnTakeAnyDamage.AddDynamic(this, &UPropertyParticipantComponent::TakeDamage);
}

void UPropertyParticipantComponent::Notify(const FString& Message)
{
	Feedback = FText::FromString(Message);
	OnFeedback.Broadcast(Feedback);
}

void UPropertyParticipantComponent::Restore(int32 InGold, float InHealth)
{
	Gold = FMath::Max(0, InGold);
	Health = FMath::Clamp(InHealth, 0.f, 100.f);
	NextAttack = 0.f;
	NextNoise = 0.f;
	if (auto* NPC = Cast<APropertyWitness>(GetOwner())) NPC->RestoreAnimationPose();
}

bool UPropertyParticipantComponent::Spend(int32 Amount)
{
	if (Amount < 0 || Gold < Amount) return false;
	Gold -= Amount;
	return true;
}

void UPropertyParticipantComponent::Recover()
{
	Health = 100.f;
	if (auto* NPC = Cast<APropertyWitness>(GetOwner())) NPC->RestoreAnimationPose();
}

APropertyAIController* UPropertyParticipantComponent::GetConfrontation() const
{
	if (!GetWorld()) return nullptr;
	APropertyAIController* Nearest = nullptr;
	double NearestDistanceSquared = TNumericLimits<double>::Max();
	for (TActorIterator<APropertyAIController> It(GetWorld()); It; ++It)
	{
		if (It->ActiveResponse != EPropertyResponseState::Confront || It->DesiredResponse != EPropertyResponseState::Confront
			|| It->GetSuspect() != GetOwner() || !It->GetPawn() || !It->HasVisibleSuspect()
			|| !GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->OwnsResponse(UPropertyIdentityComponent::GetIdentity(GetOwner()).PersonId, *It)) continue;
		const double DistanceSquared = FVector::DistSquared(It->GetPawn()->GetActorLocation(), GetOwner()->GetActorLocation());
		if (DistanceSquared < NearestDistanceSquared)
		{
			Nearest = *It;
			NearestDistanceSquared = DistanceSquared;
		}
	}
	return Nearest;
}

bool UPropertyParticipantComponent::Choose(EConfrontationChoice Choice)
{
	if (auto* Controller = GetConfrontation()) return Controller->ResolveConfrontation(GetOwner(), Choice);
	Notify(TEXT("No nearby confrontation to answer."));
	return false;
}

void UPropertyParticipantComponent::TakeDamage(AActor* Actor, float Damage, const UDamageType* Type, AController* Instigator, AActor* Causer)
{
	if (!GetOwner()->HasAuthority() || !FMath::IsFinite(Damage) || Damage <= 0.f || Health <= 0.f) return;
	Health = FMath::Max(0.f, Health - Damage);
	if (auto* NPC = Cast<APropertyWitness>(GetOwner())) NPC->PlayDamageAnimation();
	if (Health > 0.f) return;
	if (auto* Pawn = Cast<APawn>(GetOwner()))
	{
		if (auto* Controller = Cast<APropertyAIController>(Pawn->GetController())) Controller->HandleDefeat();
		else if (auto* Attacker = Cast<APropertyAIController>(Instigator)) Attacker->TargetDefeated();
	}
}

void UPropertyParticipantComponent::Attack()
{
	if (GetWorld()->GetTimeSeconds() < NextAttack || Health <= 0.f) return;
	auto* Opponent = GetConfrontation();
	if (!Opponent || !Opponent->bResisting || !Opponent->HasVisibleSuspect())
	{
		Notify(TEXT("Choose resist before attacking during a confrontation."));
		return;
	}
	APawn* Target = Opponent->GetPawn();
	if (!Target || FVector::DistSquared(GetOwner()->GetActorLocation(), Target->GetActorLocation()) > FMath::Square(190.f)) return;
	NextAttack = GetWorld()->GetTimeSeconds() + .65f;
	const APawn* Pawn = Cast<APawn>(GetOwner());
	GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->EmitNoise(GetOwner(), Target->GetActorLocation(), 1100.f, EPropertyNoiseKind::Impact);
	UGameplayStatics::ApplyDamage(Target, 25.f, Pawn ? Pawn->GetController() : nullptr, GetOwner(), UDamageType::StaticClass());
	Notify(TEXT("Strike landed."));
}

void UPropertyParticipantComponent::Distract()
{
	const float Now = GetWorld()->GetTimeSeconds();
	if (Now < NextNoise) return;
	NextNoise = Now + 4.f;
	FVector Eye;
	FRotator Rotation;
	GetOwner()->GetActorEyesViewPoint(Eye, Rotation);
	const FVector End = Eye + Rotation.Vector() * 600.f;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PropertyNoise), false, GetOwner());
	const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Eye, End, ECC_Visibility, Params);
	const auto* Player = Cast<ANPCpropertyCharacter>(GetOwner());
	GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->EmitNoise(GetOwner(), bHit ? Hit.ImpactPoint + Hit.ImpactNormal * 5.f : End, 1200.f, EPropertyNoiseKind::Distraction, Player ? Player->ImpactSound.Get() : nullptr);
	Notify(TEXT("You throw a stone."));
}

void UPropertyParticipantComponent::Save()
{
	Notify(GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->SaveProgress() ? TEXT("Progress saved.") : TEXT("Save failed. Persistent identities must be unique."));
}

void UPropertyParticipantComponent::AdvanceTime()
{
	auto* Life = GetWorld()->GetSubsystem<UPropertyLifeSubsystem>();
	Life->SetHour(Life->Hour + 6.f);
	Notify(TEXT("Six hours pass."));
}

void UPropertyParticipantComponent::Load()
{
	Notify(GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->LoadProgress() ? TEXT("Progress loaded.") : TEXT("No compatible save for this map."));
}
