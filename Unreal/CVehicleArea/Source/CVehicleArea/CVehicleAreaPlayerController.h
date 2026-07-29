// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "CVehicleAreaPlayerController.generated.h"

class UInputMappingContext;
class ACVehicleAreaPawn;
class UCVehicleAreaUI;

UCLASS(abstract, Config="Game")
class ACVehicleAreaPlayerController : public APlayerController
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

	UPROPERTY(EditAnywhere, Category="Vehicle|Respawn")
	TSubclassOf<ACVehicleAreaPawn> VehiclePawnClass;

	TObjectPtr<ACVehicleAreaPawn> VehiclePawn;

	UPROPERTY(EditAnywhere, Category="Vehicle|UI")
	TSubclassOf<UCVehicleAreaUI> VehicleUIClass;

	UPROPERTY()
	TObjectPtr<UCVehicleAreaUI> VehicleUI;

protected:

	virtual void BeginPlay() override;

	virtual void SetupInputComponent() override;

public:

	virtual void Tick(float Delta) override;

protected:

	virtual void OnPossess(APawn* InPawn) override;

	UFUNCTION()
	void OnPawnDestroyed(AActor* DestroyedPawn);

	bool ShouldUseTouchControls() const;
};
