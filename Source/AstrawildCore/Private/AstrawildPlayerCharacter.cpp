#include "AstrawildPlayerCharacter.h"

#include "AstrawildBuildingActor.h"
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
#include "AstrawildJournalSubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerController.h"
#include "AstrawildSaveSubsystem.h"
#include "AstrawildSurvivalComponent.h"
#include "AstrawildUtilityDroneActor.h"
#include "AstrawildUtilityRobotActor.h"
#include "AstrawildWorkSiteActor.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
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

    // Batch 4 — M-11: starter vendor currency so the Trader Tam economy is
    // testable from the first spawn (further Dawn Shards come from the dungeon
    // boss loot table and the Dawn Guard quest reward).
    FAstrawildItemStack DawnShards;
    DawnShards.ItemId = TEXT("Item_DawnShard");
    DawnShards.Quantity = 10;
    StarterItems.Add(DawnShards);
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
        // Batch 3 — Item A: refresh speed when a status effect applies/expires
        // (Chill/Shock change the combined multiplier).
        SurvivalComponent->OnStatusEffectApplied.AddDynamic(this, &AAstrawildPlayerCharacter::OnStatusSpeedChanged);
        SurvivalComponent->OnStatusEffectRemoved.AddDynamic(this, &AAstrawildPlayerCharacter::OnStatusSpeedChanged);
        // Batch 4 — M-2a: stamina floor while sprinting → drop out of sprint speed.
        SurvivalComponent->OnSprintExhausted.AddDynamic(this, &AAstrawildPlayerCharacter::OnSprintExhausted);
    }

    // Batch 3 — Item B: refresh speed when the stagger state changes.
    if (CombatComponent)
    {
        CombatComponent->OnStaggerStateChanged.AddDynamic(this, &AAstrawildPlayerCharacter::OnStaggerChanged);
        // Batch 4 — M-2b: refresh speed when blocking starts/stops so the
        // BlockSpeedMultiplier penalty actually applies and lifts.
        CombatComponent->OnBlockingChanged.AddDynamic(this, &AAstrawildPlayerCharacter::OnBlockingChanged);
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

        // Final production run (M9): the gamepad context coexists with KB/M so a
        // controller works the moment it is plugged in — no settings screen needed.
        if (!GamepadMappingContext)
        {
            BuildGamepadInputDefaults();
        }
        if (GamepadMappingContext)
        {
            InputSubsystem->RemoveMappingContext(GamepadMappingContext);
            InputSubsystem->AddMappingContext(GamepadMappingContext, 0);
        }
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
    // Batch 2 — Item B: dismantle building under the crosshair.
    DeleteBuildingAction = MakeRuntimeAction(TEXT("AWD_DeleteBuilding"), static_cast<uint8>(EInputActionValueType::Boolean));
    SaveAction = MakeRuntimeAction(TEXT("AWD_Save"), static_cast<uint8>(EInputActionValueType::Boolean));
    LoadAction = MakeRuntimeAction(TEXT("AWD_Load"), static_cast<uint8>(EInputActionValueType::Boolean));
    // Audit C-6: mouse-wheel piece cycling while in build mode.
    BuildCycleAction = MakeRuntimeAction(TEXT("AWD_BuildCycle"), static_cast<uint8>(EInputActionValueType::Axis1D));
    // Final production run: scanner (hold), robotics, UI screens.
    ScanAction = MakeRuntimeAction(TEXT("AWD_Scan"), static_cast<uint8>(EInputActionValueType::Boolean));
    DeployDroneAction = MakeRuntimeAction(TEXT("AWD_DeployDrone"), static_cast<uint8>(EInputActionValueType::Boolean));
    DeployRobotAction = MakeRuntimeAction(TEXT("AWD_DeployRobot"), static_cast<uint8>(EInputActionValueType::Boolean));
    InventoryAction = MakeRuntimeAction(TEXT("AWD_Inventory"), static_cast<uint8>(EInputActionValueType::Boolean));
    ResearchAction = MakeRuntimeAction(TEXT("AWD_Research"), static_cast<uint8>(EInputActionValueType::Boolean));
    PauseAction = MakeRuntimeAction(TEXT("AWD_Pause"), static_cast<uint8>(EInputActionValueType::Boolean));

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
    // Batch 2 — Item B: Z = dismantle building under the crosshair (refunds materials).
    Context->MapKey(DeleteBuildingAction, EKeys::Z);
    Context->MapKey(SaveAction, EKeys::F5);
    Context->MapKey(LoadAction, EKeys::F9);
    // Final production run: V = hold-to-scan, H = drone, J = robot, TAB = pack,
    // K = research, ESC = pause (documented in ASTRAWILD_INPUT_REFERENCE.md).
    Context->MapKey(ScanAction, EKeys::V);
    Context->MapKey(DeployDroneAction, EKeys::H);
    Context->MapKey(DeployRobotAction, EKeys::J);
    Context->MapKey(InventoryAction, EKeys::Tab);
    Context->MapKey(ResearchAction, EKeys::K);
    Context->MapKey(PauseAction, EKeys::Escape);

    DefaultMappingContext = Context;
    UE_LOG(LogAstrawild, Log, TEXT("Runtime default input mapping built (25 actions, WASD+mouse+wheel+UI)."));
}

void AAstrawildPlayerCharacter::BuildGamepadInputDefaults()
{
    // Final production run (M9): full gamepad companion context. Shares the SAME
    // action objects as KB/M (actions are input-agnostic) — only the keys differ.
    if (!MoveAction || !LookAction)
    {
        return; // KB/M defaults build first; nothing to map onto yet.
    }

    RuntimeGamepadContext = NewObject<UInputMappingContext>(this, TEXT("AWD_GamepadIMC"));
    UInputMappingContext* Context = RuntimeGamepadContext;

    // Sticks: left = move, right = look (paired 2D axes — no modifiers needed).
    Context->MapKey(MoveAction, EKeys::Gamepad_Left2D);
    Context->MapKey(LookAction, EKeys::Gamepad_Right2D);

    // Face buttons: A jump, B interact, X dodge, Y build mode.
    Context->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);
    Context->MapKey(InteractAction, EKeys::Gamepad_FaceButton_Right);
    Context->MapKey(DodgeAction, EKeys::Gamepad_FaceButton_Left);
    Context->MapKey(BuildModeAction, EKeys::Gamepad_FaceButton_Top);

    // Shoulders/triggers: RB sprint, LB block, RT attack, LT heavy attack.
    Context->MapKey(SprintAction, EKeys::Gamepad_RightShoulder);
    Context->MapKey(BlockAction, EKeys::Gamepad_LeftShoulder);
    Context->MapKey(AttackAction, EKeys::Gamepad_RightTrigger);
    Context->MapKey(HeavyAttackAction, EKeys::Gamepad_LeftTrigger);

    // D-pad: up command, right feed, down consume, left equip-best.
    Context->MapKey(CommandAction, EKeys::Gamepad_DPad_Up);
    Context->MapKey(FeedAction, EKeys::Gamepad_DPad_Right);
    Context->MapKey(ConsumeAction, EKeys::Gamepad_DPad_Down);
    Context->MapKey(EquipBestAction, EKeys::Gamepad_DPad_Left);

    // Select = rotate building, Start = pause.
    Context->MapKey(BuildRotateAction, EKeys::Gamepad_Special_Left);
    Context->MapKey(PauseAction, EKeys::Gamepad_Special_Right);

    GamepadMappingContext = Context;
    UE_LOG(LogAstrawild, Log, TEXT("Runtime gamepad input mapping built (16 mappings)."));
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
    if (DeleteBuildingAction)
    {
        EnhancedInput->BindAction(DeleteBuildingAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::DeleteBuilding);
    }
    // Final production run: scanner (hold), robotics, UI toggles.
    if (ScanAction)
    {
        EnhancedInput->BindAction(ScanAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::StartScan);
        EnhancedInput->BindAction(ScanAction, ETriggerEvent::Completed, this, &AAstrawildPlayerCharacter::StopScan);
        EnhancedInput->BindAction(ScanAction, ETriggerEvent::Canceled, this, &AAstrawildPlayerCharacter::StopScan);
    }
    if (DeployDroneAction)
    {
        EnhancedInput->BindAction(DeployDroneAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::DeployDrone);
    }
    if (DeployRobotAction)
    {
        EnhancedInput->BindAction(DeployRobotAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::DeployRobot);
    }
    if (InventoryAction)
    {
        EnhancedInput->BindAction(InventoryAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::ToggleInventoryScreenInput);
    }
    if (ResearchAction)
    {
        EnhancedInput->BindAction(ResearchAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::ToggleResearchScreenInput);
    }
    if (PauseAction)
    {
        EnhancedInput->BindAction(PauseAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::TogglePauseMenuInput);
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
    // Batch 4 — M-2a: arm the server-side stamina drain (ticks only while moving;
    // RefreshMovementSpeed still gates on the >0.05 stamina fraction).
    if (SurvivalComponent)
    {
        SurvivalComponent->SetSprintDrainActive(true);
    }
    RefreshMovementSpeed();
}

void AAstrawildPlayerCharacter::StopSprint(const FInputActionValue& Value)
{
    bSprinting = false;
    // Batch 4 — M-2a: sprint released — stop draining, regen resumes next tick.
    if (SurvivalComponent)
    {
        SurvivalComponent->SetSprintDrainActive(false);
    }
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

    // Batch 3 — Item B: stagger zeroes movement until it expires.
    if (CombatComponent && CombatComponent->IsStaggering())
    {
        SetMovementSpeed(0.0f);
        return;
    }

    // Batch 3 — Item A: combined status slow (Chill 0.5, Shock 0.3) multiplies speed.
    if (SurvivalComponent)
    {
        TargetSpeed *= SurvivalComponent->GetStatusSpeedMultiplier();
    }

    // Final production run (PHASE 12): the exosuit adds its fractional speed bonus.
    if (InventoryComponent)
    {
        TargetSpeed *= (1.0f + InventoryComponent->GetEquippedMoveSpeedBonus());
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
    // Batch 3 — Item C: also picks the strongest torso armor by ArmorRating.
    const UAstrawildItemDefinition* BestWeapon = nullptr;
    const UAstrawildItemDefinition* BestShield = nullptr;
    const UAstrawildItemDefinition* BestArmor = nullptr;
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
        if (ItemDef->ArmorRating > 0.0f && (!BestArmor || ItemDef->ArmorRating > BestArmor->ArmorRating))
        {
            BestArmor = ItemDef;
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
    if (BestArmor)
    {
        InventoryComponent->EquipItem(BestArmor->ItemId);
    }
    UE_LOG(LogAstrawildEconomy, Log, TEXT("Equip-best: weapon=%s shield=%s armor=%s."),
        BestWeapon ? *BestWeapon->ItemId.ToString() : TEXT("none"),
        BestShield ? *BestShield->ItemId.ToString() : TEXT("none"),
        BestArmor ? *BestArmor->ItemId.ToString() : TEXT("none"));
}

void AAstrawildPlayerCharacter::DeleteBuilding(const FInputActionValue& Value)
{
    // Batch 2 — Item B: dismantle the building under the crosshair (5m reach) and
    // refund its construction materials via AddItemSilent (no false ItemCollected event).
    if (!IsAlive() || !BuildingComponent || !FollowCamera || !GetWorld())
    {
        return;
    }

    const FVector Start = FollowCamera->GetComponentLocation();
    const FVector End = Start + FollowCamera->GetForwardVector() * 500.0f; // 5m reach
    FHitResult Hit;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ASTRAWILDDismantle), false, this);
    if (!GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Q))
    {
        return;
    }

    AAstrawildBuildingActor* Building = Cast<AAstrawildBuildingActor>(Hit.GetActor());
    if (!Building)
    {
        return;
    }

    const UAstrawildBuildingDefinition* Def = Building->GetBuildingDefinition();
    const bool bOk = BuildingComponent->DismantleBuilding(Building);

    // HUD toast routed through the owning player controller.
    if (APlayerController* PC = Cast<APlayerController>(GetController()))
    {
        if (AAstrawildPlayerController* AstrawildPC = Cast<AAstrawildPlayerController>(PC))
        {
            if (bOk && Def)
            {
                AstrawildPC->Notify(FText::FromString(FString::Printf(
                    TEXT("Dismantled: returned %d %s"),
                    Def->RequiredItemCount, *Def->RequiredItemId.ToString())));
            }
            else if (Def)
            {
                AstrawildPC->Notify(FText::FromString(TEXT("Inventory full — cannot refund")));
            }
        }
    }
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

    // Batch 4 — M-2a: death ends any active sprint-drain (FullRestore refills
    // stamina; a stale drain request would immediately drain it again on respawn).
    bSprinting = false;
    if (SurvivalComponent)
    {
        SurvivalComponent->SetSprintDrainActive(false);
    }

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

void AAstrawildPlayerCharacter::OnStatusSpeedChanged(FName StatusId)
{
    // Batch 3 — Item A: Chill/Shock application or expiry changes the combined multiplier.
    RefreshMovementSpeed();
}

void AAstrawildPlayerCharacter::OnStaggerChanged(bool bIsStaggered, float RemainingSeconds)
{
    // Batch 3 — Item B: entering stagger zeroes speed; leaving restores it.
    RefreshMovementSpeed();
}

void AAstrawildPlayerCharacter::OnBlockingChanged(bool bIsBlocking)
{
    // Batch 4 — M-2b: block movement penalty lives in RefreshMovementSpeed — the
    // blocking state previously changed server-side with no listener, so the
    // ×0.45 penalty never applied (and never lifted either).
    RefreshMovementSpeed();
}

void AAstrawildPlayerCharacter::OnSprintExhausted()
{
    // Batch 4 — M-2a: the stamina floor was hit while sprinting — clear the sprint
    // state so speed falls back to walking. The >0.05 stamina gate in
    // RefreshMovementSpeed keeps re-sprint suppressed until stamina recovers.
    bSprinting = false;
    if (SurvivalComponent)
    {
        SurvivalComponent->SetSprintDrainActive(false);
    }
    RefreshMovementSpeed();
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

// --- Final production run: scanner, robotics, UI input handlers ---

void AAstrawildPlayerCharacter::StartScan(const FInputActionValue& Value)
{
    // Hold-to-scan: only works with a scanner equipped (PHASE 12 framework).
    if (!InventoryComponent || InventoryComponent->EquippedScannerItemId.IsNone())
    {
        return;
    }

    UWorld* World = GetWorld();
    UAstrawildJournalSubsystem* Journal = World ? World->GetSubsystem<UAstrawildJournalSubsystem>() : nullptr;
    if (!Journal || !InventoryComponent)
    {
        return;
    }

    float Multiplier = 2.0f; // Fallback if the definition cannot resolve.
    if (const UAstrawildItemRegistrySubsystem* Registry = World->GetSubsystem<UAstrawildItemRegistrySubsystem>())
    {
        if (const UAstrawildItemDefinition* ScannerDef = Registry->FindItem(InventoryComponent->EquippedScannerItemId))
        {
            Multiplier = ScannerDef->ScannerSpeedMultiplier;
        }
    }

    bScanKeyHeld = true;
    Journal->BeginActiveScan(this, Multiplier);
}

void AAstrawildPlayerCharacter::StopScan(const FInputActionValue& Value)
{
    if (!bScanKeyHeld)
    {
        return;
    }
    bScanKeyHeld = false;

    if (UWorld* World = GetWorld())
    {
        if (UAstrawildJournalSubsystem* Journal = World->GetSubsystem<UAstrawildJournalSubsystem>())
        {
            Journal->EndActiveScan();
        }
    }
}

AAstrawildUtilityDroneActor* AAstrawildPlayerCharacter::SpawnUtilityDrone()
{
    UWorld* World = GetWorld();
    if (!World || GetLocalRole() != ROLE_Authority)
    {
        return nullptr;
    }

    // Recall path: one drone per player — pressing H again recalls it (refund item? no —
    // the drone is re-deployable for free within the session; the save system tracks it).
    if (AAstrawildUtilityDroneActor* Existing = ActiveDrone.Get())
    {
        Existing->Destroy();
        ActiveDrone = nullptr;
        if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(GetController()))
        {
            PC->Notify(FText::FromString(TEXT("Drone recalled.")));
        }
        return nullptr;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    AAstrawildUtilityDroneActor* Drone = World->SpawnActor<AAstrawildUtilityDroneActor>(
        AAstrawildUtilityDroneActor::StaticClass(), GetActorLocation(), GetActorRotation(), Params);
    if (Drone)
    {
        Drone->InitializeForOwner(this);
        ActiveDrone = Drone;

        if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
        {
            EventBus->PublishEvent(TAG_Astrawild_Event_DroneDeployed, this, GetFName(), 1, GetActorLocation());
        }
        if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(GetController()))
        {
            PC->Notify(FText::FromString(TEXT("Utility drone online — it scans and harvests for you.")));
        }
    }
    return Drone;
}

bool AAstrawildPlayerCharacter::SpawnUtilityRobot()
{
    UWorld* World = GetWorld();
    if (!World || GetLocalRole() != ROLE_Authority)
    {
        return false;
    }

    // Find the nearest work site without a robot.
    AAstrawildWorkSiteActor* Best = nullptr;
    float BestDistance = 8000.0f;
    for (TActorIterator<AAstrawildWorkSiteActor> It(World); It; ++It)
    {
        AAstrawildWorkSiteActor* Site = *It;
        if (!Site || Site->HasRobot())
        {
            continue;
        }
        const float Distance = FVector::Dist(GetActorLocation(), Site->GetActorLocation());
        if (Distance < BestDistance)
        {
            BestDistance = Distance;
            Best = Site;
        }
    }

    if (!Best)
    {
        if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(GetController()))
        {
            PC->Notify(FText::FromString(TEXT("No unmanned work site nearby.")));
        }
        return false;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    AAstrawildUtilityRobotActor* Robot = World->SpawnActor<AAstrawildUtilityRobotActor>(
        AAstrawildUtilityRobotActor::StaticClass(), GetActorLocation(), GetActorRotation(), Params);
    if (!Robot)
    {
        return false;
    }

    Robot->SetOwnerPlayerId(GetFName());
    Robot->AssignToSite(Best);

    if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
    {
        EventBus->PublishEvent(TAG_Astrawild_Event_RobotDeployed, this, Best->SiteId, 1, GetActorLocation());
    }
    if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(GetController()))
    {
        PC->Notify(FText::FromString(FString::Printf(TEXT("Utility robot assigned to %s."),
            *UEnum::GetDisplayValueAsText(Best->WorkType).ToString())));
    }
    return true;
}

void AAstrawildPlayerCharacter::DeployDrone(const FInputActionValue& Value)
{
    if (!IsAlive() || !InventoryComponent)
    {
        return;
    }

    // Recall is free; a fresh deploy consumes the drone item.
    if (ActiveDrone.IsValid())
    {
        SpawnUtilityDrone();
        return;
    }

    if (!InventoryComponent->HasItem(TEXT("Item_UtilityDrone"), 1))
    {
        if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(GetController()))
        {
            PC->Notify(FText::FromString(TEXT("No utility drone in the pack (craft one at the Auto-Forge).")));
        }
        return;
    }

    FAstrawildItemStack Cost;
    Cost.ItemId = TEXT("Item_UtilityDrone");
    Cost.Quantity = 1;
    if (InventoryComponent->ConsumeItems(TArray<FAstrawildItemStack>{Cost}))
    {
        SpawnUtilityDrone();
    }
}

void AAstrawildPlayerCharacter::DeployRobot(const FInputActionValue& Value)
{
    if (!IsAlive() || !InventoryComponent)
    {
        return;
    }

    if (!InventoryComponent->HasItem(TEXT("Item_UtilityRobot"), 1))
    {
        if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(GetController()))
        {
            PC->Notify(FText::FromString(TEXT("No utility robot in the pack (craft one at the Auto-Forge).")));
        }
        return;
    }

    FAstrawildItemStack Cost;
    Cost.ItemId = TEXT("Item_UtilityRobot");
    Cost.Quantity = 1;
    if (InventoryComponent->ConsumeItems(TArray<FAstrawildItemStack>{Cost}))
    {
        SpawnUtilityRobot();
    }
}

void AAstrawildPlayerCharacter::ToggleInventoryScreenInput(const FInputActionValue& Value)
{
    if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(GetController()))
    {
        PC->ToggleInventoryScreen();
    }
}

void AAstrawildPlayerCharacter::ToggleResearchScreenInput(const FInputActionValue& Value)
{
    if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(GetController()))
    {
        PC->ToggleResearchScreen();
    }
}

void AAstrawildPlayerCharacter::TogglePauseMenuInput(const FInputActionValue& Value)
{
    if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(GetController()))
    {
        PC->TogglePauseMenu();
    }
}
