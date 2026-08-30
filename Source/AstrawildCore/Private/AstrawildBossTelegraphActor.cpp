#include "AstrawildBossTelegraphActor.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildBossTelegraphActor::AAstrawildBossTelegraphActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // Flat warning disc (engine cylinder — REPLACE_BEFORE_RELEASE with a decal ring).
    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RootComponent = VisualMesh;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
    if (CylinderMesh.Succeeded())
    {
        VisualMesh->SetStaticMesh(CylinderMesh.Object);
    }
    // Cylinder default is 100cm radius x 100cm half-height — flatten to a disc.
    VisualMesh->SetWorldScale3D(FVector(3.5f, 3.5f, 0.04f));

    bReplicates = true;
}

void AAstrawildBossTelegraphActor::BeginPlay()
{
    Super::BeginPlay();
}

void AAstrawildBossTelegraphActor::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Pulse so the warning reads as "imminent" — grow toward the full radius,
    // then flash in the last quarter of the countdown.
    Elapsed += DeltaTime;
    const float Fraction = FMath::Clamp(Elapsed / FMath::Max(0.1f, TelegraphDuration), 0.0f, 1.0f);
    const float Pulse = Fraction > 0.75f ? (0.9f + 0.1f * FMath::Sin(Elapsed * 40.0f)) : (0.35f + 0.65f * Fraction);
    const float RadiusScale = (BlastRadius / 100.0f) * Pulse;
    VisualMesh->SetWorldScale3D(FVector(RadiusScale, RadiusScale, 0.04f));

    if (Elapsed >= TelegraphDuration && GetWorld() && GetLocalRole() == ROLE_Authority)
    {
        GetWorld()->DestroyActor(this);
    }
}
