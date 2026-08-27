// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/AstrawildCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/AstrawildAttributeComponent.h"
#include "Components/AstrawildInventoryComponent.h"
#include "Components/AstrawildCombatComponent.h"
#include "Components/AstrawildCaptureComponent.h"
#include "Components/AstrawildCraftingComponent.h"
#include "Components/AstrawildBuildingComponent.h"
#include "Interfaces/AstrawildInteractableInterface.h"
#include "AstrawildLogChannels.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"

AAstrawildCharacter::AAstrawildCharacter()
	: WalkSpeed(500.0f)
	, SprintSpeed(850.0f)
	, SprintStaminaCostPerSecond(15.0f)
	, DodgeImpulse(1300.0f)
	, DodgeDuration(0.40f)
	, DodgeStaminaCost(20.0f)
	, bIsSprinting(false)
	, bIsDodging(false)
	, CurrentMovementState(EAstrawildMovementState::Idle)
	, LookSensitivityYaw(1.0f)
	, LookSensitivityPitch(1.0f)
	, bInvertPitch(false)
	, InteractionRange(350.0f)
	, bHasFocusedInteractable(false)
	, DodgeTimer(0.0f)
	, DodgeDirection(FVector::ZeroVector)
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->JumpZVelocity = 550.0f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.0f;
	GetCharacterMovement()->GroundFriction = 8.0f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 380.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.0f;
	CameraBoom->SocketOffset = FVector(0.0f, 45.0f, 55.0f);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Attributes = CreateDefaultSubobject<UAstrawildAttributeComponent>(TEXT("Attributes"));
	Inventory = CreateDefaultSubobject<UAstrawildInventoryComponent>(TEXT("Inventory"));
	Combat = CreateDefaultSubobject<UAstrawildCombatComponent>(TEXT("Combat"));
	Capture = CreateDefaultSubobject<UAstrawildCaptureComponent>(TEXT("Capture"));
	Crafting = CreateDefaultSubobject<UAstrawildCraftingComponent>(TEXT("Crafting"));
	Building = CreateDefaultSubobject<UAstrawildBuildingComponent>(TEXT("Building"));
}

void AAstrawildCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Enhanced input subsystem registration
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (DefaultMappingContext)
			{
				Subsystem->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}

	// Starter inventory items for vertical slice
	if (Inventory)
	{
		const FGameplayTag SunwoodTag = FGameplayTag::RequestGameplayTag(FName("Item.Resource.Sunwood"), false);
		const FGameplayTag ResonatorTag = FGameplayTag::RequestGameplayTag(FName("Item.Tool.AstraResonatorBasic"), false);
		Inventory->AddItem(SunwoodTag, 10);
		Inventory->AddItem(ResonatorTag, 5);
	}
}

void AAstrawildCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Handle Dodge Timer
	if (bIsDodging)
	{
		DodgeTimer -= DeltaTime;
		if (DodgeTimer <= 0.0f)
		{
			bIsDodging = false;
			GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
		}
	}

	// Sprint stamina consumption
	if (bIsSprinting && Attributes && !bIsDodging)
	{
		const bool bHasStamina = Attributes->ModifyStamina(-SprintStaminaCostPerSecond * DeltaTime);
		if (!bHasStamina || GetCharacterMovement()->Velocity.SizeSquared() < 100.0f)
		{
			InputStopSprint();
		}
	}

	UpdateMovementState();
	UpdateInteractionFocus();
}

void AAstrawildCharacter::UpdateMovementState()
{
	if (GetCharacterMovement()->IsFalling())
	{
		CurrentMovementState = EAstrawildMovementState::Falling;
	}
	else if (bIsDodging)
	{
		CurrentMovementState = EAstrawildMovementState::Dodging;
	}
	else if (bIsSprinting && GetCharacterMovement()->Velocity.SizeSquared() > 100.0f)
	{
		CurrentMovementState = EAstrawildMovementState::Sprinting;
	}
	else if (GetCharacterMovement()->Velocity.SizeSquared() > 100.0f)
	{
		CurrentMovementState = EAstrawildMovementState::Walking;
	}
	else
	{
		CurrentMovementState = EAstrawildMovementState::Idle;
	}
}

void AAstrawildCharacter::UpdateInteractionFocus()
{
	FHitResult Hit;
	if (PerformInteractionTrace(Hit))
	{
		AActor* HitActor = Hit.GetActor();
		if (HitActor && HitActor->Implements<UAstrawildInteractableInterface>())
		{
			FocusedInteractableActor = HitActor;
			bHasFocusedInteractable = IAstrawildInteractableInterface::Execute_CanInteract(HitActor, this);
			CachedInteractionPrompt = IAstrawildInteractableInterface::Execute_GetInteractionPrompt(HitActor, this);
			return;
		}
	}

	FocusedInteractableActor = nullptr;
	bHasFocusedInteractable = false;
	CachedInteractionPrompt = FText::GetEmpty();
}

void AAstrawildCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 1. Try Enhanced Input Binding
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction)
		{
			EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAstrawildCharacter::InputMove);
		}
		if (LookAction)
		{
			EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAstrawildCharacter::InputLook);
		}
		if (JumpAction)
		{
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
		if (SprintAction)
		{
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AAstrawildCharacter::InputStartSprint);
			EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AAstrawildCharacter::InputStopSprint);
		}
		if (DodgeAction)
		{
			EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &AAstrawildCharacter::InputPerformDodge);
		}
		if (AttackAction)
		{
			EnhancedInputComponent->BindAction(AttackAction, ETriggerEvent::Started, this, &AAstrawildCharacter::InputPrimaryAttack);
		}
		if (CaptureAction)
		{
			EnhancedInputComponent->BindAction(CaptureAction, ETriggerEvent::Started, this, &AAstrawildCharacter::InputThrowResonator);
		}
		if (InteractAction)
		{
			EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AAstrawildCharacter::InputInteract);
		}
		if (SummonCompanionAction)
		{
			EnhancedInputComponent->BindAction(SummonCompanionAction, ETriggerEvent::Started, this, &AAstrawildCharacter::InputToggleSummon);
		}
		if (CycleCompanionAction)
		{
			EnhancedInputComponent->BindAction(CycleCompanionAction, ETriggerEvent::Triggered, this, &AAstrawildCharacter::InputCycleCompanion);
		}
	}

	// 2. Direct Raw Input Fallback (Ensures Play In Editor works out-of-the-box before Blueprint asset mapping)
	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AAstrawildCharacter::FallbackMoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AAstrawildCharacter::FallbackMoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AAstrawildCharacter::FallbackTurn);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AAstrawildCharacter::FallbackLookUp);

	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &AAstrawildCharacter::InputStartSprint);
	PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &AAstrawildCharacter::InputStopSprint);
	PlayerInputComponent->BindAction(TEXT("Dodge"), IE_Pressed, this, &AAstrawildCharacter::InputPerformDodge);
	PlayerInputComponent->BindAction(TEXT("Attack"), IE_Pressed, this, &AAstrawildCharacter::InputPrimaryAttack);
	PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &AAstrawildCharacter::InputInteract);
	PlayerInputComponent->BindAction(TEXT("ThrowResonator"), IE_Pressed, this, &AAstrawildCharacter::InputThrowResonator);
	PlayerInputComponent->BindAction(TEXT("ToggleSummon"), IE_Pressed, this, &AAstrawildCharacter::InputToggleSummon);
}

void AAstrawildCharacter::InputMove(const FInputActionValue& Value)
{
	if (bIsDodging)
	{
		return;
	}

	const FVector2D MovementVector = Value.Get<FVector2D>();
	if (Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(ForwardDirection, MovementVector.Y);
		AddMovementInput(RightDirection, MovementVector.X);
	}
}

void AAstrawildCharacter::InputLook(const FInputActionValue& Value)
{
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	if (Controller != nullptr)
	{
		const float PitchSign = bInvertPitch ? -1.0f : 1.0f;
		AddControllerYawInput(LookAxisVector.X * LookSensitivityYaw);
		AddControllerPitchInput(LookAxisVector.Y * LookSensitivityPitch * PitchSign);
	}
}

void AAstrawildCharacter::FallbackMoveForward(float Value)
{
	if (!FMath::IsNearlyZero(Value) && !bIsDodging && Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(ForwardDirection, Value);
	}
}

void AAstrawildCharacter::FallbackMoveRight(float Value)
{
	if (!FMath::IsNearlyZero(Value) && !bIsDodging && Controller != nullptr)
	{
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
		const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		AddMovementInput(RightDirection, Value);
	}
}

void AAstrawildCharacter::FallbackTurn(float Value)
{
	if (!FMath::IsNearlyZero(Value))
	{
		AddControllerYawInput(Value * LookSensitivityYaw);
	}
}

void AAstrawildCharacter::FallbackLookUp(float Value)
{
	if (!FMath::IsNearlyZero(Value))
	{
		const float PitchSign = bInvertPitch ? -1.0f : 1.0f;
		AddControllerPitchInput(Value * LookSensitivityPitch * PitchSign);
	}
}

void AAstrawildCharacter::InputStartSprint()
{
	if (Attributes && Attributes->CurrentStamina > 10.0f && !bIsDodging)
	{
		bIsSprinting = true;
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	}
}

void AAstrawildCharacter::InputStopSprint()
{
	bIsSprinting = false;
	if (!bIsDodging)
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	}
}

void AAstrawildCharacter::InputPerformDodge()
{
	if (bIsDodging || GetCharacterMovement()->IsFalling())
	{
		return;
	}

	if (!Attributes || !Attributes->ModifyStamina(-DodgeStaminaCost))
	{
		UE_LOG(LogAstrawild, Log, TEXT("Dodge failed: Insufficient stamina."));
		return;
	}

	bIsDodging = true;
	DodgeTimer = DodgeDuration;

	FVector MoveInput = GetLastMovementInputVector();
	if (MoveInput.IsNearlyZero())
	{
		MoveInput = GetActorForwardVector();
	}
	else
	{
		MoveInput.Normalize();
	}

	DodgeDirection = MoveInput;
	LaunchCharacter(DodgeDirection * DodgeImpulse, true, true);
	UE_LOG(LogAstrawild, Log, TEXT("Player performed Dodge roll in direction %s"), *DodgeDirection.ToString());
}

void AAstrawildCharacter::InputPrimaryAttack()
{
	if (bIsDodging)
	{
		return;
	}

	if (Building && Building->bIsBuildModeActive)
	{
		Building->PlaceBuilding();
		return;
	}

	if (Combat)
	{
		Combat->PerformMeleeAttack(1.0f, Attributes ? Attributes->ElementalAffinity : EAstrawildElement::Neutral);
	}
}

void AAstrawildCharacter::InputThrowResonator()
{
	if (bIsDodging)
	{
		return;
	}

	if (Capture)
	{
		Capture->ThrowResonator(1.0f);
	}
}

void AAstrawildCharacter::InputInteract()
{
	if (FocusedInteractableActor.IsValid())
	{
		AActor* Target = FocusedInteractableActor.Get();
		if (Target->Implements<UAstrawildInteractableInterface>())
		{
			IAstrawildInteractableInterface::Execute_PerformInteraction(Target, this);
		}
	}
	else
	{
		FHitResult Hit;
		if (PerformInteractionTrace(Hit))
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor && HitActor->Implements<UAstrawildInteractableInterface>())
			{
				IAstrawildInteractableInterface::Execute_PerformInteraction(HitActor, this);
			}
		}
	}
}

void AAstrawildCharacter::InputToggleSummon()
{
	if (!Capture)
	{
		return;
	}

	if (Capture->ActiveSummonedEcho.IsValid())
	{
		Capture->RecallActiveCompanion();
	}
	else
	{
		Capture->SummonSelectedCompanion();
	}
}

void AAstrawildCharacter::InputCycleCompanion(const FInputActionValue& Value)
{
	if (!Capture)
	{
		return;
	}

	const float Axis = Value.Get<float>();
	if (Axis > 0.0f)
	{
		Capture->SelectNextPartySlot();
	}
	else if (Axis < 0.0f)
	{
		Capture->SelectPrevPartySlot();
	}
}

bool AAstrawildCharacter::PerformInteractionTrace(FHitResult& OutHitResult)
{
	if (!FollowCamera || !GetWorld())
	{
		return false;
	}

	const FVector Start = FollowCamera->GetComponentLocation();
	const FVector End = Start + (FollowCamera->GetForwardVector() * InteractionRange);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	const FCollisionShape SphereShape = FCollisionShape::MakeSphere(25.0f); // Generous sweep for smooth interaction feel
	return GetWorld()->SweepSingleByChannel(OutHitResult, Start, End, FQuat::Identity, ECC_WorldDynamic, SphereShape, QueryParams);
}

float AAstrawildCharacter::TakeAstrawildDamage_Implementation(const FAstrawildDamageEvent& DamageEvent)
{
	// 1. Invulnerability during dodge roll
	if (bIsDodging)
	{
		UE_LOG(LogAstrawildCombat, Log, TEXT("Player dodged incoming attack (i-frame active)!"));
		return 0.0f;
	}

	if (!CanTakeDamage_Implementation(DamageEvent.InstigatorActor.Get()) || !Attributes)
	{
		return 0.0f;
	}

	// 2. Armor mitigation
	const float Defense = FMath::Max(0.0f, Attributes->DefensePower);
	const float DefenseFactor = 100.0f / (100.0f + Defense);

	// 3. Elemental Matrix calculation
	const float ElementMult = FAstrawildElementalMatrix::GetMultiplier(DamageEvent.DamageElement, Attributes->ElementalAffinity);

	// 4. Final damage
	const float FinalDamage = FMath::Max(1.0f, DamageEvent.BaseDamage * DefenseFactor * ElementMult);
	Attributes->ModifyHealth(-FinalDamage, DamageEvent.InstigatorActor.Get());

	// 5. Knockback
	if (DamageEvent.KnockbackImpulse > 0.0f)
	{
		LaunchCharacter(DamageEvent.HitDirection * DamageEvent.KnockbackImpulse, true, true);
	}

	// 6. Apply status effect
	if (DamageEvent.AppliedStatusTag.IsValid() && DamageEvent.StatusDuration > 0.0f)
	{
		Attributes->ApplyStatusEffect(DamageEvent.AppliedStatusTag, DamageEvent.StatusDuration, 1.0f, DamageEvent.InstigatorActor.Get());
	}

	UE_LOG(LogAstrawildCombat, Log, TEXT("[DAMAGEABLE] Player took %.1f [%s] damage from %s (HP: %.0f/%.0f)"),
		FinalDamage, *UEnum::GetValueAsString(DamageEvent.DamageElement),
		DamageEvent.InstigatorActor.IsValid() ? *DamageEvent.InstigatorActor->GetName() : TEXT("Unknown"),
		Attributes->CurrentHealth, Attributes->MaxHealth);

	return FinalDamage;
}

bool AAstrawildCharacter::CanTakeDamage_Implementation(AActor* Attacker)
{
	return Attributes && Attributes->IsAlive() && !bIsDodging;
}

EAstrawildElement AAstrawildCharacter::GetElementalAffinity_Implementation() const
{
	return Attributes ? Attributes->ElementalAffinity : EAstrawildElement::Neutral;
}