// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "AstrawildTypes.generated.h"

/**
 * Elemental Affinity Classification for Echoes, Spells, and Vulnerabilities.
 */
UENUM(BlueprintType)
enum class EAstrawildElement : uint8
{
	Neutral     UMETA(DisplayName = "Neutral / Primal"),
	Solar       UMETA(DisplayName = "Solar / Flare"),
	Torrent     UMETA(DisplayName = "Torrent / Hydration"),
	Geo         UMETA(DisplayName = "Geo / Verdurous"),
	Aether      UMETA(DisplayName = "Aether / Cosmic")
};

/**
 * Item categorization across survival, crafting, and combat domains.
 */
UENUM(BlueprintType)
enum class EAstrawildItemCategory : uint8
{
	Resource    UMETA(DisplayName = "Raw Resource / Material"),
	Tool        UMETA(DisplayName = "Harvesting Tool"),
	Weapon      UMETA(DisplayName = "Combat Weapon"),
	Consumable  UMETA(DisplayName = "Consumable / Resonator"),
	Equipment   UMETA(DisplayName = "Armor / Gear"),
	Building    UMETA(DisplayName = "Building Structure")
};

/**
 * Behavioral state machine representation for wild and companion Echoes.
 */
UENUM(BlueprintType)
enum class EAstrawildEchoState : uint8
{
	WildPassive         UMETA(DisplayName = "Wild (Roaming / Grazing)"),
	WildHostile         UMETA(DisplayName = "Wild (Combat Engaged)"),
	Fleeing             UMETA(DisplayName = "Fleeing (Low Health / Coward)"),
	Captured            UMETA(DisplayName = "Captured (In Transit / Ball)"),
	SummonedCompanion   UMETA(DisplayName = "Active Companion (Following Player)"),
	Working             UMETA(DisplayName = "Working at Camp / Settlement")
};

/**
 * Harvest resource node classification.
 */
UENUM(BlueprintType)
enum class EAstrawildHarvestType : uint8
{
	Lumber      UMETA(DisplayName = "Lumber (Wood Trees)"),
	Mining      UMETA(DisplayName = "Mining (Ores & Crystals)"),
	Foraging    UMETA(DisplayName = "Foraging (Plants & Shards)")
};

/**
 * Base building piece functional classification.
 */
UENUM(BlueprintType)
enum class EAstrawildBuildingType : uint8
{
	None            UMETA(DisplayName = "None / Generic"),
	Campfire        UMETA(DisplayName = "Campfire (Heat & Cooking)"),
	RestBed         UMETA(DisplayName = "Resting Shelter / Bed"),
	CraftingBench   UMETA(DisplayName = "Crafting Bench"),
	StorageChest    UMETA(DisplayName = "Storage Chest"),
	Structure       UMETA(DisplayName = "Foundation / Wall / Roof")
};

/**
 * Astra Resonator capture progression state.
 */
UENUM(BlueprintType)
enum class EAstrawildCaptureState : uint8
{
	None            UMETA(DisplayName = "None"),
	InFlight        UMETA(DisplayName = "In Flight"),
	Trapping        UMETA(DisplayName = "Containment Beam"),
	Rolling         UMETA(DisplayName = "Capture Probability Rolling"),
	Success         UMETA(DisplayName = "Capture Success"),
	Breakout        UMETA(DisplayName = "Capture Failed (Breakout)")
};

/**
 * Elemental Advantage Calculation Matrix
 */
USTRUCT(BlueprintType)
struct FAstrawildElementalMatrix
{
	GENERATED_BODY()

	static float GetMultiplier(EAstrawildElement Attacker, EAstrawildElement Defender)
	{
		if (Attacker == EAstrawildElement::Neutral || Defender == EAstrawildElement::Neutral)
		{
			return 1.0f;
		}

		// Solar > Geo (Earth/Flora) > Torrent (Water) > Solar
		if (Attacker == EAstrawildElement::Solar && Defender == EAstrawildElement::Geo)
		{
			return 1.75f; // Super effective
		}
		if (Attacker == EAstrawildElement::Geo && Defender == EAstrawildElement::Torrent)
		{
			return 1.75f;
		}
		if (Attacker == EAstrawildElement::Torrent && Defender == EAstrawildElement::Solar)
		{
			return 1.75f;
		}

		// Disadvantaged reverse
		if (Attacker == EAstrawildElement::Solar && Defender == EAstrawildElement::Torrent)
		{
			return 0.5f;
		}
		if (Attacker == EAstrawildElement::Torrent && Defender == EAstrawildElement::Geo)
		{
			return 0.5f;
		}
		if (Attacker == EAstrawildElement::Geo && Defender == EAstrawildElement::Solar)
		{
			return 0.5f;
		}

		// Same element resistance
		if (Attacker == Defender)
		{
			return 0.75f;
		}

		return 1.0f;
	}
};

/**
 * Inventory slot data container.
 */
USTRUCT(BlueprintType)
struct FAstrawildItemSlot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FGameplayTag ItemTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 Quantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float Durability = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FGuid InstanceId;

	FAstrawildItemSlot()
		: ItemTag(FGameplayTag::EmptyTag)
		, Quantity(0)
		, Durability(100.0f)
		, InstanceId(FGuid::NewGuid())
	{
	}

	FAstrawildItemSlot(const FGameplayTag& InTag, int32 InQty, float InDurability = 100.0f)
		: ItemTag(InTag)
		, Quantity(InQty)
		, Durability(InDurability)
		, InstanceId(FGuid::NewGuid())
	{
	}

	bool IsValid() const
	{
		return ItemTag.IsValid() && Quantity > 0;
	}

	void Clear()
	{
		ItemTag = FGameplayTag::EmptyTag;
		Quantity = 0;
		Durability = 100.0f;
		InstanceId.Invalidate();
	}
};

/**
 * Recipe Ingredient Specification.
 */
USTRUCT(BlueprintType)
struct FAstrawildRecipeIngredient
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe")
	FGameplayTag ItemTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe")
	int32 Quantity = 1;
};

/**
 * Complete Recipe Definition Struct.
 */
USTRUCT(BlueprintType)
struct FAstrawildRecipe : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe")
	FGameplayTag RecipeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe")
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe")
	TArray<FAstrawildRecipeIngredient> Ingredients;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe")
	FGameplayTag OutputItemTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe")
	int32 OutputQuantity = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe")
	float CraftTimeSeconds = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe")
	EAstrawildBuildingType RequiredStation = EAstrawildBuildingType::None;
};

/**
 * Combat Ability Struct for Echoes and Players.
 */
USTRUCT(BlueprintType)
struct FAstrawildEchoAbility
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	FGameplayTag AbilityTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	FText AbilityName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	EAstrawildElement Element = EAstrawildElement::Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float BaseDamage = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float CooldownSeconds = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float CastRange = 600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability")
	float Radius = 150.0f;
};

/**
 * Serialized Data for a Captured Echo (in Party or Storage Sanctuary).
 */
USTRUCT(BlueprintType)
struct FAstrawildCapturedEchoData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo")
	FGuid UniqueEchoId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo")
	FGameplayTag SpeciesTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo")
	FText CustomNickname;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo")
	int32 Level = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo")
	float CurrentEXP = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo")
	float CurrentHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo")
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo")
	float AttackPower = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo")
	float DefensePower = 15.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo")
	EAstrawildElement Element = EAstrawildElement::Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo")
	TArray<FGameplayTag> EquippedAbilities;

	FAstrawildCapturedEchoData()
		: UniqueEchoId(FGuid::NewGuid())
		, SpeciesTag(FGameplayTag::EmptyTag)
		, CustomNickname(FText::GetEmpty())
		, Level(1)
		, CurrentEXP(0.0f)
		, CurrentHealth(100.0f)
		, MaxHealth(100.0f)
		, AttackPower(20.0f)
		, DefensePower(15.0f)
		, Element(EAstrawildElement::Neutral)
	{
	}
};

/**
 * Serialized Building Structure Data.
 */
USTRUCT(BlueprintType)
struct FAstrawildBuildingSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	FGuid BuildingGuid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	FGameplayTag BuildingTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	EAstrawildBuildingType BuildingType = EAstrawildBuildingType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	FTransform WorldTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	float CurrentHealth = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Building")
	TArray<FAstrawildItemSlot> ContainerInventory;

	FAstrawildBuildingSaveData()
		: BuildingGuid(FGuid::NewGuid())
		, BuildingTag(FGameplayTag::EmptyTag)
		, BuildingType(EAstrawildBuildingType::None)
		, WorldTransform(FTransform::Identity)
		, CurrentHealth(500.0f)
	{
	}
};

/**
 * Serialized Harvest Node Status.
 */
USTRUCT(BlueprintType)
struct FAstrawildHarvestNodeSaveData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	FGuid NodeGuid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	FTransform WorldTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	bool bIsDepleted = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Environment")
	float RespawnTimeRemaining = 0.0f;

	FAstrawildHarvestNodeSaveData()
		: NodeGuid(FGuid::NewGuid())
		, WorldTransform(FTransform::Identity)
		, bIsDepleted(false)
		, RespawnTimeRemaining(0.0f)
	{
	}
};