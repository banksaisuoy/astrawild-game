#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AstrawildTypes.h"
#include "AstrawildEchoRosterSubsystem.generated.h"

class AAstrawildEchoCharacter;
class APlayerController;
class UAstrawildEchoDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildRosterChanged, int32, RosterSize);

/**
 * Captured-Echo roster (directive §4 Instance / §10 party).
 * Roster lives for the session; persistence flows through the save subsystem.
 * Max 3 active party members spawn in the world; the rest rest in the roster.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildEchoRosterSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Echo")
    FAstrawildRosterChanged OnRosterChanged;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo", meta=(ClampMin="1"))
    int32 MaxPartySize = 3;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo")
    bool AddToRoster(AAstrawildEchoCharacter* Echo);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo")
    bool RemoveFromRoster(const FGuid& InstanceId);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo")
    bool IsInRoster(const FGuid& InstanceId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo")
    TArray<FAstrawildEchoInstanceV2> GetRoster() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo")
    int32 GetRosterSize() const { return Roster.Num(); }

    /** Party members currently spawned in the world. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo")
    TArray<AAstrawildEchoCharacter*> GetSpawnedParty() const;

    /** SCP Phase 9: spawned party echoes currently assigned to base work (garrison). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo")
    int32 GetBaseGarrisonCount() const;

    void ExportForSave(TArray<FAstrawildEchoInstanceV2>& OutRoster) const;
    void ImportFromSave(const TArray<FAstrawildEchoInstanceV2>& InRoster);

    /**
     * Audit H-2: spawn the saved party (up to MaxPartySize) around the player's pawn.
     * Server-side; returns the number of Echoes spawned.
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo")
    int32 SpawnPartyActors(APlayerController* Owner);

    // --- Content Pack CP-02: evolution / progression ---

    /**
     * Pure evolution gate check (automation-tested): the current species must
     * name a target, the target must resolve, and the instance must clear BOTH
     * the level gate and the bond gate.
     */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo|Evolution")
    static bool CanEvolveInstance(const FAstrawildEchoInstanceV2& Instance,
        const UAstrawildEchoDefinition* Definition, const UAstrawildEchoDefinition* TargetDefinition);

    /** True when the roster instance currently meets its evolution gates. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo|Evolution")
    bool CanEvolve(const FGuid& InstanceId) const;

    /**
     * Evolve a roster instance: DefinitionId swaps to the evolved species
     * (stats/rarity/silhouette refresh) while level, bond, trust and personality
     * are preserved — identity survives the transformation. A spawned party actor
     * re-initializes from the new definition (full body rebuild). Server-side.
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo|Evolution")
    bool EvolveInstance(const FGuid& InstanceId);

private:
    TArray<FAstrawildEchoInstanceV2> Roster;

    /** Weak references to spawned party Echoes (world actors). */
    TArray<TWeakObjectPtr<AAstrawildEchoCharacter>> SpawnedParty;
};
