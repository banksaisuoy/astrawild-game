#include "AstrawildResourceNode.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildDurabilityComponent.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "TimerManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildResourceNode::AAstrawildResourceNode()
{
    PrimaryActorTick.bCanEverTick = false;
    SetReplicates(false);

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    RootComponent = VisualMesh;
    VisualMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

    // Shape-by-rarity kits (placeholder visuals — REPLACE_BEFORE_RELEASE with
    // per-node-type meshes bound through UAstrawildResourceNodeDefinition::MeshOverride).
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> ConeMesh(TEXT("/Engine/BasicShapes/Cone"));
    if (CubeMesh.Succeeded())
    {
        CommonShapeMesh = CubeMesh.Object;
        VisualMesh->SetStaticMesh(CubeMesh.Object);
    }
    if (SphereMesh.Succeeded())
    {
        UncommonShapeMesh = SphereMesh.Object;
    }
    if (CylinderMesh.Succeeded())
    {
        RareShapeMesh = CylinderMesh.Object;
    }
    if (ConeMesh.Succeeded())
    {
        EpicShapeMesh = ConeMesh.Object;
    }
    VisualMesh->SetWorldScale3D(FVector(0.65f, 0.65f, 0.8f));
}

void AAstrawildResourceNode::BeginPlay()
{
    Super::BeginPlay();

    // Production V2 (P0): definition-driven identity resolves FIRST; the legacy
    // direct-property path only logs. Deterministic identity means the node
    // either carries a valid item id or is explicitly disabled — the silent
    // no-op interact can no longer hide a bootstrap miss.
    ApplyNodeDefinition();

    // FR-4 (Final Run redo): node identity fallback. A map-placed node with an
    // unknown/missing NodeDefinitionId used to hard-disable itself — and when that
    // node was the FirstLight harvest target, the very first quest stalled forever
    // (defect D-1). Fall back to the Dawnwood stand (always registered by the
    // production content library) so the world stays harvestable; the Warning
    // still names the actor for the level author to fix.
    if (ResourceItemId.IsNone())
    {
        const UWorld* World = GetWorld();
        const UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
        if (const UAstrawildResourceNodeDefinition* Fallback = Registry ? Registry->FindResourceNode(TEXT("Node_Dawnwood")) : nullptr)
        {
            ResourceItemId = Fallback->ResourceItemId;
            ResourceQuantityPerHarvest = FMath::Max(1, Fallback->QuantityPerHarvest);
            RemainingQuantity = FMath::Max(1, Fallback->MaxQuantity);
            RespawnDurationSeconds = FMath::Max(0.0f, Fallback->RespawnDurationSeconds);
            UE_LOG(LogAstrawild, Warning,
                TEXT("Resource node %s had no identity (NodeDefinitionId=%s) — fell back to Node_Dawnwood (%s). Fix the spawner."),
                *GetName(), *NodeDefinitionId.ToString(), *ResourceItemId.ToString());
        }
        else
        {
            UE_LOG(LogAstrawild, Error,
                TEXT("Resource node %s has no identity and the Node_Dawnwood fallback is unregistered — disabling interaction."),
                *GetName());
            SetActorEnableCollision(false);
        }
    }
}

UAstrawildResourceNodeDefinition* AAstrawildResourceNode::GetNodeDefinition() const
{
    if (NodeDefinitionId.IsNone())
    {
        return nullptr;
    }
    const UWorld* World = GetWorld();
    const UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    return Registry ? Registry->FindResourceNode(NodeDefinitionId) : nullptr;
}

bool AAstrawildResourceNode::RequiresScannerDetection() const
{
    const UAstrawildResourceNodeDefinition* Def = GetNodeDefinition();
    return Def && Def->bRequiresScannerDetection;
}

void AAstrawildResourceNode::ApplyNodeDefinition()
{
    UAstrawildResourceNodeDefinition* Def = GetNodeDefinition();
    if (!Def)
    {
        if (!NodeDefinitionId.IsNone())
        {
            UE_LOG(LogAstrawild, Error,
                TEXT("Resource node %s references unknown node definition %s."), *GetName(), *NodeDefinitionId.ToString());
        }
        return;
    }

    // Deterministic identity: the definition is the single source of truth.
    ResourceItemId = Def->ResourceItemId;
    ResourceQuantityPerHarvest = FMath::Max(1, Def->QuantityPerHarvest);
    RemainingQuantity = FMath::Max(1, Def->MaxQuantity);
    // Final-audit M-7 (AUD-4): the respawn restores the DEFINED max, not the
    // per-harvest rate — Node_Dawnwood (3 max, 2/harvest) used to respawn at 2 forever.
    CachedMaxQuantity = FMath::Max(1, Def->MaxQuantity);
    RespawnDurationSeconds = FMath::Max(0.0f, Def->RespawnDurationSeconds);

    // Art pack (Batch 4, CP-04): real node mesh replaces the rarity shape when
    // the soft ref resolves (warmed by the registry art-pack pass). The rarity
    // shape + tint placeholder stays live otherwise — zero-asset rule.
    if (UStaticMesh* ArtMesh = Def->MeshOverride.LoadSynchronous())
    {
        VisualMesh->SetStaticMesh(ArtMesh);
        VisualMesh->SetWorldScale3D(FVector(FMath::Max(0.1f, Def->VisualScale)));
        UE_LOG(LogAstrawild, Verbose, TEXT("Resource node %s using art mesh %s."),
            *GetName(), *Def->MeshOverride.ToSoftObjectPath().ToString());
        return;
    }

    ApplyRarityShape(Def);
    VisualMesh->SetWorldScale3D(VisualMesh->GetRelativeScale3D() * FMath::Max(0.1f, Def->VisualScale));
    ApplyVisualTint(Def->NodeTint);
}

void AAstrawildResourceNode::ApplyRarityShape(const UAstrawildResourceNodeDefinition* Def)
{
    // Rarity is readable at a glance with zero assets: common → cube, uncommon →
    // sphere, rare → cylinder, epic/legendary/mythic → cone spire.
    UStaticMesh* ShapeMesh = CommonShapeMesh;
    FVector BaseScale(0.65f, 0.65f, 0.8f);
    switch (Def->Rarity)
    {
    case EAstrawildRarity::Uncommon:
        ShapeMesh = UncommonShapeMesh ? UncommonShapeMesh : CommonShapeMesh;
        BaseScale = FVector(0.55f, 0.55f, 0.7f);
        break;
    case EAstrawildRarity::Rare:
        ShapeMesh = RareShapeMesh ? RareShapeMesh : CommonShapeMesh;
        BaseScale = FVector(0.5f, 0.5f, 1.0f);
        break;
    case EAstrawildRarity::Epic:
    case EAstrawildRarity::Legendary:
    case EAstrawildRarity::Mythic:
        ShapeMesh = EpicShapeMesh ? EpicShapeMesh : CommonShapeMesh;
        BaseScale = FVector(0.45f, 0.45f, 1.3f);
        break;
    case EAstrawildRarity::Common:
    default:
        break;
    }
    if (ShapeMesh)
    {
        VisualMesh->SetStaticMesh(ShapeMesh);
    }
    VisualMesh->SetWorldScale3D(BaseScale);
}

void AAstrawildResourceNode::ApplyVisualTint(const FLinearColor& Tint)
{
    // Placeholder tint through a dynamic material instance — every node TYPE is
    // visually distinct with zero assets (REPLACE_BEFORE_RELEASE: real meshes).
    // The engine basic-shape mesh already carries a material at slot 0; if a
    // level-placed node has none we simply skip tinting (never load synchronously
    // outside the constructor).
    if (!VisualMesh || !VisualMesh->GetStaticMesh())
    {
        return;
    }
    UMaterialInterface* BaseMaterial = VisualMesh->GetMaterial(0);
    if (!BaseMaterial)
    {
        return;
    }
    UMaterialInstanceDynamic* DynMaterial = VisualMesh->CreateAndSetMaterialInstanceDynamic(0);
    if (DynMaterial)
    {
        DynMaterial->SetVectorParameterValue(TEXT("Color"), Tint);
    }
}

void AAstrawildResourceNode::Interact_Implementation(AActor* InteractingActor)
{
    if (!IsValid(InteractingActor) || ResourceItemId.IsNone() || RemainingQuantity <= 0)
    {
        return;
    }

    UAstrawildInventoryComponent* Inventory = InteractingActor->FindComponentByClass<UAstrawildInventoryComponent>();
    if (!IsValid(Inventory))
    {
        UE_LOG(LogAstrawild, Warning, TEXT("Actor %s tried to harvest %s without an inventory."), *InteractingActor->GetName(), *GetName());
        return;
    }

    // Production V2: hidden veins are harvestable only while a scanner with
    // hidden-resource detection is equipped (scanner progression value).
    if (RequiresScannerDetection())
    {
        if (!Inventory->HasHiddenResourceDetection())
        {
            UE_LOG(LogAstrawild, Verbose, TEXT("%s is a hidden vein — the harvester needs a scanner with resource detection."), *GetName());
            return;
        }
    }

    const int32 QuantityToGrant = bInfiniteResource
        ? ResourceQuantityPerHarvest
        : FMath::Min(ResourceQuantityPerHarvest, RemainingQuantity);

    // SCP Phase 12: specialized tools multiply the yield when their category
    // matches the resource (pick x3 ore, axe x3 wood, sickle x4 fiber — broken
    // tools harvest at base rate) and take one point of wear per swing.
    int32 MultipliedQuantity = QuantityToGrant;
    UAstrawildDurabilityComponent* Durability = InteractingActor->FindComponentByClass<UAstrawildDurabilityComponent>();
    if (Durability)
    {
        const float YieldMultiplier = Durability->GetHarvestYieldMultiplier(ResourceItemId);
        MultipliedQuantity = FMath::Max(1, FMath::RoundToInt(QuantityToGrant * YieldMultiplier));
        Durability->ApplyToolWear();
    }

    if (!Inventory->AddItem(ResourceItemId, MultipliedQuantity))
    {
        return;
    }

    if (bInfiniteResource)
    {
        return;
    }

    RemainingQuantity -= QuantityToGrant;
    if (RemainingQuantity <= 0)
    {
        SetActorHiddenInGame(true);
        SetActorEnableCollision(false);
        if (RespawnDurationSeconds > 0.0f)
        {
            GetWorldTimerManager().SetTimer(RespawnTimerHandle, this, &AAstrawildResourceNode::RespawnNode, RespawnDurationSeconds, false);
        }
    }
}

FText AAstrawildResourceNode::GetInteractionPrompt_Implementation() const
{
    return FText::Format(NSLOCTEXT("ASTRAWILD", "HarvestPrompt", "Harvest {0} [E]"), FText::FromName(ResourceItemId));
}

void AAstrawildResourceNode::RespawnNode()
{
    // Final-audit M-7 (AUD-4): restore the definition's MaxQuantity (cached at
    // ApplyNodeDefinition; falls back to the per-harvest rate for nodes without one).
    RemainingQuantity = FMath::Max(1, CachedMaxQuantity > 0 ? CachedMaxQuantity : ResourceQuantityPerHarvest);
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
}
