using UnrealBuildTool;

public class NPCpropertyEditor : ModuleRules
{
    public NPCpropertyEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "NPCproperty" });
        PrivateDependencyModuleNames.AddRange(new[] { "UnrealEd", "BlueprintGraph", "Kismet", "KismetCompiler", "AssetRegistry", "StateTreeModule", "StateTreeEditorModule", "GameplayStateTreeModule", "PropertyBindingUtils", "AIModule", "NavigationSystem", "AnimGraph", "AnimGraphRuntime" });
    }
}
