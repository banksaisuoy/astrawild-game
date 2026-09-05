#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildQuestComponent.generated.h"

class UAstrawildQuestDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildQuestStateChanged, FName, QuestId, bool, bCompleted);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FAstrawildObjectiveProgress, FName, QuestId, int32, ObjectiveIndex, int32, Progress, int32, Required);

/**
 * Per-player quest progression (directive §25). Fully event-driven: gameplay systems
 * publish events on the bus; this component translates them into objective progress.
 * Lives on the PlayerController so quest state survives death/respawn.
 */
UCLASS(ClassGroup=(ASTRAWILD), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildQuestComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildQuestComponent();

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Quest")
    FAstrawildQuestStateChanged OnQuestStateChanged;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Quest")
    FAstrawildObjectiveProgress OnObjectiveProgress;

    /** Quest auto-activated on first play (directive §21 first story beat). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Quest")
    FName StartingQuestId = TEXT("Quest_FirstLight");

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override; // LCP-5

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Quest")
    bool StartQuest(FName QuestId);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Quest")
    bool IsQuestActive(FName QuestId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Quest")
    bool IsQuestCompleted(FName QuestId) const;

    /** Active quest objectives for HUD. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Quest")
    TArray<FAstrawildQuestObjective> GetActiveObjectives() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Quest")
    FName GetActiveQuestId() const;

    void ExportForSave(TArray<FAstrawildQuestSaveData>& OutQuests) const;
    void ImportFromSave(const TArray<FAstrawildQuestSaveData>& InQuests);

    /**
     * Final-audit G-3: lifetime defeat counters (species/boss id -> kills observed
     * on the event bus). One-shot bosses never respawn, so a defeat that lands
     * BEFORE its quest activates would otherwise dead-end that objective forever.
     * Exported/imported beside the quest states (additive v5 save field).
     */
    void ExportDefeatCounts(TMap<FName, int32>& OutCounts) const { OutCounts = DefeatedCreatureCounts; }
    void ImportDefeatCounts(const TMap<FName, int32>& InCounts);

    /**
     * Final-audit G-1/G-3: back-fill one-shot objectives (DiscoverPOI, DefeatCreature)
     * from world history so POIs discovered or bosses defeated BEFORE a quest
     * activated can never dead-end the chain. Pure static (world-free testable):
     * the caller gathers the discovered-POI set and defeat counters.
     * @return number of objectives that received back-filled progress.
     */
    static int32 BackFillOneShotObjectives(FAstrawildQuestSaveData& State,
        const TMap<FName, int32>& DefeatedCreatureCounts,
        const TSet<FName>& DiscoveredPoiIds);

    /**
     * Final-audit F-03 test hook: ApplyEventToQuest is the objective matcher
     * itself. Exposed so world-free automation tests can drive real events
     * (e.g. proving a dismantle's negative Amount never advances PlaceBuilding).
     */
    void ApplyEventToQuest(const FAstrawildGameplayEvent& Event);

    /**
     * Final-audit F-06: public completion entry (used by AW.FastForward and
     * tests). Grants rewards exactly once, chains the next quest, and is
     * re-entrancy guarded — the same path live gameplay event completion takes.
     */
    void CompleteQuest(FName QuestId);

private:
    /**
     * LCP-5: quest state replicates to the OWNING client (the component lives
     * on the PlayerController, which replicates to its connection) so the HUD
     * tracker + screens read live progression. Mutations stay server-side —
     * the event bus only ticks there; clients receive data, never authority.
     */
    UPROPERTY(Replicated)
    TArray<FAstrawildQuestSaveData> QuestStates;

    UPROPERTY(Replicated)
    FName ActiveQuestId = NAME_None;

    UPROPERTY(Replicated)
    TArray<FName> CompletedQuestIds;

    UFUNCTION()
    void HandleGameplayEvent(const FAstrawildGameplayEvent& Event);

    /** FR-3 (Final Run redo): re-entrancy guard for CompleteQuest (see its comment). */
    bool bBusyCompletingQuest = false;

    /** Lifetime defeat counters — see ExportDefeatCounts (final-audit G-3). */
    UPROPERTY()
    TMap<FName, int32> DefeatedCreatureCounts;

    void GrantRewards(const UAstrawildQuestDefinition* Definition);

    /** Final production run: SurviveTime objectives accrue real seconds while the owner is alive. */
    void TickSurviveTimeObjectives(float DeltaTime);

    /** Gathers live world history (discovered POIs) + defeat counters and applies the static back-fill. */
    void BackFillOneShotObjectivesFromWorld(FAstrawildQuestSaveData& State);

    class UAstrawildItemRegistrySubsystem* GetRegistry() const;
    class UAstrawildEventBusSubsystem* GetEventBus() const;
    class AAstrawildPlayerCharacter* GetPlayerCharacter() const;
};
