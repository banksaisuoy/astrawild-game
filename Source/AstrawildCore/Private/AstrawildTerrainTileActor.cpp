#include "AstrawildTerrainTileActor.h"

#include "AstrawildCore.h"
#include "AstrawildLog.h"
#include "Components/ProceduralMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/Material.h"

namespace
{
    // Base field: 512m noise wavelength, 4 octaves.
    constexpr float BaseNoiseWavelength = 51200.0f;
    constexpr int32 BaseNoiseOctaves = 4;

    // Micro detail: 90m wavelength ripple layered over every zone.
    constexpr float MicroDetailWavelength = 9000.0f;
    constexpr float MicroDetailAmplitude = 70.0f;

    // Slope tinting (1 - normal.Z) — steep faces read as rock.
    constexpr float RockTintSlope = 2.2f;

    /** Ridged transform of the base field: 1 - 2|n| in [-1, 1] (sharp crests). */
    float ShapeNoise(const float N, const float RidgeBlend)
    {
        const float Ridged = 1.0f - 2.0f * FMath::Abs(N);
        return FMath::Lerp(N, Ridged, FMath::Clamp(RidgeBlend, 0.0f, 1.0f));
    }

    float SmoothQuintic(float T)
    {
        return T * T * T * (T * (T * 6.0f - 15.0f) + 10.0f);
    }
}

AAstrawildTerrainTileActor::AAstrawildTerrainTileActor()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    Mesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("TerrainMesh"));
    if (Mesh)
    {
        Mesh->SetupAttachment(RootComponent);
        Mesh->bUseAsyncCookCreation = false;
        Mesh->SetMobility(EComponentMobility::Movable);
        Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        Mesh->SetCollisionResponseToAllChannels(ECR_Block);
    }
}

FBox2D AAstrawildTerrainTileActor::GetTileBounds() const
{
    if (const FAstrawildZoneDescriptor* Desc = UAstrawildZoneSubsystem::FindZone(Zone))
    {
        return Desc->Bounds;
    }
    return FBox2D(FVector2D::ZeroVector, FVector2D::ZeroVector);
}

uint32 AAstrawildTerrainTileActor::NoiseHash(const uint32 X, const uint32 Y, const uint32 Seed)
{
    uint32 H = X * 374761393u + Y * 668265263u + Seed * 1442695041u;
    H = (H ^ (H >> 13)) * 1274126177u;
    H ^= (H >> 16);
    return H;
}

float AAstrawildTerrainTileActor::ValueNoise(const FVector2D& P, const uint32 Seed)
{
    const int32 CellX0 = FMath::FloorToInt(P.X);
    const int32 CellY0 = FMath::FloorToInt(P.Y);
    const int32 CellX1 = CellX0 + 1;
    const int32 CellY1 = CellY0 + 1;

    const float Tx = SmoothQuintic(P.X - static_cast<float>(CellX0));
    const float Ty = SmoothQuintic(P.Y - static_cast<float>(CellY0));

    const auto Hash01 = [Seed](const int32 CX, const int32 CY) -> float
    {
        return static_cast<float>(NoiseHash(static_cast<uint32>(CX), static_cast<uint32>(CY), Seed)) / 4294967295.0f;
    };

    const float N00 = Hash01(CellX0, CellY0);
    const float N10 = Hash01(CellX1, CellY0);
    const float N01 = Hash01(CellX0, CellY1);
    const float N11 = Hash01(CellX1, CellY1);

    const float Nx0 = FMath::Lerp(N00, N10, Tx);
    const float Nx1 = FMath::Lerp(N01, N11, Tx);
    return FMath::Lerp(Nx0, Nx1, Ty);
}

float AAstrawildTerrainTileActor::FbmNoise(const FVector2D& P, const uint32 Seed, const int32 Octaves)
{
    float Sum = 0.0f;
    float Amplitude = 0.5f;
    float Total = 0.0f;
    FVector2D Frequency = P;

    for (int32 Octave = 0; Octave < Octaves; ++Octave)
    {
        Sum += Amplitude * ValueNoise(Frequency, Seed + static_cast<uint32>(Octave) * 131u);
        Total += Amplitude;
        Amplitude *= 0.5f;
        Frequency *= 2.0f;
    }
    return Total > 0.0f ? Sum / Total : 0.0f;
}

float AAstrawildTerrainTileActor::EvalBaseNoise(const FVector2D& WorldXY, const int32 Seed)
{
    const float N = FbmNoise(WorldXY / BaseNoiseWavelength, static_cast<uint32>(Seed), BaseNoiseOctaves);
    return N * 2.0f - 1.0f; // [0,1] -> [-1,1]
}

float AAstrawildTerrainTileActor::EvalWorldHeight(const FVector2D& WorldXY, const int32 Seed)
{
    const float Base = EvalBaseNoise(WorldXY, Seed);

    float Weights[(int32)EAstrawildZone::Count];
    UAstrawildZoneSubsystem::ComputeZoneWeights(WorldXY, Weights);

    float Height = 0.0f;
    for (const FAstrawildZoneDescriptor& Desc : UAstrawildZoneSubsystem::GetAllZones())
    {
        const int32 Index = (int32)Desc.Zone;
        const float Weight = (Index >= 0 && Index < (int32)EAstrawildZone::Count) ? Weights[Index] : 0.0f;
        if (Weight <= 0.0f)
        {
            continue;
        }
        const float Shaped = ShapeNoise(Base, Desc.RidgeBlend);
        Height += Weight * (Desc.BaseHeight + Desc.HeightAmplitude * Shaped);
    }

    // Continuous micro-detail ripple everywhere (90m wavelength).
    Height += MicroDetailAmplitude * (FbmNoise(WorldXY / MicroDetailWavelength, static_cast<uint32>(Seed) + 7717u, 2) * 2.0f - 1.0f);

    return Height;
}

FLinearColor AAstrawildTerrainTileActor::EvalGroundTint(const FVector2D& WorldXY, const float Height, const int32 Seed)
{
    float Weights[(int32)EAstrawildZone::Count];
    UAstrawildZoneSubsystem::ComputeZoneWeights(WorldXY, Weights);

    FLinearColor Tint = FLinearColor::Black;

    // Slope estimate from the height field (central differences, 5m half-step).
    constexpr float Eps = 250.0f;
    const float Hx1 = EvalWorldHeight(WorldXY + FVector2D(Eps, 0.0f), Seed);
    const float Hx0 = EvalWorldHeight(WorldXY + FVector2D(-Eps, 0.0f), Seed);
    const float Hy1 = EvalWorldHeight(WorldXY + FVector2D(0.0f, Eps), Seed);
    const float Hy0 = EvalWorldHeight(WorldXY + FVector2D(0.0f, -Eps), Seed);
    const FVector Normal = FVector(-(Hx1 - Hx0), -(Hy1 - Hy0), 2.0f * Eps).GetSafeNormal();
    const float Slope = 1.0f - FMath::Clamp(FMath::Abs(Normal.Z), 0.0f, 1.0f);

    float FrostWeight = 0.0f;
    float EmberWeight = 0.0f;

    for (const FAstrawildZoneDescriptor& Desc : UAstrawildZoneSubsystem::GetAllZones())
    {
        const int32 Index = (int32)Desc.Zone;
        const float Weight = (Index >= 0 && Index < (int32)EAstrawildZone::Count) ? Weights[Index] : 0.0f;
        if (Weight > 0.0f)
        {
            Tint += Desc.GroundTint * Weight;
        }
        if (Desc.Zone == EAstrawildZone::FrostveilExpanse)
        {
            FrostWeight = Weight;
        }
        else if (Desc.Zone == EAstrawildZone::EmberRidge)
        {
            EmberWeight = Weight;
        }
    }

    // Steep slopes read as exposed rock regardless of biome.
    const FLinearColor Rock(0.42f, 0.40f, 0.38f);
    Tint = FMath::Lerp(Tint, Rock, FMath::Clamp(Slope * RockTintSlope, 0.0f, 1.0f));

    // Frostveil: high ground turns to clean snow.
    if (FrostWeight > 0.25f && Height > 1800.0f)
    {
        Tint = FMath::Lerp(Tint, FLinearColor(0.93f, 0.96f, 1.0f), FrostWeight);
    }

    // Ember Ridge: high ridges are bare charred rock.
    if (EmberWeight > 0.25f && Height > 2400.0f)
    {
        Tint = FMath::Lerp(Tint, FLinearColor(0.25f, 0.22f, 0.20f), EmberWeight);
    }

    // Dusk Marsh dips below Z=0 become dark muck-water pools.
    if (Height < 0.0f)
    {
        const FLinearColor MuckWater(0.14f, 0.26f, 0.28f);
        Tint = FMath::Lerp(Tint, MuckWater, FMath::Clamp(-Height / 180.0f, 0.0f, 0.85f));
    }

    // Batch 8 — the sea waterline: a pale sand ring hugging SeaLevel, then a
    // deepening blue shelf as the shelf falls away below it. Applies wherever
    // terrain dips under the global sea line, so islands read as beaches +
    // cliffs and reef spires get turquoise shallows for free.
    {
        const float SeaLevel = UAstrawildZoneSubsystem::GetSeaLevelZ();
        if (Height > SeaLevel && Height < SeaLevel + 350.0f)
        {
            // Beach band above the waterline.
            const float BeachBlend = 1.0f - (Height - SeaLevel) / 350.0f;
            Tint = FMath::Lerp(Tint, FLinearColor(0.85f, 0.80f, 0.62f), 0.7f * BeachBlend);
        }
        else if (Height <= SeaLevel)
        {
            // Underwater shelf: sand near the line, deep blue far below.
            const float Depth = SeaLevel - Height;
            const FLinearColor ShelfSand(0.78f, 0.74f, 0.58f);
            const FLinearColor DeepSea(0.05f, 0.16f, 0.30f);
            const float ShelfBlend = FMath::Clamp(Depth / 900.0f, 0.0f, 1.0f);
            Tint = FMath::Lerp(FMath::Lerp(Tint, ShelfSand, 0.75f), DeepSea, ShelfBlend);
        }
    }

    return Tint;
}

void AAstrawildTerrainTileActor::BuildTile(const FAstrawildZoneDescriptor& InZone, const int32 InSeed, const int32 InResolution)
{
    if (!Mesh)
    {
        return;
    }

    Zone = InZone.Zone;
    CachedSeed = InSeed;

    const int32 N = FMath::Clamp(InResolution, 16, 256);
    const FBox2D& Bounds = InZone.Bounds;
    const float StepX = InZone.GetSizeX() / static_cast<float>(N);
    const float StepY = InZone.GetSizeY() / static_cast<float>(N);

    // Tile origin at the rect corner; vertices are local to the actor.
    SetActorLocation(FVector(Bounds.Min.X, Bounds.Min.Y, 0.0f));

    const int32 VertexCount = (N + 1) * (N + 1);
    TArray<FVector> Vertices;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FColor> VertexColors;
    TArray<int32> Triangles;
    Vertices.Reserve(VertexCount);
    Normals.Reserve(VertexCount);
    UVs.Reserve(VertexCount);
    VertexColors.Reserve(VertexCount);
    Triangles.Reserve(N * N * 6);

    for (int32 J = 0; J <= N; ++J)
    {
        for (int32 I = 0; I <= N; ++I)
        {
            const float LocalX = static_cast<float>(I) * StepX;
            const float LocalY = static_cast<float>(J) * StepY;
            const FVector2D WorldXY(Bounds.Min.X + LocalX, Bounds.Min.Y + LocalY);

            const float Height = EvalWorldHeight(WorldXY, CachedSeed);
            Vertices.Emplace(LocalX, LocalY, Height);
            UVs.Emplace(static_cast<float>(I) / static_cast<float>(N), static_cast<float>(J) / static_cast<float>(N));
            VertexColors.Add(EvalGroundTint(WorldXY, Height, CachedSeed).ToFColor(true));

            // Analytic normal: central differences over the same field.
            constexpr float Eps = 200.0f;
            const float Hx1 = EvalWorldHeight(WorldXY + FVector2D(Eps, 0.0f), CachedSeed);
            const float Hx0 = EvalWorldHeight(WorldXY + FVector2D(-Eps, 0.0f), CachedSeed);
            const float Hy1 = EvalWorldHeight(WorldXY + FVector2D(0.0f, Eps), CachedSeed);
            const float Hy0 = EvalWorldHeight(WorldXY + FVector2D(0.0f, -Eps), CachedSeed);
            Normals.Add(FVector(-(Hx1 - Hx0), -(Hy1 - Hy0), 2.0f * Eps).GetSafeNormal());
        }
    }

    const auto VertexIndex = [N](const int32 I, const int32 J) -> int32
    {
        return J * (N + 1) + I;
    };

    for (int32 J = 0; J < N; ++J)
    {
        for (int32 I = 0; I < N; ++I)
        {
            const int32 V0 = VertexIndex(I, J);
            const int32 V1 = VertexIndex(I, J + 1);
            const int32 V2 = VertexIndex(I + 1, J);
            const int32 V3 = VertexIndex(I + 1, J + 1);

            Triangles.Add(V0);
            Triangles.Add(V1);
            Triangles.Add(V2);
            Triangles.Add(V2);
            Triangles.Add(V1);
            Triangles.Add(V3);
        }
    }

    Mesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, VertexColors, TArray<FProcMeshTangent>(), true);

    // Vertex-color material candidates from engine content (project ships zero assets).
    // DebugMeshMaterial renders vertex colors; fall back to the engine default.
    UMaterial* TileMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineDebugMaterials/DebugMeshMaterial.DebugMeshMaterial"));
    if (!TileMaterial)
    {
        TileMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
    }
    if (TileMaterial)
    {
        Mesh->SetMaterial(0, TileMaterial);
    }

    UE_LOG(LogAstrawildWorld, Log, TEXT("Terrain tile built: %s (%d quads/side, %d verts, %d tris)."),
        *InZone.ZoneId.ToString(), N, Vertices.Num(), Triangles.Num() / 3);
}

float AAstrawildTerrainTileActor::GetTerrainHeightAt(const FVector& WorldLocation) const
{
    return EvalWorldHeight(FVector2D(WorldLocation.X, WorldLocation.Y), CachedSeed);
}
