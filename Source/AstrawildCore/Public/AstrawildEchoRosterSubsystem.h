#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AstrawildTypes.h"
#include "AstrawildEchoRosterSubsystem.generated.h"

class AAstrawildEchoCharacter;
class APlayerController;
class AAstrawildPlayerController;
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
    bool AddToRoster(AAstrawildEchoCharacter* Echo, FName PlayerKey = NAME_None);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo")
    bool RemoveFromRoster(const FGuid& InstanceId);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo")
    bool IsInRoster(const FGuid& InstanceId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo")
    TArray<FAstrawildEchoInstanceV2> GetRoster() const;

    /**
     * LCP-4: this player's roster slice (shared host pool, partitioned by the
     * STABLE owner key). NAME_None keys return the legacy/undefined-owner rows
     * (single-player behavior preserved).
     */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo")
    TArray<FAstrawildEchoInstanceV2> GetRosterForPlayer(FName PlayerKey) const;

    /**
     * LCP-4 (world-free testable): the partition rule — a row belongs to the
     * player when its OwnerPlayerKey matches, or when both are the legacy
     * host/undefined identity (NAME_None). One shared pool, per-player views.
     */
    static bool IsRosterRowOwnedBy(const FAstrawildEchoInstanceV2& Row, FName PlayerKey);

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

    // --- PCR-2: the roster/party management surface ---

    /**
     * PCR-2 (world-free testable): the party-ring eligibility predicate — a
     * roster row spawns in the ring when it is a captured member (bInParty),
     * carries a valid instance identity, and is NOT benched. The pure form
     * keeps the spawn path and the automation contract on one rule.
     */
    static bool ShouldSpawnInPartyRing(const FAstrawildEchoInstanceV2& Row);

    /**
     * PCR-2: bench/unbench one of this player's captured Echoes (authority +
     * ownership validated; the ring rebuilds immediately — benched actors
     * despawn, unbenched ones spawn around the owner). Returns false when the
     * instance is unknown, not owned by the requester, or called off-authority.
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo")
    bool SetInstanceBenched(const FGuid& InstanceId, bool bBenched, APlayerController* Owner);

    /**
     * PCR-2: refresh every connected player's replicated roster mirror (the
     * LCP-5 "no client roster UI" note is superseded by the PCR-2 screen —
     * each remote PC now carries its own roster slice for read-only display;
     * mutations stay server-authoritative through the controller RPC).
     */
    void PushRosterMirrors();

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
