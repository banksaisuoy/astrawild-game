#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildTypes.h"
#include "AstrawildZoneSubsystem.h"
#include "AstrawildWorldBootstrapper.generated.h"

class UStaticMeshComponent;
class ADirectionalLight;
class ASkyLight;
class APointLight;
class APostProcessVolume;
class AExponentialHeightFog;
class AAstrawildTerrainTileActor;
class AStaticMeshActor;

/**
 * Production V2 Batch 2 — one atmosphere sample from the day/night/weather ramp
 * (pure data; automation-tested). Drives sun color, fog color/density, sky
 * light intensity and the weather sun-dim multiplier every bootstrapper tick.
 */
struct FAstrawildAtmosphereSample
{
    FLinearColor SunColor = FLinearColor::White;
    FLinearColor FogColor = FLinearColor::White;
    float FogDensity = 0.00012f;
    float SkyLightIntensity = 1.4f;
    /** Multiplier applied on top of the existing sun intensity curve (weather). */
    float SunIntensityMultiplier = 1.0f;
};

/**
 * Zero-asset world bootstrapper (directive §21/§50): on the server, builds the
 * Shattered Vale — twelve 800m x 800m terrain zones (Batch 8 "The Grand
 * Expanse", was six in Batch 7), the Dawnstead village + camp in the Dawn
 * Fields, the Driftwood Landing fishing hamlet in the Tidebreaker Isles, sea
 * water planes over the three sea zones, per-zone wildlife/resources/landmarks
 * with signature colored lighting (204-species bestiary seeding), the Hollow
 * Underlight dungeon and the Sunken Vault dungeon, plus the two Dawn Skiff
 * aircraft. Everything uses engine basic shapes so the project plays
 * immediately after compile, with no .umap/.uasset content.
 *
 * Deterministic: all spawns derive from the GameState WorldSeed through the
 * terrain height field (AAstrawildTerrainTileActor::EvalWorldHeight).
 *
 * LCP-2 (LAN co-op client world): the bootstrapper REPLICATES so clients receive
 * it, then every machine builds its own deterministic COSMETIC copy of the world
 * (lighting rig, terrain tiles, sea planes, zone landmarks, biome dressing) from
 * the replicated seed — those layers never replicate (zero bandwidth). Gameplay
 * actors (nodes/NPCs/stations/villages/skiffs/dungeons) stay authority-spawned
 * and replicate normally. The landmark/dressing scatter uses a dedicated
 * CosmeticStream (seed ^ salt) so its output is machine-order-independent;
 * dressing additionally waits for the replicated gameplay actors to stream in so
 * its exclusion bubbles match the server's exactly.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildWorldBootstrapper : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildWorldBootstrapper();

    /** Terrain quads per tile side (~6.25m spacing at 128). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|World", meta=(ClampMin="32", ClampMax="256"))
    int32 TerrainResolution = 128;

    /** Master toggles for perf tuning / test worlds. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|World")
    bool bBuildTerrain = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|World")
    bool bPopulateWildlife = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|World")
    bool bBuildLandmarks = true;

    /** Production V2 Batch 2: procedural biome dressing (trees/rocks/grass per zone). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|World")
    bool bBuildBiomeDressing = true;

    /** Production V2 Batch 2: day/night fog + sun color grading + weather coupling. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|World|Atmosphere")
    bool bEnableAtmosphere = true;

    /** Legacy knobs kept for compatibility — now scale the Dawn Fields population. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|World|Spawns", meta=(ClampMin="0"))
    int32 ResourceNodeCount = 21;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|World|Spawns", meta=(ClampMin="0"))
    int32 WildEchoCount = 8;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|World|Spawns", meta=(ClampMin="0"))
    int32 HostileCount = 2;

    virtual void BeginPlay() override;
    virtual void Tick(float DeltaTime) override;

    /** Terrain height (cm) at world XY — spawn placement helper (pure static under the hood). */
    float GroundZ(const FVector2D& WorldXY) const;

    /** Dawn Fields camp center (spawn point). */
    static FVector2D GetCampCenterXY();

    /**
     * LCP-2 (world-free testable): which machines build the deterministic
     * cosmetic world layers. Authority (standalone + listen host) builds inside
     * BeginPlay; simulated clients build after the seed syncs (bWorldSeedSynced).
     */
    static bool ShouldBuildCosmeticWorld(const ENetRole InLocalRole, const ENetMode InNetMode);

    /**
     * LCP-2 (world-free testable): minimum replicated gameplay actor count a
     * client waits for before building biome dressing (villages, dungeons,
     * portals, POI markers, skiffs — the exclusion-bubble sources). PoiCount is
     * the registry's POI definition count (identical on every machine).
     */
    static int32 ComputeExpectedReplicatedWorldActorCount(const int32 PoiCount);

    /** Cosmetic-stream salt: landmarks/dressing derive from seed ^ this value. */
    static constexpr uint32 CosmeticStreamSalt = 0x5C05E71Cu;

    /** Hollow Underlight dungeon center in the Hollow Approach. */
    static FVector2D GetDungeonCenterXY();

    /** Batch 8 — Sunken Vault dungeon center in the Tidebreaker Isles. */
    static FVector2D GetSunkenVaultCenterXY();

    /** Batch 8 — Driftwood Landing village center in the Tidebreaker Isles. */
    static FVector2D GetDriftwoodLandingXY();

    /** Final Run (FR-7) — Stormcrest zone center (the Eye gate + Glass Tyrant roam). */
    static FVector2D GetStormcrestCenterXY();

    /** Final Run (FR-7) — Eye of the Maelstrom dungeon anchor (~400m over Stormcrest). */
    static FVector2D GetEyeDungeonCenterXY();

    /**
     * Pure atmosphere ramp (automation-tested): SunAlpha 0=dawn..0.5=noon..1=dusk
     * on the 06:00-19:00 day span; night samples use bIsNight. Visibility
     * multiplier comes from the weather subsystem (1=clear, <1=rain/fog/storm).
     */
    static FAstrawildAtmosphereSample EvalAtmosphereRamp(float SunAlpha, bool bIsNight, float VisibilityMultiplier);

    /** Base sun intensity curve shared by UpdateSunRotation + UpdateAtmosphere (pure). */
    static float EvalSunBaseIntensity(float SunAlpha, bool bIsNight);

private:
    void BuildLighting();
    void BuildTerrain();
    void ScatterResourceNodes();
    void SpawnWildEchoes();
    void SpawnHostiles();
    void SpawnPointsOfInterest();
    void BuildZoneLandmarks();

    /** Production V2 Batch 2: biome dressing scatter from biome definitions. */
    void SpawnBiomeDressing();

    // --- Batch 8: living villages, sea, aircraft ---

    /** Spawns the water planes over the three sea zones. */
    void SpawnWaterPlanes();

    /** Production V2: place POI markers from registry definitions. */
    void SpawnPOIMarkers();

    /** Spawns Dawnstead + Driftwood Landing villages with their NPC rosters. */
    void SpawnVillages();

    /** Spawns the two Dawn Skiff aircraft (one per settlement). */
    void SpawnSkiffs();

    /** Highest ground spot near a center (island-safe placement). */
    FVector FindDrySpotNear(const FVector2D& Center, float SearchRadius) const;

    /** Spawn an engine basic shape prop. Returns the actor (nullptr-safe). */
    AStaticMeshActor* SpawnShape(const TCHAR* MeshPath, const FVector& Location, const FVector& Scale, const FRotator& Rotation);

    /** Spawn a tinted point light; optionally registered as a flicker light. */
    void SpawnZoneLight(const FVector& Location, const FLinearColor& Color, float Intensity, float AttenuationRadius, bool bFlicker);

    /** Random point inside a zone's inner rect (10% inset), snapped to terrain. */
    FVector RandomPointInZone(const FAstrawildZoneDescriptor& Zone, float MinDistanceToCenter);

    /** LCP-2: stream-parameterized variant — cosmetic callers pass CosmeticStream so their scatter is machine-order-independent. */
    FVector RandomPointInZoneWith(FRandomStream& Stream, const FAstrawildZoneDescriptor& Zone, float MinDistanceToCenter);

    void UpdateSunRotation();
    void UpdateFlickerLights(float TimeSeconds);

    /** Production V2 Batch 2: day/night + weather atmosphere grading (fog/sun/sky). */
    void UpdateAtmosphere();

    // --- LCP-2: LAN client world build ---

    /** Client: build the seed-independent-of-gameplay cosmetic layers (terrain/sea/lighting/landmarks). */
    void BuildClientCosmeticWorld();

    /** Client: build biome dressing once the replicated gameplay actors streamed in (or timeout). */
    void TryBuildClientDressing();

    /** True while a client-side actor-count gate still needs more streamed actors. */
    bool ClientDressingGateSatisfied() const;

    /** LCP-2 helper: count live actors of a class (client dressing gate). */
    static int32 CountActorsOf(const UWorld* World, UClass* Class);

    class AAstrawildGameState* GetGameState() const;

    FRandomStream RandomStream;
    int32 WorldSeedCached = 1337;

    /** LCP-2: dedicated stream for landmark/dressing scatter — identical on every machine. */
    FRandomStream CosmeticStream;

    /** LCP-2: client build state (authority machines stay false forever). */
    bool bPendingClientWorldBuild = false;
    bool bClientWorldBuilt = false;
    bool bPendingClientDressing = false;
    float ClientDressingWaitElapsed = 0.0f;

    /** LCP-2: hard cap for the dressing actor-stream gate (seconds). */
    static constexpr float ClientDressingGateTimeoutSeconds = 15.0f;

    UPROPERTY()
    TObjectPtr<ADirectionalLight> SunLight;

    UPROPERTY()
    TObjectPtr<ASkyLight> SkyLightActor;

    UPROPERTY()
    TObjectPtr<AExponentialHeightFog> HeightFogActor;

    UPROPERTY()
    TObjectPtr<APostProcessVolume> PostProcessVolume;

    /** Landmark lights with animated flicker (lava / marsh wisps / crystals). */
    UPROPERTY()
    TArray<TObjectPtr<APointLight>> FlickerLights;

    TArray<float> FlickerBaseIntensity;
    TArray<float> FlickerPhase;
};
