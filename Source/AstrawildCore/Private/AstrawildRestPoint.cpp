#include "AstrawildRestPoint.h"

#include "Net/UnrealNetwork.h" // LCP-2: DOREPLIFETIME

#include "AstrawildCore.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildSurvivalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildRestPoint::AAstrawildRestPoint()
{
    PrimaryActorTick.bCanEverTick = false;
    WorldObjectId = FGuid::NewGuid();

    // LCP-2: rest points replicate so LAN clients can see + route their
    // interact intent to the server copy.
    bReplicates = true;
    NetUpdateFrequency = 1.0f;

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

    // Resting fully restores the player's vitals (directive §11).
    if (AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(InteractingActor))
    {
        if (UAstrawildSurvivalComponent* Survival = Player->FindComponentByClass<UAstrawildSurvivalComponent>())
        {
            Survival->FullRestore();
        }
    }
}

FText AAstrawildRestPoint::GetInteractionPrompt_Implementation() const
{
    return NSLOCTEXT("ASTRAWILD", "RestPointPrompt", "Rest at the campfire (full recovery) [E]");
}

void AAstrawildRestPoint::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAstrawildRestPoint, WorldObjectId);
    DOREPLIFETIME(AAstrawildRestPoint, bActive);
}

FAstrawildRestPointSaveData AAstrawildRestPoint::ToSaveData() const
{
    FAstrawildRestPointSaveData Data;
    Data.WorldObjectId = WorldObjectId;
    Data.Transform = GetActorTransform();
    Data.bActive = bActive;
    return Data;
}
