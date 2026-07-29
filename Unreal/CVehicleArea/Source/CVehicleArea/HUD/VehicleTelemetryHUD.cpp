#include "VehicleTelemetryHUD.h"
#include "Dynamics/VehicleDriverAssistComponent.h"
#include "Dynamics/VehicleSurfaceResponseComponent.h"
#include "Replay/VehicleLapRecorderComponent.h"
#include "Race/RaceDirectorSubsystem.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "ChaosVehicleWheel.h"
#include "WheeledVehiclePawn.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if ENABLE_DRAW_DEBUG
#include "DrawDebugHelpers.h"
#endif

namespace VehicleHUD
{
	const FLinearColor PanelColor = FLinearColor(0.02f, 0.02f, 0.03f, 0.72f);
	const FLinearColor TextColor = FLinearColor(0.90f, 0.92f, 0.95f, 1.0f);
	const FLinearColor DimTextColor = FLinearColor(0.55f, 0.58f, 0.62f, 1.0f);
	const FLinearColor AccentColor = FLinearColor(0.20f, 0.75f, 1.00f, 1.0f);
	const FLinearColor WarnColor = FLinearColor(1.00f, 0.65f, 0.10f, 1.0f);
	const FLinearColor AlertColor = FLinearColor(1.00f, 0.25f, 0.20f, 1.0f);
	const FLinearColor GoodColor = FLinearColor(0.30f, 0.90f, 0.40f, 1.0f);
	const FLinearColor TrackColor = FLinearColor(0.12f, 0.13f, 0.15f, 0.9f);

	constexpr float ReferenceHeight = 1080.0f;
	constexpr float GaugeStartDegrees = 135.0f;
	constexpr float GaugeSweepDegrees = 270.0f;
	constexpr float TractionCircleMaxG = 2.0f;
	constexpr int32 TrailLength = 90;
}

AVehicleTelemetryHUD::AVehicleTelemetryHUD()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AVehicleTelemetryHUD::CyclePage()
{
	const uint8 Next = (static_cast<uint8>(CurrentPage) + 1) % (static_cast<uint8>(EVehicleHUDPage::Graphs) + 1);
	CurrentPage = static_cast<EVehicleHUDPage>(Next);
}

void AVehicleTelemetryHUD::SetPage(EVehicleHUDPage NewPage)
{
	CurrentPage = NewPage;
}

bool AVehicleTelemetryHUD::ResolveVehicle()
{
	APawn* CurrentPawn = GetOwningPawn();

	if (CurrentPawn != VehiclePawn)
	{
		VehiclePawn = CurrentPawn;
		MovementComponent = nullptr;
		Telemetry = nullptr;
		DriverAssists = nullptr;
		SurfaceResponse = nullptr;
		LapRecorder = nullptr;
		GForceTrail.Reset();
		PeakCombinedG = 0.0f;

		if (AWheeledVehiclePawn* Vehicle = Cast<AWheeledVehiclePawn>(CurrentPawn))
		{
			MovementComponent = Cast<UChaosWheeledVehicleMovementComponent>(Vehicle->GetVehicleMovementComponent());
			Telemetry = Vehicle->FindComponentByClass<UVehicleTelemetryComponent>();
			DriverAssists = Vehicle->FindComponentByClass<UVehicleDriverAssistComponent>();
			SurfaceResponse = Vehicle->FindComponentByClass<UVehicleSurfaceResponseComponent>();
			LapRecorder = Vehicle->FindComponentByClass<UVehicleLapRecorderComponent>();
		}
	}

	if (!HUDFont)
	{
		HUDFont = GEngine ? GEngine->GetMediumFont() : nullptr;
	}

	return MovementComponent != nullptr && Telemetry != nullptr;
}

void AVehicleTelemetryHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!Canvas)
	{
		return;
	}

	UIScale = FMath::Max(0.6f, Canvas->SizeY / VehicleHUD::ReferenceHeight);

	if (!ResolveVehicle())
	{
		return;
	}

	const FVehicleTelemetryFrame& Frame = Telemetry->GetLatestFrame();
	const float CombinedG = Frame.GetCombinedG();

	PeakCombinedG = FMath::Max(PeakCombinedG, CombinedG);

	GForceTrail.Add(FVector2D(Frame.AccelerationG.Y, Frame.AccelerationG.X));
	if (GForceTrail.Num() > VehicleHUD::TrailLength)
	{
		GForceTrail.RemoveAt(0, GForceTrail.Num() - VehicleHUD::TrailLength, EAllowShrinking::No);
	}

	DrawWorldDebug();

	if (bDrawStandings)
	{
		DrawStandingsPanel();
	}

	switch (CurrentPage)
	{
	case EVehicleHUDPage::Core:		DrawCorePage(); break;
	case EVehicleHUDPage::Wheels:	DrawWheelsPage(); break;
	case EVehicleHUDPage::Balance:	DrawBalancePage(); break;
	case EVehicleHUDPage::Graphs:	DrawGraphsPage(); break;
	default: break;
	}
}

void AVehicleTelemetryHUD::DrawLabel(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale)
{
	DrawText(Text, Color, X, Y, HUDFont, Scale * UIScale);
}

void AVehicleTelemetryHUD::DrawPanel(float X, float Y, float Width, float Height)
{
	DrawRect(VehicleHUD::PanelColor, X, Y, Width, Height);
}

FLinearColor AVehicleTelemetryHUD::GetHeatColor(float Value)
{
	const float Clamped = FMath::Clamp(Value, 0.0f, 1.0f);

	if (Clamped < 0.5f)
	{
		return FMath::Lerp(VehicleHUD::GoodColor, VehicleHUD::WarnColor, Clamped * 2.0f);
	}

	return FMath::Lerp(VehicleHUD::WarnColor, VehicleHUD::AlertColor, (Clamped - 0.5f) * 2.0f);
}

FString AVehicleTelemetryHUD::GetGearText(int32 Gear)
{
	if (Gear < 0)
	{
		return TEXT("R");
	}

	if (Gear == 0)
	{
		return TEXT("N");
	}

	return FString::FromInt(Gear);
}

FString AVehicleTelemetryHUD::FormatTime(float Seconds)
{
	return URaceDirectorSubsystem::FormatLapTime(Seconds);
}

void AVehicleTelemetryHUD::DrawArc(float CenterX, float CenterY, float Radius, float StartDegrees, float EndDegrees, const FLinearColor& Color, float Thickness, int32 Segments)
{
	const int32 Steps = FMath::Max(2, Segments);
	const float StepDegrees = (EndDegrees - StartDegrees) / Steps;

	for (int32 Step = 0; Step < Steps; ++Step)
	{
		const float A = FMath::DegreesToRadians(StartDegrees + StepDegrees * Step);
		const float B = FMath::DegreesToRadians(StartDegrees + StepDegrees * (Step + 1));

		DrawLine(
			CenterX + FMath::Cos(A) * Radius, CenterY + FMath::Sin(A) * Radius,
			CenterX + FMath::Cos(B) * Radius, CenterY + FMath::Sin(B) * Radius,
			Color, Thickness);
	}
}

void AVehicleTelemetryHUD::DrawRing(float CenterX, float CenterY, float Radius, const FLinearColor& Color, float Thickness, int32 Segments)
{
	DrawArc(CenterX, CenterY, Radius, 0.0f, 360.0f, Color, Thickness, Segments);
}

void AVehicleTelemetryHUD::DrawIndicator(float X, float Y, float Size, bool bLit, const FLinearColor& LitColor, const FString& Label)
{
	DrawRect(bLit ? LitColor : VehicleHUD::TrackColor, X, Y, Size, Size);
	DrawLabel(Label, X + Size + 5.0f * UIScale, Y - 1.0f * UIScale, bLit ? LitColor : VehicleHUD::DimTextColor, 0.72f);
}

void AVehicleTelemetryHUD::DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& FillColor, const FString& Label, bool bBidirectional)
{
	DrawRect(VehicleHUD::TrackColor, X, Y, Width, Height);

	if (bBidirectional)
	{
		const float Center = X + Width * 0.5f;
		const float Clamped = FMath::Clamp(Fraction, -1.0f, 1.0f);
		const float FillWidth = FMath::Abs(Clamped) * Width * 0.5f;

		if (FillWidth > 0.5f)
		{
			const float FillX = Clamped >= 0.0f ? Center : Center - FillWidth;
			DrawRect(FillColor, FillX, Y, FillWidth, Height);
		}

		DrawLine(Center, Y, Center, Y + Height, VehicleHUD::DimTextColor, 1.0f);
	}
	else
	{
		const float FillWidth = FMath::Clamp(Fraction, 0.0f, 1.0f) * Width;
		if (FillWidth > 0.5f)
		{
			DrawRect(FillColor, X, Y, FillWidth, Height);
		}
	}

	if (!Label.IsEmpty())
	{
		DrawLabel(Label, X + 4.0f * UIScale, Y + Height * 0.5f - 8.0f * UIScale, VehicleHUD::TextColor, 0.8f);
	}
}

void AVehicleTelemetryHUD::DrawTrace(float X, float Y, float Width, float Height, const TArray<float>& Values, const FLinearColor& Color, const FString& Label)
{
	DrawRect(VehicleHUD::TrackColor, X, Y, Width, Height);

	if (Values.Num() < 2)
	{
		DrawLabel(Label + TEXT(" (no data)"), X + 6.0f * UIScale, Y + 4.0f * UIScale, VehicleHUD::DimTextColor, 0.75f);
		return;
	}

	float MinValue = TNumericLimits<float>::Max();
	float MaxValue = TNumericLimits<float>::Lowest();

	for (const float Value : Values)
	{
		MinValue = FMath::Min(MinValue, Value);
		MaxValue = FMath::Max(MaxValue, Value);
	}

	float Range = MaxValue - MinValue;
	if (Range < KINDA_SMALL_NUMBER)
	{
		MinValue -= 0.5f;
		Range = 1.0f;
	}

	for (int32 Division = 1; Division < 4; ++Division)
	{
		const float GridY = Y + Height * (Division / 4.0f);
		DrawLine(X, GridY, X + Width, GridY, FLinearColor(1.0f, 1.0f, 1.0f, 0.05f), 1.0f);
	}

	if (MinValue < 0.0f && MaxValue > 0.0f)
	{
		const float ZeroY = Y + Height - ((0.0f - MinValue) / Range) * Height;
		DrawLine(X, ZeroY, X + Width, ZeroY, VehicleHUD::DimTextColor, 1.0f);
	}

	const float StepX = Width / (Values.Num() - 1);

	for (int32 Index = 1; Index < Values.Num(); ++Index)
	{
		const float PreviousX = X + (Index - 1) * StepX;
		const float CurrentX = X + Index * StepX;
		const float PreviousY = Y + Height - ((Values[Index - 1] - MinValue) / Range) * Height;
		const float CurrentY = Y + Height - ((Values[Index] - MinValue) / Range) * Height;

		DrawLine(PreviousX, PreviousY, CurrentX, CurrentY, Color, 1.5f);
	}

	const float LatestY = Y + Height - ((Values.Last() - MinValue) / Range) * Height;
	DrawRect(Color, X + Width - 3.0f * UIScale, LatestY - 2.0f * UIScale, 5.0f * UIScale, 5.0f * UIScale);

	DrawLabel(FString::Printf(TEXT("%s  %.2f"), *Label, Values.Last()),
		X + 6.0f * UIScale, Y + 2.0f * UIScale, VehicleHUD::TextColor, 0.72f);

	DrawLabel(FString::Printf(TEXT("%.1f"), MaxValue), X + Width - 42.0f * UIScale, Y + 2.0f * UIScale, VehicleHUD::DimTextColor, 0.65f);
	DrawLabel(FString::Printf(TEXT("%.1f"), MinValue), X + Width - 42.0f * UIScale, Y + Height - 16.0f * UIScale, VehicleHUD::DimTextColor, 0.65f);
}

void AVehicleTelemetryHUD::DrawTachometer(float CenterX, float CenterY, float Radius, const FVehicleTelemetryFrame& Frame)
{
	const float MaxRPM = FMath::Max(1000.0f, MovementComponent->EngineSetup.MaxRPM);
	const float RedlineRPM = MovementComponent->TransmissionSetup.ChangeUpRPM;
	const float RPMFraction = FMath::Clamp(Frame.EngineRPM / MaxRPM, 0.0f, 1.0f);
	const float RedlineFraction = FMath::Clamp(RedlineRPM / MaxRPM, 0.0f, 1.0f);

	const float Start = VehicleHUD::GaugeStartDegrees;
	const float Sweep = VehicleHUD::GaugeSweepDegrees;

	DrawArc(CenterX, CenterY, Radius, Start, Start + Sweep, VehicleHUD::TrackColor, 10.0f * UIScale);
	DrawArc(CenterX, CenterY, Radius, Start + Sweep * RedlineFraction, Start + Sweep, VehicleHUD::AlertColor, 4.0f * UIScale);

	const bool bNearRedline = Frame.EngineRPM >= RedlineRPM;
	DrawArc(CenterX, CenterY, Radius, Start, Start + Sweep * RPMFraction,
		bNearRedline ? VehicleHUD::AlertColor : VehicleHUD::AccentColor, 10.0f * UIScale);

	const int32 TickCount = FMath::Clamp(FMath::RoundToInt(MaxRPM / 1000.0f), 2, 12);
	for (int32 Tick = 0; Tick <= TickCount; ++Tick)
	{
		const float TickFraction = static_cast<float>(Tick) / TickCount;
		const float Angle = FMath::DegreesToRadians(Start + Sweep * TickFraction);
		const float Inner = Radius - 16.0f * UIScale;
		const float Outer = Radius - 6.0f * UIScale;

		DrawLine(
			CenterX + FMath::Cos(Angle) * Inner, CenterY + FMath::Sin(Angle) * Inner,
			CenterX + FMath::Cos(Angle) * Outer, CenterY + FMath::Sin(Angle) * Outer,
			VehicleHUD::DimTextColor, 1.5f);

		const float TextRadius = Radius - 32.0f * UIScale;
		DrawLabel(FString::FromInt(Tick),
			CenterX + FMath::Cos(Angle) * TextRadius - 4.0f * UIScale,
			CenterY + FMath::Sin(Angle) * TextRadius - 8.0f * UIScale,
			VehicleHUD::DimTextColor, 0.62f);
	}

	const float NeedleAngle = FMath::DegreesToRadians(Start + Sweep * RPMFraction);
	DrawLine(
		CenterX + FMath::Cos(NeedleAngle) * (Radius * 0.25f), CenterY + FMath::Sin(NeedleAngle) * (Radius * 0.25f),
		CenterX + FMath::Cos(NeedleAngle) * (Radius - 18.0f * UIScale), CenterY + FMath::Sin(NeedleAngle) * (Radius - 18.0f * UIScale),
		bNearRedline ? VehicleHUD::AlertColor : VehicleHUD::TextColor, 3.0f * UIScale);

	DrawLabel(GetGearText(Frame.Gear), CenterX - 10.0f * UIScale, CenterY - 34.0f * UIScale,
		bNearRedline ? VehicleHUD::AlertColor : VehicleHUD::TextColor, 2.4f);

	DrawLabel(FString::Printf(TEXT("%.0f"), Frame.SpeedKPH), CenterX - 26.0f * UIScale, CenterY + 12.0f * UIScale, VehicleHUD::TextColor, 1.6f);
	DrawLabel(TEXT("km/h"), CenterX - 16.0f * UIScale, CenterY + 40.0f * UIScale, VehicleHUD::DimTextColor, 0.7f);
	DrawLabel(FString::Printf(TEXT("%.0f rpm"), Frame.EngineRPM), CenterX - 30.0f * UIScale, CenterY + 58.0f * UIScale, VehicleHUD::DimTextColor, 0.7f);
}

void AVehicleTelemetryHUD::DrawTractionCircle(float CenterX, float CenterY, float Radius, const FVehicleTelemetryFrame& Frame)
{
	const float MaxG = VehicleHUD::TractionCircleMaxG;

	DrawRing(CenterX, CenterY, Radius, VehicleHUD::TrackColor, 2.0f);
	DrawRing(CenterX, CenterY, Radius * 0.5f, FLinearColor(1.0f, 1.0f, 1.0f, 0.10f), 1.0f);
	DrawRing(CenterX, CenterY, Radius * (1.0f / MaxG), VehicleHUD::DimTextColor, 1.0f);

	DrawLine(CenterX - Radius, CenterY, CenterX + Radius, CenterY, FLinearColor(1.0f, 1.0f, 1.0f, 0.12f), 1.0f);
	DrawLine(CenterX, CenterY - Radius, CenterX, CenterY + Radius, FLinearColor(1.0f, 1.0f, 1.0f, 0.12f), 1.0f);

	if (PeakCombinedG > KINDA_SMALL_NUMBER)
	{
		DrawRing(CenterX, CenterY, Radius * FMath::Min(PeakCombinedG / MaxG, 1.0f), VehicleHUD::WarnColor, 1.0f);
	}

	const int32 TrailCount = GForceTrail.Num();
	for (int32 Index = 0; Index < TrailCount; ++Index)
	{
		const float Age = static_cast<float>(Index) / FMath::Max(1, TrailCount - 1);
		const float PointX = CenterX + FMath::Clamp(GForceTrail[Index].X / MaxG, -1.0f, 1.0f) * Radius;
		const float PointY = CenterY - FMath::Clamp(GForceTrail[Index].Y / MaxG, -1.0f, 1.0f) * Radius;
		const float Size = 2.0f * UIScale;

		DrawRect(FLinearColor(VehicleHUD::AccentColor.R, VehicleHUD::AccentColor.G, VehicleHUD::AccentColor.B, Age * 0.6f),
			PointX - Size * 0.5f, PointY - Size * 0.5f, Size, Size);
	}

	const float CurrentX = CenterX + FMath::Clamp(Frame.AccelerationG.Y / MaxG, -1.0f, 1.0f) * Radius;
	const float CurrentY = CenterY - FMath::Clamp(Frame.AccelerationG.X / MaxG, -1.0f, 1.0f) * Radius;
	const float Marker = 6.0f * UIScale;

	DrawRect(GetHeatColor(Frame.GetCombinedG() / MaxG), CurrentX - Marker * 0.5f, CurrentY - Marker * 0.5f, Marker, Marker);

	DrawLabel(TEXT("TRACTION CIRCLE"), CenterX - Radius, CenterY - Radius - 20.0f * UIScale, VehicleHUD::AccentColor, 0.8f);
	DrawLabel(FString::Printf(TEXT("%.2f g  peak %.2f g"), Frame.GetCombinedG(), PeakCombinedG),
		CenterX - Radius, CenterY + Radius + 6.0f * UIScale, VehicleHUD::TextColor, 0.78f);
	DrawLabel(TEXT("1 g"), CenterX + Radius * (1.0f / MaxG) + 3.0f * UIScale, CenterY - 8.0f * UIScale, VehicleHUD::DimTextColor, 0.62f);
}

void AVehicleTelemetryHUD::DrawCarPlanView(float X, float Y, float Width, float Height, const FVehicleTelemetryFrame& Frame)
{
	DrawRect(VehicleHUD::TrackColor, X, Y, Width, Height);

	const float BodyInsetX = Width * 0.30f;
	const float BodyInsetY = Height * 0.10f;
	DrawRect(FLinearColor(1.0f, 1.0f, 1.0f, 0.05f), X + BodyInsetX, Y + BodyInsetY, Width - BodyInsetX * 2.0f, Height - BodyInsetY * 2.0f);

	const int32 NumWheels = FMath::Min(Frame.Wheels.Num(), MovementComponent->Wheels.Num());

	int32 FrontSeen = 0;
	int32 RearSeen = 0;

	for (int32 WheelIndex = 0; WheelIndex < NumWheels; ++WheelIndex)
	{
		UChaosVehicleWheel* Wheel = MovementComponent->Wheels[WheelIndex];
		if (!Wheel)
		{
			continue;
		}

		const bool bIsFront = Wheel->GetAxleType() != EAxleType::Rear;
		int32& SeenOnAxle = bIsFront ? FrontSeen : RearSeen;
		const bool bIsLeft = (SeenOnAxle % 2) == 0;
		++SeenOnAxle;

		const FWheelTelemetrySample& Sample = Frame.Wheels[WheelIndex];

		const float SlotX = bIsLeft ? X + Width * 0.14f : X + Width * 0.72f;
		const float SlotY = bIsFront ? Y + Height * 0.16f : Y + Height * 0.60f;
		const float SlotW = Width * 0.14f;
		const float SlotH = Height * 0.24f;

		const FLinearColor LoadColor = Sample.bInContact
			? GetHeatColor(Sample.NormalizedLoad * 2.0f)
			: VehicleHUD::DimTextColor;

		DrawRect(LoadColor, SlotX, SlotY, SlotW, SlotH);

		const float SlipFraction = FMath::Clamp(FMath::Abs(Sample.LongitudinalSlipRatio) / 0.4f, 0.0f, 1.0f);
		DrawRect(GetHeatColor(SlipFraction), SlotX, SlotY + SlotH + 3.0f * UIScale, SlotW * SlipFraction, 4.0f * UIScale);

		const FString Corner = FString::Printf(TEXT("%s%s"), bIsFront ? TEXT("F") : TEXT("R"), bIsLeft ? TEXT("L") : TEXT("R"));
		const float TextX = bIsLeft ? SlotX - 2.0f * UIScale : SlotX + SlotW - 18.0f * UIScale;

		DrawLabel(Corner, TextX, SlotY - 16.0f * UIScale, VehicleHUD::DimTextColor, 0.68f);
		DrawLabel(FString::Printf(TEXT("%.0f%%"), Sample.NormalizedLoad * 100.0f), TextX, SlotY + SlotH + 10.0f * UIScale, VehicleHUD::TextColor, 0.66f);

		if (Sample.bABSActivated)
		{
			DrawLabel(TEXT("ABS"), TextX, SlotY + SlotH * 0.4f, VehicleHUD::AlertColor, 0.62f);
		}
	}

	DrawLabel(FString::Printf(TEXT("%d/%d on ground"), Frame.WheelsInContact, NumWheels),
		X + 6.0f * UIScale, Y + Height - 18.0f * UIScale, VehicleHUD::DimTextColor, 0.68f);
}

void AVehicleTelemetryHUD::DrawInputStrip(float X, float Y, float Width, float Height)
{
	const int32 MaxSamples = FMath::Clamp(FMath::FloorToInt(Width), 2, 400);
	const float RowHeight = Height / 3.0f - 2.0f * UIScale;

	TArray<float> Values;

	Telemetry->GetChannelHistory(EVehicleTelemetryChannel::Throttle, MaxSamples, Values);
	DrawTrace(X, Y, Width, RowHeight, Values, VehicleHUD::GoodColor, TEXT("throttle"));

	Telemetry->GetChannelHistory(EVehicleTelemetryChannel::Brake, MaxSamples, Values);
	DrawTrace(X, Y + RowHeight + 2.0f * UIScale, Width, RowHeight, Values, VehicleHUD::AlertColor, TEXT("brake"));

	Telemetry->GetChannelHistory(EVehicleTelemetryChannel::Steering, MaxSamples, Values);
	DrawTrace(X, Y + (RowHeight + 2.0f * UIScale) * 2.0f, Width, RowHeight, Values, VehicleHUD::AccentColor, TEXT("steering"));
}

void AVehicleTelemetryHUD::DrawAssistLights(float X, float Y, float Width)
{
	if (!DriverAssists)
	{
		return;
	}

	const FVehicleAssistState& State = DriverAssists->GetAssistState();
	const float Size = 10.0f * UIScale;
	const float Spacing = Width / 5.0f;

	DrawIndicator(X, Y, Size, State.bTractionControlActive, VehicleHUD::WarnColor, TEXT("TC"));
	DrawIndicator(X + Spacing, Y, Size, State.bABSActive, VehicleHUD::AlertColor, TEXT("ABS"));
	DrawIndicator(X + Spacing * 2.0f, Y, Size, State.bStabilityControlActive, VehicleHUD::AccentColor, TEXT("ESC"));
	DrawIndicator(X + Spacing * 3.0f, Y, Size, State.bLaunchControlActive, VehicleHUD::GoodColor, TEXT("LC"));
	DrawIndicator(X + Spacing * 4.0f, Y, Size, DriverAssists->bActiveAeroEnabled, VehicleHUD::AccentColor, TEXT("AERO"));
}

void AVehicleTelemetryHUD::DrawLapBlock(float X, float Y, float Width)
{
	if (!LapRecorder)
	{
		return;
	}

	const FString Status = LapRecorder->IsRecording() ? TEXT("REC") : TEXT("IDLE");
	const FLinearColor StatusColor = LapRecorder->IsRecording() ? VehicleHUD::AlertColor : VehicleHUD::DimTextColor;

	DrawLabel(FString::Printf(TEXT("LAP  %s"), *Status), X, Y, StatusColor, 0.85f);
	DrawLabel(FString::Printf(TEXT("best %s"), *FormatTime(LapRecorder->GetBestLapTime())),
		X + Width * 0.5f, Y, VehicleHUD::TextColor, 0.85f);
}

void AVehicleTelemetryHUD::DrawCorePage()
{
	const FVehicleTelemetryFrame& Frame = Telemetry->GetLatestFrame();

	const float Margin = 24.0f * UIScale;
	const float GaugeRadius = 108.0f * UIScale;
	const float GaugeCenterX = Margin + GaugeRadius + 10.0f * UIScale;
	const float GaugeCenterY = Canvas->SizeY - Margin - GaugeRadius - 20.0f * UIScale;

	DrawTachometer(GaugeCenterX, GaugeCenterY, GaugeRadius, Frame);

	const float PanelX = GaugeCenterX + GaugeRadius + 20.0f * UIScale;
	const float PanelWidth = 430.0f * UIScale;
	const float PanelHeight = 248.0f * UIScale;
	const float PanelY = Canvas->SizeY - PanelHeight - Margin;

	DrawPanel(PanelX, PanelY, PanelWidth, PanelHeight);

	const float ContentX = PanelX + 14.0f * UIScale;
	const float ContentWidth = PanelWidth - 28.0f * UIScale;
	const float BarHeight = 16.0f * UIScale;

	float Cursor = PanelY + 10.0f * UIScale;

	DrawLapBlock(ContentX, Cursor, ContentWidth);
	Cursor += 24.0f * UIScale;

	DrawBar(ContentX, Cursor, ContentWidth, BarHeight, Frame.ThrottleInput, VehicleHUD::GoodColor, TEXT("throttle"));
	Cursor += BarHeight + 4.0f * UIScale;

	DrawBar(ContentX, Cursor, ContentWidth, BarHeight, Frame.BrakeInput, VehicleHUD::AlertColor, TEXT("brake"));
	Cursor += BarHeight + 4.0f * UIScale;

	DrawBar(ContentX, Cursor, ContentWidth, BarHeight, Frame.SteeringInput, VehicleHUD::AccentColor, TEXT("steering"), true);
	Cursor += BarHeight + 10.0f * UIScale;

	DrawAssistLights(ContentX, Cursor, ContentWidth);
	Cursor += 22.0f * UIScale;

	DrawLabel(FString::Printf(TEXT("long %+.2f g    lat %+.2f g    yaw %+.0f deg/s"),
		Frame.AccelerationG.X, Frame.AccelerationG.Y, Frame.YawRate),
		ContentX, Cursor, VehicleHUD::TextColor, 0.76f);
	Cursor += 18.0f * UIScale;

	const FLinearColor BalanceColor = Frame.UndersteerBalance > 0.0f ? VehicleHUD::WarnColor : VehicleHUD::AccentColor;
	DrawLabel(FString::Printf(TEXT("balance %+.1f deg  (%s)"),
		Frame.UndersteerBalance,
		Frame.UndersteerBalance > 0.5f ? TEXT("understeer") : (Frame.UndersteerBalance < -0.5f ? TEXT("oversteer") : TEXT("neutral"))),
		ContentX, Cursor, BalanceColor, 0.76f);
	Cursor += 18.0f * UIScale;

	if (SurfaceResponse)
	{
		DrawLabel(FString::Printf(TEXT("surface %s"), *SurfaceResponse->GetDominantSurface().ToString()),
			ContentX, Cursor, VehicleHUD::DimTextColor, 0.74f);
	}

	Cursor += 20.0f * UIScale;
	DrawInputStrip(ContentX, Cursor, ContentWidth, PanelY + PanelHeight - Cursor - 10.0f * UIScale);
}

void AVehicleTelemetryHUD::DrawWheelsPage()
{
	const FVehicleTelemetryFrame& Frame = Telemetry->GetLatestFrame();

	const float Margin = 24.0f * UIScale;
	const float PanelWidth = 620.0f * UIScale;
	const float PanelHeight = 300.0f * UIScale;
	const float PanelX = Margin;
	const float PanelY = Canvas->SizeY - PanelHeight - Margin;

	DrawPanel(PanelX, PanelY, PanelWidth, PanelHeight);

	const float ContentX = PanelX + 14.0f * UIScale;
	DrawLabel(TEXT("WHEELS"), ContentX, PanelY + 10.0f * UIScale, VehicleHUD::AccentColor, 0.9f);

	const float PlanWidth = 200.0f * UIScale;
	const float PlanHeight = 236.0f * UIScale;
	DrawCarPlanView(ContentX, PanelY + 34.0f * UIScale, PlanWidth, PlanHeight, Frame);

	const float DetailX = ContentX + PlanWidth + 16.0f * UIScale;
	const float DetailWidth = PanelWidth - (DetailX - PanelX) - 14.0f * UIScale;
	const float RowHeight = 54.0f * UIScale;
	const float BarHeight = 9.0f * UIScale;

	const int32 NumWheels = FMath::Min(Frame.Wheels.Num(), MovementComponent->Wheels.Num());
	float Cursor = PanelY + 34.0f * UIScale;

	for (int32 WheelIndex = 0; WheelIndex < NumWheels && WheelIndex < 4; ++WheelIndex)
	{
		const FWheelTelemetrySample& Sample = Frame.Wheels[WheelIndex];

		DrawLabel(FString::Printf(TEXT("W%d  %s  %.0f Nm"),
			WheelIndex,
			Sample.bInContact ? *Sample.SurfaceName.ToString() : TEXT("air"),
			Sample.DriveTorque),
			DetailX, Cursor, Sample.bInContact ? VehicleHUD::TextColor : VehicleHUD::DimTextColor, 0.72f);

		DrawBar(DetailX, Cursor + 15.0f * UIScale, DetailWidth, BarHeight,
			Sample.NormalizedLoad * 2.0f, GetHeatColor(Sample.NormalizedLoad * 2.0f), FString());

		DrawBar(DetailX, Cursor + 27.0f * UIScale, DetailWidth, BarHeight,
			FMath::Clamp(Sample.LongitudinalSlipRatio / 0.4f, -1.0f, 1.0f),
			GetHeatColor(FMath::Abs(Sample.LongitudinalSlipRatio) / 0.4f), FString(), true);

		DrawBar(DetailX, Cursor + 39.0f * UIScale, DetailWidth, BarHeight,
			FMath::Clamp(Sample.SlipAngle / 12.0f, -1.0f, 1.0f),
			GetHeatColor(FMath::Abs(Sample.SlipAngle) / 12.0f), FString(), true);

		Cursor += RowHeight;
	}

	DrawLabel(TEXT("load / slip ratio / slip angle"), DetailX, PanelY + PanelHeight - 20.0f * UIScale, VehicleHUD::DimTextColor, 0.66f);
}

void AVehicleTelemetryHUD::DrawBalancePage()
{
	const FVehicleTelemetryFrame& Frame = Telemetry->GetLatestFrame();

	const float Margin = 24.0f * UIScale;
	const float PanelWidth = 560.0f * UIScale;
	const float PanelHeight = 300.0f * UIScale;
	const float PanelX = Margin;
	const float PanelY = Canvas->SizeY - PanelHeight - Margin;

	DrawPanel(PanelX, PanelY, PanelWidth, PanelHeight);

	const float CircleRadius = 100.0f * UIScale;
	const float CircleCenterX = PanelX + 20.0f * UIScale + CircleRadius;
	const float CircleCenterY = PanelY + 40.0f * UIScale + CircleRadius;

	DrawTractionCircle(CircleCenterX, CircleCenterY, CircleRadius, Frame);

	const float DetailX = CircleCenterX + CircleRadius + 30.0f * UIScale;
	const float DetailWidth = PanelWidth - (DetailX - PanelX) - 16.0f * UIScale;
	const float BarHeight = 14.0f * UIScale;

	float Cursor = PanelY + 16.0f * UIScale;

	DrawLabel(TEXT("BALANCE"), DetailX, Cursor, VehicleHUD::AccentColor, 0.9f);
	Cursor += 24.0f * UIScale;

	DrawLabel(FString::Printf(TEXT("front load %.0f%%"), Frame.FrontAxleLoadFraction * 100.0f), DetailX, Cursor, VehicleHUD::TextColor, 0.74f);
	Cursor += 16.0f * UIScale;
	DrawBar(DetailX, Cursor, DetailWidth, BarHeight, (Frame.FrontAxleLoadFraction - 0.5f) * 2.0f, VehicleHUD::AccentColor, FString(), true);
	Cursor += BarHeight + 10.0f * UIScale;

	DrawLabel(FString::Printf(TEXT("left load %.0f%%"), Frame.LeftSideLoadFraction * 100.0f), DetailX, Cursor, VehicleHUD::TextColor, 0.74f);
	Cursor += 16.0f * UIScale;
	DrawBar(DetailX, Cursor, DetailWidth, BarHeight, (Frame.LeftSideLoadFraction - 0.5f) * 2.0f, VehicleHUD::WarnColor, FString(), true);
	Cursor += BarHeight + 10.0f * UIScale;

	DrawLabel(FString::Printf(TEXT("body slip %+.1f deg"), Frame.BodySlipAngle), DetailX, Cursor, VehicleHUD::TextColor, 0.74f);
	Cursor += 16.0f * UIScale;
	DrawBar(DetailX, Cursor, DetailWidth, BarHeight, FMath::Clamp(Frame.BodySlipAngle / 25.0f, -1.0f, 1.0f),
		GetHeatColor(FMath::Abs(Frame.BodySlipAngle) / 25.0f), FString(), true);
	Cursor += BarHeight + 10.0f * UIScale;

	const float YawError = Frame.TargetYawRate - Frame.YawRate;
	DrawLabel(FString::Printf(TEXT("yaw %+.0f  target %+.0f  err %+.0f"), Frame.YawRate, Frame.TargetYawRate, YawError),
		DetailX, Cursor, VehicleHUD::TextColor, 0.72f);
	Cursor += 16.0f * UIScale;
	DrawBar(DetailX, Cursor, DetailWidth, BarHeight, FMath::Clamp(YawError / 40.0f, -1.0f, 1.0f),
		GetHeatColor(FMath::Abs(YawError) / 40.0f), FString(), true);
	Cursor += BarHeight + 14.0f * UIScale;

	if (DriverAssists)
	{
		DrawLabel(DriverAssists->GetActiveAssistsLabel(), DetailX, Cursor, VehicleHUD::WarnColor, 0.74f);
	}
}

void AVehicleTelemetryHUD::DrawGraphsPage()
{
	const float Margin = 24.0f * UIScale;
	const float PanelWidth = 560.0f * UIScale;
	const float GraphHeight = 78.0f * UIScale;
	const float GraphSpacing = 10.0f * UIScale;
	const float PanelHeight = 44.0f * UIScale + (GraphHeight + GraphSpacing) * 4.0f;
	const float PanelX = Margin;
	const float PanelY = Canvas->SizeY - PanelHeight - Margin;

	DrawPanel(PanelX, PanelY, PanelWidth, PanelHeight);

	const float ContentX = PanelX + 14.0f * UIScale;
	const float ContentWidth = PanelWidth - 28.0f * UIScale;

	DrawLabel(FString::Printf(TEXT("TRACES  (%d samples held)"), Telemetry->GetFrameCount()),
		ContentX, PanelY + 10.0f * UIScale, VehicleHUD::AccentColor, 0.9f);

	float Cursor = PanelY + 36.0f * UIScale;

	const int32 MaxSamples = FMath::Clamp(FMath::FloorToInt(ContentWidth), 2, 512);

	struct FGraphSpec
	{
		EVehicleTelemetryChannel Channel;
		const TCHAR* Label;
		FLinearColor Color;
	};

	const FGraphSpec Graphs[] =
	{
		{ PrimaryGraphChannel,						TEXT("primary"),	VehicleHUD::AccentColor },
		{ SecondaryGraphChannel,					TEXT("secondary"),	VehicleHUD::WarnColor },
		{ EVehicleTelemetryChannel::YawRateError,	TEXT("yaw error"),	VehicleHUD::AlertColor },
		{ TertiaryGraphChannel,						TEXT("balance"),	VehicleHUD::GoodColor }
	};

	TArray<float> Values;

	for (const FGraphSpec& Graph : Graphs)
	{
		Telemetry->GetChannelHistory(Graph.Channel, MaxSamples, Values);
		DrawTrace(ContentX, Cursor, ContentWidth, GraphHeight, Values, Graph.Color, Graph.Label);
		Cursor += GraphHeight + GraphSpacing;
	}
}

void AVehicleTelemetryHUD::DrawStandingsPanel()
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	URaceDirectorSubsystem* Director = World->GetSubsystem<URaceDirectorSubsystem>();
	if (!Director || Director->GetRaceState() == ERaceState::Idle)
	{
		return;
	}

	const TArray<FRaceCompetitor>& Standings = Director->GetStandings();

	const float Margin = 24.0f * UIScale;
	const float PanelWidth = 300.0f * UIScale;
	const float RowHeight = 20.0f * UIScale;
	const float PanelHeight = 66.0f * UIScale + RowHeight * Standings.Num();
	const float PanelX = Canvas->SizeX - PanelWidth - Margin;
	const float PanelY = Margin;

	DrawPanel(PanelX, PanelY, PanelWidth, PanelHeight);

	const float ContentX = PanelX + 14.0f * UIScale;
	float Cursor = PanelY + 10.0f * UIScale;

	if (Director->GetRaceState() == ERaceState::Countdown)
	{
		const int32 Remaining = FMath::CeilToInt(Director->GetCountdownRemaining());
		DrawLabel(FString::Printf(TEXT("%d"), FMath::Max(1, Remaining)), ContentX, Cursor, VehicleHUD::AlertColor, 2.2f);
		Cursor += 46.0f * UIScale;
	}
	else
	{
		const FString Header = Director->GetRaceState() == ERaceState::Finished
			? TEXT("FINISHED")
			: FString::Printf(TEXT("RACE  %s"), *URaceDirectorSubsystem::FormatLapTime(Director->GetRaceElapsedTime()));

		DrawLabel(Header, ContentX, Cursor, VehicleHUD::AccentColor, 0.95f);
		Cursor += 26.0f * UIScale;
	}

	for (const FRaceCompetitor& Competitor : Standings)
	{
		const FLinearColor RowColor = Competitor.bIsPlayer ? VehicleHUD::AccentColor : VehicleHUD::TextColor;

		DrawLabel(FString::Printf(TEXT("%d. %s  L%d/%d  %s"),
			Competitor.Position,
			*Competitor.DisplayName,
			Competitor.LapsCompleted,
			Director->GetLapCount(),
			*URaceDirectorSubsystem::FormatLapTime(Competitor.BestLapTime)),
			ContentX, Cursor, RowColor, 0.78f);

		Cursor += RowHeight;
	}
}

void AVehicleTelemetryHUD::DrawWorldDebug()
{
#if ENABLE_DRAW_DEBUG
	UWorld* World = GetWorld();
	if (!World || !VehiclePawn)
	{
		return;
	}

	if (bDrawRacingLine)
	{
		if (const URaceDirectorSubsystem* Director = World->GetSubsystem<URaceDirectorSubsystem>())
		{
			const FRacingLine& Line = Director->GetReferenceLine();

			if (Line.IsValid())
			{
				float MaxTargetSpeed = KINDA_SMALL_NUMBER;
				for (const FRacingLinePoint& Point : Line.Points)
				{
					MaxTargetSpeed = FMath::Max(MaxTargetSpeed, Point.TargetSpeed);
				}

				const int32 Count = Line.Points.Num();
				for (int32 Index = 0; Index < Count; ++Index)
				{
					const int32 NextIndex = Line.bClosedLoop ? (Index + 1) % Count : Index + 1;
					if (NextIndex >= Count)
					{
						break;
					}

					const float SpeedFraction = Line.Points[Index].TargetSpeed / MaxTargetSpeed;
					const FLinearColor SegmentColor = FMath::Lerp(
						FLinearColor(1.0f, 0.15f, 0.1f),
						FLinearColor(0.2f, 1.0f, 0.3f),
						SpeedFraction);

					DrawDebugLine(World,
						Line.Points[Index].Location + FVector(0, 0, 25.0f),
						Line.Points[NextIndex].Location + FVector(0, 0, 25.0f),
						SegmentColor.ToFColor(true), false, -1.0f, 0, 6.0f);
				}
			}
		}
	}

	if (!MovementComponent || (!bDrawSuspensionLoad && !bDrawSlipVectors))
	{
		return;
	}

	const FVehicleTelemetryFrame& Frame = Telemetry->GetLatestFrame();
	const int32 NumWheels = FMath::Min(Frame.Wheels.Num(), MovementComponent->GetNumWheels());

	for (int32 WheelIndex = 0; WheelIndex < NumWheels; ++WheelIndex)
	{
		const FWheelTelemetrySample& Wheel = Frame.Wheels[WheelIndex];
		if (!Wheel.bInContact)
		{
			continue;
		}

		const FVector ContactPoint = MovementComponent->GetWheelState(WheelIndex).ContactPoint;

		if (bDrawSuspensionLoad)
		{
			const float BarHeight = Wheel.NormalizedLoad * 400.0f;
			DrawDebugLine(World, ContactPoint, ContactPoint + FVector(0, 0, BarHeight),
				GetHeatColor(Wheel.NormalizedLoad * 2.0f).ToFColor(true), false, -1.0f, 0, 5.0f);
		}

		if (bDrawSlipVectors)
		{
			const FVector SlipDirection = VehiclePawn->GetActorRightVector() * Wheel.SlipAngle
				+ VehiclePawn->GetActorForwardVector() * (Wheel.LongitudinalSlipRatio * 100.0f);

			if (!SlipDirection.IsNearlyZero())
			{
				DrawDebugDirectionalArrow(World,
					ContactPoint + FVector(0, 0, 10.0f),
					ContactPoint + FVector(0, 0, 10.0f) + SlipDirection * 3.0f,
					40.0f, FColor::Yellow, false, -1.0f, 0, 3.0f);
			}
		}
	}
#endif
}
