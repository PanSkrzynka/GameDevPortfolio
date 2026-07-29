#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VehicleTelemetryTypes.h"
#include "VehicleTelemetryComponent.generated.h"

class UChaosWheeledVehicleMovementComponent;
class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class EVehicleTelemetryChannel : uint8
{
	Speed,
	EngineRPM,
	Throttle,
	Brake,
	Steering,
	LongitudinalG,
	LateralG,
	CombinedG,
	YawRate,
	YawRateError,
	BodySlipAngle,
	UndersteerBalance,
	FrontAxleLoad
};

UCLASS(ClassGroup = (Vehicle), meta = (BlueprintSpawnableComponent))
class CVEHICLEAREA_API UVehicleTelemetryComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UVehicleTelemetryComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry", meta = (ClampMin = "1.0", ClampMax = "240.0"))
	float SampleRateHz = 60.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry", meta = (ClampMin = "16"))
	int32 HistoryCapacity = 3600;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry")
	bool bRecordOnBeginPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry", meta = (ClampMin = "0.0", ClampMax = "1.0", Units = "s"))
	float AccelSmoothTime = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telemetry", meta = (ClampMin = "1.0", Units = "cm"))
	float DefaultWheelbase = 280.0f;

	UFUNCTION(BlueprintCallable, Category = "Telemetry")
	void StartRecording();

	UFUNCTION(BlueprintCallable, Category = "Telemetry")
	void StopRecording();

	UFUNCTION(BlueprintCallable, Category = "Telemetry")
	void ClearHistory();

	UFUNCTION(BlueprintPure, Category = "Telemetry")
	bool IsRecording() const { return bIsRecording; }

	UFUNCTION(BlueprintPure, Category = "Telemetry")
	const FVehicleTelemetryFrame& GetLatestFrame() const;

	UFUNCTION(BlueprintPure, Category = "Telemetry")
	int32 GetFrameCount() const { return FrameCount; }

	UFUNCTION(BlueprintCallable, Category = "Telemetry")
	bool GetFrame(int32 IndexFromOldest, FVehicleTelemetryFrame& OutFrame) const;

	UFUNCTION(BlueprintCallable, Category = "Telemetry")
	void GetChannelHistory(EVehicleTelemetryChannel Channel, int32 MaxSamples, TArray<float>& OutValues) const;

	UFUNCTION(BlueprintCallable, Category = "Telemetry")
	FVehicleTelemetrySummary BuildSummary() const;

	UFUNCTION(BlueprintCallable, Category = "Telemetry")
	bool ExportToCSV(const FString& Label, FString& OutFilePath) const;

	UFUNCTION(BlueprintPure, Category = "Telemetry")
	UChaosWheeledVehicleMovementComponent* GetMovementComponent() const { return MovementComponent; }

	UFUNCTION(BlueprintPure, Category = "Telemetry")
	float GetWheelbase() const { return Wheelbase; }

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:

	void CaptureFrame(float DeltaTime);

	float MeasureWheelbase() const;

	static float ExtractChannel(const FVehicleTelemetryFrame& Frame, EVehicleTelemetryChannel Channel);

	void PushFrame(FVehicleTelemetryFrame&& Frame);

private:

	UPROPERTY(Transient)
	TObjectPtr<UChaosWheeledVehicleMovementComponent> MovementComponent;

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> ChassisMesh;

	UPROPERTY(Transient)
	TArray<FVehicleTelemetryFrame> History;

	int32 WriteIndex = 0;

	int32 FrameCount = 0;

	bool bIsRecording = false;

	float ElapsedTime = 0.0f;

	float SampleTimer = 0.0f;

	FVector PrevVelocity = FVector::ZeroVector;

	bool bHasPrevVelocity = false;

	FVector FilteredAccelG = FVector::ZeroVector;

	float Wheelbase = 280.0f;

	static const FVehicleTelemetryFrame EmptyFrame;
};
