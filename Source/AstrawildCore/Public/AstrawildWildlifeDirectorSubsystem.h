#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstrawildTypes.h"
#include "AstrawildWorldBootstrapper.h"
#include "AstrawildWildlifeDirectorSubsystem.generated.h"

class UAstrawildItemRegistrySubsystem;
class AAstrawildEchoCharacter;
class AAstrawildPlayerCharacter;

/**
 * CGC-1: world-free view of one bestiary species — the spawn plan's input.
 * Mirrors the registry fields the director consumes (definition id, home zone,
 * capture difficulty, primary element) plus the derived evolution-target flag,
 * so the pure plan function needs no engine world to reason about the roster.
 */
struct FAstrawildBestiaryRow
{
    FName SpeciesId = NAME_None;

    /** Home zone of the species (the director places plan individuals here). */
    EAstrawildZone HomeZone = EAstrawildZone::None;

    /** 0..1 authored capture difficulty — scales the planned individual count. */
    float CaptureDifficulty = 0.5f;

    /** Primary element — drives weather-gated rare appearances. */
    EAstrawildElementType PrimaryElement = EAstrawildElementType::None;

    /** True when another species EVOLVES INTO this one (earned, never spawned wild). */
    bool bEvolvedOnlyForm = false;
};

/** CGC-1: one planned director placement (species x home zone x individual count). */
struct FAstrawildDirectorSpawnRow
{
    FName SpeciesId = NAME_None;
    FName ZoneId = NAME_None;
    int32 Count = 0;
};

/**
 * CGC-1 — the wildlife director: closes the collection-integrity gap found by
 * the 4-dimension audit (only ~56 of 229 species were obtainable; 179 generated
 * bestiary rows never spawned anywhere; Hunt_MonolithCull targeted a species
 * placed by nothing). Census discipline: 229 species UNCHANGED — this subsystem
 * makes EXISTING species obtainable, it never registers new content.
 *
 * Four responsibilities, mirroring UAstrawildHostileSpawnerSubsystem's
 * structure/tick discipline (server-only; init from OnWorldBeginPlay + first
 * tick; deterministic seeds from the replicated world seed):
 *
 *  (a) Boot-time bestiary-wide spawn pass — every species with a home zone
 *      that no legacy ZoneWildlife row already covers gets individuals in its
 *      home zone (rare species 1, common 2), breadth-first under a per-zone
 *      cap, executed through the same construction path the bootstrapper's
 *      SpawnWildEchoes uses (definition lookup, InitializeFromDefinition,
 *      terrain-snapped placement, ecosystem registration).
 *
 *  (b) Hour-boundary dormancy turnover (OnHourChanged) — the authored
 *      EAstrawildActivityPattern data finally drives SPAWN COMPOSITION: wild
 *      echoes out of their activity window hide (invisible + collision off +
 *      AI stopped), wake when the window reopens. Engaged creatures (combat /
 *      active statuses / wounded / player in capture range) never dorm. One
 *      uniform rule for every wild echo — legacy and director spawns alike.
 *
 *  (c) Weather-gated rare appearances (OnWeatherChanged) — each of the 8
 *      weather states boosts 2-3 real elements; on a transition 1-2 boosted-
 *      element species homed in the player's zone emerge near the player
 *      (deterministic seed = world seed ^ weather-change counter), with a
 *      player toast in the NotifyPlayer channel.
 *
 *  (d) Day-cycle deficit refill (OnDayChanged) — every plan row + every legacy
 *      ZoneWildlife row refills its deficit against the live wild population
 *      (EcosystemSubsystem::GetWildPopulation — capture/defeat-aware), placed
 *      in the row's home zone under a daily per-zone cap. Cleared worlds no
 *      longer stay empty for the rest of the session.
 *
 * The three decision rules (plan, dormancy, weather map) are PURE STATIC
 * world-free functions (the ComputeVisualBand/ComputeCaptureChance precedent)
 * so the automation suite can pin them without an engine world.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildWildlifeDirectorSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    UAstrawildWildlifeDirectorSubsystem();

    // --- pure static contracts (world-free, automation-tested) ---

    /** Hard cap on NEW director individuals per zone at boot (plan discipline). */
    static constexpr int32 PerZoneSpawnCap = 24;

    /** CaptureDifficulty at/above this = "rare": 1 planned individual instead of 2. */
    static constexpr float RareDifficultyThreshold = 0.75f;

    /**
     * CaptureDifficulty at/above this = legendary one-off tier: the species is
     * placed by its own authored path (the bootstrapper's single Ancient-rare
     * Auroraling, hardest capture 0.95) and the director never duplicates it.
     */
    static constexpr float OneOffLegendaryDifficultyThreshold = 0.9f;

    /**
     * Final-act boss species (FR-7 Glass Tyrant world boss, its Eye Sentinel
     * summons, the Drowned Sovereign dungeon boss). They live behind their
     * authored boss encounters and never spawn as ordinary wild echoes.
     */
    static void GetBossEncounterSpeciesIds(TArray<FName>& OutIds);
    static bool IsBossEncounterSpecies(const FName SpeciesId);

    /**
     * World-free bestiary view from the item registry, sorted by species id
     * (deterministic order). Marks evolved-only forms via the evolution chains.
     */
    static void CollectBestiaryRows(UAstrawildItemRegistrySubsystem* Registry, TArray<FAstrawildBestiaryRow>& OutRows);

    /**
     * PURE spawn plan (world-free testable): every bestiary species with a
     * home zone that is (1) not already covered by any legacy ZoneWildlife
     * row, (2) not a boss-encounter species, (3) not an evolved-only form and
     * (4) not a legendary one-off gets individuals in its home zone —
     * Count = (CaptureDifficulty >= RareDifficultyThreshold ? 1 : 2).
     * Breadth-first under the cap: species are visited in id order so each
     * species receives its 1st individual before any receives its 2nd, and
     * per-zone NEW individuals never exceed PerZoneCap. Rows are emitted
     * sorted by (zone, species) for deterministic execution.
     */
    static TArray<FAstrawildDirectorSpawnRow> ComputeDirectorSpawnPlan(
        const TArray<FAstrawildBestiaryRow>& Bestiary,
        const TArray<FAstrawildZoneWildlifeRow>& LegacyRows,
        const int32 PerZoneCap);

    /**
     * PURE dormancy rule: dormant iff out of the activity window (the shared
     * AAstrawildEchoCharacter::IsActivityWindowOpen math) AND not recently
     * engaged. Engagement always wins — a creature in combat / capture /
     * recently damaged never vanishes on the player.
     */
    static bool ShouldBeDormant(const EAstrawildActivityPattern Pattern, const float HourOfDay, const bool bRecentlyEngaged);

    /**
     * PURE total weather map: elements each weather state boosts. Every state
     * maps non-empty; every real element value (Light/Ash/Flora/Frost/Pulse/
     * Ember) is reachable from at least one state.
     */
    static TArray<EAstrawildElementType> GetWeatherBoostedElements(const EAstrawildWeatherState Weather);

    /**
     * PURE zone resolution: the zone containing the position, or the zone with
     * the nearest center when the position is off the world grid (fail-closed
     * Dawn Fields when no zone table entry exists at all).
     */
    static EAstrawildZone ResolveNearestZone(const FVector2D& WorldXY);

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

protected:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
    // --- broadcast-signal handlers (subscribed server-side, wired once in Tick) ---

    UFUNCTION()
    void HandleHourChanged(const int32 HourOfDay);

    UFUNCTION()
    void HandleDayChanged(const int32 NewDayNumber, const bool bWasAutomatic);

    UFUNCTION()
    void HandleWeatherChanged(const EAstrawildWeatherState NewState, const EAstrawildWeatherState OldState);

    /** One-time signal wiring (the HuntSubsystem wired-once idiom; server only). */
    void WireBroadcastSignals();

    /** Bestiary-wide boot pass (runs after the bootstrapper finished its own spawns). */
    void RunBootSpawnPass();

    /** Uniform hour turnover: dorm/wake every wild echo per the activity rule. */
    void ApplyHourTurnover(const float HourOfDay);

    /** Hide/freeze or reveal/resume one wild echo (the ExecuteSleep stop idiom). */
    void ApplyDormancy(AAstrawildEchoCharacter* Echo, const bool bDormant);

    /**
     * Engagement guard: in combat now / active status effects / wounded
     * (wild echoes do not regenerate) / a player inside capture approach range.
     */
    bool IsEchoEngaged(const AAstrawildEchoCharacter* Echo);

    /** Weather-gated rare appearance near the player's zone (c). */
    void HandleWeatherRareAppearance(const EAstrawildWeatherState NewState);

    /** Day-cycle deficit refill for plan + legacy rows under a daily cap (d). */
    void HandleDayRefill(const int32 DayNumber);

    /** Shared spawn helper — the SpawnWildEchoes construction path, own stream. */
    AAstrawildEchoCharacter* SpawnWildEchoInZone(const FName SpeciesId, const FAstrawildZoneDescriptor& Zone, FRandomStream& Stream);

    /** Random ground-snapped point inside a zone (the bootstrapper's RandomPointInZone semantics). */
    static FVector RandomGroundPointInZone(FRandomStream& Stream, const FAstrawildZoneDescriptor& Zone, const int32 Seed);

    /** Resolve the local player pawn (server-side). May return null in early load. */
    AAstrawildPlayerCharacter* FindLocalPlayer() const;

    /** HUD toast through the NotifyPlayer channel (the WorldEventSubsystem wording path). */
    void BroadcastToast(const FText& Message) const;

    /** Signals wired (OnHourChanged/OnDayChanged/OnWeatherChanged subscribers). */
    bool bSignalsWired = false;

    /** Boot pass completed (one shot per world lifetime). */
    bool bBootPassDone = false;

    /** Accumulated wait for a bootstrapper that never appears (defensive). */
    float BootPassWaitElapsed = 0.0f;

    /** Monotonic weather-transition counter (deterministic rare-appearance seed). */
    int32 WeatherChangeCounter = 0;

    /** Replicated world seed (cached; re-read at boot-pass time like HostileSpawner). */
    int32 CachedWorldSeed = 1337;

    /** Bestiary view + legacy table + computed plan (kept for the refill pass). */
    TArray<FAstrawildBestiaryRow> BestiaryRows;
    TArray<FAstrawildZoneWildlifeRow> LegacyRows;
    TArray<FAstrawildDirectorSpawnRow> SpawnPlan;

    /** NEW individuals placed per zone id since the last day boundary (daily cap). */
    TMap<FName, int32> DailyZoneSpawnCount;

    /** Defensive cap on the boot-pass wait for worlds without a bootstrapper. */
    static constexpr float BootPassWaitTimeoutSeconds = 10.0f;

    /** A player within this range keeps wild echoes awake (capture approach proxy). */
    static constexpr float EngagementRadius = 2500.0f;
};
