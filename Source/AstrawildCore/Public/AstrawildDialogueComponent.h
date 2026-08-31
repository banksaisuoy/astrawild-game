#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildDataAssets.h"
#include "AstrawildDialogueComponent.generated.h"

/**
 * Production V2 Batch 3 — per-player dialogue state (P12 Story/NPC).
 *
 * Owns the persistent story-flag store and the two pure operations the
 * conversation screen needs:
 *
 *   EvaluateChoiceConditions — filters the visible replies (quest state +
 *                              story flags; all conditions AND).
 *   ApplyChoiceConsequences  — routes through the SAME authority pipelines
 *                              dialogue must never bypass: StartQuest on the
 *                              quest component, server inventory adds,
 *                              research points through the research subsystem.
 *
 * Lives on the PlayerController next to UAstrawildQuestComponent so flags
 * survive death/respawn and persist in the save (TArray<FName> DialogueFlags).
 */
UCLASS(ClassGroup=(ASTRAWILD), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildDialogueComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildDialogueComponent();

    // --- Story flags (persistent) ---

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Dialogue")
    bool HasStoryFlag(FName FlagId) const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Dialogue")
    void SetStoryFlag(FName FlagId);

    /** All set story flags (HUD/journal/debug). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Dialogue")
    TArray<FName> GetStoryFlags() const;

    // --- Pure choice evaluation (automation-tested) ---

    /**
     * All conditions are AND. NAME_None conditions are ignored. QuestState is
     * resolved through the sibling quest component when the ids are set.
     */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Dialogue")
    bool EvaluateChoiceConditions(const FAstrawildDialogueChoice& Choice) const;

    // --- Consequences ---

    /**
     * Applies quest start, flag set, item grant and research points in a fixed
     * order. Returns false only when a consequence failed hard (unknown quest
     * id) — soft consequences (inventory full → items dropped to overflow) are
     * tolerated with a log so conversations never trap the player.
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Dialogue")
    bool ApplyChoiceConsequences(const FAstrawildDialogueChoice& Choice);

    // --- Save round-trip (schema v4 additive) ---

    void ExportForSave(TArray<FName>& OutFlags) const;
    void ImportFromSave(const TArray<FName>& InFlags);

private:
    class UAstrawildQuestComponent* GetQuestComponent() const;
    class UAstrawildItemRegistrySubsystem* GetRegistry() const;
    class AAstrawildPlayerCharacter* GetPlayerCharacter() const;

    /** Set story flags (idempotent set semantics). */
    UPROPERTY()
    TArray<FName> StoryFlags;
};
