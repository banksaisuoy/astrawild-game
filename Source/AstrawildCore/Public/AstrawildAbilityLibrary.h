#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AstrawildTypes.h"
#include "AstrawildAbilityLibrary.generated.h"

class UAstrawildEchoDefinition;

/**
 * GDP-1 — "Every creature fights like itself": the Echo ability library.
 *
 * 44 code-default abilities. Authored species (ContentLibrary) reference curated
 * signature abilities by id through UAstrawildEchoDefinition::AbilityIds; every
 * other species derives a deterministic loadout from element + role + family, so
 * all 210 species cast something that fits their identity with zero extra data.
 *
 * Registration follows the ContentLibrary contract: idempotent, world-free, and
 * replaceable later by .uasset definitions that reuse the same ids.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildAbilityLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Registers all ability templates (idempotent, process-lifetime). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Ability")
    static void BuildDefaults();

    /** Exact lookup; nullptr when unknown. */
    static const FAstrawildAbilityData* FindAbility(FName AbilityId);

    /** True when the id resolves to a registered ability. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Ability")
    static bool IsKnownAbility(FName AbilityId);

    /** Number of registered ability templates (44). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Ability")
    static int32 GetAbilityCount();

    /**
     * Ability ids an Echo of the given definition can ever learn: authored
     * AbilityIds first (deduped, order preserved), then the derived element/
     * role/family loadout for ids not already present.
     */
    static TArray<FName> GetAbilityIdsForSpecies(const UAstrawildEchoDefinition* Definition);

    /**
     * Deterministic derived loadout for a species that did not author one:
     * 2 element-flavored abilities + 1 role ability + 1 family signature.
     */
    static TArray<FName> ComputeDerivedAbilityIds(EAstrawildElementType Element, EAstrawildEchoRole Role,
        EAstrawildEchoFamily Family);

    /**
     * Best ability to cast right now given what the Echo knows, what is ready,
     * and the tactical intent. Deterministic priority:
     *   bWantsHeal    -> Restore (only when a healer and the party is hurt)
     *   bWantsShield  -> Defensive (only when its own health is low)
     *   otherwise     -> Offensive > Debuff > Mobility (range-respecting).
     * Returns NAME_None when nothing is castable.
     */
    static FName ChooseAbilityForCombat(const TArray<FName>& KnownAbilityIds, const TMap<FName, float>& CooldownsRemaining,
        int32 EchoLevel, float DistanceToTarget, bool bWantsHeal, bool bWantsShield);

    /** Pure static integrity check used by the automation suite. */
    static void ValidateTable(TArray<FString>& OutProblems);
};
