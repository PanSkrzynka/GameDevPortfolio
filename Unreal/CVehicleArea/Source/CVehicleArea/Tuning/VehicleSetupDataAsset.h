#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VehicleTuningTypes.h"
#include "VehicleSetupDataAsset.generated.h"

class UChaosWheeledVehicleMovementComponent;

UCLASS(BlueprintType)
class CVEHICLEAREA_API UVehicleSetupDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity")
	FText SetupName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Identity", meta = (MultiLine = "true"))
	FText Notes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	FVehicleChassisTuning Chassis;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	FVehicleDrivetrainTuning Drivetrain;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	FVehicleAxleTuning FrontAxle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Setup")
	FVehicleAxleTuning RearAxle;

	const FVehicleAxleTuning& GetAxleTuning(EVehicleTuningAxle Axle) const;

	UFUNCTION(BlueprintCallable, Category = "Vehicle Setup")
	void ApplyStaticSetup(UChaosWheeledVehicleMovementComponent* MovementComponent) const;

	UFUNCTION(BlueprintCallable, Category = "Vehicle Setup")
	void ApplyLiveSetup(UChaosWheeledVehicleMovementComponent* MovementComponent) const;

	UFUNCTION(BlueprintCallable, Category = "Vehicle Setup")
	void ApplyTo(UChaosWheeledVehicleMovementComponent* MovementComponent, bool bRecreatePhysicsState = false) const;

	UFUNCTION(BlueprintCallable, Category = "Vehicle Setup")
	void CaptureFrom(UChaosWheeledVehicleMovementComponent* MovementComponent);

	UFUNCTION(BlueprintPure, Category = "Vehicle Setup")
	float GetFrontBrakeBias() const;

	UFUNCTION(BlueprintPure, Category = "Vehicle Setup")
	float GetTopSpeedKPH() const;

private:

	void ApplyAxleToWheel(UChaosWheeledVehicleMovementComponent* MovementComponent, int32 WheelIndex, const FVehicleAxleTuning& Axle) const;
};
