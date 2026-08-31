#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildTypes.h"
#include "AstrawildZoneSubsystem.h"
#include "AstrawildBiomeDressingActor.generated.h"

class UAstrawildBiomeDefinition;
class UInstancedStaticMeshComponent;
class UProceduralMeshComponent;

/** Canopy silhouette variants — how a placeholder tree reads per biome. */
enum class EAstrawildDressingCanopy : uint8
{
    Broadleaf, ///< trunk + wide squat cone (Dawn Fields, Verdant Reach)
    Conifer,   ///< trunk + tall narrow cone (Frost, Stormcrest, Glimmerwood)
    Palm,      ///< thin tall trunk + flattened wide cone (islands)
    Dead,      ///< trunk + tilted box branches (marsh, hollow)
    Cactus,    ///< stacked cones, no trunk (desert)
    Spire      ///< crystal cone straight from the ground (ember ridges)
};

/**
 * Per-zone dressing budget (Production V2 Batch 2 — Visual Vertical Slice).
 * Plain data struct: counts are multiplied by the biome definition's
 * DressingDensity and the world seed drives the deterministic layout.
 */
struct ASTRAWILDCORE_API FAstrawildDressingProfile
{
    int32 TreeCount = 0;
    int32 RockCount = 0;
    int32 GrassCount = 0;
    float TreeScaleMin = 0.8f;
    float TreeScaleMax = 1.5f;
    float RockScaleMin = 0.6f;
    float RockScaleMax = 2.0f;
    float GrassScaleMin = 0.7f;
    float GrassScaleMax = 1.4f;
    EAstrawildDressingCanopy CanopyStyle = EAstrawildDressingCanopy::Broadleaf;
    /** 0..1 lerp of the canopy tint toward snow-white (frost biomes). */
    float SnowBlend = 0.0f;
};

/**
 * Biome dressing (Master Plan §5/§31 — Visual Vertical Slice): the FIRST
 * runtime consumer of UAstrawildBiomeDefinition. One actor per zone scatters
 * a deterministic, gameplay-aware set of trees/rocks/grass so the twelve
 * regions stop reading as empty vertex-tinted terrain:
 *
 * - Points reject below sea level, on steep slopes (trees), and near camp /
 *   villages / dungeons / POIs / skiff pads (exclusion list from the
 *   bootstrapper) — dressing never blocks a gameplay space.
 * - PLACEHOLDER path (zero assets): merged vertex-colored ProceduralMesh
 *   sections (trunks / canopies / rocks / grass) on the same
 *   DebugMeshMaterial path as the terrain and Echo bodies.
 * - ANTIGRAVITY path: when the biome definition's TreeMeshes / RockMeshes /
 *   GrassMeshes soft refs resolve, the same transforms feed
 *   UInstancedStaticMeshComponents with the real meshes and the matching
 *   placeholder sections are skipped — a data-only visual upgrade.
 *
 * All scatter math is pure + deterministic (FRandomStream seeded from the
 * world seed) — automation-tested, and identical between runs.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildBiomeDressingActor : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildBiomeDressingActor();

    /** Zone dressing budget table (pure static — automation-tested). */
    static FAstrawildDressingProfile GetDressingProfile(EAstrawildZone Zone);

    /**
     * Placement gate (pure static — automation-tested): above the sea margin,
     * outside every exclusion bubble. Slope is checked separately per kind in
     * ScatterDressingPoints because trees/rocks/grass tolerate different
     * steepness (natural banding: rocks read on cliffs).
     */
    static bool IsPointDressable(const FVector2D& Point, const TArray<FVector2D>& ExclusionCenters,
        const TArray<float>& ExclusionRadii);

    /** Sea-level placement margin (cm) — dressing floats on nothing. */
    static float GetSeaMargin() { return 60.0f; }

    /** Max slope (cm of height over a 360cm sample) before trees reject a spot. */
    static float GetTreeSlopeLimit() { return 150.0f; }

    /** Rocks tolerate steeper ground than trees (cliff banding). */
    static float GetRockSlopeLimit() { return 260.0f; }

    /** Grass sits between trees and rocks. */
    static float GetGrassSlopeLimit() { return 210.0f; }

    /**
     * Deterministic scatter (pure static — automation-tested): fills the three
     * point arrays inside the zone rect using the terrain height field. Points
     * that fail the water/exclusion/slope gates are resampled up to 6 attempts
     * each, so forest budgets actually land on land.
     */
    static void ScatterDressingPoints(int32 Seed, const FAstrawildZoneDescriptor& Zone,
        const FAstrawildDressingProfile& Profile, const TArray<FVector2D>& ExclusionCenters,
        const TArray<float>& ExclusionRadii, TArray<FVector>& OutTreePoints,
        TArray<FVector>& OutRockPoints, TArray<FVector>& OutGrassPoints);

    /**
     * Builds this zone's dressing. BiomeDef may be null (zone-table defaults).
     * Runs on the server after every other world actor exists so the
     * exclusion list is complete.
     */
    void BuildDressing(const FAstrawildZoneDescriptor& Zone, UAstrawildBiomeDefinition* BiomeDef,
        int32 WorldSeed, const TArray<FVector2D>& ExclusionCenters, const TArray<float>& ExclusionRadii);

private:
    /** Derives a dressing tint from the zone ground tint when the biome def leaves it white. */
    static FLinearColor ResolveTint(const FLinearColor& ExplicitTint, const FLinearColor& ZoneGroundTint,
        float Brightness, float SaturationBoost);

    /** Local slope measure: max height delta over +/- SampleRadius on both axes. */
    static float EvalSlope(const FVector2D& Point, int32 WorldSeed, float SampleRadius);

    void AppendTree(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals,
        TArray<FVector2D>& UVs, TArray<FColor>& Colors, const FVector& Base, float Scale, float Yaw,
        EAstrawildDressingCanopy Canopy, const FColor& TrunkColor, const FColor& CanopyColor, FRandomStream& Stream);

    void AppendRock(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals,
        TArray<FVector2D>& UVs, TArray<FColor>& Colors, const FVector& Base, float Scale, const FRotator& Rotation,
        const FColor& RockColor);

    void AppendGrassTuft(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals,
        TArray<FVector2D>& UVs, TArray<FColor>& Colors, const FVector& Base, float Scale, const FColor& GrassColor,
        FRandomStream& Stream);

    /** Real-mesh upgrade path: ISM instances from the biome definition soft refs. */
    void BuildInstancedMeshes(UAstrawildBiomeDefinition* BiomeDef, const TArray<FVector>& TreePoints,
        const TArray<FVector>& RockPoints, const TArray<FVector>& GrassPoints, FRandomStream& Stream);

    UPROPERTY(VisibleAnywhere, Category="ASTRAWILD|Dressing")
    TObjectPtr<UProceduralMeshComponent> DressingMesh;
};
