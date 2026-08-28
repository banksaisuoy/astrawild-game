// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AstrawildTypes.h"
#include "AstrawildEchoDataAsset.generated.h"

class AAstrawildEchoBase;
class UAnimInstance;
class USkeletalMesh;
class UStaticMesh;

UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildEchoDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UAstrawildEchoDataAsset();

	// --- Identification ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Identification")
	FGameplayTag SpeciesTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Identification")
	FText SpeciesName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Identification")
	FText SpeciesTitle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Identification")
	FText LoreDescription;

	// Legacy primary element field is intentionally retained for old assets.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Classification")
	EAstrawildElement ElementalAffinity;

	// New assets should put primary affinity first; an empty array falls back to ElementalAffinity.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Classification")
	TArray<EAstrawildElement> ElementalAffinities;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Classification")
	EAstrawildEchoRole Role;

	// --- Base Attributes ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Base Stats", meta = (ClampMin = "10"))
	float BaseMaxHealth;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Base Stats", meta = (ClampMin = "1"))
	float BaseAttackPower;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Base Stats", meta = (ClampMin = "0"))
	float BaseDefensePower;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Base Stats")
	float BaseWalkSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Base Stats")
	float BaseRunSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Base Stats")
	float WorkEfficiencyMultiplier;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Capture")
	float CaptureDifficultyModifier;

	// --- Combat & Personality ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Combat")
	TArray<FAstrawildEchoAbility> InnateAbilities;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Personality")
	TArray<FGameplayTag> DefaultPersonalityPool;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Traits")
	FGameplayTagContainer PassiveTraitTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Traits")
	FGameplayTagContainer WorkSuitabilityTags;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Partner")
	FGameplayTag PartnerSkillTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Mount")
	FName MountProfileId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Breeding")
	FName BreedingGroupId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Mount")
	bool bCanBeMounted = false;

	// --- Visuals & Audio ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Visuals")
	FColor PlaceholderTint;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Visuals")
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Visuals|Animation")
	TSoftClassPtr<UAnimInstance> AnimationBlueprintClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Visuals|Animation")
	FName AnimationProfileId = TEXT("Echo_Default");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Visuals")
	TSoftObjectPtr<UStaticMesh> FallbackStaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Visuals")
	TSoftObjectPtr<UTexture2D> SpeciesIcon;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Echo Visuals")
	TSoftClassPtr<AAstrawildEchoBase> EchoPawnClass;

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override
	{
		return FPrimaryAssetId(TEXT("AstrawildEcho"), *SpeciesTag.ToString());
	}

	UFUNCTION(BlueprintCallable, Category = "Echo Factory")
	FAstrawildCapturedEchoData CreateInstance(int32 InLevel = 1, const FText& CustomName = FText::GetEmpty()) const;
};