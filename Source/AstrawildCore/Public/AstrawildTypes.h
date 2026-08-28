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
	// Keep the first five serialized values stable for legacy assets and save data.
	Neutral     UMETA(DisplayName = "Neutral / Primal"),
	Solar       UMETA(DisplayName = "Solar / Flare"),
	Torrent     UMETA(DisplayName = "Torrent / Hydration"),
	Geo         UMETA(DisplayName = "Geo / Verdurous"),
	Aether      UMETA(DisplayName = "Aether / Legacy Cosmic"),
	// Production elements are append-only. Never reorder the values above.
	Volt        UMETA(DisplayName = "Volt / Storm"),
	Glacial     UMETA(DisplayName = "Glacial / Frost"),
	Abyssal     UMETA(DisplayName = "Abyssal / Void"),
	Astra       UMETA(DisplayName = "Astra / Dawn")
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
 * Functional role classification for Echo species.
 */
UENUM(BlueprintType)
enum class EAstrawildEchoRole : uint8
{
	Exploration     UMETA(DisplayName = "Exploration / Scouting"),
	Combat          UMETA(DisplayName = "Combat / Defense"),
	BaseUtility     UMETA(DisplayName = "Base / Production Utility")
};

/**
 * Ownership state of an Echo instance.
 */
UENUM(BlueprintType)
enum class EAstrawildEchoOwnership : uint8
{
	Wild                UMETA(DisplayName = "Wild"),
	TamedCompanion      UMETA(DisplayName = "Tamed Companion"),
	CampWorker          UMETA(DisplayName = "Camp Worker"),
	StoredInSanctuary   UMETA(DisplayName = "Stored in Sanctuary")
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
	Structure       UMETA(DisplayName = "Foundation / Wall / Roof"),
	// Append-only station value; existing serialized building values remain stable.
	HeatForge       UMETA(DisplayName = "Heat Forge")
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
struct ASTRAWILDCORE_API FAstrawildElementalMatrix
{
	GENERATED_BODY()

	static float GetMultiplier(EAstrawildElement Attacker, EAstrawildElement Defender)
	{
		if (Attacker == EAstrawildElement::Neutral || Defender == EAstrawildElement::Neutral)
		{
			return 1.0f;
		}

		if (Attacker == Defender)
		{
			return 0.75f;
		}

		// Production six-way loop: Abyssal > Solar > Glacial > Geo > Volt > Torrent > Abyssal.
		if ((Attacker == EAstrawildElement::Abyssal && Defender == EAstrawildElement::Solar) ||
			(Attacker == EAstrawildElement::Solar && Defender == EAstrawildElement::Glacial) ||
			(Attacker == EAstrawildElement::Glacial && Defender == EAstrawildElement::Geo) ||
			(Attacker == EAstrawildElement::Geo && Defender == EAstrawildElement::Volt) ||
			(Attacker == EAstrawildElement::Volt && Defender == EAstrawildElement::Torrent) ||
			(Attacker == EAstrawildElement::Torrent && Defender == EAstrawildElement::Abyssal))
		{
			return 1.75f;
		}

		if ((Attacker == EAstrawildElement::Solar && Defender == EAstrawildElement::Abyssal) ||
			(Attacker == EAstrawildElement::Glacial && Defender == EAstrawildElement::Solar) ||
			(Attacker == EAstrawildElement::Geo && Defender == EAstrawildElement::Glacial) ||
			(Attacker == EAstrawildElement::Volt && Defender == EAstrawildElement::Geo) ||
			(Attacker == EAstrawildElement::Torrent && Defender == EAstrawildElement::Volt) ||
			(Attacker == EAstrawildElement::Abyssal && Defender == EAstrawildElement::Torrent))
		{
			return 0.50f;
		}

		// Legacy compatibility edges from the vertical slice remain valid.
		if ((Attacker == EAstrawildElement::Solar && Defender == EAstrawildElement::Geo) ||
			(Attacker == EAstrawildElement::Geo && Defender == EAstrawildElement::Torrent) ||
			(Attacker == EAstrawildElement::Torrent && Defender == EAstrawildElement::Solar))
		{
			return 1.75f;
		}

		if ((Attacker == EAstrawildElement::Geo && Defender == EAstrawildElement::Solar) ||
			(Attacker == EAstrawildElement::Torrent && Defender == EAstrawildElement::Geo) ||
			(Attacker == EAstrawildElement::Solar && Defender == EAstrawildElement::Torrent))
		{
			return 0.50f;
		}

		return 1.0f;
	}

	static float GetMultiplier(EAstrawildElement Attacker, const TArray<EAstrawildElement>& Defenders)
	{
		if (Defenders.Num() == 0)
		{
			return 1.0f;
		}

		float Multiplier = 1.0f;
		for (const EAstrawildElement Defender : Defenders)
		{
			Multiplier *= GetMultiplier(Attacker, Defender);
		}
		return FMath::Clamp(Multiplier, 0.25f, 2.50f);
	}
};

/**
 * Inventory slot data container.
 */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildItemSlot
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
struct ASTRAWILDCORE_API FAstrawildRecipeIngredient
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
struct ASTRAWILDCORE_API FAstrawildRecipe : public FTableRowBase
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recipe|Technology")
	FGameplayTag RequiredTechnologyTag;
};

UENUM(BlueprintType)
enum class EAstrawildQuestObjectiveType : uint8
{
    Discover UMETA(DisplayName="Discover"),
    Collect UMETA(DisplayName="Collect"),
    Defeat UMETA(DisplayName="Defeat"),
    Capture UMETA(DisplayName="Capture"),
    Craft UMETA(DisplayName="Craft"),
    Reach UMETA(DisplayName="Reach"),
    Interact UMETA(DisplayName="Interact")
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildLoreRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lore")
    FName LoreId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lore")
    FText Title;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lore", meta=(MultiLine="true"))
    FText Body;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lore")
    FGameplayTag RegionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lore")
    int32 SortOrder = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Lore")
    bool bUnlockedByDefault = false;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildQuestObjective
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    FName ObjectiveId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    EAstrawildQuestObjectiveType Type = EAstrawildQuestObjectiveType::Discover;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    FGameplayTag TargetTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    int32 RequiredQuantity = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    FText Description;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildQuestObjectiveRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    FName QuestId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    FName ObjectiveId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    EAstrawildQuestObjectiveType Type = EAstrawildQuestObjectiveType::Discover;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    FGameplayTag TargetTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    int32 RequiredQuantity = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    FText Description;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildQuestRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    FName QuestId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    FText Title;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest", meta=(MultiLine="true"))
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    FGameplayTag RegionTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    FGameplayTag PrerequisiteQuestTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    TArray<FAstrawildQuestObjective> Objectives;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    TArray<FAstrawildItemSlot> Rewards;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Quest")
    bool bMainQuest = false;
};

/**
 * Combat Ability Struct for Echoes and Players.
 */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildEchoAbility
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
 * Active temporary status condition instance (e.g. Ignite, Drenched, Stun, Shield).
 */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildActiveStatusEffect
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	FGameplayTag StatusTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	float RemainingDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	float TotalDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	float Magnitude = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	float TickInterval = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	float TimeUntilNextTick = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status")
	TWeakObjectPtr<AActor> InstigatorActor;
};

/**
 * Universal Damage Event Data Struct.
 */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildDamageEvent
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float BaseDamage = 20.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	EAstrawildElement DamageElement = EAstrawildElement::Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTag DamageTypeTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	TWeakObjectPtr<AActor> DamageCauser;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	TWeakObjectPtr<AActor> InstigatorActor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FVector HitLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FVector HitDirection = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float KnockbackImpulse = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTag AppliedStatusTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float StatusDuration = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	int32 AttackInstanceId = 0;
};

/**
 * Serialized egg state. This is intentionally data-only; incubation actors can consume it later.
 */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildEchoEggData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo Egg")
	FGuid EggId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo Egg")
	FName BreedingGroupId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo Egg")
	FGameplayTag OffspringSpeciesTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo Egg")
	FGuid ParentAId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo Egg")
	FGuid ParentBId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo Egg", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IncubationProgress = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo Egg", meta = (ClampMin = "1.0"))
	float IncubationDurationSeconds = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo Egg")
	int32 Generation = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo Egg")
	TArray<EAstrawildElement> InheritedElementalAffinities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo Egg")
	FGameplayTagContainer InheritedPassiveTraits;

	FAstrawildEchoEggData()
		: EggId(FGuid::NewGuid())
		, IncubationProgress(0.0f)
		, IncubationDurationSeconds(900.0f)
		, Generation(1)
	{
	}
};

/**
 * Complete Instance and Serialized Data for an Echo (Wild, Companion, Worker, or Stored).
 */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildCapturedEchoData
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
	float TrustScore = 50.0f;

	// Legacy primary element remains serialized for existing saves.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo")
	EAstrawildElement Element = EAstrawildElement::Neutral;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo")
	TArray<EAstrawildElement> ElementalAffinities;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo")
	EAstrawildEchoRole Role = EAstrawildEchoRole::Combat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo")
	EAstrawildEchoOwnership OwnershipState = EAstrawildEchoOwnership::Wild;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo")
	FGameplayTagContainer PersonalityTraits;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo|Traits")
	FGameplayTagContainer PassiveTraitTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo|Work")
	FGameplayTagContainer WorkSuitabilityTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo|Partner")
	FGameplayTag PartnerSkillTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo|Mount")
	FName MountProfileId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo|Breeding")
	FName BreedingGroupId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo|Breeding")
	FGuid ParentAId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo|Breeding")
	FGuid ParentBId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo|Breeding")
	int32 Generation = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo|Breeding")
	int32 MutationCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo|SAN", meta = (ClampMin = "0.0"))
	float CurrentSAN = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo|SAN", meta = (ClampMin = "1.0"))
	float MaxSAN = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo|SAN", meta = (ClampMin = "0.0"))
	float SANRecoveryRate = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Echo|Work", meta = (ClampMin = "0.1"))
	float WorkEfficiencyMultiplier = 1.0f;

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
		, TrustScore(50.0f)
		, Element(EAstrawildElement::Neutral)
		, Role(EAstrawildEchoRole::Combat)
		, OwnershipState(EAstrawildEchoOwnership::Wild)
	{
	}
};

typedef FAstrawildCapturedEchoData FAstrawildEchoInstance;

/**
 * Serialized Building Structure Data.
 */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildBuildingSaveData
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
struct ASTRAWILDCORE_API FAstrawildHarvestNodeSaveData
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

/**
 * Serialized food spoilage state kept independent from cooking table definitions.
 */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildFoodSaveState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
	FGuid ItemInstanceId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
	FGameplayTag FoodItemTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food", meta = (ClampMin = "0"))
	int32 RemainingQuantity = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food", meta = (ClampMin = "0.0"))
	float RemainingFreshnessSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food")
	bool bRefrigerated = false;
};

/**
 * Serialized active food buff state.
 */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildFoodBuffSaveState
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food Buff")
	FGameplayTag BuffTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food Buff")
	float Magnitude = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Food Buff", meta = (ClampMin = "0.0"))
	float RemainingDurationSeconds = 0.0f;
};

/**
 * Isolated Player Profile State.
 */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildPlayerProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile")
	int32 SchemaVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile")
	FGuid PlayerGuid;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile")
	FTransform PlayerTransform;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile")
	float CurrentHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile")
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile")
	float CurrentStamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile")
	float MaxStamina = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile")
	int32 PlayerLevel = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile")
	float CurrentEXP = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile")
	TArray<FAstrawildItemSlot> InventorySlots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile")
	TArray<FAstrawildCapturedEchoData> ActiveParty;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile")
	TArray<FAstrawildCapturedEchoData> ReserveStorage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile|Quest")
	TArray<FName> ActiveQuestIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile|Quest")
	TArray<FName> CompletedQuestIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile|Quest")
	TMap<FName, int32> ObjectiveProgress;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile|Technology")
	TArray<FGameplayTag> UnlockedTechnologyTags;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile|Technology", meta = (ClampMin = "0"))
	int32 ResearchPoints = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile|Survival")
	float Hunger = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile|Survival")
	float Thirst = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile|Survival")
	float BodyTemperature = 21.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile|Survival")
	float CarryWeight = 0.0f;

	// Additive fields: legacy saves deserialize with empty food state arrays.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile|Survival|Food")
	TArray<FAstrawildFoodSaveState> TrackedFood;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile|Survival|Food")
	TArray<FAstrawildFoodBuffSaveState> ActiveFoodBuffs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile|Survival|Food")
	bool bFoodStorageRefrigerated = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Player Profile")
	FTransform ActiveRespawnTransform;

	FAstrawildPlayerProfile()
		: SchemaVersion(1)
		, PlayerGuid(FGuid::NewGuid())
		, PlayerTransform(FTransform::Identity)
		, CurrentHealth(100.0f)
		, MaxHealth(100.0f)
		, CurrentStamina(100.0f)
		, MaxStamina(100.0f)
		, PlayerLevel(1)
		, CurrentEXP(0.0f)
		, ActiveRespawnTransform(FTransform::Identity)
	{
	}
};

/**
 * Isolated World Snapshot State.
 */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildWorldSnapshot
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Snapshot")
	int32 SchemaVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Snapshot")
	FDateTime SnapshotTimestamp;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Snapshot")
	TArray<FAstrawildBuildingSaveData> PlacedBuildings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Snapshot")
	TArray<FAstrawildHarvestNodeSaveData> HarvestNodes;

	// Additive field: legacy schema v1 saves deserialize with an empty array and remain valid.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Snapshot|Fast Travel")
	TArray<FName> DiscoveredSpireIds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Snapshot|Breeding")
	TArray<FAstrawildEchoEggData> IncubatingEggs;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Snapshot")
	float WorldGameTimeSeconds = 0.0f;

	FAstrawildWorldSnapshot()
		: SchemaVersion(1)
		, SnapshotTimestamp(FDateTime::Now())
		, WorldGameTimeSeconds(0.0f)
	{
	}
};

/**
 * Isolated Settings & User Preferences Profile.
 */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildSettingsProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	int32 SchemaVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	int32 ScalabilityTier = 2; // 0: Low, 1: Medium, 2: High, 3: Epic

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float MasterVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float MusicVolume = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float SFXVolume = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	float MouseSensitivity = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
	bool bInvertY = false;

	FAstrawildSettingsProfile()
		: SchemaVersion(1)
		, ScalabilityTier(2)
		, MasterVolume(1.0f)
		, MusicVolume(0.8f)
		, SFXVolume(1.0f)
		, MouseSensitivity(1.0f)
		, bInvertY(false)
	{
	}
};