#include "RacingLine.h"
#include "CVehicleArea.h"

namespace
{
	constexpr float GravityCmS2 = 980.665f;

	constexpr float KphToCms = 1.0f / 0.036f;
}

int32 FRacingLine::FindNearestPoint(const FVector& WorldLocation, int32 HintIndex, int32 SearchWindow) const
{
	if (Points.Num() == 0)
	{
		return INDEX_NONE;
	}

	int32 Best = INDEX_NONE;
	float BestDistanceSquared = TNumericLimits<float>::Max();

	const bool bUseWindow = Points.IsValidIndex(HintIndex) && SearchWindow > 0;
	const int32 First = bUseWindow ? HintIndex - SearchWindow : 0;
	const int32 Last = bUseWindow ? HintIndex + SearchWindow : Points.Num() - 1;

	for (int32 Cursor = First; Cursor <= Last; ++Cursor)
	{
		int32 Index = Cursor;

		if (bClosedLoop)
		{
			Index = ((Cursor % Points.Num()) + Points.Num()) % Points.Num();
		}
		else if (!Points.IsValidIndex(Index))
		{
			continue;
		}

		const float DistanceSquared = FVector::DistSquared(Points[Index].Location, WorldLocation);
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			Best = Index;
		}
	}

	return Best;
}

int32 FRacingLine::AdvanceIndex(int32 StartIndex, float DistanceAhead) const
{
	if (!Points.IsValidIndex(StartIndex) || TotalLength <= KINDA_SMALL_NUMBER)
	{
		return StartIndex;
	}

	const float Spacing = TotalLength / Points.Num();
	const int32 Steps = FMath::RoundToInt(DistanceAhead / FMath::Max(Spacing, KINDA_SMALL_NUMBER));
	const int32 Target = StartIndex + Steps;

	if (bClosedLoop)
	{
		return ((Target % Points.Num()) + Points.Num()) % Points.Num();
	}

	return FMath::Clamp(Target, 0, Points.Num() - 1);
}

FRacingLinePoint FRacingLine::SampleAtDistance(float Distance) const
{
	if (Points.Num() == 0)
	{
		return FRacingLinePoint();
	}

	if (Points.Num() == 1 || TotalLength <= KINDA_SMALL_NUMBER)
	{
		return Points[0];
	}

	if (bClosedLoop)
	{
		Distance = FMath::Fmod(Distance, TotalLength);
		if (Distance < 0.0f)
		{
			Distance += TotalLength;
		}
	}
	else
	{
		Distance = FMath::Clamp(Distance, 0.0f, Points.Last().Distance);
	}

	const float Spacing = TotalLength / Points.Num();
	const int32 LowIndex = FMath::Clamp(FMath::FloorToInt(Distance / Spacing), 0, Points.Num() - 1);
	const int32 HighIndex = bClosedLoop ? (LowIndex + 1) % Points.Num() : FMath::Min(LowIndex + 1, Points.Num() - 1);

	const float Alpha = FMath::Clamp((Distance - LowIndex * Spacing) / FMath::Max(Spacing, KINDA_SMALL_NUMBER), 0.0f, 1.0f);

	FRacingLinePoint Result;
	Result.Location = FMath::Lerp(Points[LowIndex].Location, Points[HighIndex].Location, Alpha);
	Result.Tangent = FMath::Lerp(Points[LowIndex].Tangent, Points[HighIndex].Tangent, Alpha).GetSafeNormal();
	Result.Curvature = FMath::Lerp(Points[LowIndex].Curvature, Points[HighIndex].Curvature, Alpha);
	Result.TargetSpeed = FMath::Lerp(Points[LowIndex].TargetSpeed, Points[HighIndex].TargetSpeed, Alpha);
	Result.RecordedSpeed = FMath::Lerp(Points[LowIndex].RecordedSpeed, Points[HighIndex].RecordedSpeed, Alpha);
	Result.Distance = Distance;

	return Result;
}

bool FRacingLine::Resample(const FVehicleLapRecording& Recording, const FRacingLineBuildSettings& Settings, TArray<FRacingLinePoint>& OutPoints, float& OutTotalLength)
{
	OutPoints.Reset();
	OutTotalLength = 0.0f;

	TArray<FVector> Positions;
	TArray<float> Speeds;
	Positions.Reserve(Recording.Samples.Num());
	Speeds.Reserve(Recording.Samples.Num());

	constexpr float MinimumStep = 10.0f;

	for (const FVehicleReplaySample& Sample : Recording.Samples)
	{
		const FVector Position = Sample.Snapshot.Transform.GetLocation();

		if (Positions.Num() > 0 && FVector::DistSquared(Positions.Last(), Position) < MinimumStep * MinimumStep)
		{
			continue;
		}

		Positions.Add(Position);
		Speeds.Add(Sample.Snapshot.LinearVelocity.Size());
	}

	if (Settings.bClosedLoop && Positions.Num() >= 3)
	{

		const FVector FirstPosition = Positions[0];
		const float FirstSpeed = Speeds[0];

		Positions.Add(FirstPosition);
		Speeds.Add(FirstSpeed);
	}

	if (Positions.Num() < 4)
	{
		return false;
	}

	TArray<float> Cumulative;
	Cumulative.SetNum(Positions.Num());
	Cumulative[0] = 0.0f;

	for (int32 Index = 1; Index < Positions.Num(); ++Index)
	{
		Cumulative[Index] = Cumulative[Index - 1] + FVector::Dist(Positions[Index - 1], Positions[Index]);
	}

	const float PathLength = Cumulative.Last();
	if (PathLength < Settings.PointSpacing * 4.0f)
	{
		return false;
	}

	const int32 PointCount = FMath::Max(4, FMath::RoundToInt(PathLength / Settings.PointSpacing));
	const float Spacing = PathLength / PointCount;

	OutPoints.Reserve(PointCount);

	int32 Cursor = 0;
	for (int32 Station = 0; Station < PointCount; ++Station)
	{
		const float TargetDistance = Station * Spacing;

		while (Cursor + 1 < Cumulative.Num() - 1 && Cumulative[Cursor + 1] < TargetDistance)
		{
			++Cursor;
		}

		const float SegmentLength = Cumulative[Cursor + 1] - Cumulative[Cursor];
		const float Alpha = SegmentLength > KINDA_SMALL_NUMBER
			? FMath::Clamp((TargetDistance - Cumulative[Cursor]) / SegmentLength, 0.0f, 1.0f)
			: 0.0f;

		FRacingLinePoint Point;
		Point.Location = FMath::Lerp(Positions[Cursor], Positions[Cursor + 1], Alpha);
		Point.RecordedSpeed = FMath::Lerp(Speeds[Cursor], Speeds[Cursor + 1], Alpha);
		Point.Distance = TargetDistance;

		OutPoints.Add(Point);
	}

	OutTotalLength = PathLength;
	return true;
}

void FRacingLine::ComputeGeometry(TArray<FRacingLinePoint>& Points, const FRacingLineBuildSettings& Settings)
{
	const int32 Count = Points.Num();
	if (Count < 3)
	{
		return;
	}

	auto WrapIndex = [Count, &Settings](int32 Index) -> int32
	{
		if (Settings.bClosedLoop)
		{
			return ((Index % Count) + Count) % Count;
		}
		return FMath::Clamp(Index, 0, Count - 1);
	};

	TArray<float> RawCurvature;
	RawCurvature.SetNumZeroed(Count);

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FVector& Previous = Points[WrapIndex(Index - 1)].Location;
		const FVector& Current = Points[Index].Location;
		const FVector& Next = Points[WrapIndex(Index + 1)].Location;

		Points[Index].Tangent = (Next - Previous).GetSafeNormal();

		const FVector EdgeA = Current - Previous;
		const FVector EdgeB = Next - Current;
		const FVector EdgeC = Next - Previous;

		const float LengthA = EdgeA.Size();
		const float LengthB = EdgeB.Size();
		const float LengthC = EdgeC.Size();

		const float Denominator = LengthA * LengthB * LengthC;
		if (Denominator > KINDA_SMALL_NUMBER)
		{

			const float TwiceArea = FMath::Abs(EdgeA.X * EdgeB.Y - EdgeA.Y * EdgeB.X);
			RawCurvature[Index] = (2.0f * TwiceArea) / Denominator;
		}
	}

	const int32 Radius = FMath::Clamp(Settings.CurvatureSmoothing, 0, Count / 2);

	if (Radius <= 0)
	{
		for (int32 Index = 0; Index < Count; ++Index)
		{
			Points[Index].Curvature = RawCurvature[Index];
		}
		return;
	}

	for (int32 Index = 0; Index < Count; ++Index)
	{
		float Sum = 0.0f;
		int32 Samples = 0;

		for (int32 Offset = -Radius; Offset <= Radius; ++Offset)
		{
			const int32 Neighbor = Index + Offset;

			if (!Settings.bClosedLoop && (Neighbor < 0 || Neighbor >= Count))
			{
				continue;
			}

			Sum += RawCurvature[WrapIndex(Neighbor)];
			++Samples;
		}

		Points[Index].Curvature = Samples > 0 ? Sum / Samples : RawCurvature[Index];
	}
}

void FRacingLine::BuildSpeedProfile(TArray<FRacingLinePoint>& Points, const FRacingLineBuildSettings& Settings)
{
	const int32 Count = Points.Num();
	if (Count < 3)
	{
		return;
	}

	const float MaxSpeed = Settings.MaxSpeedKPH * KphToCms;
	const float MinSpeed = FMath::Min(Settings.MinSpeedKPH * KphToCms, MaxSpeed);

	for (FRacingLinePoint& Point : Points)
	{
		float CorneringSpeed = MaxSpeed;

		if (Point.Curvature > KINDA_SMALL_NUMBER)
		{
			CorneringSpeed = FMath::Sqrt((Settings.LateralGrip * GravityCmS2) / Point.Curvature);
		}

		Point.TargetSpeed = FMath::Clamp(CorneringSpeed, MinSpeed, MaxSpeed);
	}

	const float Spacing = Points.Num() > 0 && Points.Last().Distance > 0.0f
		? Points.Last().Distance / FMath::Max(1, Count - 1)
		: 0.0f;

	if (Spacing <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float BrakeAccel = Settings.BrakingLimit * GravityCmS2;
	const float DriveAccel = Settings.AccelerationLimit * GravityCmS2;

	const int32 Sweeps = Settings.bClosedLoop ? 2 : 1;

	for (int32 Sweep = 0; Sweep < Sweeps; ++Sweep)
	{

		for (int32 Step = Count - 1; Step >= 0; --Step)
		{
			const int32 Index = Settings.bClosedLoop ? ((Step % Count) + Count) % Count : Step;
			const int32 NextIndex = Settings.bClosedLoop ? (Index + 1) % Count : FMath::Min(Index + 1, Count - 1);

			if (NextIndex == Index)
			{
				continue;
			}

			const float Reachable = FMath::Sqrt(FMath::Square(Points[NextIndex].TargetSpeed) + 2.0f * BrakeAccel * Spacing);
			Points[Index].TargetSpeed = FMath::Min(Points[Index].TargetSpeed, Reachable);
		}

		for (int32 Step = 0; Step < Count; ++Step)
		{
			const int32 Index = Settings.bClosedLoop ? Step % Count : Step;
			const int32 PreviousIndex = Settings.bClosedLoop ? ((Index - 1 + Count) % Count) : FMath::Max(Index - 1, 0);

			if (PreviousIndex == Index)
			{
				continue;
			}

			const float Reachable = FMath::Sqrt(FMath::Square(Points[PreviousIndex].TargetSpeed) + 2.0f * DriveAccel * Spacing);
			Points[Index].TargetSpeed = FMath::Min(Points[Index].TargetSpeed, Reachable);
		}
	}

	for (FRacingLinePoint& Point : Points)
	{
		Point.TargetSpeed = FMath::Max(Point.TargetSpeed, MinSpeed);
	}
}

FRacingLine FRacingLine::BuildFromRecording(const FVehicleLapRecording& Recording, const FRacingLineBuildSettings& Settings)
{
	FRacingLine Line;
	Line.bClosedLoop = Settings.bClosedLoop;

	if (!Recording.IsValid())
	{
		UE_LOG(LogCVehicleArea, Warning, TEXT("RacingLine: '%s' has too few samples."), *Recording.DriverLabel);
		return Line;
	}

	if (!Resample(Recording, Settings, Line.Points, Line.TotalLength))
	{
		UE_LOG(LogCVehicleArea, Warning, TEXT("RacingLine: '%s' covered too little distance."), *Recording.DriverLabel);
		Line.Points.Reset();
		return Line;
	}

	ComputeGeometry(Line.Points, Settings);
	BuildSpeedProfile(Line.Points, Settings);

	UE_LOG(LogCVehicleArea, Log, TEXT("RacingLine: %d points, %.0f m from '%s'."),
		Line.Points.Num(), Line.TotalLength / 100.0f, *Recording.DriverLabel);

	return Line;
}
