// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "TimeTrialPlayerController.generated.h"

class ATimeTrialTrackGate;
class UTimeTrialUI;
class UInputMappingContext;
class UCVehicleAreaUI;
class ACVehicleAreaPawn;

UCLASS(abstract, Config="Game")
class ATimeTrialPlayerController : public APlayerController
{
	GENERATED_BODY()

protected:

	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	UPROPERTY(EditAnywhere, Category = "Input|Steering Wheel Controls")
	bool bUseSteeringWheelControls = false;

	UPROPERTY(EditAnywhere, Category = "Input|Steering Wheel Controls", meta = (EditCondition = "bUseSteeringWheelControls"))
	UInputMappingContext* SteeringWheelInputMappingContext;

	UPROPERTY(EditAnywhere, Category="Time Trial|UI")
	TSubclassOf<UTimeTrialUI> UIWidgetClass;

	UPROPERTY()
	TObjectPtr<UTimeTrialUI> UIWidget;

	UPROPERTY(EditAnywhere, Category="Vehicle|UI")
	TSubclassOf<UCVehicleAreaUI> VehicleUIClass;

	UPROPERTY()
	TObjectPtr<UCVehicleAreaUI> VehicleUI;

	TObjectPtr<ATimeTrialTrackGate> TargetGate;

	int32 CurrentLap = 0;

	bool bRaceStarted = false;

	UPROPERTY(EditAnywhere, Category="Vehicle|Respawn")
	TSubclassOf<ACVehicleAreaPawn> VehiclePawnClass;

	TObjectPtr<ACVehicleAreaPawn> VehiclePawn;

protected:

	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;

	virtual void OnPossess(APawn* aPawn) override;

public:

	virtual void Tick(float Delta) override;

public:

	UFUNCTION()
	void StartRace();

	void IncrementLapCount();

	ATimeTrialTrackGate* GetTargetGate();

	void SetTargetGate(ATimeTrialTrackGate* Gate);

protected:

	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedPawn);

	bool ShouldUseTouchControls() const;
};
