#include "CoreMinimal.h"
#include "CVehicleArea.h"
#include "HAL/IConsoleManager.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "WheeledVehiclePawn.h"

#include "Telemetry/VehicleTelemetryComponent.h"
#include "Dynamics/VehicleDriverAssistComponent.h"
#include "Replay/VehicleLapRecorderComponent.h"
#include "Replay/VehicleGhostActor.h"
#include "Race/RacingLine.h"
#include "Race/VehicleAIController.h"
#include "Race/RaceDirectorSubsystem.h"
#include "HUD/VehicleTelemetryHUD.h"

namespace VehicleDebugCommands
{

	APawn* GetPlayerVehicle(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		APlayerController* PlayerController = World->GetFirstPlayerController();
		return PlayerController ? PlayerController->GetPawn() : nullptr;
	}

	template <typename TComponent>
	TComponent* GetPlayerVehicleComponent(UWorld* World, const TCHAR* SystemName)
	{
		APawn* Vehicle = GetPlayerVehicle(World);
		if (!Vehicle)
		{
			UE_LOG(LogCVehicleArea, Warning, TEXT("%s: no player vehicle."), SystemName);
			return nullptr;
		}

		TComponent* Component = Vehicle->FindComponentByClass<TComponent>();
		if (!Component)
		{
			UE_LOG(LogCVehicleArea, Warning, TEXT("%s: '%s' has no %s."), SystemName, *Vehicle->GetName(), *TComponent::StaticClass()->GetName());
		}

		return Component;
	}

	AVehicleTelemetryHUD* GetTelemetryHUD(UWorld* World)
	{
		if (!World)
		{
			return nullptr;
		}

		APlayerController* PlayerController = World->GetFirstPlayerController();
		AVehicleTelemetryHUD* HUD = PlayerController ? Cast<AVehicleTelemetryHUD>(PlayerController->GetHUD()) : nullptr;

		if (!HUD)
		{
			UE_LOG(LogCVehicleArea, Warning, TEXT("HUD is not a VehicleTelemetryHUD."));
		}

		return HUD;
	}

	bool ParseToggle(const TArray<FString>& Args, int32 Index = 0)
	{
		if (!Args.IsValidIndex(Index))
		{
			return true;
		}

		return Args[Index].ToBool() || Args[Index] == TEXT("1");
	}

	FRacingLineBuildSettings GetLineBuildSettings()
	{
		FRacingLineBuildSettings Settings;

		static IConsoleVariable* GripCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("CVehicle.Line.Grip"));
		static IConsoleVariable* SpacingCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("CVehicle.Line.Spacing"));

		if (GripCVar)
		{
			Settings.LateralGrip = GripCVar->GetFloat();
		}
		if (SpacingCVar)
		{
			Settings.PointSpacing = SpacingCVar->GetFloat();
		}

		return Settings;
	}
}

static TAutoConsoleVariable<float> CVarLineGrip(
	TEXT("CVehicle.Line.Grip"),
	1.1f,
	TEXT("Lateral grip the racing line builder assumes when turning curvature into a cornering speed. Raise to make the AI faster."),
	ECVF_Default);

static TAutoConsoleVariable<float> CVarLineSpacing(
	TEXT("CVehicle.Line.Spacing"),
	250.0f,
	TEXT("Distance in cm between racing line stations. Smaller is more faithful and more expensive."),
	ECVF_Default);

static FAutoConsoleCommandWithWorldAndArgs GCmdHUDPage(
	TEXT("CVehicle.HUD.Page"),
	TEXT("Sets the telemetry page: 0 off, 1 core, 2 wheels, 3 balance, 4 graphs."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		AVehicleTelemetryHUD* HUD = VehicleDebugCommands::GetTelemetryHUD(World);
		if (!HUD)
		{
			return;
		}

		if (Args.Num() == 0)
		{
			HUD->CyclePage();
		}
		else
		{
			const int32 Page = FMath::Clamp(FCString::Atoi(*Args[0]), 0, static_cast<int32>(EVehicleHUDPage::Graphs));
			HUD->SetPage(static_cast<EVehicleHUDPage>(Page));
		}

		UE_LOG(LogCVehicleArea, Log, TEXT("HUD page %d."), static_cast<int32>(HUD->CurrentPage));
	}));

static FAutoConsoleCommandWithWorldAndArgs GCmdHUDRacingLine(
	TEXT("CVehicle.HUD.RacingLine"),
	TEXT("Draws the reference racing line in the world, colored by its speed profile. [0|1]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		if (AVehicleTelemetryHUD* HUD = VehicleDebugCommands::GetTelemetryHUD(World))
		{
			HUD->bDrawRacingLine = VehicleDebugCommands::ParseToggle(Args);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCmdHUDSuspension(
	TEXT("CVehicle.HUD.SuspensionLoad"),
	TEXT("Draws a bar at each wheel showing its share of the vertical load. [0|1]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		if (AVehicleTelemetryHUD* HUD = VehicleDebugCommands::GetTelemetryHUD(World))
		{
			HUD->bDrawSuspensionLoad = VehicleDebugCommands::ParseToggle(Args);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCmdHUDSlip(
	TEXT("CVehicle.HUD.SlipVectors"),
	TEXT("Draws each wheel's slip direction and magnitude. [0|1]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		if (AVehicleTelemetryHUD* HUD = VehicleDebugCommands::GetTelemetryHUD(World))
		{
			HUD->bDrawSlipVectors = VehicleDebugCommands::ParseToggle(Args);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCmdTelemetryRecord(
	TEXT("CVehicle.Telemetry.Record"),
	TEXT("Starts or stops telemetry sampling. [0|1]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		UVehicleTelemetryComponent* Telemetry = VehicleDebugCommands::GetPlayerVehicleComponent<UVehicleTelemetryComponent>(World, TEXT("VehicleTelemetry"));
		if (!Telemetry)
		{
			return;
		}

		if (VehicleDebugCommands::ParseToggle(Args))
		{
			Telemetry->StartRecording();
			UE_LOG(LogCVehicleArea, Log, TEXT("Telemetry recording."));
		}
		else
		{
			Telemetry->StopRecording();
			UE_LOG(LogCVehicleArea, Log, TEXT("Telemetry stopped, %d samples."), Telemetry->GetFrameCount());
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCmdTelemetryClear(
	TEXT("CVehicle.Telemetry.Clear"),
	TEXT("Discards recorded telemetry history."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		if (UVehicleTelemetryComponent* Telemetry = VehicleDebugCommands::GetPlayerVehicleComponent<UVehicleTelemetryComponent>(World, TEXT("VehicleTelemetry")))
		{
			Telemetry->ClearHistory();
			UE_LOG(LogCVehicleArea, Log, TEXT("Telemetry cleared."));
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCmdTelemetryExport(
	TEXT("CVehicle.Telemetry.Export"),
	TEXT("Writes the telemetry history to Saved/Telemetry as CSV. [label]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		UVehicleTelemetryComponent* Telemetry = VehicleDebugCommands::GetPlayerVehicleComponent<UVehicleTelemetryComponent>(World, TEXT("VehicleTelemetry"));
		if (!Telemetry)
		{
			return;
		}

		const FString Label = Args.Num() > 0 ? Args[0] : TEXT("Session");

		FString WrittenPath;
		if (Telemetry->ExportToCSV(Label, WrittenPath))
		{
			UE_LOG(LogCVehicleArea, Log, TEXT("Telemetry exported to %s"), *WrittenPath);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCmdTelemetrySummary(
	TEXT("CVehicle.Telemetry.Summary"),
	TEXT("Logs peak and average figures for the recorded telemetry."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		UVehicleTelemetryComponent* Telemetry = VehicleDebugCommands::GetPlayerVehicleComponent<UVehicleTelemetryComponent>(World, TEXT("VehicleTelemetry"));
		if (!Telemetry)
		{
			return;
		}

		const FVehicleTelemetrySummary Summary = Telemetry->BuildSummary();

		UE_LOG(LogCVehicleArea, Log, TEXT("Telemetry summary: %d samples, %.1f s"), Summary.SampleCount, Summary.DurationSeconds);
		UE_LOG(LogCVehicleArea, Log, TEXT("  Peak speed        %.1f km/h"), Summary.PeakSpeedKPH);
		UE_LOG(LogCVehicleArea, Log, TEXT("  Peak engine       %.0f rpm"), Summary.PeakEngineRPM);
		UE_LOG(LogCVehicleArea, Log, TEXT("  Peak acceleration %.2f g"), Summary.PeakLongitudinalG);
		UE_LOG(LogCVehicleArea, Log, TEXT("  Peak braking      %.2f g"), Summary.PeakBrakingG);
		UE_LOG(LogCVehicleArea, Log, TEXT("  Peak cornering    %.2f g"), Summary.PeakLateralG);
		UE_LOG(LogCVehicleArea, Log, TEXT("  Peak combined     %.2f g"), Summary.PeakCombinedG);
		UE_LOG(LogCVehicleArea, Log, TEXT("  Peak body slip    %.1f deg"), Summary.PeakBodySlipAngle);
		UE_LOG(LogCVehicleArea, Log, TEXT("  Mean balance      %.2f deg"), Summary.MeanUndersteerBalance);
		UE_LOG(LogCVehicleArea, Log, TEXT("  Time slipping     %.0f%%"), Summary.SlipTimeFraction * 100.0f);
		UE_LOG(LogCVehicleArea, Log, TEXT("  Time airborne     %.0f%%"), Summary.AirborneTimeFraction * 100.0f);
	}));

static FAutoConsoleCommandWithWorldAndArgs GCmdAssists(
	TEXT("CVehicle.Assists"),
	TEXT("Master switch for all driver assists. [0|1]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		if (UVehicleDriverAssistComponent* Assists = VehicleDebugCommands::GetPlayerVehicleComponent<UVehicleDriverAssistComponent>(World, TEXT("VehicleAssists")))
		{
			const bool bEnabled = VehicleDebugCommands::ParseToggle(Args);
			Assists->SetAssistsEnabled(bEnabled);
			UE_LOG(LogCVehicleArea, Log, TEXT("Assists %s."), bEnabled ? TEXT("on") : TEXT("off"));
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCmdAssistTC(
	TEXT("CVehicle.Assists.TractionControl"),
	TEXT("Toggles closed-loop traction control. [0|1]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		if (UVehicleDriverAssistComponent* Assists = VehicleDebugCommands::GetPlayerVehicleComponent<UVehicleDriverAssistComponent>(World, TEXT("VehicleAssists")))
		{
			Assists->bTractionControlEnabled = VehicleDebugCommands::ParseToggle(Args);
			UE_LOG(LogCVehicleArea, Log, TEXT("Traction control %s."), Assists->bTractionControlEnabled ? TEXT("on") : TEXT("off"));
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCmdAssistESC(
	TEXT("CVehicle.Assists.StabilityControl"),
	TEXT("Toggles yaw stability control. [0|1]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		if (UVehicleDriverAssistComponent* Assists = VehicleDebugCommands::GetPlayerVehicleComponent<UVehicleDriverAssistComponent>(World, TEXT("VehicleAssists")))
		{
			Assists->bStabilityControlEnabled = VehicleDebugCommands::ParseToggle(Args);
			UE_LOG(LogCVehicleArea, Log, TEXT("Stability control %s."), Assists->bStabilityControlEnabled ? TEXT("on") : TEXT("off"));
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCmdAssistABS(
	TEXT("CVehicle.Assists.ABS"),
	TEXT("Toggles the supervisory ABS layer. [0|1]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		if (UVehicleDriverAssistComponent* Assists = VehicleDebugCommands::GetPlayerVehicleComponent<UVehicleDriverAssistComponent>(World, TEXT("VehicleAssists")))
		{
			Assists->bABSEnabled = VehicleDebugCommands::ParseToggle(Args);
			UE_LOG(LogCVehicleArea, Log, TEXT("ABS %s."), Assists->bABSEnabled ? TEXT("on") : TEXT("off"));
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCmdAssistAero(
	TEXT("CVehicle.Assists.ActiveAero"),
	TEXT("Toggles speed and load dependent downforce. [0|1]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		if (UVehicleDriverAssistComponent* Assists = VehicleDebugCommands::GetPlayerVehicleComponent<UVehicleDriverAssistComponent>(World, TEXT("VehicleAssists")))
		{
			Assists->bActiveAeroEnabled = VehicleDebugCommands::ParseToggle(Args);
			UE_LOG(LogCVehicleArea, Log, TEXT("Active aero %s."), Assists->bActiveAeroEnabled ? TEXT("on") : TEXT("off"));
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCmdAssistLaunch(
	TEXT("CVehicle.Assists.ArmLaunchControl"),
	TEXT("Arms launch control. Disengages by itself once the car is away."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		if (UVehicleDriverAssistComponent* Assists = VehicleDebugCommands::GetPlayerVehicleComponent<UVehicleDriverAssistComponent>(World, TEXT("VehicleAssists")))
		{
			Assists->bLaunchControlEnabled = true;
			Assists->ArmLaunchControl();
			UE_LOG(LogCVehicleArea, Log, TEXT("Launch control armed."));
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCmdLineStart(
	TEXT("CVehicle.Line.Start"),
	TEXT("Starts recording a reference lap. Drive a full lap, then run CVehicle.Line.Finish."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		if (UVehicleLapRecorderComponent* Recorder = VehicleDebugCommands::GetPlayerVehicleComponent<UVehicleLapRecorderComponent>(World, TEXT("VehicleLapRecorder")))
		{
			Recorder->BeginLap();
			UE_LOG(LogCVehicleArea, Log, TEXT("Recording lap. Return to your start point, then Line.Finish."));
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCmdLineFinish(
	TEXT("CVehicle.Line.Finish"),
	TEXT("Ends the reference lap, builds a racing line from it and hands it to the race director."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		UVehicleLapRecorderComponent* Recorder = VehicleDebugCommands::GetPlayerVehicleComponent<UVehicleLapRecorderComponent>(World, TEXT("VehicleLapRecorder"));
		if (!Recorder || !World)
		{
			return;
		}

		if (!Recorder->IsRecording())
		{
			UE_LOG(LogCVehicleArea, Warning, TEXT("Not recording. Run Line.Start first."));
			return;
		}

		const float Duration = Recorder->GetLapInProgress().GetDuration();
		Recorder->CompleteLap(Duration);

		const FVehicleLapRecording& Lap = Recorder->GetLastLap();
		if (!Lap.IsValid())
		{
			UE_LOG(LogCVehicleArea, Warning, TEXT("Lap too short to build a line."));
			return;
		}

		const FRacingLine Line = FRacingLine::BuildFromRecording(Lap, VehicleDebugCommands::GetLineBuildSettings());
		if (!Line.IsValid())
		{
			UE_LOG(LogCVehicleArea, Warning, TEXT("Could not build a racing line."));
			return;
		}

		if (URaceDirectorSubsystem* Director = World->GetSubsystem<URaceDirectorSubsystem>())
		{
			Director->SetReferenceLine(Line);
			UE_LOG(LogCVehicleArea, Log, TEXT("Racing line set: %d points, %.0f m, %.2f s."),
				Line.Points.Num(), Line.TotalLength / 100.0f, Duration);
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCmdGhost(
	TEXT("CVehicle.Ghost.Play"),
	TEXT("Spawns a ghost replaying the lap you last recorded. Pass 'best' to replay your fastest instead."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		UVehicleLapRecorderComponent* Recorder = VehicleDebugCommands::GetPlayerVehicleComponent<UVehicleLapRecorderComponent>(World, TEXT("VehicleLapRecorder"));
		if (!Recorder || !World)
		{
			return;
		}

		const bool bWantBest = Args.Num() > 0 && Args[0].Equals(TEXT("best"), ESearchCase::IgnoreCase);

		const FVehicleLapRecording& Lap = bWantBest ? Recorder->GetBestLap() : Recorder->GetLastLap();
		if (!Lap.IsValid())
		{
			UE_LOG(LogCVehicleArea, Warning, TEXT("No %s lap recorded."),
				bWantBest ? TEXT("best") : TEXT("completed"));
			return;
		}

		AWheeledVehiclePawn* PlayerVehicle = Cast<AWheeledVehiclePawn>(VehicleDebugCommands::GetPlayerVehicle(World));
		if (!PlayerVehicle)
		{
			UE_LOG(LogCVehicleArea, Warning, TEXT("No vehicle to copy for the ghost."));
			return;
		}

		for (TActorIterator<AVehicleGhostActor> It(World); It; ++It)
		{
			It->Destroy();
		}

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AVehicleGhostActor* Ghost = World->SpawnActor<AVehicleGhostActor>(AVehicleGhostActor::StaticClass(), FTransform::Identity, SpawnParams);
		if (!Ghost)
		{
			return;
		}

		Ghost->CopyAppearanceFrom(PlayerVehicle);
		Ghost->SetRecording(Lap);
		Ghost->StartPlayback();

		UE_LOG(LogCVehicleArea, Log, TEXT("Ghost: %s lap, %.2f s, %d samples, start %s."),
			bWantBest ? TEXT("best") : TEXT("last"), Lap.LapTime, Lap.Samples.Num(),
			*Ghost->GetActorLocation().ToCompactString());
	}));

static FAutoConsoleCommandWithWorldAndArgs GCmdRaceSpawnAI(
	TEXT("CVehicle.Race.SpawnAI"),
	TEXT("Spawns AI opponents behind you on the racing line. [count] [skill 0..1]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World)
		{
			return;
		}

		URaceDirectorSubsystem* Director = World->GetSubsystem<URaceDirectorSubsystem>();
		if (!Director || !Director->HasReferenceLine())
		{
			UE_LOG(LogCVehicleArea, Warning, TEXT("No racing line. Record one first."));
			return;
		}

		APawn* PlayerVehicle = VehicleDebugCommands::GetPlayerVehicle(World);
		if (!PlayerVehicle)
		{
			UE_LOG(LogCVehicleArea, Warning, TEXT("No player vehicle to copy."));
			return;
		}

		const int32 Count = Args.Num() > 0 ? FMath::Clamp(FCString::Atoi(*Args[0]), 1, 16) : 3;
		const float Skill = Args.Num() > 1 ? FMath::Clamp(FCString::Atof(*Args[1]), 0.0f, 1.0f) : 0.9f;

		const FRacingLine& Line = Director->GetReferenceLine();

		const int32 PlayerStation = Line.FindNearestPoint(PlayerVehicle->GetActorLocation(), INDEX_NONE);
		if (PlayerStation == INDEX_NONE)
		{
			return;
		}

		int32 Spawned = 0;

		for (int32 Index = 0; Index < Count; ++Index)
		{
			const float BackDistance = -600.0f * (Index + 1);
			const int32 GridStation = Line.AdvanceIndex(PlayerStation, BackDistance);

			if (!Line.Points.IsValidIndex(GridStation))
			{
				continue;
			}

			const FRacingLinePoint& GridPoint = Line.Points[GridStation];
			const FVector Right = FVector::CrossProduct(FVector::UpVector, GridPoint.Tangent).GetSafeNormal();
			const float LateralOffset = ((Index % 2) == 0) ? 220.0f : -220.0f;

			FTransform SpawnTransform;
			SpawnTransform.SetLocation(GridPoint.Location + Right * LateralOffset + FVector(0, 0, 60.0f));
			SpawnTransform.SetRotation(GridPoint.Tangent.ToOrientationQuat());

			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

			APawn* AIVehicle = World->SpawnActor<APawn>(PlayerVehicle->GetClass(), SpawnTransform, SpawnParams);
			if (!AIVehicle)
			{
				continue;
			}

			AVehicleAIController* AIController = World->SpawnActor<AVehicleAIController>(AVehicleAIController::StaticClass(), SpawnTransform, SpawnParams);
			if (!AIController)
			{
				AIVehicle->Destroy();
				continue;
			}

			AIController->Possess(AIVehicle);
			AIController->SkillLevel = Skill;
			AIController->SetRacingLine(Line);

			const bool bHoldOnGrid = Director->GetRaceState() == ERaceState::Countdown;
			AIController->SetDrivingEnabled(!bHoldOnGrid);

			Director->RegisterCompetitor(AIVehicle, FString::Printf(TEXT("AI %d"), Index + 1), false);
			++Spawned;
		}

		Director->RegisterCompetitor(PlayerVehicle, TEXT("Player"), true);

		UE_LOG(LogCVehicleArea, Log, TEXT("Spawned %d AI at skill %.2f."), Spawned, Skill);
	}));

static FAutoConsoleCommandWithWorldAndArgs GCmdRaceStart(
	TEXT("CVehicle.Race.Start"),
	TEXT("Starts a race. [laps] [countdown seconds]"),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World)
		{
			return;
		}

		URaceDirectorSubsystem* Director = World->GetSubsystem<URaceDirectorSubsystem>();
		if (!Director)
		{
			return;
		}

		if (APawn* PlayerVehicle = VehicleDebugCommands::GetPlayerVehicle(World))
		{
			Director->RegisterCompetitor(PlayerVehicle, TEXT("Player"), true);
		}

		const int32 Laps = Args.Num() > 0 ? FMath::Max(1, FCString::Atoi(*Args[0])) : 3;
		const float Countdown = Args.Num() > 1 ? FMath::Max(0.0f, FCString::Atof(*Args[1])) : 3.0f;

		Director->StartRace(Laps, Countdown);
	}));

static FAutoConsoleCommandWithWorldAndArgs GCmdRaceEnd(
	TEXT("CVehicle.Race.End"),
	TEXT("Ends the current race session."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		if (World)
		{
			if (URaceDirectorSubsystem* Director = World->GetSubsystem<URaceDirectorSubsystem>())
			{
				Director->EndRace();
				UE_LOG(LogCVehicleArea, Log, TEXT("Race ended."));
			}
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCmdRaceStandings(
	TEXT("CVehicle.Race.Standings"),
	TEXT("Logs the current running order."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		if (!World)
		{
			return;
		}

		URaceDirectorSubsystem* Director = World->GetSubsystem<URaceDirectorSubsystem>();
		if (!Director)
		{
			return;
		}

		UE_LOG(LogCVehicleArea, Log, TEXT("Standings (%.1f s)"), Director->GetRaceElapsedTime());

		for (const FRaceCompetitor& Competitor : Director->GetStandings())
		{
			UE_LOG(LogCVehicleArea, Log, TEXT("  %d. %-10s  laps %d/%d  last %s  best %s%s"),
				Competitor.Position,
				*Competitor.DisplayName,
				Competitor.LapsCompleted,
				Director->GetLapCount(),
				*URaceDirectorSubsystem::FormatLapTime(Competitor.LastLapTime),
				*URaceDirectorSubsystem::FormatLapTime(Competitor.BestLapTime),
				Competitor.bFinished ? TEXT("  FINISHED") : TEXT(""));
		}
	}));

static FAutoConsoleCommandWithWorldAndArgs GCmdRaceClear(
	TEXT("CVehicle.Race.Clear"),
	TEXT("Clears all competitors and returns to free roam."),
	FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
	{
		if (World)
		{
			if (URaceDirectorSubsystem* Director = World->GetSubsystem<URaceDirectorSubsystem>())
			{
				Director->ClearSession();
				UE_LOG(LogCVehicleArea, Log, TEXT("Race cleared."));
			}
		}
	}));
