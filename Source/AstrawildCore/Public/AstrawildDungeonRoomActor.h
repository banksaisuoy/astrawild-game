#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildTypes.h"
#include "AstrawildDungeonRoomActor.generated.h"

class UStaticMeshComponent;
class AAstrawildDungeonGateActor;

/** One hand-authored modular room template (directive §23). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildDungeonRoomTemplate
{
    GENERATED_BODY()

    /** Room shape name for the generator (e.g. "Entry", "Combat", "Puzzle", "Elite", "Boss", "Exit"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    FName RoomTypeId = NAME_None;

    /** World-space half-extents of the room (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    FVector HalfExtents = FVector(600.0f, 600.0f, 300.0f);

    /** Relative spawn points for creatures inside the room. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    TArray<FVector> CreatureSpawnOffsets;

    /** Loot table id granted when the room is cleared. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    FName ClearLootTableId = NAME_None;

    /** Boss room spawns the boss definition instead of creatures. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    bool bIsBossRoom = false;
};

/**
 * A spawned dungeon room instance (directive §23): walls (placeholder shapes),
 * encounter creatures, clear detection and rewards. Rooms notify their owning
 * dungeon when cleared; the dungeon unlocks the next gate.
 *
 * Server-authoritative: encounter state lives on the server.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildDungeonRoomActor : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildDungeonRoomActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Dungeon")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    FAstrawildDungeonRoomTemplate Template;

    /** Sequential room index within the dungeon. */
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Dungeon")
    int32 RoomIndex = 0;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Dungeon", Replicated)
    bool bCleared = false;

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Populate the encounter from the template (server). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Dungeon")
    void SpawnEncounter(const TArray<FName>& CreatureDefinitionIds);

    /** Mark cleared, grant rewards, notify the dungeon (server). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Dungeon")
    void MarkCleared();

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildRoomCleared, AAstrawildDungeonRoomActor*, Room, int32, RoomIndex);
    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Dungeon")
    FAstrawildRoomCleared OnRoomCleared;

private:
    TArray<TWeakObjectPtr<class AAstrawildEchoCharacter>> EncounterCreatures;
    float ClearCheckAccumulator = 0.0f;

    void BuildRoomShell();
    bool IsEncounterDefeated() const;
    void GrantClearReward();
};
