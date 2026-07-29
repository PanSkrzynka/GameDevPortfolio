#pragma once

#include "CoreMinimal.h"
#include "VehicleTelemetryTypes.generated.h"

USTRUCT(BlueprintType)
struct FWheelTelemetrySample
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Contact")
	bool bInContact = false;

	UPROPERTY(BlueprintReadOnly, Category = "Suspension")
	float NormalizedSuspensionLength = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Suspension", meta = (Units = "cm"))
	float SuspensionOffset = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Suspension")
	float SpringForce = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Suspension")
	float NormalizedLoad = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Grip", meta = (Units = "deg"))
	float SlipAngle = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Grip")
	float LongitudinalSlipRatio = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Grip")
	float SlipMagnitude = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Grip")
	float SkidMagnitude = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Grip")
	bool bIsSlipping = false;

	UPROPERTY(BlueprintReadOnly, Category = "Grip")
	bool bIsSkidding = false;

	UPROPERTY(BlueprintReadOnly, Category = "Assists")
	bool bABSActivated = false;

	UPROPERTY(BlueprintReadOnly, Category = "Torque", meta = (Units = "Nm"))
	float DriveTorque = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Torque", meta = (Units = "Nm"))
	float BrakeTorque = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "State", meta = (Units = "deg/s"))
	float AngularVelocity = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "State", meta = (Units = "deg"))
	float SteerAngle = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Contact")
	FName SurfaceName = NAME_None;
};

USTRUCT(BlueprintType)
struct FVehicleTelemetryFrame
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Frame")
	float TimeSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Chassis", meta = (Units = "km/h"))
	float SpeedKPH = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Drivetrain")
	float EngineRPM = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Drivetrain", meta = (Units = "Nm"))
	float EngineTorque = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Drivetrain")
	float TransmissionRPM = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Drivetrain")
	int32 Gear = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Input")
	float ThrottleInput = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Input")
	float BrakeInput = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Input")
	float SteeringInput = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Input")
	bool bHandbrake = false;

	UPROPERTY(BlueprintReadOnly, Category = "Chassis")
	FVector AccelerationG = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Chassis", meta = (Units = "deg/s"))
	float YawRate = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Chassis", meta = (Units = "deg/s"))
	float TargetYawRate = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Chassis", meta = (Units = "deg"))
	float BodySlipAngle = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Balance", meta = (Units = "deg"))
	float UndersteerBalance = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Balance")
	float FrontAxleLoadFraction = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Balance")
	float LeftSideLoadFraction = 0.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Contact")
	int32 WheelsInContact = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wheels")
	TArray<FWheelTelemetrySample> Wheels;

	float GetCombinedG() const
	{
		return FMath::Sqrt(AccelerationG.X * AccelerationG.X + AccelerationG.Y * AccelerationG.Y);
	}
};

USTRUCT(BlueprintType)
struct FVehicleTelemetrySummary
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Peaks", meta = (Units = "km/h"))
	float PeakSpeedKPH = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Peaks")
	float PeakLongitudinalG = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Peaks")
	float PeakBrakingG = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Peaks")
	float PeakLateralG = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Peaks")
	float PeakCombinedG = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Peaks", meta = (Units = "deg"))
	float PeakBodySlipAngle = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Peaks")
	float PeakEngineRPM = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Averages")
	float SlipTimeFraction = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Averages")
	float AirborneTimeFraction = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Averages", meta = (Units = "deg"))
	float MeanUndersteerBalance = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Averages", meta = (Units = "s"))
	float DurationSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Averages")
	int32 SampleCount = 0;
};
