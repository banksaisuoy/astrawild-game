#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstrawildTypes.h"
#include "AstrawildJournalSubsystem.generated.h"

class AAstrawildEchoCharacter;
class AAstrawildPlayerCharacter;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildJournalUpdated, FName, EchoDefinitionId, const FAstrawildJournalEntry&, Entry);

/**
 * Field Journal (directive §20): automatic observation of creatures in the player's
 * view. Knowledge is progression — scan/food/habitat/weakness unlock gradually and
 * feed capture bonuses + research points.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildJournalSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    UAstrawildJournalSubsystem();

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Journal")
    FAstrawildJournalUpdated OnJournalUpdated;

    /** Observation progress per real second while a creature is in clear view. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Journal", meta=(ClampMin="0.0"))
    float ObservationProgressPerSecond = 5.0f;

    /** Max distance for observation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Journal", meta=(ClampMin="100.0"))
    float ObservationDistance = 1400.0f;

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Journal")
    const FAstrawildJournalEntry* FindEntry(const AAstrawildEchoCharacter* Echo) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Journal")
    FAstrawildJournalEntry GetEntry(FName EchoDefinitionId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Journal")
    TArray<FAstrawildJournalEntry> GetAllEntries() const;

    /** Research points awarded for completing an observation. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Journal", meta=(ClampMin="0"))
    int32 ObservationResearchReward = 2;

    void ExportForSave(TArray<FAstrawildJournalEntry>& OutEntries) const;
    void ImportFromSave(const TArray<FAstrawildJournalEntry>& InEntries);

    // --- Final production run (PHASE 12): active scanner framework ---

    /** Hold-to-scan: accelerate observation while the equipped scanner is held. */
    void BeginActiveScan(AAstrawildPlayerCharacter* Scanner, float Multiplier);

    /** Release: observation returns to the passive rate. */
    void EndActiveScan();

    /** True while the given player is actively scanning. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Journal")
    bool IsScanActiveFor(const AAstrawildPlayerCharacter* Player) const;

    /**
     * Final production run: drone observation feed — adds progress for a creature
     * regardless of the player's view direction (utility drone scan pulses).
     * Runs the same milestone pipeline as direct observation.
     */
    void AddExternalObservation(const AAstrawildEchoCharacter* Echo, float Progress);

protected:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
    TMap<FName, FAstrawildJournalEntry> Entries;

    /** Throttle accumulator for the observation sweep (0.5s cadence). */
    float ObservationSweepAccumulator = 0.0f;

    /** Active scanner state (weak — a disconnect/destroy simply ends the scan). */
    TWeakObjectPtr<AAstrawildPlayerCharacter> ActiveScanner;
    float ActiveScanMultiplier = 1.0f;

    void ObservePlayer(AAstrawildPlayerCharacter* Player, float DeltaTime);
    void GrantKnowledgeMilestones(FAstrawildJournalEntry& Entry, const FName DefinitionId);
    class UAstrawildResearchSubsystem* GetResearch() const;
};
