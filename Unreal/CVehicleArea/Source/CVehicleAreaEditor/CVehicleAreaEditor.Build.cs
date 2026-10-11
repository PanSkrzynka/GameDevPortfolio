using UnrealBuildTool;
public class CVehicleAreaEditor : ModuleRules
{
    public CVehicleAreaEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrivateDependencyModuleNames.AddRange(new [] { "Core", "CoreUObject", "Engine", "CVehicleArea", "UMG", "SlateCore", "UnrealEd", "UMGEditor", "Kismet", "KismetCompiler", "BlueprintGraph", "AssetRegistry" });
    }
}
