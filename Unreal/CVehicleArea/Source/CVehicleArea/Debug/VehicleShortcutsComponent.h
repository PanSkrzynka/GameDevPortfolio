#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "VehicleShortcutsComponent.generated.h"

class UInputComponent;

UCLASS(ClassGroup = (Vehicle), meta = (BlueprintSpawnableComponent))
class CVEHICLEAREA_API UVehicleShortcutsComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UVehicleShortcutsComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcuts")
	bool bEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcuts", meta = (ClampMin = "1"))
	int32 RaceLapCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcuts|Keys")
	FKey CyclePageKey = EKeys::Tab;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcuts|Keys")
	FKey SuspensionLoadKey = EKeys::One;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcuts|Keys")
	FKey SlipVectorsKey = EKeys::Two;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcuts|Keys")
	FKey RacingLineKey = EKeys::Three;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcuts|Keys")
	FKey AssistsKey = EKeys::Four;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcuts|Keys")
	FKey TelemetryRecordKey = EKeys::Five;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcuts|Keys")
	FKey StartLapKey = EKeys::Z;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcuts|Keys")
	FKey FinishLapKey = EKeys::X;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcuts|Keys")
	FKey PlayGhostKey = EKeys::V;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcuts|Keys")
	FKey SpawnOpponentKey = EKeys::B;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcuts|Keys")
	FKey StartRaceKey = EKeys::N;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shortcuts|Keys")
	FKey ExportTelemetryKey = EKeys::M;

	UFUNCTION(BlueprintCallable, Category = "Shortcuts")
	void BindShortcuts(UInputComponent* InputComponent);

protected:

	void CycleHUDPage();
	void ToggleSuspensionLoad();
	void ToggleSlipVectors();
	void ToggleRacingLineDraw();
	void ToggleAssists();
	void ToggleTelemetryRecording();
	void StartLapRecording();
	void FinishLapRecording();
	void PlayBestLapGhost();
	void SpawnOpponent();
	void StartRace();
	void ExportTelemetry();

private:

	void Run(const FString& Command);

	void RunToggle(const TCHAR* Command, bool& State, const TCHAR* Label);

	void Notify(const FString& Message) const;

	bool bAssistsEnabled = true;
	bool bTelemetryRecording = true;
	bool bSuspensionLoadShown = false;
	bool bSlipVectorsShown = false;
	bool bRacingLineShown = false;
};
