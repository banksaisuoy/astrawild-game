#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstrawildTypes.h"
#include "AstrawildEchoDexSubsystem.generated.h"

class UAstrawildEchoDefinition;
class UAstrawildItemRegistrySubsystem;

/** Paldeck-style discovery ladder per species (Unknown -> Seen -> Captured). */
UENUM(BlueprintType)
enum class EAstrawildDexDiscoveryState : uint8
{
    Unknown UMETA(DisplayName = "Unknown"),
    Seen UMETA(DisplayName = "Seen"),
    Captured UMETA(DisplayName = "Captured")
};

/** Search + facet query (Aniimo-index style). Empty text = browse all. */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildDexQuery
{
    GENERATED_BODY()

    /** Free text matched against display name AND definition id,
     *  case-insensitive substring (the Palworld gap the mod closes). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASTRAWILD|Dex")
    FString Text;

    /** Element facet — None = all elements. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASTRAWILD|Dex")
    EAstrawildElementType Element = EAstrawildElementType::None;

    /** Home-zone facet — None = all zones. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASTRAWILD|Dex")
    EAstrawildZone Zone = EAstrawildZone::None;

    /** Role facet (only applied when bFilterRole). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASTRAWILD|Dex")
    bool bFilterRole = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASTRAWILD|Dex")
    EAstrawildEchoRole Role = EAstrawildEchoRole::Support;

    /** Discovery facet (only applied when bFilterDiscovery). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASTRAWILD|Dex")
    bool bFilterDiscovery = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASTRAWILD|Dex")
    EAstrawildDexDiscoveryState Discovery = EAstrawildDexDiscoveryState::Unknown;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildDexUpdatedSignature, FName, SpeciesId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildDexRadarAlertSignature, FName, SpeciesId);

/**
 * EDX-1 "EchoDex" — the searchable species index (Paldeck/Aniimo-style).
 *
 * Evidence-based design:
 *  - Palworld's Paldeck ships sort/filter but NO name search (players route
 *    through external wikis; the community fix is the "Pal List Searchbar"
 *    mod: search + highlight + jump-to-entry). EchoDex ships all three.
 *  - Aniimo's index adds element/role facets, map-side spawn hints and a
 *    radar alert that unlocks at ~80% regional collection progress.
 *    EchoDex mirrors the facets and the 80% radar rule.
 *
 * Layering: "Seen" is derived from the JournalSubsystem (any entry with an
 * encounter already recorded — no duplicate perception bookkeeping), while
 * "Captured ever" is tracked here because no other system persists it.
 * The capture set is runtime state in v1 (save-schema lift is a follow-up).
 */
UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildEchoDexSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // -- Lifecycle ---------------------------------------------------------

    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    // -- Search ------------------------------------------------------------

    /**
     * Pure matcher — world-free, so the automation suite can contract-test
     * search semantics without spawning a world. A definition matches when
     * every active facet passes AND (text empty OR name/id contains it).
     * The discovery facet is evaluated against the caller-supplied state
     * (defaults to Unknown for world-free contract tests).
     */
    static bool MatchesQuery(const UAstrawildEchoDefinition* Def, const FAstrawildDexQuery& Query,
        EAstrawildDexDiscoveryState State = EAstrawildDexDiscoveryState::Unknown);

    /** All registry species that satisfy the query, ordered by DefinitionId
     *  (stable Paldeck numbering — "sort by No." like the Aniimo index). */
    UFUNCTION(BlueprintCallable, Category = "ASTRAWILD|Dex")
    TArray<UAstrawildEchoDefinition*> Search(const FAstrawildDexQuery& Query) const;

    /** Direct registry lookup — keeps detail selection working even when the
     *  species is filtered out of the currently visible list. */
    UFUNCTION(BlueprintPure, Category = "ASTRAWILD|Dex")
    UAstrawildEchoDefinition* FindSpecies(FName SpeciesId) const;

    /** Total indexed species (the canon 229 across 12 zones). */
    UFUNCTION(BlueprintPure, Category = "ASTRAWILD|Dex")
    int32 GetTotalSpeciesCount() const;

    // -- Discovery state ---------------------------------------------------

    /** Seen = journal entry with an encounter; Captured = capture-ever set. */
    UFUNCTION(BlueprintPure, Category = "ASTRAWILD|Dex")
    EAstrawildDexDiscoveryState GetDiscoveryState(FName SpeciesId) const;

    /** Counts species with state >= Seen. */
    UFUNCTION(BlueprintPure, Category = "ASTRAWILD|Dex")
    int32 GetSeenCount() const;

    /** Counts species with state == Captured. */
    UFUNCTION(BlueprintPure, Category = "ASTRAWILD|Dex")
    int32 GetCapturedCount() const;

    /**
     * Records a capture (capture-ever). Returns true when the state actually
     * changed. Idempotent — repeated captures of the same species are no-ops.
     */
    UFUNCTION(BlueprintCallable, Category = "ASTRAWILD|Dex")
    bool NotifySpeciesCaptured(FName SpeciesId);

    // -- Zone progress + radar (Aniimo rule) --------------------------------

    /** 0..1 — share of the zone's species with state >= Seen. */
    UFUNCTION(BlueprintPure, Category = "ASTRAWILD|Dex")
    float GetZoneProgress(EAstrawildZone Zone) const;

    /**
     * Aniimo radar rule: a zone unlocks the radar at >= 80% progress; the
     * radar fires for STILL-UNKNOWN rare species (CaptureDifficulty >=
     * RareDifficultyThreshold). Pure — callers decide how to surface it.
     */
    UFUNCTION(BlueprintPure, Category = "ASTRAWILD|Dex")
    bool ShouldRadarAlert(FName SpeciesId) const;

    /** Fires the radar alert event for a rare nearby species (hooked by the
     *  Echo AI perception path) — HUD/screen listeners toast it. */
    UFUNCTION(BlueprintCallable, Category = "ASTRAWILD|Dex")
    void BroadcastRadarAlert(FName SpeciesId);

    /** Min capture difficulty for the radar to consider a species "rare". */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASTRAWILD|Dex", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RareDifficultyThreshold = 0.75f;

    /** Zone completion share that unlocks the radar (Aniimo: ~0.8). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASTRAWILD|Dex", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RadarUnlockProgress = 0.8f;

    // -- Events ------------------------------------------------------------

    /** One species changed state (Seen/Captured transitions). */
    UPROPERTY(BlueprintAssignable, Category = "ASTRAWILD|Dex")
    FAstrawildDexUpdatedSignature OnDexUpdated;

    /** Radar ping for a rare nearby species (Aniimo-style alert). */
    UPROPERTY(BlueprintAssignable, Category = "ASTRAWILD|Dex")
    FAstrawildDexRadarAlertSignature OnRadarAlert;

private:
    UAstrawildItemRegistrySubsystem* GetRegistry() const;

    /** Capture-ever set — runtime v1 (save lift is a tracked follow-up). */
    TSet<FName> CapturedSpecies;
};
