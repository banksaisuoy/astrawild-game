#include "AstrawildUtilityDroneActor.h"

#include "AstrawildCore.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildInteractable.h"
#include "AstrawildJournalSubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildResourceNode.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildUtilityDroneActor::AAstrawildUtilityDroneActor()
{
    PrimaryActorTick.bCanEverTick = true;

    // Placeholder body (zero-asset playability — REPLACE_BEFORE_RELEASE with the
    // drone mesh/Niagara hover effect once art exists).
    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RootComponent = VisualMesh;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
    if (SphereMesh.Succeeded())
    {
        VisualMesh->SetStaticMesh(SphereMesh.Object);
    }
    VisualMesh->SetWorldScale3D(FVector(0.45f));

    // Drones are replicated so remote clients see the companion hovering.
    bReplicates = true;
}

void AAstrawildUtilityDroneActor::BeginPlay()
{
    Super::BeginPlay();

    // Stagger the pulse phase slightly so several drones never sync-frame.
    ScanAccumulator = FMath::FRandRange(0.0f, ScanIntervalSeconds * 0.5f);
    HarvestAccumulator = FMath::FRandRange(0.0f, HarvestIntervalSeconds * 0.5f);
}

void AAstrawildUtilityDroneActor::InitializeForOwner(AAstrawildPlayerCharacter* Owner)
{
    OwnerPlayer = Owner;
    // Player identity follows the codebase convention (roster/capture): GetFName().
    OwnerPlayerId = Owner ? Owner->GetFName() : NAME_None;

    if (Owner)
    {
        // Deploy at the owner's shoulder so the first frame looks right.
        const FVector HoverPoint = Owner->GetActorLocation() + Owner->GetActorRightVector() * FollowDistance + FVector(0.0f, 0.0f, HoverHeight);
        SetActorLocation(HoverPoint);
    }

    UE_LOG(LogAstrawildAI, Log, TEXT("Utility drone deployed for %s."), *OwnerPlayerId.ToString());
}

AAstrawildPlayerCharacter* AAstrawildUtilityDroneActor::GetOwnerPlayer() const
{
    // Prefer the cached actor; re-link from the id after a save/load cycle.
    if (AAstrawildPlayerCharacter* Cached = OwnerPlayer.Get())
    {
        return Cached;
    }

    UWorld* World = GetWorld();
    if (!World || OwnerPlayerId.IsNone())
    {
        return nullptr;
    }

    for (TActorIterator<AAstrawildPlayerCharacter> It(World); It; ++It)
    {
        AAstrawildPlayerCharacter* Player = *It;
        if (Player && Player->GetFName() == OwnerPlayerId)
        {
            // Note: const-cast needed only because the helper is const; the
            // mutation is a cache refresh, not gameplay state.
            const_cast<AAstrawildUtilityDroneActor*>(this)->OwnerPlayer = Player;
            return Player;
        }
    }
    return nullptr;
}

void AAstrawildUtilityDroneActor::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Cosmetics run everywhere; gameplay pulses only on the server.
    if (AAstrawildPlayerCharacter* Owner = GetOwnerPlayer())
    {
        // Hover target: beside the owner, gently bobbing.
        BobPhase += DeltaTime * 2.2f;
        const FVector HoverPoint = Owner->GetActorLocation()
            + Owner->GetActorRightVector() * FollowDistance
            + FVector(0.0f, 0.0f, HoverHeight + BobAmplitude * FMath::Sin(BobPhase));
        const FVector Current = GetActorLocation();
        const FVector NewLocation = FMath::VInterpTo(Current, HoverPoint, DeltaTime, FollowInterpSpeed);
        SetActorLocation(NewLocation);

        if (GetLocalRole() == ROLE_Authority)
        {
            ScanAccumulator += DeltaTime;
            if (ScanAccumulator >= ScanIntervalSeconds)
            {
                ScanAccumulator = 0.0f;
                RunScanPulse();
            }

            HarvestAccumulator += DeltaTime;
            if (HarvestAccumulator >= HarvestIntervalSeconds)
            {
                HarvestAccumulator = 0.0f;
                RunHarvestPulse();
            }
        }
    }
}

void AAstrawildUtilityDroneActor::RunScanPulse()
{
    UWorld* World = GetWorld();
    UAstrawildJournalSubsystem* Journal = World ? World->GetSubsystem<UAstrawildJournalSubsystem>() : nullptr;
    if (!World || !Journal)
    {
        return;
    }

    const FVector DroneLocation = GetActorLocation();
    int32 Scanned = 0;
    for (TActorIterator<AAstrawildEchoCharacter> It(World); It; ++It)
    {
        AAstrawildEchoCharacter* Echo = *It;
        if (!Echo || !IsValid(Echo->EchoDefinition) || Echo->IsDefeated())
        {
            continue;
        }
        if (FVector::Dist(Echo->GetActorLocation(), DroneLocation) <= ScanRadius)
        {
            Journal->AddExternalObservation(Echo, ScanProgressPerPulse);
            Scanned++;
        }
    }

    if (Scanned > 0)
    {
        UE_LOG(LogAstrawildAI, Verbose, TEXT("Drone scan pulse: %d creatures in range."), Scanned);
    }
}

void AAstrawildUtilityDroneActor::RunHarvestPulse()
{
    UWorld* World = GetWorld();
    AAstrawildPlayerCharacter* Owner = GetOwnerPlayer();
    if (!World || !Owner)
    {
        return;
    }

    // Nearest harvestable node in range.
    const FVector DroneLocation = GetActorLocation();
    AAstrawildResourceNode* Best = nullptr;
    float BestDistance = HarvestRadius;
    for (TActorIterator<AAstrawildResourceNode> It(World); It; ++It)
    {
        AAstrawildResourceNode* Node = *It;
        if (!Node || Node->RemainingQuantity <= 0)
        {
            continue;
        }
        const float Distance = FVector::Dist(Node->GetActorLocation(), DroneLocation);
        if (Distance < BestDistance)
        {
            BestDistance = Distance;
            Best = Node;
        }
    }

    if (Best)
    {
        // Route through the standard interaction so loot lands in the owner's
        // inventory, weight gates apply and ItemCollected quests progress.
        IAstrawildInteractable::Execute_Interact(Best, Owner);
        UE_LOG(LogAstrawildEconomy, Verbose, TEXT("Drone harvested a node for %s."), *OwnerPlayerId.ToString());
    }
}
