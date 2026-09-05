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
                // Final-audit F-18: GameplayAbilities removed — no GAS/StateTree code
                // exists in the module (comment-only future references); the plugin is
                // disabled in the .uproject to shed the compile weight.
                "GameplayTags",
                "GameplayTasks",
                // V2 architecture (audit §F): AI, navigation, UI foundation.
                "AIModule",
                "NavigationSystem",
                "UMG",
                // Batch 7 — The Shattered Vale: runtime procedural terrain tiles.
                "ProceduralMeshComponent",
                // Content Pack CP-05 — weapon Niagara bindings (muzzle/impact/trail).
                "Niagara",
                // LCP-6 — LAN discovery beacon (UDP broadcast; gameplay networking
                // stays the engine's IpConnection replication).
                "Sockets"
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
