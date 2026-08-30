#include "AstrawildUtilityRobotActor.h"

#include "AstrawildCore.h"
#include "AstrawildLog.h"
#include "AstrawildWorkSiteActor.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildUtilityRobotActor::AAstrawildUtilityRobotActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // Placeholder chassis (zero-asset playability — REPLACE_BEFORE_RELEASE).
    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RootComponent = VisualMesh;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        VisualMesh->SetStaticMesh(CubeMesh.Object);
    }
    VisualMesh->SetWorldScale3D(FVector(0.5f, 0.5f, 0.8f));

    bReplicates = true;
}

void AAstrawildUtilityRobotActor::BeginPlay()
{
    Super::BeginPlay();

    IdlePhase = FMath::FRandRange(0.0f, PI * 2.0f);
}

AAstrawildWorkSiteActor* AAstrawildUtilityRobotActor::GetAssignedSite() const
{
    return AssignedSite.Get();
}

FName AAstrawildUtilityRobotActor::GetAssignedSiteId() const
{
    if (const AAstrawildWorkSiteActor* Site = AssignedSite.Get())
    {
        return Site->SiteId;
    }
    return NAME_None;
}

void AAstrawildUtilityRobotActor::AssignToSite(AAstrawildWorkSiteActor* Site)
{
    if (!IsValid(Site))
    {
        return;
    }

    // Leave any previous site first (one robot, one site).
    ReleaseFromSite();

    AssignedSite = Site;
    Site->AssignRobot(this);

    UE_LOG(LogAstrawildAI, Log, TEXT("Utility robot assigned to site %s."), *Site->SiteId.ToString());
}

void AAstrawildUtilityRobotActor::ReleaseFromSite()
{
    if (AAstrawildWorkSiteActor* Site = AssignedSite.Get())
    {
        Site->RemoveRobot(this);
    }
    AssignedSite = nullptr;
}

void AAstrawildUtilityRobotActor::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (AAstrawildWorkSiteActor* Site = AssignedSite.Get())
    {
        // Walk to the site's stand-off point and hold position (the site's Tick
        // produces output while the robot is parked).
        const FVector Target = Site->GetActorLocation() + Site->GetActorForwardVector() * -StandoffDistance;
        const FVector NewLocation = FMath::VInterpTo(GetActorLocation(), Target, DeltaTime, MoveInterpSpeed);
        SetActorLocation(NewLocation);
    }
    else
    {
        // Idle sway — parked robots look alive between assignments.
        IdlePhase += DeltaTime * 0.8f;
        const FVector Base = GetActorLocation();
        SetActorLocation(Base + FVector(0.0f, 0.0f, FMath::Sin(IdlePhase) * 0.35f));
    }
}
