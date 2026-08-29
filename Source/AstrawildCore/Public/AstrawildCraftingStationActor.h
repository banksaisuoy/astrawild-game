#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildInteractable.h"
#include "AstrawildCraftingStationActor.generated.h"

class UStaticMeshComponent;

/**
 * Crafting station (directive §15): campfire, workbench, forge...
 * Interacting crafts the first currently-craftable recipe that requires this station —
 * a deliberate no-UI vertical-slice behavior. The real UMG crafting screen calls the
 * same CraftingComponent API (Docs/ASTRAWILD_CRAFTING_SYSTEM.md).
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildCraftingStationActor : public AActor, public IAstrawildInteractable
{
    GENERATED_BODY()

public:
    AAstrawildCraftingStationActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Crafting")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    /** Registry id of this station kind (e.g. Station_Workbench, Station_Campfire). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Crafting")
    FName StationId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Crafting", meta=(ClampMin="100.0"))
    float UseRadius = 500.0f;

    virtual void Interact_Implementation(AActor* InteractingActor) override;
    virtual FText GetInteractionPrompt_Implementation() const override;

protected:
    virtual void BeginPlay() override;
};
