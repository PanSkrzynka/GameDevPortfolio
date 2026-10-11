#include "Property/PropertyInspectionComponent.h"
#include "Property/Demo/PropertyWitness.h"
#include "Property/AI/PropertyAIController.h"
#include "Property/NPCKnowledgeComponent.h"
#include "Property/PropertyParticipantComponent.h"
#include "Property/OwnedPickup.h"
#include "Property/OwnershipComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Property/PropertyLifeSubsystem.h"
#include "Property/PropertyWorldSubsystem.h"
#include "Property/PropertyArea.h"
#include "Property/PropertyIdentityComponent.h"
#include "GameFramework/PlayerController.h"

UPropertyInspectionComponent::UPropertyInspectionComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 1.f;
}

void UPropertyInspectionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    for (auto& ObjectMemory : Memories) ObjectMemory.ObservationAge += DeltaTime;
    for (auto& MissingReport : Reports) MissingReport.Age += DeltaTime;
    InspectObjects();
}

bool UPropertyInspectionComponent::CanInspectLocation(FVector Location) const
{
    const auto* Witness = Cast<APropertyWitness>(GetOwner());
    FVector EyeLocation;
    FRotator ViewRotation;
    Witness->GetActorEyesViewPoint(EyeLocation, ViewRotation);
    const FVector DirectionToInspection = Location - EyeLocation;
    if (DirectionToInspection.SizeSquared() > FMath::Square(FMath::Max(0.f, FMath::Min(InspectionRange, Witness->Knowledge->SightRange)))) return false;
    if (FVector::DotProduct(ViewRotation.Vector(), DirectionToInspection.GetSafeNormal()) < FMath::Cos(FMath::DegreesToRadians(Witness->Knowledge->HalfVisionAngle))) return false;
    FHitResult VisibilityHit;
    return !GetWorld()->LineTraceSingleByChannel(VisibilityHit, EyeLocation, Location, ECC_Visibility, FCollisionQueryParams(SCENE_QUERY_STAT(PropertyInspection), false, Witness));
}

void UPropertyInspectionComponent::InspectObjects()
{
    auto* Witness = Cast<APropertyWitness>(GetOwner());
    auto* WitnessController = Witness ? Cast<APropertyAIController>(Witness->GetController()) : nullptr;
    if (!bEnabled || !Witness || !Witness->HasAuthority() || !WitnessController || !Witness->Knowledge->bAlert || Witness->Participant->Health <= 0.f) return;
    APawn* Player = GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr;
    TOptional<bool> CachedPlayerVisibility;
    for (auto& ObjectMemory : Memories)
    {
        if (!ObjectMemory.bMissing && Player && FVector::DistSquared(Player->GetActorLocation(), ObjectMemory.Location) < FMath::Square(450.f))
        {
            if (!CachedPlayerVisibility.IsSet()) CachedPlayerVisibility = Witness->Knowledge->CanSeeActor(Player);
            if (!CachedPlayerVisibility.GetValue()) continue;
            ObjectMemory.NearbyPerson = UPropertyIdentityComponent::GetIdentity(Player).PersonId;
            ObjectMemory.ObservationAge = 0.f;
        }
    }
    if (WitnessController->DesiredResponse != EPropertyResponseState::Routine) return;
    TSet<FGuid> PresentItemIds;
    for (TActorIterator<AOwnedPickup> PickupIterator(GetWorld()); PickupIterator; ++PickupIterator)
    {
        if (!PickupIterator->IsAvailable()) continue;
        PresentItemIds.Add(PickupIterator->ItemId);
        if (FVector::DistSquared(Witness->GetActorLocation(), PickupIterator->GetActorLocation()) > FMath::Square(InspectionRange)) continue;
        const auto OwnerIdentity = PickupIterator->Ownership->GetLegalOwner();
        if ((OwnerIdentity.PersonId.IsNone() && OwnerIdentity.FactionId.IsNone()) || !WitnessController->CanEnforceAt(PickupIterator->GetActorLocation()) || !Witness->Knowledge->CanSeeActor(*PickupIterator)) continue;
        auto* ObjectMemory = Memories.FindByPredicate([&](const FPropertyObjectMemory& Entry) { return Entry.ItemId == PickupIterator->ItemId; });
        if (!ObjectMemory) ObjectMemory = &Memories.AddDefaulted_GetRef();
        ObjectMemory->ItemId = PickupIterator->ItemId;
        ObjectMemory->Name = PickupIterator->DisplayName;
        ObjectMemory->Location = PickupIterator->GetActorLocation();
        ObjectMemory->bMissing = false;
    }
    for (auto& ObjectMemory : Memories)
    {
        if (ObjectMemory.bMissing || PresentItemIds.Contains(ObjectMemory.ItemId) || !WitnessController->CanEnforceAt(ObjectMemory.Location) || !CanInspectLocation(ObjectMemory.Location)) continue;
        ObjectMemory.bMissing = true;
        if (Witness->Knowledge->GetKnownTheftsView().ContainsByPredicate([&](const FTheftIncident& Incident) { return Incident.Item.ItemId == ObjectMemory.ItemId; })) continue;
        Witness->Say(FText::Format(Witness->GetDialogue(EPropertyDialogueCue::MissingItem), FFormatNamedArguments{{TEXT("Item"), ObjectMemory.Name}}));
        FPropertyMissingReport MissingReport;
        MissingReport.ItemId = ObjectMemory.ItemId;
        MissingReport.Name = ObjectMemory.Name;
        MissingReport.Location = ObjectMemory.Location;
        MissingReport.Source = Witness->Identity->Identity.PersonId;
        if (ObjectMemory.ObservationAge <= 15.f) MissingReport.PossiblePerson = ObjectMemory.NearbyPerson;
        Reports.Add(MissingReport);
        auto* Area = GetWorld()->GetSubsystem<UPropertyWorldSubsystem>()->ResolveArea(ObjectMemory.Location);
        if (Area) Area->RegisterIncident(ObjectMemory.ItemId);
        WitnessController->AssignSearch(Area, ObjectMemory.Location);
        GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->EmitNoise(Witness, Witness->GetActorLocation(), 2200.f, EPropertyNoiseKind::Alarm, Witness->AlarmSound);
        OnObjectMissing.Broadcast(ObjectMemory);
        break;
    }
}

