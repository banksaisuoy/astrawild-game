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

private:
    UPROPERTY()
    TArray<FAstrawildQuestSaveData> QuestStates;

    UPROPERTY()
    FName ActiveQuestId = NAME_None;

    UPROPERTY()
    TArray<FName> CompletedQuestIds;

    UFUNCTION()
    void HandleGameplayEvent(const FAstrawildGameplayEvent& Event);

    /** FR-3 (Final Run redo): re-entrancy guard for CompleteQuest (see its comment). */
    bool bBusyCompletingQuest = false;

    void ApplyEventToQuest(const FAstrawildGameplayEvent& Event);
    void CompleteQuest(FName QuestId);
    void GrantRewards(const UAstrawildQuestDefinition* Definition);

    /** Final production run: SurviveTime objectives accrue real seconds while the owner is alive. */
    void TickSurviveTimeObjectives(float DeltaTime);

    class UAstrawildItemRegistrySubsystem* GetRegistry() const;
    class UAstrawildEventBusSubsystem* GetEventBus() const;
    class AAstrawildPlayerCharacter* GetPlayerCharacter() const;
};
