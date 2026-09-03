#include "AstrawildBuildingComponent.h"

#include "AstrawildBuildingActor.h"
#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildResearchSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

UAstrawildBuildingComponent::UAstrawildBuildingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

void UAstrawildBuildingComponent::BeginPlay()
{
    Super::BeginPlay();
}

AAstrawildPlayerCharacter* UAstrawildBuildingComponent::GetPlayer() const
{
    return Cast<AAstrawildPlayerCharacter>(GetOwner());
}

UAstrawildItemRegistrySubsystem* UAstrawildBuildingComponent::GetRegistry() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
}

UAstrawildResearchSubsystem* UAstrawildBuildingComponent::GetResearch() const
{
    const UWorld* World = GetWorld();
    return World && World->GetGameInstance() ? World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>() : nullptr;
}

void UAstrawildBuildingComponent::RebuildUnlockedList()
{
    CachedUnlockedIds.Reset();
    if (const UAstrawildItemRegistrySubsystem* Registry = GetRegistry())
    {
        for (UAstrawildBuildingDefinition* Def : Registry->GetUnlockedBuildings(NAME_None))
        {
            if (Def)
            {
                CachedUnlockedIds.Add(Def->DefinitionId);
            }
        }
    }
}

void UAstrawildBuildingComponent::TogglePlacementMode()
{
    bPlacementMode = !bPlacementMode;

    if (bPlacementMode)
    {
        RebuildUnlockedList();
        if (CachedUnlockedIds.IsEmpty())
        {
            UE_LOG(LogAstrawildBuilding, Warning, TEXT("No unlocked building definitions — placement mode refused."));
            bPlacementMode = false;
            return;
        }

        // Spawn the preview ghost (non-replicated, owner-only visual).
        UWorld* World = GetWorld();
        if (World)
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            PreviewActor = World->SpawnActor<AAstrawildBuildingActor>(AAstrawildBuildingActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
            if (PreviewActor)
            {
                PreviewActor->SetActorEnableCollision(false);
                if (UPrimitiveComponent* Mesh = PreviewActor->VisualMesh)
                {
                    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                    Mesh->SetRenderCustomDepth(true); // Silhouette-style ghost via custom depth.
                }
            }
        }
    }
    else
    {
        CancelPlacement();
    }

    OnPlacementModeChanged.Broadcast(bPlacementMode);
}

void UAstrawildBuildingComponent::CycleBuildingDefinition(const int32 Direction)
{
    if (!bPlacementMode || CachedUnlockedIds.Num() <= 1)
    {
        return;
    }
    // Audit C-6: signed direction (mouse wheel up/down) with wrap-around.
    const int32 Count = CachedUnlockedIds.Num();
    CurrentDefinitionIndex = ((CurrentDefinitionIndex % Count) + Count + (Direction >= 0 ? 1 : -1)) % Count;
}

FText UAstrawildBuildingComponent::GetCurrentDefinitionDisplayName() const
{
    const UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    const UAstrawildBuildingDefinition* Def = Registry ? Registry->FindBuilding(GetCurrentDefinitionId()) : nullptr;
    return Def ? Def->DisplayName : FText::FromString(GetCurrentDefinitionId().ToString());
}

void UAstrawildBuildingComponent::GetPlacementPieceInfo(int32& OutPieceIndex, int32& OutPieceCount) const
{
    OutPieceCount = CachedUnlockedIds.Num();
    OutPieceIndex = CachedUnlockedIds.IsValidIndex(CurrentDefinitionIndex) ? CurrentDefinitionIndex + 1 : 0;
}

void UAstrawildBuildingComponent::RotatePreview(const float Degrees)
{
    if (bPlacementMode)
    {
        PreviewYaw = FMath::Fmod(PreviewYaw + Degrees, 360.0f);
    }
}

FName UAstrawildBuildingComponent::GetCurrentDefinitionId() const
{
    if (!bPlacementMode || !CachedUnlockedIds.IsValidIndex(CurrentDefinitionIndex))
    {
        return NAME_None;
    }
    return CachedUnlockedIds[CurrentDefinitionIndex];
}

FVector UAstrawildBuildingComponent::ComputeSnappedLocation() const
{
    const AAstrawildPlayerCharacter* Player = GetPlayer();
    if (!Player)
    {
        return FVector::ZeroVector;
    }

    // Project forward from the player and snap to grid.
    const FVector Start = Player->GetActorLocation();
    const FVector Forward = Player->GetActorForwardVector().GetSafeNormal2D();
    const FVector Target = Start + Forward * PlacementReach;

    FVector Snapped(
        FMath::GridSnap(Target.X, SnapGridSize),
        FMath::GridSnap(Target.Y, SnapGridSize),
        Target.Z);

    // Ground-align via trace.
    if (UWorld* World = GetWorld())
    {
        FHitResult Hit;
        if (World->LineTraceSingleByChannel(Hit, Snapped + FVector(0, 0, 500), Snapped - FVector(0, 0, 500), ECC_Visibility))
        {
            Snapped = Hit.Location;
        }
    }
    return Snapped;
}

bool UAstrawildBuildingComponent::ValidatePlacementLocation(const FVector& Location, const float GridSize) const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    // Overlap check against existing blocking geometry.
    FCollisionShape Shape = FCollisionShape::MakeBox(FVector(GridSize * 0.45f, GridSize * 0.45f, 50.0f));
    TArray<FOverlapResult> Overlaps;
    const bool bBlocked = World->OverlapMultiByChannel(
        Overlaps, Location + FVector(0, 0, 50), FQuat::Identity, ECC_WorldStatic, Shape,
        FCollisionQueryParams::DefaultQueryParam, FCollisionResponseParams::DefaultResponseParam);

    return !bBlocked;
}

void UAstrawildBuildingComponent::UpdatePreview()
{
    if (!bPlacementMode || !PreviewActor)
    {
        return;
    }

    const UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    const FName DefId = GetCurrentDefinitionId();
    const UAstrawildBuildingDefinition* Def = Registry ? Registry->FindBuilding(DefId) : nullptr;

    const FVector Location = ComputeSnappedLocation();
    PreviewActor->SetActorLocation(Location);
    PreviewActor->SetActorRotation(FRotator(0.0f, PreviewYaw, 0.0f));

    if (Def && PreviewActor->VisualMesh)
    {
        switch (Def->Category)
        {
        case EAstrawildBuildingCategory::Foundation:
            PreviewActor->VisualMesh->SetWorldScale3D(FVector(2.0f, 2.0f, 0.2f));
            break;
        case EAstrawildBuildingCategory::Wall:
            PreviewActor->VisualMesh->SetWorldScale3D(FVector(2.0f, 0.2f, 1.5f));
            break;
        // Final Run (FR-9): preview silhouettes mirror the placed pieces.
        case EAstrawildBuildingCategory::Floor:
            PreviewActor->VisualMesh->SetWorldScale3D(FVector(2.0f, 2.0f, 0.12f));
            break;
        case EAstrawildBuildingCategory::Roof:
            PreviewActor->VisualMesh->SetWorldScale3D(FVector(2.2f, 2.2f, 0.18f));
            break;
        case EAstrawildBuildingCategory::Door:
            PreviewActor->VisualMesh->SetWorldScale3D(FVector(1.5f, 0.15f, 1.6f));
            break;
        case EAstrawildBuildingCategory::Storage:
            PreviewActor->VisualMesh->SetWorldScale3D(FVector(1.1f, 1.1f, 0.9f));
            break;
        default:
            PreviewActor->VisualMesh->SetWorldScale3D(FVector(1.2f, 1.2f, 1.0f));
            break;
        }
    }

    // Validity: inventory has materials + location is clear.
    const AAstrawildPlayerCharacter* Player = GetPlayer();
    const UAstrawildInventoryComponent* Inventory = Player ? Player->InventoryComponent : nullptr;
    const bool bHasMaterials = Def && Inventory && Inventory->HasItem(Def->RequiredItemId, Def->RequiredItemCount);
    bPlacementValid = bHasMaterials && ValidatePlacementLocation(Location, SnapGridSize);

    // Final Run (FR-9): validity tint — the preview's indicator light reads
    // GREEN when the placement resolves (materials + clear ground) and RED
    // when it does not. Zero-asset-safe: the basic-shape mesh catches the
    // colored light, so validity reads at a glance while rotating.
    if (PreviewActor && PreviewActor->PowerIndicatorLight)
    {
        PreviewActor->PowerIndicatorLight->SetIntensity(bPlacementValid ? 6.0f : 10.0f);
        PreviewActor->PowerIndicatorLight->SetAttenuationRadius(1100.0f);
        PreviewActor->PowerIndicatorLight->SetLightColor(bPlacementValid
            ? FLinearColor(0.15f, 0.95f, 0.35f, 1.0f)
            : FLinearColor(0.95f, 0.20f, 0.12f, 1.0f));
    }
}

void UAstrawildBuildingComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bPlacementMode)
    {
        UpdatePreview();
    }
}

void UAstrawildBuildingComponent::ConfirmPlacement()
{
    if (!bPlacementMode || !bPlacementValid)
    {
        return;
    }

    const FName DefId = GetCurrentDefinitionId();
    const FVector Location = PreviewActor ? PreviewActor->GetActorLocation() : ComputeSnappedLocation();

    // Consume materials locally then request authoritative placement.
    AAstrawildPlayerCharacter* Player = GetPlayer();
    UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    const UAstrawildBuildingDefinition* Def = Registry ? Registry->FindBuilding(DefId) : nullptr;
    UAstrawildInventoryComponent* Inventory = Player ? Player->InventoryComponent : nullptr;

    if (!Def || !Inventory || !Inventory->ConsumeItems({ FAstrawildItemStack{Def->RequiredItemId, Def->RequiredItemCount} }))
    {
        return;
    }

    ServerPlaceBuilding(DefId, Location, PreviewYaw);
}

void UAstrawildBuildingComponent::ServerPlaceBuilding_Implementation(const FName DefinitionId, const FVector_NetQuantize Location, const float Yaw)
{
    UWorld* World = GetWorld();
    UAstrawildItemRegistrySubsystem* Registry = GetRegistry();
    const UAstrawildBuildingDefinition* Def = Registry ? Registry->FindBuilding(DefinitionId) : nullptr;
    AAstrawildPlayerCharacter* Player = GetPlayer();

    if (!World || !Def || !Player)
    {
        return;
    }

    // Server re-validates (never trust the client, directive §28).
    if (!ValidatePlacementLocation(Location, Def->GridCellSize))
    {
        // Refund the consumed materials.
        // Final-audit F-04: AddItemSilent — the dismantle and save-restore refunds
        // were already silent; this placement-rejection path still fired
        // ItemCollected, falsely advancing a live "collect Item_Wood" objective
        // (MASTER_CONTROL §9 refund rule).
        if (Player->InventoryComponent)
        {
            Player->InventoryComponent->AddItemSilent(Def->RequiredItemId, Def->RequiredItemCount);
        }
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    AAstrawildBuildingActor* Building = World->SpawnActor<AAstrawildBuildingActor>(
        AAstrawildBuildingActor::StaticClass(), Location, FRotator(0.0f, Yaw, 0.0f), Params);

    if (Building && Building->InitializeFromDefinition(Def, Player->GetFName()))
    {
        OnBuildingPlaced.Broadcast(DefinitionId, Building);
        if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
        {
            EventBus->PublishEvent(TAG_Astrawild_Event_BuildingPlaced, Player, DefinitionId, 1, Location);
        }
        UE_LOG(LogAstrawildBuilding, Log, TEXT("Building placed: %s at %s."), *DefinitionId.ToString(), *Location.ToCompactString());
    }
}

void UAstrawildBuildingComponent::CancelPlacement()
{
    if (PreviewActor)
    {
        PreviewActor->Destroy();
        PreviewActor = nullptr;
    }
    bPlacementMode = false;
    OnPlacementModeChanged.Broadcast(false);
}

bool UAstrawildBuildingComponent::DismantleBuilding(AActor* TargetBuilding)
{
    // Authority gate — placement is already server-authoritative, dismantle follows the same rule.
    if (GetOwnerRole() != ROLE_Authority)
    {
        return false;
    }

    AAstrawildBuildingActor* Building = Cast<AAstrawildBuildingActor>(TargetBuilding);
    if (!Building)
    {
        return false;
    }

    // Don't dismantle the live preview ghost — that's already handled by CancelPlacement.
    if (Building == PreviewActor)
    {
        return false;
    }

    const UAstrawildBuildingDefinition* Def = Building->GetBuildingDefinition();
    AAstrawildPlayerCharacter* Player = GetPlayer();
    if (!Def || !Player || !Player->InventoryComponent)
    {
        return false;
    }

    // Weight-safe refund: try to add materials FIRST. If the bag is full, refuse the
    // dismantle so the player never loses materials into the void.
    if (!Player->InventoryComponent->CanAddItem(Def->RequiredItemId, Def->RequiredItemCount))
    {
        UE_LOG(LogAstrawildBuilding, Log,
            TEXT("Dismantle refused: bag cannot accept %d x %s."),
            Def->RequiredItemCount, *Def->RequiredItemId.ToString());
        return false;
    }

    // Silent refund (AddItemSilent) — does NOT publish TAG_Astrawild_Event_ItemCollected
    // so dismantling a Foundation does not falsely advance any "gather Item_Wood" quest.
    Player->InventoryComponent->AddItemSilent(Def->RequiredItemId, Def->RequiredItemCount);

    UWorld* World = GetWorld();
    if (World)
    {
        if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
        {
            EventBus->PublishEvent(TAG_Astrawild_Event_BuildingPlaced, Player,
                Def->DefinitionId, -1, Building->GetActorLocation());
        }
    }

    UE_LOG(LogAstrawildBuilding, Log,
        TEXT("Dismantled building %s — refunded %d x %s."),
        *Def->DefinitionId.ToString(), Def->RequiredItemCount, *Def->RequiredItemId.ToString());

    Building->Destroy();
    return true;
}
