#include "Property/AI/PropertyAIController.h"
#include "Property/AI/PropertyAIConstants.h"
#include "Property/Demo/PropertyWitness.h"
#include "Property/PropertyArea.h"
#include "Property/PropertyWorldSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "Engine/World.h"
#include "Property/PropertyLifeSubsystem.h"
#include "Property/PropertyParticipantComponent.h"

using namespace PropertyAIConstants;

void APropertyAIController::AdvanceRoutine(float CurrentTime)
{
	if (!Resident->Routine)
	{
		return;
	}
	const float Hour = GetWorld()->GetSubsystem<UPropertyLifeSubsystem>()->Hour;
	int32 SelectedRoutineIndex = INDEX_NONE;
	for (int32 RoutineIndex = 0; RoutineIndex < Resident->Routine->Entries.Num(); ++RoutineIndex)
	{
		const auto& RoutineStep = Resident->Routine->Entries[RoutineIndex];
		const bool bActive = RoutineStep.StartHour <= RoutineStep.EndHour
		                         ? Hour >= RoutineStep.StartHour && Hour < RoutineStep.EndHour
		                         : Hour >= RoutineStep.StartHour || Hour < RoutineStep.EndHour;
		if (bActive)
		{
			SelectedRoutineIndex = RoutineIndex;
			break;
		}
	}
	if (SelectedRoutineIndex != RoutineEntry)
	{
		ClearFocus(EAIFocusPriority::Gameplay);
		RoutineEntry = SelectedRoutineIndex;
		Waypoint = 0;
		PatrolWaitUntil = 0.f;
		StopMovement();
	}
	if (!Resident->Routine->Entries.IsValidIndex(RoutineEntry))
	{
		return;
	}
	const auto& RoutineStep = Resident->Routine->Entries[RoutineEntry];
	CurrentActivity = RoutineStep.Activity;
	const auto* PatrolArea =
	    GetWorld()->GetSubsystem<UPropertyWorldSubsystem>()->ResolveArea(Resident->GetActorLocation());
	const bool bHeightened = PatrolArea && PatrolArea->GetAlertLevel() >= EPropertyAlertLevel::Searching &&
	                         CanEnforceAt(Resident->GetActorLocation());
	Resident->GetCharacterMovement()->MaxWalkSpeed = Profile->WalkSpeed * (bHeightened ? AlertPatrolSpeedScale : 1.f);
	if (RoutineStep.Offsets.IsEmpty())
	{
		return;
	}
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
	if (FVector::DistSquared(Resident->GetActorLocation(), Destination) < FMath::Square(ArrivalDistanceCm))
	{
		StopMovement();
		ScanBaseYaw = bShouldFaceHomeDirection ? HomeRotation.Yaw : Resident->GetActorRotation().Yaw;
		bShouldFaceHomeDirection = false;
		ScanStartedAt = CurrentTime;
		PatrolWaitUntil = CurrentTime + FMath::Max(MinimumPauseSeconds, RoutineStep.WaitSeconds *
		                                                                    (bHeightened ? AlertPatrolWaitScale : 1.f));
		return;
	}
	if (CurrentTime >= NextMovementUpdateAt)
	{
		const auto Result =
		    MoveToLocation(Destination, PatrolAcceptanceRadiusCm, true, true, true, false, nullptr, false);
		NextMovementUpdateAt = CurrentTime + PatrolUpdateSeconds;
		if (Result == EPathFollowingRequestResult::Failed || bPathRequestFailed)
		{
			PatrolWaitUntil = CurrentTime + FMath::Max(MinimumFailedPatrolWaitSeconds, RoutineStep.WaitSeconds);
			bPathRequestFailed = false;
		}
	}
}

void APropertyAIController::LookAround(float CurrentTime)
{
	if (!Resident || !Profile)
	{
		return;
	}
	const FRotator Facing(0.f,
	                      ScanBaseYaw + FMath::Sin((CurrentTime - ScanStartedAt) * Profile->LookAroundSpeed) *
	                                        Profile->LookAroundAngle,
	                      0.f);
	SetFocalPoint(Resident->GetActorLocation() + Facing.Vector() * FocusDistanceCm, EAIFocusPriority::Gameplay);
}

void APropertyAIController::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
	if (Resident && Profile && Resident->Participant->Health > 0.f && ActiveResponse == DesiredResponse)
	{
		const float CurrentTime = GetWorld()->GetTimeSeconds();
		const bool bWaiting =
		    Resident->Routine && ActiveResponse == EPropertyResponseState::Routine && CurrentTime < PatrolWaitUntil;
		const bool bSearching = ActiveResponse == EPropertyResponseState::Search && !HasVisibleSuspect() &&
		                        GetMoveStatus() != EPathFollowingStatus::Moving;
		if (bWaiting || bSearching)
		{
			LookAround(CurrentTime);
		}
	}
	Super::UpdateControlRotation(DeltaTime, bUpdatePawn);
}
