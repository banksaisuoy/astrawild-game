#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AstrawildTypes.h"
#include "AstrawildDataAssets.generated.h"

class USkeletalMesh;
class UTexture2D;
class UStaticMesh;
class UMaterialInterface;
class USoundBase;

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

    // --- Final production run (PHASE 12 — advanced technology framework) ---

    /** Explicit equipment slot; Auto keeps the legacy stat-based routing. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment")
    EAstrawildEquipmentSlot EquipmentSlot = EAstrawildEquipmentSlot::Auto;

    /**
     * Thermal protection (helmet/exosuit): the equipped total widens the player's
     * comfortable temperature band by this many Celsius on BOTH sides before
     * exposure damage ticks (survival component consumes the sum).
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment", meta=(ClampMin="0.0"))
    float InsulationRating = 0.0f;

    /** Exosuit: extra stamina regenerated per second. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment", meta=(ClampMin="0.0"))
    float StaminaRegenBonus = 0.0f;

    /** Exosuit: extra carry weight (kg) added to the inventory cap. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment", meta=(ClampMin="0.0"))
    float CarryWeightBonus = 0.0f;

    /** Exosuit: fractional walk/sprint speed bonus (0.15 = +15%). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MoveSpeedBonus = 0.0f;

    /** Weapon: fires projectiles instead of the melee sweep (laser/energy path). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment")
    bool bIsRangedWeapon = false;

    /** Weapon: ammo item consumed per shot (NAME_None = no ammo requirement). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment")
    FName AmmoItemId = NAME_None;

    /** Scanner: multiplier on journal observation rate while actively scanning. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment", meta=(ClampMin="1.0", ClampMax="10.0"))
    float ScannerSpeedMultiplier = 3.0f;

    /** Deployable: consuming this item spawns a utility drone companion. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment")
    bool bDeploysDrone = false;

    /** Deployable: consuming this item spawns a utility robot worker. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment")
    bool bDeploysRobot = false;

    // --- Production V2 (additive) — weapons, armor, scanner, drone modules ---

    /** Weapon behaviour profile (firing family, damage, VFX hooks). NAME_None = legacy stat path. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment")
    FName WeaponDefinitionId = NAME_None;

    /** Content rarity (display + economy signal). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item")
    EAstrawildRarity Rarity = EAstrawildRarity::Common;

    /** Armor/helmet/exosuit: cold-side insulation in Celsius (0 = none). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment", meta=(ClampMin="0.0"))
    float ColdInsulationRating = 0.0f;

    /** Armor/helmet/exosuit: heat-side insulation in Celsius (0 = none). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment", meta=(ClampMin="0.0"))
    float HeatInsulationRating = 0.0f;

    /** Equipment tier label for UI/progression (Field/Mk I-III/Experimental). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment")
    EAstrawildTechTier TechTier = EAstrawildTechTier::Field;

    /** Scanner: observation range multiplier while equipped (1 = base 1400cm). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment", meta=(ClampMin="1.0", ClampMax="4.0"))
    float ScannerRangeMultiplier = 1.0f;

    /** Scanner: reveals hidden resource nodes (scanner-gated harvesting). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment")
    bool bHiddenResourceDetection = false;

    /** Scanner: doubles ancient-POI discovery radius + unlocks signal tracking HUD. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment")
    bool bAncientSignalTracking = false;

    /** Drone module: adds to the drone's scan pulse radius (cm). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|DroneModule", meta=(ClampMin="0.0"))
    float DroneScanRadiusBonus = 0.0f;

    /** Drone module: adds to the drone's harvest radius (cm). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|DroneModule", meta=(ClampMin="0.0"))
    float DroneHarvestRadiusBonus = 0.0f;

    /** Drone module: adds to observation progress per scan pulse. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|DroneModule", meta=(ClampMin="0.0"))
    float DroneScanRateBonus = 0.0f;

    /** Drone module: extends the deployed battery capacity (seconds). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|DroneModule", meta=(ClampMin="0.0"))
    float DroneBatteryBonusSeconds = 0.0f;

    /** Robot: chassis specialization id (mining/farming/defense profiles). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Item|Equipment")
    FName RobotDefinitionId = NAME_None;

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

    // --- Batch 8 fields (additive) — the Grand Menagerie roster (200+ species) ---

    /** Creature lineage — drives silhouette family, loot flavor and work affinities. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|Appearance")
    EAstrawildEchoFamily Family = EAstrawildEchoFamily::Beast;

    /** Procedural silhouette kit used by the runtime body builder. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|Appearance")
    EAstrawildBodyPlan BodyPlan = EAstrawildBodyPlan::Quadruped;

    /** Overall size class — scales the procedural body and the stat budget. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|Appearance")
    EAstrawildSizeClass SizeClass = EAstrawildSizeClass::Medium;

    /** Primary body tint (procedural body material). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|Appearance")
    FLinearColor PrimaryTint = FLinearColor(0.7f, 0.7f, 0.7f);

    /** Secondary tint — limbs, wings, crest accents. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|Appearance")
    FLinearColor SecondaryTint = FLinearColor(0.45f, 0.45f, 0.45f);

    /** Home zone — wildlife seeding picks habitat rows from this. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|Habitat")
    EAstrawildZone HomeZone = EAstrawildZone::DawnFields;

    /** Codex number (1-based) shown in the journal bestiary UI. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo", meta=(ClampMin="0"))
    int32 CodexIndex = 0;

    // --- Production V2 (additive) — production roster fields ---

    /** Rarity tier — capture bragging rights + spawn table weighting (Master Plan §6). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo")
    EAstrawildRarity Rarity = EAstrawildRarity::Common;

    /** Permanent party aura while this Echo fights beside the player. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo|Behavior")
    EAstrawildEchoPassive Passive = EAstrawildEchoPassive::None;

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

    // --- Production V2 (additive): research branch grouping (Master Plan §16) ---

    /** Progression branch shown in the research UI. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Tech")
    EAstrawildResearchBranch Branch = EAstrawildResearchBranch::Survival;

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

    // --- Batch 8 fields (additive) — living villages (Docs/ASTRAWILD_VILLAGES_SKIFF.md) ---

    /** Village behaviour archetype — guards patrol and fight, vendors tend a stall. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|NPC")
    EAstrawildNPCRole Role = EAstrawildNPCRole::Villager;

    /** Village this NPC calls home (waypoint provider id, e.g. Village_Dawnstead). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|NPC")
    FName VillageId = NAME_None;

    /** Robe/body tint — procedural villager appearance. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|NPC")
    FLinearColor PrimaryTint = FLinearColor(0.65f, 0.55f, 0.45f);

    /** One-line greeting shown with the interaction prompt. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|NPC")
    FText Greeting;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(FPrimaryAssetType(TEXT("NPC")), NpcId);
    }
};

// ============================================================================
// PRODUCTION V2 DATA ASSETS — weapon/resource/robot/worksite/event/POI/biome
// definitions (Master Plan §3 STEP 3: content is data, not hardcoded classes)
// ============================================================================

/**
 * Weapon behaviour profile (Master Plan §8): each family maps to a distinct
 * firing archetype. The ITEM remains the inventory/economy entity; this
 * definition carries combat behaviour + VFX/audio hooks for Antigravity.
 */
UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildWeaponDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon")
    FName WeaponId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon", meta=(MultiLine=true))
    FText Description;

    /** Family — progression ladder + UI grouping (Kinetic→Experimental). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon")
    EAstrawildWeaponFamily Family = EAstrawildWeaponFamily::Kinetic;

    /** Tier label (Field grade → Experimental). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon")
    EAstrawildTechTier Tier = EAstrawildTechTier::Field;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon")
    EAstrawildRarity Rarity = EAstrawildRarity::Common;

    /** Delivery archetype — drives the combat component execution branch. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon")
    EAstrawildWeaponFireMode FireMode = EAstrawildWeaponFireMode::Projectile;

    /** Damage per hit BEFORE the equipped-weapon attack bonus is added. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon|Damage", meta=(ClampMin="1.0"))
    float DamagePerHit = 20.0f;

    /** Seconds between shots (server-side gate). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon|Damage", meta=(ClampMin="0.05"))
    float FireIntervalSeconds = 0.35f;

    /** Elemental payload — drives weaknesses + status effects. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon|Damage")
    EAstrawildElementType Element = EAstrawildElementType::None;

    /** Ammo item consumed per shot (NAME_None = free). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon|Ammo")
    FName AmmoItemId = NAME_None;

    // --- Projectile mode tuning ---

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon|Projectile", meta=(ClampMin="500.0"))
    float ProjectileSpeed = 6000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon|Projectile", meta=(ClampMin="0.1"))
    float ProjectileVisualScale = 0.35f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon|Projectile", meta=(ClampMin="0.5"))
    float ProjectileLifetimeSeconds = 5.0f;

    // --- Homing mode tuning (missiles) ---

    /** Cone half-angle used to acquire a lock-on target (degrees). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon|Homing", meta=(ClampMin="5.0", ClampMax="45.0"))
    float LockOnConeHalfAngle = 18.0f;

    /** Max distance a lock-on target may be acquired at (cm). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon|Homing", meta=(ClampMin="1000.0"))
    float LockOnRange = 9000.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon|Homing", meta=(ClampMin="10.0"))
    float HomingAcceleration = 2400.0f;

    // --- Beam / Arc tuning (hitscan) ---

    /** Max trace distance (cm). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon|Beam", meta=(ClampMin="1000.0"))
    float BeamRange = 15000.0f;

    /** Beam: how many enemies a single shot pierces through. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon|Beam", meta=(ClampMin="0"))
    int32 PierceCount = 0;

    /** Arc: how many extra targets the bolt chains to. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon|Beam", meta=(ClampMin="0"))
    int32 ChainCount = 0;

    /** Arc: chain search radius around the previous target (cm). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon|Beam", meta=(ClampMin="100.0"))
    float ChainRadius = 600.0f;

    /** Fraction of the base damage carried by each arc chain hop (0..1). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon|Beam", meta=(ClampMin="0.0", ClampMax="1.0"))
    float ChainDamageFraction = 0.6f;

    // --- Antigravity VFX/audio contract (bind Niagara/SoundCues by id) ---

    /** Niagara muzzle flash asset id (VFX contract: NS_AW_Weap_<MuzzleVfxId>). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon|VFX")
    FName MuzzleVfxId = NAME_None;

    /** Niagara projectile/beam trail asset id. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon|VFX")
    FName TrailVfxId = NAME_None;

    /** Niagara impact asset id. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon|VFX")
    FName ImpactVfxId = NAME_None;

    /** Fire sound id (audio contract: SC_AW_Weap_<FireSoundId>). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Weapon|VFX")
    FName FireSoundId = NAME_None;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(FPrimaryAssetType(TEXT("Weapon")), WeaponId);
    }
};

/**
 * Deterministic resource node identity (P0 fix — resolves the ResourceItemId
 * bootstrap weakness flagged in the Production V2 Master Plan §1).
 * Spawners reference NodeId; the actor resolves every stat from this data.
 */
UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildResourceNodeDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|ResourceNode")
    FName NodeId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|ResourceNode")
    FText DisplayName;

    /** Item granted per harvest — the ONLY source of truth (never empty for valid defs). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|ResourceNode")
    FName ResourceItemId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|ResourceNode")
    EAstrawildRarity Rarity = EAstrawildRarity::Common;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|ResourceNode", meta=(ClampMin="1"))
    int32 QuantityPerHarvest = 2;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|ResourceNode", meta=(ClampMin="1"))
    int32 MaxQuantity = 3;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|ResourceNode", meta=(ClampMin="0.0"))
    float RespawnDurationSeconds = 30.0f;

    /** Hidden veins: harvestable only while a scanner with HiddenResourceDetection is equipped. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|ResourceNode")
    bool bRequiresScannerDetection = false;

    /** Placeholder body tint until Antigravity binds real meshes (visual contract). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|ResourceNode")
    FLinearColor NodeTint = FLinearColor(0.55f, 0.55f, 0.55f);

    /** Visual scale of the placeholder mesh. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|ResourceNode", meta=(ClampMin="0.1"))
    float VisualScale = 1.0f;

    /** Antigravity mesh contract: static mesh override for this node type. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|ResourceNode")
    TSoftObjectPtr<UStaticMesh> MeshOverride;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(FPrimaryAssetType(TEXT("ResourceNode")), NodeId);
    }
};

/**
 * Robot chassis specialization (Master Plan §12): mining/farming/defense
 * profiles share the existing robot actor — behaviour comes from this data.
 */
UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildRobotDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Robot")
    FName RobotId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Robot")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Robot", meta=(MultiLine=true))
    FText Description;

    /** Work sites of this type get the specialist rate; everything else the generic rate. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Robot")
    EAstrawildWorkType PrimaryWorkType = EAstrawildWorkType::Gathering;

    /** Work rate when the site's WorkType matches PrimaryWorkType. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Robot", meta=(ClampMin="0.0", ClampMax="4.0"))
    float SpecialistWorkRate = 1.4f;

    /** Work rate on every other site type. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Robot", meta=(ClampMin="0.0", ClampMax="4.0"))
    float GenericWorkRate = 0.6f;

    /** Movement speed multiplier toward its site. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Robot", meta=(ClampMin="0.1", ClampMax="4.0"))
    float MoveSpeedMultiplier = 1.0f;

    /** Chassis body tint (procedural placeholder until real meshes bind). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Robot")
    FLinearColor PrimaryTint = FLinearColor(0.62f, 0.62f, 0.66f);

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Robot")
    EAstrawildRarity Rarity = EAstrawildRarity::Common;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(FPrimaryAssetType(TEXT("Robot")), RobotId);
    }
};

/**
 * Work-site definition (Master Plan §7): the data-driven
 * Build → Power → Assign → Work → Consume → Produce loop. Bootstrapper
 * spawns sites from these definitions; input items gate production.
 */
UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildWorkSiteDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorkSite")
    FName SiteId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorkSite")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorkSite")
    EAstrawildWorkType WorkType = EAstrawildWorkType::Gathering;

    /** Item produced per completed cycle. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorkSite")
    FName OutputItemId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorkSite", meta=(ClampMin="1"))
    int32 OutputQuantity = 1;

    /** Inputs consumed per cycle (empty = harvest-from-the-land sites). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorkSite")
    TArray<FAstrawildItemStack> InputItems;

    /** Seconds of worker time per output cycle. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorkSite", meta=(ClampMin="1.0"))
    float SecondsPerOutput = 12.0f;

    /** Power-gated site: no grid power → no production (Master Plan §14). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorkSite")
    bool bRequiresPower = false;

    /** Zone the site is placed in (bootstrapper places by zone + offset). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorkSite")
    EAstrawildZone Zone = EAstrawildZone::DawnFields;

    /** Offset from the zone center (cm). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorkSite")
    FVector2D OffsetFromZoneCenter = FVector2D::ZeroVector;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(FPrimaryAssetType(TEXT("WorkSite")), SiteId);
    }
};

/**
 * World event definition (Master Plan §19): data-driven, save-safe ambient
 * world dynamism. The scheduler subsystem reads these; effects resolve
 * deterministically from this data (no per-event subclasses).
 */
UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildWorldEventDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorldEvent")
    FName EventId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorldEvent")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorldEvent", meta=(MultiLine=true))
    FText Description;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorldEvent")
    EAstrawildWorldEventKind Kind = EAstrawildWorldEventKind::ResourceSurge;

    /** Selection weight in the roll pool. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorldEvent", meta=(ClampMin="0.0"))
    float RarityWeight = 1.0f;

    /** In-world hours before this event may fire again. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorldEvent", meta=(ClampMin="0.0"))
    float CooldownGameHours = 24.0f;

    /** First day this event may appear on (progression gating). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorldEvent", meta=(ClampMin="1"))
    int32 MinDay = 1;

    /** Duration in in-world minutes (0 = instant). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorldEvent", meta=(ClampMin="0"))
    int32 DurationGameMinutes = 60;

    /** Restricts the event to one zone (None = world-wide). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorldEvent")
    EAstrawildZone Zone = EAstrawildZone::None;

    /** Only fires between 21:00 and 06:00. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorldEvent")
    bool bRequiresNight = false;

    // --- Deterministic effect payloads ---

    /** Research points granted to every player when the event resolves. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorldEvent|Effects", meta=(ClampMin="0"))
    int32 ResearchPointReward = 0;

    /** Loot table id granted at the event location (supply drop / meteor). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorldEvent|Effects")
    FName RewardLootTableId = NAME_None;

    /** Species id that spawns extra wild instances (migration / rare bloom). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorldEvent|Effects")
    FName SpeciesBoostId = NAME_None;

    /** How many extra instances the boost spawns. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorldEvent|Effects", meta=(ClampMin="0"))
    int32 SpeciesBoostCount = 0;

    /** Hostiles spawned near the player camp (night raid). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorldEvent|Effects", meta=(ClampMin="0"))
    int32 RaidHostileCount = 0;

    /** Resource nodes spawned at the location (resource surge / meteor crater). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorldEvent|Effects")
    TArray<FName> BonusNodeIds;

    /** Weather state forced while the event runs (None = leave weather alone). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorldEvent|Effects")
    EAstrawildWeatherState ForcedWeather = EAstrawildWeatherState::Clear;

    /** Event cares about weather state (display only until weather coupling lands). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|WorldEvent|Effects")
    bool bForcesWeather = false;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(FPrimaryAssetType(TEXT("WorldEvent")), EventId);
    }
};

/**
 * Point of interest definition (Master Plan §5/§31): the data Antigravity's
 * vertical-slice dressing binds to, and the discovery state the POI subsystem
 * tracks + saves.
 */
UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildPOIDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|POI")
    FName PoiId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|POI")
    FText DisplayName;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|POI", meta=(MultiLine=true))
    FText Description;

    /** One-line lore shown when discovered (field journal flavour). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|POI")
    FText LoreLine;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|POI")
    EAstrawildPOIType Type = EAstrawildPOIType::Landmark;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|POI")
    EAstrawildZone Zone = EAstrawildZone::DawnFields;

    /** Offset from the zone center (cm) — resolved at spawn. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|POI")
    FVector2D OffsetFromZoneCenter = FVector2D::ZeroVector;

    /** Discovery radius around the marker (cm). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|POI", meta=(ClampMin="100.0"))
    float DiscoveryRadius = 1200.0f;

    /** Loot granted the first time this POI is discovered. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|POI")
    FName RewardLootTableId = NAME_None;

    /** Research points granted on first discovery. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|POI", meta=(ClampMin="0"))
    int32 ResearchReward = 0;

    /** Ancient signal sources: only discoverable with an ancient-signal scanner. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|POI")
    bool bRequiresSignalScanner = false;

    /** Antigravity asset contract: prop/dressing set to place at this POI. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|POI")
    FName DressingSetId = NAME_None;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(FPrimaryAssetType(TEXT("POI")), PoiId);
    }
};

/**
 * Biome definition (Master Plan §5/§31 — Visual Vertical Slice support):
 * the binding surface between the 12 gameplay zones and Antigravity's
 * environment assets. Pure data: landscape/foliage/water/audio/sky references
 * the editor pipeline fills in — the runtime game keeps working with
 * placeholders until real assets land.
 */
UCLASS(BlueprintType)
class ASTRAWILDCORE_API UAstrawildBiomeDefinition : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:
    /** Zone this biome dresses (matches ZoneId in the zone table). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Biome")
    FName BiomeId = NAME_None;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Biome")
    FText DisplayName;

    /** Art direction notes for the vertical slice (one-liner). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Biome")
    FText ArtDirection;

    /** Zone gameplay data mirrored for asset binding convenience. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Biome")
    EAstrawildZone Zone = EAstrawildZone::DawnFields;

    /** Starting biome — the P1 vertical slice lands here first. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Biome")
    bool bStartingBiome = false;

    // --- Antigravity asset contract (soft refs — bind in editor, runtime falls back) ---

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Biome|Assets")
    TSoftObjectPtr<UMaterialInterface> LandscapeMaterial;

    /** Foliage types to scatter (grass ground cover). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Biome|Assets")
    TArray<TSoftObjectPtr<UStaticMesh>> GrassMeshes;

    /** Trees / large foliage. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Biome|Assets")
    TArray<TSoftObjectPtr<UStaticMesh>> TreeMeshes;

    /** Rock / cliff dressing meshes. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Biome|Assets")
    TArray<TSoftObjectPtr<UStaticMesh>> RockMeshes;

    /** Ambient audio loop (biome soundscape). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Biome|Assets")
    TSoftObjectPtr<USoundBase> AmbientAudio;

    // --- Production V2 Batch 2: placeholder dressing tints (Visual Vertical Slice §31) ---
    // White (default) = derive at runtime from the zone table's GroundTint so
    // every biome reads correctly with zero data; explicit values override.

    /** Tree canopy tint (White = derived from the zone ground tint). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Biome|Dressing")
    FLinearColor TreeCanopyTint = FLinearColor::White;

    /** Rock / boulder tint (White = derived). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Biome|Dressing")
    FLinearColor RockTint = FLinearColor::White;

    /** Grass tuft tint (White = derived). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Biome|Dressing")
    FLinearColor GrassTuftTint = FLinearColor::White;

    /** Multiplier on the zone dressing budget (0 removes that biome's scatter). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Biome|Dressing", meta=(ClampMin="0.0", ClampMax="3.0"))
    float DressingDensity = 1.0f;

    /**
     * When false, the runtime placeholder meshes never spawn — only real
     * Tree/Rock/GrassMeshes instances render (for editor-shot purity once
     * assets land). True by default: the game must look dressed with zero assets.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Biome|Dressing")
    bool bEnablePlaceholderDressing = true;

    /** Gameplay anchoring: which resource node defs spawn in this biome. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Biome|Gameplay")
    TArray<FName> ResourceNodeIds;

    /** Signature species for spawn weighting (bestiary ids). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Biome|Gameplay")
    TArray<FName> SignatureSpeciesIds;

    /** POIs anchored in this biome. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Biome|Gameplay")
    TArray<FName> PoiIds;

    virtual FPrimaryAssetId GetPrimaryAssetId() const override
    {
        return FPrimaryAssetId(FPrimaryAssetType(TEXT("Biome")), BiomeId);
    }
};
