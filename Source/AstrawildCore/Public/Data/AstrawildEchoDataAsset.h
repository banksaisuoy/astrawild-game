// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AstrawildTypes.h"
#include "AstrawildEchoDataAsset.generated.h"

class AAstrawildEchoBase;

UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildEchoDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UAstrawildEchoDataAsset();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Identification")
	FGameplayTag SpeciesTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Identification")
	FText SpeciesName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Identification")
	FText SpeciesTitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Identification")
	FText LoreDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Element")
	EAstrawildElement ElementalAffinity;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Base Stats")
	float BaseMaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Base Stats")
	float BaseAttackPower;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Base Stats")
	float BaseDefensePower;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Base Stats")
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Base Stats")
	float BaseRunSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Capture")
	float CaptureDifficultyModifier;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Combat")
	TArray<FAstrawildEchoAbility> InnateAbilities;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Visuals")
	TSoftObjectPtr<UTexture2D> SpeciesIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Visuals")
	TSoftClassPtr<AAstrawildEchoBase> EchoPawnClass;

	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("AstrawildEcho"), *SpeciesTag.ToString());
	}
};