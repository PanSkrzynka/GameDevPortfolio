// Copyright Epic Games, Inc. All Rights Reserved.

#include "CVehicleAreaPawn.h"
#include "CVehicleAreaWheelFront.h"
#include "CVehicleAreaWheelRear.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "CVehicleArea.h"
#include "TimerManager.h"
#include "Telemetry/VehicleTelemetryComponent.h"
#include "Dynamics/VehicleDriverAssistComponent.h"
#include "Dynamics/VehicleSurfaceResponseComponent.h"
#include "Replay/VehicleLapRecorderComponent.h"
#include "Debug/VehicleShortcutsComponent.h"
#include "Tuning/VehicleSetupDataAsset.h"

#define LOCTEXT_NAMESPACE "VehiclePawn"

ACVehicleAreaPawn::ACVehicleAreaPawn()
{

	FrontSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Front Spring Arm"));
	FrontSpringArm->SetupAttachment(GetMesh());
	FrontSpringArm->TargetArmLength = 0.0f;
	FrontSpringArm->bDoCollisionTest = false;
	FrontSpringArm->bEnableCameraRotationLag = true;
	FrontSpringArm->CameraRotationLagSpeed = 15.0f;
	FrontSpringArm->SetRelativeLocation(FVector(30.0f, 0.0f, 120.0f));

	FrontCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Front Camera"));
	FrontCamera->SetupAttachment(FrontSpringArm);
	FrontCamera->bAutoActivate = false;

	BackSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Back Spring Arm"));
	BackSpringArm->SetupAttachment(GetMesh());
	BackSpringArm->TargetArmLength = 650.0f;
	BackSpringArm->SocketOffset.Z = 150.0f;
	BackSpringArm->bDoCollisionTest = false;
	BackSpringArm->bInheritPitch = false;
	BackSpringArm->bInheritRoll = false;
	BackSpringArm->bEnableCameraRotationLag = true;
	BackSpringArm->CameraRotationLagSpeed = 2.0f;
	BackSpringArm->CameraLagMaxDistance = 50.0f;

	BackCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("Back Camera"));
	BackCamera->SetupAttachment(BackSpringArm);

	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetCollisionProfileName(FName("Vehicle"));

	ChaosVehicleMovement = CastChecked<UChaosWheeledVehicleMovementComponent>(GetVehicleMovement());

	Telemetry = CreateDefaultSubobject<UVehicleTelemetryComponent>(TEXT("Telemetry"));
	DriverAssists = CreateDefaultSubobject<UVehicleDriverAssistComponent>(TEXT("Driver Assists"));
	SurfaceResponse = CreateDefaultSubobject<UVehicleSurfaceResponseComponent>(TEXT("Surface Response"));
	LapRecorder = CreateDefaultSubobject<UVehicleLapRecorderComponent>(TEXT("Lap Recorder"));
	Shortcuts = CreateDefaultSubobject<UVehicleShortcutsComponent>(TEXT("Shortcuts"));
}

void ACVehicleAreaPawn::ApplyVehicleSetup(UVehicleSetupDataAsset* Setup, bool bRecreatePhysicsState)
{
	if (!Setup)
	{
		return;
	}

	VehicleSetup = Setup;
	Setup->ApplyTo(GetChaosVehicleMovement(), bRecreatePhysicsState);
}

void ACVehicleAreaPawn::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (Shortcuts)
	{
		Shortcuts->BindShortcuts(PlayerInputComponent);
	}

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{

		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &ACVehicleAreaPawn::Steering);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &ACVehicleAreaPawn::Steering);

		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &ACVehicleAreaPawn::Throttle);
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Completed, this, &ACVehicleAreaPawn::Throttle);

		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &ACVehicleAreaPawn::Brake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Started, this, &ACVehicleAreaPawn::StartBrake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &ACVehicleAreaPawn::StopBrake);

		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &ACVehicleAreaPawn::StartHandbrake);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &ACVehicleAreaPawn::StopHandbrake);

		EnhancedInputComponent->BindAction(LookAroundAction, ETriggerEvent::Triggered, this, &ACVehicleAreaPawn::LookAround);

		EnhancedInputComponent->BindAction(ToggleCameraAction, ETriggerEvent::Triggered, this, &ACVehicleAreaPawn::ToggleCamera);

		EnhancedInputComponent->BindAction(ResetVehicleAction, ETriggerEvent::Triggered, this, &ACVehicleAreaPawn::ResetVehicle);
	}
	else
	{
		UE_LOG(LogCVehicleArea, Error, TEXT("'%s' Failed to find an Enhanced Input component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ACVehicleAreaPawn::BeginPlay()
{
	Super::BeginPlay();

	GetWorld()->GetTimerManager().SetTimer(FlipCheckTimer, this, &ACVehicleAreaPawn::FlippedCheck, FlipCheckTime, true);

	if (VehicleSetup)
	{
		ApplyVehicleSetup(VehicleSetup, false);
	}
}

void ACVehicleAreaPawn::EndPlay(EEndPlayReason::Type EndPlayReason)
{

	GetWorld()->GetTimerManager().ClearTimer(FlipCheckTimer);

	Super::EndPlay(EndPlayReason);
}

void ACVehicleAreaPawn::Tick(float Delta)
{
	Super::Tick(Delta);

	bool bMovingOnGround = ChaosVehicleMovement->IsMovingOnGround();
	GetMesh()->SetAngularDamping(bMovingOnGround ? 0.0f : 3.0f);

	float CameraYaw = BackSpringArm->GetRelativeRotation().Yaw;
	CameraYaw = FMath::FInterpTo(CameraYaw, 0.0f, Delta, 1.0f);

	BackSpringArm->SetRelativeRotation(FRotator(0.0f, CameraYaw, 0.0f));
}

void ACVehicleAreaPawn::Steering(const FInputActionValue& Value)
{

	DoSteering(Value.Get<float>());
}

void ACVehicleAreaPawn::Throttle(const FInputActionValue& Value)
{

	DoThrottle(Value.Get<float>());
}

void ACVehicleAreaPawn::Brake(const FInputActionValue& Value)
{

	DoBrake(Value.Get<float>());
}

void ACVehicleAreaPawn::StartBrake(const FInputActionValue& Value)
{

	DoBrakeStart();
}

void ACVehicleAreaPawn::StopBrake(const FInputActionValue& Value)
{

	DoBrakeStop();
}

void ACVehicleAreaPawn::StartHandbrake(const FInputActionValue& Value)
{

	DoHandbrakeStart();
}

void ACVehicleAreaPawn::StopHandbrake(const FInputActionValue& Value)
{

	DoHandbrakeStop();
}

void ACVehicleAreaPawn::LookAround(const FInputActionValue& Value)
{

	DoLookAround(Value.Get<float>());
}

void ACVehicleAreaPawn::ToggleCamera(const FInputActionValue& Value)
{

	DoToggleCamera();
}

void ACVehicleAreaPawn::ResetVehicle(const FInputActionValue& Value)
{

	DoResetVehicle();
}

void ACVehicleAreaPawn::DoSteering(float SteeringValue)
{

	ChaosVehicleMovement->SetSteeringInput(SteeringValue);
}

void ACVehicleAreaPawn::DoThrottle(float ThrottleValue)
{

	ChaosVehicleMovement->SetThrottleInput(ThrottleValue);

	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void ACVehicleAreaPawn::DoBrake(float BrakeValue)
{

	ChaosVehicleMovement->SetBrakeInput(BrakeValue);

	ChaosVehicleMovement->SetThrottleInput(0.0f);
}

void ACVehicleAreaPawn::DoBrakeStart()
{

	BrakeLights(true);
}

void ACVehicleAreaPawn::DoBrakeStop()
{

	BrakeLights(false);

	ChaosVehicleMovement->SetBrakeInput(0.0f);
}

void ACVehicleAreaPawn::DoHandbrakeStart()
{

	ChaosVehicleMovement->SetHandbrakeInput(true);

	BrakeLights(true);
}

void ACVehicleAreaPawn::DoHandbrakeStop()
{

	ChaosVehicleMovement->SetHandbrakeInput(false);

	BrakeLights(false);
}

void ACVehicleAreaPawn::DoLookAround(float YawDelta)
{

	BackSpringArm->AddLocalRotation(FRotator(0.0f, YawDelta, 0.0f));
}

void ACVehicleAreaPawn::DoToggleCamera()
{

	bFrontCameraActive = !bFrontCameraActive;

	FrontCamera->SetActive(bFrontCameraActive);
	BackCamera->SetActive(!bFrontCameraActive);
}

void ACVehicleAreaPawn::DoResetVehicle()
{

	FVector ResetLocation = GetActorLocation() + FVector(0.0f, 0.0f, 50.0f);

	FRotator ResetRotation = GetActorRotation();
	ResetRotation.Pitch = 0.0f;
	ResetRotation.Roll = 0.0f;

	SetActorTransform(FTransform(ResetRotation, ResetLocation, FVector::OneVector), false, nullptr, ETeleportType::TeleportPhysics);

	GetMesh()->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
	GetMesh()->SetPhysicsLinearVelocity(FVector::ZeroVector);
}

void ACVehicleAreaPawn::FlippedCheck()
{

	const float UpDot = FVector::DotProduct(FVector::UpVector, GetMesh()->GetUpVector());

	if (UpDot < FlipCheckMinDot)
	{

		if (bPreviousFlipCheck)
		{

			DoResetVehicle();
		}

		bPreviousFlipCheck = true;

	} else {

		bPreviousFlipCheck = false;
	}
}

#undef LOCTEXT_NAMESPACE
