#include "AstrawildUtilityRobotActor.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildWorkSiteActor.h"
#include "Components/PointLightComponent.h"
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

    // Production V2: role light — chassis type readable at a glance (specialist
    // frames carry their definition tint).
    StatusLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("StatusLight"));
    StatusLight->SetupAttachment(RootComponent);
    StatusLight->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
    StatusLight->SetIntensity(4000.0f);
    StatusLight->SetAttenuationRadius(700.0f);
    StatusLight->SetLightColor(FLinearColor(0.62f, 0.62f, 0.66f));

    bReplicates = true;
    SetReplicatingMovement(true);
}

void AAstrawildUtilityRobotActor::BeginPlay()
{
    Super::BeginPlay();

    IdlePhase = FMath::FRandRange(0.0f, PI * 2.0f);
}

UAstrawildRobotDefinition* AAstrawildUtilityRobotActor::GetRobotDefinition() const
{
    if (RobotDefinitionId.IsNone())
    {
        return nullptr;
    }
    const UWorld* World = GetWorld();
    const UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    return Registry ? Registry->FindRobot(RobotDefinitionId) : nullptr;
}

void AAstrawildUtilityRobotActor::InitializeFromDefinition(UAstrawildRobotDefinition* Definition)
{
    // Production V2 (Master Plan §12): chassis data drives specialization rate,
    // locomotion and the role light — the actor stays one class.
    if (!IsValid(Definition))
    {
        return;
    }
    RobotDefinitionId = Definition->RobotId;
    MoveInterpSpeed = FMath::Clamp(MoveInterpSpeed * FMath::Clamp(Definition->MoveSpeedMultiplier, 0.1f, 4.0f), 0.1f, 8.0f);
    if (StatusLight)
    {
        StatusLight->SetLightColor(Definition->PrimaryTint);
    }
    UE_LOG(LogAstrawildAI, Log, TEXT("Robot chassis initialized: %s (specialist %s at %.2fx)."),
        *Definition->RobotId.ToString(), *UEnum::GetDisplayValueAsText(Definition->PrimaryWorkType).ToString(), Definition->SpecialistWorkRate);
}

float AAstrawildUtilityRobotActor::GetWorkRateFor(const EAstrawildWorkType SiteWorkType) const
{
    if (const UAstrawildRobotDefinition* Def = GetRobotDefinition())
    {
        return Def->PrimaryWorkType == SiteWorkType
            ? FMath::Clamp(Def->SpecialistWorkRate, 0.0f, 4.0f)
            : FMath::Clamp(Def->GenericWorkRate, 0.0f, 4.0f);
    }
    return GenericRobotWorkRate; // General-purpose frame (legacy utility robot).
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

    // Authority drives locomotion; clients mirror through replicated movement.
    if (GetLocalRole() != ROLE_Authority)
    {
        return;
    }

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
