// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AstrawildInteractableInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UAstrawildInteractableInterface : public UInterface
{
	GENERATED_BODY()
};

class ASTRAWILDCORE_API IAstrawildInteractableInterface
{
	GENERATED_BODY()

public:
	/** Returns the interaction prompt displayed on the player's HUD (e.g. "[E] Gather Sunwood", "[E] Rest") */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractionPrompt(AActor* Interactor);

	/** Determines if the interactor is currently eligible to interact */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract(AActor* Interactor);

	/** Executes the interaction action */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool PerformInteraction(AActor* Interactor);
};