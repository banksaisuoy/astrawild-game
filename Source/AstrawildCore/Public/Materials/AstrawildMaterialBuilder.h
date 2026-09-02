// Copyright Astrawild. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "AstrawildMaterialBuilder.generated.h"

/**
 * Helper class to programmatically create Material Instances and set parameters
 * for Survivor and Echoes based on verified M_Master_Surface.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildMaterialBuilder : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Create Material Instance for Survivor Exosuit
	 * @param ParentMaterial Path to M_Master_Surface
	 * @return Created Material Instance
	 */
	UFUNCTION(BlueprintCallable, Category = "Astrawild|Materials")
	static UMaterialInstanceDynamic* CreateSurvivorExosuitMI(UObject* WorldContext, const FString& ParentMaterialPath);

	/**
	 * Create Material Instance for a specific Echo species
	 * @param SpeciesName Name of the Echo (e.g., "Bastionbeetle")
	 * @param ParentMaterial Path to M_Master_Surface
	 * @return Created Material Instance
	 */
	UFUNCTION(BlueprintCallable, Category = "Astrawild|Materials")
	static UMaterialInstanceDynamic* CreateEchoMI(UObject* WorldContext, const FString& SpeciesName, const FString& ParentMaterialPath);

	/**
	 * Apply Elemental Parameters to a Material Instance
	 * @param MatInst Target Material Instance
	 * @param Element Element Type (Ember, Frost, etc.)
	 */
	UFUNCTION(BlueprintCallable, Category = "Astrawild|Materials")
	static void ApplyElementalParameters(UMaterialInstanceDynamic* MatInst, FName Element);

	/**
	 * Batch create MIs for first 3 Echoes + Survivor
	 * Used for initial setup/verification
	 */
	UFUNCTION(BlueprintCallable, Category = "Astrawild|Materials")
	static void BuildInitialMaterialSet(UObject* WorldContext, const FString& ParentMaterialPath);

private:
	// Internal helper to get element colors/stats
	static void GetElementalData(FName Element, FLinearColor& BaseColor, FLinearColor& Emissive, float& Roughness, float& Metallic);
};
