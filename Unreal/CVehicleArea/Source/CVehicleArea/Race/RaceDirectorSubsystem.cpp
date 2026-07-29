#include "RaceDirectorSubsystem.h"
#include "CVehicleArea.h"
#include "VehicleAIController.h"
#include "Replay/VehicleLapRecorderComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

void URaceDirectorSubsystem::SetReferenceLine(const FRacingLine& InLine)
{
	ReferenceLine = InLine;

	for (FRaceCompetitor& Competitor : Competitors)
	{
		Competitor.LastPointIndex = INDEX_NONE;
		Competitor.DistanceAlongLine = 0.0f;
		Competitor.CurrentSector = 0;
		Competitor.VisitedSectors.Init(false, SectorCount);
	}
}

bool URaceDirectorSubsystem::RegisterCompetitor(APawn* Vehicle, const FString& DisplayName, bool bIsPlayer)
{
	if (!Vehicle)
	{
		return false;
	}

	for (const FRaceCompetitor& Existing : Competitors)
	{
		if (Existing.Vehicle.Get() == Vehicle)
		{
			return false;
		}
	}

	FRaceCompetitor Competitor;
	Competitor.Vehicle = Vehicle;
	Competitor.DisplayName = DisplayName.IsEmpty() ? Vehicle->GetName() : DisplayName;
	Competitor.bIsPlayer = bIsPlayer;
	Competitor.VisitedSectors.Init(false, SectorCount);

	const bool bJoinedRunningSession = RaceState == ERaceState::Countdown || RaceState == ERaceState::Racing;
	if (bJoinedRunningSession)
	{
		const UWorld* World = GetWorld();
		Competitor.LapStartTime = World ? World->GetTimeSeconds() : 0.0f;

		if (ReferenceLine.IsValid())
		{
			const int32 Nearest = ReferenceLine.FindNearestPoint(Vehicle->GetActorLocation(), INDEX_NONE);
			if (Nearest != INDEX_NONE)
			{
				Competitor.LastPointIndex = Nearest;
				Competitor.DistanceAlongLine = ReferenceLine.Points[Nearest].Distance;
				Competitor.CurrentSector = (Nearest * SectorCount) / FMath::Max(1, ReferenceLine.Points.Num());
				Competitor.VisitedSectors[FMath::Clamp(Competitor.CurrentSector, 0, SectorCount - 1)] = true;
			}
		}
	}

	Competitors.Add(MoveTemp(Competitor));

	if (RaceState == ERaceState::Racing)
	{
		if (AVehicleAIController* Driver = GetAIControllerFor(Competitors.Last()))
		{
			Driver->SetDrivingEnabled(true);
		}
	}

	return true;
}

void URaceDirectorSubsystem::UnregisterCompetitor(APawn* Vehicle)
{
	Competitors.RemoveAll([Vehicle](const FRaceCompetitor& Competitor)
	{
		return Competitor.Vehicle.Get() == Vehicle;
	});
}

void URaceDirectorSubsystem::ClearSession()
{
	SetCompetitorsDriving(false);
	Competitors.Reset();
	FinishedCount = 0;
	CountdownRemaining = 0.0f;
	SetRaceState(ERaceState::Idle);
}

const FRaceCompetitor* URaceDirectorSubsystem::FindCompetitor(const APawn* Vehicle) const
{
	return Competitors.FindByPredicate([Vehicle](const FRaceCompetitor& Competitor)
	{
		return Competitor.Vehicle.Get() == Vehicle;
	});
}

AVehicleAIController* URaceDirectorSubsystem::GetAIControllerFor(const FRaceCompetitor& Competitor)
{
	APawn* Vehicle = Competitor.Vehicle.Get();
	return Vehicle ? Cast<AVehicleAIController>(Vehicle->GetController()) : nullptr;
}

bool URaceDirectorSubsystem::StartRace(int32 InLapCount, float CountdownSeconds)
{
	if (!ReferenceLine.IsValid())
	{
		UE_LOG(LogCVehicleArea, Warning, TEXT("Cannot start: no racing line."));
		return false;
	}

	RemoveStaleCompetitors();

	if (Competitors.Num() == 0)
	{
		UE_LOG(LogCVehicleArea, Warning, TEXT("Cannot start: no competitors."));
		return false;
	}

	LapCount = FMath::Max(1, InLapCount);
	FinishedCount = 0;
	CountdownRemaining = FMath::Max(0.0f, CountdownSeconds);

	const UWorld* World = GetWorld();
	const float WorldTime = World ? World->GetTimeSeconds() : 0.0f;

	for (FRaceCompetitor& Competitor : Competitors)
	{
		Competitor.LapsCompleted = 0;
		Competitor.LastLapTime = 0.0f;
		Competitor.BestLapTime = 0.0f;
		Competitor.TotalTime = 0.0f;
		Competitor.bFinished = false;
		Competitor.Position = 0;
		Competitor.LapStartTime = WorldTime;
		Competitor.LastPointIndex = INDEX_NONE;
		Competitor.VisitedSectors.Init(false, SectorCount);

		if (const APawn* Vehicle = Competitor.Vehicle.Get())
		{
			const int32 Nearest = ReferenceLine.FindNearestPoint(Vehicle->GetActorLocation(), INDEX_NONE);
			if (Nearest != INDEX_NONE)
			{
				Competitor.LastPointIndex = Nearest;
				Competitor.DistanceAlongLine = ReferenceLine.Points[Nearest].Distance;
				Competitor.CurrentSector = (Nearest * SectorCount) / FMath::Max(1, ReferenceLine.Points.Num());
				Competitor.VisitedSectors[FMath::Clamp(Competitor.CurrentSector, 0, SectorCount - 1)] = true;
			}
		}
	}

	SetCompetitorsDriving(false);
	SetRaceState(CountdownRemaining > 0.0f ? ERaceState::Countdown : ERaceState::Racing);

	if (RaceState == ERaceState::Racing)
	{
		RaceStartTime = WorldTime;
		SetCompetitorsDriving(true);
	}

	UE_LOG(LogCVehicleArea, Log, TEXT("Race started: %d laps, %d cars."), LapCount, Competitors.Num());
	return true;
}

void URaceDirectorSubsystem::EndRace()
{
	SetCompetitorsDriving(false);
	SetRaceState(ERaceState::Finished);
}

void URaceDirectorSubsystem::SetRaceState(ERaceState NewState)
{
	if (RaceState == NewState)
	{
		return;
	}

	RaceState = NewState;
	OnRaceStateChanged.Broadcast(NewState);
}

void URaceDirectorSubsystem::SetCompetitorsDriving(bool bDriving)
{
	for (const FRaceCompetitor& Competitor : Competitors)
	{
		if (AVehicleAIController* AIController = GetAIControllerFor(Competitor))
		{

			AIController->SetDrivingEnabled(bDriving && !Competitor.bFinished);
		}
	}
}

float URaceDirectorSubsystem::GetRaceElapsedTime() const
{
	if (RaceState != ERaceState::Racing && RaceState != ERaceState::Finished)
	{
		return 0.0f;
	}

	const UWorld* World = GetWorld();
	return World ? World->GetTimeSeconds() - RaceStartTime : 0.0f;
}

void URaceDirectorSubsystem::RemoveStaleCompetitors()
{
	Competitors.RemoveAll([](const FRaceCompetitor& Competitor)
	{
		return !Competitor.Vehicle.IsValid();
	});
}

void URaceDirectorSubsystem::UpdateCompetitorProgress(FRaceCompetitor& Competitor, float WorldTime)
{
	const APawn* Vehicle = Competitor.Vehicle.Get();
	if (!Vehicle || Competitor.bFinished)
	{
		return;
	}

	const int32 Nearest = ReferenceLine.FindNearestPoint(Vehicle->GetActorLocation(), Competitor.LastPointIndex);
	if (Nearest == INDEX_NONE)
	{
		return;
	}

	Competitor.LastPointIndex = Nearest;
	Competitor.DistanceAlongLine = ReferenceLine.Points[Nearest].Distance;

	const int32 PointCount = FMath::Max(1, ReferenceLine.Points.Num());
	const int32 NewSector = FMath::Clamp((Nearest * SectorCount) / PointCount, 0, SectorCount - 1);

	if (NewSector == Competitor.CurrentSector)
	{
		return;
	}

	const int32 PreviousSector = Competitor.CurrentSector;
	Competitor.CurrentSector = NewSector;

	if (Competitor.VisitedSectors.IsValidIndex(NewSector))
	{
		Competitor.VisitedSectors[NewSector] = true;
	}

	if (PreviousSector == SectorCount - 1 && NewSector == 0)
	{
		HandleLapCompletion(Competitor, WorldTime);
	}
}

void URaceDirectorSubsystem::HandleLapCompletion(FRaceCompetitor& Competitor, float WorldTime)
{

	int32 Visited = 0;
	for (const bool bVisited : Competitor.VisitedSectors)
	{
		Visited += bVisited ? 1 : 0;
	}

	const float VisitedFraction = SectorCount > 0 ? static_cast<float>(Visited) / SectorCount : 0.0f;

	Competitor.VisitedSectors.Init(false, SectorCount);
	Competitor.VisitedSectors[0] = true;

	if (VisitedFraction < MinSectorFraction)
	{
		UE_LOG(LogCVehicleArea, Verbose, TEXT("'%s': lap not counted, only %.0f%% of sectors."),
			*Competitor.DisplayName, VisitedFraction * 100.0f);

		Competitor.LapStartTime = WorldTime;
		return;
	}

	const float LapTime = WorldTime - Competitor.LapStartTime;
	Competitor.LapStartTime = WorldTime;

	++Competitor.LapsCompleted;
	Competitor.LastLapTime = LapTime;

	if (Competitor.BestLapTime <= 0.0f || LapTime < Competitor.BestLapTime)
	{
		Competitor.BestLapTime = LapTime;
	}

	if (APawn* Vehicle = Competitor.Vehicle.Get())
	{
		if (UVehicleLapRecorderComponent* Recorder = Vehicle->FindComponentByClass<UVehicleLapRecorderComponent>())
		{
			if (Recorder->IsRecording())
			{
				Recorder->CompleteLap(LapTime);
			}
			Recorder->BeginLap();
		}
	}

	OnCompetitorLapCompleted.Broadcast(Competitor.DisplayName, LapTime, Competitor.LapsCompleted);

	if (Competitor.LapsCompleted >= LapCount)
	{
		Competitor.bFinished = true;
		Competitor.TotalTime = WorldTime - RaceStartTime;

		++FinishedCount;
		OnCompetitorFinished.Broadcast(Competitor.DisplayName, FinishedCount);

		if (AVehicleAIController* AIController = GetAIControllerFor(Competitor))
		{
			AIController->SetDrivingEnabled(false);
		}
	}
}

void URaceDirectorSubsystem::UpdateStandings()
{
	const float LineLength = ReferenceLine.TotalLength;

	Competitors.Sort([LineLength](const FRaceCompetitor& A, const FRaceCompetitor& B)
	{

		if (A.bFinished != B.bFinished)
		{
			return A.bFinished;
		}

		if (A.bFinished && B.bFinished)
		{
			return A.TotalTime < B.TotalTime;
		}

		return A.GetRaceProgress(LineLength) > B.GetRaceProgress(LineLength);
	});

	for (int32 Index = 0; Index < Competitors.Num(); ++Index)
	{
		Competitors[Index].Position = Index + 1;
	}
}

void URaceDirectorSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const float WorldTime = World->GetTimeSeconds();

	if (RaceState == ERaceState::Countdown)
	{
		CountdownRemaining -= DeltaTime;

		if (CountdownRemaining <= 0.0f)
		{
			CountdownRemaining = 0.0f;
			RaceStartTime = WorldTime;

			for (FRaceCompetitor& Competitor : Competitors)
			{
				Competitor.LapStartTime = WorldTime;
			}

			SetRaceState(ERaceState::Racing);
			SetCompetitorsDriving(true);
		}

		return;
	}

	if (RaceState != ERaceState::Racing)
	{
		return;
	}

	RemoveStaleCompetitors();

	if (!ReferenceLine.IsValid())
	{
		return;
	}

	for (FRaceCompetitor& Competitor : Competitors)
	{
		if (!Competitor.bFinished)
		{
			Competitor.TotalTime = WorldTime - RaceStartTime;
		}

		UpdateCompetitorProgress(Competitor, WorldTime);
	}

	UpdateStandings();

	const bool bAnyoneRunning = Competitors.ContainsByPredicate([](const FRaceCompetitor& Competitor)
	{
		return !Competitor.bFinished;
	});

	if (!bAnyoneRunning && Competitors.Num() > 0)
	{
		SetRaceState(ERaceState::Finished);
	}
}

TStatId URaceDirectorSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(URaceDirectorSubsystem, STATGROUP_Tickables);
}

bool URaceDirectorSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{

	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

FString URaceDirectorSubsystem::FormatLapTime(float Seconds)
{
	if (Seconds <= 0.0f)
	{
		return TEXT("--:--.---");
	}

	const int32 Minutes = FMath::FloorToInt(Seconds / 60.0f);
	const float Remainder = Seconds - Minutes * 60.0f;

	return FString::Printf(TEXT("%d:%06.3f"), Minutes, Remainder);
}
