// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CVehicleArea : ModuleRules
{
	public CVehicleArea(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"ChaosVehicles",
			"PhysicsCore",
			"UMG",
			"Slate"
		});

		PublicIncludePaths.AddRange(new string[] {
			"CVehicleArea",
			"CVehicleArea/SportsCar",
			"CVehicleArea/OffroadCar",
			"CVehicleArea/Variant_OffRoad",
			"CVehicleArea/Variant_TimeTrial",
			"CVehicleArea/Variant_TimeTrial/UI"
		});

		// The vehicle engineering systems are included by folder-qualified path
		// (for example "Telemetry/VehicleTelemetryComponent.h") so that the module
		// root is the only include path they need and their names cannot collide
		// with the template's flat headers.

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
