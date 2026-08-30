#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstrawildHostileSpawnerSubsystem.generated.h"

class AAstrawildPlayerCharacter;
class UAstrawildEchoDefinition;

/**
 * Hostile respawn spawner (Batch 2 — Item A). Completes Quest 5 ("Defeat 3 Gloomfang"
 * chain) by guaranteeing a steady supply of hostile Echoes around the player's base
 * camp on the server. Clamps per-species population via EcosystemSubsystem tracking
 * so the world never overflows.
 *
 * Auto-registers as a UTickableWorldSubsystem participant (server-only tick).
 * Quest hooks already fire from AAstrawildEchoCharacter::ApplyElementalDamage →
 * OnDefeated → EventBus (TAG_Astrawild_Event_HostileDefeated) → QuestComponent, so
 * no new quest wiring is required — kills auto-increment Quest 5's DefeatCreature
 * objective.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildHostileSpawnerSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    UAstrawildHostileSpawnerSubsystem();

    /** cm-radius around the player pawn where new hostiles can spawn. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Spawner", meta=(ClampMin="500.0"))
    float SpawnRadius = 1800.0f;

    /** Target wild population for Echo_Gloomfang (Quest 5 objective target). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Spawner", meta=(ClampMin="0"))
    int32 TargetGloomfangPopulation = 4;

    /** Target wild population for Echo_Emberfang. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Spawner", meta=(ClampMin="0"))
    int32 TargetEmberfangPopulation = 2;

    /** Batch 5: target wild population for Echo_Rimefang (Frost predator). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Spawner", meta=(ClampMin="0"))
    int32 TargetRimefangPopulation = 3;

    /** Batch 5: target wild population for Echo_Voltmaw (Pulse glass-cannon — kept low). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Spawner", meta=(ClampMin="0"))
    int32 TargetVoltmawPopulation = 1;

    /** Seconds between spawn sweeps. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Spawner", meta=(ClampMin="1.0"))
    float RespawnIntervalSeconds = 25.0f;

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

protected:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
    /** Accumulated wall-clock seconds since the last spawn sweep. */
    float RespawnAccumulator = 0.0f;

    /** Stable, world-seeded stream for deterministic spawn placement. */
    FRandomStream SpawnStream;

    /** Cache of hostile species ids to track. */
    static constexpr const TCHAR* GloomfangId = TEXT("Echo_Gloomfang");
    static constexpr const TCHAR* EmberfangId  = TEXT("Echo_Emberfang");
    static constexpr const TCHAR* RimefangId   = TEXT("Echo_Rimefang");   // Batch 5 — Frost line.
    static constexpr const TCHAR* VoltmawId    = TEXT("Echo_Voltmaw");    // Batch 5 — Pulse line.

    /** Spawn one hostile of the given species around the player pawn. */
    void SpawnOneHostile(UAstrawildEchoDefinition* Definition, const FVector& Origin);

    /** Resolve the local player pawn (server-side). May return null in early load. */
    AAstrawildPlayerCharacter* FindLocalPlayer() const;
};
