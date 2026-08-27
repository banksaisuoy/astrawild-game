// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AstrawildTypes.h"
#include "AstrawildDamageableInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UAstrawildDamageableInterface : public UInterface
{
	GENERATED_BODY()
};

class ASTRAWILDCORE_API IAstrawildDamageableInterface
{
	GENERATED_BODY()

public:
	/** Applies authoritative damage and status effects to this actor */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	float TakeAstrawildDamage(const FAstrawildDamageEvent& DamageEvent);

	/** Checks if this actor can currently receive damage */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	bool CanTakeDamage(AActor* Attacker);

	/** Returns the actor's innate elemental affinity */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Combat")
	EAstrawildElement GetElementalAffinity() const;
};