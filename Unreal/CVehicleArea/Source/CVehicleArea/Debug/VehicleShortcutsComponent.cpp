#include "VehicleShortcutsComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"

UVehicleShortcutsComponent::UVehicleShortcutsComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UVehicleShortcutsComponent::BindShortcuts(UInputComponent* InputComponent)
{
	if (!InputComponent || !bEnabled)
	{
		return;
	}

	auto Bind = [InputComponent, this](const FKey& Key, void (UVehicleShortcutsComponent::* Handler)())
	{
		if (Key.IsValid())
		{
			InputComponent->BindKey(Key, IE_Pressed, this, Handler);
		}
	};

	Bind(CyclePageKey, &UVehicleShortcutsComponent::CycleHUDPage);
	Bind(SuspensionLoadKey, &UVehicleShortcutsComponent::ToggleSuspensionLoad);
	Bind(SlipVectorsKey, &UVehicleShortcutsComponent::ToggleSlipVectors);
	Bind(RacingLineKey, &UVehicleShortcutsComponent::ToggleRacingLineDraw);
	Bind(AssistsKey, &UVehicleShortcutsComponent::ToggleAssists);
	Bind(TelemetryRecordKey, &UVehicleShortcutsComponent::ToggleTelemetryRecording);
	Bind(StartLapKey, &UVehicleShortcutsComponent::StartLapRecording);
	Bind(FinishLapKey, &UVehicleShortcutsComponent::FinishLapRecording);
	Bind(PlayGhostKey, &UVehicleShortcutsComponent::PlayBestLapGhost);
	Bind(SpawnOpponentKey, &UVehicleShortcutsComponent::SpawnOpponent);
	Bind(StartRaceKey, &UVehicleShortcutsComponent::StartRace);
	Bind(ExportTelemetryKey, &UVehicleShortcutsComponent::ExportTelemetry);
}

void UVehicleShortcutsComponent::Run(const FString& Command)
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	APlayerController* Controller = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;

	if (Controller)
	{
		Controller->ConsoleCommand(Command, false);
	}
}

void UVehicleShortcutsComponent::RunToggle(const TCHAR* Command, bool& State, const TCHAR* Label)
{
	State = !State;
	Run(FString::Printf(TEXT("%s %d"), Command, State ? 1 : 0));
	Notify(FString::Printf(TEXT("%s %s"), Label, State ? TEXT("ON") : TEXT("OFF")));
}

void UVehicleShortcutsComponent::Notify(const FString& Message) const
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.5f, FColor::Cyan, Message);
	}
}

void UVehicleShortcutsComponent::CycleHUDPage()
{
	Run(TEXT("CVehicle.HUD.Page"));
}

void UVehicleShortcutsComponent::ToggleSuspensionLoad()
{
	RunToggle(TEXT("CVehicle.HUD.SuspensionLoad"), bSuspensionLoadShown, TEXT("Suspension load"));
}

void UVehicleShortcutsComponent::ToggleSlipVectors()
{
	RunToggle(TEXT("CVehicle.HUD.SlipVectors"), bSlipVectorsShown, TEXT("Slip vectors"));
}

void UVehicleShortcutsComponent::ToggleRacingLineDraw()
{
	RunToggle(TEXT("CVehicle.HUD.RacingLine"), bRacingLineShown, TEXT("Racing line"));
}

void UVehicleShortcutsComponent::ToggleAssists()
{
	RunToggle(TEXT("CVehicle.Assists"), bAssistsEnabled, TEXT("Driver assists"));
}

void UVehicleShortcutsComponent::ToggleTelemetryRecording()
{
	RunToggle(TEXT("CVehicle.Telemetry.Record"), bTelemetryRecording, TEXT("Telemetry recording"));
}

void UVehicleShortcutsComponent::StartLapRecording()
{
	Run(TEXT("CVehicle.Line.Start"));
	Notify(TEXT("Recording lap - drive a clean lap, then press the finish key"));
}

void UVehicleShortcutsComponent::FinishLapRecording()
{
	Run(TEXT("CVehicle.Line.Finish"));
	Notify(TEXT("Racing line built from recorded lap"));
}

void UVehicleShortcutsComponent::PlayBestLapGhost()
{
	Run(TEXT("CVehicle.Ghost.Play"));
}

void UVehicleShortcutsComponent::SpawnOpponent()
{
	Run(TEXT("CVehicle.Race.SpawnAI 1"));
}

void UVehicleShortcutsComponent::StartRace()
{
	Run(FString::Printf(TEXT("CVehicle.Race.Start %d"), FMath::Max(1, RaceLapCount)));
}

void UVehicleShortcutsComponent::ExportTelemetry()
{
	Run(TEXT("CVehicle.Telemetry.Export"));
}
