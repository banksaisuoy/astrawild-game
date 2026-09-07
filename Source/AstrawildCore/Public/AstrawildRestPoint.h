#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildInteractable.h"
#include "AstrawildTypes.h"
#include "AstrawildRestPoint.generated.h"

class AAstrawildRestPoint;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildRestPointActivated, AAstrawildRestPoint*, RestPoint);

UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildRestPoint : public AActor, public IAstrawildInteractable
{
    GENERATED_BODY()

public:
    AAstrawildRestPoint();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|RestPoint")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|RestPoint")
    FAstrawildRestPointActivated OnActivated;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|RestPoint", Replicated)
    FGuid WorldObjectId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|RestPoint", Replicated)
    bool bActive = false;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|RestPoint")
    void ActivateRestPoint();

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|RestPoint")
    FAstrawildRestPointSaveData ToSaveData() const;

    virtual void Interact_Implementation(AActor* InteractingActor) override;
    virtual FText GetInteractionPrompt_Implementation() const override;

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override; // LCP-2
};
