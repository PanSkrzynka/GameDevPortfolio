// Copyright Epic Games, Inc. All Rights Reserved.

#include "CVehicleAreaUI.h"

void UCVehicleAreaUI::UpdateSpeed(float NewSpeed)
{

	float FormattedSpeed = FMath::Abs(NewSpeed) * (bIsMPH ? 0.022f : 0.036f);

	OnSpeedUpdate(FormattedSpeed);
}

void UCVehicleAreaUI::UpdateGear(int32 NewGear)
{

	OnGearUpdate(NewGear);
}
