#pragma once

#include "CoreMinimal.h"
#include "ChaosVehicleWheel.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "VehicleTuningTypes.generated.h"

UENUM(BlueprintType)
enum class EVehicleTuningAxle : uint8
{
	Front,
	Rear
};

USTRUCT(BlueprintType)
struct FVehicleAxleTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel", meta = (ClampMin = "1.0", Units = "cm"))
	float WheelRadius = 35.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel", meta = (ClampMin = "1.0", Units = "cm"))
	float WheelWidth = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wheel", meta = (ClampMin = "1.0", Units = "kg"))
	float WheelMass = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grip", meta = (ClampMin = "0.0"))
	float FrictionForceMultiplier = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grip", meta = (ClampMin = "0.0"))
	float CorneringStiffness = 1000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grip", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SideSlipModifier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grip", meta = (ClampMin = "0.0"))
	float SlipThreshold = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grip", meta = (ClampMin = "0.0"))
	float SkidThreshold = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering")
	bool bAffectedBySteering = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering", meta = (ClampMin = "0.0", ClampMax = "90.0", Units = "deg"))
	float MaxSteerAngle = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brakes", meta = (ClampMin = "0.0"))
	float MaxBrakeTorque = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brakes", meta = (ClampMin = "0.0"))
	float MaxHandBrakeTorque = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brakes")
	bool bAffectedByBrake = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Brakes")
	bool bAffectedByHandbrake = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive")
	bool bAffectedByEngine = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drive")
	ETorqueCombineMethod ExternalTorqueCombineMethod = ETorqueCombineMethod::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assists")
	bool bABSEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assists")
	bool bTractionControlEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension", meta = (ClampMin = "0.0"))
	float SpringRate = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension")
	float SpringPreload = 50.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float SuspensionDampingRatio = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension", meta = (ClampMin = "0.0", Units = "cm"))
	float SuspensionMaxRaise = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension", meta = (ClampMin = "0.0", Units = "cm"))
	float SuspensionMaxDrop = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float RollbarScaling = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WheelLoadRatio = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Suspension")
	ESweepShape SweepShape = ESweepShape::Raycast;
};

USTRUCT(BlueprintType)
struct FVehicleDrivetrainTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine", meta = (ClampMin = "0.0", Units = "Nm"))
	float MaxTorque = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine", meta = (ClampMin = "1.0"))
	float MaxRPM = 6000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine", meta = (ClampMin = "1.0"))
	float EngineIdleRPM = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine", meta = (ClampMin = "0.0"))
	float EngineBrakeEffect = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine", meta = (ClampMin = "0.01"))
	float EngineRevUpMOI = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Engine", meta = (ClampMin = "0.01"))
	float EngineRevDownRate = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmission")
	bool bUseAutomaticGears = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmission")
	bool bUseAutoReverse = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmission", meta = (ClampMin = "0.01"))
	float FinalRatio = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmission", meta = (ClampMin = "0.0"))
	float ChangeUpRPM = 5500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmission", meta = (ClampMin = "0.0"))
	float ChangeDownRPM = 2000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmission", meta = (ClampMin = "0.0", Units = "s"))
	float GearChangeTime = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmission", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float TransmissionEfficiency = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmission")
	TArray<float> ForwardGearRatios = { 4.25f, 2.52f, 1.66f, 1.22f, 1.0f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Transmission")
	TArray<float> ReverseGearRatios = { 4.04f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Differential")
	EVehicleDifferential DifferentialType = EVehicleDifferential::RearWheelDrive;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Differential", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FrontRearSplit = 0.5f;
};

USTRUCT(BlueprintType)
struct FVehicleChassisTuning
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chassis", meta = (ClampMin = "1.0", Units = "kg"))
	float Mass = 1500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chassis", meta = (ClampMin = "0.0", Units = "cm"))
	float ChassisHeight = 144.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chassis", meta = (ClampMin = "0.0", Units = "cm"))
	float ChassisWidth = 180.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics", meta = (ClampMin = "0.0"))
	float DragCoefficient = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Aerodynamics", meta = (ClampMin = "0.0"))
	float DownforceCoefficient = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chassis")
	bool bEnableCenterOfMassOverride = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chassis", meta = (EditCondition = "bEnableCenterOfMassOverride"))
	FVector CenterOfMassOverride = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering")
	ESteeringType SteeringType = ESteeringType::Ackermann;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Steering", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SteeringAngleRatio = 0.7f;
};
