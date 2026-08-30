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
    static void BuildProductionTechnologies(UAstrawildItemRegistrySubsystem* Registry);
    static void BuildProductionQuests(UAstrawildItemRegistrySubsystem* Registry);
};
