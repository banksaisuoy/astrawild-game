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

    void UpdateSunRotation();
    void UpdateFlickerLights(float TimeSeconds);

    /** Production V2 Batch 2: day/night + weather atmosphere grading (fog/sun/sky). */
    void UpdateAtmosphere();

    class AAstrawildGameState* GetGameState() const;

    FRandomStream RandomStream;
    int32 WorldSeedCached = 1337;

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
