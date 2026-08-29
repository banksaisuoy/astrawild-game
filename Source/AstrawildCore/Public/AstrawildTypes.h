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
    Dead UMETA(DisplayName="Dead")
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
    SurviveTime UMETA(DisplayName="Survive Time")
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Save")
    FName OwnerPlayerId = NAME_None;
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

