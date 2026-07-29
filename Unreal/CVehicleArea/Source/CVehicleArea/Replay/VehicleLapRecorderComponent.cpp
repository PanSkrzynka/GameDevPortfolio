#include "VehicleLapRecorderComponent.h"
#include "CVehicleArea.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "WheeledVehiclePawn.h"

UVehicleLapRecorderComponent::UVehicleLapRecorderComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UVehicleLapRecorderComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AWheeledVehiclePawn* VehiclePawn = Cast<AWheeledVehiclePawn>(GetOwner()))
	{
		MovementComponent = Cast<UChaosWheeledVehicleMovementComponent>(VehiclePawn->GetVehicleMovementComponent());
	}

	if (!MovementComponent)
	{
		UE_LOG(LogCVehicleArea, Warning, TEXT("LapRecorder: no vehicle movement on '%s', disabled."),
			*GetNameSafe(GetOwner()));
		SetComponentTickEnabled(false);
	}
}

void UVehicleLapRecorderComponent::BeginLap()
{
	CurrentLap = FVehicleLapRecording();
	CurrentLap.DriverLabel = DriverLabel;

	const int32 ReserveCount = FMath::Min(MaxSamplesPerLap, FMath::CeilToInt(SampleRateHz * 120.0f));
	CurrentLap.Samples.Reserve(ReserveCount);

	ElapsedTime = 0.0f;
	SampleTimer = 0.0f;
	bWarnedSampleLimit = false;
	bIsRecording = true;

	CaptureSample();
}

void UVehicleLapRecorderComponent::AbandonLap()
{
	bIsRecording = false;
	CurrentLap.Samples.Reset();
}

bool UVehicleLapRecorderComponent::CompleteLap(float LapTime)
{
	if (!bIsRecording)
	{
		return false;
	}

	bIsRecording = false;

	CurrentLap.LapTime = LapTime;
	CurrentLap.bComplete = true;

	if (!CurrentLap.IsValid())
	{
		UE_LOG(LogCVehicleArea, Warning, TEXT("LapRecorder: lap on '%s' had too few samples."), *GetNameSafe(GetOwner()));
		return false;
	}

	LastLap = CurrentLap;

	const bool bIsNewBest = !BestLap.IsValid() || LapTime < BestLap.LapTime;
	if (bIsNewBest)
	{
		BestLap = CurrentLap;
	}

	OnLapRecorded.Broadcast(LapTime, bIsNewBest);

	UE_LOG(LogCVehicleArea, Log, TEXT("Lap '%s': %.3f s, %d samples%s."),
		*DriverLabel, LapTime, CurrentLap.Samples.Num(), bIsNewBest ? TEXT(" (new best)") : TEXT(""));

	return bIsNewBest;
}

void UVehicleLapRecorderComponent::ClearBestLap()
{
	BestLap = FVehicleLapRecording();
}

void UVehicleLapRecorderComponent::CaptureSample()
{
	if (!MovementComponent || !MovementComponent->HasValidPhysicsState())
	{
		return;
	}

	if (CurrentLap.Samples.Num() >= MaxSamplesPerLap)
	{
		if (!bWarnedSampleLimit)
		{
			UE_LOG(LogCVehicleArea, Warning, TEXT("Lap '%s': hit the %d sample cap."),
				*DriverLabel, MaxSamplesPerLap);
			bWarnedSampleLimit = true;
		}
		return;
	}

	FVehicleReplaySample Sample;
	Sample.TimeSeconds = ElapsedTime;
	Sample.Snapshot = MovementComponent->GetSnapshot();
	Sample.Throttle = MovementComponent->GetThrottleInput();
	Sample.Brake = MovementComponent->GetBrakeInput();
	Sample.Steering = MovementComponent->GetSteeringInput();
	Sample.bHandbrake = MovementComponent->GetHandbrakeInput();

	CurrentLap.Samples.Add(MoveTemp(Sample));
}

void UVehicleLapRecorderComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsRecording || DeltaTime <= SMALL_NUMBER)
	{
		return;
	}

	ElapsedTime += DeltaTime;
	SampleTimer += DeltaTime;

	const float SampleInterval = 1.0f / FMath::Max(1.0f, SampleRateHz);
	if (SampleTimer < SampleInterval)
	{
		return;
	}

	SampleTimer = 0.0f;
	CaptureSample();
}
