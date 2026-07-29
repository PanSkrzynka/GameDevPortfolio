// Copyright Epic Games, Inc. All Rights Reserved.

#include "CVehicleAreaWheelRear.h"
#include "UObject/ConstructorHelpers.h"

UCVehicleAreaWheelRear::UCVehicleAreaWheelRear()
{
	AxleType = EAxleType::Rear;
	bAffectedByHandbrake = true;
	bAffectedByEngine = true;
}
