#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AstrawildTypes.h"
#include "AstrawildPlayerCharacter.generated.h"

class AAstrawildDamageTarget;
class AAstrawildEchoCharacter;
class AAstrawildSkiffActor;
class AAstrawildUtilityDroneActor;
class UAstrawildBuildingComponent;
class UAstrawildAttributeComponent;
class UAstrawildCaptureComponent;
class UAstrawildCombatComponent;
class UAstrawildCraftingComponent;
class UAstrawildDurabilityComponent;
class UAstrawildInventoryComponent;
class UAstrawildItemDefinition;
class UAstrawildSurvivalComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UNavigationInvokerComponent;
class UProceduralMeshComponent;
class USkeletalMesh;
class USkeletalMeshComponent;
class UAnimSequenceBase;
class USpringArmComponent;
class UStaticMeshComponent;
struct FInputActionValue;

/**
 * The player character (directive §1/§9/§11).
 * Input is wired through Enhanced Input. If no editor-made Input Assets are assigned,
 * a complete default Keyboard+Mouse mapping context is constructed at runtime
 * (zero-asset playability — see Docs/ASTRAWILD_UI_ARCHITECTURE.md).
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildPlayerCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AAstrawildPlayerCharacter();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Visual")
    TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

    /**
     * Production V2 Batch 2: procedural survivor silhouette (torso/head/visor/
     * backpack/limbs — graphite + amber ASTRAWILD palette). Replaces the plain
     * grey cylinder; built on server AND owning client (BeginPlay runs locally).
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Visual")
    TObjectPtr<UProceduralMeshComponent> BodyMesh;

    /** Held weapon silhouette — rebuilt when the equipped weapon changes (family-tinted). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Visual")
    TObjectPtr<UProceduralMeshComponent> WeaponMesh;

    // ------------------------------------------------------------------
    // Art pack bindings (Batch 4 — Visual Vertical Slice, CP-01/CP-08).
    // Soft refs bound from AstrawildArtPack::GetSurvivorArt(). When the mesh
    // resolves (AwPipeline import on the host machine) the skinned exosuit
    // replaces the PMC silhouette and code-driven locomotion takes over.
    // ------------------------------------------------------------------
    UPROPERTY(EditDefaultsOnly, Category="ASTRAWILD|Art")
    TSoftObjectPtr<USkeletalMesh> SurvivorSkeletalMesh;

    UPROPERTY(EditDefaultsOnly, Category="ASTRAWILD|Art")
    TSoftObjectPtr<UAnimSequenceBase> SurvivorIdleAnim;

    UPROPERTY(EditDefaultsOnly, Category="ASTRAWILD|Art")
    TSoftObjectPtr<UAnimSequenceBase> SurvivorWalkAnim;

    UPROPERTY(EditDefaultsOnly, Category="ASTRAWILD|Art")
    TSoftObjectPtr<UAnimSequenceBase> SurvivorRunAnim;

    UPROPERTY(EditDefaultsOnly, Category="ASTRAWILD|Art")
    TSoftObjectPtr<UAnimSequenceBase> SurvivorJumpAnim;

    UPROPERTY(EditDefaultsOnly, Category="ASTRAWILD|Art")
    TSoftObjectPtr<UAnimSequenceBase> SurvivorAimAnim;

    UPROPERTY(EditDefaultsOnly, Category="ASTRAWILD|Art")
    TSoftObjectPtr<UAnimSequenceBase> SurvivorFireAnim;

    UPROPERTY(EditDefaultsOnly, Category="ASTRAWILD|Art")
    TSoftObjectPtr<UAnimSequenceBase> SurvivorGatherAnim;

    /** Skinned exosuit body (created lazily when the art pack resolves). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="ASTRAWILD|Art")
    TObjectPtr<USkeletalMeshComponent> SurvivorBody;

    /** Socket-driven held weapon mesh (replaces the PMC gun when active). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="ASTRAWILD|Art")
    TObjectPtr<UStaticMeshComponent> HeldWeaponMesh;

    /** True once the skinned body is live (PMC silhouette hidden). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient, Category="ASTRAWILD|Art")
    bool bSkeletalBodyActive = false;

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void PossessedBy(AController* NewController) override;
    virtual void PawnClientRestart() override;
    virtual void FellOutOfWorld(const UDamageType& DmgType) override;

    /** (Re)binds the Enhanced Input mapping context — called from BeginPlay, PawnClientRestart, and PossessedBy. */
    void ApplyMappingContext();
    void BuildRuntimeInputDefaults();
    void BuildGamepadInputDefaults();
    void BuildProceduralBody();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Camera")
    TObjectPtr<USpringArmComponent> CameraBoom;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Camera")
    TObjectPtr<UCameraComponent> FollowCamera;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Systems")
    TObjectPtr<UAstrawildInventoryComponent> InventoryComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Systems")
    TObjectPtr<UAstrawildCraftingComponent> CraftingComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Systems")
    TObjectPtr<UAstrawildCaptureComponent> CaptureComponent;

    /** Survival vitals (directive §11) — server authoritative. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Systems")
    TObjectPtr<UAstrawildSurvivalComponent> SurvivalComponent;

    /** Action combat (directive §9) — light/heavy/dodge/block. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Systems")
    TObjectPtr<UAstrawildCombatComponent> CombatComponent;

    /** Base building placement (directive §16). */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Systems")
    TObjectPtr<UAstrawildBuildingComponent> BuildingComponent;

    /** GDP-3: player growth — five attributes + seven milestone skills. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Systems")
    TObjectPtr<UAstrawildAttributeComponent> AttributeComponent;

    /** SCP Phase 12: equipment durability + harvest specialization + repairs. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Systems")
    TObjectPtr<UAstrawildDurabilityComponent> DurabilityComponent;

    // --- GDP-3: skill windows (public getters — the combat/capture components read them) ---

    /** True while Power Strike is queued onto the next melee swing. */
    bool IsNextMeleeEmpowered() const { return EmpoweredMeleeRemaining > 0.0f; }

    /** Spent by the combat component the moment the empowered swing lands. */
    void ConsumeEmpoweredMelee() { EmpoweredMeleeRemaining = 0.0f; }

    /** Remaining seconds of the Overcharge ranged damage window (+30% while active). */
    float GetRangedBuffRemaining() const { return RangedBuffRemaining; }

    /** Remaining seconds of the Hunter's Focus capture window (+25% chance while active). */
    float GetCaptureFocusRemaining() const { return CaptureFocusRemaining; }

    /**
     * DP-6: field-consumable timed effects — a consumed item may carry a timed
     * status payload (applied through the survival component's status-effect
     * system) and/or grant capture-focus seconds (the Hunter's Focus window).
     * Called by both consumption paths after ApplyConsumption (server).
     */
    void ApplyFieldConsumableEffects(const UAstrawildItemDefinition* ItemDef);

    /**
     * Navigation invoker (audit C-3): generates navmesh tiles around the player in
     * the zero-asset world so creature pathfinding works everywhere the player goes.
     */
    UPROPERTY(VisibleAnywhere, Category="ASTRAWILD|Systems")
    TObjectPtr<UNavigationInvokerComponent> NavInvoker;

    // --- Input assets (optional; runtime defaults are built when unset) ---
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputMappingContext> DefaultMappingContext;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> MoveAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> LookAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> SprintAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> JumpAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> InteractAction;

    /** Light attack. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> AttackAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> HeavyAttackAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> DodgeAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> BlockAction;

    /** Issue the cycled party command. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> CommandAction;

    /** Feed the targeted Echo (capture pipeline step, directive §8). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> FeedAction;

    /** Toggle build placement mode (directive §16). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> BuildModeAction;

    /** Smart-consume: eat/drink the most needed item (directive §11). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> ConsumeAction;

    /** Equip-best: auto-equip the strongest owned weapon + shield (wave 3). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> EquipBestAction;

    /** Batch 2 — Item B: dismantle the building under the crosshair, refund materials. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> DeleteBuildingAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> BuildRotateAction;

    /** Audit C-6: cycle building pieces with the mouse wheel while placing. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> BuildCycleAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> SaveAction;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> LoadAction;

    // --- Final production run inputs ---

    /** Hold-to-scan (equipped scanner accelerates journal observation). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> ScanAction;

    /** Deploy / recall the utility drone (consumes the drone item). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> DeployDroneAction;

    /** Deploy the utility robot (consumes the robot item, it mans the nearest site). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> DeployRobotAction;

    /** Toggle the inventory screen. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> InventoryAction;

    /** Toggle the research screen. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> ResearchAction;

    /** Toggle the pause menu (loop stage QUIT). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> PauseAction;

    /** Gamepad mapping context (coexists with the KB/M context — M9). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputMappingContext> GamepadMappingContext;

    /** Batch 8 — skiff descend (CTRL held; SPACE climbs through JumpAction). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> DescendAction;

    /** GDP-1: T — every owned party Echo casts its best ready ability. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> PartyAbilityAction;

    /** GDP-3: Y — smart-cast the player's best ready skill. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ASTRAWILD|Input")
    TObjectPtr<UInputAction> PlayerSkillAction;

    // --- Tunables ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat", meta=(ClampMin="0.0"))
    float AttackDamage = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat", meta=(ClampMin="50.0"))
    float AttackDistance = 280.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat", meta=(ClampMin="0.0"))
    float AttackCooldownSeconds = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Prototype")
    bool bGivePrototypeStarterItems = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Prototype")
    TArray<FAstrawildItemStack> StarterItems;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Movement", meta=(ClampMin="0.0"))
    float WalkSpeed = 450.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Movement", meta=(ClampMin="0.0"))
    float SprintSpeed = 700.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Interaction", meta=(ClampMin="50.0"))
    float InteractionDistance = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Interaction")
    bool bDrawInteractionDebug = false;

    /** Current party command cycled by CommandAction. */
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Echo")
    EAstrawildEchoCommand CurrentPartyCommand = EAstrawildEchoCommand::Follow;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Interaction")
    AActor* FindInteractableActor() const;

    /** The player's active utility drone (null when recalled/never deployed). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Drone")
    AAstrawildUtilityDroneActor* GetActiveDrone() const { return ActiveDrone.Get(); }

    /** Aim / Guard stance state. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Combat")
    bool IsAiming() const { return bGuardPose; }
    bool IsGuardPose() const { return bGuardPose; }

    /** Production V2: clear the active-drone handle (battery auto-recall path). */
    void ClearActiveDrone() { ActiveDrone = nullptr; }

    /** Batch 8 — the skiff this player is currently piloting (null = on foot). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Skiff")
    AAstrawildSkiffActor* GetPilotedSkiff() const;

    /** SCP Phase 5: the Echo this player is currently riding (null = afoot). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Mount")
    AAstrawildEchoCharacter* GetMountedEcho() const { return MountedEcho.Get(); }

    /** Called by the mount component on mount/dismount (input routing switches). */
    void SetMountedEcho(AAstrawildEchoCharacter* Echo);

    /** Called by the skiff on mount/dismount (input routing switches over). */
    void SetPilotedSkiff(AAstrawildSkiffActor* Skiff);

    /** Server: spawn a drone bound to this player (deploy key / save-load). */
    AAstrawildUtilityDroneActor* SpawnUtilityDrone();

    /** Server: spawn a utility robot and send it to the nearest unmanned site. */
    bool SpawnUtilityRobot(FName RobotDefinitionOverride = NAME_None);

    /** Production V2: robot chassis id carried by a robot item (NAME_None = general). */
    FName ResolveRobotDefinitionIdForItem(FName RobotItemId) const;

    /** Server-side respawn hook used by the game mode. */
    void HandleRespawn(const FTransform& SpawnTransform);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Player")
    bool IsAlive() const;

protected:
    // ---- Art pack animation driver (Batch 4, CP-08) ----
    /** Attempts the skinned exosuit swap; returns true when the art pack resolved. */
    bool TryActivateSkeletalBody();
    /** Code-driven locomotion selection (Idle/Walk/Run/Aim by velocity + guard). */
    void UpdateSurvivorAnimation();
    /** Fire/Jump/Gather one-shots: plays the clip once, then restores the loop. */
    void PlaySurvivorOneShot(UAnimSequenceBase* Sequence, float Duration);
    void OnSurvivorOneShotFinished();
    /** Guard pose (block) drives the Aim stance while mostly stationary. */
    void SetGuardPose(bool bEnabled);

    UPROPERTY(Transient)
    TObjectPtr<UAnimSequenceBase> CurrentLoopAnimation;

    bool bGuardPose = false;

    FTimerHandle SurvivorOneShotTimer;

    void Move(const FInputActionValue& Value);
    void Look(const FInputActionValue& Value);
    void StartSprint(const FInputActionValue& Value);
    void StopSprint(const FInputActionValue& Value);
    void Interact(const FInputActionValue& Value);
    void Attack(const FInputActionValue& Value);
    void HeavyAttack(const FInputActionValue& Value);
    void Dodge(const FInputActionValue& Value);
    void StartBlock(const FInputActionValue& Value);
    void StopBlock(const FInputActionValue& Value);
    void HandleJump(const FInputActionValue& Value);
    void CyclePartyCommand(const FInputActionValue& Value);
    void FeedTarget(const FInputActionValue& Value);

    // GDP-1/3: T = every party Echo casts its best ready ability; Y = the
    // player's smart-cast skill (priority ladder on the attribute component).
    void CastPartyAbility(const FInputActionValue& Value);
    void CastPlayerSkill(const FInputActionValue& Value);

    /** GDP-3: Power Strike window (seconds; consumed by the next melee hit). */
    float EmpoweredMeleeRemaining = 0.0f;

    /** GDP-3: Overcharge window (seconds; +30% ranged damage while active). */
    float RangedBuffRemaining = 0.0f;

    /** GDP-3: Hunter's Focus window (seconds; +25% capture chance while active). */
    float CaptureFocusRemaining = 0.0f;

    void ToggleBuildMode(const FInputActionValue& Value);
    void RotateBuilding(const FInputActionValue& Value);
    void CycleBuildingPiece(const FInputActionValue& Value);
    void SmartConsume(const FInputActionValue& Value);

    // Final production run: new input handlers.
    void StartScan(const FInputActionValue& Value);
    void StopScan(const FInputActionValue& Value);
    void DeployDrone(const FInputActionValue& Value);
    void DeployRobot(const FInputActionValue& Value);
    void ToggleInventoryScreenInput(const FInputActionValue& Value);
    void ToggleResearchScreenInput(const FInputActionValue& Value);
    void TogglePauseMenuInput(const FInputActionValue& Value);

    // Batch 8 — skiff flight inputs (routed to the piloted skiff).
    void OnJumpPressed(const FInputActionValue& Value);
    void OnJumpReleased(const FInputActionValue& Value);
    void StartDescend(const FInputActionValue& Value);
    void StopDescend(const FInputActionValue& Value);

    /** Wave 3: equip the strongest owned weapon + shield. */
    void EquipBest(const FInputActionValue& Value);

    /** Batch 2 — Item B: dismantle the building under the crosshair, refund materials. */
    void DeleteBuilding(const FInputActionValue& Value);
    void QuickSave(const FInputActionValue& Value);
    void QuickLoad(const FInputActionValue& Value);

    void RefreshMovementSpeed();

    /** Production V2 Batch 2: rebuild the held weapon mesh when equipment changes (timer-polled). */
    void RefreshHeldWeaponVisual();

    // Audit C-1b (latent compile error): every existing call passes 2 arguments while the
    // declaration demanded 3 — default bNegateY so the file compiles (value is unused).
    class UInputAction* MakeRuntimeAction(const FString& Name, uint8 ValueType, bool bNegateY = false);

    UFUNCTION()
    void OnPlayerDied();

    /** Batch 3 — Item A: status applied/ expired → recompute movement speed. */
    UFUNCTION()
    void OnStatusSpeedChanged(FName StatusId);

    /** Batch 3 — Item B: stagger entered/ left → recompute movement speed. */
    UFUNCTION()
    void OnStaggerChanged(bool bIsStaggered, float RemainingSeconds);

    /** Batch 4 — M-2b: blocking started/stopped → recompute movement speed (the
     *  BlockSpeedMultiplier penalty previously never applied because nothing
     *  listened to OnBlockingChanged). */
    UFUNCTION()
    void OnBlockingChanged(bool bIsBlocking);

    /** Batch 4 — M-2a: stamina hit the floor while sprinting → drop out of sprint. */
    UFUNCTION()
    void OnSprintExhausted();

private:
    void SetMovementSpeed(float NewSpeed);
    double LastAttackTimeSeconds = -BIG_NUMBER;
    bool bSprinting = false;

    /** Runtime-built input objects kept alive for GC. */
    UPROPERTY()
    TObjectPtr<UInputMappingContext> RuntimeMappingContext;

    UPROPERTY()
    TObjectPtr<UInputMappingContext> RuntimeGamepadContext;

    UPROPERTY()
    TArray<TObjectPtr<UInputAction>> RuntimeActions;

    /** Active utility drone (one per player; deploy key recalls). */
    UPROPERTY(VisibleAnywhere, Category="ASTRAWILD|Drone")
    TWeakObjectPtr<AAstrawildUtilityDroneActor> ActiveDrone;

    /** True while the scanner key is held (journal acceleration active). */
    bool bScanKeyHeld = false;

    /** Batch 8 — piloted skiff (weak so a destroyed skiff can't dangle input). */
    UPROPERTY(VisibleAnywhere, Category="ASTRAWILD|Skiff")
    TWeakObjectPtr<AAstrawildSkiffActor> PilotedSkiff;

    /** SCP Phase 5: ridden Echo (weak — a defeated mount auto-dismounts). */
    UPROPERTY(VisibleAnywhere, Category="ASTRAWILD|Mount")
    TWeakObjectPtr<AAstrawildEchoCharacter> MountedEcho;

    /** Production V2 Batch 2: held-weapon refresh cadence + dedupe cache. */
    FTimerHandle HeldWeaponTimerHandle;
    FName LastHeldWeaponId = NAME_None;
};
