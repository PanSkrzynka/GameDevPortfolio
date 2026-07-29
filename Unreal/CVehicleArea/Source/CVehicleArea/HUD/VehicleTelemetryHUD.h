#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Telemetry/VehicleTelemetryComponent.h"
#include "VehicleTelemetryHUD.generated.h"

class UVehicleDriverAssistComponent;
class UVehicleSurfaceResponseComponent;
class UVehicleLapRecorderComponent;
class UChaosWheeledVehicleMovementComponent;
class URaceDirectorSubsystem;

UENUM(BlueprintType)
enum class EVehicleHUDPage : uint8
{
	Off,
	Core,
	Wheels,
	Balance,
	Graphs
};

UCLASS()
class CVEHICLEAREA_API AVehicleTelemetryHUD : public AHUD
{
	GENERATED_BODY()

public:

	AVehicleTelemetryHUD();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry HUD")
	EVehicleHUDPage CurrentPage = EVehicleHUDPage::Core;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry HUD|World")
	bool bDrawRacingLine = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry HUD|World")
	bool bDrawSuspensionLoad = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry HUD|World")
	bool bDrawSlipVectors = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry HUD")
	bool bDrawStandings = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry HUD|Graphs")
	EVehicleTelemetryChannel PrimaryGraphChannel = EVehicleTelemetryChannel::Speed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry HUD|Graphs")
	EVehicleTelemetryChannel SecondaryGraphChannel = EVehicleTelemetryChannel::LateralG;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry HUD|Graphs")
	EVehicleTelemetryChannel TertiaryGraphChannel = EVehicleTelemetryChannel::UndersteerBalance;

	UFUNCTION(BlueprintCallable, Category = "Telemetry HUD")
	void CyclePage();

	UFUNCTION(BlueprintCallable, Category = "Telemetry HUD")
	void SetPage(EVehicleHUDPage NewPage);

	virtual void DrawHUD() override;

protected:

	bool ResolveVehicle();

	void DrawCorePage();
	void DrawWheelsPage();
	void DrawBalancePage();
	void DrawGraphsPage();
	void DrawStandingsPanel();
	void DrawWorldDebug();

	void DrawTachometer(float CenterX, float CenterY, float Radius, const FVehicleTelemetryFrame& Frame);
	void DrawTractionCircle(float CenterX, float CenterY, float Radius, const FVehicleTelemetryFrame& Frame);
	void DrawCarPlanView(float X, float Y, float Width, float Height, const FVehicleTelemetryFrame& Frame);
	void DrawInputStrip(float X, float Y, float Width, float Height);
	void DrawAssistLights(float X, float Y, float Width);
	void DrawLapBlock(float X, float Y, float Width);

	void DrawBar(float X, float Y, float Width, float Height, float Fraction, const FLinearColor& FillColor, const FString& Label, bool bBidirectional = false);
	void DrawPanel(float X, float Y, float Width, float Height);
	void DrawTrace(float X, float Y, float Width, float Height, const TArray<float>& Values, const FLinearColor& Color, const FString& Label);
	void DrawLabel(const FString& Text, float X, float Y, const FLinearColor& Color, float Scale = 1.0f);
	void DrawArc(float CenterX, float CenterY, float Radius, float StartDegrees, float EndDegrees, const FLinearColor& Color, float Thickness, int32 Segments = 48);
	void DrawRing(float CenterX, float CenterY, float Radius, const FLinearColor& Color, float Thickness, int32 Segments = 48);
	void DrawIndicator(float X, float Y, float Size, bool bLit, const FLinearColor& LitColor, const FString& Label);

	static FLinearColor GetHeatColor(float Value);
	static FString GetGearText(int32 Gear);
	static FString FormatTime(float Seconds);

private:

	UPROPERTY(Transient)
	TObjectPtr<APawn> VehiclePawn;

	UPROPERTY(Transient)
	TObjectPtr<UChaosWheeledVehicleMovementComponent> MovementComponent;

	UPROPERTY(Transient)
	TObjectPtr<UVehicleTelemetryComponent> Telemetry;

	UPROPERTY(Transient)
	TObjectPtr<UVehicleDriverAssistComponent> DriverAssists;

	UPROPERTY(Transient)
	TObjectPtr<UVehicleSurfaceResponseComponent> SurfaceResponse;

	UPROPERTY(Transient)
	TObjectPtr<UVehicleLapRecorderComponent> LapRecorder;

	UPROPERTY(Transient)
	TObjectPtr<UFont> HUDFont;

	float UIScale = 1.0f;

	TArray<FVector2D> GForceTrail;

	float PeakCombinedG = 0.0f;
};
