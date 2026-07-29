#pragma once

#include "CoreMinimal.h"
#include "Replay/VehicleReplayTypes.h"
#include "RacingLine.generated.h"

USTRUCT(BlueprintType)
struct FRacingLinePoint
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Racing Line")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Racing Line")
	FVector Tangent = FVector::ForwardVector;

	UPROPERTY(BlueprintReadOnly, Category = "Racing Line", meta = (Units = "cm"))
	float Distance = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Racing Line")
	float Curvature = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Racing Line")
	float TargetSpeed = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Racing Line")
	float RecordedSpeed = 0.0f;
};

USTRUCT(BlueprintType)
struct FRacingLineBuildSettings
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Racing Line", meta = (ClampMin = "50.0", Units = "cm"))
	float PointSpacing = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Racing Line", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float LateralGrip = 1.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Racing Line", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float AccelerationLimit = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Racing Line", meta = (ClampMin = "0.1", ClampMax = "3.0"))
	float BrakingLimit = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Racing Line", meta = (ClampMin = "1.0", Units = "km/h"))
	float MaxSpeedKPH = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Racing Line", meta = (ClampMin = "1.0", Units = "km/h"))
	float MinSpeedKPH = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Racing Line", meta = (ClampMin = "0", ClampMax = "20"))
	int32 CurvatureSmoothing = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Racing Line")
	bool bClosedLoop = true;
};

USTRUCT(BlueprintType)
struct CVEHICLEAREA_API FRacingLine
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Racing Line")
	TArray<FRacingLinePoint> Points;

	UPROPERTY(BlueprintReadOnly, Category = "Racing Line", meta = (Units = "cm"))
	float TotalLength = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Racing Line")
	bool bClosedLoop = true;

	bool IsValid() const { return Points.Num() >= 3; }

	int32 FindNearestPoint(const FVector& WorldLocation, int32 HintIndex = INDEX_NONE, int32 SearchWindow = 40) const;

	int32 AdvanceIndex(int32 StartIndex, float DistanceAhead) const;

	FRacingLinePoint SampleAtDistance(float Distance) const;

	static FRacingLine BuildFromRecording(const FVehicleLapRecording& Recording, const FRacingLineBuildSettings& Settings);

private:

	static bool Resample(const FVehicleLapRecording& Recording, const FRacingLineBuildSettings& Settings, TArray<FRacingLinePoint>& OutPoints, float& OutTotalLength);

	static void ComputeGeometry(TArray<FRacingLinePoint>& Points, const FRacingLineBuildSettings& Settings);

	static void BuildSpeedProfile(TArray<FRacingLinePoint>& Points, const FRacingLineBuildSettings& Settings);
};
