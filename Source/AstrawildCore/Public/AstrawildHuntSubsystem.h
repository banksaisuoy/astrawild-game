#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstrawildTypes.h"
#include "AstrawildHuntSubsystem.generated.h"

class UAstrawildEventBusSubsystem;

/**
 * PCR-5 (PG-5 gap closed): the post-game hunt system — REPEATABLE cull
 * contracts that give the post-ending world directed creature activity.
 *
 * Every contract reuses existing species + existing reward items (census
 * counts UNCHANGED by design). Progress observes the SAME defeat events the
 * quest system counts (Event.HostileDefeated / Event.EchoDefeated via the
 * event bus — server-side only), so hunts progress in single player AND co-op
 * (world-shared counting, matching the documented co-op v1 exceptions).
 *
 * Loop: open the Hunt Board [U] → pick a contract → cull the target species
 * anywhere → return and CLAIM the reward → the counter resets for another
 * round (repeatable forever; post-game purpose that feeds the existing
 * economy: loot → craft → upgrade).
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildHuntSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    /** One repeatable cull contract (pure data — automation-pinnable). */
    struct FHuntContract
    {
        FName HuntId;
        FName SpeciesId;
        int32 RequiredDefeats;
        FName RewardItemId;
        int32 RewardQuantity;
    };

    UAstrawildHuntSubsystem();

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    /** The full contract table (world-free testable — mirrors the cpp table). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Hunt")
    TArray<FName> GetHuntIds() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Hunt")
    int32 GetHuntRequiredDefeats(FName HuntId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Hunt")
    FName GetHuntSpeciesId(FName HuntId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Hunt")
    FName GetHuntRewardItemId(FName HuntId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Hunt")
    int32 GetHuntRewardQuantity(FName HuntId) const;

    /** Pure contract-table lookup (world-free — the automation contract pins it). */
    static bool FindContract(const FName HuntId, FHuntContract& OutContract);

    /**
     * This player's current defeat count toward a contract round.
     * NAME_None key = single-player row.
     */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Hunt")
    int32 GetHuntProgress(FName HuntId, FName PlayerKey) const;

    /** True when the contract round is complete and claimable. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Hunt")
    bool IsHuntComplete(FName HuntId, FName PlayerKey) const;

    /**
     * Claim the completed round: reward lands in the player's inventory via
     * AddItemSilent (no false quest credit) and the counter resets for the
     * next round. Server-authoritative only. Returns false when incomplete,
     * off-authority, or the inventory refuses the reward.
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Hunt")
    bool ClaimHunt(FName HuntId, FName PlayerKey, APawn* RewardRecipient);

    /** Save plumbing (additive v5 payload rows — no schema bump). */
    void ExportForSave(TArray<FAstrawildHuntSaveRow>& OutRows) const;
    void ImportFromSave(const TArray<FAstrawildHuntSaveRow>& InRows);

private:
    /** EventBus observer — counts target-species defeats per contract. */
    UFUNCTION()
    void HandleGameplayEvent(const FAstrawildGameplayEvent& Event);

    /** Defeat counters: PlayerKey -> HuntId -> defeats this round. */
    TMap<FName, TMap<FName, int32>> HuntProgress;

    bool bEventWired = false;
};
