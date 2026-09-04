#include "AstrawildDialogueComponent.h"

#include "AstrawildGameState.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildNPCCharacter.h"
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

    // DP-8 (NPC depth): affinity gate — the TALKING NPC's live relationship
    // decides whether the reply exists, so conversations evolve as the player
    // earns trust (talk +2 / trade +1 per day). A threshold of 0 never gates
    // (every pre-DP-8 tree stays byte-identical); a positive threshold fails
    // closed exactly like the quest conditions above when the NPC cannot be
    // resolved — a gate is a condition, not optional flavor.
    if (Choice.RequiredMinAffinity > 0)
    {
        const AAstrawildNPCCharacter* Npc = TalkingNpc.Get();
        if (!Npc || !MeetsAffinityGate(Choice.RequiredMinAffinity, Npc->Affinity))
        {
            return false;
        }
    }

    return true;
}

bool UAstrawildDialogueComponent::MeetsAffinityGate(const int32 RequiredMinAffinity, const float NpcAffinity)
{
    // Pure contract (automation-tested): 0 = ungated (default), otherwise the
    // live affinity must REACH the threshold (>=). Authoring convention maps
    // tiers to thresholds: 25 Acquaintance / 50 Friend / 75 Confidant.
    if (RequiredMinAffinity <= 0)
    {
        return true;
    }
    return NpcAffinity >= static_cast<float>(RequiredMinAffinity);
}

void UAstrawildDialogueComponent::SetTalkingNpc(AAstrawildNPCCharacter* Npc)
{
    // Called by the player controller's OpenDialogue/CloseDialogue. Weak
    // reference: a mid-conversation NPC death simply fails the gate closed.
    TalkingNpc = Npc;
}

AAstrawildNPCCharacter* UAstrawildDialogueComponent::GetTalkingNpc() const
{
    return TalkingNpc.Get();
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

    // 5) Final Run (FR-6): ending route — the one-way world verdict flows through
    // the game state exactly like every other authority pipeline. Ids are a
    // closed vocabulary: Ending_BreakCage (The Dawn That Stays) and
    // Ending_StormSleeps (The Storm That Sleeps). Unknown ids fail closed.
    if (!Choice.TriggerEndingId.IsNone())
    {
        if (UWorld* World = GetWorld())
        {
            if (AAstrawildGameState* GameState = World->GetGameState<AAstrawildGameState>())
            {
                const EAstrawildEndingState Ending = ResolveEndingForTriggerId(Choice.TriggerEndingId);
                if (Ending != EAstrawildEndingState::None)
                {
                    GameState->SetEndingState(Ending);
                }
                else
                {
                    bAllApplied = false;
                    UE_LOG(LogAstrawild, Warning, TEXT("Dialogue consequence: unknown ending id %s."), *Choice.TriggerEndingId.ToString());
                }
            }
            else
            {
                bAllApplied = false;
                UE_LOG(LogAstrawild, Warning, TEXT("Dialogue consequence: no game state for ending %s."), *Choice.TriggerEndingId.ToString());
            }
        }
    }

    return bAllApplied;
}

EAstrawildEndingState UAstrawildDialogueComponent::ResolveEndingForTriggerId(const FName TriggerEndingId)
{
    // Closed vocabulary (FR-6): the two Act 3 endings, nothing else.
    if (TriggerEndingId == TEXT("Ending_BreakCage"))
    {
        return EAstrawildEndingState::TheDawnThatStays;
    }
    if (TriggerEndingId == TEXT("Ending_StormSleeps"))
    {
        return EAstrawildEndingState::TheStormThatSleeps;
    }
    return EAstrawildEndingState::None;
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
