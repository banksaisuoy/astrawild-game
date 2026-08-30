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

    /** Stable location id published with Event.LocationReached (quest TargetId). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    FName PortalId = NAME_None;

    /** World-space teleport destination. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    FVector Destination = FVector::ZeroVector;

    /** Interaction prompt shown by the HUD. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon")
    FText PromptText;

    /** Max distance the player may stand from the portal to use it (server guard). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon", meta=(ClampMin="100.0"))
    float UseRadius = 600.0f;

    virtual void Interact_Implementation(AActor* InteractingActor) override;
    virtual FText GetInteractionPrompt_Implementation() const override;

private:
    /** Authority-side teleport + event publish (range-guarded). */
    void TeleportPlayer(class AAstrawildPlayerCharacter* Player);
};
