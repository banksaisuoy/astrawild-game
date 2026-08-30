#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AstrawildTypes.h"
#include "AstrawildEchoCharacter.generated.h"

class UAstrawildEchoDefinition;
class AAstrawildEchoCharacter;
class AAstrawildWorkSiteActor;
class UStaticMeshComponent;
class UNavigationInvokerComponent;
class UProceduralMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildEchoCaptured, AAstrawildEchoCharacter*, Echo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildEchoDamaged, AAstrawildEchoCharacter*, Echo, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildEchoDefeated, AAstrawildEchoCharacter*, Echo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildEchoLevelUp, AAstrawildEchoCharacter*, Echo, int32, NewLevel);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildEchoCommandReceived, AAstrawildEchoCharacter*, Echo, EAstrawildEchoCommand, Command);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildEchoAIStateChanged, AAstrawildEchoCharacter*, Echo, EAstrawildEchoAIState, NewState);

/**
 * Echo — the creature heart of ASTRAWILD (directive §4).
 * One class serves both wild and captured instances; behaviour is driven by
 * definition (species template) + instance state (personality, needs, bond, command).
 * Combat/needs/growth simulate server-side; UI reads replicated state.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildEchoCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AAstrawildEchoCharacter();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Echo")
    TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

    /**
     * Batch 8 — procedural species body (The Grand Menagerie). Assembles a
     * vertex-colored ProceduralMesh silhouette from the definition's body plan /
     * size class / tints (same DebugMeshMaterial trick as the terrain tiles), so
     * all 210+ species read as distinct creatures with zero art assets. The
     * legacy placeholder sphere is hidden once a body is built.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Echo|Appearance")
    TObjectPtr<UProceduralMeshComponent> BodyMesh;

    /** Builds BodyMesh from EchoDefinition appearance fields (safe no-op without a definition). */
    void BuildProceduralBody();

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Echo")
    FAstrawildEchoCaptured OnCaptured;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Echo")
    FAstrawildEchoDamaged OnDamaged;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Echo")
    FAstrawildEchoDefeated OnDefeated;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Echo")
    FAstrawildEchoLevelUp OnLevelUp;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Echo")
    FAstrawildEchoCommandReceived OnCommandReceived;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Echo")
    FAstrawildEchoAIStateChanged OnAIStateChanged;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo")
    TObjectPtr<UAstrawildEchoDefinition> EchoDefinition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo", meta=(ClampMin="1"))
    int32 Level = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Echo")
    FGuid InstanceId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Echo", meta=(ClampMin="0.0"))
    float Trust = 0.0f;

    /** 0..100 bond with its owner (directive §4 Relationship). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Echo", meta=(ClampMin="0.0", ClampMax="100.0"))
    float Bond = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Echo")
    float CurrentHealth = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Echo")
    bool bCaptured = false;

    /** Instance personality — rolled on spawn from species dominant personality (directive §5). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Echo", Replicated)
    EAstrawildPersonality Personality = EAstrawildPersonality::Curious;

    /** Runtime needs — decay server-side, drive AI (directive §4/§11). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Echo", Replicated)
    FAstrawildEchoNeeds Needs;

    /** Accumulated growth experience (directive §4 Growth). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Echo", Replicated)
    float Experience = 0.0f;

    /** Current AI state (replicated for client-side feedback, e.g. nameplate icon). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Echo", Replicated)
    EAstrawildEchoAIState CurrentAIState = EAstrawildEchoAIState::Idle;

    /** Active player command (replicated). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Echo", Replicated)
    EAstrawildEchoCommand ActiveCommand = EAstrawildEchoCommand::Follow;

    /** Owning player id once captured (multiplayer ownership, directive §28). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Echo", Replicated)
    FName OwnerPlayerId = NAME_None;

    /**
     * Batch 3 — Item A: active status effects (Burn/Chill/Poison/Shock). Server ticks
     * (DoT + expiry + speed multiplier), replicated so clients can render feedback.
     * Deliberately NOT persisted — transient combat state (restores clear it).
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Echo", Replicated)
    TArray<FAstrawildStatusEffect> StatusEffects;

    /** Assigned work site for base jobs (directive §18). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Echo")
    TWeakObjectPtr<AAstrawildWorkSiteActor> AssignedWorkSite;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo")
    bool InitializeFromDefinition(UAstrawildEchoDefinition* InDefinition, const FGuid& OptionalInstanceId = FGuid());

    /** v2 init with explicit personality override. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo")
    bool InitializeFromDefinitionWithPersonality(UAstrawildEchoDefinition* InDefinition, EAstrawildPersonality InPersonality, const FGuid& OptionalInstanceId = FGuid());

    /** Physical damage (defense-mitigated). Returns applied damage. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo")
    bool ApplyDamage(float DamageAmount);

    /** Elemental damage pipeline — weakness x1.5, own element resisted (directive §9). Returns applied damage. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo")
    float ApplyElementalDamage(float DamageAmount, EAstrawildElementType InElement);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo")
    bool Capture(float InitialTrust = 0.0f);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo")
    void AddTrust(float Amount);

    /** Feeding path — preferred food multiplies trust/bond gain (directive §8). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo")
    float Feed(FName FoodItemId, float FeedValue);

    /** Growth (directive §4): XP gain with level scaling. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo")
    void AddExperience(float Amount);

    /** Issue a player command (server). Personality gates obedience (directive §5/§10). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo")
    bool IssueCommand(EAstrawildEchoCommand Command);

    /**
     * Production V2: true when the player has a captured, healthy party Echo with
     * the given passive within Radius (static query — AI perception + inventory).
     */
    static bool HasPlayerPartyPassive(const class UWorld* World, const class AActor* Player,
        EAstrawildEchoPassive Passive, float Radius = 1500.0f);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo")
    bool IsDefeated() const { return CurrentHealth <= 0.0f; }

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo")
    float GetHealthFraction() const;

    /** Max health including level scaling. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo")
    float GetMaxHealth() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo")
    float GetAttackPower() const;

    /**
     * Capture chance in 0..1 following the design rule:
     * capture succeeds by weakening the Echo first or by building trust,
     * never at full health with zero trust, and never once defeated.
     * Feeding with preferred food and preferred weather/time add bonuses (v2).
     */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo")
    float ComputeCaptureChance() const;

    /** Species activity gate vs current world time (directive §13). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo")
    bool IsCurrentlyActiveTime() const;

    // --- Personality-driven behavior modifiers (directive §5) ---

    /** Multiplier on flee health threshold (Timid > 1 = flees earlier). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo|Personality")
    float GetFleeHealthThresholdMultiplier() const;

    /** Multiplier on aggro detection radius (Aggressive > 1 = aggro from farther). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo|Personality")
    float GetAggroRadiusMultiplier() const;

    /** Work speed multiplier (Lazy 0.6, Energetic 1.4...). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo|Personality")
    float GetWorkSpeedMultiplier() const;

    /** Command obedience 0..1 — Independent/Lazy obey less (directive §5/§10). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo|Personality")
    float GetCommandObedience() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo")
    const FAstrawildEchoStats& GetCachedStats() const { return CachedStats; }

    /**
     * Public AI state setter (audit H-8): the AI controller routes every transition
     * through here so OnAIStateChanged actually broadcasts for UI/audio consumers.
     */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo")
    void SetAIState(EAstrawildEchoAIState NewState);

    // --- Batch 3 — Item A: status effects (server-authoritative) ---

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo|Status")
    void AddStatusEffect(const FAstrawildStatusEffect& Effect);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo|Status")
    bool HasStatusEffect(FName StatusId) const;

    /** Combined speed multiplier from active statuses (1.0 when unaffected). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo|Status")
    float GetStatusSpeedMultiplier() const;

    // --- Batch 3 — Item B: stagger (heavy-hit reaction) ---

    /** True while staggered — AI pauses and movement zeroes. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo|Stagger")
    bool IsStaggered() const { return StaggerRemainingSeconds > 0.0f; }

    /** Server-side stagger entry point (clamped; zeroes speed, sets the AI state). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo|Stagger")
    void ApplyStagger(float Seconds);

    // --- Save/load v2 (schema v2, directive §27) ---

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo")
    FAstrawildEchoInstanceSaveData ToSaveData() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo")
    FAstrawildEchoInstanceV2 ToSaveDataV2() const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo")
    bool FromSaveDataV2(const FAstrawildEchoInstanceV2& Data);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaTime) override;

    UFUNCTION()
    void HandleNeedsDecay(float DeltaSeconds);

    /** Production V2 (Master Plan §6): party passive aura tick (1s cadence). */
    void ApplyPartyPassive(float AuraSeconds);

    /** Production V2: throttles the passive aura tick. */
    float PassiveAuraAccumulator = 0.0f;

private:
    FAstrawildEchoStats CachedStats;

    /** Batch 3 — Item B: server-side stagger countdown (client feedback via replicated CurrentAIState). */
    float StaggerRemainingSeconds = 0.0f;

    /** Needs simulate at a throttled cadence based on ecosystem LOD tier (directive §34). */
    float NeedsDecayAccumulator = 0.0f;

    void RollPersonalityFromDefinition();
    void RegisterWithEcosystem();
    void UnregisterFromEcosystem();
    class UAstrawildEcosystemSubsystem* GetEcosystem() const;
    /** Batch 3 — Item A: server tick of DoT/expiry/speed for StatusEffects. */
    void ApplyStatusTicks(float DeltaTime);

    /**
     * Navigation invoker (audit C-3): the zero-asset world has no authored navmesh —
     * each Echo generates navmesh tiles around itself at runtime so pathfinding
     * (MoveToLocation/MoveToActor) actually works.
     */
    UPROPERTY(VisibleAnywhere, Category="ASTRAWILD|Echo")
    TObjectPtr<UNavigationInvokerComponent> NavInvoker;
};
