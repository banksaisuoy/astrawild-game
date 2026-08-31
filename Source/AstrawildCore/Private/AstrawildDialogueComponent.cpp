#include "AstrawildDialogueComponent.h"

#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "AstrawildQuestComponent.h"
#include "AstrawildResearchSubsystem.h"
#include "AstrawildInventoryComponent.h"
#include "GameFramework/PlayerController.h"

UAstrawildDialogueComponent::UAstrawildDialogueComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UAstrawildDialogueComponent::HasStoryFlag(const FName FlagId) const
{
    return StoryFlags.Contains(FlagId);
}

void UAstrawildDialogueComponent::SetStoryFlag(const FName FlagId)
{
    if (!FlagId.IsNone() && !StoryFlags.Contains(FlagId))
    {
        StoryFlags.Add(FlagId);
        UE_LOG(LogAstrawild, Log, TEXT("Story flag set: %s."), *FlagId.ToString());
    }
}

TArray<FName> UAstrawildDialogueComponent::GetStoryFlags() const
{
    return StoryFlags;
}

bool UAstrawildDialogueComponent::EvaluateChoiceConditions(const FAstrawildDialogueChoice& Choice) const
{
    // Quest conditions resolve through the sibling quest component; a missing
    // component can only fail conditions that reference quests (NAME_None
    // conditions stay ignored).
    if (!Choice.RequiredQuestActiveId.IsNone())
    {
        const UAstrawildQuestComponent* Quests = GetQuestComponent();
        if (!Quests || !Quests->IsQuestActive(Choice.RequiredQuestActiveId))
        {
            return false;
        }
    }

    if (!Choice.RequiredQuestCompletedId.IsNone())
    {
        const UAstrawildQuestComponent* Quests = GetQuestComponent();
        if (!Quests || !Quests->IsQuestCompleted(Choice.RequiredQuestCompletedId))
        {
            return false;
        }
    }

    if (!Choice.RequiredFlagId.IsNone() && !HasStoryFlag(Choice.RequiredFlagId))
    {
        return false;
    }

    if (!Choice.ForbiddenFlagId.IsNone() && HasStoryFlag(Choice.ForbiddenFlagId))
    {
        return false;
    }

    return true;
}

bool UAstrawildDialogueComponent::ApplyChoiceConsequences(const FAstrawildDialogueChoice& Choice)
{
    bool bAllApplied = true;

    // 1) Quest start — hard fail on unknown id (the tree references content
    // that does not exist; surface it instead of silently swallowing).
    if (!Choice.StartQuestId.IsNone())
    {
        const UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
        if (Registry && Registry->FindQuest(Choice.StartQuestId))
        {
            if (UAstrawildQuestComponent* Quests = GetQuestComponent())
            {
                Quests->StartQuest(Choice.StartQuestId);
            }
            else
            {
                bAllApplied = false;
                UE_LOG(LogAstrawild, Warning, TEXT("Dialogue consequence: no quest component for %s."), *Choice.StartQuestId.ToString());
            }
        }
        else
        {
            bAllApplied = false;
            UE_LOG(LogAstrawild, Warning, TEXT("Dialogue consequence: quest %s is not registered."), *Choice.StartQuestId.ToString());
        }
    }

    // 2) Story flag.
    if (!Choice.SetFlagId.IsNone())
    {
        SetStoryFlag(Choice.SetFlagId);
    }

    // 3) Item grant — soft failure (inventory weight limits must never trap
    // the player mid-conversation; AddItem reports overflow through its own
    // channels and the conversation continues).
    if (!Choice.GiveItemId.IsNone() && Choice.GiveItemQuantity > 0)
    {
        AAstrawildPlayerCharacter* Player = GetPlayerCharacter();
        if (Player && Player->InventoryComponent)
        {
            Player->InventoryComponent->AddItem(Choice.GiveItemId, Choice.GiveItemQuantity);
        }
        else
        {
            UE_LOG(LogAstrawild, Warning, TEXT("Dialogue consequence: item grant skipped (no player/inventory)."));
        }
    }

    // 4) Research points — through the same subsystem quests use.
    if (Choice.GiveResearchPoints > 0)
    {
        const UWorld* World = GetWorld();
        if (World && World->GetGameInstance())
        {
            if (UAstrawildResearchSubsystem* Research = World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>())
            {
                Research->AddResearchPoints(Choice.GiveResearchPoints);
            }
        }
    }

    return bAllApplied;
}

void UAstrawildDialogueComponent::ExportForSave(TArray<FName>& OutFlags) const
{
    OutFlags = StoryFlags;
}

void UAstrawildDialogueComponent::ImportFromSave(const TArray<FName>& InFlags)
{
    StoryFlags = InFlags;
}

UAstrawildQuestComponent* UAstrawildDialogueComponent::GetQuestComponent() const
{
    const AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(GetOwner());
    return PC ? PC->QuestComponent : nullptr;
}

UAstrawildItemRegistrySubsystem* UAstrawildDialogueComponent::GetRegistry() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
}

AAstrawildPlayerCharacter* UAstrawildDialogueComponent::GetPlayerCharacter() const
{
    const APlayerController* PC = Cast<APlayerController>(GetOwner());
    return PC ? Cast<AAstrawildPlayerCharacter>(PC->GetPawn()) : nullptr;
}
