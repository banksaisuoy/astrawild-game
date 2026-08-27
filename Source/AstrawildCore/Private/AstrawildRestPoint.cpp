#include "AstrawildRestPoint.h"

#include "AstrawildCore.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildRestPoint::AAstrawildRestPoint()
{
    PrimaryActorTick.bCanEverTick = false;
    WorldObjectId = FGuid::NewGuid();

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    RootComponent = VisualMesh;
    VisualMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderMesh.Succeeded())
    {
        VisualMesh->SetStaticMesh(CylinderMesh.Object);
        VisualMesh->SetWorldScale3D(FVector(0.75f, 0.75f, 1.4f));
    }
}

void AAstrawildRestPoint::BeginPlay()
{
    Super::BeginPlay();

    if (!WorldObjectId.IsValid())
    {
        WorldObjectId = FGuid::NewGuid();
    }
}

void AAstrawildRestPoint::ActivateRestPoint()
{
    if (bActive)
    {
        return;
    }

    bActive = true;
    OnActivated.Broadcast(this);
}

void AAstrawildRestPoint::Interact_Implementation(AActor* InteractingActor)
{
    if (!IsValid(InteractingActor))
    {
        return;
    }
    ActivateRestPoint();
}

FText AAstrawildRestPoint::GetInteractionPrompt_Implementation() const
{
    return NSLOCTEXT("ASTRAWILD", "RestPointPrompt", "เปิดใช้งานจุดพัก");
}

FAstrawildRestPointSaveData AAstrawildRestPoint::ToSaveData() const
{
    FAstrawildRestPointSaveData Data;
    Data.WorldObjectId = WorldObjectId;
    Data.Transform = GetActorTransform();
    Data.bActive = bActive;
    return Data;
}
