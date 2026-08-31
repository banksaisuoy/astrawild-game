#include "AstrawildVillageActor.h"

#include "AstrawildCore.h"
#include "AstrawildLog.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"

namespace
{
    constexpr const TCHAR* ShapeCube = TEXT("/Engine/BasicShapes/Cube.Cube");
    constexpr const TCHAR* ShapeCylinder = TEXT("/Engine/BasicShapes/Cylinder.Cylinder");
    constexpr const TCHAR* ShapeCone = TEXT("/Engine/BasicShapes/Cone.Cone");
}

AAstrawildVillageActor::AAstrawildVillageActor()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

FVector AAstrawildVillageActor::GetWaypoint(const int32 Index) const
{
    if (Waypoints.IsEmpty())
    {
        return GetActorLocation();
    }
    const int32 Wrapped = ((Index % Waypoints.Num()) + Waypoints.Num()) % Waypoints.Num();
    return Waypoints[Wrapped];
}

void AAstrawildVillageActor::BeginPlay()
{
    Super::BeginPlay();

    if (GetLocalRole() == ROLE_Authority)
    {
        BuildVillage();
    }
}

AStaticMeshActor* AAstrawildVillageActor::SpawnShape(const TCHAR* MeshPath, const FVector& Location, const FVector& Scale, const FRotator& Rotation)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), Location, Rotation, Params);
    if (Actor)
    {
        Actor->SetMobility(EComponentMobility::Movable);
        if (UStaticMeshComponent* Mesh = Actor->GetStaticMeshComponent())
        {
            if (UStaticMesh* LoadedMesh = LoadObject<UStaticMesh>(nullptr, MeshPath))
            {
                Mesh->SetStaticMesh(LoadedMesh);
            }
            Mesh->SetWorldScale3D(Scale);
            Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Mesh->SetCollisionResponseToAllChannels(ECR_Block);
        }
    }
    return Actor;
}

void AAstrawildVillageActor::SpawnVillageLight(const FVector& Location, const FLinearColor& Color, const float Intensity)
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    APointLight* Light = World->SpawnActor<APointLight>(APointLight::StaticClass(), Location, FRotator::ZeroRotator, Params);
    if (Light)
    {
        Light->SetMobility(EComponentMobility::Movable);
        if (ULightComponent* LightComponent = Light->GetLightComponent())
        {
            LightComponent->SetLightColor(Color);
            LightComponent->SetIntensity(Intensity);
        }
        if (UPointLightComponent* PointComponent = Cast<UPointLightComponent>(Light->GetLightComponent()))
        {
            PointComponent->SetAttenuationRadius(1800.0f);
        }
    }
}

void AAstrawildVillageActor::BuildVillage()
{
    const FVector Center = GetActorLocation();
    const float GroundZ = Center.Z;

    // --- Hut ring (walls + cone roof + post), facing the center. ---
    for (int32 Hut = 0; Hut < HutCount; ++Hut)
    {
        const float Angle = 2.0f * PI * Hut / HutCount;
        const FVector HutCenter(
            Center.X + VillageRadius * FMath::Cos(Angle),
            Center.Y + VillageRadius * FMath::Sin(Angle),
            GroundZ);

        // Hut body.
        SpawnShape(ShapeCube, HutCenter + FVector(0, 0, 130), FVector(2.2, 2.2, 2.6), FRotator(0.0f, FMath::RadiansToDegrees(-Angle), 0.0f));
        // Roof.
        SpawnShape(ShapeCone, HutCenter + FVector(0, 0, 470), FVector(2.6, 2.6, 1.7), FRotator::ZeroRotator);
        // Door frame gap marker (lamp post with warm light).
        if (Hut % 2 == 0)
        {
            SpawnVillageLight(HutCenter - FVector(0, 0, 40), FLinearColor(1.0f, 0.8f, 0.5f), 1.6f);
        }
    }

    // --- Campfire: log ring + warm light — the night gather point. ---
    CampfireLocation = Center + FVector(0, 0, 20);
    for (int32 Log = 0; Log < 5; ++Log)
    {
        const float LogAngle = 2.0f * PI * Log / 5.0f;
        SpawnShape(ShapeCylinder,
            CampfireLocation + FVector(70 * FMath::Cos(LogAngle), 70 * FMath::Sin(LogAngle), 20),
            FVector(0.5, 0.5, 0.5), FRotator(0.0f, FMath::RadiansToDegrees(LogAngle), 75.0f));
    }
    SpawnShape(ShapeCone, CampfireLocation + FVector(0, 0, 60), FVector(0.9, 0.9, 1.2), FRotator::ZeroRotator); // flame marker
    SpawnVillageLight(CampfireLocation + FVector(0, 0, 120), FLinearColor(1.0f, 0.6f, 0.25f), 5.0f);

    // --- Perimeter: palisade posts (inland) or dock planks (coastal). ---
    if (bCoastal)
    {
        for (int32 Plank = 0; Plank < 8; ++Plank)
        {
            SpawnShape(ShapeCube,
                Center + FVector(VillageRadius + 800.0f, -1050.0f + Plank * 300.0f, -30),
                FVector(4.0, 1.4, 0.3), FRotator::ZeroRotator);
        }
    }
    else
    {
        for (int32 Post = 0; Post < 16; ++Post)
        {
            const float Angle = 2.0f * PI * Post / 16.0f;
            SpawnShape(ShapeCylinder,
                Center + FVector((VillageRadius + 900.0f) * FMath::Cos(Angle), (VillageRadius + 900.0f) * FMath::Sin(Angle), 130),
                FVector(0.4, 0.4, 2.6), FRotator::ZeroRotator);
        }
    }

    // --- Waypoint circuit: 6 posts on the ring road between huts and posts. ---
    Waypoints.Reset();
    for (int32 Point = 0; Point < 6; ++Point)
    {
        const float Angle = 2.0f * PI * (Point + 0.5f) / 6.0f;
        const FVector Waypoint(
            Center.X + (VillageRadius + 450.0f) * FMath::Cos(Angle),
            Center.Y + (VillageRadius + 450.0f) * FMath::Sin(Angle),
            GroundZ + 40.0f);
        Waypoints.Add(Waypoint);
        if (Point % 3 == 0)
        {
            SpawnShape(ShapeCylinder, Waypoint - FVector(0, 0, 10), FVector(0.3, 0.3, 0.8), FRotator::ZeroRotator); // road marker
        }
    }

    UE_LOG(LogAstrawildWorld, Log, TEXT("Village %s built: %d huts, %d waypoints, coastal=%d."),
        *VillageId.ToString(), HutCount, Waypoints.Num(), bCoastal ? 1 : 0);
}
