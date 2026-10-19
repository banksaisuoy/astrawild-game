#include "AstrawildAudioDirectorSubsystem.h"

#include "AstrawildArtPack.h"
#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildGameState.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildSurvivalComponent.h"
#include "AstrawildWeatherSubsystem.h"
#include "AstrawildZoneSubsystem.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

namespace
{
    /** Zone-bed fade-in: the swap ramps the new bed up over half a second. */
    constexpr float AmbienceFadeSeconds = 0.5f;

    /** Heartbeat loop volume (pre-master-volume; the cue itself is authored low). */
    constexpr float HeartbeatVolume = 0.4f;

    /** The low-HP heartbeat cue (flat import — Content/Audio). */
    const TCHAR* HeartbeatCuePath = TEXT("/Game/Audio/A_Player_Heartbeat_Low");

    /**
     * One live persistent loop registered for IMMEDIATE master-volume scaling.
     * Weak pointers: SpawnSound2D components are bAutoDestroy, so a stopped
     * loop is already dying and simply falls out of the registry on the next
     * prune — no manual lifetime bookkeeping.
     */
    struct FLiveAudioLoop
    {
        TWeakObjectPtr<UAudioComponent> Component;
        float BaseVolume = 1.0f;
    };

    TArray<FLiveAudioLoop>& GetLiveLoops()
    {
        static TArray<FLiveAudioLoop> Loops;
        return Loops;
    }

    void RegisterLiveLoop(UAudioComponent* Component, const float BaseVolume)
    {
        if (!Component)
        {
            return;
        }
        GetLiveLoops().Add({ Component, BaseVolume });
        // Opportunistic prune — at most a handful of loops live per world.
        GetLiveLoops().RemoveAll([](const FLiveAudioLoop& Entry)
        {
            return !Entry.Component.IsValid();
        });
    }

    void UnregisterLiveLoop(const UAudioComponent* Component)
    {
        if (!Component)
        {
            return;
        }
        GetLiveLoops().RemoveAll([Component](const FLiveAudioLoop& Entry)
        {
            return Entry.Component.Get() == Component;
        });
    }
}

float UAstrawildAudioDirectorSubsystem::MasterVolume = 1.0f;

UAstrawildAudioDirectorSubsystem::UAstrawildAudioDirectorSubsystem()
{
}

bool UAstrawildAudioDirectorSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UAstrawildAudioDirectorSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UAstrawildAudioDirectorSubsystem, STATGROUP_Tickables);
}

void UAstrawildAudioDirectorSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    // Weather signal: the SECOND subscriber after CGC-1's WildlifeDirector
    // (audit finding D — OnWeatherChanged had zero audio consumers). Fires
    // server-side only; pure clients converge through the replicated
    // GameState->WeatherState poll in the slow layer. Wired once.
    if (!bWeatherSignalWired)
    {
        if (UAstrawildWeatherSubsystem* Weather = InWorld.GetSubsystem<UAstrawildWeatherSubsystem>())
        {
            Weather->OnWeatherChanged.AddDynamic(this, &UAstrawildAudioDirectorSubsystem::HandleWeatherChanged);
        }
        bWeatherSignalWired = true;
    }

    UE_LOG(LogAstrawildWorld, Log,
        TEXT("Audio director online (zone ambience consumer + weather layer + heartbeat + skiff hum; master volume %.2f)."),
        MasterVolume);
}

void UAstrawildAudioDirectorSubsystem::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // Posture: everything this director plays is LOCAL 2D feedback, so it runs
    // on standalone / listen host / pure clients alike. A DEDICATED server has
    // no local listener — no 2D audio can be heard there, so it skips entirely
    // (the replicated weather poll would otherwise spawn inaudible loops).
    if (World->GetNetMode() == NM_DedicatedServer)
    {
        return;
    }

    SlowAudioAccumulator += DeltaTime;
    if (SlowAudioAccumulator >= 1.0f)
    {
        SlowAudioAccumulator -= 1.0f;
        TickSlowAudioLayer();
    }

    // Zone-bed fade-in ramp (new beds start silent and bloom over ~0.5s).
    if (AmbienceHandle && AmbienceFadeElapsed < AmbienceFadeSeconds)
    {
        AmbienceFadeElapsed = FMath::Min(AmbienceFadeElapsed + DeltaTime, AmbienceFadeSeconds);
        const float FadeAlpha = AmbienceFadeElapsed / AmbienceFadeSeconds;
        AmbienceHandle->SetVolumeMultiplier(AmbienceVolume * FadeAlpha * MasterVolume);
    }
}

void UAstrawildAudioDirectorSubsystem::Deinitialize()
{
    // World teardown: stop every kept loop so no bed outlives its world.
    StopAllLoops();
    Super::Deinitialize();
}

// ---------------------------------------------------------------------------
// Master volume (pure statics — the CGC-4 settings-screen hook)
// ---------------------------------------------------------------------------

void UAstrawildAudioDirectorSubsystem::SetMasterVolume(const float NewVolume)
{
    MasterVolume = FMath::Clamp(NewVolume, 0.0f, 1.0f);

    // Scale every live persistent loop on every world immediately (zone bed,
    // weather layer, heartbeat, skiff hum) — kept bases make this reversible.
    for (const FLiveAudioLoop& Entry : GetLiveLoops())
    {
        if (UAudioComponent* Component = Entry.Component.Get())
        {
            Component->SetVolumeMultiplier(Entry.BaseVolume * MasterVolume);
        }
    }
}

// ---------------------------------------------------------------------------
// Pure static contracts (world-free — the automation suite pins these)
// ---------------------------------------------------------------------------

FName UAstrawildAudioDirectorSubsystem::GetFootstepCueForZone(const EAstrawildZone Zone)
{
    switch (Zone)
    {
    case EAstrawildZone::SunscarDesert:
        return TEXT("Sand");
    case EAstrawildZone::FrostveilExpanse:
    case EAstrawildZone::StormcrestHighlands:
    case EAstrawildZone::HollowApproach:
        return TEXT("Stone");
    case EAstrawildZone::AzureShallows:
    case EAstrawildZone::PearlseaReef:
    case EAstrawildZone::TidebreakerIsles:
        return TEXT("Water");
    default:
        // Coarse rule (documented): everything else — the starter meadow, both
        // forests, the marsh, the volcanic ridge, the untamed wilds and the
        // area around camp — reads Grass. No proximity scan.
        return TEXT("Grass");
    }
}

FName UAstrawildAudioDirectorSubsystem::GetSpeciesCryPath(const FName SpeciesId)
{
    static const TMap<FName, FName> CryTable = {
        { TEXT("Echo_Bastionbeetle"), TEXT("/Game/Audio/A_Echo_Bastionbeetle_Skitter") },
        { TEXT("Echo_Cindermule"),    TEXT("/Game/Audio/A_Echo_Cindermule_Grunt") },
        { TEXT("Echo_Deepdelver"),    TEXT("/Game/Audio/A_Echo_Deepdelver_Rumble") },
        { TEXT("Echo_Mistmender"),    TEXT("/Game/Audio/A_Echo_Mistmender_Chime") },
        { TEXT("Echo_Terraquill"),    TEXT("/Game/Audio/A_Echo_Terraquill_Call") },
        { TEXT("Echo_Voltpylon"),     TEXT("/Game/Audio/A_Echo_Voltpylon_Hum") },
    };
    // FindRef fail-closes to NAME_None for every species without an imported
    // cry — those stay silent until the SFXSet import lands engine-side.
    return CryTable.FindRef(SpeciesId);
}

FAstrawildWeatherAudioAction UAstrawildAudioDirectorSubsystem::GetWeatherAudioAction(const EAstrawildWeatherState State)
{
    switch (State)
    {
    case EAstrawildWeatherState::Storm:
        // The storm IS wind — the gentle bed pitched down reads as a gale.
        return { TEXT("/Game/Audio/A_Amb_Wind_Gentle"), 0.50f, 0.80f, false };
    case EAstrawildWeatherState::HeavyRain:
        return { TEXT("/Game/Audio/A_Amb_Water_Lake"), 0.60f, 0.90f, false };
    case EAstrawildWeatherState::Rain:
        return { TEXT("/Game/Audio/A_Amb_Water_Lake"), 0.40f, 0.55f, false };
    case EAstrawildWeatherState::Cold:
        return { TEXT("/Game/Audio/A_Amb_Night_Crystal"), 0.30f, 0.75f, false };
    case EAstrawildWeatherState::Fog:
        return { TEXT("/Game/Audio/A_Amb_Marsh_Dusk"), 0.30f, 0.85f, false };
    case EAstrawildWeatherState::Heat:
        return { TEXT("/Game/Audio/A_Amb_Forest_Dawn"), 0.30f, 0.85f, false };
    case EAstrawildWeatherState::Cloudy:
    default:
        // Clear and Cloudy ride the zone bed alone — the weather layer stops.
        return { NAME_None, 0.0f, 1.0f, true };
    }
}

FName UAstrawildAudioDirectorSubsystem::GetZoneAmbiencePath(const UAstrawildItemRegistrySubsystem* Registry,
    const EAstrawildZone Zone)
{
    if (!Registry)
    {
        return NAME_None;
    }
    // The honest join is the biome's Zone ENUM field: the biome ids are
    // "Zone_<Name>" but three zone-table ids abbreviate (Zone_Frostveil /
    // Zone_Sunscar / Zone_Stormcrest), so id-matching would miss three zones.
    for (const UAstrawildBiomeDefinition* Biome : Registry->GetAllBiomes())
    {
        if (Biome && Biome->Zone == Zone && !Biome->AmbientAudio.IsNull())
        {
            return FName(*Biome->AmbientAudio.ToSoftObjectPath().ToString());
        }
    }
    return NAME_None;
}

const TArray<FName>& UAstrawildAudioDirectorSubsystem::GetImportedAudioCueNames()
{
    // The canonical mirror of the 36 flat-imported cues (Content/Audio —
    // the ArtSource/Audio import batch). Test 137 asserts every mapped cue
    // is a member of this list; any future import change reconciles HERE.
    static const TArray<FName> Names = {
        // Ambience beds (30s seamless loops).
        TEXT("/Game/Audio/A_Amb_Forest_Dawn"),
        TEXT("/Game/Audio/A_Amb_Marsh_Dusk"),
        TEXT("/Game/Audio/A_Amb_Night_Crystal"),
        TEXT("/Game/Audio/A_Amb_Water_Lake"),
        TEXT("/Game/Audio/A_Amb_Wind_Gentle"),
        // Species cries (6 imported).
        TEXT("/Game/Audio/A_Echo_Bastionbeetle_Skitter"),
        TEXT("/Game/Audio/A_Echo_Cindermule_Grunt"),
        TEXT("/Game/Audio/A_Echo_Deepdelver_Rumble"),
        TEXT("/Game/Audio/A_Echo_Mistmender_Chime"),
        TEXT("/Game/Audio/A_Echo_Terraquill_Call"),
        TEXT("/Game/Audio/A_Echo_Voltpylon_Hum"),
        // Footsteps (5 surfaces).
        TEXT("/Game/Audio/A_Footstep_Grass"),
        TEXT("/Game/Audio/A_Footstep_Metal"),
        TEXT("/Game/Audio/A_Footstep_Sand"),
        TEXT("/Game/Audio/A_Footstep_Stone"),
        TEXT("/Game/Audio/A_Footstep_Water"),
        // Player.
        TEXT("/Game/Audio/A_Player_Heartbeat_Low"),
        TEXT("/Game/Audio/A_Player_Jump"),
        TEXT("/Game/Audio/A_Player_Land"),
        TEXT("/Game/Audio/A_Player_Scan_Ping"),
        // UI.
        TEXT("/Game/Audio/A_UI_Cancel"),
        TEXT("/Game/Audio/A_UI_Click"),
        TEXT("/Game/Audio/A_UI_Confirm"),
        TEXT("/Game/Audio/A_UI_Craft_Done"),
        TEXT("/Game/Audio/A_UI_Hover"),
        TEXT("/Game/Audio/A_UI_Research_Done"),
        TEXT("/Game/Audio/A_UI_Warning"),
        // Weapons + capture stinger.
        TEXT("/Game/Audio/A_Weapon_Arc_Fire"),
        TEXT("/Game/Audio/A_Weapon_Charge_Loop"),
        TEXT("/Game/Audio/A_Weapon_Impact_Energy"),
        TEXT("/Game/Audio/A_Weapon_Impact_Kinetic"),
        TEXT("/Game/Audio/A_Weapon_Plasma_Fire"),
        TEXT("/Game/Audio/A_Weapon_Rail_Fire"),
        TEXT("/Game/Audio/A_Weapon_Scrap_Fire"),
        TEXT("/Game/Audio/A_Weapon_Singularity_Fire"),
        TEXT("/Game/Audio/A_Echo_Capture_Success"),
    };
    return Names;
}

// ---------------------------------------------------------------------------
// Fail-closed one-shot helpers
// ---------------------------------------------------------------------------

void UAstrawildAudioDirectorSubsystem::PlaySound2DFromPath(UWorld* World, const TCHAR* CuePath,
    const float Volume, const float Pitch)
{
    if (!World || !CuePath || FCString::Strlen(CuePath) == 0)
    {
        return; // fail-closed: no world, no sound.
    }
    if (USoundBase* Cue = LoadObject<USoundBase>(nullptr, CuePath))
    {
        UGameplayStatics::PlaySound2D(World, Cue, Volume * MasterVolume, Pitch);
    }
}

void UAstrawildAudioDirectorSubsystem::PlaySoundAtLocationFromPath(UWorld* World, const TCHAR* CuePath,
    const FVector& Location, const float Volume, const float Pitch)
{
    if (!World || !CuePath || FCString::Strlen(CuePath) == 0)
    {
        return; // fail-closed: no world, no sound.
    }
    if (USoundBase* Cue = LoadObject<USoundBase>(nullptr, CuePath))
    {
        UGameplayStatics::PlaySoundAtLocation(World, Cue, Location, Volume * MasterVolume, Pitch);
    }
}

// ---------------------------------------------------------------------------
// Runtime duties
// ---------------------------------------------------------------------------

void UAstrawildAudioDirectorSubsystem::PlayFootstep(const FName SurfaceCueName)
{
    // Fail-closed: an unresolvable surface name (or a surface whose cue never
    // imported) is a silent stride — the walk keeps playing exactly as before.
    if (SurfaceCueName.IsNone())
    {
        return;
    }
    const FString CuePath = FString::Printf(TEXT("/Game/Audio/A_Footstep_%s"), *SurfaceCueName.ToString());
    PlaySound2DFromPath(GetWorld(), *CuePath, FootstepVolume, FMath::FRandRange(0.9f, 1.1f));
}

void UAstrawildAudioDirectorSubsystem::OnSkiffBoarded()
{
    if (SkiffHumHandle)
    {
        return; // already humming
    }
    // Honest reuse (zero new assets): A_Weapon_Charge_Loop is a 1.6s engine
    // spool — pitched down to 0.45 and kept looping it reads as the skiff's
    // drive. Registered like every kept loop so master volume scales it live.
    SkiffHumHandle = SpawnKeptLoop(AstrawildArtPack::Sfx::ChargeLoop, 0.5f, 0.45f, /*bFadeIn=*/false);
}

void UAstrawildAudioDirectorSubsystem::OnSkiffExited()
{
    StopLoopHandle(SkiffHumHandle);
}

void UAstrawildAudioDirectorSubsystem::HandleWeatherChanged(EAstrawildWeatherState NewState,
    EAstrawildWeatherState /*OldState*/)
{
    // Server-side instant response (the delegate broadcasts from the weather
    // subsystem's server-only tick); clients converge via the state poll.
    ApplyWeatherAudio(NewState);
}

void UAstrawildAudioDirectorSubsystem::TickSlowAudioLayer()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const APlayerController* LocalPC = UGameplayStatics::GetPlayerController(World, 0);
    const APawn* LocalPawn = LocalPC ? LocalPC->GetPawn() : nullptr;

    // --- 1) Zone ambience bed (the bound+preloaded contract's first player) ---
    // Resolution: local pawn -> pure static zone lookup (the HUD banner
    // idiom, no replication) -> registry biome -> AmbientAudio soft path.
    // Same resolution the world-free test pins through GetZoneAmbiencePath.
    EAstrawildZone LocalZone = EAstrawildZone::None;
    if (LocalPawn)
    {
        LocalZone = UAstrawildZoneSubsystem::GetZoneAt(LocalPawn->GetActorLocation());
    }
    if (LocalZone != CurrentAmbienceZone)
    {
        CurrentAmbienceZone = LocalZone;
        StopLoopHandle(AmbienceHandle);
        if (UAstrawildItemRegistrySubsystem* Registry = World->GetSubsystem<UAstrawildItemRegistrySubsystem>())
        {
            const FName BedPath = GetZoneAmbiencePath(Registry, LocalZone);
            if (!BedPath.IsNone())
            {
                // Beds were preloaded by the ContentLibrary warm pass, so the
                // LoadSynchronous below is resident-memory-cheap. Fade-in from
                // silence over ~0.5s so zone crossings bloom, never snap.
                AmbienceHandle = SpawnKeptLoop(*BedPath.ToString(), AmbienceVolume, 1.0f, /*bFadeIn=*/true);
                AmbienceFadeElapsed = 0.0f;
            }
        }
        // Zone without a resolvable bed (incl. the untamed wilds outside the
        // 12 rects): fail-closed silence — the old bed stopped, nothing gates.
    }

    // --- 2) Weather layer (replicated-state poll; instant on the host via the
    //     OnWeatherChanged subscription above). Idempotent per state change.
    if (const AAstrawildGameState* GameState = World->GetGameState<AAstrawildGameState>())
    {
        ApplyWeatherAudio(GameState->WeatherState);
    }

    // --- 3) Low-HP heartbeat (hysteresis so a 24%->26% hover stays steady). ---
    const AAstrawildPlayerCharacter* PlayerChar = LocalPawn ? Cast<AAstrawildPlayerCharacter>(LocalPawn) : nullptr;
    const UAstrawildSurvivalComponent* Survival = PlayerChar ? PlayerChar->SurvivalComponent : nullptr;
    const float HealthFraction = Survival ? Survival->GetHealthFraction() : 1.0f;
    if (HealthFraction < HeartbeatStartFraction && !HeartbeatHandle)
    {
        HeartbeatHandle = SpawnKeptLoop(HeartbeatCuePath, HeartbeatVolume, 1.0f, /*bFadeIn=*/false);
    }
    else if (HealthFraction >= HeartbeatStopFraction && HeartbeatHandle)
    {
        StopLoopHandle(HeartbeatHandle);
    }
}

void UAstrawildAudioDirectorSubsystem::ApplyWeatherAudio(const EAstrawildWeatherState State)
{
    if (State == LastAppliedWeather)
    {
        return; // idempotent — the poll re-asserts the same state every second.
    }
    LastAppliedWeather = State;

    const FAstrawildWeatherAudioAction Action = GetWeatherAudioAction(State);
    StopLoopHandle(WeatherHandle);
    if (!Action.bStop && !Action.CuePath.IsNone())
    {
        // No fade — the weather layer reads as an immediate change riding
        // under the zone bed. Loops while the state persists (Clear/Cloudy
        // stop it through the bStop branch above).
        WeatherHandle = SpawnKeptLoop(*Action.CuePath.ToString(), Action.Volume, Action.Pitch, /*bFadeIn=*/false);
    }
}

UAudioComponent* UAstrawildAudioDirectorSubsystem::SpawnKeptLoop(const TCHAR* CuePath, const float BaseVolume,
    const float Pitch, const bool bFadeIn)
{
    UWorld* World = GetWorld();
    if (!World || !CuePath || FCString::Strlen(CuePath) == 0)
    {
        return nullptr;
    }
    if (USoundBase* Cue = LoadObject<USoundBase>(nullptr, CuePath))
    {
        // SpawnSound2D returns the loop's UAudioComponent — the KEPT handle is
        // the whole point (Stop() on zone/weather/exit change, live scaling
        // through the master-volume registry).
        if (UAudioComponent* Loop = UGameplayStatics::SpawnSound2D(World, Cue, 0.0f, Pitch))
        {
            RegisterLiveLoop(Loop, BaseVolume);
            if (!bFadeIn)
            {
                Loop->SetVolumeMultiplier(BaseVolume * MasterVolume);
            }
            return Loop;
        }
    }
    // Fail-closed: unresolvable cue = silence (never a gameplay gate); the
    // caller's zone/state bookkeeping already advanced so we do not re-load
    // the same missing path every second.
    return nullptr;
}

void UAstrawildAudioDirectorSubsystem::StopLoopHandle(TObjectPtr<UAudioComponent>& Handle)
{
    if (Handle)
    {
        if (UAudioComponent* Loop = Handle.Get())
        {
            UnregisterLiveLoop(Loop);
            Loop->Stop();
        }
        Handle = nullptr;
    }
}

void UAstrawildAudioDirectorSubsystem::StopAllLoops()
{
    StopLoopHandle(AmbienceHandle);
    StopLoopHandle(WeatherHandle);
    StopLoopHandle(HeartbeatHandle);
    StopLoopHandle(SkiffHumHandle);
}
