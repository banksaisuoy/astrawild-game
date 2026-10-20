#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstrawildTypes.h"
#include "AstrawildJournalSubsystem.generated.h"

class AAstrawildEchoCharacter;
class AAstrawildPlayerCharacter;
class UAstrawildItemRegistrySubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildJournalUpdated, FName, EchoDefinitionId, const FAstrawildJournalEntry&, Entry);

/**
 * CGC-5 (audit 2-d): one row of the per-zone completion census — how many of
 * the species that call this zone home have been observed. Plain struct (the
 * FAstrawildBestiaryRow precedent): never serialized, presentation-only.
 */
struct FAstrawildZoneCensusRow
{
    /** The zone itself (enum key — the bestiary HomeZone join; names resolve
     *  through the zone table, the map's zone-name source). */
    EAstrawildZone Zone = EAstrawildZone::None;

    /** Observed species whose HomeZone is this zone. */
    int32 Observed = 0;

    /** Total species whose HomeZone is this zone. */
    int32 Total = 0;
};

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

    // ------------------------------------------------------------------
    // CGC-5 (audit 2-d) — collection long-tail: per-zone census + titles.
    // All pure statics (world-free); the live getters read the Entries store.
    // ------------------------------------------------------------------

    /** Canonical "any real contact" rule (the journal screen's discovered test,
     *  single-sourced here so the census and the list can never disagree). */
    static bool IsJournalEntryObserved(const FAstrawildJournalEntry& Entry);

    /**
     * CGC-5: observed species ids (any real contact — encounters, partial
     * progress, any knowledge flag). The input set of ComputeZoneCensus.
     */
    TSet<FName> GetObservedSpeciesIds() const;

    /** CGC-5: species count with the scanned flag (the "species studied" mile). */
    int32 GetScannedSpeciesCount() const;

    /**
     * CGC-5: PURE per-zone completion census — one row per real zone (enum
     * order, all 12), Total = species whose HomeZone is the zone (reuses the
     * CGC-1 bestiary view — CollectBestiaryRows — never a second join).
     * World-free; null registry fail-closes to 12 zeroed rows.
     */
    static TArray<FAstrawildZoneCensusRow> ComputeZoneCensus(
        UAstrawildItemRegistrySubsystem* Registry, const TSet<FName>& ObservedSpeciesIds);

    /**
     * CGC-5: PURE crossing rule — the observation milestone title earned by
     * moving PreviousKnownCount -> NewKnownCount (species studied). Crossing
     * semantics Previous < T <= New fire EXACTLY once, stateless (no save
     * flags); the HIGHEST crossed threshold wins (a load-grant jump 0 -> 229
     * announces the journal-complete title, not every step). Empty when no
     * threshold is crossed. Thresholds: 25 / 50 / 100 / 150 / 200 / 229.
     */
    static FString GetMilestoneTitleForCrossing(const int32 PreviousKnownCount, const int32 NewKnownCount);

    /** CGC-5: PURE live derivation — the highest title whose threshold is at
     *  or below the count ("" before the 25th species). Stateless: derived
     *  from the live scanned count every time the journal summary renders. */
    static FString GetCurrentTitleForCount(const int32 KnownCount);

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

    /** FPP-1: shared cone/progress observation for one candidate (Echo or boss). */
    void ObserveCandidate(AAstrawildPlayerCharacter* Player, const FName DefinitionId,
        const FVector& TargetLocation, float DeltaTime);
    void GrantKnowledgeMilestones(FAstrawildJournalEntry& Entry, const FName DefinitionId);
    class UAstrawildResearchSubsystem* GetResearch() const;

    /** Production V2: equipped scanner range multiplier (1 = stock/no scanner). */
    static float GetScannerRangeMultiplier(const AAstrawildPlayerCharacter* Player);
};
