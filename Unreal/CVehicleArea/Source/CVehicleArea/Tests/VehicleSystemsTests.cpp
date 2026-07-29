#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Race/RacingLine.h"
#include "Race/RaceDirectorSubsystem.h"
#include "Tuning/VehicleSetupDataAsset.h"

namespace VehicleTests
{
	constexpr float GravityCmS2 = 980.665f;

	FVehicleLapRecording MakeCircularLap(float RadiusCm, int32 SampleCount, float SpeedCms)
	{
		FVehicleLapRecording Recording;
		Recording.DriverLabel = TEXT("UnitTest");
		Recording.bComplete = true;
		Recording.Samples.Reserve(SampleCount);

		const float Circumference = 2.0f * PI * RadiusCm;
		Recording.LapTime = Circumference / FMath::Max(SpeedCms, 1.0f);

		for (int32 Index = 0; Index < SampleCount; ++Index)
		{
			const float Angle = (2.0f * PI * Index) / SampleCount;

			FVehicleReplaySample Sample;
			Sample.TimeSeconds = (Recording.LapTime * Index) / SampleCount;
			Sample.Snapshot.Transform.SetLocation(FVector(FMath::Cos(Angle) * RadiusCm, FMath::Sin(Angle) * RadiusCm, 0.0f));
			Sample.Snapshot.LinearVelocity = FVector(-FMath::Sin(Angle), FMath::Cos(Angle), 0.0f) * SpeedCms;

			Recording.Samples.Add(MoveTemp(Sample));
		}

		return Recording;
	}

	FVehicleLapRecording MakeStraightIntoCorner(float StraightLengthCm, float CornerRadiusCm)
	{
		FVehicleLapRecording Recording;
		Recording.DriverLabel = TEXT("UnitTest");
		Recording.bComplete = true;

		constexpr float Step = 100.0f;
		float Time = 0.0f;

		for (float Distance = 0.0f; Distance <= StraightLengthCm; Distance += Step)
		{
			FVehicleReplaySample Sample;
			Sample.TimeSeconds = Time;
			Sample.Snapshot.Transform.SetLocation(FVector(Distance, 0.0f, 0.0f));
			Recording.Samples.Add(MoveTemp(Sample));
			Time += 0.05f;
		}

		const int32 ArcSamples = FMath::Max(8, FMath::RoundToInt((PI * 0.5f * CornerRadiusCm) / Step));
		for (int32 Index = 1; Index <= ArcSamples; ++Index)
		{
			const float Angle = (PI * 0.5f * Index) / ArcSamples;

			FVehicleReplaySample Sample;
			Sample.TimeSeconds = Time;
			Sample.Snapshot.Transform.SetLocation(FVector(
				StraightLengthCm + FMath::Sin(Angle) * CornerRadiusCm,
				CornerRadiusCm - FMath::Cos(Angle) * CornerRadiusCm,
				0.0f));

			Recording.Samples.Add(MoveTemp(Sample));
			Time += 0.05f;
		}

		return Recording;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRacingLineCurvatureTest,
	"CVehicleArea.RacingLine.CurvatureMatchesKnownCircle",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FRacingLineCurvatureTest::RunTest(const FString& Parameters)
{
	constexpr float Radius = 2000.0f;

	const FVehicleLapRecording Recording = VehicleTests::MakeCircularLap(Radius, 360, 1200.0f);

	FRacingLineBuildSettings Settings;
	Settings.PointSpacing = 200.0f;
	Settings.LateralGrip = 1.0f;
	Settings.bClosedLoop = true;
	Settings.MaxSpeedKPH = 400.0f;
	Settings.MinSpeedKPH = 1.0f;

	const FRacingLine Line = FRacingLine::BuildFromRecording(Recording, Settings);

	if (!TestTrue(TEXT("A circular lap produces a valid line"), Line.IsValid()))
	{
		return false;
	}

	const float ExpectedLength = 2.0f * PI * Radius;
	TestTrue(
		FString::Printf(TEXT("Line length %.0f is within 2%% of the circumference %.0f"), Line.TotalLength, ExpectedLength),
		FMath::Abs(Line.TotalLength - ExpectedLength) < ExpectedLength * 0.02f);

	const float ExpectedCurvature = 1.0f / Radius;

	float CurvatureSum = 0.0f;
	for (const FRacingLinePoint& Point : Line.Points)
	{
		CurvatureSum += Point.Curvature;
	}
	const float MeanCurvature = CurvatureSum / Line.Points.Num();

	TestTrue(
		FString::Printf(TEXT("Mean curvature %.6f is within 10%% of 1/r = %.6f"), MeanCurvature, ExpectedCurvature),
		FMath::Abs(MeanCurvature - ExpectedCurvature) < ExpectedCurvature * 0.10f);

	const float ExpectedSpeed = FMath::Sqrt(Settings.LateralGrip * VehicleTests::GravityCmS2 * Radius);

	float SpeedSum = 0.0f;
	for (const FRacingLinePoint& Point : Line.Points)
	{
		SpeedSum += Point.TargetSpeed;
	}
	const float MeanSpeed = SpeedSum / Line.Points.Num();

	TestTrue(
		FString::Printf(TEXT("Mean target speed %.0f cm/s is within 10%% of sqrt(mu*g*r) = %.0f"), MeanSpeed, ExpectedSpeed),
		FMath::Abs(MeanSpeed - ExpectedSpeed) < ExpectedSpeed * 0.10f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRacingLineBrakingTest,
	"CVehicleArea.RacingLine.BrakingPropagatesBackFromCorner",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FRacingLineBrakingTest::RunTest(const FString& Parameters)
{
	constexpr float StraightLength = 20000.0f;
	constexpr float CornerRadius = 800.0f;

	const FVehicleLapRecording Recording = VehicleTests::MakeStraightIntoCorner(StraightLength, CornerRadius);

	FRacingLineBuildSettings Settings;
	Settings.PointSpacing = 200.0f;
	Settings.LateralGrip = 1.0f;
	Settings.BrakingLimit = 1.0f;
	Settings.AccelerationLimit = 0.5f;
	Settings.bClosedLoop = false;
	Settings.MaxSpeedKPH = 300.0f;
	Settings.MinSpeedKPH = 10.0f;
	Settings.CurvatureSmoothing = 2;

	const FRacingLine Line = FRacingLine::BuildFromRecording(Recording, Settings);

	if (!TestTrue(TEXT("A straight-into-corner path produces a valid line"), Line.IsValid()))
	{
		return false;
	}

	int32 SlowestIndex = 0;
	float SlowestSpeed = TNumericLimits<float>::Max();

	for (int32 Index = 0; Index < Line.Points.Num(); ++Index)
	{
		if (Line.Points[Index].TargetSpeed < SlowestSpeed)
		{
			SlowestSpeed = Line.Points[Index].TargetSpeed;
			SlowestIndex = Index;
		}
	}

	TestTrue(
		FString::Printf(TEXT("The slowest station (%d of %d) is past the end of the straight"), SlowestIndex, Line.Points.Num()),
		Line.Points[SlowestIndex].Distance > StraightLength * 0.9f);

	const float ExpectedCornerSpeed = FMath::Sqrt(Settings.LateralGrip * VehicleTests::GravityCmS2 * CornerRadius);
	TestTrue(
		FString::Printf(TEXT("Corner speed %.0f cm/s is within 20%% of sqrt(mu*g*r) = %.0f"), SlowestSpeed, ExpectedCornerSpeed),
		FMath::Abs(SlowestSpeed - ExpectedCornerSpeed) < ExpectedCornerSpeed * 0.20f);

	const int32 ApproachIndex = FMath::Clamp(SlowestIndex - 8, 0, Line.Points.Num() - 1);
	TestTrue(
		FString::Printf(TEXT("Braking has propagated back up the straight: station %d is at %.0f cm/s versus %.0f in the corner"),
			ApproachIndex, Line.Points[ApproachIndex].TargetSpeed, SlowestSpeed),
		Line.Points[ApproachIndex].TargetSpeed > SlowestSpeed);

	bool bMonotonic = true;
	for (int32 Index = ApproachIndex; Index < SlowestIndex; ++Index)
	{
		if (Line.Points[Index + 1].TargetSpeed > Line.Points[Index].TargetSpeed + 1.0f)
		{
			bMonotonic = false;
			break;
		}
	}

	TestTrue(TEXT("Target speed decreases monotonically through the braking zone"), bMonotonic);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRacingLineIndexingTest,
	"CVehicleArea.RacingLine.IndexingWrapsAroundAClosedLoop",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FRacingLineIndexingTest::RunTest(const FString& Parameters)
{
	constexpr float Radius = 3000.0f;

	const FVehicleLapRecording Recording = VehicleTests::MakeCircularLap(Radius, 360, 1500.0f);

	FRacingLineBuildSettings Settings;
	Settings.PointSpacing = 250.0f;
	Settings.bClosedLoop = true;

	const FRacingLine Line = FRacingLine::BuildFromRecording(Recording, Settings);

	if (!TestTrue(TEXT("Line built"), Line.IsValid()))
	{
		return false;
	}

	const int32 Probe = Line.Points.Num() / 3;
	const FVector ProbeLocation = Line.Points[Probe].Location;

	TestEqual(TEXT("Full search finds the station under the probe"), Line.FindNearestPoint(ProbeLocation, INDEX_NONE), Probe);
	TestEqual(TEXT("Hinted search finds the same station"), Line.FindNearestPoint(ProbeLocation, Probe - 3), Probe);

	const int32 NearEnd = Line.Points.Num() - 2;
	const int32 Wrapped = Line.AdvanceIndex(NearEnd, Settings.PointSpacing * 6.0f);

	TestTrue(
		FString::Printf(TEXT("Stepping forward from station %d wrapped to %d"), NearEnd, Wrapped),
		Wrapped < NearEnd);

	const FRacingLinePoint Sampled = Line.SampleAtDistance(Line.TotalLength + 500.0f);
	const FRacingLinePoint Equivalent = Line.SampleAtDistance(500.0f);

	TestTrue(
		TEXT("Sampling past the end of a closed loop wraps to the same place"),
		FVector::Dist(Sampled.Location, Equivalent.Location) < 1.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVehicleSetupDerivedValuesTest,
	"CVehicleArea.Tuning.SetupDerivedValues",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FVehicleSetupDerivedValuesTest::RunTest(const FString& Parameters)
{
	UVehicleSetupDataAsset* Setup = NewObject<UVehicleSetupDataAsset>();
	if (!TestNotNull(TEXT("Setup asset created"), Setup))
	{
		return false;
	}

	Setup->FrontAxle.MaxBrakeTorque = 3000.0f;
	Setup->RearAxle.MaxBrakeTorque = 1000.0f;

	TestTrue(
		FString::Printf(TEXT("Front brake bias %.3f is 0.75"), Setup->GetFrontBrakeBias()),
		FMath::IsNearlyEqual(Setup->GetFrontBrakeBias(), 0.75f, 0.001f));

	Setup->FrontAxle.MaxBrakeTorque = 0.0f;
	Setup->RearAxle.MaxBrakeTorque = 0.0f;
	TestTrue(TEXT("Zero brake torque reports an even bias instead of dividing by zero"),
		FMath::IsNearlyEqual(Setup->GetFrontBrakeBias(), 0.5f, 0.001f));

	Setup->Drivetrain.MaxRPM = 7000.0f;
	Setup->Drivetrain.FinalRatio = 3.0f;
	Setup->Drivetrain.ForwardGearRatios = { 4.0f, 2.0f, 1.0f };
	Setup->RearAxle.WheelRadius = 35.0f;

	const float WheelRPM = 7000.0f / (1.0f * 3.0f);
	const float ExpectedKPH = (WheelRPM * 2.0f * PI * 35.0f) * 60.0f / 100000.0f;

	TestTrue(
		FString::Printf(TEXT("Theoretical top speed %.1f matches hand calculation %.1f"), Setup->GetTopSpeedKPH(), ExpectedKPH),
		FMath::IsNearlyEqual(Setup->GetTopSpeedKPH(), ExpectedKPH, 0.5f));

	Setup->Drivetrain.ForwardGearRatios = { 1.0f, 4.0f, 2.0f };
	TestTrue(
		TEXT("Top gear is the lowest ratio regardless of array order"),
		FMath::IsNearlyEqual(Setup->GetTopSpeedKPH(), ExpectedKPH, 0.5f));

	Setup->Drivetrain.ForwardGearRatios.Empty();
	TestTrue(TEXT("An empty gearbox reports zero rather than dividing by zero"),
		FMath::IsNearlyZero(Setup->GetTopSpeedKPH()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FLapTimeFormattingTest,
	"CVehicleArea.Race.LapTimeFormatting",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FLapTimeFormattingTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Under a minute"), URaceDirectorSubsystem::FormatLapTime(42.5f), FString(TEXT("0:42.500")));
	TestEqual(TEXT("Over a minute"), URaceDirectorSubsystem::FormatLapTime(95.25f), FString(TEXT("1:35.250")));
	TestEqual(TEXT("Exactly two minutes"), URaceDirectorSubsystem::FormatLapTime(120.0f), FString(TEXT("2:00.000")));

	TestEqual(TEXT("No time set"), URaceDirectorSubsystem::FormatLapTime(0.0f), FString(TEXT("--:--.---")));
	TestEqual(TEXT("Negative time"), URaceDirectorSubsystem::FormatLapTime(-1.0f), FString(TEXT("--:--.---")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FRacingLineDegenerateInputTest,
	"CVehicleArea.RacingLine.RejectsDegenerateRecordings",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FRacingLineDegenerateInputTest::RunTest(const FString& Parameters)
{
	FRacingLineBuildSettings Settings;

	TestFalse(TEXT("An empty recording produces no line"),
		FRacingLine::BuildFromRecording(FVehicleLapRecording(), Settings).IsValid());

	FVehicleLapRecording Stationary;
	Stationary.DriverLabel = TEXT("Stationary");
	for (int32 Index = 0; Index < 200; ++Index)
	{
		FVehicleReplaySample Sample;
		Sample.TimeSeconds = Index * 0.03f;
		Sample.Snapshot.Transform.SetLocation(FVector(100.0f, 200.0f, 0.0f));
		Stationary.Samples.Add(MoveTemp(Sample));
	}

	TestFalse(TEXT("A stationary recording produces no line"),
		FRacingLine::BuildFromRecording(Stationary, Settings).IsValid());

	FVehicleLapRecording Stub;
	Stub.DriverLabel = TEXT("Stub");
	for (int32 Index = 0; Index < 5; ++Index)
	{
		FVehicleReplaySample Sample;
		Sample.TimeSeconds = Index * 0.03f;
		Sample.Snapshot.Transform.SetLocation(FVector(Index * 20.0f, 0.0f, 0.0f));
		Stub.Samples.Add(MoveTemp(Sample));
	}

	TestFalse(TEXT("A path shorter than a few stations produces no line"),
		FRacingLine::BuildFromRecording(Stub, Settings).IsValid());

	return true;
}

#endif
