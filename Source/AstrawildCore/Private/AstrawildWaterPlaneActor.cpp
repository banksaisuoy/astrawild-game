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
    const float SeaLevel = UAstrawildZoneSubsystem::GetSeaLevelZ();
    const FVector Center(WorldRect.GetCenter(), SeaLevel);
    const FVector Extent(WorldRect.GetSize().X * 0.5f, WorldRect.GetSize().Y * 0.5f, 60.0f);

    SetActorLocation(Center);
    SurfaceCollision->InitBoxExtent(Extent);

    // Two triangles, vertex-colored: deep blue with a lighter crest band toward
    // the +X edge so the surface reads as water, not glass.
    TArray<FVector> Vertices;
    TArray<int32> Triangles;
    TArray<FVector> Normals;
    TArray<FVector2D> UVs;
    TArray<FColor> Colors;

    const FColor Deep(0.05f * 255, 0.22f * 255, 0.42f * 255, 255);
    const FColor Shallow(0.16f * 255, 0.42f * 255, 0.62f * 255, 255);

    Vertices.Add(FVector(-Extent.X, -Extent.Y, Extent.Z));
    Vertices.Add(FVector(Extent.X, -Extent.Y, Extent.Z));
    Vertices.Add(FVector(Extent.X, Extent.Y, Extent.Z));
    Vertices.Add(FVector(-Extent.X, Extent.Y, Extent.Z));
    for (int32 i = 0; i < 4; ++i)
    {
        Normals.Add(FVector::UpVector);
        UVs.Add(FVector2D::ZeroVector);
        Colors.Add(i % 2 == 0 ? Deep : Shallow);
    }
    Triangles.Append({ 0, 1, 2, 0, 2, 3 });

    SurfaceMesh->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);

    if (UMaterial* WaterMaterial = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineDebugMaterials/DebugMeshMaterial.DebugMeshMaterial")))
    {
        SurfaceMesh->SetMaterial(0, WaterMaterial);
    }

    UE_LOG(LogAstrawildWorld, Log, TEXT("Water plane built over %s at Z=%.0f."), *WorldRect.ToString(), SeaLevel);
}
