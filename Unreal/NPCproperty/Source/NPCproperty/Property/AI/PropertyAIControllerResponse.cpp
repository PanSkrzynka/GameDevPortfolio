#include "Property/AI/PropertyAIController.h"
#include "Property/AI/PropertyAIConstants.h"
#include "Property/Demo/PropertyWitness.h"
#include "Property/NPCKnowledgeComponent.h"
#include "Property/PropertyIdentityComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/World.h"
#include "Property/PropertyLifeSubsystem.h"
#include "Property/PropertyParticipantComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/DamageType.h"

using namespace PropertyAIConstants;

void APropertyAIController::EnterResponse(EPropertyResponseState State)
{
	if (!IsValid(Resident) || !Profile)
	{
		return;
	}
	ActiveResponse = State;
	ResponseStartedAt = GetWorld()->GetTimeSeconds();
	ResponseUpdateElapsed = 0.f;
	NextMovementUpdateAt = 0.f;
	bPathRequestFailed = false;
	StopMovement();
	ClearFocus(EAIFocusPriority::Gameplay);
	if (GetWorld()->GetTimeSeconds() < RecoveryEndsAt)
	{
		return;
	}
	const float ResponseSpeed = Resident->SelectResponseSpeed(State, Profile->WalkSpeed, Profile->ChaseSpeed);
	Resident->GetCharacterMovement()->MaxWalkSpeed =
	    FMath::IsFinite(ResponseSpeed) ? FMath::Clamp(ResponseSpeed, 1.f, MaximumResponseSpeedCmPerSecond)
	                                   : Profile->WalkSpeed;
	if (State == EPropertyResponseState::Routine)
	{
		Suspect.Reset();
		bResisting = false;
		bInvestigatingLocation = false;
		PatrolWaitUntil = 0.f;
		bShouldFaceHomeDirection =
		    FVector::DistSquared(Resident->GetActorLocation(), HomeLocation) < FMath::Square(ArrivalDistanceCm);
		if (bShouldFaceHomeDirection)
		{
			SetFocalPoint(Resident->GetActorLocation() + HomeRotation.Vector() * FocusDistanceCm,
			              EAIFocusPriority::Gameplay);
		}
		SetControlRotation(Resident->GetActorRotation());
	}
	else if (State == EPropertyResponseState::Search)
	{
		if (!bPursuingOffender && Suspect.IsValid())
		{
			PursuitStartedAt = ResponseStartedAt;
		}
		SearchPointIndex = 0;
		SearchStartedAt = -1.f;
		SearchPauseEndsAt = -1.f;
		ScanBaseYaw = Resident->GetActorRotation().Yaw;
		ScanStartedAt = GetWorld()->GetTimeSeconds();
		MoveToSearchPoint();
	}
	else if (State == EPropertyResponseState::Return)
	{
		const auto Result =
		    MoveToLocation(HomeLocation, ReturnAcceptanceRadiusCm, true, true, true, false, nullptr, false);
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

void APropertyAIController::AdvanceResponse(float DeltaTime)
{
	if (!IsValid(Resident) || !Profile || ActiveResponse != DesiredResponse)
	{
		return;
	}
	ResponseUpdateElapsed += DeltaTime;
	if (ResponseUpdateElapsed < ResponseIntervalSeconds)
	{
		return;
	}
	ResponseUpdateElapsed = 0.f;
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	const float TimeInState = CurrentTime - ResponseStartedAt;
	if (CurrentTime < RecoveryEndsAt)
	{
		return;
	}
	if (Resident->Participant->Health <= 0.f)
	{
		Resident->Participant->Recover();
		EnterResponse(EPropertyResponseState::Return);
	}
	if (bPursuingOffender && Suspect.IsValid() &&
	    FVector::DistSquared(Resident->GetActorLocation(), Suspect->GetActorLocation()) >
	        FMath::Square(FMath::Max(MinimumChaseBreakOffDistanceCm, Profile->ChaseBreakOffDistance)))
	{
		StopMovement();
		ClearFocus(EAIFocusPriority::Gameplay);
		RequestReturn();
		return;
	}
	if (!bPursuingOffender && !bQuestioning && !bResisting &&
	    (ActiveResponse == EPropertyResponseState::Warn || ActiveResponse == EPropertyResponseState::Chase ||
	     ActiveResponse == EPropertyResponseState::Confront))
	{
		if (HasVisibleSuspect() || !IsProtectedTrespasser(Suspect.Get()))
		{
			AdvanceTrespassFollow(CurrentTime, TimeInState);
		}
		else
		{
			ClearFocus(EAIFocusPriority::Gameplay);
			DesiredResponse = EPropertyResponseState::Search;
		}
		return;
	}
	if ((ActiveResponse == EPropertyResponseState::Chase || ActiveResponse == EPropertyResponseState::Search) &&
	    ((!Suspect.IsValid() && !bInvestigatingLocation) ||
	     CurrentTime - PursuitStartedAt > Profile->MaxPursuitSeconds ||
	     FVector::DistSquared(Resident->GetActorLocation(), HomeLocation) > FMath::Square(Profile->LeashDistance)))
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
			if (CurrentTime - SuspectLastSeenAt > Profile->LostSightSeconds)
			{
				RequestReturn();
			}
		}
		else if (!IsProtectedTrespasser(Suspect.Get()))
		{
			RequestReturn();
		}
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
				bPathRequestFailed = MoveToLocation(LastKnownLocation, ChaseAcceptanceRadiusCm, true, true, true, false,
				                                    nullptr, false) == EPathFollowingRequestResult::Failed;
				NextMovementUpdateAt = CurrentTime + ChaseUpdateSeconds;
			}
			if (CurrentTime - SuspectLastSeenAt >= Profile->LostSightSeconds)
			{
				DesiredResponse = EPropertyResponseState::Search;
			}
		}
		else if (!bPursuingOffender && !IsProtectedTrespasser(Suspect.Get()))
		{
			RequestReturn();
		}
		else
		{
			SetFocus(Suspect.Get());
			if (FVector::DistSquared(Resident->GetActorLocation(), Suspect->GetActorLocation()) <=
			    FMath::Square(Profile->ConfrontDistance))
			{
				DesiredResponse = EPropertyResponseState::Confront;
			}
			else if (CurrentTime >= NextMovementUpdateAt)
			{
				const auto Result =
				    MoveToLocation(LastKnownLocation, ChaseAcceptanceRadiusCm, true, true, true, false, nullptr, false);
				bPathRequestFailed = Result == EPathFollowingRequestResult::Failed;
				NextMovementUpdateAt = CurrentTime + ChaseUpdateSeconds;
			}
		}
		if (bPathRequestFailed && DesiredResponse == EPropertyResponseState::Chase)
		{
			DesiredResponse = EPropertyResponseState::Search;
		}
		break;
	case EPropertyResponseState::Search:
		if (SearchRole != EPropertySearchRole::None)
		{
			if (TimeInState >= Profile->SearchSeconds * AssignedSearchDurationScale)
			{
				RequestReturn();
			}
			else if (CurrentTime >= NextMovementUpdateAt)
			{
				MoveToSearchPoint();
			}
		}
		else
		{
			if (SearchStartedAt >= 0.f && CurrentTime - SearchStartedAt >= Profile->SearchSeconds)
			{
				RequestReturn();
			}
			else if (GetMoveStatus() != EPathFollowingStatus::Moving)
			{
				if (SearchStartedAt < 0.f)
				{
					SearchStartedAt = CurrentTime;
				}
				if (SearchPauseEndsAt < 0.f)
				{
					SearchPauseEndsAt = CurrentTime + FMath::Max(MinimumPauseSeconds, Profile->SearchPointPauseSeconds);
					ScanBaseYaw = Resident->GetActorRotation().Yaw;
					ScanStartedAt = CurrentTime;
				}
				else if (CurrentTime >= SearchPauseEndsAt)
				{
					MoveToSearchPoint();
				}
			}
		}
		break;
	case EPropertyResponseState::Confront:
		if (bQuestioning)
		{
			if (!bVisible || TimeInState > QuestioningTimeoutSeconds ||
			    FVector::DistSquared(Resident->GetActorLocation(), Suspect->GetActorLocation()) >
			        FMath::Square(QuestioningBreakOffDistanceCm))
			{
				RequestReturn();
			}
			else
			{
				SetFocus(Suspect.Get());
			}
			break;
		}
		if (!bVisible)
		{
			DesiredResponse = EPropertyResponseState::Search;
		}
		else if (FVector::DistSquared(Resident->GetActorLocation(), Suspect->GetActorLocation()) >
		         FMath::Square(Profile->ConfrontDistance * ConfrontBreakOffDistanceScale))
		{
			DesiredResponse = EPropertyResponseState::Chase;
		}
		else
		{
			SetFocus(Suspect.Get());
			if (bResisting && CurrentTime >= NextAttackAt)
			{
				NextAttackAt = CurrentTime + AttackIntervalSeconds;
				Resident->PlayAttackAnimation();
				UGameplayStatics::ApplyDamage(Suspect.Get(), AttackDamage, this, Resident, UDamageType::StaticClass());
			}
			else if (!bPursuingOffender && !bResisting && TimeInState >= Profile->ConfrontSeconds)
			{
				RequestReturn();
			}
		}
		break;
	case EPropertyResponseState::Return:
		if (FVector::DistSquared(Resident->GetActorLocation(), HomeLocation) < FMath::Square(ArrivalDistanceCm) ||
		    bPathRequestFailed || TimeInState >= ReturnTimeoutSeconds)
		{
			DesiredResponse = EPropertyResponseState::Routine;
		}
		break;
	default:
		break;
	}
}

void APropertyAIController::AdvanceWarning(float CurrentTime, float TimeInState)
{
	const FName PersonId = UPropertyIdentityComponent::GetIdentity(Suspect.Get()).PersonId;
	const int32 PriorOffences = Resident->Knowledge->CountKnownThefts(PersonId);
	if (TimeInState < Profile->WarningSeconds / (1.f + PriorOffenceWarningWeight * PriorOffences))
	{
		return;
	}
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
	if (DistanceSquared <= FMath::Square(Profile->ConfrontDistance))
	{
		StopMovement();
	}
	else if (CurrentTime >= NextMovementUpdateAt)
	{
		MoveToActor(Suspect.Get(), FMath::Max(MinimumFollowRadiusCm, Profile->ConfrontDistance * FollowRadiusScale),
		            true, true, true, nullptr, false);
		NextMovementUpdateAt = CurrentTime + ChaseUpdateSeconds;
	}
	if (ActiveResponse == EPropertyResponseState::Warn)
	{
		AdvanceWarning(CurrentTime, TimeInState);
	}
	else if (ActiveResponse == EPropertyResponseState::Chase &&
	         DistanceSquared <= FMath::Square(Profile->ConfrontDistance))
	{
		DesiredResponse = EPropertyResponseState::Confront;
	}
	else if (ActiveResponse == EPropertyResponseState::Confront &&
	         DistanceSquared > FMath::Square(Profile->ConfrontDistance * ConfrontBreakOffDistanceScale))
	{
		DesiredResponse = EPropertyResponseState::Chase;
	}
}
