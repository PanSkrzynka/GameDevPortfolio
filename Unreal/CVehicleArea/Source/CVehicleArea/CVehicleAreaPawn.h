// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "CVehicleAreaPawn.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UInputAction;
class UChaosWheeledVehicleMovementComponent;
class UVehicleTelemetryComponent;
class UVehicleDriverAssistComponent;
class UVehicleSurfaceResponseComponent;
class UVehicleLapRecorderComponent;
class UVehicleShortcutsComponent;
class UVehicleSetupDataAsset;
struct FInputActionValue;

UCLASS(abstract)
class ACVehicleAreaPawn : public AWheeledVehiclePawn
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* FrontSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FrontCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	USpringArmComponent* BackSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* BackCamera;

	TObjectPtr<UChaosWheeledVehicleMovementComponent> ChaosVehicleMovement;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UVehicleTelemetryComponent> Telemetry;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UVehicleDriverAssistComponent> DriverAssists;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UVehicleSurfaceResponseComponent> SurfaceResponse;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UVehicleLapRecorderComponent> LapRecorder;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category ="Components", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UVehicleShortcutsComponent> Shortcuts;

protected:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vehicle Setup")
	TObjectPtr<UVehicleSetupDataAsset> VehicleSetup;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SteeringAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ThrottleAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* BrakeAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* HandbrakeAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAroundAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ToggleCameraAction;

	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ResetVehicleAction;

	bool bFrontCameraActive = false;

	bool bPreviousFlipCheck = false;

	UPROPERTY(EditAnywhere, Category="Flip Check", meta = (Units = "s"))
	float FlipCheckTime = 3.0f;

	UPROPERTY(EditAnywhere, Category="Flip Check")
	float FlipCheckMinDot = -0.2f;

	FTimerHandle FlipCheckTimer;

public:
	ACVehicleAreaPawn();

	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

	virtual void BeginPlay() override;

	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	virtual void Tick(float Delta) override;

protected:

	void Steering(const FInputActionValue& Value);

	void Throttle(const FInputActionValue& Value);

	void Brake(const FInputActionValue& Value);

	void StartBrake(const FInputActionValue& Value);
	void StopBrake(const FInputActionValue& Value);

	void StartHandbrake(const FInputActionValue& Value);
	void StopHandbrake(const FInputActionValue& Value);

	void LookAround(const FInputActionValue& Value);

	void ToggleCamera(const FInputActionValue& Value);

	void ResetVehicle(const FInputActionValue& Value);

public:

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoSteering(float SteeringValue);

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoThrottle(float ThrottleValue);

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoBrake(float BrakeValue);

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoBrakeStart();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoBrakeStop();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoHandbrakeStart();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoHandbrakeStop();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoLookAround(float YawDelta);

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoToggleCamera();

	UFUNCTION(BlueprintCallable, Category="Input")
	void DoResetVehicle();

protected:

	UFUNCTION(BlueprintImplementableEvent, Category="Vehicle")
	void BrakeLights(bool bBraking);

	UFUNCTION()
	void FlippedCheck();

public:

	FORCEINLINE USpringArmComponent* GetFrontSpringArm() const { return FrontSpringArm; }

	FORCEINLINE UCameraComponent* GetFollowCamera() const { return FrontCamera; }

	FORCEINLINE USpringArmComponent* GetBackSpringArm() const { return BackSpringArm; }

	FORCEINLINE UCameraComponent* GetBackCamera() const { return BackCamera; }

	FORCEINLINE const TObjectPtr<UChaosWheeledVehicleMovementComponent>& GetChaosVehicleMovement() const { return ChaosVehicleMovement; }

	FORCEINLINE UVehicleTelemetryComponent* GetTelemetry() const { return Telemetry; }

	FORCEINLINE UVehicleDriverAssistComponent* GetDriverAssists() const { return DriverAssists; }

	FORCEINLINE UVehicleSurfaceResponseComponent* GetSurfaceResponse() const { return SurfaceResponse; }

	FORCEINLINE UVehicleLapRecorderComponent* GetLapRecorder() const { return LapRecorder; }

	UFUNCTION(BlueprintCallable, Category="Vehicle Setup")
	void ApplyVehicleSetup(UVehicleSetupDataAsset* Setup, bool bRecreatePhysicsState = false);
};
