#include "AstrawildWaterPlaneActor.h"

#include "AstrawildCore.h"
#include "AstrawildLog.h"
#include "AstrawildZoneSubsystem.h"
#include "Components/BoxComponent.h"
#include "Materials/Material.h"
#include "ProceduralMeshComponent.h"

AAstrawildWaterPlaneActor::AAstrawildWaterPlaneActor()
{
    PrimaryActorTick.bCanEverTick = false;

    // Solid, walkable sea surface (stylized shallow sea — see class comment).
    SurfaceCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("SurfaceCollision"));
    SurfaceCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    SurfaceCollision->SetCollisionResponseToAllChannels(ECR_Block);
    RootComponent = SurfaceCollision;

    SurfaceMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("SurfaceMesh"));
    SurfaceMesh->SetupAttachment(SurfaceCollision);
    SurfaceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AAstrawildWaterPlaneActor::BuildPlane(const FBox2D& WorldRect)
{
    // World-ocean contract: the surface always sits at the global sea level.
    BuildPlaneAtZ(WorldRect, UAstrawildZoneSubsystem::GetSeaLevelZ());
}

void AAstrawildWaterPlaneActor::BuildPlaneAtZ(const FBox2D& WorldRect, const float SurfaceZ, const float BoxThicknessCm)
{
    const FVector Center(WorldRect.GetCenter(), SurfaceZ);
    const float BoxHalfZ = FMath::Max(5.0f, BoxThicknessCm * 0.5f);
    const FVector Extent(WorldRect.GetSize().X * 0.5f, WorldRect.GetSize().Y * 0.5f, BoxHalfZ);

    SetActorLocation(Center);
    SurfaceCollision->InitBoxExtent(Extent);

    // Subdivided water grid with wave crests and deep/shallow color gradient.
    constexpr int32 GridDivs = 16;
    const float StepX = (Extent.X * 2.0f) / static_cast<float>(GridDivs);
    const float StepY = (Extent.Y * 2.0f) / static_cast<float>(GridDivs);

    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FColor> Colors;

    const FColor DeepColor(12, 56, 108, 255);
    const FColor MidColor(32, 108, 160, 255);
    const FColor CrestColor(74, 180, 210, 255);

    Vertices.Reserve((GridDivs + 1) * (GridDivs + 1));
    Normals.Reserve((GridDivs + 1) * (GridDivs + 1));
    UVs.Reserve((GridDivs + 1) * (GridDivs + 1));
    Colors.Reserve((GridDivs + 1) * (GridDivs + 1));
    Triangles.Reserve(GridDivs * GridDivs * 6);

    for (int32 J = 0; J <= GridDivs; ++J)
    {
        for (int32 I = 0; I <= GridDivs; ++I)
        {
            const float LocalX = -Extent.X + static_cast<float>(I) * StepX;
            const float LocalY = -Extent.Y + static_cast<float>(J) * StepY;

            // Subtle wave crest ripple on the water surface.
            const float WaveOffset = FMath::Sin(LocalX * 0.0004f) * FMath::Cos(LocalY * 0.0004f) * 18.0f;
            Vertices.Emplace(LocalX, LocalY, Extent.Z + WaveOffset);

            Normals.Add(FVector::UpVector);
            UVs.Emplace(static_cast<float>(I) / static_cast<float>(GridDivs), static_cast<float>(J) / static_cast<float>(GridDivs));

            // Depth blend based on radial distance from center
            const float DistFraction = FVector2D(LocalX / Extent.X, LocalY / Extent.Y).Size();
            if (DistFraction > 0.8f)
            {
                Colors.Add(CrestColor);
            }
            else if (DistFraction > 0.4f)
            {
                Colors.Add(MidColor);
            }
            else
            {
                Colors.Add(DeepColor);
            }
        }
    }

    const auto VertexIndex = [](const int32 I, const int32 J) -> int32
    {
        return J * (GridDivs + 1) + I;
    };

    for (int32 J = 0; J < GridDivs; ++J)
    {
        for (int32 I = 0; I < GridDivs; ++I)
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

    SurfaceMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);

    UMaterial* WaterMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineDebugMaterials/DebugMeshMaterial.DebugMeshMaterial"));
    if (!WaterMaterial)
    {
        WaterMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
    }
    if (WaterMaterial)
    {
        SurfaceMesh->SetMaterial(0, WaterMaterial);
    }

    UE_LOG(LogAstrawildWorld, Log, TEXT("Water plane built over %s at Z=%.0f (%d verts, %d tris)."),
        *WorldRect.ToString(), SurfaceZ, Vertices.Num(), Triangles.Num() / 3);
}
