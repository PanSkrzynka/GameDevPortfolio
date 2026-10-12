#include "Property/AI/PropertyAIController.h"
#include "Property/AI/PropertyAIConstants.h"
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
#include "TimerManager.h"
#include "Engine/World.h"
#include "Property/PropertyLifeSubsystem.h"
#include "Property/PropertyParticipantComponent.h"

using namespace PropertyAIConstants;

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
	SightConfig->SetMaxAge(SightMemorySeconds);
	SightConfig->SightRadius = DefaultSightRadiusCm;
	SightConfig->LoseSightRadius = DefaultLoseSightRadiusCm;
	SightConfig->PeripheralVisionAngleDegrees = DefaultVisionHalfAngleDegrees;
	GetPerceptionComponent()->ConfigureSense(*SightConfig);
	GetPerceptionComponent()->SetDominantSense(UAISense_Sight::StaticClass());
}

void APropertyAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	Resident = Cast<APropertyWitness>(InPawn);
	if (!Resident)
	{
		return;
	}
	Profile =
	    Resident->ResponseProfile ? Resident->ResponseProfile.Get() : GetMutableDefault<UPropertyResponseProfile>();
	Resident->GetCharacterMovement()->RotationRate = FRotator(0.f, Profile->TurnRate, 0.f);
	HomeLocation = Resident->GetActorLocation();
	HomeRotation = Resident->GetActorRotation();
	DesiredResponse = EPropertyResponseState::Routine;
	SightConfig->SightRadius = Resident->Knowledge->SightRange;
	SightConfig->LoseSightRadius = SightConfig->SightRadius + LoseSightMarginCm;
	SightConfig->PeripheralVisionAngleDegrees = Resident->Knowledge->HalfVisionAngle;
	GetPerceptionComponent()->ConfigureSense(*SightConfig);
	GetPerceptionComponent()->SetDominantSense(UAISense_Sight::StaticClass());
	Resident->Knowledge->OnTheftWitnessed.AddDynamic(this, &APropertyAIController::HandleTheft);
	if (Resident->ResponseTree)
	{
		StateTree->SetStateTree(Resident->ResponseTree);
		StateTree->StartLogic();
		GetWorldTimerManager().SetTimer(AwarenessTimer, this, &APropertyAIController::EvaluateAwareness,
		                                AwarenessIntervalSeconds, true);
	}
}

void APropertyAIController::Cleanup()
{
	SearchArea.Reset();
	SearchRole = EPropertySearchRole::None;
	SearchSlot = INDEX_NONE;
	bQuestioning = false;
	if (GetWorld())
	{
		GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->ReleaseResponse(this);
	}
	GetWorldTimerManager().ClearTimer(AwarenessTimer);
	StateTree->StopLogic(TEXT("Controller released"));
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	if (IsValid(Resident))
	{
		Resident->Knowledge->OnTheftWitnessed.RemoveDynamic(this, &APropertyAIController::HandleTheft);
	}
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
	if (!IsValid(Resident))
	{
		return false;
	}
	if (IsGuard())
	{
		return true;
	}
	return CanEnforceArea(GetWorld()->GetSubsystem<UPropertyWorldSubsystem>()->ResolveArea(Location));
}

bool APropertyAIController::CanEnforceArea(const APropertyArea* Area) const
{
	return IsValid(Resident) &&
	       (IsGuard() || (IsValid(Area) && Area->IsAssigned(Resident->Identity->Identity.PersonId)));
}

bool APropertyAIController::IsProtectedTrespasser(AActor* Actor) const
{
	if (!IsValid(Actor) || !IsValid(Resident) || Actor == Resident)
	{
		return false;
	}
	const auto* VisitorPawn = Cast<APawn>(Actor);
	if (!VisitorPawn || !VisitorPawn->IsPlayerControlled())
	{
		return false;
	}
	const auto* Area = GetWorld()->GetSubsystem<UPropertyWorldSubsystem>()->ResolveArea(Actor->GetActorLocation());
	if (!Area || Area->CanEnter(UPropertyIdentityComponent::GetIdentity(Actor)))
	{
		return false;
	}
	return CanEnforceArea(Area);
}

void APropertyAIController::EvaluateAwareness()
{
	if (!IsValid(Resident) || !Resident->Knowledge->bAlert || !Profile)
	{
		return;
	}
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (CurrentTime < RecoveryEndsAt)
	{
		return;
	}
	if (IsGuard())
	{
		ShareEvidence(true);
	}
	if (CurrentTime >= NextEvidenceReportAt)
	{
		ShareEvidence();
		NextEvidenceReportAt = CurrentTime + EvidenceReportIntervalSeconds;
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
				if (bPursuingOffender || IsProtectedTrespasser(Suspect.Get()))
				{
					DesiredResponse = EPropertyResponseState::Chase;
				}
				else
				{
					RequestReturn();
				}
			}
		}
		return;
	}
	if (CurrentTime < IgnoreNewResponsesUntil ||
	    (DesiredResponse != EPropertyResponseState::Routine &&
	     !(bInvestigatingLocation && DesiredResponse == EPropertyResponseState::Search)))
	{
		return;
	}
	TArray<AActor*> Visible;
	GetPerceptionComponent()->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), Visible);
	for (AActor* Actor : Visible)
	{
		if (!Resident->Knowledge->CanSeeActor(Actor))
		{
			continue;
		}
		const FName PersonId = UPropertyIdentityComponent::GetIdentity(Actor).PersonId;
		if (HasActionableCases(PersonId) && StartResponse(Actor, true))
		{
			break;
		}
		if (TryQuestion(Actor))
		{
			break;
		}
		if (IsProtectedTrespasser(Actor))
		{
			if (StartResponse(Actor, false))
			{
				break;
			}
		}
	}
}

void APropertyAIController::HandleTheft(FTheftIncident Incident)
{
	if (!IsValid(Resident) || !Profile)
	{
		return;
	}
	if (!CanEnforceAt(Incident.Location))
	{
		return;
	}
	AActor* Offender = Resident->Knowledge->GetObservedOffender(Incident.IncidentId);
	if (!IsValid(Offender))
	{
		return;
	}
	if (auto* Area = GetWorld()->GetSubsystem<UPropertyWorldSubsystem>()->ResolveArea(Incident.Location))
	{
		Area->RegisterIncident(Incident.Item.ItemId, true);
	}
	Resident->RaiseAlarm(Incident.Location);
	if (IsGuard())
	{
		GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->ReportToGuard(Resident, Resident);
	}
	StartResponse(Offender, true);
}

void APropertyAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
	Super::OnMoveCompleted(RequestID, Result);
	if (Result.Code == EPathFollowingResult::Blocked || Result.Code == EPathFollowingResult::OffPath ||
	    Result.Code == EPathFollowingResult::Invalid)
	{
		bPathRequestFailed = true;
	}
}

bool APropertyAIController::IsGuard() const
{
	return Profile && Profile->bProtectAllProperty;
}

bool APropertyAIController::StartResponse(AActor* Actor, bool bCrime)
{
	if (!IsValid(Actor) || !IsValid(Resident) || GetWorld()->GetTimeSeconds() < RecoveryEndsAt)
	{
		return false;
	}
	if (bCrime && Profile &&
	    FVector::DistSquared(Resident->GetActorLocation(), Actor->GetActorLocation()) >
	        FMath::Square(FMath::Max(MinimumChaseBreakOffDistanceCm, Profile->ChaseBreakOffDistance)))
	{
		return false;
	}
	const auto* TargetPawn = Cast<APawn>(Actor);
	if (!TargetPawn || !TargetPawn->IsPlayerControlled())
	{
		return false;
	}
	const FName PersonId = UPropertyIdentityComponent::GetIdentity(Actor).PersonId;
	if (!GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->ClaimResponse(PersonId, this))
	{
		return false;
	}
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
	IgnoreNewResponsesUntil = GetWorld()->GetTimeSeconds() + LoadGraceSeconds;
	NextEvidenceReportAt = IgnoreNewResponsesUntil;
	NextNoiseInvestigationAt = 0.f;
	DesiredResponse =
	    Resident->Participant->Health <= 0.f ? EPropertyResponseState::Return : EPropertyResponseState::Routine;
	StateTree->RestartLogic();
}

float APropertyAIController::GetRemainingRecovery() const
{
	return FMath::Max(0.f, RecoveryEndsAt - GetWorld()->GetTimeSeconds());
}
