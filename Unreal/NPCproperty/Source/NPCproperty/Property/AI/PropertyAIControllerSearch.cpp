#include "Property/AI/PropertyAIController.h"
#include "Property/AI/PropertyAIConstants.h"
#include "Property/Demo/PropertyWitness.h"
#include "Property/NPCKnowledgeComponent.h"
#include "Property/PropertyArea.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/BoxComponent.h"
#include "Property/PropertyParticipantComponent.h"

using namespace PropertyAIConstants;

void APropertyAIController::MoveToSearchPoint()
{
	ClearFocus(EAIFocusPriority::Gameplay);
	SearchPauseEndsAt = -1.f;
	const FVector Offsets[] = {FVector::ZeroVector, FVector(1, 0, 0), FVector(0, 1, 0), FVector(-1, 0, 0),
	                           FVector(0, -1, 0)};
	FVector Candidate = LastKnownLocation + Offsets[SearchPointIndex % UE_ARRAY_COUNT(Offsets)] * Profile->SearchRadius;
	if (SearchArea.IsValid() && SearchRole != EPropertySearchRole::None)
	{
		const auto* Area = SearchArea.Get();
		if (SearchRole == EPropertySearchRole::WatchExit)
		{
			Candidate = Area->GetExitLocation();
		}
		else if (SearchRole == EPropertySearchRole::InspectRoom)
		{
			const FVector Extent = Area->Bounds->GetScaledBoxExtent() * RoomInspectionExtentScale;
			const float Angle = SearchPointIndex * UE_PI * .5f;
			Candidate =
			    Area->GetActorLocation() + FVector(FMath::Cos(Angle) * Extent.X, FMath::Sin(Angle) * Extent.Y, 0);
		}
		else
		{
			const float Angle =
			    SearchSlot * SearchSlotAngleRadians + (SearchPointIndex % 2) * SearchSweepAngleOffsetRadians;
			const float Radius =
			    FMath::Max(Area->Bounds->GetScaledBoxExtent().X, Area->Bounds->GetScaledBoxExtent().Y) +
			    SearchPerimeterMarginCm;
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
	if (Navigation && Navigation->ProjectPointToNavigation(Candidate, Projected, SearchNavigationExtent))
	{
		SearchDestination = Projected.Location;
		bPathRequestFailed = MoveToLocation(SearchDestination, SearchAcceptanceRadiusCm, true, true, true, false,
		                                    nullptr, false) == EPathFollowingRequestResult::Failed;
	}
	NextMovementUpdateAt =
	    GetWorld()->GetTimeSeconds() +
	    (SearchRole != EPropertySearchRole::None ? AssignedSearchUpdateSeconds : SoloSearchUpdateSeconds);
}

bool APropertyAIController::Investigate(FVector Location, bool bAlarm)
{
	const float CurrentTime = GetWorld()->GetTimeSeconds();
	if (!Resident || !Profile || Location.ContainsNaN() || !CanEnforceAt(Location) || !Resident->Knowledge->bAlert ||
	    Resident->Participant->Health <= 0.f || CurrentTime < RecoveryEndsAt || SearchRole != EPropertySearchRole::None)
	{
		return false;
	}
	if (!bAlarm && CurrentTime < NextNoiseInvestigationAt)
	{
		return false;
	}
	if (bInvestigatingLocation && DesiredResponse == EPropertyResponseState::Search && !Suspect.IsValid())
	{
		if (!bAlarm && FVector::DistSquared(LastKnownLocation, Location) < FMath::Square(NoiseRetargetDistanceCm))
		{
			return false;
		}
		LastKnownLocation = Location;
		SearchPointIndex = 0;
		ResponseStartedAt = CurrentTime;
		ScanBaseYaw = Resident->GetActorRotation().Yaw;
		ScanStartedAt = CurrentTime;
		NextNoiseInvestigationAt = CurrentTime + Profile->NoiseRetargetCooldown;
		MoveToSearchPoint();
		return true;
	}
	if (DesiredResponse != EPropertyResponseState::Routine)
	{
		return false;
	}
	bInvestigatingLocation = true;
	bPursuingOffender = false;
	LastKnownLocation = Location;
	PursuitStartedAt = CurrentTime;
	NextNoiseInvestigationAt = CurrentTime + Profile->NoiseRetargetCooldown;
	DesiredResponse = EPropertyResponseState::Search;
	return true;
}

void APropertyAIController::AssignSearch(APropertyArea* Area, FVector Location)
{
	if (!Area)
	{
		Investigate(Location, true);
		return;
	}
	if (!IsValid(Area) || !Resident || !CanEnforceAt(Location) || Suspect.IsValid() ||
	    Resident->Participant->Health <= 0.f || !Resident->Knowledge->bAlert)
	{
		return;
	}
	if (SearchArea == Area && SearchRole != EPropertySearchRole::None)
	{
		return;
	}
	if (DesiredResponse != EPropertyResponseState::Routine &&
	    !(DesiredResponse == EPropertyResponseState::Search && bInvestigatingLocation))
	{
		return;
	}
	TSet<int32> UsedSlots;
	bool bExitWatched = false;
	bool bRoomInspected = false;
	for (TActorIterator<APropertyAIController> ActorIterator(GetWorld()); ActorIterator; ++ActorIterator)
	{
		if (*ActorIterator == this || ActorIterator->SearchArea != Area ||
		    ActorIterator->DesiredResponse != EPropertyResponseState::Search)
		{
			continue;
		}
		UsedSlots.Add(ActorIterator->SearchSlot);
		bExitWatched |= ActorIterator->SearchRole == EPropertySearchRole::WatchExit;
		bRoomInspected |= ActorIterator->SearchRole == EPropertySearchRole::InspectRoom;
	}
	SearchSlot = 0;
	while (UsedSlots.Contains(SearchSlot))
	{
		++SearchSlot;
	}
	SearchRole = IsGuard() && !bExitWatched
	                 ? EPropertySearchRole::WatchExit
	                 : (!bRoomInspected ? EPropertySearchRole::InspectRoom : EPropertySearchRole::SweepNearby);
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
