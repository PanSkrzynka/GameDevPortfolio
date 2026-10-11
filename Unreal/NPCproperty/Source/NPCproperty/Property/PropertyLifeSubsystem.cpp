#include "Property/PropertyLifeSubsystem.h"
#include "Property/PropertyInspectionComponent.h"
#include "Property/AI/PropertyAIController.h"
#include "Property/Demo/PropertyWitness.h"
#include "Property/NPCKnowledgeComponent.h"
#include "Property/PropertyIdentityComponent.h"
#include "Property/PropertyInteractionComponent.h"
#include "Property/PropertyParticipantComponent.h"
#include "Property/CrimeWorldSubsystem.h"
#include "Property/OwnedPickup.h"
#include "Property/OwnershipComponent.h"
#include "Property/PropertyArea.h"
#include "Property/PropertyWorldSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

void UPropertyLifeSubsystem::Tick(float DeltaTime)
{
	if (bRestoring) return;
	if (SecondsPerHour > 0.f) SetHour(Hour + DeltaTime / SecondsPerHour);
	GetWorld()->GetSubsystem<UPropertyWorldSubsystem>()->AdvanceAlerts(DeltaTime);
}

TStatId UPropertyLifeSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UPropertyLifeSubsystem, STATGROUP_Tickables);
}

bool UPropertyLifeSubsystem::IsTickable() const
{
	return !IsTemplate() && GetWorld() && GetWorld()->IsGameWorld() && GetWorld()->HasBegunPlay();
}

void UPropertyLifeSubsystem::SetHour(float Value)
{
	if (FMath::IsFinite(Value)) Hour = FMath::Fmod(FMath::Fmod(Value, 24.f) + 24.f, 24.f);
}

bool UPropertyLifeSubsystem::IsSettled(FGuid IncidentId) const
{
	return Settled.Contains(IncidentId);
}

void UPropertyLifeSubsystem::Settle(const TArray<FTheftIncident>& TheftCases)
{
	for (const auto& TheftIncident : TheftCases) Settled.Add(TheftIncident.IncidentId);
}

bool UPropertyLifeSubsystem::ClaimResponse(FName SuspectId, APropertyAIController* Controller)
{
	if (SuspectId.IsNone() || !IsValid(Controller)) return false;
	Responders.Add(Controller, SuspectId);
	return true;
}

bool UPropertyLifeSubsystem::OwnsResponse(FName SuspectId, APropertyAIController* Controller) const
{
	const auto* Current = Responders.Find(Controller);
	return Current && *Current == SuspectId;
}

void UPropertyLifeSubsystem::ReleaseResponse(APropertyAIController* Controller)
{
	for (auto ResponseIterator = Responders.CreateIterator(); ResponseIterator; ++ResponseIterator)
	{
		if (!ResponseIterator.Key().IsValid() || ResponseIterator.Key().Get() == Controller) ResponseIterator.RemoveCurrent();
	}
}

bool UPropertyLifeSubsystem::ReportToGuard(APropertyWitness* Witness, APropertyWitness* Guard)
{
	const auto* Controller = IsValid(Guard) ? Cast<APropertyAIController>(Guard->GetController()) : nullptr;
	if (Witness == Guard && Controller && Controller->IsGuard() && Guard->GetWorld() == GetWorld()
		&& Guard->HasAuthority() && Guard->Knowledge->bAlert && Guard->Participant->Health > 0.f)
	{
		bool bFiled = false;
		for (const auto& Incident : Guard->Knowledge->KnownThefts)
			if (!Guard->Knowledge->EvidenceSources.Contains(Incident.IncidentId)) bFiled |= IssueBounty(Incident, Guard, Guard);
		return bFiled;
	}
	return Controller && Controller->IsGuard() && Communicate(Witness, Guard);
}

bool UPropertyLifeSubsystem::IssueBounty(const FTheftIncident& Incident, APropertyWitness* Witness, APropertyWitness* Guard)
{
	if (bRestoring || !Guard->HasAuthority() || Incident.Offender.PersonId.IsNone() || Witness->Identity->Identity.PersonId.IsNone()
		|| Guard->Identity->Identity.PersonId.IsNone() || IsSettled(Incident.IncidentId)
		|| Bounties.ContainsByPredicate([&](const FPropertyBounty& Entry) { return Entry.Incident.IncidentId == Incident.IncidentId; })) return false;
	if (!GetWorld()->GetSubsystem<UCrimeWorldSubsystem>()->Incidents.ContainsByPredicate([&](const FTheftIncident& Entry)
		{ return Entry.IncidentId == Incident.IncidentId && Entry.Item.ItemId == Incident.Item.ItemId && Entry.Offender.PersonId == Incident.Offender.PersonId; })) return false;
	FPropertyBounty Bounty;
	Bounty.Incident = Incident;
	Bounty.Reporter = Witness->Identity->Identity.PersonId;
	Bounty.ReceivingGuard = Guard->Identity->Identity.PersonId;
	Bounties.Add(Bounty);
	return true;
}

int32 UPropertyLifeSubsystem::GetBountyAmount(FName PersonId) const
{
	int32 TotalBounty = 0;
	for (const auto& Entry : Bounties)
		if (Entry.Incident.Offender.PersonId == PersonId && !IsSettled(Entry.Incident.IncidentId)) TotalBounty += Entry.Amount;
	return TotalBounty;
}

TArray<FTheftIncident> UPropertyLifeSubsystem::GetBountyCases(FName PersonId) const
{
	TArray<FTheftIncident> TheftCases;
	for (const auto& Entry : Bounties)
		if (Entry.Incident.Offender.PersonId == PersonId && !IsSettled(Entry.Incident.IncidentId)) TheftCases.Add(Entry.Incident);
	return TheftCases;
}

bool UPropertyLifeSubsystem::Communicate(APropertyWitness* Witness, APropertyWitness* Listener)
{
	if (!IsValid(Witness) || !IsValid(Listener) || Witness == Listener || Witness->GetWorld() != GetWorld() || Listener->GetWorld() != GetWorld()) return false;
	const auto* Sender = Cast<APropertyAIController>(Witness->GetController());
	auto* Receiver = Cast<APropertyAIController>(Listener->GetController());
	if (!Sender || !Receiver || !Listener->Knowledge->bAlert || Listener->Participant->Health <= 0.f || Witness->Participant->Health <= 0.f) return false;
	if (Witness->Identity->Identity.PersonId.IsNone() || FVector::DistSquared(Witness->GetActorLocation(), Listener->GetActorLocation()) > FMath::Square(1000.f) || !Witness->Knowledge->CanSeeActor(Listener)) return false;
	bool bShared = false;
	const bool bGuardRelay = Sender->IsGuard() && Receiver->IsGuard();
	for (const auto& TheftIncident : Witness->Knowledge->KnownThefts)
	{
		if (!Sender->CanEnforceAt(TheftIncident.Location) || !Receiver->CanEnforceAt(TheftIncident.Location)) continue;
		const FName* OriginalWitness = Witness->Knowledge->EvidenceSources.Find(TheftIncident.IncidentId);
		if (IsSettled(TheftIncident.IncidentId) || (OriginalWitness && !bGuardRelay)) continue;
		if (Receiver->IsGuard() && !OriginalWitness) bShared |= IssueBounty(TheftIncident, Witness, Listener);
		if (Listener->Knowledge->KnownThefts.ContainsByPredicate([&](const FTheftIncident& Existing) { return Existing.IncidentId == TheftIncident.IncidentId; })) continue;
		Listener->Knowledge->KnownThefts.Add(TheftIncident);
		Listener->Knowledge->EvidenceSources.Add(TheftIncident.IncidentId, OriginalWitness ? *OriginalWitness : Witness->Identity->Identity.PersonId);
		bShared = true;
	}
	if (bGuardRelay)
	{
		for (const auto& Bounty : Bounties)
		{
			const auto& Incident = Bounty.Incident;
			if (IsSettled(Incident.IncidentId) || Listener->Knowledge->KnownThefts.ContainsByPredicate([&](const FTheftIncident& Existing) { return Existing.IncidentId == Incident.IncidentId; })) continue;
			Listener->Knowledge->KnownThefts.Add(Incident);
			Listener->Knowledge->EvidenceSources.Add(Incident.IncidentId, Bounty.Reporter);
			bShared = true;
		}
	}
    bool bMissingShared = false;
    for (const auto& Report : Witness->Inspection->Reports)
    {
        if ((Report.Source != Witness->Identity->Identity.PersonId && !bGuardRelay) || Report.Age > 90.f || IsMissingItemResolved(Report.ItemId) || !Sender->CanEnforceAt(Report.Location) || !Receiver->CanEnforceAt(Report.Location)) continue;
        if (Listener->Inspection->Reports.ContainsByPredicate([&](const FPropertyMissingReport& Existing) { return Existing.ItemId == Report.ItemId; })) continue;
        Listener->Inspection->Reports.Add(Report);
        auto* Area = GetWorld()->GetSubsystem<UPropertyWorldSubsystem>()->ResolveArea(Report.Location);
        if (Area && Receiver->IsGuard()) Area->RegisterGuardReport(Report.ItemId);
        Receiver->AssignSearch(Area, Report.Location);
        bMissingShared = true;
    }
    if (bMissingShared)
    {
        Witness->SayCue(EPropertyDialogueCue::RequestSearch);
        Listener->SayCue(Receiver->SearchRole == EPropertySearchRole::WatchExit ? EPropertyDialogueCue::AcceptExitWatch : EPropertyDialogueCue::AcceptSearch);
    }
	if (bShared)
	{
		Witness->SayCue(EPropertyDialogueCue::ReportTheft);
		Listener->SayCue(EPropertyDialogueCue::AcceptReport);
		Receiver->RespondToCrimeReport();
	}
	return bShared || bMissingShared;
}

int32 UPropertyLifeSubsystem::EmitNoise(AActor* Source, FVector Location, float Radius, EPropertyNoiseKind Kind, USoundBase* Sound)
{
	if (bRestoring || Location.ContainsNaN() || !FMath::IsFinite(Radius) || Radius <= 0.f || (Source && Source->GetWorld() != GetWorld())) return 0;
	Radius = FMath::Clamp(Radius, 1.f, 5000.f);
	const auto* Areas = GetWorld()->GetSubsystem<UPropertyWorldSubsystem>();
	const auto* NoiseArea = Areas->ResolveArea(Location);
	const auto* SourcePawn = Cast<APawn>(Source);
	const bool bPlayerNoise = SourcePawn && SourcePawn->IsPlayerControlled();
	const auto* PlayerArea = bPlayerNoise ? Areas->ResolveArea(SourcePawn->GetActorLocation()) : nullptr;
	if (bPlayerNoise && (!PlayerArea || PlayerArea->CanEnter(UPropertyIdentityComponent::GetIdentity(Source)))) return 0;
	int32 ListenerCount = 0;
	for (TActorIterator<APropertyWitness> ActorIterator(GetWorld()); ActorIterator; ++ActorIterator)
	{
		if (*ActorIterator == Source || !ActorIterator->Knowledge->bAlert || ActorIterator->Participant->Health <= 0.f) continue;
		const auto* Controller = Cast<APropertyAIController>(ActorIterator->GetController());
		if (!Controller || !Controller->CanEnforceArea(NoiseArea)) continue;
		if (bPlayerNoise && !Controller->CanEnforceArea(PlayerArea)) continue;
		FVector EarLocation;
		FRotator ListenerRotation;
		ActorIterator->GetActorEyesViewPoint(EarLocation, ListenerRotation);
		float HearingRadius = FMath::Min(Radius, FMath::Max(0.f, ActorIterator->Hearing->HearingRange)) * FMath::Clamp(ActorIterator->Hearing->HearingDistanceMultiplier, .1f, 4.f);
		const double DistanceSquared = FVector::DistSquared(EarLocation, Location);
		if (HearingRadius <= 0.f || DistanceSquared >= FMath::Square(HearingRadius)) continue;
		FCollisionQueryParams SoundTraceParams(SCENE_QUERY_STAT(PropertyHearing), false, *ActorIterator);
		if (Source) SoundTraceParams.AddIgnoredActor(Source);
		FHitResult SoundHit;
		const bool bObstructed = GetWorld()->LineTraceSingleByChannel(SoundHit, EarLocation, Location, ECC_Visibility, SoundTraceParams);
		if (bObstructed) HearingRadius *= FMath::Clamp(ActorIterator->Hearing->OccludedMultiplier, 0.f, 1.f);
		if (HearingRadius <= 0.f || DistanceSquared >= FMath::Square(HearingRadius)) continue;
		ActorIterator->Hearing->Hear(Location, 1.f - FMath::Sqrt(DistanceSquared) / HearingRadius, Kind);
		++ListenerCount;
	}
	return ListenerCount;
}

void UPropertyLifeSubsystem::EmitDisturbance(FVector Location, float Loudness)
{
	EmitNoise(nullptr, Location, Loudness, EPropertyNoiseKind::Distraction);
}

bool UPropertyLifeSubsystem::SaveProgress(const FString& Slot)
{
	if (bRestoring || Slot.IsEmpty() || Slot.Contains(TEXT("/")) || Slot.Contains(TEXT("\\"))) return false;
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* Player = PlayerController ? PlayerController->GetPawn() : nullptr;
	auto* Inventory = Player ? Player->FindComponentByClass<UPropertyInteractionComponent>() : nullptr;
	auto* Participant = Player ? Player->FindComponentByClass<UPropertyParticipantComponent>() : nullptr;
	if (!Inventory || !Participant || Inventory->bCommitting) return false;
	auto* SavedState = Cast<UPropertySaveGame>(UGameplayStatics::CreateSaveGameObject(UPropertySaveGame::StaticClass()));
	SavedState->Map = UGameplayStatics::GetCurrentLevelName(this, true);
	SavedState->PlayerTransform = Player->GetActorTransform();
	if (const auto* Character = Cast<ACharacter>(Player)) SavedState->bPlayerCrouched = Character->bIsCrouched;
	SavedState->Inventory = Inventory->Inventory;
	SavedState->Gold = Participant->Gold;
	SavedState->Health = Participant->Health;
	SavedState->Hour = Hour;
	SavedState->Settled = Settled;
	SavedState->Bounties = Bounties;
	SavedState->ReturnedGoods = ReturnedGoods;
	SavedState->Incidents = GetWorld()->GetSubsystem<UCrimeWorldSubsystem>()->Incidents;
	TSet<FGuid> ItemIds;
	for (const auto& Item : SavedState->Inventory)
	{
		if (!Item.ItemId.IsValid() || ItemIds.Contains(Item.ItemId)) return false;
		ItemIds.Add(Item.ItemId);
	}
	for (TActorIterator<AOwnedPickup> ActorIterator(GetWorld()); ActorIterator; ++ActorIterator)
	{
		if (!ActorIterator->IsAvailable()) continue;
		if (!ActorIterator->ItemId.IsValid() || ItemIds.Contains(ActorIterator->ItemId)) return false;
		ItemIds.Add(ActorIterator->ItemId);
		FPropertySavedPickup Snapshot;
		Snapshot.ActorClass = FSoftClassPath(ActorIterator->GetClass());
		Snapshot.Transform = ActorIterator->GetActorTransform();
		Snapshot.Item.ItemId = ActorIterator->ItemId;
		Snapshot.Item.DisplayName = ActorIterator->DisplayName;
		Snapshot.Item.LegalOwner = ActorIterator->Ownership->GetLegalOwner();
		Snapshot.OwnershipMode = ActorIterator->Ownership->OwnershipMode;
		Snapshot.OwnerOverride = ActorIterator->Ownership->LegalOwner;
		Snapshot.Permission = ActorIterator->Ownership->TakePermission;
		SavedState->Pickups.Add(Snapshot);
	}
	TSet<FName> People;
	People.Add(UPropertyIdentityComponent::GetIdentity(Player).PersonId);
	for (TActorIterator<APropertyWitness> ActorIterator(GetWorld()); ActorIterator; ++ActorIterator)
	{
		const auto* Controller = Cast<APropertyAIController>(ActorIterator->GetController());
		if (!Controller) continue;
		FPropertySavedNPC Snapshot;
		Snapshot.PersonId = ActorIterator->Identity->Identity.PersonId;
		if (Snapshot.PersonId.IsNone() || People.Contains(Snapshot.PersonId)) return false;
		People.Add(Snapshot.PersonId);
		Snapshot.Transform = ActorIterator->GetActorTransform();
		Snapshot.Home = Controller->HomeLocation;
		Snapshot.HomeRotation = Controller->GetHomeRotation();
		Snapshot.Memories = ActorIterator->Knowledge->KnownThefts;
		Snapshot.Sources = ActorIterator->Knowledge->EvidenceSources;
		Snapshot.ObjectMemories = ActorIterator->Inspection->Memories;
		Snapshot.MissingReports = ActorIterator->Inspection->Reports;
		Snapshot.RoutineEntry = Controller->RoutineEntry;
		Snapshot.Waypoint = Controller->Waypoint;
		Snapshot.Health = ActorIterator->Participant->Health;
		Snapshot.RecoverySeconds = Controller->GetRemainingRecovery();
		SavedState->NPCs.Add(Snapshot);
	}
	for (TActorIterator<APropertyArea> ActorIterator(GetWorld()); ActorIterator; ++ActorIterator)
	{
		FPropertySavedArea Snapshot;
		Snapshot.ActorName = ActorIterator->GetFName();
		Snapshot.AlertScore = ActorIterator->AlertScore;
		Snapshot.QuietSeconds = ActorIterator->QuietSeconds;
		Snapshot.AlertedItems = ActorIterator->AlertedItems;
		Snapshot.ReportedItems = ActorIterator->ReportedItems;
		Snapshot.AssignedNPCs = ActorIterator->AssignedNPCs;
		Snapshot.Owner = ActorIterator->LegalOwner;
		Snapshot.Permission = ActorIterator->EntryPermission;
		SavedState->Areas.Add(Snapshot);
	}
	return UGameplayStatics::SaveGameToSlot(SavedState, Slot, 0);
}

bool UPropertyLifeSubsystem::LoadProgress(const FString& Slot)
{
	if (bRestoring || Slot.IsEmpty() || Slot.Contains(TEXT("/")) || Slot.Contains(TEXT("\\")) || !UGameplayStatics::DoesSaveGameExist(Slot, 0)) return false;
	auto* SavedState = Cast<UPropertySaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
	if (!SavedState || SavedState->Version < 2 || SavedState->Version > 4 || SavedState->Map != UGameplayStatics::GetCurrentLevelName(this, true)) return false;
	TSet<FGuid> BountyIds;
	for (const auto& Entry : SavedState->Bounties)
	{
		if (Entry.Amount != 100 || Entry.Reporter.IsNone() || Entry.ReceivingGuard.IsNone() || Entry.Incident.Offender.PersonId.IsNone()
			|| !Entry.Incident.IncidentId.IsValid() || BountyIds.Contains(Entry.Incident.IncidentId)
			|| !SavedState->Incidents.ContainsByPredicate([&](const FTheftIncident& Incident) { return Incident.IncidentId == Entry.Incident.IncidentId && Incident.Offender.PersonId == Entry.Incident.Offender.PersonId && Incident.Item.ItemId == Entry.Incident.Item.ItemId; })) return false;
		BountyIds.Add(Entry.Incident.IncidentId);
	}
	APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
	APawn* Player = PlayerController ? PlayerController->GetPawn() : nullptr;
	auto* Inventory = Player ? Player->FindComponentByClass<UPropertyInteractionComponent>() : nullptr;
	auto* Participant = Player ? Player->FindComponentByClass<UPropertyParticipantComponent>() : nullptr;
	if (!Inventory || !Participant || Inventory->bCommitting || SavedState->Inventory.Num() > Inventory->Capacity || SavedState->PlayerTransform.ContainsNaN()) return false;
	TMap<FName, APropertyWitness*> ResidentsByIdentity;
	for (TActorIterator<APropertyWitness> ActorIterator(GetWorld()); ActorIterator; ++ActorIterator)
	{
		if (!Cast<APropertyAIController>(ActorIterator->GetController())) continue;
		if (ResidentsByIdentity.Contains(ActorIterator->Identity->Identity.PersonId)) return false;
		ResidentsByIdentity.Add(ActorIterator->Identity->Identity.PersonId, *ActorIterator);
	}
	TSet<FName> SavedPeople;
	for (const auto& Snapshot : SavedState->NPCs)
	{
		if (!ResidentsByIdentity.Contains(Snapshot.PersonId) || SavedPeople.Contains(Snapshot.PersonId) || Snapshot.Transform.ContainsNaN()) return false;
		SavedPeople.Add(Snapshot.PersonId);
	}
	if (ResidentsByIdentity.Num() != SavedPeople.Num()) return false;
	TSet<FGuid> ItemIds;
	for (const auto& Item : SavedState->Inventory)
	{
		if (!Item.ItemId.IsValid() || ItemIds.Contains(Item.ItemId)) return false;
		ItemIds.Add(Item.ItemId);
	}
	for (const auto& Snapshot : SavedState->Pickups)
	{
		UClass* Class = Snapshot.ActorClass.TryLoadClass<AOwnedPickup>();
		if (!Class || !Class->IsChildOf(AOwnedPickup::StaticClass()) || Snapshot.Transform.ContainsNaN() || !Snapshot.Item.ItemId.IsValid() || ItemIds.Contains(Snapshot.Item.ItemId)) return false;
		ItemIds.Add(Snapshot.Item.ItemId);
	}
	TGuardValue<bool> Restoring(bRestoring, true);
	TGuardValue<bool> InventoryTransaction(Inventory->bCommitting, true);
	TArray<AOwnedPickup*> ExistingPickups;
	for (TActorIterator<AOwnedPickup> ActorIterator(GetWorld()); ActorIterator; ++ActorIterator) ExistingPickups.Add(*ActorIterator);
	TArray<AOwnedPickup*> RestoredPickups;
	for (const auto& Snapshot : SavedState->Pickups)
	{
		auto* Pickup = GetWorld()->SpawnActorDeferred<AOwnedPickup>(Snapshot.ActorClass.TryLoadClass<AOwnedPickup>(), Snapshot.Transform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
		if (!Pickup)
		{
			for (auto* Created : RestoredPickups) Created->Destroy();
			return false;
		}
		Pickup->ItemId = Snapshot.Item.ItemId;
		Pickup->DisplayName = Snapshot.Item.DisplayName;
		Pickup->Ownership->OwnershipMode = Snapshot.OwnershipMode;
		Pickup->Ownership->LegalOwner = SavedState->Version == 2 ? Snapshot.Item.LegalOwner : Snapshot.OwnerOverride;
		Pickup->Ownership->TakePermission = Snapshot.Permission;
		Pickup->FinishSpawning(Snapshot.Transform);
		RestoredPickups.Add(Pickup);
	}
	for (auto* Pickup : ExistingPickups) Pickup->Destroy();
	Inventory->Inventory = SavedState->Inventory;
	Participant->Restore(SavedState->Gold, SavedState->Health);
	Player->SetActorTransform(SavedState->PlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
	if (auto* Character = Cast<ACharacter>(Player))
	{
		auto* Movement = Character->GetCharacterMovement();
		Movement->StopMovementImmediately();
		Movement->bWantsToCrouch = SavedState->bPlayerCrouched;
		if (SavedState->bPlayerCrouched) Movement->Crouch(false);
		else Movement->UnCrouch(false);
		Character->SetActorTransform(SavedState->PlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
	}
	GetWorld()->GetSubsystem<UCrimeWorldSubsystem>()->Incidents = SavedState->Incidents;
	Settled = SavedState->Settled;
	Bounties = SavedState->Version >= 4 ? SavedState->Bounties : TArray<FPropertyBounty>();
	ReturnedGoods = SavedState->ReturnedGoods;
	SetHour(SavedState->Hour);
	Responders.Reset();
	for (const auto& Snapshot : SavedState->NPCs)
	{
		auto* Resident = ResidentsByIdentity.FindChecked(Snapshot.PersonId);
		Resident->Knowledge->KnownThefts = Snapshot.Memories;
		Resident->Knowledge->EvidenceSources = Snapshot.Sources;
		Resident->Knowledge->ObservedOffenders.Reset();
		Resident->Hearing->ResetHearing();
		Resident->Inspection->Memories = Snapshot.ObjectMemories;
		Resident->Inspection->Reports = Snapshot.MissingReports;
		Resident->Participant->Restore(Resident->Participant->Gold, Snapshot.Health);
		Resident->SetActorTransform(Snapshot.Transform, false, nullptr, ETeleportType::TeleportPhysics);
		Resident->GetCharacterMovement()->StopMovementImmediately();
		CastChecked<APropertyAIController>(Resident->GetController())->ResetAfterLoad(Snapshot);
	}
	for (TActorIterator<APropertyArea> ActorIterator(GetWorld()); ActorIterator; ++ActorIterator)
	{
		const auto* Snapshot = SavedState->Areas.FindByPredicate([&](const FPropertySavedArea& Entry) { return Entry.ActorName == ActorIterator->GetFName(); });
		if (!Snapshot) continue;
		ActorIterator->AssignedNPCs = Snapshot->AssignedNPCs;
		ActorIterator->AlertScore = FMath::Clamp(Snapshot->AlertScore, 0.f, 120.f);
		ActorIterator->QuietSeconds = FMath::Max(0.f, Snapshot->QuietSeconds);
		ActorIterator->AlertedItems = Snapshot->AlertedItems;
		ActorIterator->ReportedItems = Snapshot->ReportedItems;
		ActorIterator->LegalOwner = Snapshot->Owner;
		ActorIterator->EntryPermission = Snapshot->Permission;
	}
	return true;
}

bool UPropertyLifeSubsystem::IsMissingItemResolved(FGuid ItemId) const
{
    if (ReturnedGoods.ContainsByPredicate([&](const FOwnedItemRecord& Item) { return Item.ItemId == ItemId; })) return true;
    return GetWorld()->GetSubsystem<UCrimeWorldSubsystem>()->Incidents.ContainsByPredicate([&](const FTheftIncident& TheftIncident) { return TheftIncident.Item.ItemId == ItemId && IsSettled(TheftIncident.IncidentId); });
}

bool UPropertyLifeSubsystem::InspectPossessions(APropertyWitness* Inspector, AActor* Player)
{
    auto* InspectorController = IsValid(Inspector) ? Cast<APropertyAIController>(Inspector->GetController()) : nullptr;
    auto* Inventory = IsValid(Player) ? Player->FindComponentByClass<UPropertyInteractionComponent>() : nullptr;
    if (!InspectorController || !InspectorController->bQuestioning || InspectorController->GetSuspect() != Player || !Inventory || !Inspector->Knowledge->CanSeeActor(Player)) return false;
    bool bFound = false;
    auto* Crimes = GetWorld()->GetSubsystem<UCrimeWorldSubsystem>();
    for (const auto& Item : Inventory->GetInventory())
    {
        if (!Item.bStolen || IsMissingItemResolved(Item.ItemId)) continue;
        const auto* Report = Inspector->Inspection->Reports.FindByPredicate([&](const FPropertyMissingReport& Entry) { return Entry.ItemId == Item.ItemId && InspectorController->CanEnforceAt(Entry.Location); });
        if (!Report) continue;
        const auto* Original = Crimes->Incidents.FindByPredicate([&](const FTheftIncident& TheftIncident) { return TheftIncident.Item.ItemId == Item.ItemId; });
        FTheftIncident Evidence;
        if (Original) Evidence = *Original;
        else
        {
            Evidence.IncidentId = FGuid::NewGuid();
            Evidence.Item = Item;
            Evidence.Location = Report->Location;
            Evidence.WorldTime = GetWorld()->GetTimeSeconds();
            Evidence.Offender = UPropertyIdentityComponent::GetIdentity(Player);
            Crimes->Incidents.Add(Evidence);
        }
        Evidence.EvidenceKind = EPropertyEvidenceKind::RecoveredGoods;
        if (!Inspector->Knowledge->KnownThefts.ContainsByPredicate([&](const FTheftIncident& TheftIncident) { return TheftIncident.IncidentId == Evidence.IncidentId; })) Inspector->Knowledge->KnownThefts.Add(Evidence);
        Inspector->Knowledge->ObservedOffenders.Add(Evidence.IncidentId, Player);
        Inspector->Knowledge->EvidenceSources.Remove(Evidence.IncidentId);
        bFound = true;
    }
    return bFound;
}
