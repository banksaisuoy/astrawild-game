#include "AstrawildSaveSubsystem.h"

#include "AstrawildBuildingActor.h"
#include "AstrawildCore.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildEchoRosterSubsystem.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameState.h"
#include "AstrawildJournalSubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildQuestComponent.h"
#include "AstrawildResearchSubsystem.h"
#include "AstrawildSurvivalComponent.h"
#include "AstrawildTimeSubsystem.h"
#include "AstrawildWeatherSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

uint32 UAstrawildSaveSubsystem::ComputeChecksum(const int32 SchemaVersion, const FDateTime& SavedAtUtc)
{
    // FNV-1a over schema version + ISO timestamp — catches tampered/truncated headers.
    const FString Source = FString::Printf(TEXT("%d|%s"), SchemaVersion, *SavedAtUtc.ToString());

    uint32 Hash = 2166136261u;
    for (const TCHAR Char : Source)
    {
        Hash ^= static_cast<uint32>(Char);
        Hash *= 16777619u;
    }
    return Hash;
}

bool UAstrawildSaveSubsystem::SaveWorld(UWorld* World, const FString& SlotName, const int32 UserIndex)
{
    if (!World || World->GetNetMode() == NM_Client || SlotName.IsEmpty())
    {
        UE_LOG(LogAstrawildSave, Error, TEXT("SaveWorld rejected: invalid world or slot."));
        return false;
    }

    UAstrawildSaveGame* SaveGame = Cast<UAstrawildSaveGame>(UGameplayStatics::CreateSaveGameObject(UAstrawildSaveGame::StaticClass()));
    if (!IsValid(SaveGame))
    {
        UE_LOG(LogAstrawildSave, Error, TEXT("SaveWorld failed: could not create SaveGame object."));
        return false;
    }

    SaveGame->SaveSchemaVersion = CurrentSchemaVersion;
    SaveGame->SavedAtUtc = FDateTime::UtcNow();
    SaveGame->IntegrityChecksum = ComputeChecksum(SaveGame->SaveSchemaVersion, SaveGame->SavedAtUtc);

    // --- World state ---
    if (const AAstrawildGameState* GameState = World->GetGameState<AAstrawildGameState>())
    {
        SaveGame->WorldState.ElapsedWorldMinutes = GameState->TimeOfDayMinutes;
        SaveGame->WorldState.DayNumber = GameState->DayNumber;
        SaveGame->WorldState.Weather = GameState->WeatherState;
        SaveGame->WorldState.Seed = GameState->WorldSeed;
    }

    // --- Player (first player — single-player-first architecture) ---
    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        if (AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(PC->GetPawn()))
        {
            SaveGame->PlayerTransform = Player->GetActorTransform();
            if (Player->SurvivalComponent)
            {
                SaveGame->PlayerSurvival = Player->SurvivalComponent->GetStats();
            }
            if (Player->InventoryComponent)
            {
                SaveGame->PlayerInventory = Player->InventoryComponent->GetItemStacks();
            }
        }

        // Quests live on the controller.
        if (UAstrawildQuestComponent* Quests = PC->FindComponentByClass<UAstrawildQuestComponent>())
        {
            Quests->ExportForSave(SaveGame->Quests);
        }
    }

    // --- Echo roster (captured creatures) ---
    if (World->GetGameInstance())
    {
        if (UAstrawildEchoRosterSubsystem* Roster = World->GetGameInstance()->GetSubsystem<UAstrawildEchoRosterSubsystem>())
        {
            Roster->ExportForSave(SaveGame->EchoRosterV2);
        }
        if (UAstrawildResearchSubsystem* Research = World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>())
        {
            Research->ExportForSave(SaveGame->Research);
        }
    }

    // --- Buildings ---
    for (TActorIterator<AAstrawildBuildingActor> It(World); It; ++It)
    {
        SaveGame->Buildings.Add(It->ToSaveData());
    }

    // --- Journal ---
    if (const UAstrawildJournalSubsystem* Journal = World->GetSubsystem<UAstrawildJournalSubsystem>())
    {
        Journal->ExportForSave(SaveGame->Journal);
    }

    const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, UserIndex);
    if (bSaved)
    {
        UE_LOG(LogAstrawildSave, Log, TEXT("World saved to slot %s (schema %d, %d buildings, roster %d)."),
            *SlotName, CurrentSchemaVersion, SaveGame->Buildings.Num(), SaveGame->EchoRosterV2.Num());
    }
    else
    {
        UE_LOG(LogAstrawildSave, Error, TEXT("SaveWorld failed for slot %s."), *SlotName);
    }
    return bSaved;
}

bool UAstrawildSaveSubsystem::MigrateV1ToV2(UAstrawildSaveGame* SaveGame) const
{
    if (!SaveGame || SaveGame->SaveSchemaVersion != 1)
    {
        return false;
    }

    // v1 -> v2: lift roster entries into the richer struct with defaults (directive §27 migration).
    for (const FAstrawildEchoInstanceSaveData& Old : SaveGame->EchoRoster)
    {
        FAstrawildEchoInstanceV2 New;
        New.InstanceId = Old.InstanceId;
        New.DefinitionId = Old.DefinitionId;
        New.Level = Old.Level;
        New.Trust = Old.Trust;
        New.Personality = EAstrawildPersonality::Curious;
        New.bInParty = Old.bInRoster;
        New.LastKnownTransform = Old.LastKnownTransform;
        SaveGame->EchoRosterV2.Add(New);
    }

    SaveGame->SaveSchemaVersion = 2;
    UE_LOG(LogAstrawildSave, Log, TEXT("Migrated save from schema v1 to v2 (%d roster entries)."), SaveGame->EchoRosterV2.Num());
    return true;
}

bool UAstrawildSaveSubsystem::LoadWorld(UWorld* World, const FString& SlotName, const int32 UserIndex)
{
    if (!World || World->GetNetMode() == NM_Client || !DoesSaveExist(SlotName, UserIndex))
    {
        return false;
    }

    UAstrawildSaveGame* SaveGame = Cast<UAstrawildSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
    if (!IsValid(SaveGame))
    {
        UE_LOG(LogAstrawildSave, Error, TEXT("LoadWorld: slot %s is corrupt."), *SlotName);
        return false;
    }

    // Integrity check (directive §27 checksum strategy).
    if (SaveGame->IntegrityChecksum != 0 &&
        SaveGame->IntegrityChecksum != ComputeChecksum(SaveGame->SaveSchemaVersion, SaveGame->SavedAtUtc))
    {
        UE_LOG(LogAstrawildSave, Error, TEXT("LoadWorld: checksum mismatch on slot %s — refusing to load."), *SlotName);
        return false;
    }

    if (SaveGame->SaveSchemaVersion < CurrentSchemaVersion)
    {
        MigrateV1ToV2(SaveGame);
    }

    // --- World state ---
    if (AAstrawildGameState* GameState = World->GetGameState<AAstrawildGameState>())
    {
        GameState->SetTimeOfDayMinutes(SaveGame->WorldState.ElapsedWorldMinutes);
        while (GameState->DayNumber < SaveGame->WorldState.DayNumber)
        {
            GameState->AdvanceDay();
        }
        GameState->SetWeatherState(SaveGame->WorldState.Weather);
        GameState->SetWorldSeed(SaveGame->WorldState.Seed);
    }

    // --- Research ---
    if (World->GetGameInstance())
    {
        if (UAstrawildResearchSubsystem* Research = World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>())
        {
            Research->ImportFromSave(SaveGame->Research);
        }
        if (UAstrawildEchoRosterSubsystem* Roster = World->GetGameInstance()->GetSubsystem<UAstrawildEchoRosterSubsystem>())
        {
            Roster->ImportFromSave(SaveGame->EchoRosterV2);
        }
    }

    // --- Player ---
    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        if (AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(PC->GetPawn()))
        {
            Player->SetActorTransform(SaveGame->PlayerTransform, false, nullptr, ETeleportType::TeleportPhysics);
            if (Player->SurvivalComponent)
            {
                Player->SurvivalComponent->FullRestore();
            }
            if (Player->InventoryComponent)
            {
                Player->InventoryComponent->SetItemStacks(SaveGame->PlayerInventory);
            }

            // Respawn party Echoes around the player (directive §10 party of 3).
            if (World->GetGameInstance())
            {
                if (UAstrawildEchoRosterSubsystem* Roster = World->GetGameInstance()->GetSubsystem<UAstrawildEchoRosterSubsystem>())
                {
                    // Despawn current party first.
                    for (AAstrawildEchoCharacter* Echo : Roster->GetSpawnedParty())
                    {
                        if (IsValid(Echo))
                        {
                            Echo->Destroy();
                        }
                    }
                    Roster->ImportFromSave(SaveGame->EchoRosterV2);
                }
            }
        }

        if (UAstrawildQuestComponent* Quests = PC->FindComponentByClass<UAstrawildQuestComponent>())
        {
            Quests->ImportFromSave(SaveGame->Quests);
        }
    }

    // --- Buildings: remove placed, respawn from data ---
    for (TActorIterator<AAstrawildBuildingActor> It(World); It; ++It)
    {
        It->Destroy();
    }
    for (const FAstrawildBuildingSaveData& Data : SaveGame->Buildings)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        AAstrawildBuildingActor* Building = World->SpawnActor<AAstrawildBuildingActor>(
            AAstrawildBuildingActor::StaticClass(), Data.Transform.GetLocation(),
            Data.Transform.Rotator(), Params);
        if (Building)
        {
            Building->FromSaveData(Data);
        }
    }

    // --- Journal ---
    if (UAstrawildJournalSubsystem* Journal = World->GetSubsystem<UAstrawildJournalSubsystem>())
    {
        Journal->ImportFromSave(SaveGame->Journal);
    }

    UE_LOG(LogAstrawildSave, Log, TEXT("World loaded from slot %s (day %d, %d buildings)."),
        *SlotName, SaveGame->WorldState.DayNumber, SaveGame->Buildings.Num());
    return true;
}

bool UAstrawildSaveSubsystem::SaveSnapshot(const TArray<FAstrawildItemStack>& Inventory, const TArray<FAstrawildEchoInstanceSaveData>& EchoRoster, const TArray<FAstrawildRestPointSaveData>& RestPoints, const FGuid& ActiveRestPointId, const FString& SlotName, const int32 UserIndex)
{
    if (SlotName.IsEmpty())
    {
        UE_LOG(LogAstrawildSave, Error, TEXT("SaveSnapshot rejected: SlotName is empty."));
        return false;
    }

    UAstrawildSaveGame* SaveGame = Cast<UAstrawildSaveGame>(UGameplayStatics::CreateSaveGameObject(UAstrawildSaveGame::StaticClass()));
    if (!IsValid(SaveGame))
    {
        UE_LOG(LogAstrawildSave, Error, TEXT("SaveSnapshot failed: could not create SaveGame object."));
        return false;
    }

    SaveGame->SaveSchemaVersion = CurrentSchemaVersion;
    SaveGame->SavedAtUtc = FDateTime::UtcNow();
    SaveGame->IntegrityChecksum = ComputeChecksum(SaveGame->SaveSchemaVersion, SaveGame->SavedAtUtc);
    SaveGame->PlayerInventory = Inventory;
    SaveGame->EchoRoster = EchoRoster;
    SaveGame->RestPoints = RestPoints;
    SaveGame->ActiveRestPointId = ActiveRestPointId;

    const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, UserIndex);
    if (!bSaved)
    {
        UE_LOG(LogAstrawildSave, Error, TEXT("SaveSnapshot failed for slot %s."), *SlotName);
    }
    return bSaved;
}

bool UAstrawildSaveSubsystem::LoadSnapshot(TArray<FAstrawildItemStack>& OutInventory, TArray<FAstrawildEchoInstanceSaveData>& OutEchoRoster, TArray<FAstrawildRestPointSaveData>& OutRestPoints, FGuid& OutActiveRestPointId, const FString& SlotName, const int32 UserIndex)
{
    OutInventory.Reset();
    OutEchoRoster.Reset();
    OutRestPoints.Reset();
    OutActiveRestPointId.Invalidate();

    if (SlotName.IsEmpty() || !UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex))
    {
        return false;
    }

    UAstrawildSaveGame* SaveGame = Cast<UAstrawildSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, UserIndex));
    if (!IsValid(SaveGame))
    {
        return false;
    }

    OutInventory = SaveGame->PlayerInventory;
    OutEchoRoster = SaveGame->EchoRoster;
    OutRestPoints = SaveGame->RestPoints;
    OutActiveRestPointId = SaveGame->ActiveRestPointId;
    return true;
}

bool UAstrawildSaveSubsystem::DoesSaveExist(const FString& SlotName, const int32 UserIndex) const
{
    return !SlotName.IsEmpty() && UGameplayStatics::DoesSaveGameExist(SlotName, UserIndex);
}

bool UAstrawildSaveSubsystem::DeleteSave(const FString& SlotName, const int32 UserIndex)
{
    if (!DoesSaveExist(SlotName, UserIndex))
    {
        return false;
    }
    return UGameplayStatics::DeleteGameInSlot(SlotName, UserIndex);
}
