#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildInteractable.h"
#include "AstrawildResourceNode.generated.h"

class UStaticMeshComponent;

UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildResourceNode : public AActor, public IAstrawildInteractable
{
    GENERATED_BODY()

public:
    AAstrawildResourceNode();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Resource")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Resource")
    FName ResourceItemId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Resource", meta=(ClampMin="1"))
    int32 ResourceQuantityPerHarvest = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Resource", meta=(ClampMin="1"))
    int32 RemainingQuantity = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Resource")
    bool bInfiniteResource = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Resource", meta=(ClampMin="0.0"))
    float RespawnDurationSeconds = 30.0f;

    virtual void Interact_Implementation(AActor* InteractingActor) override;
    virtual FText GetInteractionPrompt_Implementation() const override;

protected:
    virtual void BeginPlay() override;

private:
    FTimerHandle RespawnTimerHandle;
    void RespawnNode();
};
