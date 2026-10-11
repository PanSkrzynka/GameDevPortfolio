using UnrealBuildTool;
using System.Collections.Generic;

public class NPCpropertyEditorTarget : TargetRules
{
	public NPCpropertyEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
		ExtraModuleNames.Add("NPCproperty");
		ExtraModuleNames.Add("NPCpropertyEditor");
	}
}
