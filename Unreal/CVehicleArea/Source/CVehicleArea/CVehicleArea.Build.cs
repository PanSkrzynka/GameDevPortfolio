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
			"Slate",
			"SlateCore"
		});

		PublicIncludePaths.AddRange(new string[] {
			"CVehicleArea",
			"CVehicleArea/SportsCar",
			"CVehicleArea/OffroadCar",
			"CVehicleArea/Variant_OffRoad",
			"CVehicleArea/Variant_TimeTrial",
			"CVehicleArea/Variant_TimeTrial/UI"
		});


		PrivateDependencyModuleNames.AddRange(new string[] { });


	}
}
