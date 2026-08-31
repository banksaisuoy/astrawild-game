#include "AstrawildVfxActor.h"

#include "AstrawildLog.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "ProceduralMeshComponent.h"

// ===========================================================================
// FAstrawildVfxPalette — one color language for every placeholder effect.
// ===========================================================================

FLinearColor FAstrawildVfxPalette::GetElementTint(const EAstrawildElementType Element)
{
    // Warm-to-cool spectral identity per element — deliberately distinct at
    // gameplay distance (no two elements share a hue family).
    switch (Element)
    {
    case EAstrawildElementType::Light:  return FLinearColor(0.98f, 0.95f, 0.72f); // pale gold
    case EAstrawildElementType::Ash:    return FLinearColor(0.62f, 0.58f, 0.55f); // grey drift
    case EAstrawildElementType::Flora:  return FLinearColor(0.36f, 0.88f, 0.42f); // chlorophyll
    case EAstrawildElementType::Frost:  return FLinearColor(0.55f, 0.88f, 0.98f); // glacial
    case EAstrawildElementType::Pulse:  return FLinearColor(0.36f, 0.92f, 0.86f); // resonant teal
    case EAstrawildElementType::Ember:  return FLinearColor(1.00f, 0.48f, 0.20f); // forge
    case EAstrawildElementType::None:   break;
    default: break;
    }
    return FLinearColor(0.85f, 0.85f, 0.85f); // neutral kinetic
}

FLinearColor FAstrawildVfxPalette::GetRarityTint(const EAstrawildRarity Rarity)
{
    switch (Rarity)
    {
    case EAstrawildRarity::Common:    return FLinearColor(0.78f, 0.78f, 0.78f);
    case EAstrawildRarity::Uncommon:  return FLinearColor(0.40f, 0.90f, 0.44f);
    case EAstrawildRarity::Rare:      return FLinearColor(0.34f, 0.78f, 0.94f); // cyan, never pure blue
    case EAstrawildRarity::Epic:      return FLinearColor(0.76f, 0.42f, 0.95f);
    case EAstrawildRarity::Legendary: return FLinearColor(1.00f, 0.72f, 0.22f); // amber
    case EAstrawildRarity::Mythic:    return FLinearColor(0.96f, 0.28f, 0.42f); // crimson
    default: break;
    }
    return FLinearColor::White;
}

FLinearColor FAstrawildVfxPalette::GetWeaponFamilyTint(const EAstrawildWeaponFamily Family)
{
    // Industrial families stay metallic/warm; energy families run the element
    // spectrum so the held weapon telegraphs its firing behavior.
    switch (Family)
    {
    case EAstrawildWeaponFamily::Kinetic:      return FLinearColor(0.72f, 0.70f, 0.66f); // gunmetal
    case EAstrawildWeaponFamily::Pulse:        return FLinearColor(0.36f, 0.92f, 0.86f); // teal
    case EAstrawildWeaponFamily::Plasma:       return FLinearColor(0.95f, 0.38f, 0.82f); // magenta
    case EAstrawildWeaponFamily::Laser:        return FLinearColor(0.98f, 0.30f, 0.30f); // red
    case EAstrawildWeaponFamily::Arc:          return FLinearColor(0.55f, 0.85f, 1.00f); // electric
    case EAstrawildWeaponFamily::Rail:         return FLinearColor(0.90f, 0.88f, 0.80f); // charged white
    case EAstrawildWeaponFamily::Missile:      return FLinearColor(1.00f, 0.55f, 0.25f); // exhaust
    case EAstrawildWeaponFamily::Experimental: return FLinearColor(1.00f, 0.80f, 0.35f); // starlance gold
    default: break;
    }
    return FLinearColor(0.70f, 0.70f, 0.70f);
}

FLinearColor FAstrawildVfxPalette::GetScannerTint(const FName ScannerItemId)
{
    // Tier identity: Field = clean teal, Array = amber instrumentation,
    // Oracle = violet resonance (matches the hidden-vein/ancient-signal theme).
    if (ScannerItemId == TEXT("Item_ArrayScanner"))
    {
        return FLinearColor(1.00f, 0.76f, 0.30f);
    }
    if (ScannerItemId == TEXT("Item_OracleScanner"))
    {
        return FLinearColor(0.78f, 0.48f, 1.00f);
    }
    return FLinearColor(0.42f, 0.92f, 0.86f);
}

// ===========================================================================
// AAstrawildBeamVfxActor
// ===========================================================================

namespace
{
    /** Append one axis-aligned quad into shared mesh arrays. */
    void AppendQuad(TArray<FVector>& Vertices, TArray<int32>& Triangles, TArray<FVector>& Normals,
        TArray<FVector2D>& UVs, TArray<FColor>& Colors,
        const FVector& A, const FVector& B, const FVector& C, const FVector& D, const FColor& Color)
    {
        const int32 Base = Vertices.Num();
        const FVector Edge1 = B - A;
        const FVector Edge2 = D - A;
        const FVector Normal = FVector::CrossProduct(Edge1, Edge2).GetSafeNormal();

        Vertices.Append({ A, B, C, D });
        for (int32 i = 0; i < 4; ++i)
        {
            Normals.Add(Normal);
            UVs.Add(FVector2D(0.0f, 0.0f));
            Colors.Add(Color);
        }
        Triangles.Append({ Base, Base + 1, Base + 2, Base, Base + 2, Base + 3 });
    }
}

AAstrawildBeamVfxActor::AAstrawildBeamVfxActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.0f; // smooth fade — transient actor lives < 0.3s

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    BeamMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("BeamMesh"));
    BeamMesh->SetupAttachment(RootComponent);
    BeamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BeamMesh->SetCastShadow(false);
    BeamMesh->bVisibleInRayTracing = false;

    BeamLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("BeamLight"));
    BeamLight->SetupAttachment(RootComponent);
    BeamLight->SetCastShadows(false);
    BeamLight->SetIntensity(0.0f);
    BeamLight->SetAttenuationRadius(900.0f);
}

UMaterial* AAstrawildBeamVfxActor::LoadVertexColorMaterial()
{
    // Same guaranteed vertex-color path as the terrain tiles and Echo bodies.
    UMaterial* Material = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineDebugMaterials/DebugMeshMaterial.DebugMeshMaterial"));
    if (!Material)
    {
        Material = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial"));
    }
    return Material;
}

bool AAstrawildBeamVfxActor::ComputeBeamTransform(const FVector& Start, const FVector& End, FVector& OutCenter,
    FRotator& OutRotation, float& OutLength)
{
    const FVector Delta = End - Start;
    OutLength = Delta.Size();
    if (OutLength < 1.0f)
    {
        return false; // coincident points — no beam to draw
    }
    OutCenter = Start + Delta * 0.5f;
    OutRotation = Delta.Rotation();
    return true;
}

void AAstrawildBeamVfxActor::ComputeArcJitter(const int32 Seed, const FVector& A, const FVector& B,
    const int32 SubSegmentCount, const float JitterAmplitude, TArray<FVector>& OutWaypoints)
{
    OutWaypoints.Reset();
    OutWaypoints.Add(A);

    const int32 SubSegments = FMath::Max(0, SubSegmentCount);
    if (SubSegments == 0)
    {
        OutWaypoints.Add(B);
        return;
    }

    FRandomStream Stream(Seed);
    const FVector Direction = (B - A).GetSafeNormal();
    // Deterministic perpendicular basis for the jitter plane.
    FVector Side = FVector::CrossProduct(Direction, FVector::UpVector);
    if (Side.SizeSquared() < 0.01f)
    {
        Side = FVector::CrossProduct(Direction, FVector::ForwardVector);
    }
    Side = Side.GetSafeNormal();
    const FVector Up = FVector::CrossProduct(Side, Direction).GetSafeNormal();

    for (int32 Index = 1; Index <= SubSegments; ++Index)
    {
        const float Alpha = static_cast<float>(Index) / static_cast<float>(SubSegments + 1);
        FVector Point = FMath::Lerp(A, B, Alpha);
        // Every interior waypoint jitters EXCEPT the final approach — the bolt
        // zig-zags mid-flight but arrives clean on its target.
        if (Index < SubSegments)
        {
            const float Lateral = Stream.FRandRange(-JitterAmplitude, JitterAmplitude);
            const float Vertical = Stream.FRandRange(-JitterAmplitude, JitterAmplitude);
            Point += Side * Lateral + Up * Vertical;
        }
        OutWaypoints.Add(Point);
    }
    OutWaypoints.Add(B);
}

void AAstrawildBeamVfxActor::BuildBeamGeometry(const TArray<FVector>& InWaypoints, const FLinearColor& Tint, const bool bJagged)
{
    if (!BeamMesh || InWaypoints.Num() < 2)
    {
        return;
    }

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FColor> Colors;

    const FColor CoreColor = Tint.ToFColor(false);
    const FColor TipColor = FLinearColor(Tint.R * 1.25f + 0.05f, Tint.G * 1.25f + 0.05f, Tint.B * 1.25f + 0.05f, 1.0f).ToFColor(false);

    // Each consecutive waypoint pair becomes one local prism segment oriented
    // along that hop; segments are stored relative to the actor origin at the
    // FIRST waypoint so a single actor meshes the whole (possibly jagged) path.
    const FVector ActorOrigin = InWaypoints[0];
    for (int32 Index = 0; Index < InWaypoints.Num() - 1; ++Index)
    {
        const FVector Start = InWaypoints[Index] - ActorOrigin;
        const FVector End = InWaypoints[Index + 1] - ActorOrigin;
        FVector Center;
        FRotator Rotation;
        float Length = 0.0f;
        if (!ComputeBeamTransform(Start, End, Center, Rotation, Length))
        {
            continue;
        }

        const float H = 0.5f;
        const FTransform SegmentTransform(Rotation, Center);
        const FVector A = SegmentTransform.TransformPosition(FVector(-Length * 0.5f, -H, -H));
        const FVector B = SegmentTransform.TransformPosition(FVector(Length * 0.5f, -H, -H));
        const FVector C = SegmentTransform.TransformPosition(FVector(Length * 0.5f, H, -H));
        const FVector D = SegmentTransform.TransformPosition(FVector(-Length * 0.5f, H, -H));
        const FVector E = SegmentTransform.TransformPosition(FVector(-Length * 0.5f, -H, H));
        const FVector Fp = SegmentTransform.TransformPosition(FVector(Length * 0.5f, -H, H));
        const FVector G = SegmentTransform.TransformPosition(FVector(Length * 0.5f, H, H));
        const FVector Ht = SegmentTransform.TransformPosition(FVector(-Length * 0.5f, H, H));

        AppendQuad(Vertices, Triangles, Normals, UVs, Colors, A, B, C, D, CoreColor);
        AppendQuad(Vertices, Triangles, Normals, UVs, Colors, E, Ht, G, Fp, CoreColor);
        AppendQuad(Vertices, Triangles, Normals, UVs, Colors, A, E, Fp, B, CoreColor);
        AppendQuad(Vertices, Triangles, Normals, UVs, Colors, D, C, G, Ht, CoreColor);
        AppendQuad(Vertices, Triangles, Normals, UVs, Colors, B, Fp, G, C, TipColor);
        AppendQuad(Vertices, Triangles, Normals, UVs, Colors, A, D, Ht, E, TipColor);
    }

    if (Vertices.Num() == 0)
    {
        return;
    }

    SetActorLocation(ActorOrigin);
    BeamMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);
    if (UMaterial* Material = LoadVertexColorMaterial())
    {
        BeamMesh->SetMaterial(0, Material);
    }

    // Midpoint light: brightest on jagged chains (arc weapons read as violent).
    const FVector MidPoint = InWaypoints[InWaypoints.Num() / 2];
    if (BeamLight)
    {
        BeamLight->SetRelativeLocation(MidPoint - ActorOrigin);
        BeamLight->SetLightColor(Tint);
        BeamLight->SetIntensity(bJagged ? 8000.0f : 5000.0f);
    }
}

void AAstrawildBeamVfxActor::BuildMuzzleGeometry(const FLinearColor& Tint)
{
    if (!BeamMesh)
    {
        return;
    }

    // Small bright octahedron-ish core: two cones tip to tip along the fire
    // direction, baked in local space (actor rotation carries the direction).
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FColor> Colors;

    const FColor CoreColor = FLinearColor(Tint.R * 1.3f + 0.1f, Tint.G * 1.3f + 0.1f, Tint.B * 1.3f + 0.1f, 1.0f).ToFColor(false);
    const FColor EdgeColor = Tint.ToFColor(false);

    const int32 Slices = 8;
    const float Radius = 22.0f;
    const FVector FrontTip(Radius * 1.6f, 0.0f, 0.0f);
    const FVector BackTip(-Radius * 1.1f, 0.0f, 0.0f);
    const FVector RingCenter(0.0f, 0.0f, 0.0f);

    const int32 Base = Vertices.Num();
    Vertices.Add(FrontTip);
    Normals.Add(FVector(1.0f, 0.0f, 0.0f));
    UVs.Add(FVector2D::ZeroVector);
    Colors.Add(CoreColor);
    for (int32 Slice = 0; Slice <= Slices; ++Slice)
    {
        const float Theta = 2.0f * PI * static_cast<float>(Slice) / static_cast<float>(Slices);
        Vertices.Add(RingCenter + FVector(0.0f, FMath::Cos(Theta) * Radius, FMath::Sin(Theta) * Radius));
        Normals.Add(FVector(0.0f, FMath::Cos(Theta), FMath::Sin(Theta)));
        UVs.Add(FVector2D::ZeroVector);
        Colors.Add(EdgeColor);
    }
    for (int32 Slice = 0; Slice < Slices; ++Slice)
    {
        Triangles.Append({ Base, Base + 1 + Slice + 1, Base + 1 + Slice });
    }

    const int32 BackBase = Vertices.Num();
    Vertices.Add(BackTip);
    Normals.Add(FVector(-1.0f, 0.0f, 0.0f));
    UVs.Add(FVector2D::ZeroVector);
    Colors.Add(CoreColor);
    for (int32 Slice = 0; Slice <= Slices; ++Slice)
    {
        const float Theta = 2.0f * PI * static_cast<float>(Slice) / static_cast<float>(Slices);
        Vertices.Add(RingCenter + FVector(0.0f, FMath::Cos(Theta) * Radius, FMath::Sin(Theta) * Radius));
        Normals.Add(FVector(0.0f, FMath::Cos(Theta), FMath::Sin(Theta)));
        UVs.Add(FVector2D::ZeroVector);
        Colors.Add(EdgeColor);
    }
    for (int32 Slice = 0; Slice < Slices; ++Slice)
    {
        Triangles.Append({ BackBase, BackBase + 1 + Slice, BackBase + 1 + Slice + 1 });
    }

    BeamMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);
    if (UMaterial* Material = LoadVertexColorMaterial())
    {
        BeamMesh->SetMaterial(0, Material);
    }

    if (BeamLight)
    {
        BeamLight->SetRelativeLocation(FVector::ZeroVector);
        BeamLight->SetLightColor(Tint);
        BeamLight->SetIntensity(12000.0f);
        BeamLight->SetAttenuationRadius(700.0f);
    }
}

AAstrawildBeamVfxActor* AAstrawildBeamVfxActor::SpawnBeam(UWorld* World, const FVector& Start, const FVector& End,
    const FLinearColor& Tint, const float Thickness, const float LifetimeSeconds)
{
    if (!World || World->GetNetMode() == NM_DedicatedServer)
    {
        return nullptr; // Cosmetic: nothing to draw on a headless server.
    }

    FVector Center;
    FRotator Rotation;
    float Length = 0.0f;
    if (!ComputeBeamTransform(Start, End, Center, Rotation, Length))
    {
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AAstrawildBeamVfxActor* Vfx = World->SpawnActor<AAstrawildBeamVfxActor>(
        AAstrawildBeamVfxActor::StaticClass(), Center, Rotation, Params);
    if (!Vfx)
    {
        return nullptr;
    }

    Vfx->LifetimeSeconds = FMath::Max(0.05f, LifetimeSeconds);
    Vfx->BuildBeamGeometry({ Start, End }, Tint, false);
    Vfx->BeamMesh->SetRelativeScale3D(FVector(1.0f, FMath::Max(2.0f, Thickness), FMath::Max(2.0f, Thickness)));
    return Vfx;
}

AAstrawildBeamVfxActor* AAstrawildBeamVfxActor::SpawnArcChain(UWorld* World, const TArray<FVector>& HopPoints,
    const FLinearColor& Tint, const float LifetimeSeconds)
{
    if (!World || World->GetNetMode() == NM_DedicatedServer || HopPoints.Num() < 2)
    {
        return nullptr;
    }

    // Assemble the full jittered path: each hop splits into zig-zag waypoints.
    TArray<FVector> Waypoints;
    for (int32 Index = 0; Index < HopPoints.Num() - 1; ++Index)
    {
        TArray<FVector> HopWaypoints;
        // Deterministic per-hop seed from the quantized hop delta (no engine
        // hash dependency — the lightning shape is stable across runs).
        const FVector Delta = HopPoints[Index] - HopPoints[Index + 1];
        const uint32 Hx = static_cast<uint32>(FMath::Abs(FMath::FloorToInt(Delta.X * 0.01f)));
        const uint32 Hy = static_cast<uint32>(FMath::Abs(FMath::FloorToInt(Delta.Y * 0.01f))) * 31u;
        const uint32 Hz = static_cast<uint32>(FMath::Abs(FMath::FloorToInt(Delta.Z * 0.01f))) * 961u;
        const int32 Seed = static_cast<int32>((Hx ^ Hy ^ Hz) & 0x7fffffffu);
        ComputeArcJitter(Seed, HopPoints[Index], HopPoints[Index + 1], 3, 34.0f, HopWaypoints);
        if (Index == 0)
        {
            Waypoints.Append(HopWaypoints);
        }
        else
        {
            Waypoints.Append(HopWaypoints.GetData() + 1, HopWaypoints.Num() - 1); // skip duplicated joint
        }
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AAstrawildBeamVfxActor* Vfx = World->SpawnActor<AAstrawildBeamVfxActor>(
        AAstrawildBeamVfxActor::StaticClass(), Waypoints[0], FRotator::ZeroRotator, Params);
    if (!Vfx)
    {
        return nullptr;
    }

    Vfx->LifetimeSeconds = FMath::Max(0.05f, LifetimeSeconds);
    Vfx->BuildBeamGeometry(Waypoints, Tint, true);
    Vfx->BeamMesh->SetRelativeScale3D(FVector(1.0f, 11.0f, 11.0f));
    return Vfx;
}

AAstrawildBeamVfxActor* AAstrawildBeamVfxActor::SpawnMuzzleFlash(UWorld* World, const FVector& Location,
    const FVector& Direction, const FLinearColor& Tint, const float LifetimeSeconds)
{
    if (!World || World->GetNetMode() == NM_DedicatedServer)
    {
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AAstrawildBeamVfxActor* Vfx = World->SpawnActor<AAstrawildBeamVfxActor>(
        AAstrawildBeamVfxActor::StaticClass(), Location, Direction.Rotation(), Params);
    if (!Vfx)
    {
        return nullptr;
    }

    Vfx->LifetimeSeconds = FMath::Max(0.04f, LifetimeSeconds);
    Vfx->BuildMuzzleGeometry(Tint);
    return Vfx;
}

void AAstrawildBeamVfxActor::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    ElapsedSeconds += DeltaTime;
    const float Alpha = FMath::Clamp(ElapsedSeconds / FMath::Max(0.01f, LifetimeSeconds), 0.0f, 1.0f);
    const float Fade = 1.0f - Alpha;

    if (BeamMesh)
    {
        // Collapse thickness; keep length so the beam "burns out" rather than shrinking.
        const FVector Scale = BeamMesh->GetRelativeScale3D();
        const float Thickness = FMath::Max(0.05f, Scale.Y * Fade);
        BeamMesh->SetRelativeScale3D(FVector(Scale.X, Thickness, Thickness));
    }
    if (BeamLight)
    {
        BeamLight->SetIntensity(BeamLight->Intensity * FMath::Max(0.0f, Fade));
    }

    if (Alpha >= 1.0f)
    {
        Destroy();
    }
}

// ===========================================================================
// AAstrawildScannerPulseActor
// ===========================================================================

AAstrawildScannerPulseActor::AAstrawildScannerPulseActor()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.0f;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

    RingMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("RingMesh"));
    RingMesh->SetupAttachment(RootComponent);
    RingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RingMesh->SetCastShadow(false);
    RingMesh->bVisibleInRayTracing = false;

    PulseLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("PulseLight"));
    PulseLight->SetupAttachment(RootComponent);
    PulseLight->SetCastShadows(false);
    PulseLight->SetAttenuationRadius(1200.0f);
}

void AAstrawildScannerPulseActor::BuildRingGeometry(const int32 Segments, const float InnerFraction,
    const FLinearColor& Tint, TArray<FVector>& OutVertices, TArray<int32>& OutTriangles,
    TArray<FVector>& OutNormals, TArray<FVector2D>& OutUVs, TArray<FColor>& OutColors)
{
    OutVertices.Reset();
    OutTriangles.Reset();
    OutNormals.Reset();
    OutUVs.Reset();
    OutColors.Reset();

    const int32 SafeSegments = FMath::Max(3, Segments);
    const float Inner = FMath::Clamp(InnerFraction, 0.05f, 0.98f);

    const FColor OuterColor = Tint.ToFColor(false);
    const FColor InnerColor = FLinearColor(Tint.R * 0.45f, Tint.G * 0.45f, Tint.B * 0.45f, 1.0f).ToFColor(false);

    // Flat horizontal annulus at unit outer radius (scaled at runtime).
    for (int32 Slice = 0; Slice <= SafeSegments; ++Slice)
    {
        const float Theta = 2.0f * PI * static_cast<float>(Slice) / static_cast<float>(SafeSegments);
        const float CosT = FMath::Cos(Theta);
        const float SinT = FMath::Sin(Theta);

        OutVertices.Add(FVector(CosT, SinT, 0.0f));          // outer ring
        OutNormals.Add(FVector(0.0f, 0.0f, 1.0f));
        OutUVs.Add(FVector2D(1.0f, 0.0f));
        OutColors.Add(OuterColor);

        OutVertices.Add(FVector(CosT * Inner, SinT * Inner, 0.0f)); // inner ring
        OutNormals.Add(FVector(0.0f, 0.0f, 1.0f));
        OutUVs.Add(FVector2D(0.0f, 0.0f));
        OutColors.Add(InnerColor);
    }

    for (int32 Slice = 0; Slice < SafeSegments; ++Slice)
    {
        const int32 OuterA = Slice * 2;
        const int32 InnerA = Slice * 2 + 1;
        const int32 OuterB = (Slice + 1) * 2;
        const int32 InnerB = (Slice + 1) * 2 + 1;
        OutTriangles.Append({ OuterA, OuterB, InnerB, OuterA, InnerB, InnerA });
    }
}

AAstrawildScannerPulseActor* AAstrawildScannerPulseActor::SpawnPulse(UWorld* World, const FVector& Origin,
    const FLinearColor& Tint, const float MaxRadius, const float DurationSeconds)
{
    if (!World || World->GetNetMode() == NM_DedicatedServer)
    {
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AAstrawildScannerPulseActor* Pulse = World->SpawnActor<AAstrawildScannerPulseActor>(
        AAstrawildScannerPulseActor::StaticClass(), Origin, FRotator::ZeroRotator, Params);
    if (!Pulse)
    {
        return nullptr;
    }

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FColor> Colors;
    BuildRingGeometry(48, 0.86f, Tint, Vertices, Triangles, Normals, UVs, Colors);

    Pulse->RingMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);
    if (UMaterial* Material = AAstrawildBeamVfxActor::LoadVertexColorMaterial())
    {
        Pulse->RingMesh->SetMaterial(0, Material);
    }

    Pulse->DurationSeconds = FMath::Max(0.2f, DurationSeconds);
    Pulse->MaxRadius = FMath::Max(500.0f, MaxRadius);
    Pulse->RingMesh->SetRelativeScale3D(FVector(0.05f, 0.05f, 1.0f));

    Pulse->PulseLight->SetLightColor(Tint);
    Pulse->PulseLight->SetIntensity(6000.0f);
    return Pulse;
}

void AAstrawildScannerPulseActor::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    ElapsedSeconds += DeltaTime;
    const float Alpha = FMath::Clamp(ElapsedSeconds / FMath::Max(0.01f, DurationSeconds), 0.0f, 1.0f);

    // Ease-out expansion: fast initial burst, gentle arrival at max radius.
    const float Expanded = 1.0f - FMath::Square(1.0f - Alpha);
    const float Radius = FMath::Max(50.0f, MaxRadius * Expanded);
    if (RingMesh)
    {
        RingMesh->SetRelativeScale3D(FVector(Radius, Radius, 1.0f));
    }
    if (PulseLight)
    {
        PulseLight->SetIntensity(6000.0f * (1.0f - Alpha));
    }

    if (Alpha >= 1.0f)
    {
        Destroy();
    }
}

AAstrawildCaptureVfxActor::AAstrawildCaptureVfxActor()
{
    PrimaryActorTick.bCanEverTick = true;

    CaptureMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("CaptureMesh"));
    CaptureMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CaptureMesh->SetCastShadow(false);
    RootComponent = CaptureMesh;

    CaptureLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CaptureLight"));
    CaptureLight->SetupAttachment(CaptureMesh);
    CaptureLight->SetAttenuationRadius(800.0f);
    CaptureLight->SetCastShadows(false);
    CaptureLight->SetIntensity(0.0f);
}

AAstrawildCaptureVfxActor* AAstrawildCaptureVfxActor::SpawnCaptureVfx(UWorld* World, const FVector& TargetLocation,
    const FLinearColor& Tint, const float InInitialRadius, const float InDurationSeconds)
{
    if (!World || World->GetNetMode() == NM_DedicatedServer)
    {
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AAstrawildCaptureVfxActor* CaptureVfx = World->SpawnActor<AAstrawildCaptureVfxActor>(
        AAstrawildCaptureVfxActor::StaticClass(), TargetLocation, FRotator::ZeroRotator, Params);
    if (!CaptureVfx)
    {
        return nullptr;
    }

    CaptureVfx->BaseTint = Tint;
    CaptureVfx->InitialRadius = FMath::Max(60.0f, InInitialRadius);
    CaptureVfx->DurationSeconds = FMath::Max(0.3f, InDurationSeconds);

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FColor> Colors;

    AAstrawildScannerPulseActor::BuildRingGeometry(32, 0.90f, Tint, Vertices, Triangles, Normals, UVs, Colors);

    CaptureVfx->CaptureMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);

    if (UMaterial* Material = AAstrawildBeamVfxActor::LoadVertexColorMaterial())
    {
        CaptureVfx->CaptureMesh->SetMaterial(0, Material);
    }

    CaptureVfx->CaptureLight->SetLightColor(Tint);
    CaptureVfx->CaptureLight->SetIntensity(5000.0f);
    return CaptureVfx;
}

void AAstrawildCaptureVfxActor::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    ElapsedSeconds += DeltaTime;
    const float Alpha = FMath::Clamp(ElapsedSeconds / FMath::Max(0.01f, DurationSeconds), 0.0f, 1.0f);

    // Fast spin + inward contraction
    const float CurrentScale = FMath::Max(0.05f, (1.0f - Alpha) * InitialRadius);
    if (CaptureMesh)
    {
        CaptureMesh->SetRelativeScale3D(FVector(CurrentScale, CurrentScale, CurrentScale));
        CaptureMesh->AddLocalRotation(FRotator(DeltaTime * 180.0f, DeltaTime * 240.0f, DeltaTime * 120.0f));
    }

    if (CaptureLight)
    {
        // Light pulses bright as it collapses
        const float PulseIntensity = 3000.0f + 5000.0f * FMath::Sin(Alpha * PI);
        CaptureLight->SetIntensity(PulseIntensity);
    }

    if (Alpha >= 1.0f)
    {
        Destroy();
    }
}
