#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildInteractable.h"
#include "AstrawildResonancePillarActor.generated.h"

class UStaticMeshComponent;
class AAstrawildDungeonRoomActor;

/**
 * DP-9 (dungeon depth) — the resonance pillar: one of the three interactable
 * pillars a puzzle room spawns (see AAstrawildDungeonRoomActor). Each pillar
 * carries its required attunement ORDER (PillarIndex); the owning room runs
 * the sequence state machine — this actor is the interactable surface plus its
 * replicated lit state.
 *
 * Mirrors the AAstrawildDungeonPortalActor interactable pattern exactly:
 * server-authoritative, single-player/listen-server executes inline
 * (dedicated-client routing arrives with the H-9 multiplayer batch — the same
 * policy as PlayerController::OpenShop).
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildResonancePillarActor : public AActor, public IAstrawildInteractable
{
    GENERATED_BODY()

public:
    AAstrawildResonancePillarActor();

    /** Tall resonance stone (engine basic shape — REPLACE_BEFORE_RELEASE with the authored arch meshes). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Dungeon")
    TObjectPtr<UStaticMeshComponent> PillarMesh;

    /** Required attunement order within the puzzle sequence (0 = first). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Dungeon", meta=(ClampMin="0", ClampMax="8"))
    int32 PillarIndex = 0;

    /** Owning puzzle room (weak — set by the room after spawn). */
    TWeakObjectPtr<AAstrawildDungeonRoomActor> OwningRoom;

    /** Replicated lit state (attuned pillars glow; wrong order resets all). */
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Dungeon", ReplicatedUsing=OnRep_bActivated)
    bool bActivated = false;

    /** Server: set the lit state (replicates; clients re-apply the visual). */
    void SetActivated(bool bNewActivated);

    virtual void Interact_Implementation(AActor* InteractingActor) override;
    virtual FText GetInteractionPrompt_Implementation() const override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
    /** Placeholder tint through a dynamic material instance (ResourceNode idiom). */
    void ApplyActivationVisual();

    UFUNCTION()
    void OnRep_bActivated();
};
