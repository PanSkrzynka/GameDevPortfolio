#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "VehicleSurfaceTable.generated.h"

class UPhysicalMaterial;

USTRUCT(BlueprintType)
struct FVehicleSurfaceResponse
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surface")
	FName SurfaceName = TEXT("Default");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Handling", meta = (ClampMin = "0.0", ClampMax = "3.0"))
	float FrictionScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Handling", meta = (ClampMin = "0.05", ClampMax = "3.0"))
	float SlipCurveScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presentation")
	bool bIsLoose = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presentation")
	FLinearColor EffectColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presentation", meta = (ClampMin = "0.0"))
	float EffectIntensity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Presentation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float AudioBlend = 0.0f;
};

UCLASS(BlueprintType)
class CVEHICLEAREA_API UVehicleSurfaceTable : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surfaces")
	FVehicleSurfaceResponse DefaultResponse;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surfaces")
	TMap<TObjectPtr<UPhysicalMaterial>, FVehicleSurfaceResponse> Responses;

	UFUNCTION(BlueprintCallable, Category = "Surfaces")
	const FVehicleSurfaceResponse& GetResponse(const UPhysicalMaterial* Material) const;
};
