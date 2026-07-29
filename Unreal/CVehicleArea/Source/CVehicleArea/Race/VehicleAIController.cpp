#include "VehicleAIController.h"
#include "CVehicleArea.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "ChaosVehicleWheel.h"
#include "WheeledVehiclePawn.h"
#include "Components/SkeletalMeshComponent.h"

namespace
{
	constexpr float CmsToKph = 0.036f;
}

AVehicleAIController::AVehicleAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	bAttachToPawn = false;
}

void AVehicleAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	AWheeledVehiclePawn* VehiclePawn = Cast<AWheeledVehiclePawn>(InPawn);
	if (!VehiclePawn)
	{
		UE_LOG(LogCVehicleArea, Warning, TEXT("AI: '%s' is not a wheeled vehicle."), *GetNameSafe(InPawn));
		return;
	}

	MovementComponent = Cast<UChaosWheeledVehicleMovementComponent>(VehiclePawn->GetVehicleMovementComponent());
	if (!MovementComponent)
	{
		return;
	}

	if (const USkeletalMeshComponent* Mesh = VehiclePawn->GetMesh())
	{
		float MinX = TNumericLimits<float>::Max();
		float MaxX = TNumericLimits<float>::Lowest();
		int32 Found = 0;

		for (const FChaosWheelSetup& Setup : MovementComponent->WheelSetups)
		{
			const int32 BoneIndex = Mesh->GetBoneIndex(Setup.BoneName);
			if (BoneIndex == INDEX_NONE)
			{
				continue;
			}

			const FVector BoneLocation = Mesh->GetBoneTransform(BoneIndex, FTransform::Identity).GetLocation();
			MinX = FMath::Min(MinX, BoneLocation.X);
			MaxX = FMath::Max(MaxX, BoneLocation.X);
			++Found;
		}

		if (Found >= 2 && (MaxX - MinX) > 1.0f)
		{
			Wheelbase = MaxX - MinX;
		}
	}

	float LargestSteerAngle = 0.0f;
	for (const FChaosWheelSetup& Setup : MovementComponent->WheelSetups)
	{
		if (!Setup.WheelClass)
		{
			continue;
		}

		if (const UChaosVehicleWheel* Defaults = Setup.WheelClass.GetDefaultObject())
		{
			if (Defaults->bAffectedBySteering)
			{
				LargestSteerAngle = FMath::Max(LargestSteerAngle, Defaults->MaxSteerAngle);
			}
		}
	}

	if (LargestSteerAngle > 1.0f)
	{
		MaxSteerAngle = LargestSteerAngle;
	}

	LastPointIndex = INDEX_NONE;
	SpeedIntegral = 0.0f;
	PrevSpeedError = 0.0f;
	PrevSteering = 0.0f;
}

void AVehicleAIController::OnUnPossess()
{
	if (MovementComponent)
	{
		ApplyInputs(0.0f, 0.0f, 1.0f, true);
	}

	MovementComponent = nullptr;

	Super::OnUnPossess();
}

void AVehicleAIController::SetRacingLine(const FRacingLine& InRacingLine)
{
	RacingLine = InRacingLine;

	LastPointIndex = INDEX_NONE;
	SpeedIntegral = 0.0f;
	PrevSpeedError = 0.0f;
}

void AVehicleAIController::SetDrivingEnabled(bool bEnabled)
{
	bDrivingEnabled = bEnabled;

	if (!bEnabled && MovementComponent)
	{

		ApplyInputs(0.0f, 0.0f, 1.0f, true);

		SpeedIntegral = 0.0f;
		PrevSpeedError = 0.0f;
		PrevSteering = 0.0f;
	}
}

void AVehicleAIController::ApplyInputs(float Steering, float Throttle, float Brake, bool bHandbrake)
{
	if (!MovementComponent)
	{
		return;
	}

	MovementComponent->SetSteeringInput(FMath::Clamp(Steering, -1.0f, 1.0f));
	MovementComponent->SetThrottleInput(FMath::Clamp(Throttle, 0.0f, 1.0f));
	MovementComponent->SetBrakeInput(FMath::Clamp(Brake, 0.0f, 1.0f));
	MovementComponent->SetHandbrakeInput(bHandbrake);
}

float AVehicleAIController::UpdateSteering(const FTransform& VehicleTransform, float ForwardSpeed, int32 NearestIndex, float DeltaTime)
{

	const float Lookahead = FMath::Clamp(
		BaseLookahead + FMath::Abs(ForwardSpeed) * LookaheadPerSpeed,
		BaseLookahead,
		MaxLookahead);

	const FRacingLinePoint& Nearest = RacingLine.Points[NearestIndex];
	const FRacingLinePoint Target = RacingLine.SampleAtDistance(Nearest.Distance + Lookahead);
	AimPoint = Target.Location;

	const FVector LocalTarget = VehicleTransform.InverseTransformPosition(Target.Location);

	const float TargetDistanceSquared = FMath::Max(LocalTarget.SizeSquared2D(), 1.0f);
	const float Curvature = (2.0f * LocalTarget.Y) / TargetDistanceSquared;

	const float SteerAngleRad = FMath::Atan(Curvature * Wheelbase);
	const float SteerAngleDeg = FMath::RadiansToDegrees(SteerAngleRad);

	float Steering = (SteerAngleDeg / FMath::Max(MaxSteerAngle, 1.0f)) * SteeringGain;

	if (ForwardSpeed < -10.0f)
	{
		Steering = -Steering;
	}

	Steering = FMath::Clamp(Steering, -1.0f, 1.0f);

	Steering = FMath::FInterpTo(PrevSteering, Steering, DeltaTime, SteeringRate);
	PrevSteering = Steering;

	return Steering;
}

void AVehicleAIController::UpdateSpeedControl(float ForwardSpeed, int32 NearestIndex, float DeltaTime, float& OutThrottle, float& OutBrake)
{
	const FRacingLinePoint& Nearest = RacingLine.Points[NearestIndex];

	const float LookaheadDistance = FMath::Abs(ForwardSpeed) * SpeedLookaheadTime;
	const FRacingLinePoint Ahead = RacingLine.SampleAtDistance(Nearest.Distance + LookaheadDistance);

	const float ProfileSpeed = FMath::Min(Nearest.TargetSpeed, Ahead.TargetSpeed);

	const float SkillScale = FMath::Lerp(MinSpeedScale, 1.0f, FMath::Clamp(SkillLevel, 0.0f, 1.0f));
	DesiredSpeed = ProfileSpeed * SkillScale;

	const float Error = DesiredSpeed - ForwardSpeed;

	SpeedIntegral = FMath::Clamp(SpeedIntegral + Error * DeltaTime, -SpeedIntegralMax, SpeedIntegralMax);

	const float Derivative = DeltaTime > SMALL_NUMBER ? (Error - PrevSpeedError) / DeltaTime : 0.0f;
	PrevSpeedError = Error;

	const float Command = Error * SpeedGain
		+ SpeedIntegral * SpeedIntegralGain
		+ Derivative * SpeedDerivativeGain;

	if (Command >= 0.0f)
	{
		OutThrottle = FMath::Clamp(Command, 0.0f, 1.0f);
		OutBrake = 0.0f;

		if (Error < 0.0f)
		{
			SpeedIntegral = FMath::FInterpTo(SpeedIntegral, 0.0f, DeltaTime, 2.0f);
		}
	}
	else
	{
		OutThrottle = 0.0f;
		OutBrake = FMath::Clamp(-Command, 0.0f, 1.0f);
	}
}

bool AVehicleAIController::UpdateRecovery(float SpeedKPH, float DeltaTime)
{

	if (RecoveryTimer > 0.0f)
	{
		RecoveryTimer -= DeltaTime;

		if (RecoveryTimer <= 0.0f)
		{
			RecoveryTimer = 0.0f;
			StuckTimer = 0.0f;

			SpeedIntegral = 0.0f;
			PrevSpeedError = 0.0f;
			return false;
		}

		ApplyInputs(RecoverySteering, 0.0f, 1.0f, false);
		return true;
	}

	const bool bTryingToMove = MovementComponent && MovementComponent->GetThrottleInput() > 0.2f;

	if (SpeedKPH < StuckSpeed && bTryingToMove)
	{
		StuckTimer += DeltaTime;

		if (StuckTimer >= StuckTimeout)
		{
			RecoveryTimer = RecoveryDuration;

			RecoverySteering = PrevSteering >= 0.0f ? -1.0f : 1.0f;

			UE_LOG(LogCVehicleArea, Verbose, TEXT("AI '%s' stuck, reversing."), *GetNameSafe(GetPawn()));
			return true;
		}
	}
	else
	{
		StuckTimer = 0.0f;
	}

	return false;
}

void AVehicleAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	APawn* ControlledPawn = GetPawn();
	if (!MovementComponent || !ControlledPawn || DeltaTime <= SMALL_NUMBER)
	{
		return;
	}

	if (!bDrivingEnabled)
	{
		ApplyInputs(0.0f, 0.0f, 1.0f, true);
		return;
	}

	if (!RacingLine.IsValid())
	{

		ApplyInputs(0.0f, 0.0f, 1.0f, true);
		return;
	}

	const FTransform VehicleTransform = ControlledPawn->GetActorTransform();
	const FVector LocalVelocity = VehicleTransform.InverseTransformVectorNoScale(ControlledPawn->GetVelocity());
	const float ForwardSpeed = LocalVelocity.X;
	const float SpeedKPH = FMath::Abs(ForwardSpeed) * CmsToKph;

	if (UpdateRecovery(SpeedKPH, DeltaTime))
	{
		return;
	}

	int32 NearestIndex = RacingLine.FindNearestPoint(VehicleTransform.GetLocation(), LastPointIndex);

	if (NearestIndex != INDEX_NONE)
	{
		const float DistanceToLine = FVector::Dist(RacingLine.Points[NearestIndex].Location, VehicleTransform.GetLocation());
		if (DistanceToLine > MaxLineDistance)
		{
			NearestIndex = RacingLine.FindNearestPoint(VehicleTransform.GetLocation(), INDEX_NONE);
		}
	}

	if (NearestIndex == INDEX_NONE)
	{
		ApplyInputs(0.0f, 0.0f, 1.0f, true);
		return;
	}

	LastPointIndex = NearestIndex;
	DistanceAlongLine = RacingLine.Points[NearestIndex].Distance;

	const float Steering = UpdateSteering(VehicleTransform, ForwardSpeed, NearestIndex, DeltaTime);

	float Throttle = 0.0f;
	float Brake = 0.0f;
	UpdateSpeedControl(ForwardSpeed, NearestIndex, DeltaTime, Throttle, Brake);

	ApplyInputs(Steering, Throttle, Brake, false);
}
