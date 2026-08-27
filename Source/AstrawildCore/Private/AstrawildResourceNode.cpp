#include "AstrawildResourceNode.h"

#include "AstrawildCore.h"
#include "AstrawildInventoryComponent.h"
#include "TimerManager.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildResourceNode::AAstrawildResourceNode()
{
    PrimaryActorTick.bCanEverTick = false;
    SetReplicates(false);

    VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
    RootComponent = VisualMesh;
    VisualMesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        VisualMesh->SetStaticMesh(CubeMesh.Object);
        VisualMesh->SetWorldScale3D(FVector(0.65f, 0.65f, 0.8f));
    }
}

void AAstrawildResourceNode::BeginPlay()
{
    Super::BeginPlay();

    if (ResourceItemId.IsNone())
    {
        UE_LOG(LogAstrawild, Warning, TEXT("Resource node %s has no ResourceItemId."), *GetName());
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

    const int32 QuantityToGrant = bInfiniteResource
        ? ResourceQuantityPerHarvest
        : FMath::Min(ResourceQuantityPerHarvest, RemainingQuantity);

    if (!Inventory->AddItem(ResourceItemId, QuantityToGrant))
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
    return FText::Format(NSLOCTEXT("ASTRAWILD", "HarvestPrompt", "เก็บ {0}"), FText::FromName(ResourceItemId));
}

void AAstrawildResourceNode::RespawnNode()
{
    RemainingQuantity = FMath::Max(1, ResourceQuantityPerHarvest);
    SetActorHiddenInGame(false);
    SetActorEnableCollision(true);
}
