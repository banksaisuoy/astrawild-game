#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildTypes.h"
#include "AstrawildDungeonRoomActor.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class AAstrawildDungeonGateActor;
class AAstrawildResonancePillarActor;
class AAstrawildBossHazardActor;
class AAstrawildWaterPlaneActor;

/**
 * DP-9 (dungeon depth): per-dungeon theme identity — resolved from the
 * generator's stable DungeonId so the three canonical dungeons read
 * differently IN-ROOM without a single bootstrapper literal changing.
 */
UENUM(BlueprintType)
enum class EAstrawildDungeonTheme : uint8
{
    None UMETA(DisplayName = "Unthemed (legacy placeholder shell)"),
    HollowUnderlight UMETA(DisplayName = "Hollow Underlight — dark, tight, ash-choked"),
    SunkenVault UMETA(DisplayName = "Sunken Vault — wide, flooded, reed-lit"),
    MaelstromEye UMETA(DisplayName = "Eye of the Maelstrom — tall monoliths, pulsing storm light")
};

/**
 * DP-9: room-level hazard identity while a room is UNCLEARED. Reuses the
 * existing hazard/status vocabulary only (no new damage verbs):
 *   - AshLung      -> stamina-regen suppression status (the DP-7 zone verb, clamped)
 *   - Waterlogged  -> movement slow status (the Chill/Shock SpeedMultiplier verb)
 *   - EnergyPulse  -> periodic AAstrawildBossHazardActor tiles (the boss-arena pattern)
 */
UENUM(BlueprintType)
enum class EAstrawildRoomHazardType : uint8
{
    None UMETA(DisplayName = "No room hazard"),
    AshLung UMETA(DisplayName = "Ash lung — suppressed stamina regen while uncleared"),
    Waterlogged UMETA(DisplayName = "Waterlogged — slowed movement while uncleared"),
    EnergyPulse UMETA(DisplayName = "Energy pulse — periodic hazard tiles while uncleared")
};

/**
 * DP-9: per-dungeon theme data (plain struct, same house style as
 * FAstrawildDressingProfile). Every field is derived table data resolved by
 * MakeThemeProfile — the room shell, dressing, light, water accent and hazard
 * all read from ONE profile so the three dungeons are distinct at a glance.
 */
struct ASTRAWILDCORE_API FAstrawildDungeonThemeProfile
{
    EAstrawildDungeonTheme Theme = EAstrawildDungeonTheme::None;

    /** Floor + wall tint (ResourceNode MID "Color" idiom). */
    FLinearColor ShellTint = FLinearColor::White;

    /** Accent point-light tint / intensity (0 = no accent light). */
    FLinearColor AccentLightTint = FLinearColor::White;
    float AccentLightIntensity = 0.0f;

    /** Pulse rate of the accent light (cycles/s; 0 = steady — the Eye's storm pulse). */
    float AccentLightPulseRate = 0.0f;

    /** Room footprint multiplier applied by the generator (tight / wide / standard). */
    FVector ExtentScale = FVector(1.0f, 1.0f, 1.0f);

    /** Perimeter side-wall height (cm; 0 = no side walls — the legacy shell). */
    float SideWallHeight = 0.0f;
    float SideWallThickness = 80.0f;

    /** ArtPack biome bindings for room dressing vocabulary (existing mesh paths only). */
    FName RockBiomeId = NAME_None;
    FName FloraBiomeId = NAME_None;
    /** Optional ArtPack resource-node mesh accent (the Eye's ancient-tech vein). */
    FName TechNodeArtId = NAME_None;

    int32 RockCount = 0;
    int32 FloraCount = 0;
    int32 TechCount = 0;
    float DressingScaleMin = 0.8f;
    float DressingScaleMax = 1.3f;
    /** Vertical rock elongation (1.0 natural; the Eye stretches rocks into monolith pillars). */
    float RockElongation = 1.0f;

    /** Sunken Vault only: flooded-floor water-plane accent. */
    bool bWaterFloorAccent = false;

    /** Room-level hazard identity (active while uncleared). */
    EAstrawildRoomHazardType Hazard = EAstrawildRoomHazardType::None;
    /** AshLung: stamina-regen suppression per second (mild, clamped at zero net regen). */
    float HazardPressure = 0.0f;
    /** Waterlogged: movement speed multiplier (< 1 slows). */
    float HazardSpeedMultiplier = 1.0f;
    /** EnergyPulse: cadence between tile waves (s), tile count / radius / dps / lifetime. */
    float HazardPulseInterval = 0.0f;
    int32 HazardTileCount = 0;
    float HazardTileRadius = 0.0f;
    float HazardTileDamagePerSecond = 0.0f;
    float HazardTileLifetime = 0.0f;
};

/** One hand-authored modular room template (directive §23). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildDungeonRoomTemplate
{
    GENERATED_BODY()

    /** Room shape name for the generator (e.g. "Entry", "Combat", "Puzzle", "Elite", "Boss", "Exit"). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    FName RoomTypeId = NAME_None;

    /** World-space half-extents of the room (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    FVector HalfExtents = FVector(600.0f, 600.0f, 300.0f);

    /** Relative spawn points for creatures inside the room. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    TArray<FVector> CreatureSpawnOffsets;

    /** Loot table id granted when the room is cleared. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    FName ClearLootTableId = NAME_None;

    /** Batch 8: defeat event id published by this room's boss (per-dungeon quest target). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    FName BossDefeatEventId = TEXT("Creature_UnderlightWarden");

    /** Boss room spawns the boss definition instead of creatures. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    bool bIsBossRoom = false;

    /**
     * DP-9 (additive): per-dungeon theme identity — the generator resolves it
     * from DungeonId and the room builds its themed shell/dressing/hazard from
     * the profile table. None = the legacy placeholder shell (fail-closed
     * default; pre-DP-9 rooms byte-identical).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    EAstrawildDungeonTheme Theme = EAstrawildDungeonTheme::None;

    /**
     * DP-9 (additive): puzzle rooms spawn resonance pillars and only clear when
     * the attunement sequence is solved (plus the encounter defeated).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    bool bRequiresPuzzleSolve = false;
};

/**
 * A spawned dungeon room instance (directive §23): walls (placeholder shapes),
 * encounter creatures, clear detection and rewards. Rooms notify their owning
 * dungeon when cleared; the dungeon unlocks the next gate.
 *
 * DP-9 (dungeon depth): the room shell is now themed per dungeon — tinted
 * floor + perimeter side walls + accent light + deterministic ArtPack dressing
 * (+ the Sunken Vault's water-plane accent) — and uncleared rooms carry a
 * per-dungeon hazard (ash lung / waterlogged / energy-pulse tiles). Puzzle
 * rooms run the resonance-pillar attunement sequence (see
 * AAstrawildResonancePillarActor).
 *
 * Server-authoritative: encounter state lives on the server. Shell visuals are
 * server-authored in the shared single-player/listen-server world — the same
 * posture as the existing floor plate (Template is not replicated).
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildDungeonRoomActor : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildDungeonRoomActor();

    // --- DP-9 pure statics (world-free, automation-tested) ---

    /**
     * DP-9: theme resolution per dungeon id. The 3 canonical dungeons resolve
     * 3 distinct themes; unknown/empty ids fail closed to None (the legacy
     * unthemed shell — identity never breaks a dungeon).
     */
    static EAstrawildDungeonTheme ResolveDungeonTheme(const FName DungeonId);

    /** DP-9: the theme data table (shell tint/proportions/dressing/hazard). */
    static FAstrawildDungeonThemeProfile MakeThemeProfile(EAstrawildDungeonTheme Theme);

    /** DP-9: status id of the room-level ash-lung (Underlight uncleared rooms). */
    static FName GetRoomAshLungStatusId() { return TEXT("AshLung"); }

    /** DP-9: status id of the room-level waterlogged slow (Sunken Vault uncleared rooms). */
    static FName GetRoomWaterloggedStatusId() { return TEXT("Waterlogged"); }

    /**
     * DP-9: resonance-pillar sequence result. Pillars carry their required
     * order position; the attuned pillar must be the NEXT one or the whole
     * sequence resets.
     */
    enum class EPillarActivityResult : uint8
    {
        Advanced,      ///< The attuned pillar was the next in sequence.
        Completed,     ///< The final pillar of the sequence — puzzle solved.
        ResetRequired  ///< Wrong order (or stale input) — sequence resets.
    };

    /**
     * DP-9: pure sequence evaluation — AttemptedPillarIndex must equal
     * ActivatedCount to advance; the last pillar completes; anything else
     * resets. Out-of-range inputs fail closed to a reset.
     */
    static EPillarActivityResult EvaluatePillarActivation(int32 AttemptedPillarIndex, int32 ActivatedCount, int32 TotalPillars);

    /** DP-9: true when the attunement window elapsed (the sequence resets). */
    static bool IsPillarSequenceExpired(float ElapsedSeconds, float WindowSeconds);

    /** DP-9: resonance pillars per puzzle room. */
    static int32 GetPuzzlePillarCount() { return 3; }

    /** DP-9: attunement window (s) — generous for a guarded room. */
    static float GetPuzzleSequenceWindowSeconds() { return 45.0f; }

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Dungeon")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    /** DP-9: perimeter side walls (±Y of the chain axis — the ±X faces stay open for the gates). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Dungeon")
    TObjectPtr<UStaticMeshComponent> LeftWallMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Dungeon")
    TObjectPtr<UStaticMeshComponent> RightWallMesh;

    /** DP-9: themed accent light (the Eye's storm pulse; hidden until themed). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Dungeon")
    TObjectPtr<UPointLightComponent> RoomLight;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon", ReplicatedUsing=OnRep_Template)
    FAstrawildDungeonRoomTemplate Template;

    /** LCP-2: client-side themed-shell rebuild after the template replicates. */
    UFUNCTION()
    void OnRep_Template();

    /** Batch 8: per-dungeon quest target identifier emitted upon boss defeat. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    FName BossDefeatEventId;

    /** Final Run (FR-7): phase-2 summon species override (Eye Sentinels for the Sovereign). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    FName BossSummonSpeciesId;

    /** Sequential room index within the dungeon. */
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Dungeon", Replicated)
    int32 RoomIndex = 0;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Dungeon", Replicated)
    bool bCleared = false;

    /** Rebuild the placeholder shell from the current Template (generator assigns it after spawn). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Dungeon")
    void RefreshRoomShell();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Populate the encounter from the template (server). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Dungeon")
    void SpawnEncounter(const TArray<FName>& CreatureDefinitionIds);

    /** Mark cleared, grant rewards, notify the dungeon (server). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Dungeon")
    void MarkCleared();

    /**
     * Batch 6 — gap M-7: restore a previously-cleared room from save. Destroys
     * the freshly-generated encounter WITHOUT the defeat pipeline (no events, no
     * loot — both already happened the first time) and marks the room cleared.
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Dungeon")
    void RestoreClearedState();

    /**
     * DP-9: server-side pillar attunement entry point (the pillar actor's
     * Interact routes here). Evaluates the pure sequence verb and applies it.
     */
    void NotifyPillarInteracted(int32 PillarIndex);

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildRoomCleared, AAstrawildDungeonRoomActor*, Room, int32, RoomIndex);
    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Dungeon")
    FAstrawildRoomCleared OnRoomCleared;

private:
    TArray<TWeakObjectPtr<class AAstrawildEchoCharacter>> EncounterCreatures;

    /** Audit C-5: boss-room encounter — the phased boss character (never spawned before). */
    TWeakObjectPtr<class AAstrawildEchoBossCharacter> BossCreature;

    float ClearCheckAccumulator = 0.0f;

    // --- DP-9: theme shell + puzzle + hazard state (server-side) ---

    /** True once ApplyThemeShell ran (guards the double BeginPlay/Refresh path). */
    bool bThemeApplied = false;

    /** Resonance pillars owned by this puzzle room. */
    TArray<TWeakObjectPtr<AAstrawildResonancePillarActor>> PuzzlePillars;
    int32 PuzzleActivatedCount = 0;
    float PuzzleWindowElapsed = 0.0f;
    bool bPuzzleSolved = false;

    /** Eye energy-pulse state: spawn cadence + live tiles. */
    float HazardPulseElapsed = 0.0f;
    int32 HazardPulseCounter = 0;
    TArray<TWeakObjectPtr<AAstrawildBossHazardActor>> HazardTiles;

    /** Sunken Vault water-floor accent (owned — dies with the room). */
    TWeakObjectPtr<AAstrawildWaterPlaneActor> WaterFloorAccent;

    /** Cached accent-light pulse parameters (0 = steady). */
    float CachedLightBaseIntensity = 0.0f;
    float CachedLightPulseRate = 0.0f;
    float LightPhase = 0.0f;

    void BuildRoomShell();

    /** DP-9: one-shot themed visuals (walls/tint/light/dressing/water/pillars). */
    void ApplyThemeShell();

    /** DP-9: deterministic ArtPack dressing scatter (visual only, no collision). */
    void BuildRoomDressing(const FAstrawildDungeonThemeProfile& Profile);

    /** DP-9: spawn the resonance pillars for a puzzle room. */
    void SpawnPuzzlePillars();

    /** DP-9: reset the attunement sequence (wrong order / window expiry). */
    void ResetPuzzleSequence();

    /** DP-9: room-level hazard tick (active while uncleared; server). */
    void ApplyRoomHazard(float DeltaTime);

    /** DP-9: shed the hazard identity on clear — statuses removed, tiles dissipated, pillars locked lit. */
    void ClearRoomHazardIdentity();

    /** DP-9: spawn one wave of pulse tiles at deterministic rotated positions. */
    void SpawnHazardPulseTiles(const FAstrawildDungeonThemeProfile& Profile);

    /** DP-9: apply a status to every live player inside the room bounds (server). */
    void ApplyStatusToPlayersInside(const FAstrawildStatusEffect& Effect);

    /** DP-9: remove a status from every live player inside the room bounds (server). */
    void RemoveStatusFromPlayersInside(const FName StatusId);

    /** DP-9: XY/Z containment against the template extents (vertical band generous). */
    bool IsPlayerInsideRoom(const class AAstrawildPlayerCharacter* Player) const;

    bool IsEncounterDefeated() const;
    void GrantClearReward();

public:
    /** Audit C-4: rooms with no encounter at all (e.g. Entry) clear instantly. */
    bool HasEncounter() const { return !EncounterCreatures.IsEmpty() || BossCreature.IsValid(); }

    /** DP-9: puzzle-room state for save restore / debug. */
    bool IsPuzzleSolved() const { return bPuzzleSolved; }
};
