#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AstrawildTypes.h"
#include "AstrawildEchoRosterSubsystem.generated.h"

class AAstrawildEchoCharacter;

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

    void ExportForSave(TArray<FAstrawildEchoInstanceV2>& OutRoster) const;
    void ImportFromSave(const TArray<FAstrawildEchoInstanceV2>& InRoster);

private:
    TArray<FAstrawildEchoInstanceV2> Roster;

    /** Weak references to spawned party Echoes (world actors). */
    TArray<TWeakObjectPtr<AAstrawildEchoCharacter>> SpawnedParty;
};
