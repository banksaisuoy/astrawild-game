#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildInteractable.h"
#include "AstrawildPOIMarkerActor.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class UAstrawildPOIDefinition;

/**
 * Production V2 (Master Plan §5/§31): world marker for a data-driven point of
 * interest. The bootstrapper spawns one per registered POI definition; the POI
 * subsystem tracks discovery (radius check + rewards). Placeholder visuals are
 * a tinted pillar + light (REPLACE_BEFORE_RELEASE — Antigravity binds the
 * DressingSetId prop set per definition).
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildPOIMarkerActor : public AActor, public IAstrawildInteractable
{
    GENERATED_BODY()

public:
    AAstrawildPOIMarkerActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|POI")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|POI")
    TObjectPtr<UPointLightComponent> BeaconLight;

    /** POI definition id this marker represents. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|POI")
    FName PoiId = NAME_None;

    /** Server: resolve visuals + light from the definition. */
    void InitializeFromDefinition(UAstrawildPOIDefinition* Definition);

    /** Server: dim the beacon after discovery (the mystery resolved). */
    void MarkDiscovered();

    /** IAstrawildInteractable: reading the marker re-prints its lore line. */
    virtual void Interact_Implementation(AActor* InteractingActor) override;
    virtual FText GetInteractionPrompt_Implementation() const override;

protected:
    virtual void BeginPlay() override;
};
