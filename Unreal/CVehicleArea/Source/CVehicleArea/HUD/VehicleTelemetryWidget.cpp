#include "VehicleTelemetryWidget.h"
#include "VehicleTelemetryHUD.h"
#include "Dynamics/VehicleDriverAssistComponent.h"
#include "Dynamics/VehicleSurfaceResponseComponent.h"
#include "Replay/VehicleLapRecorderComponent.h"
#include "Race/RaceDirectorSubsystem.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

namespace
{
constexpr float TraceWindowSeconds = 5.f;

}

void UVehicleTelemetryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    const APlayerController* Player = GetOwningPlayer();
    const AVehicleTelemetryHUD* HUD = Player ? Cast<AVehicleTelemetryHUD>(Player->GetHUD()) : nullptr;
    const APawn* Pawn = GetOwningPlayerPawn();
    const UVehicleTelemetryComponent* Telemetry = Pawn ? Pawn->FindComponentByClass<UVehicleTelemetryComponent>() : nullptr;
    const bool bAvailable = HUD && HUD->bShowHUD && Telemetry;
    const auto PageVisibility = [bAvailable](bool bVisible) { return bAvailable && bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed; };
    CoreVisibility = PageVisibility(HUD && HUD->CurrentPage == EVehicleHUDPage::Core);
    WheelsVisibility = PageVisibility(HUD && HUD->CurrentPage == EVehicleHUDPage::Wheels);
    BalanceVisibility = PageVisibility(HUD && HUD->CurrentPage == EVehicleHUDPage::Balance);
    GraphsVisibility = PageVisibility(HUD && HUD->CurrentPage == EVehicleHUDPage::Graphs);
    StandingsVisibility = ESlateVisibility::Collapsed;
    if (!bAvailable) { return; }
    if (LastPawn.Get() != Pawn) { LastPawn = const_cast<APawn*>(Pawn); PeakCombinedG = 0.f; }
    const FVehicleTelemetryFrame& Frame = Telemetry->GetLatestFrame();
    PeakCombinedG = FMath::Max(PeakCombinedG, Frame.GetCombinedG());
    SpeedText = FText::FromString(FString::Printf(TEXT("%.0f km/h"), Frame.SpeedKPH));
    GearText = FText::FromString(Frame.Gear < 0 ? TEXT("R") : Frame.Gear == 0 ? TEXT("N") : FString::FromInt(Frame.Gear));
    RPMText = FText::FromString(FString::Printf(TEXT("%.0f rpm"), Frame.EngineRPM));
    const auto* Movement = Telemetry->GetMovementComponent();
    RPMFraction = Movement ? FMath::Clamp(Frame.EngineRPM / FMath::Max(1000.f, Movement->EngineSetup.MaxRPM), 0.f, 1.f) : 0.f;
    Throttle = Frame.ThrottleInput;
    Brake = Frame.BrakeInput;
    Steering = (Frame.SteeringInput + 1.f) * 0.5f;
    FrontLoad = Frame.FrontAxleLoadFraction;
    LeftLoad = Frame.LeftSideLoadFraction;
    DynamicsText = FText::FromString(FString::Printf(TEXT("Long %+.2f g   Lat %+.2f g   Total %.2f g (peak %.2f)\nYaw %+.0f deg/s   Balance %+.1f deg"), Frame.AccelerationG.X, Frame.AccelerationG.Y, Frame.GetCombinedG(), PeakCombinedG, Frame.YawRate, Frame.UndersteerBalance));
    BalanceText = FText::FromString(FString::Printf(TEXT("Front %.0f%%   Left %.0f%%\nBody slip %+.1f deg\nYaw %+.0f   Target %+.0f   Error %+.0f\n%s"), FrontLoad*100.f, LeftLoad*100.f, Frame.BodySlipAngle, Frame.YawRate, Frame.TargetYawRate, Frame.TargetYawRate-Frame.YawRate, Frame.UndersteerBalance > .5f ? TEXT("UNDERSTEER") : Frame.UndersteerBalance < -.5f ? TEXT("OVERSTEER") : TEXT("NEUTRAL")));
    const auto* Recorder = Pawn->FindComponentByClass<UVehicleLapRecorderComponent>();
    LapText = Recorder ? FText::FromString(FString::Printf(TEXT("LAP %s   Best %s"), Recorder->IsRecording() ? TEXT("REC") : TEXT("IDLE"), *URaceDirectorSubsystem::FormatLapTime(Recorder->GetBestLapTime()))) : FText::GetEmpty();
    const auto* Surface = Pawn->FindComponentByClass<UVehicleSurfaceResponseComponent>();
    SurfaceText = Surface ? FText::FromName(Surface->GetDominantSurface()) : FText::GetEmpty();
    const auto* Assists = Pawn->FindComponentByClass<UVehicleDriverAssistComponent>();
    AssistsText = Assists ? FText::FromString(Assists->GetActiveAssistsLabel()) : FText::GetEmpty();
    FText* WheelTexts[] = { &Wheel0Text, &Wheel1Text, &Wheel2Text, &Wheel3Text };
    float* Loads[] = { &Wheel0Load, &Wheel1Load, &Wheel2Load, &Wheel3Load };
    float* Slips[] = { &Wheel0Slip, &Wheel1Slip, &Wheel2Slip, &Wheel3Slip };
    float* Angles[] = { &Wheel0Angle, &Wheel1Angle, &Wheel2Angle, &Wheel3Angle };
    for (int32 i = 0; i < 4; ++i)
    {
        *WheelTexts[i] = FText::GetEmpty(); *Loads[i] = 0.f; *Slips[i] = .5f; *Angles[i] = .5f;
        if (!Frame.Wheels.IsValidIndex(i)) { continue; }
        const auto& W = Frame.Wheels[i];
        *WheelTexts[i] = FText::FromString(FString::Printf(TEXT("W%d  %s  %.0f Nm  Load %.0f%%\nSlip %+.2f  Angle %+.1f deg  %s"), i, W.bInContact ? *W.SurfaceName.ToString() : TEXT("AIR"), W.DriveTorque, W.NormalizedLoad*100.f, W.LongitudinalSlipRatio, W.SlipAngle, W.bABSActivated ? TEXT("ABS") : TEXT("")));
        *Loads[i] = FMath::Clamp(W.NormalizedLoad*2.f,0.f,1.f);
        *Slips[i] = (FMath::Clamp(W.LongitudinalSlipRatio/.4f,-1.f,1.f)+1.f)*.5f;
        *Angles[i] = (FMath::Clamp(W.SlipAngle/12.f,-1.f,1.f)+1.f)*.5f;
    }
    GraphSummaryText = FText::FromString(FString::Printf(TEXT("TRACES  (%d samples)"), Telemetry->GetFrameCount()));
    const EVehicleTelemetryChannel Channels[] = { HUD->PrimaryGraphChannel, HUD->SecondaryGraphChannel, EVehicleTelemetryChannel::YawRateError, HUD->TertiaryGraphChannel };
    FText* GraphTexts[] = { &Graph0Text, &Graph1Text, &Graph2Text, &Graph3Text };
    for (int32 i=0; i<4; ++i)
    {
        TArray<FVector2D> Samples;
        Telemetry->GetChannelWindow(Channels[i], TraceWindowSeconds, Samples);
        const FString Name = StaticEnum<EVehicleTelemetryChannel>()->GetDisplayNameTextByValue(static_cast<int64>(Channels[i])).ToString();
        if (Samples.IsEmpty()) { *GraphTexts[i] = FText::FromString(Name + TEXT(" (no data)")); continue; }
        const FVector2D Range = Telemetry->GetChannelDisplayRange(Channels[i]);
        *GraphTexts[i] = FText::FromString(FString::Printf(TEXT("%s  %.2f   [axis %.2f .. %.2f]"), *Name, Samples.Last().Y, Range.X, Range.Y));
    }
    const auto* Director = GetWorld()->GetSubsystem<URaceDirectorSubsystem>();
    if (Director && HUD->bDrawStandings && Director->GetRaceState() != ERaceState::Idle)
    {
        StandingsVisibility = ESlateVisibility::HitTestInvisible;
        FString Rows = Director->GetRaceState() == ERaceState::Countdown ? FString::Printf(TEXT("%d\n"), FMath::Max(1,FMath::CeilToInt(Director->GetCountdownRemaining()))) : Director->GetRaceState() == ERaceState::Finished ? TEXT("FINISHED\n") : FString::Printf(TEXT("RACE %s\n"), *URaceDirectorSubsystem::FormatLapTime(Director->GetRaceElapsedTime()));
        for (const auto& C : Director->GetStandings()) { Rows += FString::Printf(TEXT("%s%d. %s  L%d/%d  %s\n"), C.bIsPlayer ? TEXT("> ") : TEXT(""), C.Position, *C.DisplayName, C.LapsCompleted, Director->GetLapCount(), *URaceDirectorSubsystem::FormatLapTime(C.BestLapTime)); }
        StandingsText = FText::FromString(Rows);
    }
}

TArray<FVector2D> UVehicleTelemetryPlot::GetPlotPoints(const FPaintContext& Context) const
{
    TArray<FVector2D> Points;
    const FVector2D PlotSize = Context.AllottedGeometry.GetLocalSize();
    if (!StaticPoints.IsEmpty())
    {
        for (const auto& P : StaticPoints) { Points.Add(P * PlotSize); }
        return Points;
    }
    const APawn* Pawn = GetOwningPlayerPawn();
    const auto* Telemetry = Pawn ? Pawn->FindComponentByClass<UVehicleTelemetryComponent>() : nullptr;
    if (!Telemetry) { return Points; }
    if (bTractionTrail)
    {
        const int32 Count = Telemetry->GetFrameCount();
        for (int32 i = FMath::Max(0, Count - 90); i < Count; ++i)
        {
            FVehicleTelemetryFrame Frame;
            if (!Telemetry->GetFrame(i, Frame)) { continue; }
            const FVector2D G(FMath::Clamp(Frame.AccelerationG.Y / 2.f, -1.f, 1.f), FMath::Clamp(Frame.AccelerationG.X / 2.f, -1.f, 1.f));
            Points.Add(FVector2D((G.X + 1.f) * .5f * PlotSize.X, (1.f - G.Y) * .5f * PlotSize.Y));
        }
        return Points;
    }
    auto SelectedChannel = Channel;
    const auto* Player = GetOwningPlayer();
    const auto* HUD = Player ? Cast<AVehicleTelemetryHUD>(Player->GetHUD()) : nullptr;
    if (HUD) {
        if (HUDGraphIndex == 0) SelectedChannel = HUD->PrimaryGraphChannel;
        if (HUDGraphIndex == 1) SelectedChannel = HUD->SecondaryGraphChannel;
        if (HUDGraphIndex == 2) SelectedChannel = EVehicleTelemetryChannel::YawRateError;
        if (HUDGraphIndex == 3) SelectedChannel = HUD->TertiaryGraphChannel;
    }
    TArray<FVector2D> Samples;
    Telemetry->GetChannelWindow(SelectedChannel, TraceWindowSeconds, Samples);
    if (Samples.IsEmpty()) { return Points; }
    const double EndTime = Telemetry->GetRecordingTime();
    const double StartTime = EndTime - TraceWindowSeconds;
    const FVector2D Range = Telemetry->GetChannelDisplayRange(SelectedChannel);
    const auto AddPoint = [&Points, &PlotSize, &Range, StartTime](double Time, double Value)
    {
        const double Fraction = FMath::Clamp((Value - Range.X) / (Range.Y - Range.X), 0.0, 1.0);
        Points.Add(FVector2D((Time - StartTime) / TraceWindowSeconds * PlotSize.X, (1.0 - Fraction) * PlotSize.Y));
    };
    for (int32 i = 0; i < Samples.Num(); ++i)
    {
        const FVector2D& Sample = Samples[i];
        if (Sample.X < StartTime) { continue; }
        if (i > 0 && Samples[i - 1].X < StartTime)
        {
            const FVector2D& Previous = Samples[i - 1];
            const double Alpha = (StartTime - Previous.X) / (Sample.X - Previous.X);
            AddPoint(StartTime, FMath::Lerp(Previous.Y, Sample.Y, Alpha));
        }
        AddPoint(Sample.X, Sample.Y);
    }
    if (Samples.Last().X < EndTime)
    {
        if (Points.IsEmpty()) { AddPoint(StartTime, Samples.Last().Y); }
        AddPoint(EndTime, Samples.Last().Y);
    }
    return Points;
}
