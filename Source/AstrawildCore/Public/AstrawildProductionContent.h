#pragma once

#include "CoreMinimal.h"

class UAstrawildItemRegistrySubsystem;

/**
 * Production V2 content wave (Master Plan §3 STEP 3 — data-driven content
 * foundation). Registers CODE_DEFAULT definitions for:
 *   - weapon profiles (8 families × distinct firing archetypes),
 *   - armor tier sets (Mk II / Mk III / Experimental),
 *   - scanner tiers (range/speed/hidden-vein/ancient-signal),
 *   - drone modules + specialist robot chassis,
 *   - deterministic resource node identities (P0 fix),
 *   - work-site production chains (consume→produce),
 *   - world events, POIs and biome asset contracts (Visual Vertical Slice).
 *
 * Everything flows through the item registry's same-id override contract:
 * authored .uasset definitions replace these without code changes.
 */
class ASTRAWILDCORE_API UAstrawildProductionContent
{
public:
    /** Register every Production V2 content family (called from BuildDefaults). */
    static void BuildAll(UAstrawildItemRegistrySubsystem* Registry);

    static void BuildWeapons(UAstrawildItemRegistrySubsystem* Registry);
    static void BuildArmorAndScanners(UAstrawildItemRegistrySubsystem* Registry);
    static void BuildRobotics(UAstrawildItemRegistrySubsystem* Registry);
    static void BuildResourceNodes(UAstrawildItemRegistrySubsystem* Registry);
    static void BuildWorkSites(UAstrawildItemRegistrySubsystem* Registry);
    static void BuildWorldEvents(UAstrawildItemRegistrySubsystem* Registry);
    static void BuildPOIs(UAstrawildItemRegistrySubsystem* Registry);
    static void BuildBiomes(UAstrawildItemRegistrySubsystem* Registry);
    static void BuildProductionEchoes(UAstrawildItemRegistrySubsystem* Registry);
    static void BuildEvolutionTargets(UAstrawildItemRegistrySubsystem* Registry);
    static void BuildProductionTechnologies(UAstrawildItemRegistrySubsystem* Registry);
    static void BuildProductionQuests(UAstrawildItemRegistrySubsystem* Registry);
    static void BuildDialogueTrees(UAstrawildItemRegistrySubsystem* Registry);

    /**
     * Final Run (FR-5) — Act 3 "The Storm Crown" content pack:
     *  - MQ-13..17 quest chain (Storm Anchors → Crown Relay → Eye of the
     *    Maelstrom → The Drowned Sovereign → First Dawn Again),
     *  - the boss species roster (Glass Tyrant / Eye Sentinel / Drowned
     *    Sovereign),
     *  - items (Sovereign Core, Maelstrom Glass, Skiff Stratos Coil),
     *    tech (Skiff Engineering), recipe, loot (Loot_EyeCore),
     *  - Warden Maren's ending dialogue (Ending_BreakCage /
     *    Ending_StormSleeps → the two endings).
     */
    static void BuildFinalRunContent(UAstrawildItemRegistrySubsystem* Registry);

    /**
     * DCP-1 — the SQ-23 post-game side-quest batch (user directive
     * "re-open every deferred item, playable-first"). Five standalone
     * post-game quests (the single-active-quest rule is respected: no
     * NextQuestId chaining, each completes on its own), offered through
     * NPC dialogue gated on the MQ-17 terminus (Quest_FirstDawnAgain
     * completed = post-game by definition) + one-time offer flags:
     *  - Quest_PostVigil       (Maren)  — hostile cull patrol,
     *  - Quest_PostFieldNotes  (Wren)   — observe/capture science beat,
     *  - Quest_PostGlassTrade  (Tam)    — maelstrom-glass economy beat,
     *  - Quest_PostDeepRecords (Nima)   — sea POI + capture beat,
     *  - Quest_PostLongWatch   (Kael)   — zone tour + survival watch.
     * Every objective target resolves against EXISTING content ids
     * (species / items / POIs / zones) — no new world spawns required.
     */
    static void BuildPostGameQuests(UAstrawildItemRegistrySubsystem* Registry);
};
