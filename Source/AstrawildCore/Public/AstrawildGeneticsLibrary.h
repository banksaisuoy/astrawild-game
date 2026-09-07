#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AstrawildGeneticsLibrary.generated.h"

/**
 * SCP Phase 10 — breeding genetics (directive [3] Phase 10.3).
 *
 * Pure, deterministic library (FRandomStream-seeded) rolled when an egg
 * hatches: four passive-trait slots drawn from the parent pools plus the
 * wild pool, and hidden IVs (0-31 per stat) that bias the offspring's growth.
 * All effects are consumed by the echo stat/work paths on spawn.
 */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildGeneticsProfile
{
    GENERATED_BODY()

    /** Four passive trait ids (may repeat only via inheritance rules). */
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Genetics")
    TArray<FName> Traits;

    /** Hidden IVs: Health / Attack / Defense / Speed (0-31 each). */
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Genetics")
    FVector4 IVs = FVector4(0.0f, 0.0f, 0.0f, 0.0f);

    bool IsValid() const { return Traits.Num() == 4; }
};

UCLASS()
class ASTRAWILDCORE_API UAstrawildGeneticsLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Full passive trait vocabulary (directive Phase 10.3 + healthy pool). */
    static const TArray<FName>& GetTraitPool();

    /** Trait effect contracts: Swift +30% speed, Artisan +50% work, Ferocious +20% attack, Sturdy +20% HP, Lucky +10% capture. */
    static float GetTraitSpeedMultiplier(FName TraitId);
    static float GetTraitWorkMultiplier(FName TraitId);
    static float GetTraitAttackMultiplier(FName TraitId);
    static float GetTraitHealthMultiplier(FName TraitId);
    static float GetTraitCaptureBonus(FName TraitId);

    /** Combined multipliers for a trait set (e.g. 2x Swift = 1.3 * 1.3). */
    static float ComputeTraitSpeedMultiplier(const TArray<FName>& Traits);
    static float ComputeTraitWorkMultiplier(const TArray<FName>& Traits);
    static float ComputeTraitAttackMultiplier(const TArray<FName>& Traits);
    static float ComputeTraitHealthMultiplier(const TArray<FName>& Traits);

    /**
     * Roll an offspring profile: each of the 4 slots inherits from a random
     * parent (70%) or mutates into the wild pool (30%); IVs average the
     * parents ± a 0-8 mutation swing. Deterministic per seed.
     */
    static FAstrawildGeneticsProfile RollOffspring(const TArray<FName>& ParentATraits,
        const TArray<FName>& ParentBTraits, int32 Seed);

    /** IV stat contribution: +1% per IV point (0..31 -> 0..31%). */
    static float ComputeIVStatMultiplier(float IV);
};
