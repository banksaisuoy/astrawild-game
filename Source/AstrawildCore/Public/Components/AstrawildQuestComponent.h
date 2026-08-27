#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildQuestComponent.generated.h"

class UDataTable;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestStateChangedSignature, FName, QuestId);

UCLASS(ClassGroup=(Astrawild), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildQuestComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildQuestComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Quest")
    TObjectPtr<UDataTable> QuestTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Quest")
    TObjectPtr<UDataTable> ObjectiveTable;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Quest")
    TArray<FName> ActiveQuestIds;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Quest")
    TArray<FName> CompletedQuestIds;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Quest")
    TMap<FName, int32> ObjectiveProgress;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Quest|Events")
    FOnQuestStateChangedSignature OnQuestStarted;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Quest|Events")
    FOnQuestStateChangedSignature OnQuestUpdated;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Quest|Events")
    FOnQuestStateChangedSignature OnQuestCompleted;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Quest")
    bool StartQuest(FName QuestId);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Quest")
    bool AddObjectiveProgress(FName QuestId, FName ObjectiveId, int32 Amount = 1);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Quest")
    bool CompleteQuest(FName QuestId);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Quest")
    bool IsQuestActive(FName QuestId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Quest")
    bool IsQuestCompleted(FName QuestId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Quest")
    bool IsQuestComplete(FName QuestId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Quest")
    int32 GetObjectiveProgress(FName QuestId, FName ObjectiveId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Quest")
    bool GetQuestData(FName QuestId, FAstrawildQuestRow& OutQuest) const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Save")
    void ExportToProfile(FAstrawildPlayerProfile& OutProfile) const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Save")
    void ImportFromProfile(const FAstrawildPlayerProfile& InProfile);

private:
    static FName MakeObjectiveKey(FName QuestId, FName ObjectiveId);
    const FAstrawildQuestRow* FindQuest(FName QuestId) const;
    const FAstrawildQuestObjectiveRow* FindObjective(FName QuestId, FName ObjectiveId) const;
};
