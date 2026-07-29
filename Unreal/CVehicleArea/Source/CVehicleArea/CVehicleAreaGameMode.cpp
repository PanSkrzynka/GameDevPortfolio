// Copyright Epic Games, Inc. All Rights Reserved.

#include "CVehicleAreaGameMode.h"
#include "CVehicleAreaPlayerController.h"
#include "HUD/VehicleTelemetryHUD.h"

ACVehicleAreaGameMode::ACVehicleAreaGameMode()
{
	PlayerControllerClass = ACVehicleAreaPlayerController::StaticClass();

	HUDClass = AVehicleTelemetryHUD::StaticClass();
}
