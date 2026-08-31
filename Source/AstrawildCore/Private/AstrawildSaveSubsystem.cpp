#include "AstrawildSaveSubsystem.h"

#include "AstrawildBuildingActor.h"
#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildDialogueComponent.h"
#include "AstrawildDungeonGeneratorActor.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildEchoRosterSubsystem.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameState.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildJournalSubsystem.h"
#include "AstrawildPOISubsystem.h"
#include "AstrawildWorldEventSubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPowerSubsystem.h"
#include "AstrawildQuestComponent.h"
#include "AstrawildResearchSubsystem.h"
#include "AstrawildRestPoint.h"
#include "AstrawildSurvivalComponent.h"
#include "AstrawildTimeSubsystem.h"
#include "AstrawildUtilityDroneActor.h"
#include "AstrawildUtilityRobotActor.h"
#include "AstrawildWeatherSubsystem.h"
#include "AstrawildWorkSiteActor.h"
#include "AstrawildZoneSubsystem.h"
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
                SaveGame->EquippedWeaponId = Player->InventoryComponent->EquippedItemId;
                SaveGame->EquippedShieldId = Player->InventoryComponent->EquippedShieldItemId;
                // Batch 3 — Item C: persist the torso armor slot.
                SaveGame->EquippedArmorId = Player->InventoryComponent->EquippedArmorItemId;
                // Final production run (v3): the advanced slots ride along.
                SaveGame->EquippedHelmetId = Player->InventoryComponent->EquippedHelmetItemId;
                SaveGame->EquippedExosuitId = Player->InventoryComponent->EquippedExosuitItemId;
                SaveGame->EquippedScannerId = Player->InventoryComponent->EquippedScannerItemId;
            }

            // Final production run (v3): the deployed drone companion.
            if (AAstrawildUtilityDroneActor* Drone = Player->GetActiveDrone())
            {
                FAstrawildDroneSaveData DroneData;
                DroneData.OwnerPlayerId = Drone->GetOwnerPlayerId();
                DroneData.Transform = Drone->GetActorTransform();
                DroneData.bDeployed = true;
                // Production V2 (v4): battery state persists mid-drain.
                DroneData.BatteryRemainingSeconds = Drone->BatteryRemainingSeconds;
                SaveGame->Drones.Add(DroneData);
            }
        }

        // Quests live on the controller.
        if (UAstrawildQuestComponent* Quests = PC->FindComponentByClass<UAstrawildQuestComponent>())
        {
            Quests->ExportForSave(SaveGame->Quests);
        }

        // Batch 3 — persistent story flags live beside them.
        if (UAstrawildDialogueComponent* Dialogue = PC->FindComponentByClass<UAstrawildDialogueComponent>())
        {
            Dialogue->ExportForSave(SaveGame->DialogueFlags);
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

    // --- Final production run (v3): rest points (previously only the legacy
    //     SaveSnapshot API ever persisted these). ---
    for (TActorIterator<AAstrawildRestPoint> RestIt(World); RestIt; ++RestIt)
    {
        SaveGame->RestPoints.Add(RestIt->ToSaveData());
        if (RestIt->bActive)
        {
            SaveGame->ActiveRestPointId = RestIt->WorldObjectId;
        }
    }

    // --- Final production run (v3): work sites — stored output + assignments so the
    //     automation loop survives a reload (previously transient). ---
    for (TActorIterator<AAstrawildWorkSiteActor> SiteIt(World); SiteIt; ++SiteIt)
    {
        SaveGame->WorkSites.Add(SiteIt->ExportForSave());
    }

    // --- Final production run (v3): utility robots + their site assignments. ---
    for (TActorIterator<AAstrawildUtilityRobotActor> RobotIt(World); RobotIt; ++RobotIt)
    {
        FAstrawildRobotSaveData RobotData;
        RobotData.OwnerPlayerId = RobotIt->GetOwnerPlayerId();
        RobotData.Transform = RobotIt->GetActorTransform();
        RobotData.AssignedSiteId = RobotIt->GetAssignedSiteId();
        SaveGame->Robots.Add(RobotData);
    }

    // --- Final production run (v3): power grid buffered charge. ---
    if (const UAstrawildPowerSubsystem* Power = World->GetSubsystem<UAstrawildPowerSubsystem>())
    {
        SaveGame->PowerGrid.StoredEnergy = Power->GetStoredEnergy();
    }

    // --- Dungeons (Batch 6 — gap M-7): cleared-room progression so reloads stop
    //     resurrecting encounters. In-progress rooms respawn fresh by policy.
    for (TActorIterator<AAstrawildDungeonGeneratorActor> DungeonIt(World); DungeonIt; ++DungeonIt)
    {
        SaveGame->Dungeons.Add(DungeonIt->ExportForSave());
    }

    // --- Journal ---
    if (const UAstrawildJournalSubsystem* Journal = World->GetSubsystem<UAstrawildJournalSubsystem>())
    {
        Journal->ExportForSave(SaveGame->Journal);
    }

    // --- Zones (Batch 7 — The Shattered Vale): discovery list ---
    if (const UAstrawildZoneSubsystem* ZoneSub = World->GetSubsystem<UAstrawildZoneSubsystem>())
    {
        ZoneSub->ExportForSave(SaveGame->Zones.DiscoveredZones);
    }

    // --- Production V2 (v4): world-event scheduler + POI discoveries ---
    if (const UAstrawildWorldEventSubsystem* WorldEvents = World->GetSubsystem<UAstrawildWorldEventSubsystem>())
    {
        WorldEvents->ExportForSave(SaveGame->WorldEvents);
    }
    if (const UAstrawildPOISubsystem* POIs = World->GetSubsystem<UAstrawildPOISubsystem>())
    {
        POIs->ExportForSave(SaveGame->DiscoveredPOIIds);
    }

    const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveGame, SlotName, UserIndex);
    if (bSaved)
    {
        UE_LOG(LogAstrawildSave, Log, TEXT("World saved to slot %s (schema %d, %d buildings, roster %d, %d dungeons, %d zones, %d work sites, %d robots, %d drones, grid %.0f)."),
            *SlotName, CurrentSchemaVersion, SaveGame->Buildings.Num(), SaveGame->EchoRosterV2.Num(), SaveGame->Dungeons.Num(), SaveGame->Zones.DiscoveredZones.Num(),
            SaveGame->WorkSites.Num(), SaveGame->Robots.Num(), SaveGame->Drones.Num(), SaveGame->PowerGrid.StoredEnergy);
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

void UAstrawildSaveSubsystem::MigrateV2ToV3(UAstrawildSaveGame* SaveGame) const
{
    if (!SaveGame || SaveGame->SaveSchemaVersion != 2)
    {
        return;
    }

    // v2 -> v3: purely additive — the new payload (advanced equipment ids, work
    // sites, drones, robots, grid charge) default-initializes. Stamping the version
    // keeps future migrations honest about which payloads a save carries.
    SaveGame->SaveSchemaVersion = 3;
    UE_LOG(LogAstrawildSave, Log, TEXT("Migrated save from schema v2 to v3 (additive)."));
}

void UAstrawildSaveSubsystem::MigrateV3ToV4(UAstrawildSaveGame* SaveGame) const
{
    if (!SaveGame || SaveGame->SaveSchemaVersion != 3)
    {
        return;
    }

    // v3 -> v4: purely additive — world-event scheduler state + POI discovery
    // list default-init (no events running, nothing discovered). Drone battery
    // defaults to full on legacy saves (deployed drones recharge on migration).
    SaveGame->SaveSchemaVersion = 4;
    UE_LOG(LogAstrawildSave, Log, TEXT("Migrated save from schema v3 to v4 (additive)."));
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
        MigrateV2ToV3(SaveGame);
        MigrateV3ToV4(SaveGame);
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
                // Audit H-1: restore the saved vitals snapshot instead of FullRestore(),
                // which silently reset hunger/thirst/health on every load.
                Player->SurvivalComponent->SetStatsForRestore(SaveGame->PlayerSurvival);
            }
            if (Player->InventoryComponent)
            {
                Player->InventoryComponent->SetItemStacks(SaveGame->PlayerInventory);
                // Wave 3: restore equipment only when the items survived in the inventory.
                if (!SaveGame->EquippedWeaponId.IsNone() && Player->InventoryComponent->HasItem(SaveGame->EquippedWeaponId, 1))
                {
                    Player->InventoryComponent->EquipItem(SaveGame->EquippedWeaponId);
                }
                if (!SaveGame->EquippedShieldId.IsNone() && Player->InventoryComponent->HasItem(SaveGame->EquippedShieldId, 1))
                {
                    Player->InventoryComponent->EquipItem(SaveGame->EquippedShieldId);
                }
                // Batch 3 — Item C: restore the torso armor slot (HasItem-guarded, same
                // pattern as weapon/shield — stale ids from removed items are skipped).
                if (!SaveGame->EquippedArmorId.IsNone() && Player->InventoryComponent->HasItem(SaveGame->EquippedArmorId, 1))
                {
                    Player->InventoryComponent->EquipItem(SaveGame->EquippedArmorId);
                }
                // Final production run (v3): the advanced slots, same HasItem guard.
                if (!SaveGame->EquippedHelmetId.IsNone() && Player->InventoryComponent->HasItem(SaveGame->EquippedHelmetId, 1))
                {
                    Player->InventoryComponent->EquipItem(SaveGame->EquippedHelmetId);
                }
                if (!SaveGame->EquippedExosuitId.IsNone() && Player->InventoryComponent->HasItem(SaveGame->EquippedExosuitId, 1))
                {
                    Player->InventoryComponent->EquipItem(SaveGame->EquippedExosuitId);
                }
                if (!SaveGame->EquippedScannerId.IsNone() && Player->InventoryComponent->HasItem(SaveGame->EquippedScannerId, 1))
                {
                    Player->InventoryComponent->EquipItem(SaveGame->EquippedScannerId);
                }
            }

            // Audit H-2: respawn the captured party AROUND the player — the roster was
            // already imported once above; the old second ImportFromSave was redundant and
            // this block previously only despawned the party without ever recreating it.
            if (World->GetGameInstance())
            {
                if (UAstrawildEchoRosterSubsystem* Roster = World->GetGameInstance()->GetSubsystem<UAstrawildEchoRosterSubsystem>())
                {
                    // Despawn the pre-load party first (fresh actors get spawned below).
                    for (AAstrawildEchoCharacter* Echo : Roster->GetSpawnedParty())
                    {
                        if (IsValid(Echo))
                        {
                            Echo->Destroy();
                        }
                    }
                    Roster->SpawnPartyActors(PC);
                }
            }

            // Final production run (v3): recall any live drone before (possibly)
            // re-deploying from the save payload below.
            if (AAstrawildUtilityDroneActor* ExistingDrone = Player->GetActiveDrone())
            {
                ExistingDrone->Destroy();
            }
        }

        if (UAstrawildQuestComponent* Quests = PC->FindComponentByClass<UAstrawildQuestComponent>())
        {
            Quests->ImportFromSave(SaveGame->Quests);
        }

        // Batch 3 — restore the story flags (dialogue gates survive reloads).
        if (UAstrawildDialogueComponent* Dialogue = PC->FindComponentByClass<UAstrawildDialogueComponent>())
        {
            Dialogue->ImportFromSave(SaveGame->DialogueFlags);
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

    // --- Final production run (v3): rest points — restore activation flags by id. ---
    for (TActorIterator<AAstrawildRestPoint> RestIt(World); RestIt; ++RestIt)
    {
        const FAstrawildRestPointSaveData* Found = SaveGame->RestPoints.FindByPredicate(
            [RestIt](const FAstrawildRestPointSaveData& Record) { return Record.WorldObjectId == RestIt->WorldObjectId; });
        if (Found)
        {
            RestIt->bActive = Found->bActive;
        }
    }

    // --- Final production run (v3): work sites — restore identity/output by site id,
    //     then re-link Echo assignments (party actors were spawned above; robots
    //     respawn below and re-attach by site id). ---
    for (TActorIterator<AAstrawildWorkSiteActor> SiteIt(World); SiteIt; ++SiteIt)
    {
        const FAstrawildWorkSiteSaveData* Found = SaveGame->WorkSites.FindByPredicate(
            [SiteIt](const FAstrawildWorkSiteSaveData& Record) { return Record.SiteId == SiteIt->SiteId; });
        if (!Found)
        {
            continue;
        }

        SiteIt->ImportFromSave(*Found);

        if (World->GetGameInstance())
        {
            if (UAstrawildEchoRosterSubsystem* Roster = World->GetGameInstance()->GetSubsystem<UAstrawildEchoRosterSubsystem>())
            {
                for (const FGuid& InstanceId : Found->AssignedEchoInstanceIds)
                {
                    for (AAstrawildEchoCharacter* Echo : Roster->GetSpawnedParty())
                    {
                        if (IsValid(Echo) && Echo->InstanceId == InstanceId)
                        {
                            SiteIt->AssignWorker(Echo);
                            break;
                        }
                    }
                }
            }
        }
    }

    // --- Final production run (v3): utility robots — respawn and re-attach by site id. ---
    for (TActorIterator<AAstrawildUtilityRobotActor> RobotIt(World); RobotIt; ++RobotIt)
    {
        RobotIt->Destroy();
    }
    for (const FAstrawildRobotSaveData& RobotData : SaveGame->Robots)
    {
        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        AAstrawildUtilityRobotActor* Robot = World->SpawnActor<AAstrawildUtilityRobotActor>(
            AAstrawildUtilityRobotActor::StaticClass(), RobotData.Transform.GetLocation(), RobotData.Transform.Rotator(), Params);
        if (!Robot)
        {
            continue;
        }
        Robot->SetOwnerPlayerId(RobotData.OwnerPlayerId);

        // Production V2 (v4): specialist chassis resolve their profile by id.
        if (!RobotData.RobotDefinitionId.IsNone())
        {
            if (UAstrawildItemRegistrySubsystem* Registry = World->GetSubsystem<UAstrawildItemRegistrySubsystem>())
            {
                if (UAstrawildRobotDefinition* RobotDef = Registry->FindRobot(RobotData.RobotDefinitionId))
                {
                    Robot->InitializeFromDefinition(RobotDef);
                }
            }
        }

        if (!RobotData.AssignedSiteId.IsNone())
        {
            for (TActorIterator<AAstrawildWorkSiteActor> SiteIt(World); SiteIt; ++SiteIt)
            {
                if (SiteIt->SiteId == RobotData.AssignedSiteId && !SiteIt->HasRobot())
                {
                    Robot->AssignToSite(*SiteIt);
                    break;
                }
            }
        }
    }

    // --- Final production run (v3): drones — re-deploy the saved companion. ---
    for (const FAstrawildDroneSaveData& DroneData : SaveGame->Drones)
    {
        if (!DroneData.bDeployed)
        {
            continue;
        }

        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            if (AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(PC->GetPawn()))
            {
                if (AAstrawildUtilityDroneActor* Drone = Player->SpawnUtilityDrone())
                {
                    Drone->SetActorTransform(DroneData.Transform, false, nullptr, ETeleportType::TeleportPhysics);
                    // Production V2 (v4): resume the battery mid-drain (legacy
                    // saves default to full via the default-init field).
                    if (DroneData.BatteryRemainingSeconds > 0.0f)
                    {
                        Drone->BatteryRemainingSeconds = DroneData.BatteryRemainingSeconds;
                    }
                }
            }
        }
    }

    // --- Journal ---
    if (UAstrawildJournalSubsystem* Journal = World->GetSubsystem<UAstrawildJournalSubsystem>())
    {
        Journal->ImportFromSave(SaveGame->Journal);
    }

    // --- Zones (Batch 7 — The Shattered Vale): restore the discovered-zone list.
    if (UAstrawildZoneSubsystem* ZoneSub = World->GetSubsystem<UAstrawildZoneSubsystem>())
    {
        ZoneSub->ImportFromSave(SaveGame->Zones.DiscoveredZones);
    }

    // --- Production V2 (v4): world-event scheduler + POI discoveries. ---
    if (UAstrawildWorldEventSubsystem* WorldEvents = World->GetSubsystem<UAstrawildWorldEventSubsystem>())
    {
        WorldEvents->ImportFromSave(SaveGame->WorldEvents);
    }
    if (UAstrawildPOISubsystem* POIs = World->GetSubsystem<UAstrawildPOISubsystem>())
    {
        POIs->ImportFromSave(SaveGame->DiscoveredPOIIds);
    }

    // --- Dungeons (Batch 6 — gap M-7): generators already built their rooms during
    //     bootstrapper BeginPlay; apply the cleared-room snapshot on top. Cleared
    //     rooms lose their freshly-spawned encounters; gates reopen in sync.
    for (TActorIterator<AAstrawildDungeonGeneratorActor> DungeonIt(World); DungeonIt; ++DungeonIt)
    {
        const FAstrawildDungeonSaveData* Found = SaveGame->Dungeons.FindByPredicate(
            [&DungeonIt](const FAstrawildDungeonSaveData& Record)
            {
                return Record.DungeonId == DungeonIt->DungeonId;
            });
        if (Found)
        {
            DungeonIt->ApplySavedState(*Found);
        }
    }

    // Batch 2 — Item C: re-resolve the power grid immediately so the first frame the
    // player sees after load is correct (no 2s brownout flicker while waiting for the
    // natural Tick cadence). Buildings auto-registered with the power subsystem
    // through BeginPlay during the spawn loop above, so ResolveGridNow sees them.
    if (UAstrawildPowerSubsystem* Power = World->GetSubsystem<UAstrawildPowerSubsystem>())
    {
        // Final production run (v3): restore buffered charge AFTER the grid totals
        // are known (SetStoredEnergy clamps to live battery capacity).
        Power->ResolveGridNow();
        Power->SetStoredEnergy(SaveGame->PowerGrid.StoredEnergy);
    }

    UE_LOG(LogAstrawildSave, Log, TEXT("World loaded from slot %s (day %d, %d buildings, %d work sites, %d robots, grid %.0f)."),
        *SlotName, SaveGame->WorldState.DayNumber, SaveGame->Buildings.Num(), SaveGame->WorkSites.Num(), SaveGame->Robots.Num(), SaveGame->PowerGrid.StoredEnergy);
    return true;
}

bool UAstrawildSaveSubsystem::LoadLatest(UWorld* World, const int32 UserIndex)
{
    // Audit H-3: pick the newest slot between the manual save and the autosave. Reads the
    // SavedAtUtc header only (cheap) — corrupt slots fall back to the alternative.
    static const TCHAR* MainSlot = TEXT("ASTRAWILD_Main");
    static const TCHAR* AutoSlot = TEXT("ASTRAWILD_Auto");

    const bool bMainExists = DoesSaveExist(MainSlot, UserIndex);
    const bool bAutoExists = DoesSaveExist(AutoSlot, UserIndex);

    if (!bMainExists && !bAutoExists)
    {
        UE_LOG(LogAstrawildSave, Warning, TEXT("LoadLatest: no save slots exist."));
        return false;
    }
    if (!bAutoExists)
    {
        return LoadWorld(World, MainSlot, UserIndex);
    }
    if (!bMainExists)
    {
        return LoadWorld(World, AutoSlot, UserIndex);
    }

    FDateTime MainTime(0);
    FDateTime AutoTime(0);
    if (const UAstrawildSaveGame* MainSave = Cast<UAstrawildSaveGame>(UGameplayStatics::LoadGameFromSlot(MainSlot, UserIndex)))
    {
        MainTime = MainSave->SavedAtUtc;
    }
    if (const UAstrawildSaveGame* AutoSave = Cast<UAstrawildSaveGame>(UGameplayStatics::LoadGameFromSlot(AutoSlot, UserIndex)))
    {
        AutoTime = AutoSave->SavedAtUtc;
    }

    const FString& Chosen = (AutoTime > MainTime) ? FString(AutoSlot) : FString(MainSlot);
    UE_LOG(LogAstrawildSave, Log, TEXT("LoadLatest: choosing slot %s (auto %s vs main %s)."),
        *Chosen, *AutoTime.ToString(), *MainTime.ToString());
    return LoadWorld(World, Chosen, UserIndex);
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

    // Audit fix: this legacy API fills ONLY v1 fields — stamp schema 1 so a later
    // LoadWorld migrates the roster instead of reading an empty EchoRosterV2 while
    // believing it is already schema v2 (which silently stranded legacy saves).
    SaveGame->SaveSchemaVersion = 1;
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
