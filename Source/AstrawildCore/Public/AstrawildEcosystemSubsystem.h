#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstrawildEcosystemSubsystem.generated.h"

class AAstrawildEchoCharacter;

/** Simulation fidelity tiers (directive §34). */
UENUM(BlueprintType)
enum class EAstrawildSimulationTier : uint8
{
    /** Player vicinity — full AI, movement, needs, combat. */
    Tier0_Full UMETA(DisplayName="Tier 0 — Full"),
    /** Nearby — reduced-rate AI updates, needs continue, movement simplified. */
    Tier1_Reduced UMETA(DisplayName="Tier 1 — Reduced"),
    /** Far — statistical only: needs decay, population bookkeeping, no movement. */
    Tier2_Statistical UMETA(DisplayName="Tier 2 — Statistical"),
    /** Very far / despawned — world-level bookkeeping only. */
    Tier3_World UMETA(DisplayName="Tier 3 — World")
};

/** Population record for abstract world-level simulation (directive §7). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildSpeciesPopulation
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Ecosystem")
    FName DefinitionId = NAME_None;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Ecosystem", meta=(ClampMin="0"))
    int32 WildCount = 0;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Ecosystem", meta=(ClampMin="0"))
    int32 CapturedCount = 0;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Ecosystem", meta=(ClampMin="0"))
    int32 DefeatedCount = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildPopulationChanged, FName, DefinitionId, int32, NewWildCount);

/**
 * World ecosystem + simulation LOD manager (directive §7/§34).
 * Echoes register on spawn and unregister on death/capture. The subsystem assigns each
 * Echo a simulation tier based on distance to the nearest player and exposes the
 * recommended update interval. Tier assignment runs on a throttled cadence — never per frame.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildEcosystemSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    UAstrawildEcosystemSubsystem();

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Ecosystem")
    FAstrawildPopulationChanged OnPopulationChanged;

    /** Tier0 distance (cm) — full simulation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Ecosystem|LOD")
    float Tier0Distance = 3000.0f;

    /** Tier1 distance (cm) — reduced simulation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Ecosystem|LOD")
    float Tier1Distance = 8000.0f;

    /** Tier2 distance (cm) — statistical simulation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Ecosystem|LOD")
    float Tier2Distance = 20000.0f;

    /** Seconds between tier re-evaluation sweeps. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Ecosystem|LOD", meta=(ClampMin="0.25"))
    float TierUpdateIntervalSeconds = 1.0f;

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

    void RegisterEcho(AAstrawildEchoCharacter* Echo);
    void UnregisterEcho(AAstrawildEchoCharacter* Echo);

    /** Current simulation tier for an Echo (Tier0 by default). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Ecosystem")
    EAstrawildSimulationTier GetTierForEcho(const AAstrawildEchoCharacter* Echo) const;

    /** Recommended AI update interval (seconds) for a tier. */
    static float GetRecommendedUpdateInterval(EAstrawildSimulationTier Tier);

    /** Wild population of one species. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Ecosystem")
    int32 GetWildPopulation(FName DefinitionId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Ecosystem")
    TArray<FAstrawildSpeciesPopulation> GetPopulations() const;

    /** Count a capture (removes from wild pool, adds to captured). */
    void NotifyCaptured(FName DefinitionId);

    /** Count a defeat. */
    void NotifyDefeated(FName DefinitionId);

    // --- Food chain (directive §7): predator species hunt prey species. ---

    /** Declare that PredatorId preys on PreyId. Idempotent. */
    void AddPredatorPair(FName PredatorId, FName PreyId);

    /** Is the species a predator of anything? */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Ecosystem")
    bool IsPredator(FName SpeciesId) const;

    /** Does Predator hunt Prey? */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Ecosystem")
    bool IsPreyOf(FName PredatorId, FName PreyId) const;

    /** Preferred prey species ids for a predator (empty when herbivore). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Ecosystem")
    TArray<FName> GetPreySpecies(FName PredatorId) const;

    // --- Herd behavior (directive §7): social species group together. ---

    /** Declare a species as herding (Social personalities seek their own kind). */
    void MarkHerdingSpecies(FName SpeciesId);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Ecosystem")
    bool IsHerdingSpecies(FName SpeciesId) const;

    /** Nearest herd anchor (a same-species Echo) for cohesion movement, or nullptr. */
    AAstrawildEchoCharacter* FindHerdAnchor(const AAstrawildEchoCharacter* Echo, float MaxDistance) const;

    /** Nearest visible prey Echo for a predator within MaxDistance, or nullptr. */
    AAstrawildEchoCharacter* FindPreyFor(const AAstrawildEchoCharacter* Predator, float MaxDistance) const;

protected:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
    /** Weak pointers — actors may die without unregistering; sweep clears stale entries. */
    TArray<TWeakObjectPtr<AAstrawildEchoCharacter>> RegisteredEchoes;
    TMap<FName, FAstrawildSpeciesPopulation> Populations;
    TMap<FObjectKey, EAstrawildSimulationTier> EchoTiers;
    float TierSweepAccumulator = 0.0f;

    /** Predator -> prey species ids (directive §7 food chain). */
    TMap<FName, TArray<FName>> PredatorChains;

    /** Species ids that move in herds. */
    TSet<FName> HerdingSpecies;

    /** Register the default Dawn Fields food chain (directive §21 content). */
    void BuildDefaultFoodChain();

    void RunTierSweep();
    float FindNearestPlayerDistance(const FVector& Location) const;
    static EAstrawildSimulationTier DistanceToTier(float Distance, float Tier0, float Tier1, float Tier2);
};
