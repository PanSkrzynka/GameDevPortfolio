#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Telemetry/VehicleTelemetryComponent.h"
#include "VehicleTelemetryWidget.generated.h"

UCLASS(Abstract)
class CVEHICLEAREA_API UVehicleTelemetryWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    FText SpeedText;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    FText GearText;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    FText RPMText;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    FText LapText;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    FText AssistsText;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    FText DynamicsText;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    FText SurfaceText;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    FText BalanceText;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    FText StandingsText;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    FText GraphSummaryText;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    FText Graph0Text;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    FText Graph1Text;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    FText Graph2Text;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    FText Graph3Text;

    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    FText Wheel0Text;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    FText Wheel1Text;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    FText Wheel2Text;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    FText Wheel3Text;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    float RPMFraction = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    float Throttle = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    float Brake = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    float Steering = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    float FrontLoad = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    float LeftLoad = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    float Wheel0Load = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    float Wheel0Slip = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    float Wheel0Angle = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    float Wheel1Load = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    float Wheel1Slip = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    float Wheel1Angle = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    float Wheel2Load = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    float Wheel2Slip = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    float Wheel2Angle = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    float Wheel3Load = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    float Wheel3Slip = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    float Wheel3Angle = 0.0f;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    ESlateVisibility CoreVisibility = ESlateVisibility::Collapsed;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    ESlateVisibility WheelsVisibility = ESlateVisibility::Collapsed;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    ESlateVisibility BalanceVisibility = ESlateVisibility::Collapsed;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    ESlateVisibility GraphsVisibility = ESlateVisibility::Collapsed;
    UPROPERTY(BlueprintReadOnly, Category = "Telemetry|Presentation")
    ESlateVisibility StandingsVisibility = ESlateVisibility::Collapsed;

private:
    TWeakObjectPtr<APawn> LastPawn;
    float PeakCombinedG = 0.f;
protected:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
};

UCLASS(Abstract)
class CVEHICLEAREA_API UVehicleTelemetryPlot : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
    EVehicleTelemetryChannel Channel = EVehicleTelemetryChannel::Speed;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
    int32 HUDGraphIndex = -1;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
    FLinearColor TraceColor = FLinearColor(0.2f, 0.75f, 1.0f);
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
    TArray<FVector2D> StaticPoints;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
    bool bTractionTrail = false;
    UFUNCTION(BlueprintPure, Category = "Telemetry")
    TArray<FVector2D> GetPlotPoints(const FPaintContext& Context) const;
};
