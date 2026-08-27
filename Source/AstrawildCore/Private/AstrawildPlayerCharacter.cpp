#include "AstrawildPlayerCharacter.h"

#include "AstrawildCore.h"
#include "AstrawildCaptureComponent.h"
#include "AstrawildDamageTarget.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildCraftingComponent.h"
#include "AstrawildInteractable.h"
#include "AstrawildInventoryComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "DrawDebugHelpers.h"

AAstrawildPlayerCharacter::AAstrawildPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    GetCapsuleComponent()->InitCapsuleSize(42.0f, 96.0f);
    bUseControllerRotationYaw = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
    GetCharacterMovement()->JumpZVelocity = 600.0f;
    GetCharacterMovement()->AirControl = 0.35f;
    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    CameraBoom->SetupAttachment(RootComponent);
    CameraBoom->TargetArmLength = 360.0f;
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
    PlaceholderMesh->SetupAttachment(RootComponent);
    PlaceholderMesh->SetCollisionProfileName(TEXT("NoCollision"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderMesh.Succeeded())
    {
        PlaceholderMesh->SetStaticMesh(CylinderMesh.Object);
        PlaceholderMesh->SetWorldScale3D(FVector(0.45f, 0.45f, 0.95f));
        PlaceholderMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 96.0f));
    }

    InventoryComponent = CreateDefaultSubobject<UAstrawildInventoryComponent>(TEXT("Inventory"));
    CraftingComponent = CreateDefaultSubobject<UAstrawildCraftingComponent>(TEXT("Crafting"));
    CaptureComponent = CreateDefaultSubobject<UAstrawildCaptureComponent>(TEXT("Capture"));

    FAstrawildItemStack Wood;
    Wood.ItemId = TEXT("Item_Wood");
    Wood.Quantity = 20;
    StarterItems.Add(Wood);

    FAstrawildItemStack Stone;
    Stone.ItemId = TEXT("Item_Stone");
    Stone.Quantity = 20;
    StarterItems.Add(Stone);

    FAstrawildItemStack Resonator;
    Resonator.ItemId = TEXT("Item_Resonator");
    Resonator.Quantity = 3;
    StarterItems.Add(Resonator);
}

void AAstrawildPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    SetMovementSpeed(WalkSpeed);

    if (bGivePrototypeStarterItems && HasAuthority() && InventoryComponent && InventoryComponent->GetItemStacks().IsEmpty())
    {
        for (const FAstrawildItemStack& StarterItem : StarterItems)
        {
            if (StarterItem.IsValid())
            {
                InventoryComponent->AddItem(StarterItem.ItemId, StarterItem.Quantity);
            }
        }
    }

    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!IsValid(PlayerController))
    {
        return;
    }

    ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
    UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer
        ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()
        : nullptr;

    if (InputSubsystem && DefaultMappingContext)
    {
        InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
    }
    else
    {
        UE_LOG(LogAstrawild, Warning, TEXT("Player input mapping is not assigned on %s."), *GetName());
    }
}

void AAstrawildPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!EnhancedInput)
    {
        UE_LOG(LogAstrawild, Error, TEXT("ASTRAWILD player requires EnhancedInputComponent."));
        return;
    }

    if (MoveAction)
    {
        EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAstrawildPlayerCharacter::Move);
    }
    if (LookAction)
    {
        EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AAstrawildPlayerCharacter::Look);
    }
    if (SprintAction)
    {
        EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::StartSprint);
        EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &AAstrawildPlayerCharacter::StopSprint);
        EnhancedInput->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AAstrawildPlayerCharacter::StopSprint);
    }
    if (InteractAction)
    {
        EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::Interact);
    }
    if (AttackAction)
    {
        EnhancedInput->BindAction(AttackAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::Attack);
    }
}

void AAstrawildPlayerCharacter::Move(const FInputActionValue& Value)
{
    const FVector2D MovementVector = Value.Get<FVector2D>();
    if (!Controller || MovementVector.IsNearlyZero())
    {
        return;
    }

    const FRotator ControlRotation = Controller->GetControlRotation();
    const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
    const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    AddMovementInput(ForwardDirection, MovementVector.Y);
    AddMovementInput(RightDirection, MovementVector.X);
}

void AAstrawildPlayerCharacter::Look(const FInputActionValue& Value)
{
    const FVector2D LookAxisVector = Value.Get<FVector2D>();
    AddControllerYawInput(LookAxisVector.X);
    AddControllerPitchInput(LookAxisVector.Y);
}

void AAstrawildPlayerCharacter::StartSprint(const FInputActionValue& Value)
{
    SetMovementSpeed(SprintSpeed);
}

void AAstrawildPlayerCharacter::StopSprint(const FInputActionValue& Value)
{
    SetMovementSpeed(WalkSpeed);
}

void AAstrawildPlayerCharacter::Interact(const FInputActionValue& Value)
{
    AActor* InteractableActor = FindInteractableActor();
    if (!IsValid(InteractableActor) || !InteractableActor->GetClass()->ImplementsInterface(UAstrawildInteractable::StaticClass()))
    {
        return;
    }

    IAstrawildInteractable::Execute_Interact(InteractableActor, this);
}

void AAstrawildPlayerCharacter::Attack(const FInputActionValue& Value)
{
    const UWorld* World = GetWorld();
    if (!World || (World->GetTimeSeconds() - LastAttackTimeSeconds) < AttackCooldownSeconds || !FollowCamera)
    {
        return;
    }

    LastAttackTimeSeconds = World->GetTimeSeconds();
    const FVector Start = FollowCamera->GetComponentLocation();
    const FVector End = Start + FollowCamera->GetForwardVector() * AttackDistance;
    FHitResult HitResult;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ASTRAWILDPlayerAttack), false, this);
    if (!World->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams))
    {
        return;
    }

    if (AAstrawildDamageTarget* DamageTarget = Cast<AAstrawildDamageTarget>(HitResult.GetActor()))
    {
        DamageTarget->ApplyDamage(AttackDamage);
    }
    else if (AAstrawildEchoCharacter* Echo = Cast<AAstrawildEchoCharacter>(HitResult.GetActor()))
    {
        Echo->ApplyDamage(AttackDamage);
    }
}

void AAstrawildPlayerCharacter::SetMovementSpeed(const float NewSpeed)
{
    if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
    {
        MovementComponent->MaxWalkSpeed = FMath::Max(0.0f, NewSpeed);
    }
}

AActor* AAstrawildPlayerCharacter::FindInteractableActor() const
{
    if (!FollowCamera || !GetWorld())
    {
        return nullptr;
    }

    const FVector Start = FollowCamera->GetComponentLocation();
    const FVector End = Start + FollowCamera->GetForwardVector() * InteractionDistance;
    FHitResult HitResult;
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ASTRAWILDInteraction), false, this);
    const bool bHit = GetWorld()->LineTraceSingleByChannel(HitResult, Start, End, ECC_Visibility, QueryParams);

    if (bDrawInteractionDebug)
    {
        DrawDebugLine(GetWorld(), Start, End, bHit ? FColor::Green : FColor::Red, false, 0.1f, 0, 1.5f);
    }

    return bHit ? HitResult.GetActor() : nullptr;
}
