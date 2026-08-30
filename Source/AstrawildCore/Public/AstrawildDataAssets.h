#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AstrawildTypes.h"
#include "AstrawildDataAssets.generated.h"

class USkeletalMesh;
class UTexture2D;

UENUM(BlueprintType)
enum class EAstrawildItemCategory : uint8
{
    Material UMETA(DisplayName="Material"),
    Consumable UMETA(DisplayName="Consumable"),
    Equipment UMETA(DisplayName="Equipment"),
    CreatureItem UMETA(DisplayName="Creature Item"),
    BuildingItem UMETA(DisplayName="Building Item"),
    QuestItem UMETA(DisplayName="Quest Item")
};

UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildItemDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item")
    FName ItemId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item", meta=(MultiLine=true))
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item", meta=(ClampMin="1"))
    int32 MaxStackSize = 99;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item", meta=(ClampMin="0.0"))
    float Weight = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item")
    TSoftObjectPtr<UTexture2D> Icon;

    // --- V2 fields (additive) ---

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item")
    EAstrawildItemCategory Category = EAstrawildItemCategory::Material;

    /** Consumable: hunger/thirst restoration (directive §11). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Food", meta=(ClampMin="0.0", ClampMax="100.0"))
    float FoodValue = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Food", meta=(ClampMin="0.0", ClampMax="100.0"))
    float WaterValue = 0.0f;

    /** Consumable: health restoration. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Food", meta=(ClampMin="0.0"))
    float HealValue = 0.0f;

    /** Equipment: base attack added to player. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment", meta=(ClampMin="0.0"))
    float AttackPower = 0.0f;

    /** Equipment: damage reduction fraction 0..0.8 when blocking. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment", meta=(ClampMin="0.0", ClampMax="0.8"))
    float BlockMitigation = 0.0f;

    /**
     * Batch 3 — Item A: element carried by equipment (weapons). None = fall back to
     * the combat component's AttackElement tunable (legacy behaviour).
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment")
    EAstrawildElementType Element = EAstrawildElementType::None;

    /**
     * Batch 3 — Item C: armor rating for torso armor. Feeds the diminishing-returns
     * damage-reduction formula ComputeArmorFraction(Rating, K) on the combat component.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment", meta=(ClampMin="0.0"))
    float ArmorRating = 0.0f;

    /** Echo food preference bonus multiplier when fed this item (directive §8). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Food", meta=(ClampMin="0.0", ClampMax="3.0"))
    float EchoFeedValue = 0.0f;

    /**
     * Batch 4 — M-11: vendor buy price in the NPC's currency item. 0 = not
     * tradeable. Sell value at a vendor is half the buy price (floor 1) via
     * AAstrawildNPCCharacter::ComputeVendorSellValue. The currency item itself
     * must keep VendorPrice = 0 so it cannot be bought with itself.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Economy", meta=(ClampMin="0"))
    int32 VendorPrice = 0;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(FPrimaryAssetType(TEXT("Item")), ItemId);
    }
};

UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildRecipeDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Recipe")
    FName RecipeId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Recipe")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Recipe")
    TArray<FAstrawildItemStack> Ingredients;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Recipe")
    TArray<FAstrawildItemStack> Outputs;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Recipe", meta=(ClampMin="0.0"))
    float CraftDurationSeconds = 0.0f;

    // --- V2 fields (additive) ---

    /** Technology that must be unlocked before this recipe is craftable (NAME_None = always). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Recipe")
    FName RequiredTechId = NAME_None;

    /** Crafting station required (NAME_None = craft anywhere). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Recipe")
    FName RequiredStationId = NAME_None;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(FPrimaryAssetType(TEXT("Recipe")), RecipeId);
    }
};

UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildEchoDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo")
    FName DefinitionId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo", meta=(MultiLine=true))
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo")
    EAstrawildElementType Element = EAstrawildElementType::None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo")
    EAstrawildEchoRole Role = EAstrawildEchoRole::Support;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo")
    FAstrawildEchoStats BaseStats;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo", meta=(ClampMin="0.0"))
    float TrustGainOnCapture = 10.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo")
    TSoftObjectPtr<USkeletalMesh> SkeletalMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo")
    TArray<FName> AbilityIds;

    // --- V2 fields (additive) — full species template per directive §4/§5 ---

    /** Activity gating vs world time-of-day (directive §13). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|Behavior")
    EAstrawildActivityPattern ActivityPattern = EAstrawildActivityPattern::Diurnal;

    /** Preferred personality archetype rolled on spawn — drives AI thresholds (directive §5). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|Behavior")
    EAstrawildPersonality DominantPersonality = EAstrawildPersonality::Curious;

    /** Preferred food item ids — feeding these boosts capture/trust (directive §8). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|Capture")
    TArray<FName> PreferredFoodIds;

    /** Biome tags this species naturally inhabits. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|Behavior")
    TArray<FName> HabitatBiomeIds;

    /** Weather states this species prefers — capture bonus during them (directive §8/§12). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|Capture")
    TArray<EAstrawildWeatherState> PreferredWeather;

    /** 0..1 — base difficulty of capture before weaken/trust factors. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|Capture", meta=(ClampMin="0.0", ClampMax="1.0"))
    float CaptureDifficulty = 0.4f;

    /** Element this Echo is weak against — 1.5x damage taken (directive §9). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|Combat")
    EAstrawildElementType WeaknessElement = EAstrawildElementType::None;

    /** Flat damage reduction vs its own element. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|Combat", meta=(ClampMin="0.0", ClampMax="0.8"))
    float ElementalResistance = 0.2f;

    /** Work affinities — not every Echo does every job equally (directive §18). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|Work")
    TArray<FAstrawildWorkAffinity> WorkAffinities;

    /** Needs decay per in-game hour (directive §4 Needs). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|Needs", meta=(ClampMin="0.0"))
    float HungerDecayPerHour = 4.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|Needs", meta=(ClampMin="0.0"))
    float EnergyDecayPerHour = 6.0f;

    /** XP required to reach level 2 (scales per level). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|Growth", meta=(ClampMin="1.0"))
    float BaseExperienceToLevel = 100.0f;

    /** Loot dropped on defeat (directive §6 loot). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|Combat")
    TArray<FAstrawildItemStack> DefeatLoot;

    /** Hostile species attack players unprovoked (directive §21 first hostile). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|Behavior")
    bool bHostileToPlayers = false;

    /** Perception range in cm (sight). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|AI", meta=(ClampMin="100.0"))
    float SightRadius = 1500.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|AI", meta=(ClampMin="100.0"))
    float LoseSightRadius = 2200.0f;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(FPrimaryAssetType(TEXT("Echo")), DefinitionId);
    }
};

/** Modular building piece definition (directive §16). */
UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildBuildingDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Building")
    FName DefinitionId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Building")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Building")
    EAstrawildBuildingCategory Category = EAstrawildBuildingCategory::Foundation;

    /** Item consumed from inventory when placing. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Building")
    FName RequiredItemId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Building", meta=(ClampMin="1"))
    int32 RequiredItemCount = 1;

    /** Technology required before placement (NAME_None = always). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Building")
    FName RequiredTechId = NAME_None;

    /** Grid cell size in cm this piece occupies (200 = one foundation cell). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Building", meta=(ClampMin="50.0"))
    float GridCellSize = 200.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Building", meta=(ClampMin="1.0"))
    float MaxHealth = 500.0f;

    // --- Power integration (directive §17) ---

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Building|Power")
    EAstrawildPowerRole PowerRole = EAstrawildPowerRole::Consumer;

    /** Units of power produced per second when active (Generator). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Building|Power", meta=(ClampMin="0.0"))
    float PowerGeneration = 0.0f;

    /** Units of power drawn per second when switched on (Consumer). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Building|Power", meta=(ClampMin="0.0"))
    float PowerDraw = 0.0f;

    /** Battery capacity in power-seconds (Battery). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Building|Power", meta=(ClampMin="0.0"))
    float BatteryCapacity = 0.0f;

    /** Work type enabled when powered (workstation) — for Echo work assignment. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Building")
    EAstrawildWorkType EnabledWorkType = EAstrawildWorkType::None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Building")
    TSoftObjectPtr<UTexture2D> Icon;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(FPrimaryAssetType(TEXT("Building")), DefinitionId);
    }
};

/** Technology tree node (directive §19). */
UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildTechnologyDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Tech")
    FName TechId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Tech")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Tech", meta=(MultiLine=true))
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Tech")
    EAstrawildTechEra Era = EAstrawildTechEra::Primitive;

    /** Research point cost to unlock. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Tech", meta=(ClampMin="0"))
    int32 ResearchCost = 10;

    /** All of these must be unlocked first. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Tech")
    TArray<FName> PrerequisiteTechIds;

    /** Recipes unlocked by this tech. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Tech")
    TArray<FName> UnlockedRecipeIds;

    /** Buildings unlocked by this tech. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Tech")
    TArray<FName> UnlockedBuildingIds;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(FPrimaryAssetType(TEXT("Tech")), TechId);
    }
};

/** Data-driven quest (directive §25). */
UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildQuestDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Quest")
    FName QuestId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Quest")
    FText Title;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Quest", meta=(MultiLine=true))
    FText Summary;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Quest")
    TArray<FAstrawildQuestObjective> Objectives;

    /** Item rewards granted on completion. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Quest")
    TArray<FAstrawildItemStack> RewardItems;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Quest", meta=(ClampMin="0"))
    int32 RewardResearchPoints = 0;

    /** Next quest in the chain, auto-activated on completion (NAME_None = end). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Quest")
    FName NextQuestId = NAME_None;

    /** Tech unlocked instantly on completion (story-critical research, directive §19). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Quest")
    FName RewardTechId = NAME_None;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(FPrimaryAssetType(TEXT("Quest")), QuestId);
    }
};

/** Loot table — weighted drops (directive §6 loot). */
UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildLootTableDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Loot")
    FName LootTableId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Loot")
    TArray<FAstrawildItemStack> GuaranteedDrops;

    /** Chance 0..1 for one extra random roll from GuaranteedDrops. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Loot", meta=(ClampMin="0.0", ClampMax="1.0"))
    float BonusRollChance = 0.0f;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(FPrimaryAssetType(TEXT("LootTable")), LootTableId);
    }
};

/** NPC definition — schedule, dialogue topics, quest hooks (directive §26). */
UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildNPCDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|NPC")
    FName NpcId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|NPC")
    FText DisplayName;

    /** Quest offered when talked to. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|NPC")
    FName OfferedQuestId = NAME_None;

    /** Fixed shop loot table id (future extension). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|NPC")
    FName ShopLootTableId = NAME_None;

    /**
     * Batch 4 — M-11: item id of the currency this vendor trades in. Wares come
     * from the ShopLootTableId loot table; prices come from each item's
     * VendorPrice. No currency configured → the shop is closed (NotAVendor).
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|NPC")
    FName CurrencyItemId = NAME_None;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(FPrimaryAssetType(TEXT("NPC")), NpcId);
    }
};
