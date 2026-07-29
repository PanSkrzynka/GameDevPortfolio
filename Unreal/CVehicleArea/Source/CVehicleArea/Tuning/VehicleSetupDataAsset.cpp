#include "VehicleSetupDataAsset.h"
#include "CVehicleArea.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "ChaosVehicleWheel.h"

const FVehicleAxleTuning& UVehicleSetupDataAsset::GetAxleTuning(EVehicleTuningAxle Axle) const
{
	return Axle == EVehicleTuningAxle::Front ? FrontAxle : RearAxle;
}

void UVehicleSetupDataAsset::ApplyStaticSetup(UChaosWheeledVehicleMovementComponent* MovementComponent) const
{
	if (!MovementComponent)
	{
		return;
	}

	MovementComponent->Mass = Chassis.Mass;
	MovementComponent->ChassisHeight = Chassis.ChassisHeight;
	MovementComponent->ChassisWidth = Chassis.ChassisWidth;
	MovementComponent->DragCoefficient = Chassis.DragCoefficient;
	MovementComponent->DownforceCoefficient = Chassis.DownforceCoefficient;
	MovementComponent->bEnableCenterOfMassOverride = Chassis.bEnableCenterOfMassOverride;
	MovementComponent->CenterOfMassOverride = Chassis.CenterOfMassOverride;

	MovementComponent->SteeringSetup.SteeringType = Chassis.SteeringType;
	MovementComponent->SteeringSetup.AngleRatio = Chassis.SteeringAngleRatio;

	MovementComponent->EngineSetup.MaxTorque = Drivetrain.MaxTorque;
	MovementComponent->EngineSetup.MaxRPM = Drivetrain.MaxRPM;
	MovementComponent->EngineSetup.EngineIdleRPM = Drivetrain.EngineIdleRPM;
	MovementComponent->EngineSetup.EngineBrakeEffect = Drivetrain.EngineBrakeEffect;
	MovementComponent->EngineSetup.EngineRevUpMOI = Drivetrain.EngineRevUpMOI;
	MovementComponent->EngineSetup.EngineRevDownRate = Drivetrain.EngineRevDownRate;

	MovementComponent->TransmissionSetup.bUseAutomaticGears = Drivetrain.bUseAutomaticGears;
	MovementComponent->TransmissionSetup.bUseAutoReverse = Drivetrain.bUseAutoReverse;
	MovementComponent->TransmissionSetup.FinalRatio = Drivetrain.FinalRatio;
	MovementComponent->TransmissionSetup.ChangeUpRPM = Drivetrain.ChangeUpRPM;
	MovementComponent->TransmissionSetup.ChangeDownRPM = Drivetrain.ChangeDownRPM;
	MovementComponent->TransmissionSetup.GearChangeTime = Drivetrain.GearChangeTime;
	MovementComponent->TransmissionSetup.TransmissionEfficiency = Drivetrain.TransmissionEfficiency;
	MovementComponent->TransmissionSetup.ForwardGearRatios = Drivetrain.ForwardGearRatios;
	MovementComponent->TransmissionSetup.ReverseGearRatios = Drivetrain.ReverseGearRatios;

	MovementComponent->DifferentialSetup.DifferentialType = Drivetrain.DifferentialType;
	MovementComponent->DifferentialSetup.FrontRearSplit = Drivetrain.FrontRearSplit;
}

void UVehicleSetupDataAsset::ApplyAxleToWheel(UChaosWheeledVehicleMovementComponent* MovementComponent, int32 WheelIndex, const FVehicleAxleTuning& Axle) const
{
	MovementComponent->SetWheelRadius(WheelIndex, Axle.WheelRadius);
	MovementComponent->SetWheelFrictionMultiplier(WheelIndex, Axle.FrictionForceMultiplier);
	MovementComponent->SetWheelMaxBrakeTorque(WheelIndex, Axle.MaxBrakeTorque);
	MovementComponent->SetWheelHandbrakeTorque(WheelIndex, Axle.MaxHandBrakeTorque);
	MovementComponent->SetWheelMaxSteerAngle(WheelIndex, Axle.MaxSteerAngle);

	MovementComponent->SetAffectedByBrake(WheelIndex, Axle.bAffectedByBrake);
	MovementComponent->SetAffectedByHandbrake(WheelIndex, Axle.bAffectedByHandbrake);
	MovementComponent->SetAffectedBySteering(WheelIndex, Axle.bAffectedBySteering);
	MovementComponent->SetAffectedByEngine(WheelIndex, Axle.bAffectedByEngine);

	MovementComponent->SetABSEnabled(WheelIndex, Axle.bABSEnabled);
	MovementComponent->SetTractionControlEnabled(WheelIndex, Axle.bTractionControlEnabled);
	MovementComponent->SetTorqueCombineMethod(Axle.ExternalTorqueCombineMethod, WheelIndex);

	MovementComponent->SetSuspensionParams(
		Axle.SpringRate,
		Axle.SuspensionDampingRatio,
		Axle.SpringPreload,
		Axle.SuspensionMaxRaise,
		Axle.SuspensionMaxDrop,
		WheelIndex);
}

void UVehicleSetupDataAsset::ApplyLiveSetup(UChaosWheeledVehicleMovementComponent* MovementComponent) const
{
	if (!MovementComponent || !MovementComponent->HasValidPhysicsState())
	{
		return;
	}

	MovementComponent->SetMaxEngineTorque(Drivetrain.MaxTorque);
	MovementComponent->SetDragCoefficient(Chassis.DragCoefficient);
	MovementComponent->SetDownforceCoefficient(Chassis.DownforceCoefficient);
	MovementComponent->SetDifferentialFrontRearSplit(Drivetrain.FrontRearSplit);

	const int32 NumWheels = MovementComponent->Wheels.Num();
	for (int32 WheelIndex = 0; WheelIndex < NumWheels; ++WheelIndex)
	{
		UChaosVehicleWheel* Wheel = MovementComponent->Wheels[WheelIndex];
		if (!Wheel)
		{
			continue;
		}

		switch (Wheel->GetAxleType())
		{
		case EAxleType::Front:
			ApplyAxleToWheel(MovementComponent, WheelIndex, FrontAxle);
			break;

		case EAxleType::Rear:
			ApplyAxleToWheel(MovementComponent, WheelIndex, RearAxle);
			break;

		default:
			UE_LOG(LogCVehicleArea, Warning, TEXT("VehicleSetup '%s': wheel %d has no axle type."), *GetName(), WheelIndex);
			break;
		}
	}
}

void UVehicleSetupDataAsset::ApplyTo(UChaosWheeledVehicleMovementComponent* MovementComponent, bool bRecreatePhysicsState) const
{
	if (!MovementComponent)
	{
		return;
	}

	ApplyStaticSetup(MovementComponent);

	if (bRecreatePhysicsState && MovementComponent->HasValidPhysicsState())
	{
		MovementComponent->RecreatePhysicsState();
	}

	ApplyLiveSetup(MovementComponent);
}

void UVehicleSetupDataAsset::CaptureFrom(UChaosWheeledVehicleMovementComponent* MovementComponent)
{
	if (!MovementComponent)
	{
		return;
	}

	Chassis.Mass = MovementComponent->Mass;
	Chassis.ChassisHeight = MovementComponent->ChassisHeight;
	Chassis.ChassisWidth = MovementComponent->ChassisWidth;
	Chassis.DragCoefficient = MovementComponent->DragCoefficient;
	Chassis.DownforceCoefficient = MovementComponent->DownforceCoefficient;
	Chassis.bEnableCenterOfMassOverride = MovementComponent->bEnableCenterOfMassOverride;
	Chassis.CenterOfMassOverride = MovementComponent->CenterOfMassOverride;
	Chassis.SteeringType = MovementComponent->SteeringSetup.SteeringType;
	Chassis.SteeringAngleRatio = MovementComponent->SteeringSetup.AngleRatio;

	Drivetrain.MaxTorque = MovementComponent->EngineSetup.MaxTorque;
	Drivetrain.MaxRPM = MovementComponent->EngineSetup.MaxRPM;
	Drivetrain.EngineIdleRPM = MovementComponent->EngineSetup.EngineIdleRPM;
	Drivetrain.EngineBrakeEffect = MovementComponent->EngineSetup.EngineBrakeEffect;
	Drivetrain.EngineRevUpMOI = MovementComponent->EngineSetup.EngineRevUpMOI;
	Drivetrain.EngineRevDownRate = MovementComponent->EngineSetup.EngineRevDownRate;

	Drivetrain.bUseAutomaticGears = MovementComponent->TransmissionSetup.bUseAutomaticGears;
	Drivetrain.bUseAutoReverse = MovementComponent->TransmissionSetup.bUseAutoReverse;
	Drivetrain.FinalRatio = MovementComponent->TransmissionSetup.FinalRatio;
	Drivetrain.ChangeUpRPM = MovementComponent->TransmissionSetup.ChangeUpRPM;
	Drivetrain.ChangeDownRPM = MovementComponent->TransmissionSetup.ChangeDownRPM;
	Drivetrain.GearChangeTime = MovementComponent->TransmissionSetup.GearChangeTime;
	Drivetrain.TransmissionEfficiency = MovementComponent->TransmissionSetup.TransmissionEfficiency;
	Drivetrain.ForwardGearRatios = MovementComponent->TransmissionSetup.ForwardGearRatios;
	Drivetrain.ReverseGearRatios = MovementComponent->TransmissionSetup.ReverseGearRatios;

	Drivetrain.DifferentialType = MovementComponent->DifferentialSetup.DifferentialType;
	Drivetrain.FrontRearSplit = MovementComponent->DifferentialSetup.FrontRearSplit;

	for (const FChaosWheelSetup& WheelSetup : MovementComponent->WheelSetups)
	{
		if (!WheelSetup.WheelClass)
		{
			continue;
		}

		UChaosVehicleWheel* Defaults = WheelSetup.WheelClass.GetDefaultObject();
		if (!Defaults || Defaults->GetAxleType() == EAxleType::Undefined)
		{
			continue;
		}

		FVehicleAxleTuning& Target = (Defaults->GetAxleType() == EAxleType::Front) ? FrontAxle : RearAxle;

		Target.WheelRadius = Defaults->WheelRadius;
		Target.WheelWidth = Defaults->WheelWidth;
		Target.WheelMass = Defaults->WheelMass;
		Target.FrictionForceMultiplier = Defaults->FrictionForceMultiplier;
		Target.CorneringStiffness = Defaults->CorneringStiffness;
		Target.SideSlipModifier = Defaults->SideSlipModifier;
		Target.SlipThreshold = Defaults->SlipThreshold;
		Target.SkidThreshold = Defaults->SkidThreshold;
		Target.bAffectedBySteering = Defaults->bAffectedBySteering;
		Target.MaxSteerAngle = Defaults->MaxSteerAngle;
		Target.MaxBrakeTorque = Defaults->MaxBrakeTorque;
		Target.MaxHandBrakeTorque = Defaults->MaxHandBrakeTorque;
		Target.bAffectedByBrake = Defaults->bAffectedByBrake;
		Target.bAffectedByHandbrake = Defaults->bAffectedByHandbrake;
		Target.bAffectedByEngine = Defaults->bAffectedByEngine;
		Target.ExternalTorqueCombineMethod = Defaults->ExternalTorqueCombineMethod;
		Target.bABSEnabled = Defaults->bABSEnabled;
		Target.bTractionControlEnabled = Defaults->bTractionControlEnabled;
		Target.SpringRate = Defaults->SpringRate;
		Target.SpringPreload = Defaults->SpringPreload;
		Target.SuspensionDampingRatio = Defaults->SuspensionDampingRatio;
		Target.SuspensionMaxRaise = Defaults->SuspensionMaxRaise;
		Target.SuspensionMaxDrop = Defaults->SuspensionMaxDrop;
		Target.RollbarScaling = Defaults->RollbarScaling;
		Target.WheelLoadRatio = Defaults->WheelLoadRatio;
		Target.SweepShape = Defaults->SweepShape;
	}
}

float UVehicleSetupDataAsset::GetFrontBrakeBias() const
{
	const float Total = FrontAxle.MaxBrakeTorque + RearAxle.MaxBrakeTorque;
	return Total > KINDA_SMALL_NUMBER ? FrontAxle.MaxBrakeTorque / Total : 0.5f;
}

float UVehicleSetupDataAsset::GetTopSpeedKPH() const
{
	if (Drivetrain.ForwardGearRatios.Num() == 0 || Drivetrain.FinalRatio <= KINDA_SMALL_NUMBER)
	{
		return 0.0f;
	}

	float TopGearRatio = TNumericLimits<float>::Max();
	for (const float Ratio : Drivetrain.ForwardGearRatios)
	{
		if (Ratio > KINDA_SMALL_NUMBER)
		{
			TopGearRatio = FMath::Min(TopGearRatio, Ratio);
		}
	}

	if (TopGearRatio >= TNumericLimits<float>::Max())
	{
		return 0.0f;
	}

	const float WheelRPM = Drivetrain.MaxRPM / (TopGearRatio * Drivetrain.FinalRatio);
	const float WheelCircumferenceCm = 2.0f * PI * RearAxle.WheelRadius;
	const float CmPerMinute = WheelRPM * WheelCircumferenceCm;

	return CmPerMinute * 60.0f / 100000.0f;
}
