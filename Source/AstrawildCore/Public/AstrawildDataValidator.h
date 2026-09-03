#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AstrawildDataValidator.generated.h"

class UAstrawildItemRegistrySubsystem;
class UWorld;

/**
 * SCP Phase 1 — runtime data validation (directive [3] Phase 1.2).
 *
 * Two layers:
 *  1. World-free static contracts (ValidateStaticTables) — the generated bestiary,
 *     the ability library and the element weakness chain. Callable from automation
 *     tests with no world.
 *  2. Registry validation (ValidateRegistry) — reference integrity across the full
 *     code-default content set: recipe inputs/outputs, tech prerequisites and
 *     unlocks, quest targets, building requirements, echo stat bounds. Runs from
 *     UAstrawildDataValidationSubsystem on world begin and reports every problem
 *     through the error reporter + LogAstrawild so a Standalone build can be
 *     diagnosed without an editor attached.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildDataValidatorLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /**
     * Static-table contracts (world-free):
     *  - bestiary: row count, id uniqueness, stat bounds, zone coverage
     *  - ability library: template count, id uniqueness, numeric bounds
     *  - element chain: canonical weakness chain (Flora->Ember->Frost->Pulse->Light,
     *    Light/Ash unweakable) is a total function over the element set
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Validation")
    static void ValidateStaticTables(TArray<FString>& OutProblems);

    /**
     * Registry contracts (runtime): every reference id that content data claims
     * must resolve inside the same registry. Duplicate ids, dangling recipe
     * inputs/outputs, unknown tech prerequisites, quest targets that no system
     * can ever produce, buildings requiring items that do not exist — all
     * appended as human-readable problem lines. Returns the number of problems.
     */
    static int32 ValidateRegistry(const UAstrawildItemRegistrySubsystem* Registry, TArray<FString>& OutProblems);

    /**
     * FNV-1a identity hash over the sorted definition id sets — a cheap
     * "did the content actually change" tripwire for handoff documentation
     * (mirrors the save checksum approach).
     */
    static uint32 ComputeContentChecksum(const UAstrawildItemRegistrySubsystem* Registry);

    /** Convenience: runs both layers and returns true when zero problems were found. */
    static bool ValidateAll(const UAstrawildItemRegistrySubsystem* Registry, TArray<FString>& OutProblems);
};

/**
 * World-bound runner: validates the live registry right after the content set is
 * built (OnWorldBegin) so the first log of every session carries a definitive
 * data-health verdict for the Standalone build (directive [3] Phase 1.2).
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildDataValidationSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // UWorldSubsystem interface.
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    /** True after a successful (zero problems) validation pass. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Validation")
    bool IsDataValid() const { return bLastValidationClean; }

    /** Number of problems found by the last validation pass. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Validation")
    int32 GetLastProblemCount() const { return LastProblemCount; }

private:
    bool bLastValidationClean = false;
    int32 LastProblemCount = 0;
};
