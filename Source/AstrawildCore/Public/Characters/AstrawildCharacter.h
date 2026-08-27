// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Interfaces/AstrawildDamageableInterface.h"
#include "AstrawildCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UAstrawildAttributeComponent;
class UAstrawildInventoryComponent;
class UAstrawildCombatComponent;
class UAstrawildCaptureComponent;
class UAstrawildCraftingComponent;
class UAstrawildBuildingComponent;
class UAstrawildFeedbackComponent;
class UAstrawildQuestComponent;
class UAstrawildSurvivalComponent;
class UAstrawildEnvironmentHazardComponent;
class UAnimInstance;
class USkeletalMesh;
class UInputMappingContext;
class UInputAction;

UENUM(BlueprintType)
enum class EAstrawildMovementState : uint8
{
	Idle        UMETA(DisplayName = "Idle"),
	Walking     UMETA(DisplayName = "Walking"),
	Sprinting   UMETA(DisplayName = "Sprinting"),
	Dodging     UMETA(DisplayName = "Dodging / Rolling"),
	Falling     UMETA(DisplayName = "Falling / In Air")
};

UCLASS()
class ASTRAWILDCORE_API AAstrawildCharacter : public ACharacter, public IAstrawildDamageableInterface
{
	GENERATED_BODY()

public:
	AAstrawildCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

public:
	// --- Visual / Animation Contract ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals|Animation")
	TSoftObjectPtr<USkeletalMesh> PlayerSkeletalMesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals|Animation")
	TSoftClassPtr<UAnimInstance> PlayerAnimationBlueprintClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Visuals|Animation")
	FName AnimationProfileId = TEXT("Player_Default");

	// --- Camera Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	// --- Gameplay Components ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAstrawildAttributeComponent> Attributes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAstrawildInventoryComponent> Inventory;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAstrawildCombatComponent> Combat;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAstrawildCaptureComponent> Capture;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAstrawildCraftingComponent> Crafting;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAstrawildBuildingComponent> Building;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Feedback")
	TObjectPtr<UAstrawildFeedbackComponent> Feedback;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Quest")
	TObjectPtr<UAstrawildQuestComponent> Quest;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Survival")
	TObjectPtr<UAstrawildSurvivalComponent> Survival;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components|Survival")
	TObjectPtr<UAstrawildEnvironmentHazardComponent> EnvironmentHazard;

	// --- Movement Settings ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float WalkSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement")
	float SprintStaminaCostPerSecond;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dodge")
	float DodgeImpulse;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dodge")
	float DodgeDuration;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Movement|Dodge")
	float DodgeStaminaCost;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bIsSprinting;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	bool bIsDodging;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Movement")
	EAstrawildMovementState CurrentMovementState;

	// --- Camera Sensitivity Settings ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Sensitivity")
	float LookSensitivityYaw;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Sensitivity")
	float LookSensitivityPitch;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Sensitivity")
	bool bInvertPitch;

	// --- Interaction Settings ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	float InteractionRange;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TWeakObjectPtr<AActor> FocusedInteractableActor;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	FText CachedInteractionPrompt;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	bool bHasFocusedInteractable;

	// --- Input Actions (Enhanced Input) ---
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> DodgeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AttackAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CaptureAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SummonCompanionAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CycleCompanionAction;

public:
	// --- Action Handlers (Enhanced Input & Direct Fallback) ---
	UFUNCTION(BlueprintCallable, Category = "Input")
	void InputMove(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void InputLook(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "Input")
	void InputStartSprint();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void InputStopSprint();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void InputPerformDodge();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void InputPrimaryAttack();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void InputThrowResonator();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void InputInteract();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void InputToggleSummon();

	UFUNCTION(BlueprintCallable, Category = "Input")
	void InputCycleCompanion(const FInputActionValue& Value);

	// --- Direct Axis & Action Handlers for Fallback Input ---
	void FallbackMoveForward(float Value);
	void FallbackMoveRight(float Value);
	void FallbackTurn(float Value);
	void FallbackLookUp(float Value);

	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool PerformInteractionTrace(FHitResult& OutHitResult);

	UFUNCTION(BlueprintPure, Category = "Movement")
	EAstrawildMovementState GetCurrentMovementState() const { return CurrentMovementState; }

	// --- IAstrawildDamageableInterface Implementation ---
	virtual float TakeAstrawildDamage_Implementation(const FAstrawildDamageEvent& DamageEvent) override;
	virtual bool CanTakeDamage_Implementation(AActor* Attacker) override;
	virtual EAstrawildElement GetElementalAffinity_Implementation() const override;

private:
	float DodgeTimer;
	FVector DodgeDirection;

	void UpdateMovementState();
	void UpdateInteractionFocus();
	void ApplyVisualRepresentation();
};