#include "Property/PropertyArea.h"
#include "Property/PropertyWorldSubsystem.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

APropertyArea::APropertyArea()
{
	PrimaryActorTick.bCanEverTick = false;
	Bounds = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	SetRootComponent(Bounds);
	Bounds->SetBoxExtent(FVector(300, 300, 150));
	Bounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Bounds->SetGenerateOverlapEvents(false);
	DisplayName = FText::FromString(TEXT("Private property"));
}

bool APropertyArea::IsAssigned(FName PersonId) const
{
	return !PersonId.IsNone() && AssignedNPCs.Contains(PersonId);
}

bool APropertyArea::ContainsPoint(FVector WorldPoint) const
{
	const FVector Local = Bounds->GetComponentTransform().InverseTransformPosition(WorldPoint);
	const FVector Extent = Bounds->GetUnscaledBoxExtent();
	return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Y) <= Extent.Y && FMath::Abs(Local.Z) <= Extent.Z;
}

bool APropertyArea::CanEnter(const FPropertyIdentity& Visitor) const
{
	if (GetAlertLevel() == EPropertyAlertLevel::Lockdown && (Visitor.PersonId.IsNone() || Visitor.PersonId != LegalOwner.PersonId) && !IsAssigned(Visitor.PersonId)) return false;
	return EntryPermission.Allows(Visitor, LegalOwner);
}

double APropertyArea::GetBoundsVolume() const
{
	const FVector Extent = Bounds->GetScaledBoxExtent().GetAbs();
	return Extent.X * Extent.Y * Extent.Z * 8.0;
}

void APropertyArea::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetSubsystem<UPropertyWorldSubsystem>()->RegisterArea(this);
}

void APropertyArea::EndPlay(const EEndPlayReason::Type Reason)
{
	if (auto* Registry = GetWorld()->GetSubsystem<UPropertyWorldSubsystem>())
	{
		Registry->UnregisterArea(this);
	}
	Super::EndPlay(Reason);
}

EPropertyAlertLevel APropertyArea::GetAlertLevel() const
{
    if (AlertScore >= 90.f) return EPropertyAlertLevel::Lockdown;
    if (AlertScore >= 40.f) return EPropertyAlertLevel::Searching;
    return AlertScore > 0.f ? EPropertyAlertLevel::Suspicious : EPropertyAlertLevel::Calm;
}

void APropertyArea::RegisterIncident(FGuid ItemId, bool bWitnessed)
{
    if (!ItemId.IsValid() || AlertedItems.Contains(ItemId)) return;
    AlertedItems.Add(ItemId);
    AlertScore = FMath::Min(120.f, AlertScore + (bWitnessed ? 50.f : 25.f));
    QuietSeconds = 0.f;
}

void APropertyArea::RegisterGuardReport(FGuid ItemId)
{
    if (!ItemId.IsValid() || ReportedItems.Contains(ItemId)) return;
    ReportedItems.Add(ItemId);
    AlertScore = FMath::Min(120.f, AlertScore + 20.f);
    QuietSeconds = 0.f;
}

void APropertyArea::AdvanceAlert(float DeltaTime)
{
    if (!FMath::IsFinite(DeltaTime) || DeltaTime <= 0.f) return;
    const float Previous = QuietSeconds;
    QuietSeconds += DeltaTime;
    const float DecayTime = FMath::Max(0.f, QuietSeconds - FMath::Max(0.f, AlertDecayDelay)) - FMath::Max(0.f, Previous - FMath::Max(0.f, AlertDecayDelay));
    AlertScore = FMath::Max(0.f, AlertScore - DecayTime * FMath::Max(0.f, AlertDecayRate));
}

FVector APropertyArea::GetExitLocation() const
{
    return GetActorTransform().TransformPosition(ExitOffset);
}
