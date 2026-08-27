// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AstrawildCore : ModuleRules
{
	public AstrawildCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				"AstrawildCore/Public"
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				"AstrawildCore/Private"
			}
		);

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"InputCore",
				"EnhancedInput",
				"GameplayTags",
				"AIModule",
				"NavigationSystem",
				"UMG",
				"PhysicsCore"
			}
		);

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore"
			}
		);
	}
}