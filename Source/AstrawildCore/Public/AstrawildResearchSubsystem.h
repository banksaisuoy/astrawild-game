#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AstrawildTypes.h"
#include "AstrawildResearchSubsystem.generated.h"

class UAstrawildTechnologyDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildTechUnlocked, FName, TechId, const UAstrawildTechnologyDefinition*, Definition);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildResearchPointsChanged, int32, NewPoints);

/**
 * Technology research (directive §19). Research points come from exploration and the
 * field journal; unlocks gate recipes and buildings. Game-instance scope: co-op
 * sessions share one research pool (documented co-op decision).
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildResearchSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Research")
    FAstrawildTechUnlocked OnTechUnlocked;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Research")
    FAstrawildResearchPointsChanged OnResearchPointsChanged;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Research")
    bool IsTechUnlocked(FName TechId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Research")
    int32 GetResearchPoints() const { return ResearchPoints; }

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Research")
    void AddResearchPoints(int32 Amount);

    /** Can the tech be unlocked right now (prereqs + points)? */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Research")
    bool CanUnlockTech(FName TechId) const;

    /** Prerequisites missing for a tech (empty when all satisfied). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Research")
    TArray<FName> GetMissingPrerequisites(FName TechId) const;

    /** Attempt to unlock; consumes research points on success. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Research")
    bool TryUnlockTech(FName TechId);

    /**
     * Batch 6: unlock without cost or prerequisites — the dungeon's unique
     * technology reward path (roadmap V3 §21). Broadcasts the same events as
     * TryUnlockTech so quests/journal/UI react normally. No-op when already
     * unlocked. Returns true when this call performed the unlock.
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Research")
    bool ForceUnlockTech(FName TechId);

    /**
     * Audit C-2: auto-grant every cost-0 root technology so crafting gates open from
     * session start (previously no legitimate path unlocked ANY tech — the tree was
     * cosmetic and quests 4-6 were impossible without cheats).
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Research")
    void GrantStartingTechnologies();

    /**
     * Audit C-2: cheapest currently unlockable tech (ties broken by registry order).
     * Returns NAME_None when nothing is unlockable; OutCost/OutName describe the pick.
     */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Research")
    FName GetNextUnlockableTechId(int32& OutCost, FText& OutDisplayName) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Research")
    TArray<FName> GetUnlockedTechIds() const;

    void ExportForSave(FAstrawildResearchSaveData& OutData) const;
    void ImportFromSave(const FAstrawildResearchSaveData& InData);

private:
    UPROPERTY()
    TArray<FName> UnlockedTechIds;

    UPROPERTY()
    int32 ResearchPoints = 0;

    class UAstrawildItemRegistrySubsystem* GetRegistryFromWorld() const;
};
