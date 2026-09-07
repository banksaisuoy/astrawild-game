#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildInteractable.h"
#include "AstrawildDungeonPortalActor.generated.h"

class UStaticMeshComponent;

/**
 * Dungeon portal (Batch 6 — Item C): an interactable resonance pad that ferries
 * the player between the Dawn Fields camp edge and a dungeon entrance. Publishing
 * Event.LocationReached with its PortalId also gives the previously publisher-less
 * ReachLocation quest objective type its first producer.
 *
 * Server-authoritative teleport: single-player/listen-server executes inline
 * (dedicated-client routing arrives with the H-9 multiplayer batch — the same
 * policy as PlayerController::OpenShop).
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildDungeonPortalActor : public AActor, public IAstrawildInteractable
{
    GENERATED_BODY()

public:
    AAstrawildDungeonPortalActor();

    /** Flat resonance pad (engine basic shape — REPLACE_BEFORE_RELEASE). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Dungeon")
    TObjectPtr<UStaticMeshComponent> PortalMesh;

    /** Stable location id published with Event.LocationReached (quest TargetId). LCP-2: replicated for client prompts. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon", Replicated)
    FName PortalId = NAME_None;

    /** World-space teleport destination. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    FVector Destination = FVector::ZeroVector;

    /** Interaction prompt shown by the HUD. LCP-2: replicated for client prompts. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon", Replicated)
    FText PromptText;

    /** Max distance the player may stand from the portal to use it (server guard). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon", meta=(ClampMin="100.0"))
    float UseRadius = 600.0f;

    /**
     * Batch 8 — publish-only markers: interacting fires Event.LocationReached but
     * never teleports (survey beacons for the ReachLocation objective, e.g. the
     * Driftwood Landing discovery marker for "Wings over the Vale").
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    bool bPublishOnly = false;

    virtual void Interact_Implementation(AActor* InteractingActor) override;
    virtual FText GetInteractionPrompt_Implementation() const override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override; // LCP-2

private:
    /** Authority-side teleport + event publish (range-guarded). */
    void TeleportPlayer(class AAstrawildPlayerCharacter* Player);
};
