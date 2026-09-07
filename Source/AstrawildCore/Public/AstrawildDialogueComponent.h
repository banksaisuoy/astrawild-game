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
 *                              story flags + the DP-8 affinity gate; all
 *                              conditions AND).
 *   ApplyChoiceConsequences  — routes through the SAME authority pipelines
 *                              dialogue must never bypass: StartQuest on the
 *                              quest component, server inventory adds,
 *                              research points through the research subsystem.
 *
 * Lives on the PlayerController next to UAstrawildQuestComponent so flags
 * survive death/respawn and persist in the save (TArray<FName> DialogueFlags).
 *
 * DP-8 (NPC depth): the component also tracks which NPC the player is talking
 * to (set/cleared by OpenDialogue/CloseDialogue — transient, never saved) so
 * the affinity gate can read the LIVE relationship while a conversation is
 * open. The gate logic itself is a pure static (automation-tested).
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
     * All conditions are AND. NAME_None conditions are ignored; a
     * RequiredMinAffinity of 0 is ignored. QuestState is resolved through the
     * sibling quest component when the ids are set; the affinity gate is
     * resolved against the talking NPC (fail-closed when it cannot be
     * resolved — a gate is a condition, not optional flavor).
     */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Dialogue")
    bool EvaluateChoiceConditions(const FAstrawildDialogueChoice& Choice) const;

    /**
     * DP-8 (NPC depth): pure affinity-gate resolver (automation-tested).
     * Threshold <= 0 never gates (pre-DP-8 trees stay byte-identical);
     * otherwise the talking NPC's affinity must reach the threshold. Tier
     * boundaries: 25 Acquaintance / 50 Friend / 75 Confidant.
     */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Dialogue")
    static bool MeetsAffinityGate(int32 RequiredMinAffinity, float NpcAffinity);

    /**
     * LCP-3 (world-free testable): structural validation of a remotely
     * submitted dialogue choice. Resolves the node inside the tree and returns
     * the choice at the index — nullptr on ANY mismatch (unknown tree/node,
     * out-of-range index). The server RPC re-derives everything from registry
     * truth; a modified client can never reach a hidden choice.
     */
    static const FAstrawildDialogueChoice* ResolveValidatedChoice(
        const class UAstrawildDialogueTreeDefinition* Tree, FName NodeId, int32 ChoiceIndex);

    // --- Talking-NPC tracking (DP-8 affinity gate source) ---

    /** Set while a conversation screen is open (OpenDialogue); cleared on close. */
    void SetTalkingNpc(class AAstrawildNPCCharacter* Npc);

    /** The NPC the player is currently talking to (nullptr outside conversations). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Dialogue")
    class AAstrawildNPCCharacter* GetTalkingNpc() const;

    // --- Consequences ---

    /**
     * Applies quest start, flag set, item grant and research points in a fixed
     * order. Returns false only when a consequence failed hard (unknown quest
     * id) — soft consequences (inventory full → items dropped to overflow) are
     * tolerated with a log so conversations never trap the player.
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Dialogue")
    bool ApplyChoiceConsequences(const FAstrawildDialogueChoice& Choice);

    /**
     * Final Run (FR-6): pure ending-id resolver (automation-tested closed
     * vocabulary). Ending_BreakCage → The Dawn That Stays;
     * Ending_StormSleeps → The Storm That Sleeps; anything else → None.
     */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Dialogue")
    static EAstrawildEndingState ResolveEndingForTriggerId(FName TriggerEndingId);

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

    /** DP-8: the NPC behind the open conversation screen (transient — never saved). */
    TWeakObjectPtr<class AAstrawildNPCCharacter> TalkingNpc;
};
