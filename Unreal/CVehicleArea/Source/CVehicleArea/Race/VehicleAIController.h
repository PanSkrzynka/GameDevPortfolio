#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Controller.h"
#include "RacingLine.h"
#include "VehicleAIController.generated.h"

class UChaosWheeledVehicleMovementComponent;

UCLASS()
class CVEHICLEAREA_API AVehicleAIController : public AController
{
	GENERATED_BODY()

public:

	AVehicleAIController();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Steering", meta = (ClampMin = "50.0", Units = "cm"))
	float BaseLookahead = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Steering", meta = (ClampMin = "0.0"))
	float LookaheadPerSpeed = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Steering", meta = (ClampMin = "100.0", Units = "cm"))
	float MaxLookahead = 3000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Steering", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float SteeringGain = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Steering", meta = (ClampMin = "1.0", ClampMax = "50.0"))
	float SteeringRate = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Speed", meta = (ClampMin = "0.0"))
	float SpeedGain = 0.006f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Speed", meta = (ClampMin = "0.0"))
	float SpeedIntegralGain = 0.0015f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Speed", meta = (ClampMin = "0.0"))
	float SpeedDerivativeGain = 0.0008f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Speed", meta = (ClampMin = "0.0"))
	float SpeedIntegralMax = 4000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Speed", meta = (ClampMin = "0.0", ClampMax = "5.0", Units = "s"))
	float SpeedLookaheadTime = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Skill", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SkillLevel = 0.9f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Skill", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float MinSpeedScale = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Recovery", meta = (ClampMin = "0.0", Units = "km/h"))
	float StuckSpeed = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Recovery", meta = (ClampMin = "0.1", Units = "s"))
	float StuckTimeout = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Recovery", meta = (ClampMin = "0.1", Units = "s"))
	float RecoveryDuration = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Recovery", meta = (ClampMin = "100.0", Units = "cm"))
	float MaxLineDistance = 3000.0f;

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetRacingLine(const FRacingLine& InRacingLine);

	UFUNCTION(BlueprintPure, Category = "AI")
	bool HasRacingLine() const { return RacingLine.IsValid(); }

	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetDrivingEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "AI")
	bool IsDriving() const { return bDrivingEnabled; }

	UFUNCTION(BlueprintPure, Category = "AI")
	float GetDistanceAlongLine() const { return DistanceAlongLine; }

	UFUNCTION(BlueprintPure, Category = "AI")
	float GetTargetSpeed() const { return DesiredSpeed; }

	UFUNCTION(BlueprintPure, Category = "AI")
	FVector GetPursuitTarget() const { return AimPoint; }

	const FRacingLine& GetRacingLine() const { return RacingLine; }

	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaTime) override;

protected:

	float UpdateSteering(const FTransform& VehicleTransform, float ForwardSpeed, int32 NearestIndex, float DeltaTime);

	void UpdateSpeedControl(float ForwardSpeed, int32 NearestIndex, float DeltaTime, float& OutThrottle, float& OutBrake);

	bool UpdateRecovery(float SpeedKPH, float DeltaTime);

	void ApplyInputs(float Steering, float Throttle, float Brake, bool bHandbrake);

private:

	UPROPERTY(Transient)
	TObjectPtr<UChaosWheeledVehicleMovementComponent> MovementComponent;

	FRacingLine RacingLine;

	bool bDrivingEnabled = true;

	int32 LastPointIndex = INDEX_NONE;

	float DistanceAlongLine = 0.0f;

	float DesiredSpeed = 0.0f;

	FVector AimPoint = FVector::ZeroVector;

	float PrevSteering = 0.0f;

	float SpeedIntegral = 0.0f;

	float PrevSpeedError = 0.0f;

	float StuckTimer = 0.0f;

	float RecoveryTimer = 0.0f;

	float RecoverySteering = 0.0f;

	float Wheelbase = 280.0f;

	float MaxSteerAngle = 40.0f;
};
