#include "AstrawildHuntSubsystem.h"

#include "AstrawildCore.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildLog.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

namespace
{
    // PCR-5: the repeatable cull-contract table — every row reuses an EXISTING
    // species and an EXISTING reward item (census gates unchanged by design).
    // Targets = Tier-B zone signature species (the visually upgraded roster),
    // rewards scale with the required count. The loop feeds the existing
    // economy: hunt -> loot -> craft -> upgrade.
    const UAstrawildHuntSubsystem::FHuntContract* GetContractTable()
    {
        static const TArray<UAstrawildHuntSubsystem::FHuntContract> Table =
        {
            { TEXT("Hunt_DuskmothCull"),   TEXT("Echo_Duskmoth"),   5, TEXT("Item_DawnShard"),    3 },
            { TEXT("Hunt_StonehideCull"),  TEXT("Echo_Stonehide"),  4, TEXT("Item_Stone"),        6 },
            { TEXT("Hunt_EmberfangCull"),  TEXT("Echo_Emberfang"),  3, TEXT("Item_EmberAsh"),     5 },
            { TEXT("Hunt_RimefangCull"),   TEXT("Echo_Rimefang"),   3, TEXT("Item_DawnShard"),   2 },
            { TEXT("Hunt_BrinefinCull"),   TEXT("Echo_Brinefin"),   5, TEXT("Item_RawMeat"),      6 },
            { TEXT("Hunt_SunhideCull"),    TEXT("Echo_Sunhide"),    5, TEXT("Item_DuneGlass"),    4 },
            { TEXT("Hunt_VerdantbloomCull"), TEXT("Echo_Verdantbloom"), 4, TEXT("Item_Fiber"),    8 },
            { TEXT("Hunt_MonolithCull"),   TEXT("Echo_Vespermonolith"), 1, TEXT("Item_StormSilver"), 2 },
        };
        return Table.GetData();
    }

    constexpr int32 ContractTableSize = 8;
}

UAstrawildHuntSubsystem::UAstrawildHuntSubsystem()
{
}

bool UAstrawildHuntSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UAstrawildHuntSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UAstrawildHuntSubsystem, STATGROUP_Tickables);
}

void UAstrawildHuntSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
}

void UAstrawildHuntSubsystem::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Wire the event-bus observer once (server-side only — the same authority
    // rule the quest defeat counters use; clients receive state through the
    // save/claim routing, never by counting locally).
    if (!bEventWired)
    {
        UWorld* World = GetWorld();
        if (World && World->GetNetMode() != NM_Client)
        {
            if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
            {
                EventBus->OnGameplayEvent.AddDynamic(this, &UAstrawildHuntSubsystem::HandleGameplayEvent);
                bEventWired = true;
            }
        }
    }
}

TArray<FName> UAstrawildHuntSubsystem::GetHuntIds() const
{
    TArray<FName> Out;
    for (int32 Index = 0; Index < ContractTableSize; ++Index)
    {
        Out.Add(GetContractTable()[Index].HuntId);
    }
    return Out;
}

int32 UAstrawildHuntSubsystem::GetHuntRequiredDefeats(const FName HuntId) const
{
    FHuntContract Contract;
    return FindContract(HuntId, Contract) ? Contract.RequiredDefeats : 0;
}

FName UAstrawildHuntSubsystem::GetHuntSpeciesId(const FName HuntId) const
{
    FHuntContract Contract;
    return FindContract(HuntId, Contract) ? Contract.SpeciesId : NAME_None;
}

FName UAstrawildHuntSubsystem::GetHuntRewardItemId(const FName HuntId) const
{
    FHuntContract Contract;
    return FindContract(HuntId, Contract) ? Contract.RewardItemId : NAME_None;
}

int32 UAstrawildHuntSubsystem::GetHuntRewardQuantity(const FName HuntId) const
{
    FHuntContract Contract;
    return FindContract(HuntId, Contract) ? Contract.RewardQuantity : 0;
}

bool UAstrawildHuntSubsystem::FindContract(const FName HuntId, FHuntContract& OutContract)
{
    for (int32 Index = 0; Index < ContractTableSize; ++Index)
    {
        if (GetContractTable()[Index].HuntId == HuntId)
        {
            OutContract = GetContractTable()[Index];
            return true;
        }
    }
    return false;
}

int32 UAstrawildHuntSubsystem::GetHuntProgress(const FName HuntId, const FName PlayerKey) const
{
    const TMap<FName, int32>* PlayerProgress = HuntProgress.Find(PlayerKey);
    const int32* Count = PlayerProgress ? PlayerProgress->Find(HuntId) : nullptr;
    return Count ? *Count : 0;
}

bool UAstrawildHuntSubsystem::IsHuntComplete(const FName HuntId, const FName PlayerKey) const
{
    FHuntContract Contract;
    return FindContract(HuntId, Contract)
        && GetHuntProgress(HuntId, PlayerKey) >= Contract.RequiredDefeats;
}

void UAstrawildHuntSubsystem::HandleGameplayEvent(const FAstrawildGameplayEvent& Event)
{
    // Same defeat events the quest lifetime counters observe (final-audit G-3).
    if (Event.EventTag != TAG_Astrawild_Event_HostileDefeated
        && Event.EventTag != TAG_Astrawild_Event_EchoDefeated)
    {
        return;
    }
    if (Event.TargetId.IsNone() || Event.Amount <= 0)
    {
        return;
    }

    // Count for EVERY player's round (world-shared counting — the documented
    // co-op v1 exception class; LAN_COOP_SPEC §3): the party culls together.
    for (TPair<FName, TMap<FName, int32>>& PlayerPair : HuntProgress)
    {
        for (int32 Index = 0; Index < ContractTableSize; ++Index)
        {
            const FHuntContract& Contract = GetContractTable()[Index];
            if (Contract.SpeciesId != Event.TargetId)
            {
                continue;
            }
            int32* Count = PlayerPair.Value.Find(Contract.HuntId);
            if (Count && *Count < Contract.RequiredDefeats)
            {
                // Only rounds IN PROGRESS accumulate; claimed/completed rounds
                // stay at their cap until claimed (claim resets to 0).
                *Count = FMath::Min(Contract.RequiredDefeats, *Count + FMath::Max(1, Event.Amount));
            }
        }
    }
}

bool UAstrawildHuntSubsystem::ClaimHunt(const FName HuntId, const FName PlayerKey, APawn* RewardRecipient)
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return false; // server-authoritative only
    }

    FHuntContract Contract;
    if (!FindContract(HuntId, Contract) || !IsHuntComplete(HuntId, PlayerKey))
    {
        return false;
    }

    // Reward lands silently (AddItemSilent — no false quest/collect credit).
    UAstrawildInventoryComponent* Inventory = RewardRecipient
        ? RewardRecipient->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr;
    if (!Inventory || !Inventory->AddItemSilent(Contract.RewardItemId, Contract.RewardQuantity))
    {
        return false;
    }

    // Reset the round — repeatable forever.
    if (TMap<FName, int32>* PlayerProgress = HuntProgress.Find(PlayerKey))
    {
        PlayerProgress->Remove(HuntId);
    }

    UE_LOG(LogAstrawild, Log, TEXT("PCR-5: hunt %s claimed by %s (%s x%d)."),
        *HuntId.ToString(), *PlayerKey.ToString(),
        *Contract.RewardItemId.ToString(), Contract.RewardQuantity);
    return true;
}

void UAstrawildHuntSubsystem::ExportForSave(TArray<FAstrawildHuntSaveRow>& OutRows) const
{
    OutRows.Reset();
    for (const TPair<FName, TMap<FName, int32>>& PlayerPair : HuntProgress)
    {
        for (const TPair<FName, int32>& HuntPair : PlayerPair.Value)
        {
            if (HuntPair.Value > 0)
            {
                FAstrawildHuntSaveRow Row;
                Row.PlayerKey = PlayerPair.Key;
                Row.HuntId = HuntPair.Key;
                Row.Defeats = FMath::Clamp(HuntPair.Value, 0, 999);
                OutRows.Add(Row);
            }
        }
    }
}

void UAstrawildHuntSubsystem::ImportFromSave(const TArray<FAstrawildHuntSaveRow>& InRows)
{
    HuntProgress.Reset();
    for (const FAstrawildHuntSaveRow& Row : InRows)
    {
        FHuntContract Contract;
        if (!FindContract(Row.HuntId, Contract))
        {
            UE_LOG(LogAstrawild, Warning, TEXT("PCR-5: dropped unknown hunt id %s on import."), *Row.HuntId.ToString());
            continue; // fail-closed: unknown/crafted ids never enter progress
        }
        if (Row.Defeats <= 0)
        {
            continue;
        }
        HuntProgress.FindOrAdd(Row.PlayerKey).Add(Row.HuntId,
            FMath::Clamp(Row.Defeats, 0, Contract.RequiredDefeats));
    }
}
