#pragma once

#include "CoreMinimal.h"

class UAstrawildItemRegistrySubsystem;

/**
 * Batch 8 — "The Grand Menagerie": the generated 200-species bestiary table.
 *
 * Source of truth: Scripts/generate_bestiary.py emits both this table's .cpp and
 * Docs/ASTRAWILD_BESTIARY_CODEX.md (one designed roster, two artifacts — never
 * edit the generated .cpp by hand).
 *
 * Every species carries the full Batch 8 appearance contract (family / body plan /
 * size class / two tint colors / home zone) plus the complete gameplay template
 * (element, weakness, role, personality, activity pattern, stats, capture
 * difficulty, diet, loot, work affinities), so the existing AI, capture, combat
 * and work-site systems run every species with zero extra code.
 */
namespace AstrawildBestiary
{
    /** Registers all generated species with the item registry (idempotent per world). */
    ASTRAWILDCORE_API void RegisterAll(UAstrawildItemRegistrySubsystem* Registry);

    /** Number of generated species rows (200). */
    ASTRAWILDCORE_API int32 GetRowCount();

    /**
     * Pure static integrity check used by the automation test suite: unique ids,
     * sane stats, capture difficulty bounds, weakness != element, every home zone
     * id resolves, and all 12 zones are covered. Appends human-readable problems.
     */
    ASTRAWILDCORE_API void ValidateTable(TArray<FString>& OutProblems);
}
