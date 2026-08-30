#include "AstrawildDungeonGateActor.h"

#include "AstrawildCore.h"
#include "AstrawildLog.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildDungeonGateActor::AAstrawildDungeonGateActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    // Blocking volume: thin across the chain axis (X), spanning the passage (Y).
    GateCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("GateCollision"));
    RootComponent = GateCollision;
    GateCollision->SetBoxExtent(FVector(60.0f, 520.0f, 420.0f));
    GateCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    GateCollision->SetCollisionResponseToAllChannels(ECR_Block);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));

    // Pillars flanking the passage (tall cylinders — REPLACE_BEFORE_RELEASE with
    // authored resonance-arch meshes).
    const float PillarOffsetY = 560.0f;
    if (CylinderMesh.Succeeded())
    {
        LeftPillarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftPillarMesh"));
        LeftPillarMesh->SetupAttachment(RootComponent);
        LeftPillarMesh->SetStaticMesh(CylinderMesh.Object);
        LeftPillarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        LeftPillarMesh->SetRelativeLocation(FVector(0.0f, -PillarOffsetY, 300.0f));
        LeftPillarMesh->SetRelativeScale3D(FVector(1.6f, 1.6f, 6.0f));

        RightPillarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightPillarMesh"));
        RightPillarMesh->SetupAttachment(RootComponent);
        RightPillarMesh->SetStaticMesh(CylinderMesh.Object);
        RightPillarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        RightPillarMesh->SetRelativeLocation(FVector(0.0f, PillarOffsetY, 300.0f));
        RightPillarMesh->SetRelativeScale3D(FVector(1.6f, 1.6f, 6.0f));
    }

    // Crossbar: the visible seal. Chest height while sealed; lifted into the
    // lintel while open — the physical metaphor survives the placeholder shapes.
    if (CubeMesh.Succeeded())
    {
        CrossbarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrossbarMesh"));
        CrossbarMesh->SetupAttachment(RootComponent);
        CrossbarMesh->SetStaticMesh(CubeMesh.Object);
        CrossbarMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        // Engine cube = 100cm; span the passage plus the pillar bases.
        CrossbarMesh->SetRelativeScale3D(FVector(1.2f, (PillarOffsetY * 2.0f + 160.0f) / 100.0f, 0.8f));
        CrossbarMesh->SetRelativeLocation(FVector(0.0f, 0.0f, SealedCrossbarZ));
    }
}

void AAstrawildDungeonGateActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAstrawildDungeonGateActor, bOpen);
}

void AAstrawildDungeonGateActor::ApplyGateState()
{
    // Collision is toggled locally on BOTH sides: enabled/disabled does not
    // replicate by itself, so the server applies it directly and clients apply
    // it again in OnRep_bOpen.
    if (GateCollision)
    {
        GateCollision->SetCollisionEnabled(bOpen ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryAndPhysics);
    }
    if (CrossbarMesh)
    {
        CrossbarMesh->SetRelativeLocation(FVector(0.0f, 0.0f, bOpen ? OpenCrossbarZ : SealedCrossbarZ));
    }
}

void AAstrawildDungeonGateActor::OnRep_bOpen()
{
    ApplyGateState();
}

void AAstrawildDungeonGateActor::OpenGate()
{
    if (GetLocalRole() != ROLE_Authority || bOpen)
    {
        return;
    }

    bOpen = true;
    ApplyGateState();
    UE_LOG(LogAstrawildAI, Log, TEXT("Dungeon gate opened."));
}

void AAstrawildDungeonGateActor::SealGate()
{
    if (GetLocalRole() != ROLE_Authority || !bOpen)
    {
        return;
    }

    bOpen = false;
    ApplyGateState();
    UE_LOG(LogAstrawildAI, Log, TEXT("Dungeon gate sealed."));
}
