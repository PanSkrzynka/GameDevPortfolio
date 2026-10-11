#include "Property/AI/PropertyAIController.h"
#include "Property/Demo/PropertyWitness.h"
#include "Property/NPCKnowledgeComponent.h"
#include "Property/PropertyIdentityComponent.h"
#include "Property/PropertyArea.h"
#include "Property/PropertyWorldSubsystem.h"
#include "Components/StateTreeAIComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Property/PropertyLifeSubsystem.h"
#include "Property/PropertyInspectionComponent.h"
#include "Components/BoxComponent.h"
#include "Property/PropertyParticipantComponent.h"
#include "Property/PropertyInteractionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"
#include "Components/TextRenderComponent.h"

APropertyAIController::APropertyAIController()
{
	bStartAILogicOnPossess = false;
	bAttachToPawn = true;
	StateTree = CreateDefaultSubobject<UStateTreeAIComponent>(TEXT("StateTree"));
	StateTree->SetStartLogicAutomatically(false);
	SetPerceptionComponent(*CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("Perception")));
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->SetMaxAge(2.f);
	SightConfig->SightRadius = 1400.f;
	SightConfig->LoseSightRadius = 1600.f;
	SightConfig->PeripheralVisionAngleDegrees = 70.f;
	GetPerceptionComponent()->ConfigureSense(*SightConfig);
	GetPerceptionComponent()->SetDominantSense(UAISense_Sight::StaticClass());
}

void APropertyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	Resident = Cast<APropertyWitness>(InPawn);
	if (!Resident) return;
	Profile = Resident->ResponseProfile ? Resident->ResponseProfile.Get() : GetMutableDefault<UPropertyResponseProfile>();
	Resident->GetCharacterMovement()->RotationRate = FRotator(0.f, Profile->TurnRate, 0.f);
	HomeLocation = Resident->GetActorLocation();
	HomeRotation = Resident->GetActorRotation();
	DesiredResponse = EPropertyResponseState::Routine;
	SightConfig->SightRadius = Resident->Knowledge->SightRange;
	SightConfig->LoseSightRadius = SightConfig->SightRadius + 200.f;
	SightConfig->PeripheralVisionAngleDegrees = Resident->Knowledge->HalfVisionAngle;
	GetPerceptionComponent()->ConfigureSense(*SightConfig);
	GetPerceptionComponent()->SetDominantSense(UAISense_Sight::StaticClass());
	Resident->Knowledge->OnTheftWitnessed.AddDynamic(this, &APropertyAIController::HandleTheft);
	if (Resident->ResponseTree)
	{
		StateTree->SetStateTree(Resident->ResponseTree);
		StateTree->StartLogic();
		GetWorldTimerManager().SetTimer(AwarenessTimer, this, &APropertyAIController::EvaluateAwareness, .2f, true);
	}
}

void APropertyAIController::Cleanup()
{
	SearchArea.Reset();
	SearchRole = EPropertySearchRole::None;
	SearchSlot = INDEX_NONE;
	bQuestioning = false;
	if (GetWorld()) GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->ReleaseResponse(this);
	GetWorldTimerManager().ClearTimer(AwarenessTimer);
	StateTree->StopLogic(TEXT("Controller released"));
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	if (IsValid(Resident)) Resident->Knowledge->OnTheftWitnessed.RemoveDynamic(this, &APropertyAIController::HandleTheft);
	Suspect.Reset();
	Resident = nullptr;
}

void APropertyAIController::OnUnPossess()
{
	Cleanup();
	Super::OnUnPossess();
}

void APropertyAIController::EndPlay(const EEndPlayReason::Type Reason)
{
	Cleanup();
	Super::EndPlay(Reason);
}

bool APropertyAIController::HasVisibleSuspect() const
{
	return IsValid(Resident) && Suspect.IsValid() && Resident->Knowledge->CanSeeActor(Suspect.Get());
}

bool APropertyAIController::CanEnforceAt(FVector Location) const
{
	if (!IsValid(Resident)) return false;
	if (IsGuard()) return true;
	return CanEnforceArea(GetWorld()->GetSubsystem<UPropertyWorldSubsystem>()->ResolveArea(Location));
}

bool APropertyAIController::CanEnforceArea(const APropertyArea* Area) const
{
	return IsValid(Resident) && (IsGuard() || (IsValid(Area) && Area->IsAssigned(Resident->Identity->Identity.PersonId)));
}

bool APropertyAIController::IsProtectedTrespasser(AActor* Actor) const
{
	if (!IsValid(Actor) || !IsValid(Resident) || Actor == Resident) return false;
	const auto* VisitorPawn = Cast<APawn>(Actor);
	if (!VisitorPawn || !VisitorPawn->IsPlayerControlled()) return false;
	const auto* Area = GetWorld()->GetSubsystem<UPropertyWorldSubsystem>()->ResolveArea(Actor->GetActorLocation());
	if (!Area || Area->CanEnter(UPropertyIdentityComponent::GetIdentity(Actor))) return false;
	return CanEnforceArea(Area);
}

void APropertyAIController::EvaluateAwareness()
{
	if (!IsValid(Resident) || !Resident->Knowledge->bAlert || !Profile) return;
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < RecoveryEndsAt) return;
	if (IsGuard()) ShareEvidence(true);
	if (CurrentTime >= NextEvidenceReportAt)
	{
		ShareEvidence();
		NextEvidenceReportAt = CurrentTime + 2.f;
	}
	RespondToCrimeReport();
	if (Suspect.IsValid())
	{
		auto* LifeSubsystem = GetWorld()->GetSubsystem<UPropertyLifeSubsystem>();
		if (!LifeSubsystem->OwnsResponse(UPropertyIdentityComponent::GetIdentity(Suspect.Get()).PersonId, this))
		{
			RequestReturn();
			return;
		}
		if (bPursuingOffender && !HasActionableCases(UPropertyIdentityComponent::GetIdentity(Suspect.Get()).PersonId))
		{
			RequestReturn();
			return;
		}
		if (HasVisibleSuspect())
		{
			LastKnownLocation = Suspect->GetActorLocation();
			SuspectLastSeenAt = CurrentTime;
			if (DesiredResponse == EPropertyResponseState::Search)
			{
				if (bPursuingOffender || IsProtectedTrespasser(Suspect.Get())) DesiredResponse = EPropertyResponseState::Chase;
				else RequestReturn();
			}
		}
		return;
	}
	if (CurrentTime < IgnoreNewResponsesUntil || (DesiredResponse != EPropertyResponseState::Routine && !(bInvestigatingLocation && DesiredResponse == EPropertyResponseState::Search))) return;
	TArray<AActor*> Visible;
	GetPerceptionComponent()->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), Visible);
	for (AActor* Actor : Visible)
	{
		if (!Resident->Knowledge->CanSeeActor(Actor)) continue;
		const FName PersonId = UPropertyIdentityComponent::GetIdentity(Actor).PersonId;
		if (HasActionableCases(PersonId) && StartResponse(Actor, true)) break;
		if (TryQuestion(Actor)) break;
		if (IsProtectedTrespasser(Actor))
		{
			if (StartResponse(Actor, false)) break;
		}
	}
}

void APropertyAIController::HandleTheft(FTheftIncident Incident)
{
	if (!IsValid(Resident) || !Profile) return;
	if (!CanEnforceAt(Incident.Location)) return;
	AActor* Offender = Resident->Knowledge->GetObservedOffender(Incident.IncidentId);
	if (!IsValid(Offender)) return;
	if (auto* Area = GetWorld()->GetSubsystem<UPropertyWorldSubsystem>()->ResolveArea(Incident.Location)) Area->RegisterIncident(Incident.Item.ItemId, true);
	Resident->RaiseAlarm(Incident.Location);
	if (IsGuard()) GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->ReportToGuard(Resident, Resident);
	StartResponse(Offender, true);
}

void APropertyAIController::EnterResponse(EPropertyResponseState State)
{
	if (!IsValid(Resident) || !Profile) return;
	ActiveResponse = State;
	ResponseStartedAt = GetWorld()->GetTimeSeconds();
	ResponseUpdateElapsed = 0.f;
	NextMovementUpdateAt = 0.f;
	bPathRequestFailed = false;
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	if (GetWorld()->GetTimeSeconds() < RecoveryEndsAt) return;
	const float ResponseSpeed = Resident->SelectResponseSpeed(State, Profile->WalkSpeed, Profile->ChaseSpeed);
	Resident->GetCharacterMovement()->MaxWalkSpeed = FMath::IsFinite(ResponseSpeed) ? FMath::Clamp(ResponseSpeed, 1.f, 1200.f) : Profile->WalkSpeed;
	if (State == EPropertyResponseState::Routine)
	{
		Suspect.Reset();
		bResisting = false;
		bInvestigatingLocation = false;
		PatrolWaitUntil = 0.f;
        bShouldFaceHomeDirection = FVector::DistSquared(Resident->GetActorLocation(), HomeLocation) < FMath::Square(120.f);
        if (bShouldFaceHomeDirection) SetFocalPoint(Resident->GetActorLocation() + HomeRotation.Vector() * 1000.f, EAIFocusPriority::Gameplay);
        SetControlRotation(Resident->GetActorRotation());
	}
	else if (State == EPropertyResponseState::Search)
	{
		if (!bPursuingOffender && Suspect.IsValid()) PursuitStartedAt = ResponseStartedAt;
		SearchPointIndex = 0;
		SearchStartedAt = -1.f;
		SearchPauseEndsAt = -1.f;
		ScanBaseYaw = Resident->GetActorRotation().Yaw;
		ScanStartedAt = GetWorld()->GetTimeSeconds();
		MoveToSearchPoint();
	}
	else if (State == EPropertyResponseState::Return)
	{
		const auto Result = MoveToLocation(HomeLocation, 70.f, true, true, true, false, nullptr, false);
		bPathRequestFailed = Result == EPathFollowingRequestResult::Failed;
	}
	Resident->ReceiveResponseChanged(State);
}

void APropertyAIController::RequestReturn()
{
	SearchArea.Reset();
	SearchRole = EPropertySearchRole::None;
	SearchSlot = INDEX_NONE;
	bQuestioning = false;
	GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->ReleaseResponse(this);
	DesiredResponse = EPropertyResponseState::Return;
	IgnoreNewResponsesUntil = GetWorld()->GetTimeSeconds() + Profile->ReengagementDelay;
	Suspect.Reset();
	bPursuingOffender = false;
	bInvestigatingLocation = false;
	bResisting = false;
}

void APropertyAIController::MoveToSearchPoint()
{
	ClearFocus(EAIFocusPriority::Gameplay);
	SearchPauseEndsAt = -1.f;
	const FVector Offsets[] = {FVector::ZeroVector, FVector(1, 0, 0), FVector(0, 1, 0), FVector(-1, 0, 0), FVector(0, -1, 0)};
	FVector Candidate = LastKnownLocation + Offsets[SearchPointIndex % UE_ARRAY_COUNT(Offsets)] * Profile->SearchRadius;
    if (SearchArea.IsValid() && SearchRole != EPropertySearchRole::None)
    {
        const auto* Area = SearchArea.Get();
        if (SearchRole == EPropertySearchRole::WatchExit) Candidate = Area->GetExitLocation();
        else if (SearchRole == EPropertySearchRole::InspectRoom)
        {
            const FVector Extent = Area->Bounds->GetScaledBoxExtent() * .6f;
            const float Angle = SearchPointIndex * UE_PI * .5f;
            Candidate = Area->GetActorLocation() + FVector(FMath::Cos(Angle) * Extent.X, FMath::Sin(Angle) * Extent.Y, 0);
        }
        else
        {
            const float Angle = SearchSlot * 2.39996f + (SearchPointIndex % 2) * .35f;
            const float Radius = FMath::Max(Area->Bounds->GetScaledBoxExtent().X, Area->Bounds->GetScaledBoxExtent().Y) + 200.f;
            Candidate = Area->GetActorLocation() + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0);
        }
        Candidate.Z = Resident->GetActorLocation().Z;
        ScanBaseYaw = (Area->GetActorLocation() - Candidate).Rotation().Yaw;
        ScanStartedAt = GetWorld()->GetTimeSeconds();
    }
	++SearchPointIndex;
	auto* Navigation = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	FNavLocation Projected;
	bPathRequestFailed = true;
	if (Navigation && Navigation->ProjectPointToNavigation(Candidate, Projected, FVector(150, 150, 250)))
	{
		SearchDestination = Projected.Location;
		bPathRequestFailed = MoveToLocation(SearchDestination, 65.f, true, true, true, false, nullptr, false) == EPathFollowingRequestResult::Failed;
	}
	NextMovementUpdateAt = GetWorld()->GetTimeSeconds() + (SearchRole != EPropertySearchRole::None ? 4.f : 1.5f);
}

void APropertyAIController::AdvanceResponse(float DeltaTime)
{
	if (!IsValid(Resident) || !Profile || ActiveResponse != DesiredResponse) return;
	ResponseUpdateElapsed += DeltaTime;
	if (ResponseUpdateElapsed < .15f) return;
	ResponseUpdateElapsed = 0.f;
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const float TimeInState = CurrentTime - ResponseStartedAt;
	if (CurrentTime < RecoveryEndsAt) return;
	if (Resident->Participant->Health <= 0.f)
	{
		Resident->Participant->Recover();
		EnterResponse(EPropertyResponseState::Return);
	}
	if (bPursuingOffender && Suspect.IsValid()
		&& FVector::DistSquared(Resident->GetActorLocation(), Suspect->GetActorLocation()) > FMath::Square(FMath::Max(100.f, Profile->ChaseBreakOffDistance)))
	{
		StopMovement();
		ClearFocus(EAIFocusPriority::Gameplay);
		RequestReturn();
		return;
	}
	if (!bPursuingOffender && !bQuestioning && !bResisting
		&& (ActiveResponse == EPropertyResponseState::Warn || ActiveResponse == EPropertyResponseState::Chase || ActiveResponse == EPropertyResponseState::Confront))
	{
		if (HasVisibleSuspect() || !IsProtectedTrespasser(Suspect.Get())) AdvanceTrespassFollow(CurrentTime, TimeInState);
		else
		{
			ClearFocus(EAIFocusPriority::Gameplay);
			DesiredResponse = EPropertyResponseState::Search;
		}
		return;
	}
	if ((ActiveResponse == EPropertyResponseState::Chase || ActiveResponse == EPropertyResponseState::Search)
		&& ((!Suspect.IsValid() && !bInvestigatingLocation) || CurrentTime - PursuitStartedAt > Profile->MaxPursuitSeconds
			|| FVector::DistSquared(Resident->GetActorLocation(), HomeLocation) > FMath::Square(Profile->LeashDistance)))
	{
		RequestReturn();
		return;
	}
	const bool bVisible = HasVisibleSuspect();
	switch (ActiveResponse)
	{
	case EPropertyResponseState::Routine:
		AdvanceRoutine(CurrentTime);
		break;
	case EPropertyResponseState::Warn:
		if (!bVisible)
		{
			if (CurrentTime - SuspectLastSeenAt > Profile->LostSightSeconds) RequestReturn();
		}
		else if (!IsProtectedTrespasser(Suspect.Get())) RequestReturn();
		else
		{
			SetFocus(Suspect.Get());
			AdvanceWarning(CurrentTime, TimeInState);
		}
		break;
	case EPropertyResponseState::Chase:
		if (!bVisible)
		{
			ClearFocus(EAIFocusPriority::Gameplay);
			if (CurrentTime >= NextMovementUpdateAt)
			{
				bPathRequestFailed = MoveToLocation(LastKnownLocation, 70.f, true, true, true, false, nullptr, false) == EPathFollowingRequestResult::Failed;
				NextMovementUpdateAt = CurrentTime + .5f;
			}
			if (CurrentTime - SuspectLastSeenAt >= Profile->LostSightSeconds) DesiredResponse = EPropertyResponseState::Search;
		}
		else if (!bPursuingOffender && !IsProtectedTrespasser(Suspect.Get())) RequestReturn();
		else
		{
			SetFocus(Suspect.Get());
			if (FVector::DistSquared(Resident->GetActorLocation(), Suspect->GetActorLocation()) <= FMath::Square(Profile->ConfrontDistance))
			{
				DesiredResponse = EPropertyResponseState::Confront;
			}
			else if (CurrentTime >= NextMovementUpdateAt)
			{
				const auto Result = MoveToLocation(LastKnownLocation, 70.f, true, true, true, false, nullptr, false);
				bPathRequestFailed = Result == EPathFollowingRequestResult::Failed;
				NextMovementUpdateAt = CurrentTime + .5f;
			}
		}
		if (bPathRequestFailed && DesiredResponse == EPropertyResponseState::Chase) DesiredResponse = EPropertyResponseState::Search;
		break;
	case EPropertyResponseState::Search:
		if (SearchRole != EPropertySearchRole::None)
		{
			if (TimeInState >= Profile->SearchSeconds * 2.f) RequestReturn();
			else if (CurrentTime >= NextMovementUpdateAt) MoveToSearchPoint();
		}
		else
		{
			if (SearchStartedAt >= 0.f && CurrentTime - SearchStartedAt >= Profile->SearchSeconds) RequestReturn();
			else if (GetMoveStatus() != EPathFollowingStatus::Moving)
			{
				if (SearchStartedAt < 0.f) SearchStartedAt = CurrentTime;
				if (SearchPauseEndsAt < 0.f)
				{
					SearchPauseEndsAt = CurrentTime + FMath::Max(.1f, Profile->SearchPointPauseSeconds);
					ScanBaseYaw = Resident->GetActorRotation().Yaw;
					ScanStartedAt = CurrentTime;
				}
				else if (CurrentTime >= SearchPauseEndsAt) MoveToSearchPoint();
			}
		}
		break;
	case EPropertyResponseState::Confront:
        if (bQuestioning)
        {
            if (!bVisible || TimeInState > 20.f || FVector::DistSquared(Resident->GetActorLocation(), Suspect->GetActorLocation()) > FMath::Square(400.f)) RequestReturn();
            else SetFocus(Suspect.Get());
            break;
        }
		if (!bVisible) DesiredResponse = EPropertyResponseState::Search;
		else if (FVector::DistSquared(Resident->GetActorLocation(), Suspect->GetActorLocation()) > FMath::Square(Profile->ConfrontDistance * 1.5f)) DesiredResponse = EPropertyResponseState::Chase;
		else
		{
			SetFocus(Suspect.Get());
			if (bResisting && CurrentTime >= NextAttackAt)
			{
				NextAttackAt = CurrentTime + 1.f;
				Resident->PlayAttackAnimation();
				UGameplayStatics::ApplyDamage(Suspect.Get(), 15.f, this, Resident, UDamageType::StaticClass());
			}
			else if (!bPursuingOffender && !bResisting && TimeInState >= Profile->ConfrontSeconds) RequestReturn();
		}
		break;
	case EPropertyResponseState::Return:
		if (FVector::DistSquared(Resident->GetActorLocation(), HomeLocation) < FMath::Square(120.f) || bPathRequestFailed || TimeInState >= 15.f) DesiredResponse = EPropertyResponseState::Routine;
		break;
	default:
		break;
	}
}

void APropertyAIController::AdvanceWarning(float CurrentTime, float TimeInState)
{
	const FName PersonId = UPropertyIdentityComponent::GetIdentity(Suspect.Get()).PersonId;
	const int32 PriorOffences = Resident->Knowledge->CountKnownThefts(PersonId);
	if (TimeInState < Profile->WarningSeconds / (1.f + .5f * PriorOffences)) return;
	PursuitStartedAt = CurrentTime;
	DesiredResponse = EPropertyResponseState::Chase;
}

void APropertyAIController::AdvanceTrespassFollow(float CurrentTime, float TimeInState)
{
	if (!IsProtectedTrespasser(Suspect.Get()))
	{
		StopMovement();
		ClearFocus(EAIFocusPriority::Gameplay);
		RequestReturn();
		return;
	}
	SetFocus(Suspect.Get());
	const float DistanceSquared = FVector::DistSquared(Resident->GetActorLocation(), Suspect->GetActorLocation());
	if (DistanceSquared <= FMath::Square(Profile->ConfrontDistance)) StopMovement();
	else if (CurrentTime >= NextMovementUpdateAt)
	{
		MoveToActor(Suspect.Get(), FMath::Max(10.f, Profile->ConfrontDistance * .5f), true, true, true, nullptr, false);
		NextMovementUpdateAt = CurrentTime + .5f;
	}
	if (ActiveResponse == EPropertyResponseState::Warn) AdvanceWarning(CurrentTime, TimeInState);
	else if (ActiveResponse == EPropertyResponseState::Chase && DistanceSquared <= FMath::Square(Profile->ConfrontDistance)) DesiredResponse = EPropertyResponseState::Confront;
	else if (ActiveResponse == EPropertyResponseState::Confront && DistanceSquared > FMath::Square(Profile->ConfrontDistance * 1.5f)) DesiredResponse = EPropertyResponseState::Chase;
}

void APropertyAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);
	if (Result.Code == EPathFollowingResult::Blocked || Result.Code == EPathFollowingResult::OffPath || Result.Code == EPathFollowingResult::Invalid) bPathRequestFailed = true;
}

bool APropertyAIController::IsGuard() const
{
	return Profile && Profile->bProtectAllProperty;
}

bool APropertyAIController::StartResponse(AActor* Actor, bool bCrime)
{
	if (!IsValid(Actor) || !IsValid(Resident) || GetWorld()->GetTimeSeconds() < RecoveryEndsAt) return false;
	if (bCrime && Profile && FVector::DistSquared(Resident->GetActorLocation(), Actor->GetActorLocation()) > FMath::Square(FMath::Max(100.f, Profile->ChaseBreakOffDistance))) return false;
	const auto* TargetPawn = Cast<APawn>(Actor);
	if (!TargetPawn || !TargetPawn->IsPlayerControlled()) return false;
	const FName PersonId = UPropertyIdentityComponent::GetIdentity(Actor).PersonId;
	if (!GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->ClaimResponse(PersonId, this)) return false;
	SearchArea.Reset();
	SearchRole = EPropertySearchRole::None;
	SearchSlot = INDEX_NONE;
	bQuestioning = false;
	Suspect = Actor;
	LastKnownLocation = Actor->GetActorLocation();
	SuspectLastSeenAt = GetWorld()->GetTimeSeconds();
	PursuitStartedAt = SuspectLastSeenAt;
	bPursuingOffender = bCrime;
	bInvestigatingLocation = false;
	bResisting = false;
	DesiredResponse = bCrime ? EPropertyResponseState::Chase : EPropertyResponseState::Warn;
	return true;
}

int32 APropertyAIController::GetCompensation() const
{
	if (!Resident || !Suspect.IsValid()) return 0;
	const FName PersonId = UPropertyIdentityComponent::GetIdentity(Suspect.Get()).PersonId;
	const int32 UnsettledTheftCount = CountActionableCases(PersonId);
	if (UnsettledTheftCount == 0) return 0;
	if (IsGuard()) return UnsettledTheftCount * 100;
	const int32 KnownTheftCount = Resident->Knowledge->CountKnownThefts(PersonId);
	return UnsettledTheftCount * 100 + FMath::Max(0, KnownTheftCount - 1) * 50;
}

bool APropertyAIController::ResolveConfrontation(AActor* Player, EConfrontationChoice Choice)
{
	if (Choice > EConfrontationChoice::Submit) return false;
	if (!IsValid(Player) || Player != Suspect.Get() || !HasAuthority() || !Resident || !HasVisibleSuspect()
		|| ActiveResponse != EPropertyResponseState::Confront || DesiredResponse != EPropertyResponseState::Confront
		|| FVector::DistSquared(Resident->GetActorLocation(), Player->GetActorLocation()) > FMath::Square(Profile->ConfrontDistance * 1.5f)) return false;
	if (!GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->OwnsResponse(UPropertyIdentityComponent::GetIdentity(Player).PersonId, this)) return false;
	auto* Participant = Player->FindComponentByClass<UPropertyParticipantComponent>();
	auto* Inventory = Player->FindComponentByClass<UPropertyInteractionComponent>();
	if (!Participant || !Inventory || Inventory->bCommitting) return false;
	if (IsEscortingTrespasser()) return false;
    if (bQuestioning)
    {
        if (Choice == EConfrontationChoice::Submit) return false;
        NextQuestionAt = GetWorld()->GetTimeSeconds() + 60.f;
        if (Choice == EConfrontationChoice::PayCompensation && GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->InspectPossessions(Resident, Player))
        {
            Resident->Say(Resident->GetDialogue(EPropertyDialogueCue::GoodsFound));
            bQuestioning = false;
            bPursuingOffender = true;
            Resident->ReceiveResponseChanged(EPropertyResponseState::Confront);
            Participant->Notify(Resident->GetDialogue(EPropertyDialogueCue::GoodsFoundFeedback).ToString());
            return true;
        }
        const FName Person = UPropertyIdentityComponent::GetIdentity(Player).PersonId;
        for (auto& Report : Resident->Inspection->Reports) if (Report.PossiblePerson == Person) Report.PossiblePerson = NAME_None;
        Resident->SayCue(Choice == EConfrontationChoice::Resist ? EPropertyDialogueCue::DeclineInspection : EPropertyDialogueCue::ReleaseSuspect);
        Participant->Notify(Resident->GetDialogue(Choice == EConfrontationChoice::PayCompensation ? EPropertyDialogueCue::CleanInspectionFeedback : EPropertyDialogueCue::QuestioningEndedFeedback).ToString());
        RequestReturn();
        return true;
    }
	TGuardValue<bool> Transaction(Inventory->bCommitting, true);
	if (Choice == EConfrontationChoice::Resist)
	{
		if (bResisting) return false;
		bResisting = true;
		NextAttackAt = GetWorld()->GetTimeSeconds() + 1.f;
		Participant->Notify(Resident->GetDialogue(EPropertyDialogueCue::ResistFeedback).ToString());
		return true;
	}
	const auto TheftCases = GetActionableCases(UPropertyIdentityComponent::GetIdentity(Player).PersonId);
	TSet<FGuid> StolenItemIds;
	for (const auto& TheftIncident : TheftCases) StolenItemIds.Add(TheftIncident.Item.ItemId);
	if (Choice == EConfrontationChoice::ReturnGoods)
	{
		for (const FGuid& ItemId : StolenItemIds)
		{
			if (!Inventory->Inventory.ContainsByPredicate([&](const FOwnedItemRecord& Item) { return Item.ItemId == ItemId; }))
			{
				Participant->Notify(Resident->GetDialogue(EPropertyDialogueCue::MissingGoodsFeedback).ToString());
				return false;
			}
		}
	}
	const int32 CompensationCost = GetCompensation();
	if (Choice == EConfrontationChoice::PayCompensation && !Participant->Spend(CompensationCost))
	{
		Participant->Notify(FText::Format(Resident->GetDialogue(EPropertyDialogueCue::CannotPayFeedback), FFormatNamedArguments{{TEXT("Amount"), FText::AsNumber(CompensationCost)}}).ToString());
		return false;
	}
	auto* LifeSubsystem = GetWorld()->GetSubsystem<UPropertyLifeSubsystem>();
	if (Choice == EConfrontationChoice::PayCompensation)
	{
		for (auto& Item : Inventory->Inventory)
		{
			if (StolenItemIds.Contains(Item.ItemId))
			{
				Item.bStolen = false;
				Item.LegalOwner = UPropertyIdentityComponent::GetIdentity(Player);
			}
		}
	}
	else
	{
		for (const auto& Item : Inventory->Inventory)
		{
			if (StolenItemIds.Contains(Item.ItemId))
			{
				auto Returned = Item;
				Returned.bStolen = false;
				LifeSubsystem->ReturnedGoods.Add(Returned);
			}
		}
		Inventory->Inventory.RemoveAll([&](const FOwnedItemRecord& Item) { return StolenItemIds.Contains(Item.ItemId); });
		if (Choice == EConfrontationChoice::Submit) Participant->Spend(FMath::Min(Participant->Gold, CompensationCost));
	}
	LifeSubsystem->Settle(TheftCases);
	Participant->Notify(Resident->GetDialogue(Choice == EConfrontationChoice::PayCompensation ? EPropertyDialogueCue::PaidFeedback : EPropertyDialogueCue::ReturnedFeedback).ToString());
	RequestReturn();
	return true;
}

void APropertyAIController::TargetDefeated()
{
	AActor* Target = Suspect.Get();
	if (ResolveConfrontation(Target, EConfrontationChoice::Submit))
	{
		if (auto* Participant = Target->FindComponentByClass<UPropertyParticipantComponent>())
		{
			Participant->Recover();
			Participant->Notify(Resident->GetDialogue(EPropertyDialogueCue::DefeatedFeedback).ToString());
		}
	}
}

void APropertyAIController::HandleDefeat()
{
	RequestReturn();
	StopMovement();
	RecoveryEndsAt = GetWorld()->GetTimeSeconds() + 15.f;
	IgnoreNewResponsesUntil = RecoveryEndsAt + Profile->ReengagementDelay;
	Resident->StatusLabel->SetText(Resident->GetDialogue(EPropertyDialogueCue::RecoveringStatus));
}

void APropertyAIController::RespondToCrimeReport()
{
	if (!Resident || !Profile || !Resident->Knowledge->bAlert || Resident->Participant->Health <= 0.f) return;
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < RecoveryEndsAt || CurrentTime < IgnoreNewResponsesUntil || (Suspect.IsValid() && bPursuingOffender)) return;
	for (TActorIterator<APawn> PlayerIterator(GetWorld()); PlayerIterator; ++PlayerIterator)
	{
		if (!PlayerIterator->IsPlayerControlled() || !Resident->Knowledge->CanSeeActor(*PlayerIterator)) continue;
		if (HasActionableCases(UPropertyIdentityComponent::GetIdentity(*PlayerIterator).PersonId) && StartResponse(*PlayerIterator, true)) return;
	}
}

void APropertyAIController::ShareEvidence(bool bGuardsOnly)
{
	auto* LifeSubsystem = GetWorld()->GetSubsystem<UPropertyLifeSubsystem>();
	if (Resident->Knowledge->GetKnownTheftsView().IsEmpty() && Resident->Inspection->Reports.IsEmpty() && (!IsGuard() || LifeSubsystem->Bounties.IsEmpty())) return;
	for (TActorIterator<APropertyWitness> ActorIterator(GetWorld()); ActorIterator; ++ActorIterator)
	{
		if (*ActorIterator == Resident) continue;
		const auto* ListenerController = Cast<APropertyAIController>(ActorIterator->GetController());
		if (bGuardsOnly && (!ListenerController || !ListenerController->IsGuard())) continue;
		LifeSubsystem->Communicate(Resident, *ActorIterator);
	}
}

bool APropertyAIController::Investigate(FVector Location, bool bAlarm)
{
    const float CurrentTime = GetWorld()->GetTimeSeconds();
    if (!Resident || !Profile || Location.ContainsNaN() || !CanEnforceAt(Location) || !Resident->Knowledge->bAlert || Resident->Participant->Health <= 0.f || CurrentTime < RecoveryEndsAt || SearchRole != EPropertySearchRole::None) return false;
    if (!bAlarm && CurrentTime < NextNoiseInvestigationAt) return false;
    if (bInvestigatingLocation && DesiredResponse == EPropertyResponseState::Search && !Suspect.IsValid())
    {
        if (!bAlarm && FVector::DistSquared(LastKnownLocation, Location) < FMath::Square(100.f)) return false;
        LastKnownLocation = Location;
        SearchPointIndex = 0;
        ResponseStartedAt = CurrentTime;
        ScanBaseYaw = Resident->GetActorRotation().Yaw;
        ScanStartedAt = CurrentTime;
        NextNoiseInvestigationAt = CurrentTime + Profile->NoiseRetargetCooldown;
        MoveToSearchPoint();
        return true;
    }
    if (DesiredResponse != EPropertyResponseState::Routine) return false;
    bInvestigatingLocation = true;
    bPursuingOffender = false;
    LastKnownLocation = Location;
    PursuitStartedAt = CurrentTime;
    NextNoiseInvestigationAt = CurrentTime + Profile->NoiseRetargetCooldown;
    DesiredResponse = EPropertyResponseState::Search;
    return true;
}

void APropertyAIController::AdvanceRoutine(float CurrentTime)
{
	if (!Resident->Routine) return;
	const float Hour = GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->Hour;
	int32 SelectedRoutineIndex = INDEX_NONE;
	for (int32 RoutineIndex = 0; RoutineIndex < Resident->Routine->Entries.Num(); ++RoutineIndex)
	{
		const auto& RoutineStep = Resident->Routine->Entries[RoutineIndex];
		const bool bActive = RoutineStep.StartHour <= RoutineStep.EndHour ? Hour >= RoutineStep.StartHour && Hour < RoutineStep.EndHour : Hour >= RoutineStep.StartHour || Hour < RoutineStep.EndHour;
		if (bActive) { SelectedRoutineIndex = RoutineIndex; break; }
	}
	if (SelectedRoutineIndex != RoutineEntry)
	{
		ClearFocus(EAIFocusPriority::Gameplay);
		RoutineEntry = SelectedRoutineIndex;
		Waypoint = 0;
		PatrolWaitUntil = 0.f;
		StopMovement();
	}
	if (!Resident->Routine->Entries.IsValidIndex(RoutineEntry)) return;
	const auto& RoutineStep = Resident->Routine->Entries[RoutineEntry];
	CurrentActivity = RoutineStep.Activity;
    const auto* PatrolArea = GetWorld()->GetSubsystem<UPropertyWorldSubsystem>()->ResolveArea(Resident->GetActorLocation());
    const bool bHeightened = PatrolArea && PatrolArea->GetAlertLevel() >= EPropertyAlertLevel::Searching && CanEnforceAt(Resident->GetActorLocation());
    Resident->GetCharacterMovement()->MaxWalkSpeed = Profile->WalkSpeed * (bHeightened ? 1.25f : 1.f);
	if (RoutineStep.Offsets.IsEmpty()) return;
	Waypoint = FMath::Clamp(Waypoint, 0, RoutineStep.Offsets.Num() - 1);
	if (PatrolWaitUntil > 0.f)
	{
		if (CurrentTime < PatrolWaitUntil)
		{
			LookAround(CurrentTime);
			return;
		}
		Waypoint = (Waypoint + 1) % RoutineStep.Offsets.Num();
		PatrolWaitUntil = 0.f;
		ClearFocus(EAIFocusPriority::Gameplay);
	}
	const FVector Destination = HomeLocation + RoutineStep.Offsets[Waypoint];
	if (FVector::DistSquared(Resident->GetActorLocation(), Destination) < FMath::Square(120.f))
	{
		StopMovement();
		ScanBaseYaw = bShouldFaceHomeDirection ? HomeRotation.Yaw : Resident->GetActorRotation().Yaw;
		bShouldFaceHomeDirection = false;
		ScanStartedAt = CurrentTime;
		PatrolWaitUntil = CurrentTime + FMath::Max(.1f, RoutineStep.WaitSeconds * (bHeightened ? .5f : 1.f));
		return;
	}
	if (CurrentTime >= NextMovementUpdateAt)
	{
		const auto Result = MoveToLocation(Destination, 65.f, true, true, true, false, nullptr, false);
		NextMovementUpdateAt = CurrentTime + 1.f;
		if (Result == EPathFollowingRequestResult::Failed || bPathRequestFailed)
		{
			PatrolWaitUntil = CurrentTime + FMath::Max(1.f, RoutineStep.WaitSeconds);
			bPathRequestFailed = false;
		}
	}
}

void APropertyAIController::LookAround(float CurrentTime)
{
    if (!Resident || !Profile) return;
    const FRotator Facing(0.f, ScanBaseYaw + FMath::Sin((CurrentTime - ScanStartedAt) * Profile->LookAroundSpeed) * Profile->LookAroundAngle, 0.f);
    SetFocalPoint(Resident->GetActorLocation() + Facing.Vector() * 1000.f, EAIFocusPriority::Gameplay);
}

void APropertyAIController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
    if (Resident && Profile && Resident->Participant->Health > 0.f && ActiveResponse == DesiredResponse)
    {
        const float CurrentTime = GetWorld()->GetTimeSeconds();
        const bool bWaiting = Resident->Routine && ActiveResponse == EPropertyResponseState::Routine && CurrentTime < PatrolWaitUntil;
        const bool bSearching = ActiveResponse == EPropertyResponseState::Search && !HasVisibleSuspect() && GetMoveStatus() != EPathFollowingStatus::Moving;
        if (bWaiting || bSearching) LookAround(CurrentTime);
    }
    Super::UpdateControlRotation(DeltaTime, bUpdatePawn);
}

void APropertyAIController::ResetAfterLoad(const FPropertySavedNPC& Snapshot)
{
	RequestReturn();
	PatrolWaitUntil = 0.f;
	bShouldFaceHomeDirection = false;
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	HomeLocation = Snapshot.Home;
	HomeRotation = Snapshot.HomeRotation;
	RoutineEntry = Snapshot.RoutineEntry;
	Waypoint = Snapshot.Waypoint;
	RecoveryEndsAt = GetWorld()->GetTimeSeconds() + FMath::Max(0.f, Snapshot.RecoverySeconds);
	IgnoreNewResponsesUntil = GetWorld()->GetTimeSeconds() + 1.f;
	NextEvidenceReportAt = IgnoreNewResponsesUntil;
	NextNoiseInvestigationAt = 0.f;
	DesiredResponse = Resident->Participant->Health <= 0.f ? EPropertyResponseState::Return : EPropertyResponseState::Routine;
	StateTree->RestartLogic();
}

float APropertyAIController::GetRemainingRecovery() const
{
	return FMath::Max(0.f, RecoveryEndsAt - GetWorld()->GetTimeSeconds());
}

TArray<FTheftIncident> APropertyAIController::GetActionableCases(FName PersonId) const
{
	TArray<FTheftIncident> TheftCases;
	VisitActionableCases(PersonId, [&](const FTheftIncident& TheftIncident)
	{
		TheftCases.Add(TheftIncident);
		return true;
	});
	return TheftCases;
}

void APropertyAIController::VisitActionableCases(FName PersonId, TFunctionRef<bool(const FTheftIncident&)> Visitor) const
{
	if (!Resident || PersonId.IsNone()) return;
	const auto* LifeSubsystem = GetWorld()->GetSubsystem<UPropertyLifeSubsystem>();
	const auto& KnownThefts = Resident->Knowledge->GetKnownTheftsView();
	for (const auto& TheftIncident : KnownThefts)
	{
		if (TheftIncident.Offender.PersonId == PersonId && !LifeSubsystem->IsSettled(TheftIncident.IncidentId) && CanEnforceAt(TheftIncident.Location) && !Visitor(TheftIncident)) return;
	}
	if (!IsGuard()) return;
	for (const auto& Bounty : LifeSubsystem->Bounties)
	{
		const auto& TheftIncident = Bounty.Incident;
		if (TheftIncident.Offender.PersonId != PersonId || LifeSubsystem->IsSettled(TheftIncident.IncidentId)) continue;
		if (KnownThefts.ContainsByPredicate([&](const FTheftIncident& KnownIncident) { return KnownIncident.IncidentId == TheftIncident.IncidentId; })) continue;
		if (!Visitor(TheftIncident)) return;
	}
}

bool APropertyAIController::HasActionableCases(FName PersonId) const
{
	bool bHasUnsettledTheft = false;
	VisitActionableCases(PersonId, [&](const FTheftIncident&) { bHasUnsettledTheft = true; return false; });
	return bHasUnsettledTheft;
}

int32 APropertyAIController::CountActionableCases(FName PersonId) const
{
	int32 TheftCount = 0;
	VisitActionableCases(PersonId, [&](const FTheftIncident&) { ++TheftCount; return true; });
	return TheftCount;
}

void APropertyAIController::AssignSearch(APropertyArea* Area, FVector Location)
{
    if (!Area) { Investigate(Location, true); return; }
    if (!IsValid(Area) || !Resident || !CanEnforceAt(Location) || Suspect.IsValid() || Resident->Participant->Health <= 0.f || !Resident->Knowledge->bAlert) return;
    if (SearchArea == Area && SearchRole != EPropertySearchRole::None) return;
    if (DesiredResponse != EPropertyResponseState::Routine && !(DesiredResponse == EPropertyResponseState::Search && bInvestigatingLocation)) return;
    TSet<int32> UsedSlots;
    bool bExitWatched = false;
    bool bRoomInspected = false;
    for (TActorIterator<APropertyAIController> ActorIterator(GetWorld()); ActorIterator; ++ActorIterator)
    {
        if (*ActorIterator == this || ActorIterator->SearchArea != Area || ActorIterator->DesiredResponse != EPropertyResponseState::Search) continue;
        UsedSlots.Add(ActorIterator->SearchSlot);
        bExitWatched |= ActorIterator->SearchRole == EPropertySearchRole::WatchExit;
        bRoomInspected |= ActorIterator->SearchRole == EPropertySearchRole::InspectRoom;
    }
    SearchSlot = 0;
    while (UsedSlots.Contains(SearchSlot)) ++SearchSlot;
    SearchRole = IsGuard() && !bExitWatched ? EPropertySearchRole::WatchExit : (!bRoomInspected ? EPropertySearchRole::InspectRoom : EPropertySearchRole::SweepNearby);
    SearchArea = Area;
    bInvestigatingLocation = true;
    bPursuingOffender = false;
    LastKnownLocation = Location;
    PursuitStartedAt = GetWorld()->GetTimeSeconds();
    DesiredResponse = EPropertyResponseState::Search;
    SearchPointIndex = 0;
    if (ActiveResponse == EPropertyResponseState::Search)
    {
        MoveToSearchPoint();
        Resident->ReceiveResponseChanged(EPropertyResponseState::Search);
    }
}

bool APropertyAIController::TryQuestion(AActor* Player)
{
    const auto* QuestionedPawn = Cast<APawn>(Player);
    if (!Resident || !QuestionedPawn || !QuestionedPawn->IsPlayerControlled() || !Resident->Knowledge->CanSeeActor(Player) || Suspect.IsValid() || GetWorld()->GetTimeSeconds() < NextQuestionAt || FVector::DistSquared(Resident->GetActorLocation(), Player->GetActorLocation()) > FMath::Square(Profile->ConfrontDistance)) return false;
    const FName Person = UPropertyIdentityComponent::GetIdentity(Player).PersonId;
    const auto* LifeSubsystem = GetWorld()->GetSubsystem<UPropertyLifeSubsystem>();
    const bool bSuspected = Resident->Inspection->Reports.ContainsByPredicate([&](const FPropertyMissingReport& Report)
    {
        return Report.PossiblePerson == Person && !Person.IsNone() && Report.Age <= 90.f && !LifeSubsystem->IsMissingItemResolved(Report.ItemId) && CanEnforceAt(Report.Location);
    });
    if (!bSuspected || !StartResponse(Player, false)) return false;
    bQuestioning = true;
    NextQuestionAt = GetWorld()->GetTimeSeconds() + 60.f;
    DesiredResponse = EPropertyResponseState::Confront;
    Resident->Say(Resident->GetDialogue(EPropertyDialogueCue::StartQuestioning));
    return true;
}
