#include "VehicleTelemetryComponent.h"
#include "CVehicleArea.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "ChaosVehicleWheel.h"
#include "ChaosVehicleManagerAsyncCallback.h"
#include "WheeledVehiclePawn.h"
#include "Components/SkeletalMeshComponent.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

namespace
{

	constexpr float GravityCmS2 = 980.665f;

	constexpr float CmsToKph = 0.036f;

	bool IsWheelOutputValid(UChaosWheeledVehicleMovementComponent* Movement, int32 WheelIndex)
	{
		return Movement
			&& Movement->HasValidPhysicsState()
			&& Movement->PhysicsVehicleOutput().IsValid()
			&& Movement->PhysicsVehicleOutput()->Wheels.IsValidIndex(WheelIndex);
	}
}

const FVehicleTelemetryFrame UVehicleTelemetryComponent::EmptyFrame = FVehicleTelemetryFrame();

UVehicleTelemetryComponent::UVehicleTelemetryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UVehicleTelemetryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AWheeledVehiclePawn* VehiclePawn = Cast<AWheeledVehiclePawn>(GetOwner()))
	{
		MovementComponent = Cast<UChaosWheeledVehicleMovementComponent>(VehiclePawn->GetVehicleMovementComponent());
		ChassisMesh = VehiclePawn->GetMesh();
	}

	if (!MovementComponent)
	{
		UE_LOG(LogCVehicleArea, Warning, TEXT("Telemetry: no vehicle movement on '%s', disabled."),
			*GetNameSafe(GetOwner()));
		SetComponentTickEnabled(false);
		return;
	}

	History.SetNum(FMath::Max(16, HistoryCapacity));
	Wheelbase = MeasureWheelbase();

	if (bRecordOnBeginPlay)
	{
		StartRecording();
	}
}

float UVehicleTelemetryComponent::MeasureWheelbase() const
{

	if (!ChassisMesh || !MovementComponent)
	{
		return DefaultWheelbase;
	}

	float MinX = TNumericLimits<float>::Max();
	float MaxX = TNumericLimits<float>::Lowest();
	int32 FoundBones = 0;

	for (const FChaosWheelSetup& WheelSetup : MovementComponent->WheelSetups)
	{
		const int32 BoneIndex = ChassisMesh->GetBoneIndex(WheelSetup.BoneName);
		if (BoneIndex == INDEX_NONE)
		{
			continue;
		}

		const FVector BoneLocation = ChassisMesh->GetBoneTransform(BoneIndex, FTransform::Identity).GetLocation();
		MinX = FMath::Min(MinX, BoneLocation.X);
		MaxX = FMath::Max(MaxX, BoneLocation.X);
		++FoundBones;
	}

	const float Measured = MaxX - MinX;
	if (FoundBones < 2 || Measured < 1.0f)
	{
		UE_LOG(LogCVehicleArea, Log, TEXT("Telemetry: no wheelbase on '%s', using %.1f cm."),
			*GetNameSafe(GetOwner()), DefaultWheelbase);
		return DefaultWheelbase;
	}

	return Measured;
}

void UVehicleTelemetryComponent::StartRecording()
{
	bIsRecording = true;
	ElapsedTime = 0.0f;
	SampleTimer = 0.0f;
	bHasPrevVelocity = false;
	FilteredAccelG = FVector::ZeroVector;
}

void UVehicleTelemetryComponent::StopRecording()
{
	bIsRecording = false;
}

void UVehicleTelemetryComponent::ClearHistory()
{
	WriteIndex = 0;
	FrameCount = 0;
}

void UVehicleTelemetryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsRecording || !MovementComponent || DeltaTime <= SMALL_NUMBER)
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

	const float CaptureDelta = SampleTimer;
	SampleTimer = 0.0f;

	CaptureFrame(CaptureDelta);
}

void UVehicleTelemetryComponent::CaptureFrame(float DeltaTime)
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	FVehicleTelemetryFrame Frame;
	Frame.TimeSeconds = ElapsedTime;

	const FTransform OwnerTransform = Owner->GetActorTransform();
	const FVector WorldVelocity = Owner->GetVelocity();
	const FVector LocalVelocity = OwnerTransform.InverseTransformVectorNoScale(WorldVelocity);

	Frame.SpeedKPH = WorldVelocity.Size() * CmsToKph;

	FVector RawAccelerationG = FVector::ZeroVector;
	if (bHasPrevVelocity)
	{
		const FVector WorldAcceleration = (WorldVelocity - PrevVelocity) / DeltaTime;
		RawAccelerationG = OwnerTransform.InverseTransformVectorNoScale(WorldAcceleration) / GravityCmS2;
	}
	PrevVelocity = WorldVelocity;
	bHasPrevVelocity = true;

	if (AccelSmoothTime > KINDA_SMALL_NUMBER)
	{
		const float Alpha = FMath::Clamp(DeltaTime / AccelSmoothTime, 0.0f, 1.0f);
		FilteredAccelG = FMath::Lerp(FilteredAccelG, RawAccelerationG, Alpha);
	}
	else
	{
		FilteredAccelG = RawAccelerationG;
	}
	Frame.AccelerationG = FilteredAccelG;

	if (ChassisMesh && ChassisMesh->IsSimulatingPhysics())
	{
		Frame.YawRate = ChassisMesh->GetPhysicsAngularVelocityInDegrees().Z;
	}

	if (LocalVelocity.Size2D() > 50.0f)
	{
		Frame.BodySlipAngle = FMath::RadiansToDegrees(FMath::Atan2(LocalVelocity.Y, LocalVelocity.X));
	}

	Frame.EngineRPM = MovementComponent->GetEngineRotationSpeed();
	Frame.Gear = MovementComponent->GetCurrentGear();
	Frame.ThrottleInput = MovementComponent->GetThrottleInput();
	Frame.BrakeInput = MovementComponent->GetBrakeInput();
	Frame.SteeringInput = MovementComponent->GetSteeringInput();
	Frame.bHandbrake = MovementComponent->GetHandbrakeInput();

	if (MovementComponent->HasValidPhysicsState() && MovementComponent->PhysicsVehicleOutput().IsValid())
	{
		Frame.EngineTorque = MovementComponent->PhysicsVehicleOutput()->EngineTorque;
		Frame.TransmissionRPM = MovementComponent->PhysicsVehicleOutput()->TransmissionRPM;
	}

	const int32 NumWheels = MovementComponent->GetNumWheels();
	Frame.Wheels.Reserve(NumWheels);

	float TotalSpringForce = 0.0f;
	float FrontSpringForce = 0.0f;
	float LeftSpringForce = 0.0f;
	float FrontSlipSum = 0.0f;
	float RearSlipSum = 0.0f;
	int32 FrontSlipCount = 0;
	int32 RearSlipCount = 0;
	float SteeredAngleSum = 0.0f;
	int32 SteeredCount = 0;

	const float ForwardSpeed = FMath::Abs(LocalVelocity.X);

	for (int32 WheelIndex = 0; WheelIndex < NumWheels; ++WheelIndex)
	{
		const FWheelStatus& Status = MovementComponent->GetWheelState(WheelIndex);

		FWheelTelemetrySample Sample;
		Sample.bInContact = Status.bInContact;
		Sample.NormalizedSuspensionLength = Status.NormalizedSuspensionLength;
		Sample.SpringForce = Status.SpringForce;
		Sample.SlipAngle = Status.SlipAngle;
		Sample.SlipMagnitude = Status.SlipMagnitude;
		Sample.SkidMagnitude = Status.SkidMagnitude;
		Sample.bIsSlipping = Status.bIsSlipping;
		Sample.bIsSkidding = Status.bIsSkidding;
		Sample.bABSActivated = Status.bABSActivated;
		Sample.DriveTorque = Status.DriveTorque;
		Sample.BrakeTorque = Status.BrakeTorque;

		if (Status.PhysMaterial.IsValid())
		{
			Sample.SurfaceName = Status.PhysMaterial->GetFName();
		}

		UChaosVehicleWheel* Wheel = MovementComponent->Wheels.IsValidIndex(WheelIndex) ? MovementComponent->Wheels[WheelIndex] : nullptr;
		EAxleType AxleType = EAxleType::Undefined;

		if (Wheel && IsWheelOutputValid(MovementComponent, WheelIndex))
		{
			AxleType = Wheel->GetAxleType();

			const float AngularVelocityRadS = Wheel->GetWheelAngularVelocity();
			const float WheelRadius = Wheel->GetWheelRadius();

			Sample.AngularVelocity = FMath::RadiansToDegrees(AngularVelocityRadS);
			Sample.SteerAngle = Wheel->GetSteerAngle();
			Sample.SuspensionOffset = Wheel->GetSuspensionOffset();

			const float WheelSurfaceSpeed = AngularVelocityRadS * WheelRadius;
			const float Reference = FMath::Max(ForwardSpeed, 100.0f);
			Sample.LongitudinalSlipRatio = (WheelSurfaceSpeed - ForwardSpeed) / Reference;

			if (Wheel->bAffectedBySteering)
			{
				SteeredAngleSum += Sample.SteerAngle;
				++SteeredCount;
			}
		}

		if (Status.bInContact)
		{
			++Frame.WheelsInContact;
			TotalSpringForce += Status.SpringForce;

			if (AxleType == EAxleType::Front)
			{
				FrontSpringForce += Status.SpringForce;
				FrontSlipSum += FMath::Abs(Status.SlipAngle);
				++FrontSlipCount;
			}
			else if (AxleType == EAxleType::Rear)
			{
				RearSlipSum += FMath::Abs(Status.SlipAngle);
				++RearSlipCount;
			}

			if (Wheel && MovementComponent->WheelSetups.IsValidIndex(WheelIndex) && ChassisMesh)
			{
				const int32 BoneIndex = ChassisMesh->GetBoneIndex(MovementComponent->WheelSetups[WheelIndex].BoneName);
				if (BoneIndex != INDEX_NONE)
				{
					const float BoneY = ChassisMesh->GetBoneTransform(BoneIndex, FTransform::Identity).GetLocation().Y;
					if (BoneY < 0.0f)
					{
						LeftSpringForce += Status.SpringForce;
					}
				}
			}
		}

		Frame.Wheels.Add(MoveTemp(Sample));
	}

	if (TotalSpringForce > KINDA_SMALL_NUMBER)
	{
		for (FWheelTelemetrySample& Sample : Frame.Wheels)
		{
			Sample.NormalizedLoad = Sample.SpringForce / TotalSpringForce;
		}
		Frame.FrontAxleLoadFraction = FrontSpringForce / TotalSpringForce;
		Frame.LeftSideLoadFraction = LeftSpringForce / TotalSpringForce;
	}

	if (FrontSlipCount > 0 && RearSlipCount > 0)
	{
		Frame.UndersteerBalance = (FrontSlipSum / FrontSlipCount) - (RearSlipSum / RearSlipCount);
	}

	if (SteeredCount > 0 && Wheelbase > KINDA_SMALL_NUMBER)
	{
		const float MeanSteerRad = FMath::DegreesToRadians(SteeredAngleSum / SteeredCount);
		const float YawRateRadS = (LocalVelocity.X / Wheelbase) * FMath::Tan(MeanSteerRad);
		Frame.TargetYawRate = FMath::RadiansToDegrees(YawRateRadS);
	}

	PushFrame(MoveTemp(Frame));
}

void UVehicleTelemetryComponent::PushFrame(FVehicleTelemetryFrame&& Frame)
{
	if (History.Num() == 0)
	{
		return;
	}

	History[WriteIndex] = MoveTemp(Frame);
	WriteIndex = (WriteIndex + 1) % History.Num();
	FrameCount = FMath::Min(FrameCount + 1, History.Num());
}

const FVehicleTelemetryFrame& UVehicleTelemetryComponent::GetLatestFrame() const
{
	if (FrameCount == 0 || History.Num() == 0)
	{
		return EmptyFrame;
	}

	const int32 LatestIndex = (WriteIndex - 1 + History.Num()) % History.Num();
	return History[LatestIndex];
}

bool UVehicleTelemetryComponent::GetFrame(int32 IndexFromOldest, FVehicleTelemetryFrame& OutFrame) const
{
	if (IndexFromOldest < 0 || IndexFromOldest >= FrameCount || History.Num() == 0)
	{
		return false;
	}

	const int32 OldestIndex = (WriteIndex - FrameCount + History.Num()) % History.Num();
	OutFrame = History[(OldestIndex + IndexFromOldest) % History.Num()];
	return true;
}

float UVehicleTelemetryComponent::ExtractChannel(const FVehicleTelemetryFrame& Frame, EVehicleTelemetryChannel Channel)
{
	switch (Channel)
	{
	case EVehicleTelemetryChannel::Speed:				return Frame.SpeedKPH;
	case EVehicleTelemetryChannel::EngineRPM:			return Frame.EngineRPM;
	case EVehicleTelemetryChannel::Throttle:			return Frame.ThrottleInput;
	case EVehicleTelemetryChannel::Brake:				return Frame.BrakeInput;
	case EVehicleTelemetryChannel::Steering:			return Frame.SteeringInput;
	case EVehicleTelemetryChannel::LongitudinalG:		return Frame.AccelerationG.X;
	case EVehicleTelemetryChannel::LateralG:			return Frame.AccelerationG.Y;
	case EVehicleTelemetryChannel::CombinedG:			return Frame.GetCombinedG();
	case EVehicleTelemetryChannel::YawRate:				return Frame.YawRate;
	case EVehicleTelemetryChannel::YawRateError:		return Frame.YawRate - Frame.TargetYawRate;
	case EVehicleTelemetryChannel::BodySlipAngle:		return Frame.BodySlipAngle;
	case EVehicleTelemetryChannel::UndersteerBalance:	return Frame.UndersteerBalance;
	case EVehicleTelemetryChannel::FrontAxleLoad:		return Frame.FrontAxleLoadFraction;
	default:											return 0.0f;
	}
}

void UVehicleTelemetryComponent::GetChannelHistory(EVehicleTelemetryChannel Channel, int32 MaxSamples, TArray<float>& OutValues) const
{
	OutValues.Reset();

	if (FrameCount == 0)
	{
		return;
	}

	const int32 Wanted = (MaxSamples > 0) ? FMath::Min(MaxSamples, FrameCount) : FrameCount;
	OutValues.Reserve(Wanted);

	for (int32 Index = 0; Index < Wanted; ++Index)
	{
		const int32 SourceIndex = (Wanted == 1)
			? FrameCount - 1
			: FMath::RoundToInt(static_cast<float>(Index) * (FrameCount - 1) / (Wanted - 1));

		FVehicleTelemetryFrame Frame;
		if (GetFrame(SourceIndex, Frame))
		{
			OutValues.Add(ExtractChannel(Frame, Channel));
		}
	}
}

FVehicleTelemetrySummary UVehicleTelemetryComponent::BuildSummary() const
{
	FVehicleTelemetrySummary Summary;

	if (FrameCount == 0)
	{
		return Summary;
	}

	double BalanceAccumulator = 0.0;
	int32 SlippingFrames = 0;
	int32 AirborneFrames = 0;
	float FirstTime = 0.0f;
	float LastTime = 0.0f;

	for (int32 Index = 0; Index < FrameCount; ++Index)
	{
		FVehicleTelemetryFrame Frame;
		if (!GetFrame(Index, Frame))
		{
			continue;
		}

		if (Index == 0)
		{
			FirstTime = Frame.TimeSeconds;
		}
		LastTime = Frame.TimeSeconds;

		Summary.PeakSpeedKPH = FMath::Max(Summary.PeakSpeedKPH, Frame.SpeedKPH);
		Summary.PeakEngineRPM = FMath::Max(Summary.PeakEngineRPM, Frame.EngineRPM);
		Summary.PeakLongitudinalG = FMath::Max(Summary.PeakLongitudinalG, Frame.AccelerationG.X);
		Summary.PeakBrakingG = FMath::Max(Summary.PeakBrakingG, -Frame.AccelerationG.X);
		Summary.PeakLateralG = FMath::Max(Summary.PeakLateralG, FMath::Abs(Frame.AccelerationG.Y));
		Summary.PeakCombinedG = FMath::Max(Summary.PeakCombinedG, Frame.GetCombinedG());
		Summary.PeakBodySlipAngle = FMath::Max(Summary.PeakBodySlipAngle, FMath::Abs(Frame.BodySlipAngle));

		BalanceAccumulator += Frame.UndersteerBalance;

		bool bAnySlipping = false;
		for (const FWheelTelemetrySample& Wheel : Frame.Wheels)
		{
			bAnySlipping |= Wheel.bIsSlipping;
		}
		SlippingFrames += bAnySlipping ? 1 : 0;
		AirborneFrames += (Frame.WheelsInContact < Frame.Wheels.Num()) ? 1 : 0;

		++Summary.SampleCount;
	}

	if (Summary.SampleCount > 0)
	{
		Summary.MeanUndersteerBalance = static_cast<float>(BalanceAccumulator / Summary.SampleCount);
		Summary.SlipTimeFraction = static_cast<float>(SlippingFrames) / Summary.SampleCount;
		Summary.AirborneTimeFraction = static_cast<float>(AirborneFrames) / Summary.SampleCount;
		Summary.DurationSeconds = LastTime - FirstTime;
	}

	return Summary;
}

bool UVehicleTelemetryComponent::ExportToCSV(const FString& Label, FString& OutFilePath) const
{
	if (FrameCount == 0)
	{
		UE_LOG(LogCVehicleArea, Warning, TEXT("Telemetry: nothing to export."));
		return false;
	}

	int32 MaxWheels = 0;
	for (int32 Index = 0; Index < FrameCount; ++Index)
	{
		FVehicleTelemetryFrame Frame;
		if (GetFrame(Index, Frame))
		{
			MaxWheels = FMath::Max(MaxWheels, Frame.Wheels.Num());
		}
	}

	TArray<FString> Lines;
	Lines.Reserve(FrameCount + 1);

	FString Header = TEXT("Time,SpeedKPH,EngineRPM,EngineTorque,TransmissionRPM,Gear,Throttle,Brake,Steering,Handbrake,")
		TEXT("LongG,LatG,VertG,CombinedG,YawRate,TargetYawRate,YawRateError,BodySlipAngle,UndersteerBalance,")
		TEXT("FrontAxleLoad,LeftSideLoad,WheelsInContact");

	for (int32 WheelIndex = 0; WheelIndex < MaxWheels; ++WheelIndex)
	{
		Header += FString::Printf(
			TEXT(",W%d_InContact,W%d_SuspLength,W%d_SuspOffset,W%d_SpringForce,W%d_Load,W%d_SlipAngle,")
			TEXT("W%d_SlipRatio,W%d_SlipMag,W%d_SkidMag,W%d_Slipping,W%d_Skidding,W%d_ABS,")
			TEXT("W%d_DriveTorque,W%d_BrakeTorque,W%d_AngVel,W%d_SteerAngle,W%d_Surface"),
			WheelIndex, WheelIndex, WheelIndex, WheelIndex, WheelIndex, WheelIndex,
			WheelIndex, WheelIndex, WheelIndex, WheelIndex, WheelIndex, WheelIndex,
			WheelIndex, WheelIndex, WheelIndex, WheelIndex, WheelIndex);
	}
	Lines.Add(MoveTemp(Header));

	for (int32 Index = 0; Index < FrameCount; ++Index)
	{
		FVehicleTelemetryFrame Frame;
		if (!GetFrame(Index, Frame))
		{
			continue;
		}

		FString Row = FString::Printf(
			TEXT("%.4f,%.3f,%.1f,%.2f,%.1f,%d,%.3f,%.3f,%.3f,%d,%.4f,%.4f,%.4f,%.4f,%.3f,%.3f,%.3f,%.3f,%.4f,%.4f,%.4f,%d"),
			Frame.TimeSeconds, Frame.SpeedKPH, Frame.EngineRPM, Frame.EngineTorque, Frame.TransmissionRPM,
			Frame.Gear, Frame.ThrottleInput, Frame.BrakeInput, Frame.SteeringInput, Frame.bHandbrake ? 1 : 0,
			Frame.AccelerationG.X, Frame.AccelerationG.Y, Frame.AccelerationG.Z, Frame.GetCombinedG(),
			Frame.YawRate, Frame.TargetYawRate, Frame.YawRate - Frame.TargetYawRate,
			Frame.BodySlipAngle, Frame.UndersteerBalance,
			Frame.FrontAxleLoadFraction, Frame.LeftSideLoadFraction, Frame.WheelsInContact);

		for (int32 WheelIndex = 0; WheelIndex < MaxWheels; ++WheelIndex)
		{
			if (Frame.Wheels.IsValidIndex(WheelIndex))
			{
				const FWheelTelemetrySample& Wheel = Frame.Wheels[WheelIndex];
				Row += FString::Printf(
					TEXT(",%d,%.4f,%.3f,%.1f,%.4f,%.3f,%.4f,%.3f,%.3f,%d,%d,%d,%.1f,%.1f,%.1f,%.3f,%s"),
					Wheel.bInContact ? 1 : 0, Wheel.NormalizedSuspensionLength, Wheel.SuspensionOffset,
					Wheel.SpringForce, Wheel.NormalizedLoad, Wheel.SlipAngle, Wheel.LongitudinalSlipRatio,
					Wheel.SlipMagnitude, Wheel.SkidMagnitude,
					Wheel.bIsSlipping ? 1 : 0, Wheel.bIsSkidding ? 1 : 0, Wheel.bABSActivated ? 1 : 0,
					Wheel.DriveTorque, Wheel.BrakeTorque, Wheel.AngularVelocity, Wheel.SteerAngle,
					*Wheel.SurfaceName.ToString());
			}
			else
			{
				Row += TEXT(",,,,,,,,,,,,,,,,,");
			}
		}

		Lines.Add(MoveTemp(Row));
	}

	const FString SafeLabel = Label.IsEmpty() ? TEXT("Session") : FPaths::MakeValidFileName(Label);
	const FString FileName = FString::Printf(TEXT("%s_%s.csv"), *SafeLabel, *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
	const FString FullPath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Telemetry"), FileName);

	if (!FFileHelper::SaveStringArrayToFile(Lines, *FullPath))
	{
		UE_LOG(LogCVehicleArea, Error, TEXT("VehicleTelemetry: failed to write '%s'."), *FullPath);
		return false;
	}

	OutFilePath = FullPath;
	UE_LOG(LogCVehicleArea, Log, TEXT("VehicleTelemetry: wrote %d samples to '%s'."), FrameCount, *FullPath);
	return true;
}
