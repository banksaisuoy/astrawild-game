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
    uint32 IntegrityChecksum = 0;

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

    /** FNV-1a integrity hash for the save header fields. */
    static uint32 ComputeChecksum(int32 SchemaVersion, const FDateTime& SavedAtUtc);

private:
    static constexpr int32 CurrentSchemaVersion = 2;

    bool MigrateV1ToV2(UAstrawildSaveGame* SaveGame) const;
};
