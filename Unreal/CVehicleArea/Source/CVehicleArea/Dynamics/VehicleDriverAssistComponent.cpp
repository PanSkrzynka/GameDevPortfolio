#include "VehicleDriverAssistComponent.h"
#include "CVehicleArea.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "ChaosVehicleManagerAsyncCallback.h"
#include "WheeledVehiclePawn.h"
#include "Components/SkeletalMeshComponent.h"

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

UVehicleDriverAssistComponent::UVehicleDriverAssistComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UVehicleDriverAssistComponent::BeginPlay()
{
	Super::BeginPlay();

	if (AWheeledVehiclePawn* VehiclePawn = Cast<AWheeledVehiclePawn>(GetOwner()))
	{
		MovementComponent = Cast<UChaosWheeledVehicleMovementComponent>(VehiclePawn->GetVehicleMovementComponent());
		ChassisMesh = VehiclePawn->GetMesh();
	}

	if (!MovementComponent)
	{
		UE_LOG(LogCVehicleArea, Warning, TEXT("DriverAssist: no vehicle movement on '%s', disabled."),
			*GetNameSafe(GetOwner()));
		SetComponentTickEnabled(false);
		return;
	}

	BaseDownforce = MovementComponent->DownforceCoefficient;
	bHasBaseDownforce = true;

	CacheWheelInfo();
}

void UVehicleDriverAssistComponent::CacheWheelInfo()
{
	WheelInfos.Reset();

	const int32 NumWheels = MovementComponent->WheelSetups.Num();
	WheelInfos.Reserve(NumWheels);

	float MinX = TNumericLimits<float>::Max();
	float MaxX = TNumericLimits<float>::Lowest();
	int32 MeasuredBones = 0;

	const EVehicleDifferential Differential = MovementComponent->DifferentialSetup.DifferentialType;

	for (int32 WheelIndex = 0; WheelIndex < NumWheels; ++WheelIndex)
	{
		FWheelInfo Info;
		Info.Index = WheelIndex;

		const FChaosWheelSetup& Setup = MovementComponent->WheelSetups[WheelIndex];
		if (Setup.WheelClass)
		{
			if (UChaosVehicleWheel* Defaults = Setup.WheelClass.GetDefaultObject())
			{
				Info.Axle = Defaults->GetAxleType();
			}
		}

		if (ChassisMesh)
		{
			const int32 BoneIndex = ChassisMesh->GetBoneIndex(Setup.BoneName);
			if (BoneIndex != INDEX_NONE)
			{
				const FVector BoneLocation = ChassisMesh->GetBoneTransform(BoneIndex, FTransform::Identity).GetLocation();
				Info.bIsLeft = BoneLocation.Y < 0.0f;
				MinX = FMath::Min(MinX, BoneLocation.X);
				MaxX = FMath::Max(MaxX, BoneLocation.X);
				++MeasuredBones;
			}
		}

		switch (Info.Axle)
		{
		case EAxleType::Front:
			Info.bIsDriven = (Differential == EVehicleDifferential::AllWheelDrive || Differential == EVehicleDifferential::FrontWheelDrive);
			break;
		case EAxleType::Rear:
			Info.bIsDriven = (Differential == EVehicleDifferential::AllWheelDrive || Differential == EVehicleDifferential::RearWheelDrive);
			break;
		default:
			Info.bIsDriven = false;
			break;
		}

		WheelInfos.Add(Info);
	}

	if (MeasuredBones >= 2 && (MaxX - MinX) > 1.0f)
	{
		Wheelbase = MaxX - MinX;
	}

	SlipIntegrators.SetNumZeroed(NumWheels);
	PendingDriveTorque.SetNumZeroed(NumWheels);
	PendingBrakeTorque.SetNumZeroed(NumWheels);
	PrevDriveTorque.SetNumZeroed(NumWheels);
	PrevBrakeTorque.SetNumZeroed(NumWheels);
	AssistState.Wheels.SetNum(NumWheels);

	SetupTorqueCombine();
}

void UVehicleDriverAssistComponent::SetupTorqueCombine()
{

	for (const FWheelInfo& Info : WheelInfos)
	{
		MovementComponent->SetTorqueCombineMethod(ETorqueCombineMethod::Additive, Info.Index);
	}
}

void UVehicleDriverAssistComponent::SetAssistsEnabled(bool bEnabled)
{
	bAssistsEnabled = bEnabled;

	if (!bEnabled)
	{

		for (int32 Index = 0; Index < PendingDriveTorque.Num(); ++Index)
		{
			PendingDriveTorque[Index] = 0.0f;
			PendingBrakeTorque[Index] = 0.0f;
			SlipIntegrators[Index] = 0.0f;

			if (MovementComponent && MovementComponent->HasValidPhysicsState())
			{
				MovementComponent->SetDriveTorque(0.0f, Index);
				MovementComponent->SetBrakeTorque(0.0f, Index);
			}
		}

		AssistState = FVehicleAssistState();
		AssistState.Wheels.SetNum(WheelInfos.Num());

		if (MovementComponent && bHasBaseDownforce && MovementComponent->HasValidPhysicsState())
		{
			MovementComponent->SetDownforceCoefficient(BaseDownforce);
		}
	}
}

void UVehicleDriverAssistComponent::ArmLaunchControl()
{
	bLaunchControlArmed = bLaunchControlEnabled;
}

float UVehicleDriverAssistComponent::ComputeSlipRatio(int32 WheelIndex, float ForwardSpeed) const
{
	if (!IsWheelOutputValid(MovementComponent, WheelIndex) || !MovementComponent->Wheels.IsValidIndex(WheelIndex))
	{
		return 0.0f;
	}

	UChaosVehicleWheel* Wheel = MovementComponent->Wheels[WheelIndex];
	if (!Wheel)
	{
		return 0.0f;
	}

	const float WheelSurfaceSpeed = Wheel->GetWheelAngularVelocity() * Wheel->GetWheelRadius();

	const float Reference = FMath::Max(FMath::Abs(ForwardSpeed), 100.0f);
	return (WheelSurfaceSpeed - ForwardSpeed) / Reference;
}

void UVehicleDriverAssistComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!MovementComponent || !MovementComponent->HasValidPhysicsState() || DeltaTime <= SMALL_NUMBER)
	{
		return;
	}

	if (!bAssistsEnabled)
	{
		return;
	}

	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	for (int32 Index = 0; Index < PendingDriveTorque.Num(); ++Index)
	{
		PendingDriveTorque[Index] = 0.0f;
		PendingBrakeTorque[Index] = 0.0f;
	}

	AssistState.bTractionControlActive = false;
	AssistState.bStabilityControlActive = false;
	AssistState.bABSActive = false;

	const FVector LocalVelocity = Owner->GetActorTransform().InverseTransformVectorNoScale(Owner->GetVelocity());
	const float ForwardSpeed = LocalVelocity.X;
	const float SpeedKPH = Owner->GetVelocity().Size() * CmsToKph;

	UpdateLaunchControl(SpeedKPH);
	UpdateWheelSlipControllers(DeltaTime, ForwardSpeed);
	UpdateStabilityControl(DeltaTime, ForwardSpeed);
	UpdateActiveAero(SpeedKPH);

	for (int32 Index = 0; Index < PendingDriveTorque.Num(); ++Index)
	{
		MovementComponent->SetDriveTorque(PendingDriveTorque[Index], Index);
		MovementComponent->SetBrakeTorque(PendingBrakeTorque[Index], Index);

		PrevDriveTorque[Index] = PendingDriveTorque[Index];
		PrevBrakeTorque[Index] = PendingBrakeTorque[Index];
	}
}

void UVehicleDriverAssistComponent::UpdateWheelSlipControllers(float DeltaTime, float ForwardSpeed)
{
	const int32 NumWheels = MovementComponent->GetNumWheels();

	const float ActiveSlipTarget = AssistState.bLaunchControlActive ? LaunchControlTargetSlip : TractionControlTargetSlip;

	for (int32 WheelIndex = 0; WheelIndex < NumWheels; ++WheelIndex)
	{
		if (!WheelInfos.IsValidIndex(WheelIndex) || !AssistState.Wheels.IsValidIndex(WheelIndex))
		{
			continue;
		}

		const FWheelInfo& Info = WheelInfos[WheelIndex];
		const FWheelStatus& Status = MovementComponent->GetWheelState(WheelIndex);

		FVehicleAssistWheelState& WheelState = AssistState.Wheels[WheelIndex];
		WheelState.TractionControlTorque = 0.0f;
		WheelState.ABSReleaseTorque = 0.0f;
		WheelState.TractionControlAmount = 0.0f;

		const float SlipRatio = ComputeSlipRatio(WheelIndex, ForwardSpeed);
		WheelState.SlipRatio = SlipRatio;

		if (!Status.bInContact)
		{

			SlipIntegrators[WheelIndex] = FMath::FInterpTo(SlipIntegrators[WheelIndex], 0.0f, DeltaTime, 5.0f);
			continue;
		}

		const float UnassistedDriveTorque = Status.DriveTorque - PrevDriveTorque[WheelIndex];
		const float UnassistedBrakeTorque = Status.BrakeTorque - PrevBrakeTorque[WheelIndex];

		const bool bWantsTractionControl = (bTractionControlEnabled || AssistState.bLaunchControlActive)
			&& Info.bIsDriven
			&& UnassistedDriveTorque > KINDA_SMALL_NUMBER;

		if (bWantsTractionControl && SlipRatio > ActiveSlipTarget)
		{
			const float Error = SlipRatio - ActiveSlipTarget;

			SlipIntegrators[WheelIndex] = FMath::Clamp(
				SlipIntegrators[WheelIndex] + Error * DeltaTime,
				0.0f,
				TractionControlIntegralMax);

			const float Requested = Error * TractionControlGain
				+ SlipIntegrators[WheelIndex] * TractionControlIntegralGain;

			const float Reduction = FMath::Clamp(Requested, 0.0f, UnassistedDriveTorque);

			PendingDriveTorque[WheelIndex] -= Reduction;
			WheelState.TractionControlTorque = -Reduction;
			WheelState.TractionControlAmount = UnassistedDriveTorque > KINDA_SMALL_NUMBER
				? FMath::Clamp(Reduction / UnassistedDriveTorque, 0.0f, 1.0f)
				: 0.0f;

			AssistState.bTractionControlActive |= Reduction > KINDA_SMALL_NUMBER;
		}
		else
		{
			SlipIntegrators[WheelIndex] = FMath::FInterpTo(SlipIntegrators[WheelIndex], 0.0f, DeltaTime, 5.0f);
		}

		if (bABSEnabled && UnassistedBrakeTorque > KINDA_SMALL_NUMBER && SlipRatio < -ABSTargetSlip)
		{
			const float Overshoot = -ABSTargetSlip - SlipRatio;
			const float Requested = Overshoot * ABSGain;

			const float Relief = FMath::Clamp(Requested, 0.0f, UnassistedBrakeTorque);

			PendingBrakeTorque[WheelIndex] -= Relief;
			WheelState.ABSReleaseTorque = -Relief;

			AssistState.bABSActive |= Relief > KINDA_SMALL_NUMBER;
		}
	}
}

void UVehicleDriverAssistComponent::UpdateStabilityControl(float DeltaTime, float ForwardSpeed)
{
	AssistState.YawRateError = 0.0f;
	AssistState.TargetYawRate = 0.0f;
	AssistState.HandlingState = EVehicleHandlingState::Neutral;

	for (FVehicleAssistWheelState& WheelState : AssistState.Wheels)
	{
		WheelState.StabilityBrakeTorque = 0.0f;
	}

	if (!ChassisMesh || !ChassisMesh->IsSimulatingPhysics())
	{
		return;
	}

	const float SpeedKPH = FMath::Abs(ForwardSpeed) * CmsToKph;
	if (SpeedKPH < StabilityMinSpeed)
	{
		return;
	}

	float SteerAngleSum = 0.0f;
	int32 SteeredCount = 0;

	for (const FWheelInfo& Info : WheelInfos)
	{
		if (!MovementComponent->Wheels.IsValidIndex(Info.Index) || !IsWheelOutputValid(MovementComponent, Info.Index))
		{
			continue;
		}

		UChaosVehicleWheel* Wheel = MovementComponent->Wheels[Info.Index];
		if (Wheel && Wheel->bAffectedBySteering)
		{
			SteerAngleSum += Wheel->GetSteerAngle();
			++SteeredCount;
		}
	}

	if (SteeredCount == 0)
	{
		return;
	}

	const float MeanSteerRad = FMath::DegreesToRadians(SteerAngleSum / SteeredCount);

	float TargetYawRateRad = (ForwardSpeed / Wheelbase) * FMath::Tan(MeanSteerRad);

	const float SpeedForLimit = FMath::Max(FMath::Abs(ForwardSpeed), 100.0f);
	const float MaxYawRateRad = (AssumedLateralGrip * GravityCmS2) / SpeedForLimit;
	TargetYawRateRad = FMath::Clamp(TargetYawRateRad, -MaxYawRateRad, MaxYawRateRad);

	const float TargetYawRateDeg = FMath::RadiansToDegrees(TargetYawRateRad);
	const float ActualYawRateDeg = ChassisMesh->GetPhysicsAngularVelocityInDegrees().Z;

	AssistState.TargetYawRate = TargetYawRateDeg;

	const float Error = FMath::Abs(ActualYawRateDeg) - FMath::Abs(TargetYawRateDeg);
	AssistState.YawRateError = Error;

	if (!bStabilityControlEnabled || FMath::Abs(Error) < YawRateDeadband)
	{
		return;
	}

	const bool bTurningRight = FMath::Abs(TargetYawRateDeg) > 1.0f
		? TargetYawRateDeg > 0.0f
		: ActualYawRateDeg > 0.0f;

	const float Magnitude = FMath::Abs(Error) - YawRateDeadband;
	const float BrakeTorque = FMath::Clamp(Magnitude * StabilityControlGain, 0.0f, StabilityMaxBrakeTorque);

	const bool bOversteering = Error > 0.0f;

	AssistState.HandlingState = bOversteering ? EVehicleHandlingState::Oversteer : EVehicleHandlingState::Understeer;

	const EAxleType TargetAxle = bOversteering ? EAxleType::Front : EAxleType::Rear;

	const bool bWantLeftSide = bOversteering ? !bTurningRight : bTurningRight;

	for (const FWheelInfo& Info : WheelInfos)
	{
		if (Info.Axle != TargetAxle || Info.bIsLeft != bWantLeftSide)
		{
			continue;
		}

		if (!AssistState.Wheels.IsValidIndex(Info.Index))
		{
			continue;
		}

		if (!MovementComponent->GetWheelState(Info.Index).bInContact)
		{
			continue;
		}

		PendingBrakeTorque[Info.Index] += BrakeTorque;
		AssistState.Wheels[Info.Index].StabilityBrakeTorque = BrakeTorque;
		AssistState.bStabilityControlActive = true;
	}
}

void UVehicleDriverAssistComponent::UpdateActiveAero(float SpeedKPH)
{
	if (!bHasBaseDownforce)
	{
		return;
	}

	if (!bActiveAeroEnabled)
	{
		AssistState.ActiveDownforceCoefficient = BaseDownforce;
		return;
	}

	const float SpeedAlpha = FMath::Clamp(SpeedKPH / FMath::Max(1.0f, AeroRefSpeed), 0.0f, 1.0f);
	float Coefficient = FMath::Lerp(MinDownforce, MaxDownforce, SpeedAlpha);

	const float SteeringDemand = FMath::Abs(MovementComponent->GetSteeringInput());
	const float BrakeDemand = MovementComponent->GetBrakeInput();
	const float Demand = FMath::Clamp(FMath::Max(SteeringDemand, BrakeDemand), 0.0f, 1.0f);

	const float ShedAmount = StraightLineShed * (1.0f - Demand);
	Coefficient = FMath::Lerp(Coefficient, MinDownforce, ShedAmount);

	MovementComponent->SetDownforceCoefficient(Coefficient);
	AssistState.ActiveDownforceCoefficient = Coefficient;
}

void UVehicleDriverAssistComponent::UpdateLaunchControl(float SpeedKPH)
{
	if (!bLaunchControlEnabled)
	{
		bLaunchControlArmed = false;
		AssistState.bLaunchControlActive = false;
		return;
	}

	if (bLaunchControlArmed && SpeedKPH > LaunchControlMaxSpeed)
	{
		bLaunchControlArmed = false;
	}

	AssistState.bLaunchControlActive = bLaunchControlArmed
		&& MovementComponent->GetThrottleInput() > 0.5f
		&& SpeedKPH <= LaunchControlMaxSpeed;
}

FString UVehicleDriverAssistComponent::GetActiveAssistsLabel() const
{
	if (!bAssistsEnabled)
	{
		return TEXT("ASSISTS OFF");
	}

	TArray<FString> Active;
	if (AssistState.bTractionControlActive)	{ Active.Add(TEXT("TC")); }
	if (AssistState.bStabilityControlActive)	{ Active.Add(TEXT("ESC")); }
	if (AssistState.bABSActive)				{ Active.Add(TEXT("ABS")); }
	if (AssistState.bLaunchControlActive)		{ Active.Add(TEXT("LC")); }

	return Active.Num() > 0 ? FString::Join(Active, TEXT(" ")) : FString();
}
