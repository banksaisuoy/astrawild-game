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
class AAstrawildTerrainTileActor;
class AStaticMeshActor;

/**
 * Zero-asset world bootstrapper (directive §21/§50): on the server, builds the
 * Shattered Vale — six 800m x 800m terrain zones (Batch 7, closes gap M-13),
 * the Dawn Fields starting camp, per-zone wildlife/resources/landmarks with
 * signature colored lighting, and the Hollow Underlight dungeon approach.
 * Everything uses engine basic shapes so the project plays immediately after
 * compile, with no .umap/.uasset content.
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

private:
    void BuildLighting();
    void BuildTerrain();
    void ScatterResourceNodes();
    void SpawnWildEchoes();
    void SpawnHostiles();
    void SpawnPointsOfInterest();
    void BuildZoneLandmarks();

    /** Spawn an engine basic shape prop. Returns the actor (nullptr-safe). */
    AStaticMeshActor* SpawnShape(const TCHAR* MeshPath, const FVector& Location, const FVector& Scale, const FRotator& Rotation);

    /** Spawn a tinted point light; optionally registered as a flicker light. */
    void SpawnZoneLight(const FVector& Location, const FLinearColor& Color, float Intensity, float AttenuationRadius, bool bFlicker);

    /** Random point inside a zone's inner rect (10% inset), snapped to terrain. */
    FVector RandomPointInZone(const FAstrawildZoneDescriptor& Zone, float MinDistanceToCenter);

    void UpdateSunRotation();
    void UpdateFlickerLights(float TimeSeconds);

    class AAstrawildGameState* GetGameState() const;

    FRandomStream RandomStream;
    int32 WorldSeedCached = 1337;

    UPROPERTY()
    TObjectPtr<ADirectionalLight> SunLight;

    /** Landmark lights with animated flicker (lava / marsh wisps / crystals). */
    UPROPERTY()
    TArray<TObjectPtr<APointLight>> FlickerLights;

    TArray<float> FlickerBaseIntensity;
    TArray<float> FlickerPhase;
};
