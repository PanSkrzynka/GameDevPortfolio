// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TimeTrialUI.generated.h"

class UTimeTrialStartUI;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStartRaceDelegate);

UCLASS(abstract)
class UTimeTrialUI : public UUserWidget
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, Category="Start Countdown")
	TSubclassOf<UTimeTrialStartUI> StartUIClass;

	float LastLapTime = 0.0f;

	float BestLapTime = 0.0f;

	float LapStartTime = 0.0f;

	int32 CurrentLap = 0;

public:

	FStartRaceDelegate OnRaceStart;

protected:

	virtual void NativeConstruct() override;

public:

	void UpdateLapCount(int32 Lap, float NewLapStartTime);

	UFUNCTION(BlueprintImplementableEvent, Category="Time Trial", meta = (DisplayName = "Update Laps"))
	void BP_UpdateLaps();

protected:

	UFUNCTION()
	void StartRace();

	UFUNCTION(BlueprintPure, Category="Time Trial")
	int32 GetCurrentLap() const { return CurrentLap; };

	UFUNCTION(BlueprintPure, Category="Time Trial")
	float GetBestLapTime() const { return BestLapTime; };

	UFUNCTION(BlueprintPure, Category="Time Trial")
	float GetLapStartTime() const { return LapStartTime; };
};
