// Copyright Epic Games, Inc. All Rights Reserved.

#include "TimeTrialPlayerController.h"
#include "TimeTrialUI.h"
#include "Engine/World.h"
#include "TimeTrialGameMode.h"
#include "TimeTrialTrackGate.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "CVehicleAreaUI.h"
#include "CVehicleAreaPawn.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "Blueprint/UserWidget.h"
#include "CVehicleArea.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "Widgets/Input/SVirtualJoystick.h"

void ATimeTrialPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void ATimeTrialPlayerController::SetupInputComponent()
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

		UIWidget = CreateWidget<UTimeTrialUI>(this, UIWidgetClass);

		if (UIWidget)
		{
			UIWidget->AddToPlayerScreen(0);

			UIWidget->OnRaceStart.AddDynamic(this, &ATimeTrialPlayerController::StartRace);

		} else {

			UE_LOG(LogCVehicleArea, Error, TEXT("Could not spawn Time Trial UI widget."));

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

void ATimeTrialPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	VehiclePawn = CastChecked<ACVehicleAreaPawn>(InPawn);

	VehiclePawn->OnDestroyed.AddDynamic(this, &ATimeTrialPlayerController::OnPawnDestroyed);

	if (!bRaceStarted)
	{
		VehiclePawn->DisableInput(this);
	}
}

void ATimeTrialPlayerController::Tick(float Delta)
{
	Super::Tick(Delta);

	if (IsValid(VehiclePawn) && IsValid(VehicleUI))
	{
		VehicleUI->UpdateSpeed(VehiclePawn->GetChaosVehicleMovement()->GetForwardSpeed());
		VehicleUI->UpdateGear(VehiclePawn->GetChaosVehicleMovement()->GetCurrentGear());
	}
}

void ATimeTrialPlayerController::StartRace()
{

	if (ATimeTrialGameMode* GM = Cast<ATimeTrialGameMode>(GetWorld()->GetAuthGameMode()))
	{
		SetTargetGate(GM->GetFinishLine()->GetNextMarker());
	}

	bRaceStarted = true;

	CurrentLap = 0;
	IncrementLapCount();

	if (GetPawn())
	{
		GetPawn()->EnableInput(this);
	}
}

void ATimeTrialPlayerController::IncrementLapCount()
{

	++CurrentLap;

	UIWidget->UpdateLapCount(CurrentLap, GetWorld()->GetTimeSeconds());
}

ATimeTrialTrackGate* ATimeTrialPlayerController::GetTargetGate()
{
	return TargetGate.Get();
}

void ATimeTrialPlayerController::SetTargetGate(ATimeTrialTrackGate* Gate)
{
	TargetGate = Gate;
}

void ATimeTrialPlayerController::OnPawnDestroyed(AActor* DestroyedPawn)
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

bool ATimeTrialPlayerController::ShouldUseTouchControls() const
{

	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}
