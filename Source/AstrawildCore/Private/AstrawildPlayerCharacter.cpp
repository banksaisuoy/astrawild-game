#include "AstrawildPlayerCharacter.h"

#include "AstrawildBuildingComponent.h"
#include "AstrawildCaptureComponent.h"
#include "AstrawildCombatComponent.h"
#include "AstrawildCore.h"
#include "AstrawildCraftingComponent.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildEchoAIController.h"
#include "AstrawildEchoRosterSubsystem.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameMode.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildInteractable.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildSaveSubsystem.h"
#include "AstrawildSurvivalComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "NavigationInvokerComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildPlayerCharacter::AAstrawildPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    bReplicates = true;
    SetReplicatingMovement(true);

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
    SurvivalComponent = CreateDefaultSubobject<UAstrawildSurvivalComponent>(TEXT("Survival"));
    CombatComponent = CreateDefaultSubobject<UAstrawildCombatComponent>(TEXT("Combat"));
    BuildingComponent = CreateDefaultSubobject<UAstrawildBuildingComponent>(TEXT("Building"));

    // Audit C-3: broad navmesh generation around the player covers the camp, the
    // arena interior and the dungeon approach in the zero-asset world.
    NavInvoker = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavInvoker"));
    NavInvoker->SetRadii(12000.0f, 16000.0f);

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
    RefreshMovementSpeed();

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

    if (SurvivalComponent)
    {
        SurvivalComponent->OnDied.AddDynamic(this, &AAstrawildPlayerCharacter::OnPlayerDied);
    }

    // Input context binding also runs here for the initial spawn; PossessedBy covers
    // respawned pawns (audit C-8 — RestartPlayer spawns the pawn before possession,
    // so BeginPlay alone used to leave respawned pawns without an input mapping).
    ApplyMappingContext();
}

void AAstrawildPlayerCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    // Audit C-8: rebind input on every possession so respawned pawns keep control.
    ApplyMappingContext();
}

void AAstrawildPlayerCharacter::ApplyMappingContext()
{
    APlayerController* PlayerController = Cast<APlayerController>(GetController());
    if (!IsValid(PlayerController))
    {
        return;
    }

    // Build a complete default mapping context when no editor assets are assigned
    // (zero-asset playability — directive §50).
    if (!DefaultMappingContext)
    {
        BuildRuntimeInputDefaults();
    }

    ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
    UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer
        ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>()
        : nullptr;

    if (InputSubsystem && DefaultMappingContext)
    {
        // Idempotent: remove first so repeated possession never stacks the context.
        InputSubsystem->RemoveMappingContext(DefaultMappingContext);
        InputSubsystem->AddMappingContext(DefaultMappingContext, 0);
    }
    else if (GetNetMode() != NM_DedicatedServer)
    {
        UE_LOG(LogAstrawild, Warning, TEXT("Player input mapping is not assigned on %s."), *GetName());
    }
}

void AAstrawildPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Super::EndPlay(EndPlayReason);
}

void AAstrawildPlayerCharacter::FellOutOfWorld(const UDamageType& DmgType)
{
    Super::FellOutOfWorld(DmgType);
    if (SurvivalComponent && GetLocalRole() == ROLE_Authority)
    {
        SurvivalComponent->ApplyDamage(9999.0f);
    }
}

bool AAstrawildPlayerCharacter::IsAlive() const
{
    return !SurvivalComponent || !SurvivalComponent->IsDead();
}

UInputAction* AAstrawildPlayerCharacter::MakeRuntimeAction(const FString& Name, const uint8 ValueType, const bool bNegateY)
{
    UInputAction* Action = NewObject<UInputAction>(this, *Name);
    Action->ValueType = static_cast<EInputActionValueType>(ValueType);
    RuntimeActions.Add(Action);
    return Action;
}

void AAstrawildPlayerCharacter::BuildRuntimeInputDefaults()
{
    // --- Actions ---
    MoveAction = MakeRuntimeAction(TEXT("AWD_Move"), static_cast<uint8>(EInputActionValueType::Axis2D));
    LookAction = MakeRuntimeAction(TEXT("AWD_Look"), static_cast<uint8>(EInputActionValueType::Axis2D));
    SprintAction = MakeRuntimeAction(TEXT("AWD_Sprint"), static_cast<uint8>(EInputActionValueType::Boolean));
    JumpAction = MakeRuntimeAction(TEXT("AWD_Jump"), static_cast<uint8>(EInputActionValueType::Boolean));
    InteractAction = MakeRuntimeAction(TEXT("AWD_Interact"), static_cast<uint8>(EInputActionValueType::Boolean));
    AttackAction = MakeRuntimeAction(TEXT("AWD_LightAttack"), static_cast<uint8>(EInputActionValueType::Boolean));
    HeavyAttackAction = MakeRuntimeAction(TEXT("AWD_HeavyAttack"), static_cast<uint8>(EInputActionValueType::Boolean));
    DodgeAction = MakeRuntimeAction(TEXT("AWD_Dodge"), static_cast<uint8>(EInputActionValueType::Boolean));
    BlockAction = MakeRuntimeAction(TEXT("AWD_Block"), static_cast<uint8>(EInputActionValueType::Boolean));
    CommandAction = MakeRuntimeAction(TEXT("AWD_Command"), static_cast<uint8>(EInputActionValueType::Boolean));
    FeedAction = MakeRuntimeAction(TEXT("AWD_Feed"), static_cast<uint8>(EInputActionValueType::Boolean));
    BuildModeAction = MakeRuntimeAction(TEXT("AWD_BuildMode"), static_cast<uint8>(EInputActionValueType::Boolean));
    BuildRotateAction = MakeRuntimeAction(TEXT("AWD_BuildRotate"), static_cast<uint8>(EInputActionValueType::Boolean));
    ConsumeAction = MakeRuntimeAction(TEXT("AWD_Consume"), static_cast<uint8>(EInputActionValueType::Boolean));
    EquipBestAction = MakeRuntimeAction(TEXT("AWD_EquipBest"), static_cast<uint8>(EInputActionValueType::Boolean));
    SaveAction = MakeRuntimeAction(TEXT("AWD_Save"), static_cast<uint8>(EInputActionValueType::Boolean));
    LoadAction = MakeRuntimeAction(TEXT("AWD_Load"), static_cast<uint8>(EInputActionValueType::Boolean));
    // Audit C-6: mouse-wheel piece cycling while in build mode.
    BuildCycleAction = MakeRuntimeAction(TEXT("AWD_BuildCycle"), static_cast<uint8>(EInputActionValueType::Axis1D));

    RuntimeMappingContext = NewObject<UInputMappingContext>(this, TEXT("AWD_DefaultIMC"));
    UInputMappingContext* Context = RuntimeMappingContext;

    // --- Movement: WASD. A single 2D action fed by four keys with explicit axis modifiers. ---
    auto MapMoveKey = [&Context, this](const FKey& Key, const float X, const float Y)
    {
        FEnhancedActionKeyMapping& Mapping = Context->MapKey(MoveAction, Key);
        if (X < 0.0f)
        {
            UInputModifierNegate* NegateX = NewObject<UInputModifierNegate>(this);
            NegateX->bX = true;
            NegateX->bY = false;
            NegateX->bZ = false;
            Mapping.Modifiers.Add(NegateX);
        }
        if (Y < 0.0f)
        {
            UInputModifierNegate* NegateY = NewObject<UInputModifierNegate>(this);
            NegateY->bX = false;
            NegateY->bY = true;
            NegateY->bZ = false;
            Mapping.Modifiers.Add(NegateY);
        }
    };
    MapMoveKey(EKeys::W, 0.0f, 1.0f);
    MapMoveKey(EKeys::S, 0.0f, -1.0f);
    MapMoveKey(EKeys::D, 1.0f, 0.0f);
    MapMoveKey(EKeys::A, -1.0f, 0.0f);

    // --- Look: mouse delta. Negate Y for standard third-person pitch. ---
    FEnhancedActionKeyMapping& LookMapping = Context->MapKey(LookAction, EKeys::Mouse2D);
    UInputModifierNegate* LookNegateY = NewObject<UInputModifierNegate>(this);
    LookNegateY->bX = false;
    LookNegateY->bY = true;
    LookNegateY->bZ = false;
    LookMapping.Modifiers.Add(LookNegateY);

    // --- Simple key bindings. ---
    Context->MapKey(SprintAction, EKeys::LeftShift);
    Context->MapKey(JumpAction, EKeys::SpaceBar);
    Context->MapKey(InteractAction, EKeys::E);
    Context->MapKey(AttackAction, EKeys::LeftMouseButton);
    Context->MapKey(HeavyAttackAction, EKeys::F);
    Context->MapKey(DodgeAction, EKeys::Q);
    Context->MapKey(BlockAction, EKeys::RightMouseButton);
    Context->MapKey(CommandAction, EKeys::C);
    Context->MapKey(FeedAction, EKeys::R);
    Context->MapKey(BuildModeAction, EKeys::B);
    Context->MapKey(BuildRotateAction, EKeys::N);
    Context->MapKey(BuildCycleAction, EKeys::MouseWheelAxis);
    Context->MapKey(ConsumeAction, EKeys::G);
    Context->MapKey(EquipBestAction, EKeys::X);
    Context->MapKey(SaveAction, EKeys::F5);
    Context->MapKey(LoadAction, EKeys::F9);

    DefaultMappingContext = Context;
    UE_LOG(LogAstrawild, Log, TEXT("Runtime default input mapping built (18 actions, WASD+mouse+wheel)."));
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
    if (JumpAction)
    {
        EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::HandleJump);
        EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
    }
    if (InteractAction)
    {
        EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::Interact);
    }
    if (AttackAction)
    {
        EnhancedInput->BindAction(AttackAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::Attack);
    }
    if (HeavyAttackAction)
    {
        EnhancedInput->BindAction(HeavyAttackAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::HeavyAttack);
    }
    if (DodgeAction)
    {
        EnhancedInput->BindAction(DodgeAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::Dodge);
    }
    if (BlockAction)
    {
        EnhancedInput->BindAction(BlockAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::StartBlock);
        EnhancedInput->BindAction(BlockAction, ETriggerEvent::Completed, this, &AAstrawildPlayerCharacter::StopBlock);
        EnhancedInput->BindAction(BlockAction, ETriggerEvent::Canceled, this, &AAstrawildPlayerCharacter::StopBlock);
    }
    if (CommandAction)
    {
        EnhancedInput->BindAction(CommandAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::CyclePartyCommand);
    }
    if (FeedAction)
    {
        EnhancedInput->BindAction(FeedAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::FeedTarget);
    }
    if (BuildModeAction)
    {
        EnhancedInput->BindAction(BuildModeAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::ToggleBuildMode);
    }
    if (BuildRotateAction)
    {
        EnhancedInput->BindAction(BuildRotateAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::RotateBuilding);
    }
    if (BuildCycleAction)
    {
        EnhancedInput->BindAction(BuildCycleAction, ETriggerEvent::Triggered, this, &AAstrawildPlayerCharacter::CycleBuildingPiece);
    }
    if (ConsumeAction)
    {
        EnhancedInput->BindAction(ConsumeAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::SmartConsume);
    }
    if (SaveAction)
    {
        EnhancedInput->BindAction(SaveAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::QuickSave);
    }
    if (LoadAction)
    {
        EnhancedInput->BindAction(LoadAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::QuickLoad);
    }
    if (EquipBestAction)
    {
        EnhancedInput->BindAction(EquipBestAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::EquipBest);
    }
}

void AAstrawildPlayerCharacter::Move(const FInputActionValue& Value)
{
    if (!IsAlive())
    {
        return;
    }

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
    if (!IsAlive())
    {
        return;
    }

    bSprinting = true;
    RefreshMovementSpeed();
}

void AAstrawildPlayerCharacter::StopSprint(const FInputActionValue& Value)
{
    bSprinting = false;
    RefreshMovementSpeed();
}

void AAstrawildPlayerCharacter::RefreshMovementSpeed()
{
    float TargetSpeed = WalkSpeed;

    if (bSprinting && IsAlive())
    {
        // Cannot sprint while exhausted (directive §11 stamina).
        const bool bHasStamina = !SurvivalComponent || SurvivalComponent->GetStaminaFraction() > 0.05f;
        if (bHasStamina)
        {
            TargetSpeed = SprintSpeed;
        }
    }

    if (CombatComponent && CombatComponent->IsBlocking())
    {
        TargetSpeed *= CombatComponent->BlockSpeedMultiplier;
    }

    SetMovementSpeed(TargetSpeed);
}

void AAstrawildPlayerCharacter::HandleJump(const FInputActionValue& Value)
{
    if (IsAlive())
    {
        Jump();
    }
}

void AAstrawildPlayerCharacter::Interact(const FInputActionValue& Value)
{
    if (!IsAlive())
    {
        return;
    }

    AActor* InteractableActor = FindInteractableActor();

    // Standard interactables (nodes, stations, rest points, NPCs).
    if (IsValid(InteractableActor) && InteractableActor->GetClass()->ImplementsInterface(UAstrawildInteractable::StaticClass()))
    {
        IAstrawildInteractable::Execute_Interact(InteractableActor, this);
        return;
    }

    // Wild Echo in reach -> capture attempt (directive §8).
    if (AAstrawildEchoCharacter* Echo = Cast<AAstrawildEchoCharacter>(InteractableActor))
    {
        if (CaptureComponent && !Echo->bCaptured)
        {
            if (CaptureComponent->TryCapture(Echo))
            {
                // Join the roster + party (directive §4/§10).
                if (UGameInstance* GameInstance = GetGameInstance())
                {
                    if (UAstrawildEchoRosterSubsystem* Roster = GameInstance->GetSubsystem<UAstrawildEchoRosterSubsystem>())
                    {
                        Roster->AddToRoster(Echo);
                    }
                }
            }
        }
    }
}

void AAstrawildPlayerCharacter::Attack(const FInputActionValue& Value)
{
    if (!IsAlive() || !CombatComponent)
    {
        return;
    }

    // While in build placement mode, the primary button confirms placement instead.
    if (BuildingComponent && BuildingComponent->IsPlacing())
    {
        BuildingComponent->ConfirmPlacement();
        return;
    }

    CombatComponent->RequestLightAttack();
}

void AAstrawildPlayerCharacter::HeavyAttack(const FInputActionValue& Value)
{
    if (IsAlive() && CombatComponent)
    {
        CombatComponent->RequestHeavyAttack();
    }
}

void AAstrawildPlayerCharacter::Dodge(const FInputActionValue& Value)
{
    if (!IsAlive() || !CombatComponent)
    {
        return;
    }

    // Dodge along the current movement input direction (or forward).
    FVector DodgeDirection = GetLastMovementInputVector().GetSafeNormal2D();
    if (DodgeDirection.IsNearlyZero())
    {
        DodgeDirection = GetActorForwardVector().GetSafeNormal2D();
    }
    CombatComponent->RequestDodge(DodgeDirection);
}

void AAstrawildPlayerCharacter::StartBlock(const FInputActionValue& Value)
{
    if (IsAlive() && CombatComponent)
    {
        CombatComponent->RequestSetBlocking(true);
    }
}

void AAstrawildPlayerCharacter::StopBlock(const FInputActionValue& Value)
{
    if (CombatComponent)
    {
        CombatComponent->RequestSetBlocking(false);
    }
}

void AAstrawildPlayerCharacter::CyclePartyCommand(const FInputActionValue& Value)
{
    if (!IsAlive() || GetLocalRole() != ROLE_Authority)
    {
        return;
    }

    // Follow -> Attack -> Defend -> Stay -> Work -> Follow ...
    switch (CurrentPartyCommand)
    {
    case EAstrawildEchoCommand::Follow: CurrentPartyCommand = EAstrawildEchoCommand::Attack; break;
    case EAstrawildEchoCommand::Attack: CurrentPartyCommand = EAstrawildEchoCommand::Defend; break;
    case EAstrawildEchoCommand::Defend: CurrentPartyCommand = EAstrawildEchoCommand::Stay; break;
    case EAstrawildEchoCommand::Stay: CurrentPartyCommand = EAstrawildEchoCommand::Work; break;
    default: CurrentPartyCommand = EAstrawildEchoCommand::Follow; break;
    }

    // Broadcast the command to every owned party Echo (directive §10).
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    const FName OwnerId = GetFName();
    TArray<AActor*> Echoes;
    UGameplayStatics::GetAllActorsOfClass(World, AAstrawildEchoCharacter::StaticClass(), Echoes);
    for (AActor* Actor : Echoes)
    {
        AAstrawildEchoCharacter* Echo = Cast<AAstrawildEchoCharacter>(Actor);
        if (Echo && Echo->bCaptured && Echo->OwnerPlayerId == OwnerId)
        {
            Echo->IssueCommand(CurrentPartyCommand);
        }
    }

    UE_LOG(LogAstrawildAI, Log, TEXT("Party command -> %d"), static_cast<int32>(CurrentPartyCommand));
}

void AAstrawildPlayerCharacter::FeedTarget(const FInputActionValue& Value)
{
    if (!IsAlive() || GetLocalRole() != ROLE_Authority || !InventoryComponent)
    {
        return;
    }

    AActor* Target = FindInteractableActor();
    AAstrawildEchoCharacter* Echo = Cast<AAstrawildEchoCharacter>(Target);
    if (!Echo || Echo->IsDefeated() || !IsValid(Echo->EchoDefinition))
    {
        return;
    }

    // Preferred food first, then any consumable (directive §8).
    for (const FName FoodId : Echo->EchoDefinition->PreferredFoodIds)
    {
        if (InventoryComponent->HasItem(FoodId, 1))
        {
            if (InventoryComponent->RemoveItem(FoodId, 1))
            {
                Echo->Feed(FoodId, 8.0f);
                return;
            }
        }
    }

    // Fallback: any item with food value in the registry.
    if (UAstrawildItemRegistrySubsystem* Registry = GetWorld()->GetSubsystem<UAstrawildItemRegistrySubsystem>())
    {
        for (const FAstrawildItemStack& Stack : InventoryComponent->GetItemStacks())
        {
            const UAstrawildItemDefinition* ItemDef = Registry->FindItem(Stack.ItemId);
            if (ItemDef && ItemDef->EchoFeedValue > 0.0f && InventoryComponent->RemoveItem(Stack.ItemId, 1))
            {
                Echo->Feed(Stack.ItemId, ItemDef->EchoFeedValue);
                return;
            }
        }
    }
}

void AAstrawildPlayerCharacter::ToggleBuildMode(const FInputActionValue& Value)
{
    if (IsAlive() && BuildingComponent)
    {
        BuildingComponent->TogglePlacementMode();
    }
}

void AAstrawildPlayerCharacter::RotateBuilding(const FInputActionValue& Value)
{
    if (BuildingComponent)
    {
        BuildingComponent->RotatePreview(15.0f);
    }
}

void AAstrawildPlayerCharacter::CycleBuildingPiece(const FInputActionValue& Value)
{
    // Audit C-6: mouse-wheel cycling through the unlocked building pieces while in
    // placement mode (previously only one arbitrary piece was ever placeable).
    if (!BuildingComponent || !BuildingComponent->IsPlacing())
    {
        return;
    }

    const float Axis = Value.Get<float>();
    if (!FMath::IsNearlyZero(Axis))
    {
        BuildingComponent->CycleBuildingDefinition(Axis > 0.0f ? 1 : -1);
    }
}

void AAstrawildPlayerCharacter::SmartConsume(const FInputActionValue& Value)
{
    if (!IsAlive() || GetLocalRole() != ROLE_Authority || !InventoryComponent || !SurvivalComponent)
    {
        return;
    }

    // Smart consume (directive §11): address the most depleted vital first.
    const FAstrawildSurvivalStats& Stats = SurvivalComponent->GetStats();
    const bool bNeedWater = Stats.Thirst < Stats.Hunger;

    UAstrawildItemRegistrySubsystem* Registry = GetWorld() ? GetWorld()->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    if (!Registry)
    {
        return;
    }

    // Rank consumables: strongest for the needed vital first.
    const UAstrawildItemDefinition* Best = nullptr;
    float BestScore = 0.0f;
    for (const FAstrawildItemStack& Stack : InventoryComponent->GetItemStacks())
    {
        const UAstrawildItemDefinition* ItemDef = Registry->FindItem(Stack.ItemId);
        if (!ItemDef || ItemDef->Category != EAstrawildItemCategory::Consumable)
        {
            continue;
        }
        const float Score = (bNeedWater ? ItemDef->WaterValue : ItemDef->FoodValue) + ItemDef->HealValue * 0.5f;
        if (Score > BestScore)
        {
            BestScore = Score;
            Best = ItemDef;
        }
    }

    if (Best && BestScore > 0.0f && InventoryComponent->RemoveItem(Best->ItemId, 1))
    {
        SurvivalComponent->ApplyConsumption(Best->FoodValue, Best->WaterValue, Best->HealValue);
        UE_LOG(LogAstrawildEconomy, Log, TEXT("Consumed %s (+%.0f food, +%.0f water, +%.0f heal)."),
            *Best->ItemId.ToString(), Best->FoodValue, Best->WaterValue, Best->HealValue);
    }
}

void AAstrawildPlayerCharacter::EquipBest(const FInputActionValue& Value)
{
    if (!IsAlive() || GetLocalRole() != ROLE_Authority || !InventoryComponent)
    {
        return;
    }

    UAstrawildItemRegistrySubsystem* Registry = GetWorld() ? GetWorld()->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    if (!Registry)
    {
        return;
    }

    // Equip-best (wave 3): strongest owned weapon by AttackPower + strongest shield by BlockMitigation.
    const UAstrawildItemDefinition* BestWeapon = nullptr;
    const UAstrawildItemDefinition* BestShield = nullptr;
    for (const FAstrawildItemStack& Stack : InventoryComponent->GetItemStacks())
    {
        const UAstrawildItemDefinition* ItemDef = Registry->FindItem(Stack.ItemId);
        if (!ItemDef || ItemDef->Category != EAstrawildItemCategory::Equipment)
        {
            continue;
        }
        if (ItemDef->AttackPower > 0.0f && (!BestWeapon || ItemDef->AttackPower > BestWeapon->AttackPower))
        {
            BestWeapon = ItemDef;
        }
        if (ItemDef->BlockMitigation > 0.0f && (!BestShield || ItemDef->BlockMitigation > BestShield->BlockMitigation))
        {
            BestShield = ItemDef;
        }
    }

    if (BestWeapon)
    {
        InventoryComponent->EquipItem(BestWeapon->ItemId);
    }
    if (BestShield)
    {
        InventoryComponent->EquipItem(BestShield->ItemId);
    }
    UE_LOG(LogAstrawildEconomy, Log, TEXT("Equip-best: weapon=%s shield=%s."),
        BestWeapon ? *BestWeapon->ItemId.ToString() : TEXT("none"),
        BestShield ? *BestShield->ItemId.ToString() : TEXT("none"));
}

void AAstrawildPlayerCharacter::QuickSave(const FInputActionValue& Value)
{
    if (GetLocalRole() != ROLE_Authority)
    {
        return;
    }

    if (UAstrawildSaveSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<UAstrawildSaveSubsystem>())
    {
        SaveSubsystem->SaveWorld(GetWorld());
    }
}

void AAstrawildPlayerCharacter::QuickLoad(const FInputActionValue& Value)
{
    if (GetLocalRole() != ROLE_Authority)
    {
        return;
    }

    if (UAstrawildSaveSubsystem* SaveSubsystem = GetGameInstance()->GetSubsystem<UAstrawildSaveSubsystem>())
    {
        // Audit H-3: load the newest slot — the autosave was previously unreachable.
        SaveSubsystem->LoadLatest(GetWorld());
    }
}

void AAstrawildPlayerCharacter::OnPlayerDied()
{
    UE_LOG(LogAstrawildCombat, Log, TEXT("Player died — awaiting respawn."));

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        DisableInput(PC);
    }

    // Ask the game mode for a respawn after a short delay (directive §11).
    if (UWorld* World = GetWorld())
    {
        if (AAstrawildGameMode* GameMode = World->GetAuthGameMode<AAstrawildGameMode>())
        {
            GameMode->RequestPlayerRespawn(GetController(), 5.0f);
        }
    }
}

void AAstrawildPlayerCharacter::HandleRespawn(const FTransform& SpawnTransform)
{
    SetActorTransform(SpawnTransform, false, nullptr, ETeleportType::TeleportPhysics);

    if (SurvivalComponent)
    {
        SurvivalComponent->FullRestore();
    }

    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        EnableInput(PC);
    }
    RefreshMovementSpeed();
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
