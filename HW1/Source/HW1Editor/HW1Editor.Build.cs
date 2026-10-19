using UnrealBuildTool;

public class HW1Editor : ModuleRules
{
	public HW1Editor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine" });
		PrivateDependencyModuleNames.AddRange(new[] {
			"HW1", "UnrealEd", "BlueprintGraph", "Kismet", "KismetCompiler",
			"BlueprintEditorLibrary", "AnimGraph", "AnimGraphRuntime", "AssetRegistry",
			"EnhancedInput", "InputCore", "Json", "JsonUtilities", "Slate", "SlateCore",
			"LevelEditor"
		});
	}
}
