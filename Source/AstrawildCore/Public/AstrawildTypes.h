#pragma once

#include "CoreMinimal.h"
#include "AstrawildTypes.generated.h"

UENUM(BlueprintType)
enum class EAstrawildElementType : uint8
{
    None UMETA(DisplayName="None"),
    Light UMETA(DisplayName="Light"),
    Ash UMETA(DisplayName="Ash"),
    Flora UMETA(DisplayName="Flora"),
    Frost UMETA(DisplayName="Frost"),
    Pulse UMETA(DisplayName="Pulse"),
    Ember UMETA(DisplayName="Ember")
};

UENUM(BlueprintType)
enum class EAstrawildEchoRole : uint8
{
    Explorer UMETA(DisplayName="Explorer"),
    Combat UMETA(DisplayName="Combat"),
    Base UMETA(DisplayName="Base"),
    Support UMETA(DisplayName="Support")
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildStableId
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Identity")
    FName Value = NAME_None;

    FAstrawildStableId() = default;
    explicit FAstrawildStableId(const FName InValue) : Value(InValue) {}

    bool IsValid() const { return !Value.IsNone(); }
    friend bool operator==(const FAstrawildStableId& A, const FAstrawildStableId& B)
    {
        return A.Value == B.Value;
    }
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildItemStack
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Inventory")
    FName ItemId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Inventory", meta=(ClampMin="0"))
    int32 Quantity = 0;

    bool IsValid() const
    {
        return !ItemId.IsNone() && Quantity > 0;
    }
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildEchoStats
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo", meta=(ClampMin="1.0"))
    float MaxHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo", meta=(ClampMin="0.0"))
    float AttackPower = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo", meta=(ClampMin="0.0"))
    float Defense = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo", meta=(ClampMin="0.0"))
    float MoveSpeed = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo", meta=(ClampMin="0.0"))
    float Stamina = 100.0f;

    /** 0 = very easy to capture, 1 = almost impossible. Scales the weaken bonus. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo", meta=(ClampMin="0.0", ClampMax="1.0"))
    float CaptureResilience = 0.35f;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildEchoInstanceSaveData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FGuid InstanceId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FName DefinitionId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save", meta=(ClampMin="1"))
    int32 Level = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save", meta=(ClampMin="0.0"))
    float Trust = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save", meta=(ClampMin="0"))
    int32 Experience = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    bool bInRoster = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FTransform LastKnownTransform;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildRestPointSaveData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FGuid WorldObjectId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FTransform Transform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    bool bActive = false;
};

// ============================================================================
// V2 TYPES — personality, needs, weather, time, work, quests, power, save v2
// (additive; all v1 structs above remain valid for save migration v1→v2)
// ============================================================================

/** Creature personality archetypes (directive §5). Drives real behavior differences. */
UENUM(BlueprintType)
enum class EAstrawildPersonality : uint8
{
    Brave UMETA(DisplayName="Brave", ToolTip="Stands ground longer, higher aggression threshold before flee"),
    Timid UMETA(DisplayName="Timid", ToolTip="Flees earlier, harder to approach, flees farther"),
    Aggressive UMETA(DisplayName="Aggressive", ToolTip="Attacks on sight at shorter range"),
    Curious UMETA(DisplayName="Curious", ToolTip="Investigates unknown actors, easier observation"),
    Loyal UMETA(DisplayName="Loyal", ToolTip="Commands answered faster, trust grows faster"),
    Lazy UMETA(DisplayName="Lazy", ToolTip="Slower work rate, longer sleep"),
    Energetic UMETA(DisplayName="Energetic", ToolTip="Faster movement & work, drains energy quicker"),
    Protective UMETA(DisplayName="Protective", ToolTip="Prioritizes defending owner over self"),
    Independent UMETA(DisplayName="Independent", ToolTip="Wanders farther, ignores some commands"),
    Social UMETA(DisplayName="Social", ToolTip="Seeks herd, buffs nearby allies mood")
};

/** Diurnal / nocturnal / crepuscular activity gating (directive §13). */
UENUM(BlueprintType)
enum class EAstrawildActivityPattern : uint8
{
    Diurnal UMETA(DisplayName="Diurnal"),
    Nocturnal UMETA(DisplayName="Nocturnal"),
    Crepuscular UMETA(DisplayName="Crepuscular", ToolTip="Active at dawn and dusk")
};

/** Dynamic weather states (directive §12). */
UENUM(BlueprintType)
enum class EAstrawildWeatherState : uint8
{
    Clear UMETA(DisplayName="Clear"),
    Cloudy UMETA(DisplayName="Cloudy"),
    Rain UMETA(DisplayName="Rain"),
    HeavyRain UMETA(DisplayName="Heavy Rain"),
    Storm UMETA(DisplayName="Storm"),
    Fog UMETA(DisplayName="Fog"),
    Heat UMETA(DisplayName="Heat"),
    Cold UMETA(DisplayName="Cold")
};

/**
 * World zones of the Shattered Vale (Batch 7 — directive §21/M-13; Batch 8 expands
 * the grid from 3x2 to 4x3): twelve rectangular regions tiling the 3.2km x 2.4km
 * surface world. Zone lookup is a pure static (see UAstrawildZoneSubsystem) so HUD
 * clients resolve locally without replication. Entries are appended-only —
 * existing saves serialize zone discovery by enum value.
 */
UENUM(BlueprintType)
enum class EAstrawildZone : uint8
{
    None UMETA(DisplayName="Wilderness"),
    DawnFields UMETA(DisplayName="Dawn Fields"),
    DuskMarsh UMETA(DisplayName="Dusk Marsh"),
    EmberRidge UMETA(DisplayName="Ember Ridge"),
    FrostveilExpanse UMETA(DisplayName="Frostveil Expanse"),
    Glimmerwood UMETA(DisplayName="Glimmerwood"),
    HollowApproach UMETA(DisplayName="Hollow Approach"),
    // --- Batch 8 "The Grand Expanse" (appended-only, save-safe) ---
    AzureShallows UMETA(DisplayName="Azure Shallows"),
    TidebreakerIsles UMETA(DisplayName="Tidebreaker Isles"),
    SunscarDesert UMETA(DisplayName="Sunscar Desert"),
    StormcrestHighlands UMETA(DisplayName="Stormcrest Highlands"),
    VerdantReach UMETA(DisplayName="Verdant Reach"),
    PearlseaReef UMETA(DisplayName="Pearlsea Reef"),
    Count UMETA(Hidden)
};

/**
 * Batch 8 — bestiary family: the broad creature lineage a species belongs to.
 * Drives silhouette family, loot flavor and work affinities across the 200+
 * species roster (Docs/ASTRAWILD_BESTIARY_CODEX.md).
 */
UENUM(BlueprintType)
enum class EAstrawildEchoFamily : uint8
{
    Beast UMETA(DisplayName="Beast"),
    Dragon UMETA(DisplayName="Dragon"),
    Construct UMETA(DisplayName="Construct"),
    Spirit UMETA(DisplayName="Spirit"),
    Elemental UMETA(DisplayName="Elemental"),
    Aquatic UMETA(DisplayName="Aquatic"),
    Insectoid UMETA(DisplayName="Insectoid"),
    Flora UMETA(DisplayName="Flora Kindred"),
    Avian UMETA(DisplayName="Avian"),
    Ancient UMETA(DisplayName="Ancient")
};

/**
 * Batch 8 — body plan: which procedural silhouette kit the runtime body builder
 * assembles for the species (AAstrawildEchoCharacter::BuildProceduralBody).
 */
UENUM(BlueprintType)
enum class EAstrawildBodyPlan : uint8
{
    Quadruped UMETA(DisplayName="Quadruped"),
    Biped UMETA(DisplayName="Biped"),
    Serpent UMETA(DisplayName="Serpent"),
    Floating UMETA(DisplayName="Floating"),
    Insectoid UMETA(DisplayName="Insectoid"),
    Avian UMETA(DisplayName="Avian"),
    Crystalline UMETA(DisplayName="Crystalline"),
    Amorphous UMETA(DisplayName="Amorphous")
};

/** Batch 8 — size class: scales the procedural body and the base stat budget. */
UENUM(BlueprintType)
enum class EAstrawildSizeClass : uint8
{
    Tiny UMETA(DisplayName="Tiny"),
    Small UMETA(DisplayName="Small"),
    Medium UMETA(DisplayName="Medium"),
    Large UMETA(DisplayName="Large"),
    Huge UMETA(DisplayName="Huge")
};

/** Batch 8 — NPC role: drives village behaviour (patrol, shop, guard duty, quests). */
UENUM(BlueprintType)
enum class EAstrawildNPCRole : uint8
{
    Villager UMETA(DisplayName="Villager"),
    Vendor UMETA(DisplayName="Vendor"),
    Guard UMETA(DisplayName="Guard"),
    QuestGiver UMETA(DisplayName="Quest Giver"),
    Elder UMETA(DisplayName="Elder")
};

/** Player-issued Echo commands (directive §10). */
UENUM(BlueprintType)
enum class EAstrawildEchoCommand : uint8
{
    Follow UMETA(DisplayName="Follow"),
    Attack UMETA(DisplayName="Attack My Target"),
    Defend UMETA(DisplayName="Defend Me"),
    Stay UMETA(DisplayName="Stay"),
    Retreat UMETA(DisplayName="Retreat"),
    HoldPosition UMETA(DisplayName="Hold Position"),
    Work UMETA(DisplayName="Go Work")
};

/** Echo AI states (directive §6). Mirrored as gameplay tags State.Creature.* */
UENUM(BlueprintType)
enum class EAstrawildEchoAIState : uint8
{
    Idle UMETA(DisplayName="Idle"),
    Explore UMETA(DisplayName="Explore"),
    SearchFood UMETA(DisplayName="Search Food"),
    Eat UMETA(DisplayName="Eat"),
    Sleep UMETA(DisplayName="Sleep"),
    Socialize UMETA(DisplayName="Socialize"),
    Investigate UMETA(DisplayName="Investigate"),
    Flee UMETA(DisplayName="Flee"),
    Alert UMETA(DisplayName="Alert"),
    Combat UMETA(DisplayName="Combat"),
    Protect UMETA(DisplayName="Protect"),
    Follow UMETA(DisplayName="Follow"),
    Work UMETA(DisplayName="Work"),
    ReturnHome UMETA(DisplayName="Return Home"),
    Injured UMETA(DisplayName="Injured"),
    Dead UMETA(DisplayName="Dead"),
    // Batch 3 — Item B: appended-only (serialization-safe for existing saves).
    Staggered UMETA(DisplayName="Staggered")
};

/**
 * Batch 4 — M-11: vendor transaction outcome. Returned by the NPC purchase/sell
 * API so HUD/cheat callers can surface an actionable message per failure mode.
 */
UENUM(BlueprintType)
enum class EAstrawildVendorResult : uint8
{
    Success UMETA(DisplayName="Success"),
    /** The NPC has no shop (no ShopLootTableId/CurrencyItemId on its definition). */
    NotAVendor UMETA(DisplayName="Not A Vendor"),
    /** The item is not among this vendor's wares (or has no vendor value when selling). */
    NotAWare UMETA(DisplayName="Not A Ware"),
    /** The purchaser cannot afford the total price in the vendor's currency. */
    NotEnoughCurrency UMETA(DisplayName="Not Enough Currency"),
    /** Adding the purchase would exceed the inventory weight cap. */
    TooHeavy UMETA(DisplayName="Too Heavy"),
    /** The purchaser is outside the vendor's trade range. */
    TooFarAway UMETA(DisplayName="Too Far Away"),
    /** Malformed request (bad quantity, missing components, called on a client). */
    InvalidRequest UMETA(DisplayName="Invalid Request")
};

/** Base work types for Echoes (directive §18). */
UENUM(BlueprintType)
enum class EAstrawildWorkType : uint8
{
    None UMETA(DisplayName="None"),
    Gathering UMETA(DisplayName="Gathering"),
    Farming UMETA(DisplayName="Farming"),
    Mining UMETA(DisplayName="Mining"),
    Transport UMETA(DisplayName="Transport"),
    Cooking UMETA(DisplayName="Cooking"),
    Crafting UMETA(DisplayName="Crafting Assistance"),
    ResearchAssist UMETA(DisplayName="Research Assistance"),
    PowerGeneration UMETA(DisplayName="Power Generation"),
    Defense UMETA(DisplayName="Defense"),
    Construction UMETA(DisplayName="Construction")
};

/** Quest objective types (directive §25). Event-driven progression. */
UENUM(BlueprintType)
enum class EAstrawildQuestObjectiveType : uint8
{
    ReachLocation UMETA(DisplayName="Reach Location"),
    CollectItem UMETA(DisplayName="Collect Item"),
    CaptureEcho UMETA(DisplayName="Capture Echo"),
    DefeatCreature UMETA(DisplayName="Defeat Creature"),
    CraftRecipe UMETA(DisplayName="Craft Recipe"),
    PlaceBuilding UMETA(DisplayName="Place Building"),
    UnlockTechnology UMETA(DisplayName="Unlock Technology"),
    ObserveEcho UMETA(DisplayName="Observe Echo"),
    SurviveTime UMETA(DisplayName="Survive Time"),
    // Final production run (appended — serialization-safe for existing saves):
    // consumes Event.ZoneEntered (published by the zone subsystem since Batch 7).
    VisitZone UMETA(DisplayName="Visit Zone"),
    // Production V2 (appended — serialization-safe): consumes Event.PoiDiscovered
    // published by the POI subsystem when a point of interest is first discovered.
    DiscoverPOI UMETA(DisplayName="Discover Point of Interest")
};

/** Crafting / research eras (directive §19). */
UENUM(BlueprintType)
enum class EAstrawildTechEra : uint8
{
    Primitive UMETA(DisplayName="Primitive"),
    Mechanical UMETA(DisplayName="Mechanical"),
    Electrical UMETA(DisplayName="Electrical"),
    AdvancedEnergy UMETA(DisplayName="Advanced Energy"),
    Ancient UMETA(DisplayName="Ancient Technology"),
    Eco UMETA(DisplayName="Eco Technology")
};

/** Building categories for modular snap system (directive §16). */
UENUM(BlueprintType)
enum class EAstrawildBuildingCategory : uint8
{
    Foundation UMETA(DisplayName="Foundation"),
    Wall UMETA(DisplayName="Wall"),
    Floor UMETA(DisplayName="Floor"),
    Roof UMETA(DisplayName="Roof"),
    Door UMETA(DisplayName="Door"),
    Storage UMETA(DisplayName="Storage"),
    Workstation UMETA(DisplayName="Workstation"),
    Farm UMETA(DisplayName="Farm"),
    Power UMETA(DisplayName="Power"),
    Defense UMETA(DisplayName="Defense"),
    CreatureHousing UMETA(DisplayName="Creature Housing"),
    Research UMETA(DisplayName="Research"),
    Decoration UMETA(DisplayName="Decoration")
};

/** Power node roles (directive §17). */
UENUM(BlueprintType)
enum class EAstrawildPowerRole : uint8
{
    Generator UMETA(DisplayName="Generator"),
    Battery UMETA(DisplayName="Battery"),
    Consumer UMETA(DisplayName="Consumer")
};

/** Echo needs — drives AI & work efficiency (directive §4/§5). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildEchoNeeds
{
    GENERATED_BODY()

    /** 0..100, lower = hungrier. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo|Needs", meta=(ClampMin="0.0", ClampMax="100.0"))
    float Hunger = 100.0f;

    /** 0..100, lower = more tired. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo|Needs", meta=(ClampMin="0.0", ClampMax="100.0"))
    float Energy = 100.0f;

    /** 0..100, lower = unhappy (affects work + growth). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo|Needs", meta=(ClampMin="0.0", ClampMax="100.0"))
    float Mood = 80.0f;

    bool IsCritical() const
    {
        return Hunger <= 15.0f || Energy <= 10.0f;
    }
};

/** Per-work-type affinity of a species (directive §18). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildWorkAffinity
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo|Work")
    EAstrawildWorkType WorkType = EAstrawildWorkType::None;

    /** 0..2; 1 = baseline, 2 = twice as effective. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo|Work", meta=(ClampMin="0.0", ClampMax="2.0"))
    float Affinity = 1.0f;
};

/** Player survival vitals (directive §11). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildSurvivalStats
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival", meta=(ClampMin="0.0"))
    float Health = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival", meta=(ClampMin="0.0"))
    float MaxHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival", meta=(ClampMin="0.0"))
    float Stamina = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival", meta=(ClampMin="0.0"))
    float MaxStamina = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival", meta=(ClampMin="0.0", ClampMax="100.0"))
    float Hunger = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival", meta=(ClampMin="0.0", ClampMax="100.0"))
    float Thirst = 100.0f;

    /** World temperature as felt by the player, Celsius. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival")
    float Temperature = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival")
    bool bIsDead = false;
};

/** A runtime status effect instance (directive §11/§12). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildStatusEffect
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Status")
    FName StatusId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Status", meta=(ClampMin="0.0"))
    float RemainingSeconds = 0.0f;

    /** Optional periodic damage tick. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Status", meta=(ClampMin="0.0"))
    float DamagePerSecond = 0.0f;

    /** Optional movement speed multiplier while active. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Status", meta=(ClampMin="0.0"))
    float SpeedMultiplier = 1.0f;
};

/** Full Echo instance state (roster + runtime). Extends v1 save struct. */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildEchoInstanceV2
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo")
    FGuid InstanceId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo")
    FName DefinitionId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo")
    EAstrawildPersonality Personality = EAstrawildPersonality::Curious;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo", meta=(ClampMin="1"))
    int32 Level = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo", meta=(ClampMin="0.0"))
    float Experience = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo", meta=(ClampMin="0.0"))
    float Trust = 0.0f;

    /** 0..100 — deepens with time, feeding and shared combat (directive §4 Relationship). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo", meta=(ClampMin="0.0", ClampMax="100.0"))
    float Bond = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo")
    FAstrawildEchoNeeds Needs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo")
    FTransform LastKnownTransform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo")
    bool bInParty = false;
};

/** Quest objective definition + runtime progress (directive §25). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildQuestObjective
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Quest")
    EAstrawildQuestObjectiveType Type = EAstrawildQuestObjectiveType::CollectItem;

    /** Target id: item id, recipe id, tech id, echo definition id, location name... */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Quest")
    FName TargetId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Quest", meta=(ClampMin="1"))
    int32 RequiredCount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Quest")
    FText ObjectiveText;

    /** Runtime progress — not saved in definition data assets. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Quest")
    int32 ProgressCount = 0;

    bool IsComplete() const { return ProgressCount >= RequiredCount; }
};

/** Save data for one building instance (directive §16/§27). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildBuildingSaveData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FGuid BuildingId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FName DefinitionId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FTransform Transform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    float CurrentHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    float StoredCharge = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    bool bIsSwitchedOn = true;

    /**
     * Batch 2 — Item C: snapshot of the building's last resolved power state at save
     * time. Additive — older saves deserialize with false and re-resolve on the first
     * power grid tick (≤2s), so missing data does not corrupt a save.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    bool bIsPowered = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FName OwnerPlayerId = NAME_None;
};

/**
 * Dungeon progression snapshot (Batch 6 — gap M-7): reloads previously resurrected
 * cleared encounters. Cleared rooms stay cleared; in-progress rooms respawn fresh
 * (documented regeneration policy — see Docs/ASTRAWILD_DUNGEON_BOSS.md).
 */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildDungeonSaveData
{
    GENERATED_BODY()

    /** Stable dungeon id (matches the generator's DungeonId). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FName DungeonId = NAME_None;

    /** Cleared room indices (the entry room is always index 0). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<int32> ClearedRoomIndices;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save", meta=(ClampMin="0"))
    int32 RoomsCleared = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save", meta=(ClampMin="0"))
    int32 TotalRooms = 0;

    /** True when every room (including the boss room) is cleared. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    bool bCompleted = false;
};

/** Zone discovery state (Batch 7 — The Shattered Vale, additive like the dungeon payload). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildZoneSaveData
{
    GENERATED_BODY()

    /** Zones the players have set foot in — drives the HUD discovery counter. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<EAstrawildZone> DiscoveredZones;
};

/** World state saved with the game (directive §27). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildWorldSaveData
{
    GENERATED_BODY()

    /** Minutes of in-world time since campaign start. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    int32 ElapsedWorldMinutes = 8 * 60;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    int32 DayNumber = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    EAstrawildWeatherState Weather = EAstrawildWeatherState::Clear;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    int32 Seed = 0;
};

/** Research state (directive §19). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildResearchSaveData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FName> UnlockedTechIds;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save", meta=(ClampMin="0"))
    int32 ResearchPoints = 0;
};

/** Quest runtime state (directive §25). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildQuestSaveData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FName QuestId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FAstrawildQuestObjective> Objectives;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    bool bActive = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    bool bCompleted = false;
};

/** Field journal knowledge entry (directive §20). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildJournalEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Journal")
    FName EchoDefinitionId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Journal")
    bool bScanned = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Journal")
    bool bFoodDiscovered = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Journal")
    bool bHabitatDiscovered = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Journal")
    bool bWeaknessDiscovered = false;

    /** Observation progress 0..100 — full observation unlocks capture bonus. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Journal", meta=(ClampMin="0.0", ClampMax="100.0"))
    float ObservationProgress = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Journal", meta=(ClampMin="0"))
    int32 TimesEncountered = 0;
};

// ============================================================================
// FINAL PRODUCTION RUN TYPES — advanced equipment, robotics, save schema v3
// (additive; appended-only so every existing save keeps deserializing)
// ============================================================================

/**
 * Explicit equipment slot routing (final production run PHASE 12). Auto keeps the
 * legacy stat-based routing (AttackPower→weapon, BlockMitigation→shield,
 * ArmorRating→torso); new advanced items declare their slot explicitly.
 */
UENUM(BlueprintType)
enum class EAstrawildEquipmentSlot : uint8
{
    Auto UMETA(DisplayName="Auto (stat-routed)"),
    Weapon UMETA(DisplayName="Weapon"),
    Shield UMETA(DisplayName="Shield"),
    Torso UMETA(DisplayName="Torso Armor"),
    Helmet UMETA(DisplayName="Helmet"),
    Exosuit UMETA(DisplayName="Exosuit"),
    Scanner UMETA(DisplayName="Scanner")
};

/** Work-site snapshot (save schema v3 — closes the automation persistence gap). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildWorkSiteSaveData
{
    GENERATED_BODY()

    /** Stable site id (bootstrapper-placed sites use fixed ids; player-built use guids). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FName SiteId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    EAstrawildWorkType WorkType = EAstrawildWorkType::Gathering;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FName OutputItemId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FTransform Transform;

    /** Output accumulated but not yet collected. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save", meta=(ClampMin="0"))
    int32 StoredOutput = 0;

    /** Roster instance ids of the Echoes assigned at save time. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FGuid> AssignedEchoInstanceIds;

    /** True when a utility robot mans this site. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    bool bHasRobot = false;

    // --- Production V2 (additive, save schema v4): consume->produce loop state ---

    /** Input buffer staged for the next production cycle (definition-driven sites). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FAstrawildItemStack> InputBuffer;

    /** Output produced per completed work cycle (definition default 1). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save", meta=(ClampMin="1"))
    int32 OutputQuantity = 1;
};

/** Utility drone snapshot (save schema v3). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildDroneSaveData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FName OwnerPlayerId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FTransform Transform;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    bool bDeployed = false;

    // --- Production V2 (additive, save schema v4): drone battery state ---

    /** Battery seconds remaining at save time (modules extend the capacity). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save", meta=(ClampMin="0.0"))
    float BatteryRemainingSeconds = 0.0f;
};

/** Utility robot snapshot (save schema v3). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildRobotSaveData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FName OwnerPlayerId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FTransform Transform;

    /** Site id the robot was working at save time. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FName AssignedSiteId = NAME_None;

    // --- Production V2 (additive, save schema v4): robot specialization ---

    /** Robot definition id (specialized chassis: mining/farming/defense). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FName RobotDefinitionId = NAME_None;
};

/** Power-grid snapshot (save schema v3 — battery charge finally persists). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildPowerGridSaveData
{
    GENERATED_BODY()

    /** Aggregate energy buffered in the grid's batteries (power-seconds). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save", meta=(ClampMin="0.0"))
    float StoredEnergy = 0.0f;
};


// ============================================================================
// PRODUCTION V2 TYPES — data-driven content foundation (Master Plan §3-19)
// (additive; appended-only so every existing save keeps deserializing)
// ============================================================================

/** Content rarity ladder shared by items, Echoes, weapons and events (Master Plan §6). */
UENUM(BlueprintType)
enum class EAstrawildRarity : uint8
{
    Common UMETA(DisplayName="Common"),
    Uncommon UMETA(DisplayName="Uncommon"),
    Rare UMETA(DisplayName="Rare"),
    Epic UMETA(DisplayName="Epic"),
    Legendary UMETA(DisplayName="Legendary"),
    Mythic UMETA(DisplayName="Mythic")
};

/** Weapon family — each maps to a distinct firing archetype (Master Plan §8). */
UENUM(BlueprintType)
enum class EAstrawildWeaponFamily : uint8
{
    Kinetic UMETA(DisplayName="Kinetic"),
    Pulse UMETA(DisplayName="Pulse"),
    Plasma UMETA(DisplayName="Plasma"),
    Laser UMETA(DisplayName="Laser"),
    Arc UMETA(DisplayName="Arc"),
    Rail UMETA(DisplayName="Rail"),
    Missile UMETA(DisplayName="Missile"),
    Experimental UMETA(DisplayName="Experimental")
};

/** How a weapon delivers damage — drives the combat component's execution branch. */
UENUM(BlueprintType)
enum class EAstrawildWeaponFireMode : uint8
{
    Projectile UMETA(DisplayName="Projectile"),
    HomingProjectile UMETA(DisplayName="Homing Projectile"),
    Beam UMETA(DisplayName="Beam (hitscan, pierce)"),
    ArcChain UMETA(DisplayName="Arc (hitscan, chains)")
};

/** Equipment technology tier — armor sets and weapons share the ladder (Master Plan §9). */
UENUM(BlueprintType)
enum class EAstrawildTechTier : uint8
{
    Field UMETA(DisplayName="Field Grade"),
    Mk1 UMETA(DisplayName="Mk I"),
    Mk2 UMETA(DisplayName="Mk II"),
    Mk3 UMETA(DisplayName="Mk III"),
    Experimental UMETA(DisplayName="Experimental")
};

/** Echo passive trait — permanent aura while the Echo is in the active party (Master Plan §6). */
UENUM(BlueprintType)
enum class EAstrawildEchoPassive : uint8
{
    None UMETA(DisplayName="None"),
    PartyHeal UMETA(DisplayName="Mending Aura (party heal)"),
    PlayerStamina UMETA(DisplayName="Rhythm Aura (player stamina regen)"),
    CarryBoost UMETA(DisplayName="Pack Instinct (carry weight)"),
    ThreatDampener UMETA(DisplayName="Calm Presence (reduces wild aggro)")
};

/** Research tree branch — display/progression grouping (Master Plan §16). */
UENUM(BlueprintType)
enum class EAstrawildResearchBranch : uint8
{
    Survival UMETA(DisplayName="Survival"),
    Tools UMETA(DisplayName="Tools"),
    Weapons UMETA(DisplayName="Weapons"),
    Armor UMETA(DisplayName="Armor"),
    Energy UMETA(DisplayName="Energy"),
    Automation UMETA(DisplayName="Automation"),
    Scanner UMETA(DisplayName="Scanner"),
    EchoTech UMETA(DisplayName="Echo Technology"),
    Exploration UMETA(DisplayName="Exploration")
};

/** Point-of-interest archetype (Master Plan §5 world production). */
UENUM(BlueprintType)
enum class EAstrawildPOIType : uint8
{
    Landmark UMETA(DisplayName="Landmark"),
    AncientTech UMETA(DisplayName="Ancient Technology"),
    Ruin UMETA(DisplayName="Starter Ruin"),
    CaveEntrance UMETA(DisplayName="Cave Entrance"),
    Watchtower UMETA(DisplayName="Watchtower"),
    SignalSource UMETA(DisplayName="Ancient Signal Source")
};

/** World-event archetype — deterministic effects live in the definition data (Master Plan §19). */
UENUM(BlueprintType)
enum class EAstrawildWorldEventKind : uint8
{
    StormSurge UMETA(DisplayName="Storm Surge"),
    Migration UMETA(DisplayName="Creature Migration"),
    ResourceSurge UMETA(DisplayName="Resource Surge"),
    SupplyDrop UMETA(DisplayName="Supply Drop"),
    AncientSignal UMETA(DisplayName="Ancient Signal"),
    NightRaid UMETA(DisplayName="Night Raid"),
    MeteorFall UMETA(DisplayName="Meteor Fall"),
    RareEchoBloom UMETA(DisplayName="Rare Echo Bloom"),
    BossStirring UMETA(DisplayName="Boss Stirring")
};

// --- Save schema v4 extensions (additive) ---

/** Active/completed world-event runtime snapshot (save schema v4). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildWorldEventSaveData
{
    GENERATED_BODY()

    /** Definition id of the event. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FName EventId = NAME_None;

    /** In-world minute the event ends (0 = not time-limited). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save", meta=(ClampMin="0"))
    int32 EndAbsoluteMinute = 0;

    /** Zone the event is anchored to (None = world-wide). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    EAstrawildZone Zone = EAstrawildZone::None;

    /** Location payload (supply crate / meteor crater / signal marker). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FVector Location = FVector::ZeroVector;
};

/** World-event scheduler snapshot (save schema v4). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildWorldEventScheduleSaveData
{
    GENERATED_BODY()

    /** Events currently running. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    TArray<FAstrawildWorldEventSaveData> ActiveEvents;

    /** Absolute in-world minute of the next scheduled roll (deterministic resumption). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save", meta=(ClampMin="0"))
    int32 NextRollAbsoluteMinute = 0;

    /** Per-event cooldown end (absolute in-world minute); pruned as they expire. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    TMap<FName, int32> CooldownEndMinutes;
};
