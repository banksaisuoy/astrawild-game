#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AstrawildInteractable.generated.h"

UINTERFACE(BlueprintType)
class ASTRAWILDCORE_API UAstrawildInteractable : public UInterface
{
    GENERATED_BODY()
};

class ASTRAWILDCORE_API IAstrawildInteractable
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="ASTRAWILD|Interaction")
    void Interact(AActor* InteractingActor);

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="ASTRAWILD|Interaction")
    FText GetInteractionPrompt() const;
};
