#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VehicleReplayTypes.h"
#include "VehicleLapRecorderComponent.generated.h"

class UChaosWheeledVehicleMovementComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLapRecorded, float, LapTime, bool, bIsNewBest);

UCLASS(ClassGroup = (Vehicle), meta = (BlueprintSpawnableComponent))
class CVEHICLEAREA_API UVehicleLapRecorderComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UVehicleLapRecorderComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recording", meta = (ClampMin = "5.0", ClampMax = "120.0"))
	float SampleRateHz = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recording", meta = (ClampMin = "100"))
	int32 MaxSamplesPerLap = 12000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recording")
	FString DriverLabel = TEXT("Player");

	UPROPERTY(BlueprintAssignable, Category = "Recording")
	FOnLapRecorded OnLapRecorded;

	UFUNCTION(BlueprintCallable, Category = "Recording")
	void BeginLap();

	UFUNCTION(BlueprintCallable, Category = "Recording")
	bool CompleteLap(float LapTime);

	UFUNCTION(BlueprintCallable, Category = "Recording")
	void AbandonLap();

	UFUNCTION(BlueprintPure, Category = "Recording")
	bool IsRecording() const { return bIsRecording; }

	const FVehicleLapRecording& GetBestLap() const { return BestLap; }

	const FVehicleLapRecording& GetLastLap() const { return LastLap; }

	const FVehicleLapRecording& GetLapInProgress() const { return CurrentLap; }

	UFUNCTION(BlueprintPure, Category = "Recording")
	bool HasBestLap() const { return BestLap.IsValid(); }

	UFUNCTION(BlueprintPure, Category = "Recording")
	float GetBestLapTime() const { return BestLap.IsValid() ? BestLap.LapTime : 0.0f; }

	UFUNCTION(BlueprintCallable, Category = "Recording")
	void ClearBestLap();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:

	void CaptureSample();

	UPROPERTY(Transient)
	TObjectPtr<UChaosWheeledVehicleMovementComponent> MovementComponent;

	UPROPERTY(Transient)
	FVehicleLapRecording CurrentLap;

	UPROPERTY(Transient)
	FVehicleLapRecording BestLap;

	UPROPERTY(Transient)
	FVehicleLapRecording LastLap;

	bool bIsRecording = false;

	float ElapsedTime = 0.0f;

	float SampleTimer = 0.0f;

	bool bWarnedSampleLimit = false;
};
