#include "AstrawildUtilityDroneActor.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildInteractable.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildJournalSubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildPlayerController.h"
#include "AstrawildResourceNode.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
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
    SetReplicatingMovement(true);
}

void AAstrawildUtilityDroneActor::BeginPlay()
{
    Super::BeginPlay();

    // Stagger the pulse phase slightly so several drones never sync-frame.
    ScanAccumulator = FMath::FRandRange(0.0f, ScanIntervalSeconds * 0.5f);
    HarvestAccumulator = FMath::FRandRange(0.0f, HarvestIntervalSeconds * 0.5f);
}

void AAstrawildUtilityDroneActor::InitializeForOwner(AAstrawildPlayerCharacter* InOwner)
{
    OwnerPlayer = InOwner;
    // Player identity follows the codebase convention (roster/capture): GetFName().
    OwnerPlayerId = InOwner ? InOwner->GetFName() : NAME_None;

    if (InOwner)
    {
        // Deploy at the owner's shoulder so the first frame looks right.
        const FVector HoverPoint = InOwner->GetActorLocation() + InOwner->GetActorRightVector() * FollowDistance + FVector(0.0f, 0.0f, HoverHeight);
        SetActorLocation(HoverPoint);
        // Production V2: fresh deploy = full battery (modules extend capacity).
        BatteryRemainingSeconds = GetEffectiveBatterySeconds();
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

    // Authority drives the hover/locomotion; clients mirror through replicated
    // movement (local cosmetic ticking would fight replication updates).
    if (GetLocalRole() != ROLE_Authority)
    {
        return;
    }

    if (AAstrawildPlayerCharacter* TargetPlayer = GetOwnerPlayer())
    {
        // Production V2: battery drains while deployed; at 0 the drone auto-recalls.
        if (BatteryRemainingSeconds > 0.0f)
        {
            BatteryRemainingSeconds = FMath::Max(0.0f, BatteryRemainingSeconds - DeltaTime);
        }
        if (BatteryRemainingSeconds <= 0.0f)
        {
            if (UWorld* World = GetWorld())
            {
                if (APlayerController* PC = World->GetFirstPlayerController())
                {
                    if (AAstrawildPlayerController* AstrawildPC = Cast<AAstrawildPlayerController>(PC))
                    {
                        AstrawildPC->Notify(FText::FromString(TEXT("Drone battery depleted — recalled for recharge.")));
                    }
                }
                // Recall = destroy + clear the player's active-drone handle (deploy
                // key redeploys free, mirroring the manual recall semantics).
                if (TargetPlayer && TargetPlayer->GetActiveDrone() == this)
                {
                    TargetPlayer->ClearActiveDrone();
                }
                World->DestroyActor(this);
                return;
            }
        }

        // Hover target: beside the owner, gently bobbing.
        BobPhase += DeltaTime * 2.2f;
        const FVector HoverPoint = TargetPlayer->GetActorLocation()
            + TargetPlayer->GetActorRightVector() * FollowDistance
            + FVector(0.0f, 0.0f, HoverHeight + BobAmplitude * FMath::Sin(BobPhase));
        const FVector Current = GetActorLocation();
        const FVector NewLocation = FMath::VInterpTo(Current, HoverPoint, DeltaTime, FollowInterpSpeed);
        SetActorLocation(NewLocation);

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

void AAstrawildUtilityDroneActor::RunScanPulse()
{
    UWorld* World = GetWorld();
    UAstrawildJournalSubsystem* Journal = World ? World->GetSubsystem<UAstrawildJournalSubsystem>() : nullptr;
    if (!World || !Journal)
    {
        return;
    }

    // Production V2: modules extend scan radius + rate (best per category).
    float ScanRadiusBonus = 0.0f, HarvestRadiusBonus = 0.0f, ScanRateBonus = 0.0f, BatteryBonus = 0.0f;
    if (const AAstrawildPlayerCharacter* PlayerChar = GetOwnerPlayer())
    {
        if (const UAstrawildInventoryComponent* Inventory = PlayerChar->FindComponentByClass<UAstrawildInventoryComponent>())
        {
            ResolveModules(Inventory, ScanRadiusBonus, HarvestRadiusBonus, ScanRateBonus, BatteryBonus);
        }
    }
    const float EffectiveScanRadius = ScanRadius + ScanRadiusBonus;
    const float EffectiveProgress = ScanProgressPerPulse + ScanRateBonus;

    const FVector DroneLocation = GetActorLocation();
    int32 Scanned = 0;
    for (TActorIterator<AAstrawildEchoCharacter> It(World); It; ++It)
    {
        AAstrawildEchoCharacter* Echo = *It;
        if (!Echo || !Echo->EchoDefinition || Echo->IsDefeated())
        {
            continue;
        }
        if (FVector::Dist(Echo->GetActorLocation(), DroneLocation) <= EffectiveScanRadius)
        {
            Journal->AddExternalObservation(Echo, EffectiveProgress);
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
    AAstrawildPlayerCharacter* TargetPlayer = GetOwnerPlayer();
    if (!World || !TargetPlayer)
    {
        return;
    }

    // Nearest harvestable node in range (module-extended radius).
    float ScanRadiusBonus = 0.0f, HarvestRadiusBonus = 0.0f, ScanRateBonus = 0.0f, BatteryBonus = 0.0f;
    if (const UAstrawildInventoryComponent* Inventory = Owner->FindComponentByClass<UAstrawildInventoryComponent>())
    {
        ResolveModules(Inventory, ScanRadiusBonus, HarvestRadiusBonus, ScanRateBonus, BatteryBonus);
    }
    const FVector DroneLocation = GetActorLocation();
    AAstrawildResourceNode* Best = nullptr;
    float BestDistance = HarvestRadius + HarvestRadiusBonus;
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

float AAstrawildUtilityDroneActor::GetEffectiveBatterySeconds() const
{
    // Base capacity + the best battery module in the owner's inventory.
    float ScanRadiusBonus = 0.0f, HarvestRadiusBonus = 0.0f, ScanRateBonus = 0.0f, BatteryBonus = 0.0f;
    if (const AAstrawildPlayerCharacter* PlayerChar = GetOwnerPlayer())
    {
        if (const UAstrawildInventoryComponent* Inventory = PlayerChar->FindComponentByClass<UAstrawildInventoryComponent>())
        {
            ResolveModules(Inventory, ScanRadiusBonus, HarvestRadiusBonus, ScanRateBonus, BatteryBonus);
        }
    }
    return FMath::Max(60.0f, BaseBatterySeconds + BatteryBonus);
}

float AAstrawildUtilityDroneActor::GetBatteryFraction() const
{
    const float Capacity = GetEffectiveBatterySeconds();
    return Capacity > 0.0f ? FMath::Clamp(BatteryRemainingSeconds / Capacity, 0.0f, 1.0f) : 0.0f;
}

void AAstrawildUtilityDroneActor::ResolveModules(const UAstrawildInventoryComponent* Inventory,
    float& OutScanRadiusBonus, float& OutHarvestRadiusBonus, float& OutScanRateBonus, float& OutBatteryBonus)
{
    // Best module per category (not additive across duplicates) — deterministic
    // and readable for players: one slot per upgrade type, strongest wins.
    OutScanRadiusBonus = 0.0f;
    OutHarvestRadiusBonus = 0.0f;
    OutScanRateBonus = 0.0f;
    OutBatteryBonus = 0.0f;
    if (!Inventory)
    {
        return;
    }
    const UWorld* World = Inventory->GetWorld();
    const UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    if (!Registry)
    {
        return;
    }
    for (const FAstrawildItemStack& Stack : Inventory->GetItemStacks())
    {
        if (const UAstrawildItemDefinition* ItemDef = Registry->FindItem(Stack.ItemId))
        {
            OutScanRadiusBonus = FMath::Max(OutScanRadiusBonus, ItemDef->DroneScanRadiusBonus);
            OutHarvestRadiusBonus = FMath::Max(OutHarvestRadiusBonus, ItemDef->DroneHarvestRadiusBonus);
            OutScanRateBonus = FMath::Max(OutScanRateBonus, ItemDef->DroneScanRateBonus);
            OutBatteryBonus = FMath::Max(OutBatteryBonus, ItemDef->DroneBatteryBonusSeconds);
        }
    }
}
