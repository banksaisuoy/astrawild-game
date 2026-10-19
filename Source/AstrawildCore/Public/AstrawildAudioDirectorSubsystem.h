#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstrawildTypes.h"
#include "AstrawildAudioDirectorSubsystem.generated.h"

class UAudioComponent;
class UAstrawildItemRegistrySubsystem;

/**
 * CGC-3: the resolved audio action for one weather state — PURE DATA so the
 * automation suite can pin the whole 8-state map without an engine world
 * (the ComputeVisualBand / ComputeDirectorSpawnPlan precedent). CuePath is a
 * full /Game/ path or NAME_None; bStop marks the states whose weather layer
 * ENDS (Clear/Cloudy — the world bed alone carries those skies).
 */
struct FAstrawildWeatherAudioAction
{
    /** Full /Game/ cue path, or NAME_None when the action stops the layer. */
    FName CuePath = NAME_None;

    /** Loop volume for the weather bed (pre-master-volume). */
    float Volume = 0.0f;

    /** Loop pitch for the weather bed. */
    float Pitch = 1.0f;

    /** True when the state plays NO bed and stops any running one. */
    bool bStop = false;
};

/**
 * CGC-3 — the audio director: the consumer for the audio content that audit
 * 2-b found BOUND BUT DEAD (every biome carries a preloaded 30s seamless-loop
 * AmbientAudio bed that no line of code ever played; OnWeatherChanged had zero
 * audio consumers; 27 of the 36 imported cues were unused). ZERO new assets —
 * everything played here already exists in Content/Audio.
 *
 * NETMODE POSTURE (deliberate, documented): this director runs on ALL world
 * types INCLUDING pure LAN clients — unlike the WildlifeDirector/HostileSpawner
 * (server-only spawners), every sound it manages is LOCAL 2D FEEDBACK for the
 * machine it runs on (the zone bed under YOUR ears, YOUR heartbeat, the hum of
 * the skiff YOU pilot). Each machine resolves ITS local player pawn through
 * UGameplayStatics::GetPlayerController(World, 0) and the pure static zone
 * table (the exact RefreshZoneBanner idiom), so no replication is needed for
 * the bed to be right on every screen. Weather follows the REPLICATED
 * GameState->WeatherState poll (the server-only OnWeatherChanged delegate is
 * ALSO subscribed for instant host-side response — the second subscriber after
 * CGC-1's WildlifeDirector; pure clients converge through the 1s poll).
 *
 * Everything is FAIL-CLOSED: a null world / null registry / unresolvable zone /
 * missing cue is a silent skip — audio enriches, it NEVER gates gameplay.
 *
 * The pure decision maps (footstep-surface-by-zone, weather action, species
 * cry, zone ambience path, master volume) are PURE STATIC world-free functions
 * so test 137 pins them without an engine world.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildAudioDirectorSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    UAstrawildAudioDirectorSubsystem();

    // -------------------------------------------------------------------
    // Master volume (the forward-compatible hook the CGC-4 settings screen
    // writes). Pure statics; every Play helper and every persistent loop
    // scales by it immediately.
    // -------------------------------------------------------------------

    /** Global 0..1 master volume for every director-managed cue. Default 1.0. */
    static float GetMasterVolume() { return MasterVolume; }

    /**
     * Set the global master volume (clamped 0..1). Scales every live
     * persistent-loop component IMMEDIATELY (zone bed / weather layer /
     * heartbeat / skiff hum on every director instance) and is applied at
     * spawn time by every one-shot helper.
     */
    static void SetMasterVolume(const float NewVolume);

    // -------------------------------------------------------------------
    // Pure static contracts (world-free, automation-test pinned)
    // -------------------------------------------------------------------

    /** Volume the zone ambience bed plays at (pre-master-volume). */
    static constexpr float AmbienceVolume = 0.4f;

    /** Volume the footstep dispatcher plays at (pre-master-volume). */
    static constexpr float FootstepVolume = 0.3f;

    /** Horizontal centimeters traveled per footstep (the player cadence). */
    static constexpr float FootstepStrideCm = 220.0f;

    /** Heartbeat starts when the local player's health fraction drops below this. */
    static constexpr float HeartbeatStartFraction = 0.25f;

    /** Hysteresis: the heartbeat only stops once health recovers to this. */
    static constexpr float HeartbeatStopFraction = 0.30f;

    /**
     * Footstep surface cue SUFFIX for a zone (Grass/Sand/Stone/Water), played
     * as /Game/Audio/A_Footstep_<Suffix>. COARSE RULE (documented, honest):
     * the suffix derives from the zone's biome identity only — no proximity
     * scan, so the starter base in Dawn Fields reads Grass and the camp's
     * metal structures do not change it. Sunscar Desert -> Sand, the three
     * sea zones -> Water, the three rocky zones (Frostveil/Stormcrest/Hollow
     * Approach) -> Stone, everything else (incl. the untamed wilds) -> Grass.
     * A_Footstep_Metal stays imported-but-unused by this map (station
     * interiors are a future surface context, not a zone).
     */
    static FName GetFootstepCueForZone(const EAstrawildZone Zone);

    /**
     * Full /Game/ cry path for a species with an IMPORTED cry cue — exactly
     * the six flat-imported species cries. Every other species returns
     * NAME_None and stays SILENT (fail-closed by design: the remaining 16
     * SFXSet vocalization cues live in ArtSource/Audio/Echoes and only
     * resolve after import_echo_bases.py lands them engine-side — the
     * existing EchoCharacter weakness-hit SFXSet path already covers those
     * species reactively).
     */
    static FName GetSpeciesCryPath(const FName SpeciesId);

    /**
     * The resolved audio action for a weather state (the 8-state map):
     * Storm -> wind bed (deep pitch), HeavyRain/Rain -> water beds,
     * Cold -> night-crystal bed low, Fog -> marsh bed low, Heat -> forest
     * bed pitched down, Clear/Cloudy -> stop (the zone bed alone carries
     * the calm sky). Beds LOOP while the state persists.
     */
    static FAstrawildWeatherAudioAction GetWeatherAudioAction(const EAstrawildWeatherState State);

    /**
     * Zone -> biome -> AmbientAudio soft path (the bound+preloaded 30s bed).
     * The SAME resolution the director's runtime player uses: the registry's
     * biome whose Zone field matches (the biome BiomeId is Zone_<Name> but
     * three zone-table ids abbreviate, so the ENUM field is the honest join).
     * NAME_None when the registry/biome/cue is missing (fail-closed silence).
     */
    static FName GetZoneAmbiencePath(const UAstrawildItemRegistrySubsystem* Registry, const EAstrawildZone Zone);

    /**
     * The canonical mirror of the 36 flat-imported audio cues
     * (Content/Audio/*.uasset, the ArtSource/Audio import batch) as full
     * /Game/Audio/<name> paths. Single source of truth for the test's
     * "every mapped cue is real" assertions and any future import guard.
     */
    static const TArray<FName>& GetImportedAudioCueNames();

    // -------------------------------------------------------------------
    // Fail-closed one-shot helpers (static — usable from any call site with
    // just a UWorld; every volume scales by MasterVolume at call time).
    // -------------------------------------------------------------------

    /** PlaySound2D by /Game/ path. Silent no-op when world/cue missing. */
    static void PlaySound2DFromPath(UWorld* World, const TCHAR* CuePath,
        const float Volume, const float Pitch = 1.0f);

    /** PlaySoundAtLocation by /Game/ path. Silent no-op when world/cue missing. */
    static void PlaySoundAtLocationFromPath(UWorld* World, const TCHAR* CuePath,
        const FVector& Location, const float Volume, const float Pitch = 1.0f);

    // -------------------------------------------------------------------
    // Runtime duties
    // -------------------------------------------------------------------

    /**
     * Footstep dispatcher: loads /Game/Audio/A_Footstep_<SurfaceCueName> and
     * plays it as a local 2D blip (pitch-jittered 0.9..1.1 so a sprinting
     * stride never sounds mechanical). Fail-closed: unknown surface name or
     * missing cue = silent skip.
     */
    void PlayFootstep(const FName SurfaceCueName);

    /** Skiff engine hum ON (honest reuse: the 1.6s A_Weapon_Charge_Loop cue IS an engine spool). */
    void OnSkiffBoarded();

    /** Skiff engine hum OFF. */
    void OnSkiffExited();

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
    virtual void Deinitialize() override;

protected:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
    UFUNCTION()
    void HandleWeatherChanged(EAstrawildWeatherState NewState, EAstrawildWeatherState OldState);

    /** Stop a kept loop handle (fail-closed on an already-destroyed component). */
    void StopLoopHandle(TObjectPtr<UAudioComponent>& Handle);

    /** Stop + forget every persistent loop (world teardown / teardown safety). */
    void StopAllLoops();

    /** 1s-cadence block: zone bed swap, weather poll, heartbeat hysteresis. */
    void TickSlowAudioLayer();

    /** Apply the weather action for a state (idempotent; no-op when unchanged). */
    void ApplyWeatherAudio(const EAstrawildWeatherState State);

    /**
     * Spawn a kept looping bed handle (registered for immediate master-volume
     * scaling). bFadeIn=true leaves it silent for the caller's fade ramp;
     * false applies BaseVolume x MasterVolume at once.
     */
    UAudioComponent* SpawnKeptLoop(const TCHAR* CuePath, const float BaseVolume,
        const float Pitch, const bool bFadeIn);

    // --- kept persistent-loop handles (bAutoDestroy components; Stop() tears them down) ---
    UPROPERTY()
    TObjectPtr<UAudioComponent> AmbienceHandle;

    UPROPERTY()
    TObjectPtr<UAudioComponent> WeatherHandle;

    UPROPERTY()
    TObjectPtr<UAudioComponent> HeartbeatHandle;

    UPROPERTY()
    TObjectPtr<UAudioComponent> SkiffHumHandle;

    /** Zone whose bed the current handle plays (NAME_None = silent/no bed). */
    FName CurrentAmbienceZone = NAME_None;

    /** Last weather state whose action was APPLIED (idempotence for the poll). */
    EAstrawildWeatherState LastAppliedWeather = EAstrawildWeatherState::Clear;

    /** Slow-layer cadence accumulator (1.0s). */
    float SlowAudioAccumulator = 1.0f;

    /** Zone-bed fade-in elapsed (0..AmbienceFadeSeconds; bed starts near-silent). */
    float AmbienceFadeElapsed = 0.0f;

    /** OnWeatherChanged wired-once flag (the HuntSubsystem/WildlifeDirector idiom). */
    bool bWeatherSignalWired = false;
};
