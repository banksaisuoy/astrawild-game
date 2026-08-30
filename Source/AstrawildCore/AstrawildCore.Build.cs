using UnrealBuildTool;

public class AstrawildCore : ModuleRules
{
    public AstrawildCore(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "EnhancedInput",
                "GameplayAbilities",
                "GameplayTags",
                "GameplayTasks",
                // V2 architecture (audit §F): AI, navigation, UI foundation.
                "AIModule",
                "NavigationSystem",
                "UMG",
                // Batch 7 — The Shattered Vale: runtime procedural terrain tiles.
                "ProceduralMeshComponent"
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
