// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MMO_V03 : ModuleRules
{
	public MMO_V03(ReadOnlyTargetRules Target) : base(Target)
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
			"MMO_V03",
			"MMO_V03/Variant_Platforming",
			"MMO_V03/Variant_Platforming/Animation",
			"MMO_V03/Variant_Combat",
			"MMO_V03/Variant_Combat/AI",
			"MMO_V03/Variant_Combat/Animation",
			"MMO_V03/Variant_Combat/Gameplay",
			"MMO_V03/Variant_Combat/Interfaces",
			"MMO_V03/Variant_Combat/UI",
			"MMO_V03/Variant_SideScrolling",
			"MMO_V03/Variant_SideScrolling/AI",
			"MMO_V03/Variant_SideScrolling/Gameplay",
			"MMO_V03/Variant_SideScrolling/Interfaces",
			"MMO_V03/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
