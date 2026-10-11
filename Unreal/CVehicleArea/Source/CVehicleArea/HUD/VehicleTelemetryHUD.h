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

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void DrawHUD() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Telemetry HUD")
	TSubclassOf<class UUserWidget> TelemetryWidgetClass;

protected:
	bool ResolveVehicle();
	void DrawWorldDebug();
	static FLinearColor GetHeatColor(float Value);

private:
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> TelemetryWidget;
	UPROPERTY(Transient)
	TObjectPtr<APawn> VehiclePawn;
	UPROPERTY(Transient)
	TObjectPtr<UChaosWheeledVehicleMovementComponent> MovementComponent;
	UPROPERTY(Transient)
	TObjectPtr<UVehicleTelemetryComponent> Telemetry;
};
