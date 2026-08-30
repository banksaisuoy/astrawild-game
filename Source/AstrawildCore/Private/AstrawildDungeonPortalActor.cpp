#include "AstrawildDungeonPortalActor.h"

#include "AstrawildCore.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildDungeonPortalActor::AAstrawildDungeonPortalActor()
{
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;

    // Flat resonance pad on the ground (cylinder squashed — REPLACE_BEFORE_RELEASE).
    PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
    RootComponent = PortalMesh;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderMesh.Succeeded())
    {
        PortalMesh->SetStaticMesh(CylinderMesh.Object);
        PortalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        PortalMesh->SetRelativeScale3D(FVector(2.4f, 2.4f, 0.12f));
    }
}

FText AAstrawildDungeonPortalActor::GetInteractionPrompt_Implementation() const
{
    return PromptText.IsEmpty() ? FText::FromString(TEXT("Use portal [E]")) : PromptText;
}

void AAstrawildDungeonPortalActor::Interact_Implementation(AActor* InteractingActor)
{
    AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(InteractingActor);
    if (!Player)
    {
        return;
    }

    if (GetLocalRole() != ROLE_Authority)
    {
        // Dedicated-client routing arrives with the H-9 multiplayer batch (same
        // policy as PlayerController::OpenShop — single-player/listen-server inline).
        return;
    }

    TeleportPlayer(Player);
}

void AAstrawildDungeonPortalActor::TeleportPlayer(AAstrawildPlayerCharacter* Player)
{
    if (!Player)
    {
        return;
    }

    // Server-side range guard: the interaction trace allows ~300cm, the pad is
    // generous with 600cm, but a teleport from across the arena is rejected.
    if (FVector::Dist(GetActorLocation(), Player->GetActorLocation()) > UseRadius)
    {
        return;
    }

    // Batch 8 — publish-only survey markers never move the player.
    if (bPublishOnly)
    {
        if (UWorld* World = GetWorld())
        {
            if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
            {
                EventBus->PublishEvent(TAG_Astrawild_Event_LocationReached, Player, PortalId, 1, GetActorLocation());
            }
        }
        UE_LOG(LogAstrawildAI, Log, TEXT("Survey marker %s charted."), *PortalId.ToString());
        return;
    }

    if (Destination.IsNearlyZero())
    {
        return;
    }

    Player->SetActorLocation(Destination, false, nullptr, ETeleportType::TeleportPhysics);

    // First publisher for the ReachLocation objective type (directive §25 — the
    // event existed, the tag existed, nothing ever published it).
    if (UWorld* World = GetWorld())
    {
        if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
        {
            EventBus->PublishEvent(TAG_Astrawild_Event_LocationReached, Player, PortalId, 1, GetActorLocation());
        }
    }

    UE_LOG(LogAstrawildAI, Log, TEXT("Portal %s teleported the player."), *PortalId.ToString());
}
