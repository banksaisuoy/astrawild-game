#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildDungeonRoomActor.h"
#include "AstrawildDungeonGeneratorActor.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildDungeonCompleted, class AAstrawildDungeonGeneratorActor*, Dungeon);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildDungeonProgress, int32, RoomsCleared, int32, TotalRooms);

/**
 * Hand-authored modular dungeon generator (directive §23): a linear chain of
 * rooms — Entry → Combat → (Puzzle) → Elite → Boss → Exit — built procedurally
 * from room templates so no .umap is required. Gates stay sealed until the
 * previous room clears.
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

    /** Audit C-4: research points granted to the shared pool when the dungeon is completed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon", meta=(ClampMin="0"))
    int32 DungeonCompletionResearchPoints = 10;

    virtual void BeginPlay() override;

    /** Build the room chain (server). Deterministic given the world seed. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Dungeon")
    void Generate();

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Dungeon")
    int32 GetRoomsCleared() const { return RoomsCleared; }

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Dungeon")
    TArray<AAstrawildDungeonRoomActor*> GetRooms() const { return Rooms; }

private:
    UPROPERTY()
    TArray<TObjectPtr<AAstrawildDungeonRoomActor>> Rooms;

    int32 RoomsCleared = 0;
    FRandomStream RandomStream;

    FAstrawildDungeonRoomTemplate MakeTemplate(int32 RoomIndex) const;
    UFUNCTION()
    void HandleRoomCleared(AAstrawildDungeonRoomActor* Room, int32 ClearedRoomIndex);
};
