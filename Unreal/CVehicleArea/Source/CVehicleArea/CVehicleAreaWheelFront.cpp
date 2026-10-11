#include "CVehicleAreaWheelFront.h"
#include "UObject/ConstructorHelpers.h"

UCVehicleAreaWheelFront::UCVehicleAreaWheelFront()
{
	AxleType = EAxleType::Front;
	bAffectedBySteering = true;
	MaxSteerAngle = 40.f;
}
