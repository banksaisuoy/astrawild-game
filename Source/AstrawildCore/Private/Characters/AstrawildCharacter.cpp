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
#include "Environment/AstrawildHarvestableNode.h"
#include "Environment/AstrawildBuildingPiece.h"
#include "AstrawildLogChannels.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"

AAstrawildCharacter::AAstrawildCharacter()
	: WalkSpeed(500.0f)
	, SprintSpeed(850.0f)
	, SprintStaminaCostPerSecond(15.0f)
	, bIsSprinting(false)
	, InteractionRange(350.0f)
{
	PrimaryActorTick.bCanEverTick = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 500.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->JumpZVelocity = 550.0f;
	GetCharacterMovement()->AirControl = 0.35f;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->SocketOffset = FVector(0.0f, 50.0f, 60.0f);

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

	// Sprint stamina consumption
	if (bIsSprinting && Attributes)
	{
		const bool bHasStamina = Attributes->ModifyStamina(-SprintStaminaCostPerSecond * DeltaTime);
		if (!bHasStamina || GetCharacterMovement()->Velocity.SizeSquared() < 100.0f)
		{
			InputStopSprint();
		}
	}
}

void AAstrawildCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

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
}

void AAstrawildCharacter::InputMove(const FInputActionValue& Value)
{
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
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}

void AAstrawildCharacter::InputStartSprint()
{
	if (Attributes && Attributes->CurrentStamina > 10.0f)
	{
		bIsSprinting = true;
		GetCharacterMovement()->MaxWalkSpeed = SprintSpeed;
	}
}

void AAstrawildCharacter::InputStopSprint()
{
	bIsSprinting = false;
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void AAstrawildCharacter::InputPrimaryAttack()
{
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
	if (Capture)
	{
		Capture->ThrowResonator(1.0f);
	}
}

void AAstrawildCharacter::InputInteract()
{
	FHitResult Hit;
	if (PerformInteractionTrace(Hit))
	{
		AActor* HitActor = Hit.GetActor();
		if (AAstrawildHarvestableNode* Node = Cast<AAstrawildHarvestableNode>(HitActor))
		{
			int32 OutQty = 0;
			Node->Harvest(1.0f, EAstrawildHarvestType::Foraging, this, OutQty);
		}
		else if (AAstrawildBuildingPiece* Piece = Cast<AAstrawildBuildingPiece>(HitActor))
		{
			Piece->Interact(this);
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

	return GetWorld()->LineTraceSingleByChannel(OutHitResult, Start, End, ECC_WorldDynamic, QueryParams);
}