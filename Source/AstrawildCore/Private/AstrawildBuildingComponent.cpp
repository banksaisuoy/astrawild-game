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
    TArray<FHitResult> Hits;
    const bool bBlocked = World->OverlapMultiByChannel(
        Hits, Location + FVector(0, 0, 50), FQuat::Identity, ECC_WorldStatic, Shape,
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
        if (Player->InventoryComponent)
        {
            Player->InventoryComponent->AddItem(Def->RequiredItemId, Def->RequiredItemCount);
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
