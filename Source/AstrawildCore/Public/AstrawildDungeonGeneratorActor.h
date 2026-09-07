#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildDungeonRoomActor.h"
#include "AstrawildDungeonGeneratorActor.generated.h"

class AAstrawildDungeonGateActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildDungeonCompleted, class AAstrawildDungeonGeneratorActor*, Dungeon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildDungeonProgress, int32, RoomsCleared, int32, TotalRooms);

/**
 * Hand-authored modular dungeon generator (directive §23): a linear chain of
 * rooms — Entry → Combat → (Puzzle) → Elite → Boss → Exit — built procedurally
 * from room templates so no .umap is required. Gates stay sealed until the
 * previous room clears (Batch 6 implements the gate actor for real).
 *
 * Server-authoritative: generation and progression run on the server only.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildDungeonGeneratorActor : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildDungeonGeneratorActor();

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Dungeon")
    FAstrawildDungeonCompleted OnDungeonCompleted;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Dungeon")
    FAstrawildDungeonProgress OnDungeonProgress;

    /** Stable dungeon id — the save system maps records to generators through it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    FName DungeonId = TEXT("Dungeon_HollowUnderlight");

    /** Total rooms including entry and boss. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon", meta=(ClampMin="3", ClampMax="12"))
    int32 RoomCount = 5;

    /** Distance between consecutive room centers (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon", meta=(ClampMin="800.0"))
    float RoomSpacing = 2200.0f;

    /** Creature pool cycled through combat rooms. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    TArray<FName> CreaturePoolIds;

    /** Boss species definition id. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    FName BossDefinitionId = TEXT("Echo_Gloomfang");

    /**
     * Batch 8: event id the boss publishes on defeat (quest matcher target).
     * Distinct per dungeon so the Sunken Vault warden completes its own quest,
     * not the Underlight one.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    FName BossDefeatEventId = TEXT("Creature_UnderlightWarden");

    /** Audit C-4: research points granted to the shared pool when the dungeon is completed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon", meta=(ClampMin="0"))
    int32 DungeonCompletionResearchPoints = 10;

    /**
     * Batch 6: unique technology granted (cost-free, prereq-free) on first
     * completion — roadmap V3 §21 "bosses drop a unique technology reward".
     * The Ancient era opens ONLY through the Hollow Underlight.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    FName RewardTechnologyId = TEXT("Tech_AncientResonance");

    // --- Final Run (FR-7): per-dungeon boss overrides (Eye of the Maelstrom) ---

    /**
     * FR-7: when set, the boss room's clear loot table is this id instead of
     * the generic Loot_DungeonBoss (the Sovereign drops Loot_EyeCore — the
     * Sovereign Core + Maelstrom Glass).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    FName BossLootTableId = NAME_None;

    /** FR-7: when set, the boss summons THIS species in phase 2 (Eye Sentinels). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    FName BossSummonSpeciesId = NAME_None;

    virtual void BeginPlay() override;

    /** Build the room chain (server). Deterministic given the world seed. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Dungeon")
    void Generate();

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Dungeon")
    int32 GetRoomsCleared() const { return RoomsCleared; }

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Dungeon")
    TArray<AAstrawildDungeonRoomActor*> GetRooms() const { return Rooms; }

    /** Batch 6 — gap M-7: snapshot for the save subsystem. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Dungeon")
    FAstrawildDungeonSaveData ExportForSave() const;

    /** Batch 6 — gap M-7: apply a save record onto the generated rooms (server). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Dungeon")
    void ApplySavedState(const FAstrawildDungeonSaveData& Data);

private:
    UPROPERTY()
    TArray<TObjectPtr<AAstrawildDungeonRoomActor>> Rooms;

    /** Gates[i] seals the passage between room i and room i+1; opens when room i clears. */
    UPROPERTY()
    TArray<TObjectPtr<AAstrawildDungeonGateActor>> Gates;

    int32 RoomsCleared = 0;
    FRandomStream RandomStream;

    FAstrawildDungeonRoomTemplate MakeTemplate(int32 RoomIndex) const;
    UFUNCTION()
    void HandleRoomCleared(AAstrawildDungeonRoomActor* Room, int32 ClearedRoomIndex);
};
