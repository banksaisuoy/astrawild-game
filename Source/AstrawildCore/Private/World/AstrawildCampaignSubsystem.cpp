#include "World/AstrawildCampaignSubsystem.h"

#include "Engine/DataTable.h"
#include "Engine/World.h"

bool UAstrawildCampaignSubsystem::StartChapter(const FName ChapterId)
{
    if (!HasAuthorityForCampaign() || ChapterId.IsNone() || IsChapterCompleted(ChapterId) || ActiveChapterId != NAME_None)
    {
        return false;
    }
    const FAstrawildCampaignChapterRow* Chapter = FindChapter(ChapterId);
    if (!Chapter)
    {
        return false;
    }
    ActiveChapterId = ChapterId;
    OnChapterStarted.Broadcast(ChapterId);
    return true;
}

bool UAstrawildCampaignSubsystem::CompleteChapter(const FName ChapterId, const TArray<FName>& CompletedQuestIds, const bool bRequiredBossDefeated)
{
    if (!HasAuthorityForCampaign() || IsChapterCompleted(ChapterId))
    {
        return false;
    }
    const FAstrawildCampaignChapterRow* Chapter = FindChapter(ChapterId);
    if (!Chapter || (ActiveChapterId != NAME_None && ActiveChapterId != ChapterId))
    {
        return false;
    }
    for (const FName RequiredQuestId : Chapter->RequiredQuestIds)
    {
        if (!CompletedQuestIds.Contains(RequiredQuestId))
        {
            return false;
        }
    }
    if (Chapter->RequiredBossEncounterTag.IsValid() && !bRequiredBossDefeated)
    {
        return false;
    }
    CompletedChapterIds.AddUnique(ChapterId);
    if (ActiveChapterId == ChapterId)
    {
        ActiveChapterId = NAME_None;
    }
    OnChapterCompleted.Broadcast(ChapterId);
    return true;
}

bool UAstrawildCampaignSubsystem::ChooseEnding(const FGameplayTag EndingTag)
{
    if (!HasAuthorityForCampaign() || !EndingTag.IsValid() || !IsCampaignComplete())
    {
        return false;
    }
    if (CampaignTable)
    {
        TArray<FAstrawildCampaignChapterRow*> rows;
        CampaignTable->GetAllRows<FAstrawildCampaignChapterRow>(TEXT("AstrawildEndingLookup"), rows);
        for (const FAstrawildCampaignChapterRow* row : rows)
        {
            if (row && row->bIsFinalChapter && row->EndingChoiceTags.Contains(EndingTag))
            {
                ChosenEndingTags.AddUnique(EndingTag);
                return true;
            }
        }
    }
    return false;
}

bool UAstrawildCampaignSubsystem::GetChapterData(const FName ChapterId, FAstrawildCampaignChapterRow& OutChapter) const
{
    const FAstrawildCampaignChapterRow* Chapter = FindChapter(ChapterId);
    if (!Chapter)
    {
        OutChapter = FAstrawildCampaignChapterRow();
        return false;
    }
    OutChapter = *Chapter;
    return true;
}

bool UAstrawildCampaignSubsystem::IsChapterCompleted(const FName ChapterId) const
{
    return CompletedChapterIds.Contains(ChapterId);
}

bool UAstrawildCampaignSubsystem::IsCampaignComplete() const
{
    if (!CampaignTable)
    {
        return false;
    }
    TArray<FAstrawildCampaignChapterRow*> rows;
    CampaignTable->GetAllRows<FAstrawildCampaignChapterRow>(TEXT("AstrawildCampaignComplete"), rows);
    if (rows.Num() == 0)
    {
        return false;
    }
    for (const FAstrawildCampaignChapterRow* row : rows)
    {
        if (row && !CompletedChapterIds.Contains(row->ChapterId))
        {
            return false;
        }
    }
    return true;
}

const FAstrawildCampaignChapterRow* UAstrawildCampaignSubsystem::FindChapter(const FName ChapterId) const
{
    if (!CampaignTable || ChapterId.IsNone())
    {
        return nullptr;
    }
    TArray<FAstrawildCampaignChapterRow*> rows;
    CampaignTable->GetAllRows<FAstrawildCampaignChapterRow>(TEXT("AstrawildCampaignLookup"), rows);
    FAstrawildCampaignChapterRow** Found = rows.FindByPredicate([ChapterId](const FAstrawildCampaignChapterRow* row)
    {
        return row && row->ChapterId == ChapterId;
    });
    return Found ? *Found : nullptr;
}

bool UAstrawildCampaignSubsystem::HasAuthorityForCampaign() const
{
    return !GetWorld() || GetWorld()->GetNetMode() != NM_Client;
}
