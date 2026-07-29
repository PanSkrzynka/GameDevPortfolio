#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ChaosVehicleWheel.h"
#include "VehicleDriverAssistComponent.generated.h"

class UChaosWheeledVehicleMovementComponent;
class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class EVehicleHandlingState : uint8
{

	Neutral,

	Understeer,

	Oversteer
};

USTRUCT(BlueprintType)
struct FVehicleAssistWheelState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Assists", meta = (Units = "Nm"))
	float TractionControlTorque = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Assists", meta = (Units = "Nm"))
	float StabilityBrakeTorque = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Assists", meta = (Units = "Nm"))
	float ABSReleaseTorque = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Assists")
	float SlipRatio = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Assists")
	float TractionControlAmount = 0.0f;
};

USTRUCT(BlueprintType)
struct FVehicleAssistState
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Assists")
	EVehicleHandlingState HandlingState = EVehicleHandlingState::Neutral;

	UPROPERTY(BlueprintReadOnly, Category = "Assists", meta = (Units = "deg/s"))
	float YawRateError = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Assists", meta = (Units = "deg/s"))
	float TargetYawRate = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Assists")
	bool bStabilityControlActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Assists")
	bool bTractionControlActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Assists")
	bool bABSActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Assists")
	bool bLaunchControlActive = false;

	UPROPERTY(BlueprintReadOnly, Category = "Assists")
	float ActiveDownforceCoefficient = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Assists")
	TArray<FVehicleAssistWheelState> Wheels;
};

UCLASS(ClassGroup = (Vehicle), meta = (BlueprintSpawnableComponent))
class CVEHICLEAREA_API UVehicleDriverAssistComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UVehicleDriverAssistComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assists")
	bool bAssistsEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traction Control")
	bool bTractionControlEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traction Control", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float TractionControlTargetSlip = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traction Control", meta = (ClampMin = "0.0"))
	float TractionControlGain = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traction Control", meta = (ClampMin = "0.0"))
	float TractionControlIntegralGain = 6000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Traction Control", meta = (ClampMin = "0.0"))
	float TractionControlIntegralMax = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stability Control")
	bool bStabilityControlEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stability Control", meta = (ClampMin = "0.0", Units = "deg/s"))
	float YawRateDeadband = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stability Control", meta = (ClampMin = "0.0"))
	float StabilityControlGain = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stability Control", meta = (ClampMin = "0.0", Units = "Nm"))
	float StabilityMaxBrakeTorque = 2500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stability Control", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float AssumedLateralGrip = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stability Control", meta = (ClampMin = "0.0", Units = "km/h"))
	float StabilityMinSpeed = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ABS")
	bool bABSEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ABS", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float ABSTargetSlip = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ABS", meta = (ClampMin = "0.0"))
	float ABSGain = 6000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Active Aero")
	bool bActiveAeroEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Active Aero", meta = (ClampMin = "0.0"))
	float MinDownforce = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Active Aero", meta = (ClampMin = "0.0"))
	float MaxDownforce = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Active Aero", meta = (ClampMin = "1.0", Units = "km/h"))
	float AeroRefSpeed = 200.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Active Aero", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float StraightLineShed = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Launch Control")
	bool bLaunchControlEnabled = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Launch Control", meta = (ClampMin = "0.0", Units = "km/h"))
	float LaunchControlMaxSpeed = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Launch Control", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float LaunchControlTargetSlip = 0.08f;

	UFUNCTION(BlueprintPure, Category = "Assists")
	const FVehicleAssistState& GetAssistState() const { return AssistState; }

	UFUNCTION(BlueprintCallable, Category = "Assists")
	void SetAssistsEnabled(bool bEnabled);

	UFUNCTION(BlueprintCallable, Category = "Assists")
	void ArmLaunchControl();

	UFUNCTION(BlueprintPure, Category = "Assists")
	FString GetActiveAssistsLabel() const;

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	struct FWheelInfo
	{
		int32 Index = INDEX_NONE;
		EAxleType Axle = EAxleType::Undefined;
		bool bIsLeft = false;
		bool bIsDriven = false;
	};

	void CacheWheelInfo();

	void SetupTorqueCombine();

	void UpdateWheelSlipControllers(float DeltaTime, float ForwardSpeed);

	void UpdateStabilityControl(float DeltaTime, float ForwardSpeed);

	void UpdateActiveAero(float SpeedKPH);

	void UpdateLaunchControl(float SpeedKPH);

	float ComputeSlipRatio(int32 WheelIndex, float ForwardSpeed) const;

private:

	UPROPERTY(Transient)
	TObjectPtr<UChaosWheeledVehicleMovementComponent> MovementComponent;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> ChassisMesh;

	UPROPERTY(Transient)
	FVehicleAssistState AssistState;

	TArray<FWheelInfo> WheelInfos;

	TArray<float> SlipIntegrators;

	TArray<float> PendingDriveTorque;
	TArray<float> PendingBrakeTorque;

	TArray<float> PrevDriveTorque;
	TArray<float> PrevBrakeTorque;

	float Wheelbase = 280.0f;

	bool bLaunchControlArmed = false;

	float BaseDownforce = 0.3f;

	bool bHasBaseDownforce = false;
};
