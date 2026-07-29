#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VehicleSurfaceTable.h"
#include "VehicleSurfaceResponseComponent.generated.h"

class UChaosWheeledVehicleMovementComponent;
class UPhysicalMaterial;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnWheelSurfaceChanged, int32, WheelIndex, FName, NewSurface, FName, PreviousSurface);

UCLASS(ClassGroup = (Vehicle), meta = (BlueprintSpawnableComponent))
class CVEHICLEAREA_API UVehicleSurfaceResponseComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UVehicleSurfaceResponseComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surfaces")
	TObjectPtr<UVehicleSurfaceTable> SurfaceTable;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surfaces")
	bool bAffectHandling = true;

	UPROPERTY(BlueprintAssignable, Category = "Surfaces")
	FOnWheelSurfaceChanged OnWheelSurfaceChanged;

	UFUNCTION(BlueprintCallable, Category = "Surfaces")
	const FVehicleSurfaceResponse& GetWheelResponse(int32 WheelIndex) const;

	UFUNCTION(BlueprintPure, Category = "Surfaces")
	FName GetDominantSurface() const { return DominantSurface; }

	UFUNCTION(BlueprintPure, Category = "Surfaces")
	float GetLooseSurfaceFraction() const { return LooseFraction; }

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	void ApplyToWheel(int32 WheelIndex, const FVehicleSurfaceResponse& Response);

private:

	UPROPERTY(Transient)
	TObjectPtr<UChaosWheeledVehicleMovementComponent> MovementComponent;

	TArray<float> BaseFriction;

	TArray<FName> WheelSurfaces;

	TArray<FVehicleSurfaceResponse> WheelResponses;

	FName DominantSurface = NAME_None;

	float LooseFraction = 0.0f;

	static const FVehicleSurfaceResponse FallbackResponse;
};
