#include "VehicleSurfaceResponseComponent.h"
#include "CVehicleArea.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "ChaosVehicleWheel.h"
#include "WheeledVehiclePawn.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

const FVehicleSurfaceResponse UVehicleSurfaceResponseComponent::FallbackResponse = FVehicleSurfaceResponse();

UVehicleSurfaceResponseComponent::UVehicleSurfaceResponseComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UVehicleSurfaceResponseComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AWheeledVehiclePawn* VehiclePawn = Cast<AWheeledVehiclePawn>(GetOwner()))
	{
		MovementComponent = Cast<UChaosWheeledVehicleMovementComponent>(VehiclePawn->GetVehicleMovementComponent());
	}

	if (!MovementComponent)
	{
		SetComponentTickEnabled(false);
		return;
	}

	const int32 NumWheels = MovementComponent->WheelSetups.Num();

	BaseFriction.SetNum(NumWheels);
	WheelSurfaces.Init(NAME_None, NumWheels);
	WheelResponses.SetNum(NumWheels);

	for (int32 WheelIndex = 0; WheelIndex < NumWheels; ++WheelIndex)
	{
		float WheelFriction = 2.0f;

		const FChaosWheelSetup& Setup = MovementComponent->WheelSetups[WheelIndex];
		if (Setup.WheelClass)
		{
			if (const UChaosVehicleWheel* Defaults = Setup.WheelClass.GetDefaultObject())
			{
				WheelFriction = Defaults->FrictionForceMultiplier;
			}
		}

		BaseFriction[WheelIndex] = WheelFriction;
	}

	if (!SurfaceTable)
	{
		UE_LOG(LogCVehicleArea, Log, TEXT("SurfaceResponse: no surface table on '%s'."),
			*GetNameSafe(GetOwner()));
	}
}

const FVehicleSurfaceResponse& UVehicleSurfaceResponseComponent::GetWheelResponse(int32 WheelIndex) const
{
	return WheelResponses.IsValidIndex(WheelIndex) ? WheelResponses[WheelIndex] : FallbackResponse;
}

void UVehicleSurfaceResponseComponent::ApplyToWheel(int32 WheelIndex, const FVehicleSurfaceResponse& Response)
{
	if (!bAffectHandling || !MovementComponent->HasValidPhysicsState())
	{
		return;
	}

	if (!BaseFriction.IsValidIndex(WheelIndex))
	{
		return;
	}

	MovementComponent->SetWheelFrictionMultiplier(WheelIndex, BaseFriction[WheelIndex] * Response.FrictionScale);
	MovementComponent->SetWheelSlipGraphMultiplier(WheelIndex, Response.SlipCurveScale);
}

void UVehicleSurfaceResponseComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!SurfaceTable || !MovementComponent || !MovementComponent->HasValidPhysicsState())
	{
		return;
	}

	const int32 NumWheels = MovementComponent->GetNumWheels();

	TMap<FName, int32> SurfaceCounts;
	int32 GroundedWheels = 0;
	int32 LooseWheels = 0;

	for (int32 WheelIndex = 0; WheelIndex < NumWheels; ++WheelIndex)
	{
		if (!WheelSurfaces.IsValidIndex(WheelIndex))
		{
			continue;
		}

		const FWheelStatus& Status = MovementComponent->GetWheelState(WheelIndex);

		if (!Status.bInContact)
		{
			continue;
		}

		++GroundedWheels;

		const UPhysicalMaterial* Material = Status.PhysMaterial.Get();
		const FVehicleSurfaceResponse& Response = SurfaceTable->GetResponse(Material);

		SurfaceCounts.FindOrAdd(Response.SurfaceName)++;
		if (Response.bIsLoose)
		{
			++LooseWheels;
		}

		if (WheelSurfaces[WheelIndex] != Response.SurfaceName)
		{
			const FName PreviousSurface = WheelSurfaces[WheelIndex];

			WheelSurfaces[WheelIndex] = Response.SurfaceName;
			WheelResponses[WheelIndex] = Response;

			ApplyToWheel(WheelIndex, Response);

			OnWheelSurfaceChanged.Broadcast(WheelIndex, Response.SurfaceName, PreviousSurface);
		}
	}

	FName BestSurface = NAME_None;
	int32 BestCount = 0;
	for (const TPair<FName, int32>& Pair : SurfaceCounts)
	{
		if (Pair.Value > BestCount)
		{
			BestCount = Pair.Value;
			BestSurface = Pair.Key;
		}
	}

	DominantSurface = BestSurface;
	LooseFraction = GroundedWheels > 0 ? static_cast<float>(LooseWheels) / GroundedWheels : 0.0f;
}
