#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/AstrawildCampaignData.h"
#include "AstrawildCampaignSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAstrawildCampaignChapterStartedSignature, FName, ChapterId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAstrawildCampaignChapterCompletedSignature, FName, ChapterId);

UCLASS()
class ASTRAWILDCORE_API UAstrawildCampaignSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Campaign")
    TObjectPtr<UDataTable> CampaignTable;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Campaign")
    FName ActiveChapterId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Campaign")
    TArray<FName> CompletedChapterIds;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Campaign")
    TArray<FGameplayTag> ChosenEndingTags;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Campaign|Events")
    FOnAstrawildCampaignChapterStartedSignature OnChapterStarted;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Campaign|Events")
    FOnAstrawildCampaignChapterCompletedSignature OnChapterCompleted;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Campaign")
    bool StartChapter(FName ChapterId);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Campaign")
    bool CompleteChapter(FName ChapterId, const TArray<FName>& CompletedQuestIds, bool bRequiredBossDefeated);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Campaign")
    bool ChooseEnding(FGameplayTag EndingTag);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Campaign")
    bool GetChapterData(FName ChapterId, FAstrawildCampaignChapterRow& OutChapter) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Campaign")
    bool IsChapterCompleted(FName ChapterId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Campaign")
    bool IsCampaignComplete() const;

private:
    const FAstrawildCampaignChapterRow* FindChapter(FName ChapterId) const;
    bool HasAuthorityForCampaign() const;
};
