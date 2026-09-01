#include "AstrawildBiomeDressingActor.h"

#include "AstrawildArtPack.h"
#include "AstrawildDataAssets.h"
#include "AstrawildLog.h"
#include "AstrawildTerrainTileActor.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "ProceduralMeshComponent.h"

// ===========================================================================
// Zone dressing budget table — every zone reads differently at first glance.
// ===========================================================================

FAstrawildDressingProfile AAstrawildBiomeDressingActor::GetDressingProfile(const EAstrawildZone Zone)
{
    using EZone = EAstrawildZone;
    using ECanopy = EAstrawildDressingCanopy;

    FAstrawildDressingProfile Profile;
    switch (Zone)
    {
    case EZone::DawnFields:
        Profile = { 55, 30, 90, 0.9f, 1.6f, 0.6f, 1.8f, 0.7f, 1.4f, ECanopy::Broadleaf, 0.0f };
        break;
    case EZone::VerdantReach:
        Profile = { 85, 30, 110, 1.0f, 1.9f, 0.6f, 1.8f, 0.8f, 1.6f, ECanopy::Broadleaf, 0.0f };
        break;
    case EZone::Glimmerwood:
        Profile = { 60, 30, 80, 0.9f, 1.7f, 0.7f, 2.0f, 0.7f, 1.3f, ECanopy::Conifer, 0.0f };
        break;
    case EZone::FrostveilExpanse:
        Profile = { 42, 28, 22, 0.8f, 1.5f, 0.7f, 2.1f, 0.6f, 1.2f, ECanopy::Conifer, 0.55f };
        break;
    case EZone::StormcrestHighlands:
        Profile = { 30, 48, 24, 0.7f, 1.3f, 0.8f, 2.3f, 0.6f, 1.1f, ECanopy::Conifer, 0.30f };
        break;
    case EZone::EmberRidge:
        Profile = { 30, 42, 14, 0.9f, 1.8f, 0.7f, 2.2f, 0.6f, 1.2f, ECanopy::Spire, 0.0f };
        break;
    case EZone::DuskMarsh:
        Profile = { 40, 24, 55, 0.9f, 1.7f, 0.6f, 1.7f, 0.9f, 1.7f, ECanopy::Dead, 0.0f };
        break;
    case EZone::HollowApproach:
        Profile = { 28, 28, 10, 0.9f, 1.8f, 0.7f, 2.0f, 0.6f, 1.1f, ECanopy::Dead, 0.0f };
        break;
    case EZone::SunscarDesert:
        Profile = { 22, 34, 30, 0.8f, 1.5f, 0.7f, 2.0f, 0.6f, 1.2f, ECanopy::Cactus, 0.0f };
        break;
    case EZone::AzureShallows:
        Profile = { 14, 18, 22, 0.9f, 1.5f, 0.6f, 1.6f, 0.8f, 1.4f, ECanopy::Palm, 0.0f };
        break;
    case EZone::TidebreakerIsles:
        Profile = { 20, 24, 26, 0.9f, 1.6f, 0.6f, 1.7f, 0.8f, 1.4f, ECanopy::Palm, 0.0f };
        break;
    case EZone::PearlseaReef:
        Profile = { 8, 16, 12, 0.8f, 1.3f, 0.6f, 1.6f, 0.7f, 1.2f, ECanopy::Palm, 0.0f };
        break;
    default:
        Profile = { 20, 20, 20, 0.8f, 1.4f, 0.7f, 1.6f, 0.7f, 1.2f, ECanopy::Broadleaf, 0.0f };
        break;
    }
    return Profile;
}

// ===========================================================================
// Placement gates (pure — automation-tested)
// ===========================================================================

bool AAstrawildBiomeDressingActor::IsPointDressable(const FVector2D& Point, const TArray<FVector2D>& ExclusionCenters,
    const TArray<float>& ExclusionRadii)
{
    const int32 Count = FMath::Min(ExclusionCenters.Num(), ExclusionRadii.Num());
    for (int32 Index = 0; Index < Count; ++Index)
    {
        if (FVector2D::DistSquared(Point, ExclusionCenters[Index]) < FMath::Square(FMath::Max(1.0f, ExclusionRadii[Index])))
        {
            return false;
        }
    }
    return true;
}

float AAstrawildBiomeDressingActor::EvalSlope(const FVector2D& Point, const int32 WorldSeed, const float SampleRadius)
{
    const float H0 = AAstrawildTerrainTileActor::EvalWorldHeight(Point, WorldSeed);
    const float Hx = AAstrawildTerrainTileActor::EvalWorldHeight(Point + FVector2D(SampleRadius, 0.0f), WorldSeed);
    const float Hy = AAstrawildTerrainTileActor::EvalWorldHeight(Point + FVector2D(0.0f, SampleRadius), WorldSeed);
    const float Hx2 = AAstrawildTerrainTileActor::EvalWorldHeight(Point - FVector2D(SampleRadius, 0.0f), WorldSeed);
    const float Hy2 = AAstrawildTerrainTileActor::EvalWorldHeight(Point - FVector2D(0.0f, SampleRadius), WorldSeed);
    return FMath::Max(FMath::Max(FMath::Abs(Hx - H0), FMath::Abs(Hx2 - H0)),
        FMath::Max(FMath::Abs(Hy - H0), FMath::Abs(Hy2 - H0)));
}

void AAstrawildBiomeDressingActor::ScatterDressingPoints(const int32 Seed, const FAstrawildZoneDescriptor& Zone,
    const FAstrawildDressingProfile& Profile, const TArray<FVector2D>& ExclusionCenters,
    const TArray<float>& ExclusionRadii, TArray<FVector>& OutTreePoints, TArray<FVector>& OutRockPoints,
    TArray<FVector>& OutGrassPoints)
{
    OutTreePoints.Reset();
    OutRockPoints.Reset();
    OutGrassPoints.Reset();

    // Deterministic per-zone stream — the layout never changes between runs.
    const int32 ZoneIndex = static_cast<int32>(Zone.Zone);
    FRandomStream Stream(Seed * 7919 + ZoneIndex * 104729);

    const float SeaFloorZ = UAstrawildZoneSubsystem::GetSeaLevelZ() + GetSeaMargin();
    const float Inset = 2500.0f; // keep dressing off the zone seams (blend band lives there)

    // One candidate generator shared by all three kinds.
    const auto TryPlace = [&](const int32 Count, const float SlopeLimit, TArray<FVector>& OutPoints) -> int32
    {
        int32 Placed = 0;
        int32 Attempts = 0;
        const int32 MaxAttempts = Count * 6 + 12;
        while (Placed < Count && Attempts < MaxAttempts)
        {
            ++Attempts;
            const FVector2D Candidate(
                Stream.FRandRange(Zone.Bounds.Min.X + Inset, Zone.Bounds.Max.X - Inset),
                Stream.FRandRange(Zone.Bounds.Min.Y + Inset, Zone.Bounds.Max.Y - Inset));

            if (!IsPointDressable(Candidate, ExclusionCenters, ExclusionRadii))
            {
                continue;
            }
            const float GroundZ = AAstrawildTerrainTileActor::EvalWorldHeight(Candidate, Seed);
            if (GroundZ < SeaFloorZ)
            {
                continue; // water — sea zones keep their islets only
            }
            if (EvalSlope(Candidate, Seed, 180.0f) > SlopeLimit)
            {
                continue;
            }
            OutPoints.Add(FVector(Candidate.X, Candidate.Y, GroundZ));
            ++Placed;
        }
        return Placed;
    };

    TryPlace(Profile.TreeCount, GetTreeSlopeLimit(), OutTreePoints);
    TryPlace(Profile.RockCount, GetRockSlopeLimit(), OutRockPoints);
    TryPlace(Profile.GrassCount, GetGrassSlopeLimit(), OutGrassPoints);
}

// ===========================================================================
// Tint derivation
// ===========================================================================

FLinearColor AAstrawildBiomeDressingActor::ResolveTint(const FLinearColor& ExplicitTint,
    const FLinearColor& ZoneGroundTint, const float Brightness, const float SaturationBoost)
{
    const FLinearColor White(1.0f, 1.0f, 1.0f, 1.0f);
    if (!ExplicitTint.Equals(White, 0.004f))
    {
        return ExplicitTint; // explicit data wins (Antigravity / production content)
    }

    // Derive from the zone identity tint: brightness + saturation shaping keeps
    // each biome's dressing in the same family as its terrain vertex tint.
    FLinearColor Tint = ZoneGroundTint * Brightness;
    const float Luma = Tint.R * 0.3f + Tint.G * 0.59f + Tint.B * 0.11f;
    Tint = FLinearColor(
        Luma + (Tint.R - Luma) * SaturationBoost,
        Luma + (Tint.G - Luma) * SaturationBoost,
        Luma + (Tint.B - Luma) * SaturationBoost,
        1.0f);
    return FLinearColor(FMath::Clamp(Tint.R, 0.02f, 1.0f), FMath::Clamp(Tint.G, 0.02f, 1.0f),
        FMath::Clamp(Tint.B, 0.02f, 1.0f), 1.0f);
}

// ===========================================================================
// Geometry append helpers (merged into per-kind sections)
// ===========================================================================

namespace
{
    void PushDressingQuad(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals,
        TArray<FVector2D>& UVs, TArray<FColor>& Colors, const FVector& A, const FVector& B, const FVector& C,
        const FVector& D, const FColor& Color)
    {
        const int32 Base = Vertices.Num();
        const FVector Normal = FVector::CrossProduct(B - A, D - A).GetSafeNormal();
        Vertices.Append({ A, B, C, D });
        for (int32 i = 0; i < 4; ++i)
        {
            Normals.Add(Normal);
            UVs.Add(FVector2D(0.0f, 0.0f));
            Colors.Add(Color);
        }
        Triangles.Append({ Base, Base + 1, Base + 2, Base, Base + 2, Base + 3 });
    }

    /** Open-ended cylinder (trunks) — cheap, no caps needed at gameplay distance. */
    void PushCylinder(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals,
        TArray<FVector2D>& UVs, TArray<FColor>& Colors, const FVector& BaseCenter, float Radius, float HalfHeight,
        const FColor& Color, const FTransform& World, int32 Sides = 7)
    {
        const int32 Base = Vertices.Num();
        for (int32 Side = 0; Side <= Sides; ++Side)
        {
            const float Theta = 2.0f * PI * static_cast<float>(Side) / static_cast<float>(Sides);
            const FVector Dir(FMath::Cos(Theta), FMath::Sin(Theta), 0.0f);
            const FVector BottomLocal = BaseCenter + FVector(0.0f, 0.0f, -HalfHeight) + Dir * Radius;
            const FVector TopLocal = BaseCenter + FVector(0.0f, 0.0f, HalfHeight) + Dir * Radius;
            Vertices.Add(World.TransformPosition(BottomLocal));
            Normals.Add(World.TransformVector(Dir));
            UVs.Add(FVector2D(0.0f, 0.0f));
            Colors.Add(Color);
            Vertices.Add(World.TransformPosition(TopLocal));
            Normals.Add(World.TransformVector(Dir));
            UVs.Add(FVector2D(0.0f, 1.0f));
            Colors.Add(Color);
        }
        for (int32 Side = 0; Side < Sides; ++Side)
        {
            const int32 A = Base + Side * 2;
            const int32 B = Base + Side * 2 + 1;
            const int32 C = Base + Side * 2 + 3;
            const int32 D = Base + Side * 2 + 2;
            Triangles.Append({ A, B, C, A, C, D });
        }
    }

    /** Cone with base cap (canopies / cactus / grass tufts). */
    void PushCone(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals,
        TArray<FVector2D>& UVs, TArray<FColor>& Colors, const FVector& BaseCenter, float BaseRadius, float Height,
        const FColor& Color, const FTransform& World, int32 Sides = 7)
    {
        const int32 Base = Vertices.Num();
        const FVector TipLocal = BaseCenter + FVector(0.0f, 0.0f, Height);
        Vertices.Add(World.TransformPosition(TipLocal));
        Normals.Add(World.TransformVector(FVector::UpVector));
        UVs.Add(FVector2D(0.5f, 1.0f));
        Colors.Add(Color);
        for (int32 Side = 0; Side <= Sides; ++Side)
        {
            const float Theta = 2.0f * PI * static_cast<float>(Side) / static_cast<float>(Sides);
            const FVector Dir(FMath::Cos(Theta), FMath::Sin(Theta), 0.0f);
            const FVector RingLocal = BaseCenter + Dir * BaseRadius;
            Vertices.Add(World.TransformPosition(RingLocal));
            Normals.Add(World.TransformVector(Dir));
            UVs.Add(FVector2D(0.0f, 0.0f));
            Colors.Add(Color);
        }
        for (int32 Side = 0; Side < Sides; ++Side)
        {
            Triangles.Append({ Base, Base + 1 + Side + 1, Base + 1 + Side });
        }
        // Base cap (visible from ridges above).
        const int32 CapBase = Vertices.Num();
        Vertices.Add(World.TransformPosition(BaseCenter));
        Normals.Add(World.TransformVector(FVector(0.0f, 0.0f, -1.0f)));
        UVs.Add(FVector2D(0.5f, 0.5f));
        Colors.Add(Color);
        for (int32 Side = 0; Side <= Sides; ++Side)
        {
            const float Theta = 2.0f * PI * static_cast<float>(Side) / static_cast<float>(Sides);
            const FVector Dir(FMath::Cos(Theta), FMath::Sin(Theta), 0.0f);
            Vertices.Add(World.TransformPosition(BaseCenter + Dir * BaseRadius));
            Normals.Add(World.TransformVector(FVector(0.0f, 0.0f, -1.0f)));
            UVs.Add(FVector2D(0.0f, 0.0f));
            Colors.Add(Color);
        }
        for (int32 Side = 0; Side < Sides; ++Side)
        {
            Triangles.Append({ CapBase, CapBase + 1 + Side, CapBase + 1 + Side + 1 });
        }
    }

    void PushBox(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals,
        TArray<FVector2D>& UVs, TArray<FColor>& Colors, const FVector& Center, const FVector& HalfSize,
        const FColor& Color, const FTransform& World)
    {
        const FVector Corners[8] = {
            Center + FVector(-HalfSize.X, -HalfSize.Y, -HalfSize.Z),
            Center + FVector( HalfSize.X, -HalfSize.Y, -HalfSize.Z),
            Center + FVector( HalfSize.X,  HalfSize.Y, -HalfSize.Z),
            Center + FVector(-HalfSize.X,  HalfSize.Y, -HalfSize.Z),
            Center + FVector(-HalfSize.X, -HalfSize.Y,  HalfSize.Z),
            Center + FVector( HalfSize.X, -HalfSize.Y,  HalfSize.Z),
            Center + FVector( HalfSize.X,  HalfSize.Y,  HalfSize.Z),
            Center + FVector(-HalfSize.X,  HalfSize.Y,  HalfSize.Z),
        };
        const FVector W0 = World.TransformPosition(Corners[0]);
        const FVector W1 = World.TransformPosition(Corners[1]);
        const FVector W2 = World.TransformPosition(Corners[2]);
        const FVector W3 = World.TransformPosition(Corners[3]);
        const FVector W4 = World.TransformPosition(Corners[4]);
        const FVector W5 = World.TransformPosition(Corners[5]);
        const FVector W6 = World.TransformPosition(Corners[6]);
        const FVector W7 = World.TransformPosition(Corners[7]);
        PushDressingQuad(Vertices, Triangles, Normals, UVs, Colors, W0, W1, W2, W3, Color); // bottom
        PushDressingQuad(Vertices, Triangles, Normals, UVs, Colors, W4, W7, W6, W5, Color); // top
        PushDressingQuad(Vertices, Triangles, Normals, UVs, Colors, W0, W4, W5, W1, Color); // front
        PushDressingQuad(Vertices, Triangles, Normals, UVs, Colors, W3, W2, W6, W7, Color); // back
        PushDressingQuad(Vertices, Triangles, Normals, UVs, Colors, W1, W5, W6, W2, Color); // right
        PushDressingQuad(Vertices, Triangles, Normals, UVs, Colors, W0, W3, W7, W4, Color); // left
    }
}

void AAstrawildBiomeDressingActor::AppendTree(TArray<FVector>& Vertices, TArray<int32>& Triangles,
    TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FColor>& Colors, const FVector& Base, const float Scale,
    const float Yaw, const EAstrawildDressingCanopy Canopy, const FColor& TrunkColor, const FColor& CanopyColor,
    FRandomStream& Stream)
{
    // Trunk lean: dead/marsh trees tilt up to 7 degrees; the rest barely sway.
    const float MaxLean = (Canopy == EAstrawildDressingCanopy::Dead) ? 7.0f : 3.0f;
    const FRotator Lean(Stream.FRandRange(-MaxLean, MaxLean), Yaw, Stream.FRandRange(-2.0f, 2.0f));
    const FTransform TrunkTransform(FRotator(0.0f, Yaw, 0.0f), Base);
    const FTransform LeanTransform(Lean, Base);

    switch (Canopy)
    {
    case EAstrawildDressingCanopy::Broadleaf:
    {
        PushCylinder(Vertices, Triangles, Normals, UVs, Colors, FVector(0, 0, 260 * Scale), 26 * Scale, 260 * Scale,
            TrunkColor, TrunkTransform);
        PushCone(Vertices, Triangles, Normals, UVs, Colors, FVector(0, 0, 430 * Scale), 210 * Scale, 300 * Scale,
            CanopyColor, LeanTransform, 8);
        break;
    }
    case EAstrawildDressingCanopy::Conifer:
    {
        PushCylinder(Vertices, Triangles, Normals, UVs, Colors, FVector(0, 0, 200 * Scale), 20 * Scale, 200 * Scale,
            TrunkColor, TrunkTransform);
        PushCone(Vertices, Triangles, Normals, UVs, Colors, FVector(0, 0, 300 * Scale), 150 * Scale, 620 * Scale,
            CanopyColor, LeanTransform, 8);
        break;
    }
    case EAstrawildDressingCanopy::Palm:
    {
        PushCylinder(Vertices, Triangles, Normals, UVs, Colors, FVector(0, 0, 330 * Scale), 16 * Scale, 330 * Scale,
            TrunkColor, LeanTransform);
        // Flattened crown of fronds: one squat wide cone + a small tip cone.
        PushCone(Vertices, Triangles, Normals, UVs, Colors, FVector(0, 0, 620 * Scale), 190 * Scale, 90 * Scale,
            CanopyColor, TrunkTransform, 8);
        PushCone(Vertices, Triangles, Normals, UVs, Colors, FVector(0, 0, 680 * Scale), 90 * Scale, 150 * Scale,
            CanopyColor, TrunkTransform, 6);
        break;
    }
    case EAstrawildDressingCanopy::Dead:
    {
        PushCylinder(Vertices, Triangles, Normals, UVs, Colors, FVector(0, 0, 300 * Scale), 24 * Scale, 300 * Scale,
            TrunkColor, LeanTransform);
        // Two tilted branch boxes — the gnarled marsh silhouette.
        PushBox(Vertices, Triangles, Normals, UVs, Colors, FVector(90 * Scale, 0, 430 * Scale),
            FVector(110 * Scale, 12 * Scale, 12 * Scale), TrunkColor, FTransform(FRotator(0, Yaw + 20, 28), Base));
        PushBox(Vertices, Triangles, Normals, UVs, Colors, FVector(-70 * Scale, 40 * Scale, 520 * Scale),
            FVector(90 * Scale, 12 * Scale, 12 * Scale), TrunkColor, FTransform(FRotator(0, Yaw + 150, -20), Base));
        break;
    }
    case EAstrawildDressingCanopy::Cactus:
    {
        PushCone(Vertices, Triangles, Normals, UVs, Colors, FVector(0, 0, 40 * Scale), 55 * Scale, 420 * Scale,
            CanopyColor, TrunkTransform, 7);
        PushCone(Vertices, Triangles, Normals, UVs, Colors,
            FVector(110 * Scale, 30 * Scale, 190 * Scale), 34 * Scale, 200 * Scale, CanopyColor,
            FTransform(FRotator(0, Yaw + 40, 0), Base), 6);
        break;
    }
    case EAstrawildDressingCanopy::Spire:
    {
        // Crystal spire straight from the ground with a darker root skirt.
        PushCone(Vertices, Triangles, Normals, UVs, Colors, FVector(0, 0, 30 * Scale), 85 * Scale, 640 * Scale,
            CanopyColor, LeanTransform, 6);
        PushCone(Vertices, Triangles, Normals, UVs, Colors, FVector(0, 0, 10 * Scale), 130 * Scale, 90 * Scale,
            TrunkColor, TrunkTransform, 7);
        break;
    }
    default:
    {
        PushCylinder(Vertices, Triangles, Normals, UVs, Colors, FVector(0, 0, 220 * Scale), 24 * Scale, 220 * Scale,
            TrunkColor, TrunkTransform);
        PushCone(Vertices, Triangles, Normals, UVs, Colors, FVector(0, 0, 360 * Scale), 180 * Scale, 340 * Scale,
            CanopyColor, TrunkTransform, 8);
        break;
    }
    }
}

void AAstrawildBiomeDressingActor::AppendRock(TArray<FVector>& Vertices, TArray<int32>& Triangles,
    TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FColor>& Colors, const FVector& Base, const float Scale,
    const FRotator& Rotation, const FColor& RockColor)
{
    // Irregular boulder: non-uniform box sunk 20% into the ground so it never
    // floats on the height field.
    const FTransform Transform(Rotation, Base - FVector(0, 0, 22 * Scale));
    const FVector HalfSize(
        85 * Scale * (0.8f + 0.4f * Rotation.Roll / 90.0f),
        70 * Scale,
        60 * Scale);
    PushBox(Vertices, Triangles, Normals, UVs, Colors, FVector::ZeroVector, HalfSize, RockColor, Transform);
}

void AAstrawildBiomeDressingActor::AppendGrassTuft(TArray<FVector>& Vertices, TArray<int32>& Triangles,
    TArray<FVector>& Normals, TArray<FVector2D>& UVs, TArray<FColor>& Colors, const FVector& Base, const float Scale,
    const FColor& GrassColor, FRandomStream& Stream)
{
    // Three thin blades leaning apart — reads as a tuft at gameplay distance,
    // four triangles of geometry each.
    for (int32 Blade = 0; Blade < 3; ++Blade)
    {
        const float Angle = Stream.FRandRange(0.0f, 360.0f);
        const float Lean = Stream.FRandRange(8.0f, 20.0f);
        const FTransform Transform(FRotator(Lean, Angle, 0.0f), Base);
        PushCone(Vertices, Triangles, Normals, UVs, Colors, FVector(0, 0, 0), 11 * Scale, 65 * Scale,
            GrassColor, Transform, 4);
    }
}

// ===========================================================================
// Actor
// ===========================================================================

AAstrawildBiomeDressingActor::AAstrawildBiomeDressingActor()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    DressingMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("DressingMesh"));
    DressingMesh->SetupAttachment(RootComponent);
    DressingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); // visual dressing — terrain owns collision
    DressingMesh->SetCastShadow(false);
    DressingMesh->bVisibleInRayTracing = false;
}

void AAstrawildBiomeDressingActor::BuildInstancedMeshes(UAstrawildBiomeDefinition* BiomeDef,
    const TArray<FVector>& TreePoints, const TArray<FVector>& RockPoints, const TArray<FVector>& GrassPoints,
    FRandomStream& Stream)
{
    const auto BuildISMFromPaths = [this](const TArray<const TCHAR*>& Paths, const TArray<FVector>& Points,
        const FName Label, FRandomStream& InStream) -> bool
    {
        TArray<UStaticMesh*> Loaded;
        for (const TCHAR* Path : Paths)
        {
            if (Path && *Path)
            {
                if (UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, Path))
                {
                    Loaded.Add(Mesh);
                }
            }
        }
        if (Loaded.IsEmpty() || Points.IsEmpty())
        {
            return false;
        }

        for (int32 MeshIdx = 0; MeshIdx < Loaded.Num(); ++MeshIdx)
        {
            UStaticMesh* CurrentMesh = Loaded[MeshIdx];
            UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(this,
                *FString::Printf(TEXT("ISM_%s_%d"), *Label.ToString(), MeshIdx));
            if (!ISM)
            {
                continue;
            }
            ISM->RegisterComponent();
            ISM->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
            ISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            ISM->SetCollisionResponseToAllChannels(ECR_Block);
            ISM->SetStaticMesh(CurrentMesh);

            for (int32 Index = MeshIdx; Index < Points.Num(); Index += Loaded.Num())
            {
                const FVector& Point = Points[Index];
                const float Yaw = InStream.FRandRange(0.0f, 360.0f);
                const float Pitch = InStream.FRandRange(-2.5f, 2.5f);
                const float Roll = InStream.FRandRange(-2.5f, 2.5f);
                const float Scale = InStream.FRandRange(0.85f, 1.35f);
                FTransform Instance(FRotator(Pitch, Yaw, Roll), Point, FVector(Scale));
                ISM->AddInstance(Instance, false);
            }
        }
        return true;
    };

    // 1. Check DataAsset first
    if (BiomeDef)
    {
        const auto BuildISMFromSoft = [this](const TArray<TSoftObjectPtr<UStaticMesh>>& Meshes, const TArray<FVector>& Points,
            const FName Label, FRandomStream& InStream) -> bool
        {
            TArray<UStaticMesh*> Loaded;
            for (const TSoftObjectPtr<UStaticMesh>& SoftMesh : Meshes)
            {
                if (UStaticMesh* Mesh = SoftMesh.LoadSynchronous())
                {
                    Loaded.Add(Mesh);
                }
            }
            if (Loaded.IsEmpty() || Points.IsEmpty())
            {
                return false;
            }
            UInstancedStaticMeshComponent* ISM = NewObject<UInstancedStaticMeshComponent>(this, *(FString("ISM_") + Label.ToString()));
            if (ISM)
            {
                ISM->RegisterComponent();
                ISM->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
                ISM->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                ISM->SetCollisionResponseToAllChannels(ECR_Block);
                ISM->SetStaticMesh(Loaded[0]);
                for (int32 Index = 0; Index < Points.Num(); ++Index)
                {
                    const FVector& Point = Points[Index];
                    const float Yaw = InStream.FRandRange(0.0f, 360.0f);
                    const float Scale = InStream.FRandRange(0.85f, 1.25f);
                    FTransform Instance(FRotator(0.0f, Yaw, 0.0f), Point, FVector(Scale));
                    ISM->AddInstance(Instance, false);
                }
            }
            return true;
        };

        BuildISMFromSoft(BiomeDef->TreeMeshes, TreePoints, TEXT("Trees"), Stream);
        BuildISMFromSoft(BiomeDef->RockMeshes, RockPoints, TEXT("Rocks"), Stream);
        BuildISMFromSoft(BiomeDef->GrassMeshes, GrassPoints, TEXT("Grass"), Stream);
    }
    else
    {
        // 2. Resolve real 3D models from AstrawildArtPack
        const TArray<const TCHAR*> Trees = {
            TEXT("/Game/Environment/SM_Tree_Broadleaf.SM_Tree_Broadleaf"),
            TEXT("/Game/Environment/SM_Tree_Conifer.SM_Tree_Conifer"),
            TEXT("/Game/Environment/SM_Tree_SporeCanopy.SM_Tree_SporeCanopy")
        };
        const TArray<const TCHAR*> Rocks = {
            TEXT("/Game/Environment/SM_Rock_Granite_L.SM_Rock_Granite_L"),
            TEXT("/Game/Environment/SM_Rock_Granite_M.SM_Rock_Granite_M"),
            TEXT("/Game/Environment/SM_Rock_Granite_S.SM_Rock_Granite_S"),
            TEXT("/Game/Environment/SM_Rock_Boulder_Moss.SM_Rock_Boulder_Moss")
        };
        const TArray<const TCHAR*> Grass = {
            TEXT("/Game/Environment/SM_Grass_Tuft.SM_Grass_Tuft"),
            TEXT("/Game/Environment/SM_Fern.SM_Fern"),
            TEXT("/Game/Environment/SM_SporeBush.SM_SporeBush"),
            TEXT("/Game/Environment/SM_GlowReed.SM_GlowReed")
        };

        BuildISMFromPaths(Trees, TreePoints, TEXT("Trees"), Stream);
        BuildISMFromPaths(Rocks, RockPoints, TEXT("Rocks"), Stream);
        BuildISMFromPaths(Grass, GrassPoints, TEXT("Grass"), Stream);
    }
}

void AAstrawildBiomeDressingActor::BuildDressing(const FAstrawildZoneDescriptor& Zone,
    UAstrawildBiomeDefinition* BiomeDef, const int32 WorldSeed, const TArray<FVector2D>& ExclusionCenters,
    const TArray<float>& ExclusionRadii)
{
    SetActorLocation(FVector(Zone.GetCenter(), 0.0f));

    // Budget: zone profile x biome density multiplier.
    FAstrawildDressingProfile Profile = GetDressingProfile(Zone.Zone);
    const float Density = BiomeDef ? FMath::Clamp(BiomeDef->DressingDensity, 0.0f, 3.0f) : 1.0f;
    Profile.TreeCount = FMath::RoundToInt(Profile.TreeCount * Density);
    Profile.RockCount = FMath::RoundToInt(Profile.RockCount * Density);
    Profile.GrassCount = FMath::RoundToInt(Profile.GrassCount * Density);

    TArray<FVector> TreePoints, RockPoints, GrassPoints;
    ScatterDressingPoints(WorldSeed, Zone, Profile, ExclusionCenters, ExclusionRadii,
        TreePoints, RockPoints, GrassPoints);

    const int32 ZoneIndex = static_cast<int32>(Zone.Zone);
    FRandomStream Stream(WorldSeed * 7919 + ZoneIndex * 104729 + 13);

    // Build real 3D instanced meshes
    BuildInstancedMeshes(BiomeDef, TreePoints, RockPoints, GrassPoints, Stream);

    // If real meshes loaded, disable placeholder geometry entirely
    const bool bHasRealMeshes = (LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Environment/SM_Tree_Broadleaf.SM_Tree_Broadleaf")) != nullptr);
    const bool bUsePlaceholderTrees = !bHasRealMeshes;
    const bool bUsePlaceholderRocks = !bHasRealMeshes;
    const bool bUsePlaceholderGrass = !bHasRealMeshes;

    // ---- Placeholder geometry (merged sections per kind) ----
    const FLinearColor GroundTint = Zone.GroundTint;
    FLinearColor CanopyTint = ResolveTint(BiomeDef ? BiomeDef->TreeCanopyTint : FLinearColor::White, GroundTint, 0.92f, 1.35f);
    const FLinearColor RockTintLinear = ResolveTint(BiomeDef ? BiomeDef->RockTint : FLinearColor::White, GroundTint, 0.42f, 0.55f);
    const FLinearColor GrassTintLinear = ResolveTint(BiomeDef ? BiomeDef->GrassTuftTint : FLinearColor::White, GroundTint, 1.08f, 1.4f);
    const FLinearColor TrunkTintLinear = ResolveTint(FLinearColor::White, GroundTint, 0.34f, 0.75f);

    // Snow blend (frost biomes): canopy lerps toward white.
    if (Profile.SnowBlend > 0.0f)
    {
        CanopyTint = FMath::Lerp(CanopyTint, FLinearColor(0.93f, 0.95f, 0.98f, 1.0f), FMath::Clamp(Profile.SnowBlend, 0.0f, 1.0f));
    }

    const FColor TrunkColor = TrunkTintLinear.ToFColor(false);
    const FColor CanopyColor = CanopyTint.ToFColor(false);
    const FColor RockColor = RockTintLinear.ToFColor(false);
    const FColor GrassColor = GrassTintLinear.ToFColor(false);

    // Section 0: trees (trunk + canopy merged per instance).
    if (bUsePlaceholderTrees)
    {
        TArray<FVector> SectionVertices;
        TArray<int32> SectionTriangles;
        TArray<FVector> SectionNormals;
        TArray<FVector2D> SectionUVs;
        TArray<FColor> SectionColors;

        for (const FVector& Point : TreePoints)
        {
            const float Scale = Stream.FRandRange(Profile.TreeScaleMin, Profile.TreeScaleMax);
            const float Yaw = Stream.FRandRange(0.0f, 360.0f);
            AppendTree(SectionVertices, SectionTriangles, SectionNormals, SectionUVs, SectionColors, Point, Scale,
                Yaw, Profile.CanopyStyle, TrunkColor, CanopyColor, Stream);
        }

        if (SectionVertices.Num() > 0 && DressingMesh)
        {
            DressingMesh->CreateMeshSection(0, SectionVertices, SectionTriangles, SectionNormals, SectionUVs,
                SectionColors, TArray<FProcMeshTangent>(), false);
        }
    }

    // Section 1: rocks.
    if (bUsePlaceholderRocks)
    {
        TArray<FVector> SectionVertices;
        TArray<int32> SectionTriangles;
        TArray<FVector> SectionNormals;
        TArray<FVector2D> SectionUVs;
        TArray<FColor> SectionColors;

        for (const FVector& Point : RockPoints)
        {
            const float Scale = Stream.FRandRange(Profile.RockScaleMin, Profile.RockScaleMax);
            const FRotator Rotation(Stream.FRandRange(-18.0f, 18.0f), Stream.FRandRange(0.0f, 360.0f), Stream.FRandRange(-14.0f, 14.0f));
            AppendRock(SectionVertices, SectionTriangles, SectionNormals, SectionUVs, SectionColors, Point, Scale,
                Rotation, RockColor);
        }

        if (SectionVertices.Num() > 0 && DressingMesh)
        {
            DressingMesh->CreateMeshSection(1, SectionVertices, SectionTriangles, SectionNormals, SectionUVs,
                SectionColors, TArray<FProcMeshTangent>(), false);
        }
    }

    // Section 2: grass tufts.
    if (bUsePlaceholderGrass)
    {
        TArray<FVector> SectionVertices;
        TArray<int32> SectionTriangles;
        TArray<FVector> SectionNormals;
        TArray<FVector2D> SectionUVs;
        TArray<FColor> SectionColors;

        for (const FVector& Point : GrassPoints)
        {
            const float Scale = Stream.FRandRange(Profile.GrassScaleMin, Profile.GrassScaleMax);
            AppendGrassTuft(SectionVertices, SectionTriangles, SectionNormals, SectionUVs, SectionColors, Point,
                Scale, GrassColor, Stream);
        }

        if (SectionVertices.Num() > 0 && DressingMesh)
        {
            DressingMesh->CreateMeshSection(2, SectionVertices, SectionTriangles, SectionNormals, SectionUVs,
                SectionColors, TArray<FProcMeshTangent>(), false);
        }
    }

    // Shared PBR material across every section.
    if (DressingMesh && DressingMesh->GetNumSections() > 0)
    {
        UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materials/M_Master_Surface.M_Master_Surface"));
        if (!Material)
        {
            Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineDebugMaterials/DebugMeshMaterial.DebugMeshMaterial"));
        }
        if (!Material)
        {
            Material = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
        }
        if (Material)
        {
            for (int32 SectionIndex = 0; SectionIndex < DressingMesh->GetNumSections(); ++SectionIndex)
            {
                DressingMesh->SetMaterial(SectionIndex, Material);
            }
        }
    }

    UE_LOG(LogAstrawildWorld, Log,
        TEXT("Biome dressing built for %s: %d trees, %d rocks, %d grass tufts (placeholder paths: trees=%d rocks=%d grass=%d)."),
        *Zone.ZoneId.ToString(), TreePoints.Num(), RockPoints.Num(), GrassPoints.Num(),
        bUsePlaceholderTrees ? 1 : 0, bUsePlaceholderRocks ? 1 : 0, bUsePlaceholderGrass ? 1 : 0);
}
