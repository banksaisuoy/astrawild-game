#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildTypes.h"
#include "AstrawildZoneSubsystem.h"
#include "AstrawildTerrainTileActor.generated.h"

class UProceduralMeshComponent;

/**
 * One rectangular terrain tile of the Shattered Vale (Batch 7 — directive §21/M-13).
 *
 * The world height field is a pure static function of (X, Y, Seed):
 *   height = sum(zoneWeight_i * (Base_i + Amp_i * ridgeBlend(fbm(P)))) + microDetail
 * Zone weights form a partition of unity with ~60m falloff past each zone rect,
 * so neighboring tiles sample identical border heights and never seam.
 *
 * Mesh output uses UProceduralMeshComponent with analytic normals (central
 * differences over the same height field) and per-vertex biome tint colors.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildTerrainTileActor : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildTerrainTileActor();

    /** Grid quads per side (128 default -> ~6.25m spacing on an 800m tile). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Terrain", meta=(ClampMin="16", ClampMax="256"))
    int32 Resolution = 128;

    /** Zone this tile was built for (set by BuildTile). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Terrain")
    EAstrawildZone Zone = EAstrawildZone::None;

    /**
     * Build the mesh for the given zone rect. Must be called after spawn on the
     * server (the bootstrapper does this immediately after SpawnActor).
     */
    void BuildTile(const FAstrawildZoneDescriptor& InZone, int32 InSeed, int32 InResolution);

    FBox2D GetTileBounds() const;

    /** World-space terrain height (cm) at the given XY — pure and deterministic. */
    static float EvalWorldHeight(const FVector2D& WorldXY, int32 Seed);

    /** Biome vertex tint at a world position — pure and deterministic. */
    static FLinearColor EvalGroundTint(const FVector2D& WorldXY, float Height, int32 Seed);

    /** Base FBM field in [-1, 1] shared by every zone (continuity source). */
    static float EvalBaseNoise(const FVector2D& WorldXY, int32 Seed);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Terrain")
    float GetTerrainHeightAt(const FVector& WorldLocation) const;

private:
    /** Deterministic 32-bit hash noise helpers (value noise + FBM). */
    static uint32 NoiseHash(uint32 X, uint32 Y, uint32 Seed);
    static float ValueNoise(const FVector2D& P, uint32 Seed);
    static float FbmNoise(const FVector2D& P, uint32 Seed, int32 Octaves);

    UPROPERTY(VisibleAnywhere, Category="ASTRAWILD|Terrain")
    TObjectPtr<UProceduralMeshComponent> Mesh;

    int32 CachedSeed = 1337;
};
