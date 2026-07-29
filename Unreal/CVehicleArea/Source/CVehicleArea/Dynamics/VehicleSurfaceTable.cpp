#include "VehicleSurfaceTable.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

const FVehicleSurfaceResponse& UVehicleSurfaceTable::GetResponse(const UPhysicalMaterial* Material) const
{
	if (Material)
	{

		if (const FVehicleSurfaceResponse* Found = Responses.Find(const_cast<UPhysicalMaterial*>(Material)))
		{
			return *Found;
		}
	}

	return DefaultResponse;
}
