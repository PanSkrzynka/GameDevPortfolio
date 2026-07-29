// Copyright Epic Games, Inc. All Rights Reserved.

#include "CVehicleAreaPlayerController.h"
#include "CVehicleAreaPawn.h"
#include "CVehicleAreaUI.h"
#include "EnhancedInputSubsystems.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Blueprint/UserWidget.h"
#include "CVehicleArea.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Widgets/Input/SVirtualJoystick.h"

void ACVehicleAreaPlayerController::BeginPlay()
{
	Super::BeginPlay();

	bAttachToPawn = true;
}

void ACVehicleAreaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (IsLocalPlayerController())
	{

		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}

			if (bUseSteeringWheelControls)
			{
				Subsystem->AddMappingContext(SteeringWheelInputMappingContext, 0);
			}
		}
	}

	if (IsLocalPlayerController())
	{
		if (ShouldUseTouchControls())
		{

			MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

			if (MobileControlsWidget)
			{

				MobileControlsWidget->AddToPlayerScreen(0);

			} else {

				UE_LOG(LogCVehicleArea, Error, TEXT("Could not spawn mobile controls widget."));

			}
		}

		VehicleUI = CreateWidget<UCVehicleAreaUI>(this, VehicleUIClass);

		if (VehicleUI)
		{
			VehicleUI->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogCVehicleArea, Error, TEXT("Could not spawn vehicle UI widget."));

		}
	}
}

void ACVehicleAreaPlayerController::Tick(float Delta)
{
	Super::Tick(Delta);

	if (IsValid(VehiclePawn) && IsValid(VehicleUI))
	{
		VehicleUI->UpdateSpeed(VehiclePawn->GetChaosVehicleMovement()->GetForwardSpeed());
		VehicleUI->UpdateGear(VehiclePawn->GetChaosVehicleMovement()->GetCurrentGear());
	}
}

void ACVehicleAreaPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	VehiclePawn = CastChecked<ACVehicleAreaPawn>(InPawn);

	VehiclePawn->OnDestroyed.AddDynamic(this, &ACVehicleAreaPlayerController::OnPawnDestroyed);
}

void ACVehicleAreaPlayerController::OnPawnDestroyed(AActor* DestroyedPawn)
{

	TArray<AActor*> ActorList;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), ActorList);

	if (ActorList.Num() > 0)
	{

		const FTransform SpawnTransform = ActorList[0]->GetActorTransform();

		if (ACVehicleAreaPawn* RespawnedVehicle = GetWorld()->SpawnActor<ACVehicleAreaPawn>(VehiclePawnClass, SpawnTransform))
		{

			Possess(RespawnedVehicle);
		}
	}
}

bool ACVehicleAreaPlayerController::ShouldUseTouchControls() const
{

	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
