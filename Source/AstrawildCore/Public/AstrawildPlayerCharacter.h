#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AstrawildTypes.h"
#include "AstrawildPlayerCharacter.generated.h"

class AAstrawildDamageTarget;
class AAstrawildEchoCharacter;
class UAstrawildBuildingComponent;
class UAstrawildCaptureComponent;
class UAstrawildCombatComponent;
class UAstrawildCraftingComponent;
class UAstrawildInventoryComponent;
class UAstrawildSurvivalComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UNavigationInvokerComponent;
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

    virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void PossessedBy(AController* NewController) override;
    virtual void FellOutOfWorld(const UDamageType& DmgType) override;

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

    /** Server-side respawn hook used by the game mode. */
    void HandleRespawn(const FTransform& SpawnTransform);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Player")
    bool IsAlive() const;

protected:
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
    void ToggleBuildMode(const FInputActionValue& Value);
    void RotateBuilding(const FInputActionValue& Value);
    void CycleBuildingPiece(const FInputActionValue& Value);
    void SmartConsume(const FInputActionValue& Value);

    /** Wave 3: equip the strongest owned weapon + shield. */
    void EquipBest(const FInputActionValue& Value);

    /** Batch 2 — Item B: dismantle the building under the crosshair, refund materials. */
    void DeleteBuilding(const FInputActionValue& Value);
    void QuickSave(const FInputActionValue& Value);
    void QuickLoad(const FInputActionValue& Value);

    void RefreshMovementSpeed();

    /** Builds a complete default Enhanced Input setup in code (zero-asset playability). */
    void BuildRuntimeInputDefaults();
    // Audit C-1b (latent compile error): every existing call passes 2 arguments while the
    // declaration demanded 3 — default bNegateY so the file compiles (value is unused).
    class UInputAction* MakeRuntimeAction(const FString& Name, uint8 ValueType, bool bNegateY = false);

    /** (Re)binds the Enhanced Input mapping context — called from BeginPlay and every PossessedBy (audit C-8). */
    void ApplyMappingContext();

    UFUNCTION()
    void OnPlayerDied();

    /** Batch 3 — Item A: status applied/ expired → recompute movement speed. */
    UFUNCTION()
    void OnStatusSpeedChanged(FName StatusId);

    /** Batch 3 — Item B: stagger entered/ left → recompute movement speed. */
    UFUNCTION()
    void OnStaggerChanged(bool bIsStaggered, float RemainingSeconds);

private:
    void SetMovementSpeed(float NewSpeed);
    double LastAttackTimeSeconds = -BIG_NUMBER;
    bool bSprinting = false;

    /** Runtime-built input objects kept alive for GC. */
    UPROPERTY()
    TObjectPtr<UInputMappingContext> RuntimeMappingContext;

    UPROPERTY()
    TArray<TObjectPtr<UInputAction>> RuntimeActions;
};
