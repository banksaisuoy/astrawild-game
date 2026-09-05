#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameFramework/SaveGame.h"
#include "AstrawildTypes.h"
#include "AstrawildSaveSubsystem.generated.h"

/**
 * Production save system (directive §27): schema v2 with v1 migration, FNV-1a
 * integrity checksum, autosave and full world orchestration.
 * Everything persistent resolves through stable ids.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    int32 SaveSchemaVersion = 2;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    FDateTime SavedAtUtc;

    /** FNV-1a checksum over schema version + timestamp — corruption tripwire. */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    int64 IntegrityChecksum = 0;

    // --- v1 payload (kept for migration) ---
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FAstrawildItemStack> PlayerInventory;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FAstrawildEchoInstanceSaveData> EchoRoster;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FAstrawildRestPointSaveData> RestPoints;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    FGuid ActiveRestPointId;

    // --- v2 payload ---
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    FAstrawildWorldSaveData WorldState;

    /** Wave 3: equipped weapon/shield ids persist across sessions (additive v2 payload). */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    FName EquippedWeaponId = NAME_None;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    FName EquippedShieldId = NAME_None;

    /** Batch 3 — Item C: equipped torso armor persists (additive, no schema bump). */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    FName EquippedArmorId = NAME_None;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    FAstrawildSurvivalStats PlayerSurvival;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    FTransform PlayerTransform;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FAstrawildEchoInstanceV2> EchoRosterV2;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FAstrawildBuildingSaveData> Buildings;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    FAstrawildResearchSaveData Research;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FAstrawildQuestSaveData> Quests;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FAstrawildJournalEntry> Journal;

    /** Batch 6 — gap M-7: dungeon room-clear progression (additive, no schema bump). */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FAstrawildDungeonSaveData> Dungeons;

    /** Batch 7 — The Shattered Vale: zone discovery (additive, no schema bump). */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    FAstrawildZoneSaveData Zones;

    // --- v3 payload (final production run — closes the last save gaps) ---

    /** Advanced equipment slots (additive v3). */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    FName EquippedHelmetId = NAME_None;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    FName EquippedExosuitId = NAME_None;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    FName EquippedScannerId = NAME_None;

    /** Work sites: stored output + worker assignments (the automation loop persists). */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FAstrawildWorkSiteSaveData> WorkSites;

    /** Deployed utility drones. */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FAstrawildDroneSaveData> Drones;

    /** Deployed utility robots + their site assignments. */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FAstrawildRobotSaveData> Robots;

    /** Power grid buffered energy (battery charge finally survives reloads). */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    FAstrawildPowerGridSaveData PowerGrid;

    // --- v4 payload (Production V2 — world events, POIs, drone battery) ---

    /** World-event scheduler state (active events + roll clock + cooldowns). */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    FAstrawildWorldEventScheduleSaveData WorldEvents;

    /** Discovered points of interest. */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FName> DiscoveredPOIIds;

    /** Production V2 Batch 3: persistent story flags set through dialogue choices
     *  (v4 payload extension — additive, older v4 saves deserialize empty). */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FName> DialogueFlags;

    // --- v5 payload (Final Run — Act 3 ending state) ---

    /** GDP-3: player attribute levels/XP (absent in pre-GDP saves = fresh states). */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FAstrawildAttributeSaveData> Attributes;

    /** GDP-4: per-NPC affinity (absent in pre-GDP saves = strangers). */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FAstrawildNPCAffinitySaveData> NPCAffinities;

    /** Ending choice as int32-cast EAstrawildEndingState (0 = None = story in play). */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    int32 EndingState = 0;

    /** True once any ending was chosen — post-game free-roam flag. */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    bool bPostGameUnlocked = false;

    /** Final-audit G-3 (additive v5, no schema bump): lifetime defeat counters
     *  (species/boss id -> kills). One-shot bosses defeated before their quest
     *  activates must stay creditable across sessions — see QuestComponent. */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TMap<FName, int32> DefeatedCreatureCounts;

    // --- SCP (additive v5, no schema bump) — survival depth persistence ---

    /** SCP Phase 12: equipment wear pools (item id -> remaining durability).
     *  Absent in pre-SCP saves = pristine equipment (identical semantics). */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TMap<FName, float> EquipmentDurability;

    /** SCP Phase 12: perishable freshness (item id -> remaining seconds). */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TMap<FName, float> FoodFreshness;

    // --- LCP-4 (LAN co-op, additive v5 payload — no schema bump) ---

    /** One block per non-host LAN player, keyed by the stable player key (empty = single-player / pre-LCP saves). */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FAstrawildCoopPlayerSaveBlock> CoopPlayers;
};

UCLASS()
class ASTRAWILDCORE_API UAstrawildSaveSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** Save the whole world state (server call). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Save")
    bool SaveWorld(UWorld* World, const FString& SlotName = TEXT("ASTRAWILD_Main"), int32 UserIndex = 0);

    /** Load and apply the whole world state (server call). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Save")
    bool LoadWorld(UWorld* World, const FString& SlotName = TEXT("ASTRAWILD_Main"), int32 UserIndex = 0);

    /**
     * Audit H-3: load the newest existing save — the autosave slot wins when it is more
     * recent than the manual slot. Previously the autosave was written every 5 minutes but
     * NO code path ever loaded it, breaking crash recovery entirely.
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Save")
    bool LoadLatest(UWorld* World, int32 UserIndex = 0);

    /** v1-compatible snapshot API kept for legacy callers. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Save")
    bool SaveSnapshot(const TArray<FAstrawildItemStack>& Inventory, const TArray<FAstrawildEchoInstanceSaveData>& EchoRoster, const TArray<FAstrawildRestPointSaveData>& RestPoints, const FGuid& ActiveRestPointId, const FString& SlotName = TEXT("ASTRAWILD_Main"), int32 UserIndex = 0);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Save")
    bool LoadSnapshot(TArray<FAstrawildItemStack>& OutInventory, TArray<FAstrawildEchoInstanceSaveData>& OutEchoRoster, TArray<FAstrawildRestPointSaveData>& OutRestPoints, FGuid& OutActiveRestPointId, const FString& SlotName = TEXT("ASTRAWILD_Main"), int32 UserIndex = 0);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Save")
    bool DoesSaveExist(const FString& SlotName = TEXT("ASTRAWILD_Main"), int32 UserIndex = 0) const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Save")
    bool DeleteSave(const FString& SlotName = TEXT("ASTRAWILD_Main"), int32 UserIndex = 0);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Save")
    int32 GetCurrentSchemaVersion() const { return CurrentSchemaVersion; }

    // --- LCP-4: LAN co-op per-player persistence ---

    /**
     * LCP-4: build the per-player block for a connected non-host player
     * (host machine only; the data lives on the server-side pawn/components).
     */
    FAstrawildCoopPlayerSaveBlock BuildCoopPlayerBlock(APlayerController* PC) const;

    /** LCP-4: apply one per-player block to a connected controller (host machine only). */
    void ApplyCoopPlayerBlock(APlayerController* PC, const FAstrawildCoopPlayerSaveBlock& Block);

    /**
     * LCP-4: late-join / reconnect restore — checks the in-session player
     * cache first, then the LATEST save's CoopPlayers blocks. Returns true
     * when a block matched and applied (host machine only).
     */
    bool TryRestoreLateJoinPlayer(APlayerController* PC);

    /** LCP-4: snapshot a connected player's block into the in-session cache (disconnects, autosaves). */
    void SnapshotPlayerForSession(APlayerController* PC);

    /** FNV-1a integrity hash for the save header fields. */
    static uint32 ComputeChecksum(int32 SchemaVersion, const FDateTime& SavedAtUtc);

private:
    static constexpr int32 CurrentSchemaVersion = 5;

    /**
     * FR-2 (Final Run redo): hard cap on day catch-up during load. A corrupted
     * DayNumber (e.g. 2 billion) used to spin the AdvanceDay loop forever —
     * the classic boot freeze. Anything beyond current day + this cap is treated
     * as corruption and clamped (logged).
     */
    static constexpr int32 MaxCatchUpDays = 365;

    bool MigrateV1ToV2(UAstrawildSaveGame* SaveGame) const;

    /** v2 -> v3 is purely additive (new fields default-init) — logged for traceability. */
    void MigrateV2ToV3(UAstrawildSaveGame* SaveGame) const;

    /** v3 -> v4 is purely additive (world events + POIs default-init). */
    void MigrateV3ToV4(UAstrawildSaveGame* SaveGame) const;

    /** v4 -> v5 (Final Run): purely additive — ending state defaults to None
     *  (story in play) and post-game stays locked on legacy saves. */
    void MigrateV4ToV5(UAstrawildSaveGame* SaveGame) const;

    /**
     * LCP-4: in-session per-player blocks (reconnect restore source). Lives on
     * the HOST's GameInstance subsystem — never on clients (PART 7: the host
     * owns the authoritative world save; clients never write world state).
     */
    TMap<FName, FAstrawildCoopPlayerSaveBlock> SessionPlayerBlocks;
};
