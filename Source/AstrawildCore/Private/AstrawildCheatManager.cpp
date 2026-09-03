#include "AstrawildCheatManager.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildGameState.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildNPCCharacter.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "AstrawildQuestComponent.h"
#include "AstrawildResearchSubsystem.h"
#include "AstrawildSaveSubsystem.h"
#include "AstrawildSurvivalComponent.h"
#include "AstrawildTimeSubsystem.h"
#include "AstrawildWeatherSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

AAstrawildPlayerCharacter* UAstrawildCheatManager::GetPlayer() const
{
    const APlayerController* PC = Cast<APlayerController>(GetOuter());
    return PC ? Cast<AAstrawildPlayerCharacter>(PC->GetPawn()) : nullptr;
}

void UAstrawildCheatManager::SpawnEcho(const FName EchoDefinitionId)
{
    UWorld* World = GetWorld();
    AAstrawildPlayerCharacter* Player = GetPlayer();
    UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    UAstrawildEchoDefinition* Definition = Registry ? Registry->FindEcho(EchoDefinitionId) : nullptr;

    if (!World || !Player || !Definition)
    {
        UE_LOG(LogAstrawild, Warning, TEXT("AW.SpawnEcho: unknown Echo id %s."), *EchoDefinitionId.ToString());
        return;
    }

    const FVector Location = Player->GetActorLocation() + Player->GetActorForwardVector() * 400.0f;
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    if (AAstrawildEchoCharacter* Echo = World->SpawnActor<AAstrawildEchoCharacter>(AAstrawildEchoCharacter::StaticClass(), Location, FRotator::ZeroRotator, Params))
    {
        Echo->InitializeFromDefinition(Definition);
    }
}

void UAstrawildCheatManager::GiveItem(const FName ItemId, const int32 Quantity)
{
    AAstrawildPlayerCharacter* Player = GetPlayer();
    if (Player && Player->InventoryComponent)
    {
        Player->InventoryComponent->AddItem(ItemId, FMath::Max(1, Quantity));
    }
}

namespace
{
    /** Batch 4 — M-11: nearest vendor NPC within trade range of the player
     *  (the AW.BuyItem / AW.SellItem cheats route through the same
     *  server-authoritative API a future shop UMG screen will use). */
    AAstrawildNPCCharacter* FindNearestVendor(const AAstrawildPlayerCharacter* Player)
    {
        if (!Player)
        {
            return nullptr;
        }
        AAstrawildNPCCharacter* Best = nullptr;
        float BestDistSq = FMath::Square(600.0f);
        for (TActorIterator<AAstrawildNPCCharacter> It(Player->GetWorld()); It; ++It)
        {
            // REVIEW-4 (L-1): skip NPCs without a configured shop so the cheat
            // targets the nearest actual vendor (Warden Maren never shadows Tam).
            if (!It->NpcDefinition || It->NpcDefinition->ShopLootTableId.IsNone() || It->NpcDefinition->CurrencyItemId.IsNone())
            {
                continue;
            }
            const float DistSq = FVector::DistSquared(It->GetActorLocation(), Player->GetActorLocation());
            if (DistSq < BestDistSq)
            {
                Best = *It;
                BestDistSq = DistSq;
            }
        }
        return Best;
    }

    /** Map a vendor result to an actionable HUD message. */
    FString VendorResultMessage(const EAstrawildVendorResult Result, const FName ItemId, const int32 Quantity)
    {
        switch (Result)
        {
        case EAstrawildVendorResult::Success:
            return TEXT(""); // The vendor API already notified the player.
        case EAstrawildVendorResult::NotAVendor:
            return TEXT("Nearest NPC is not a vendor.");
        case EAstrawildVendorResult::NotAWare:
            return FString::Printf(TEXT("%s is not traded here (or has no vendor price)."), *ItemId.ToString());
        case EAstrawildVendorResult::NotEnoughCurrency:
            return FString::Printf(TEXT("Not enough vendor currency for %d x %s."), Quantity, *ItemId.ToString());
        case EAstrawildVendorResult::TooHeavy:
            return FString::Printf(TEXT("Too heavy to carry %d x %s."), Quantity, *ItemId.ToString());
        case EAstrawildVendorResult::TooFarAway:
            return TEXT("Get closer to the vendor to trade.");
        case EAstrawildVendorResult::InvalidRequest:
        default:
            return FString::Printf(TEXT("Invalid trade request for %s."), *ItemId.ToString());
        }
    }
}

void UAstrawildCheatManager::BuyItem(const FName ItemId, int32 Quantity)
{
    AAstrawildPlayerCharacter* Player = GetPlayer();
    AAstrawildNPCCharacter* Vendor = FindNearestVendor(Player);
    if (!Player || !Vendor)
    {
        UE_LOG(LogAstrawild, Warning, TEXT("AW.BuyItem: no vendor NPC within 6 m of the player."));
        return;
    }
    Quantity = FMath::Clamp(Quantity, 1, 99);
    const EAstrawildVendorResult Result = Vendor->TryPurchase(Player, ItemId, Quantity);
    const FString Message = VendorResultMessage(Result, ItemId, Quantity);
    if (!Message.IsEmpty())
    {
        UE_LOG(LogAstrawild, Warning, TEXT("AW.BuyItem: %s"), *Message);
        if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(Player->GetController()))
        {
            PC->Notify(FText::FromString(Message));
        }
    }
}

void UAstrawildCheatManager::SellItem(const FName ItemId, int32 Quantity)
{
    AAstrawildPlayerCharacter* Player = GetPlayer();
    AAstrawildNPCCharacter* Vendor = FindNearestVendor(Player);
    if (!Player || !Vendor)
    {
        UE_LOG(LogAstrawild, Warning, TEXT("AW.SellItem: no vendor NPC within 6 m of the player."));
        return;
    }
    Quantity = FMath::Clamp(Quantity, 1, 99);
    const EAstrawildVendorResult Result = Vendor->TrySell(Player, ItemId, Quantity);
    const FString Message = VendorResultMessage(Result, ItemId, Quantity);
    if (!Message.IsEmpty())
    {
        UE_LOG(LogAstrawild, Warning, TEXT("AW.SellItem: %s"), *Message);
        if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(Player->GetController()))
        {
            PC->Notify(FText::FromString(Message));
        }
    }
}

void UAstrawildCheatManager::EquipItem(const FName ItemId)
{
    AAstrawildPlayerCharacter* Player = GetPlayer();
    if (Player && Player->InventoryComponent)
    {
        if (!Player->InventoryComponent->EquipItem(ItemId))
        {
            UE_LOG(LogAstrawildAI, Warning, TEXT("AW.EquipItem failed — item missing or not equipment: %s"), *ItemId.ToString());
        }
    }
}

void UAstrawildCheatManager::SetTime(const int32 Hour, const int32 Minute)
{
    UWorld* World = GetWorld();
    if (UAstrawildTimeSubsystem* Time = World ? World->GetSubsystem<UAstrawildTimeSubsystem>() : nullptr)
    {
        Time->SetTimeOfDay(Hour, Minute);
    }
}

void UAstrawildCheatManager::SetWeather(const FName WeatherName)
{
    UWorld* World = GetWorld();
    UAstrawildWeatherSubsystem* Weather = World ? World->GetSubsystem<UAstrawildWeatherSubsystem>() : nullptr;
    if (!Weather)
    {
        return;
    }

    const FString Name = WeatherName.ToString().ToLower();
    if (Name == TEXT("clear")) { Weather->ForceWeather(EAstrawildWeatherState::Clear); }
    else if (Name == TEXT("cloudy")) { Weather->ForceWeather(EAstrawildWeatherState::Cloudy); }
    else if (Name == TEXT("rain")) { Weather->ForceWeather(EAstrawildWeatherState::Rain); }
    else if (Name == TEXT("heavyrain")) { Weather->ForceWeather(EAstrawildWeatherState::HeavyRain); }
    else if (Name == TEXT("storm")) { Weather->ForceWeather(EAstrawildWeatherState::Storm); }
    else if (Name == TEXT("fog")) { Weather->ForceWeather(EAstrawildWeatherState::Fog); }
    else if (Name == TEXT("heat")) { Weather->ForceWeather(EAstrawildWeatherState::Heat); }
    else if (Name == TEXT("cold")) { Weather->ForceWeather(EAstrawildWeatherState::Cold); }
    else
    {
        UE_LOG(LogAstrawild, Warning, TEXT("AW.SetWeather: unknown weather %s (clear/cloudy/rain/heavyrain/storm/fog/heat/cold)."), *WeatherName.ToString());
    }
}

void UAstrawildCheatManager::God()
{
    if (AAstrawildPlayerCharacter* Player = GetPlayer())
    {
        if (Player->SurvivalComponent)
        {
            Player->SurvivalComponent->SetGodMode(!Player->SurvivalComponent->IsGodMode());
        }
    }
}

void UAstrawildCheatManager::HealAll()
{
    if (AAstrawildPlayerCharacter* Player = GetPlayer())
    {
        if (Player->SurvivalComponent)
        {
            Player->SurvivalComponent->FullRestore();
        }
    }
}

void UAstrawildCheatManager::ResearchPoints(const int32 Amount)
{
    const UWorld* World = GetWorld();
    if (World && World->GetGameInstance())
    {
        if (UAstrawildResearchSubsystem* Research = World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>())
        {
            Research->AddResearchPoints(FMath::Max(0, Amount));
        }
    }
}

void UAstrawildCheatManager::UnlockTech(const FName TechId)
{
    const UWorld* World = GetWorld();
    if (World && World->GetGameInstance())
    {
        if (UAstrawildResearchSubsystem* Research = World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>())
        {
            Research->TryUnlockTech(TechId);
        }
    }
}

void UAstrawildCheatManager::SaveNow()
{
    UWorld* World = GetWorld();
    if (World && World->GetGameInstance())
    {
        if (UAstrawildSaveSubsystem* Save = World->GetGameInstance()->GetSubsystem<UAstrawildSaveSubsystem>())
        {
            Save->SaveWorld(World);
        }
    }
}

void UAstrawildCheatManager::LoadNow()
{
    UWorld* World = GetWorld();
    if (World && World->GetGameInstance())
    {
        if (UAstrawildSaveSubsystem* Save = World->GetGameInstance()->GetSubsystem<UAstrawildSaveSubsystem>())
        {
            // Audit H-3: newest slot (auto vs manual) instead of the manual slot only.
            Save->LoadLatest(World);
        }
    }
}

void UAstrawildCheatManager::CaptureAll()
{
    UWorld* World = GetWorld();
    AAstrawildPlayerCharacter* Player = GetPlayer();
    if (!World || !Player)
    {
        return;
    }

    for (TActorIterator<AAstrawildEchoCharacter> It(World); It; ++It)
    {
        AAstrawildEchoCharacter* Echo = *It;
        if (Echo && !Echo->bCaptured && !Echo->IsDefeated() && Echo->EchoDefinition)
        {
            Echo->OwnerPlayerId = Player->GetFName();
            Echo->Capture(50.0f);
        }
    }
}

void UAstrawildCheatManager::TeleportForward(const float Distance)
{
    AAstrawildPlayerCharacter* Player = GetPlayer();
    if (Player)
    {
        const FVector Target = Player->GetActorLocation() + Player->GetActorForwardVector() * FMath::Max(0.0f, Distance);
        Player->SetActorLocation(Target + FVector(0, 0, 100.0f), false, nullptr, ETeleportType::TeleportPhysics);
    }
}

UAstrawildQuestComponent* UAstrawildCheatManager::GetQuests() const
{
    // The quest component lives on the PlayerController (survives respawn) —
    // the same outer this CheatManager hangs off.
    const APlayerController* PC = Cast<APlayerController>(GetOuter());
    return PC ? PC->FindComponentByClass<UAstrawildQuestComponent>() : nullptr;
}

void UAstrawildCheatManager::FastForward(const FName QuestId)
{
    UAstrawildQuestComponent* Quests = GetQuests();
    if (!Quests)
    {
        UE_LOG(LogAstrawild, Warning, TEXT("AW.FastForward: no quest component on the player controller."));
        return;
    }

    // Walk the ACTIVE chain forward until the requested quest completes.
    // CompleteQuest is the live path: rewards fire exactly once, the next quest
    // auto-starts, and the re-entrancy guard keeps nested broadcasts safe.
    int32 Steps = 0;
    while (!Quests->GetActiveQuestId().IsNone() && Steps < 20)
    {
        const FName Current = Quests->GetActiveQuestId();
        if (Current == QuestId)
        {
            break;
        }
        Quests->CompleteQuest(Current);
        ++Steps;
    }

    if (Quests->GetActiveQuestId() == QuestId)
    {
        Quests->CompleteQuest(QuestId);
        UE_LOG(LogAstrawild, Log, TEXT("AW.FastForward: completed the chain through %s (%d quests)."),
            *QuestId.ToString(), Steps + 1);
    }
    else if (Quests->IsQuestCompleted(QuestId))
    {
        UE_LOG(LogAstrawild, Log, TEXT("AW.FastForward: %s was already completed."), *QuestId.ToString());
    }
    else
    {
        UE_LOG(LogAstrawild, Warning, TEXT("AW.FastForward: %s is not on the active chain (active: %s)."),
            *QuestId.ToString(), *Quests->GetActiveQuestId().ToString());
    }
}
