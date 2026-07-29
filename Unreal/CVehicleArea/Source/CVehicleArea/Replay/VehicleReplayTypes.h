#pragma once

#include "CoreMinimal.h"
#include "SnapshotData.h"
#include "VehicleReplayTypes.generated.h"

USTRUCT(BlueprintType)
struct FVehicleReplaySample
{
	GENERATED_BODY()

	UPROPERTY()
	float TimeSeconds = 0.0f;

	UPROPERTY()
	FWheeledSnaphotData Snapshot;

	UPROPERTY()
	float Throttle = 0.0f;

	UPROPERTY()
	float Brake = 0.0f;

	UPROPERTY()
	float Steering = 0.0f;

	UPROPERTY()
	bool bHandbrake = false;
};

USTRUCT(BlueprintType)
struct FVehicleLapRecording
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Lap")
	FString DriverLabel;

	UPROPERTY(BlueprintReadOnly, Category = "Lap", meta = (Units = "s"))
	float LapTime = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Lap")
	bool bComplete = false;

	UPROPERTY()
	TArray<FVehicleReplaySample> Samples;

	bool IsValid() const { return Samples.Num() >= 2; }

	float GetDuration() const
	{
		return Samples.Num() >= 2 ? Samples.Last().TimeSeconds - Samples[0].TimeSeconds : 0.0f;
	}
};
