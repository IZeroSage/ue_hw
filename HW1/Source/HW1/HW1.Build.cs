// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class HW1 : ModuleRules
{
	public HW1(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"HW1",
			"HW1/Variant_Platforming",
			"HW1/Variant_Platforming/Animation",
			"HW1/Variant_Combat",
			"HW1/Variant_Combat/AI",
			"HW1/Variant_Combat/Animation",
			"HW1/Variant_Combat/Gameplay",
			"HW1/Variant_Combat/Interfaces",
			"HW1/Variant_Combat/UI",
			"HW1/Variant_SideScrolling",
			"HW1/Variant_SideScrolling/AI",
			"HW1/Variant_SideScrolling/Gameplay",
			"HW1/Variant_SideScrolling/Interfaces",
			"HW1/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
