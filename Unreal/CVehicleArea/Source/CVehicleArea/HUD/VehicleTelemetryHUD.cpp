#include "VehicleTelemetryHUD.h"
#include "Dynamics/VehicleDriverAssistComponent.h"
#include "Dynamics/VehicleSurfaceResponseComponent.h"
#include "Replay/VehicleLapRecorderComponent.h"
#include "Race/RaceDirectorSubsystem.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "ChaosVehicleWheel.h"
#include "WheeledVehiclePawn.h"
#include "Blueprint/UserWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#if ENABLE_DRAW_DEBUG
#include "DrawDebugHelpers.h"
#endif


AVehicleTelemetryHUD::AVehicleTelemetryHUD()
{
    PrimaryActorTick.bCanEverTick = false;
    static ConstructorHelpers::FClassFinder<UUserWidget> Widget(TEXT("/Game/VehicleTemplate/Blueprints/UI/WBP_VehicleTelemetry"));
    TelemetryWidgetClass = Widget.Class;
}

void AVehicleTelemetryHUD::BeginPlay()
{
    Super::BeginPlay();
    APlayerController* Player = GetOwningPlayerController();
    if (Player && Player->IsLocalController() && TelemetryWidgetClass)
    {
        TelemetryWidget = CreateWidget<UUserWidget>(Player, TelemetryWidgetClass);
        if (TelemetryWidget) { TelemetryWidget->AddToPlayerScreen(10); }
    }
}

void AVehicleTelemetryHUD::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (TelemetryWidget) { TelemetryWidget->RemoveFromParent(); TelemetryWidget = nullptr; }
    Super::EndPlay(EndPlayReason);
}

void AVehicleTelemetryHUD::CyclePage()
{
    CurrentPage = static_cast<EVehicleHUDPage>((static_cast<uint8>(CurrentPage) + 1) % 5);
}

void AVehicleTelemetryHUD::SetPage(EVehicleHUDPage NewPage) { CurrentPage = NewPage; }

void AVehicleTelemetryHUD::DrawHUD()
{
    Super::DrawHUD();
    if (TelemetryWidget)
    {
        TelemetryWidget->SetVisibility(bShowHUD ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
    }
    if (bShowHUD && ResolveVehicle()) { DrawWorldDebug(); }
}

bool AVehicleTelemetryHUD::ResolveVehicle()
{
	APawn* CurrentPawn = GetOwningPawn();

	if (CurrentPawn != VehiclePawn)
	{
		VehiclePawn = CurrentPawn;
		MovementComponent = nullptr;
		Telemetry = nullptr;

		if (AWheeledVehiclePawn* Vehicle = Cast<AWheeledVehiclePawn>(CurrentPawn))
		{
			MovementComponent = Cast<UChaosWheeledVehicleMovementComponent>(Vehicle->GetVehicleMovementComponent());
			Telemetry = Vehicle->FindComponentByClass<UVehicleTelemetryComponent>();
		}
	}

	return MovementComponent != nullptr && Telemetry != nullptr;
}

FLinearColor AVehicleTelemetryHUD::GetHeatColor(float Value)
{
	const float Clamped = FMath::Clamp(Value, 0.0f, 1.0f);

	if (Clamped < 0.5f)
	{
		return FMath::Lerp(FLinearColor(0.3f, 0.9f, 0.4f), FLinearColor(1.0f, 0.65f, 0.1f), Clamped * 2.0f);
	}

	return FMath::Lerp(FLinearColor(1.0f, 0.65f, 0.1f), FLinearColor(1.0f, 0.25f, 0.2f), (Clamped - 0.5f) * 2.0f);
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
