#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AstrawildInteractable.h"
#include "AstrawildNPCCharacter.generated.h"

class UAstrawildNPCDefinition;
class UStaticMeshComponent;

/**
 * NPC base (directive §26): interaction offers quests from its NPC definition.
 * Schedule/dialogue/faction architecture-ready; conversation data assets arrive
 * with the content pass (see Docs/ASTRAWILD_QUEST_SYSTEM.md).
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildNPCCharacter : public ACharacter, public IAstrawildInteractable
{
    GENERATED_BODY()

public:
    AAstrawildNPCCharacter();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|NPC")
    TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="ASTRAWILD|NPC")
    TObjectPtr<UAstrawildNPCDefinition> NpcDefinition;

    virtual void Interact_Implementation(AActor* InteractingActor) override;
    virtual FText GetInteractionPrompt_Implementation() const override;

protected:
    virtual void BeginPlay() override;
};
