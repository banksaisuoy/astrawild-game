#include "AstrawildPlayerCharacter.h"

#include "AstrawildBuildingActor.h"
#include "AstrawildBuildingComponent.h"
#include "AstrawildCaptureComponent.h"
#include "AstrawildCombatComponent.h"
#include "AstrawildCore.h"
#include "AstrawildCraftingComponent.h"
#include "AstrawildDataAssets.h"
#include "AstrawildArtPack.h"
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
#include "AstrawildSkiffActor.h"
#include "AstrawildSaveSubsystem.h"
#include "AstrawildSurvivalComponent.h"
#include "AstrawildUtilityDroneActor.h"
#include "AstrawildUtilityRobotActor.h"
#include "AstrawildVfxActor.h"
#include "AstrawildWorkSiteActor.h"
#include "Camera/CameraComponent.h"
#include "Animation/AnimSequenceBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Materials/Material.h"
#include "ProceduralMeshComponent.h"
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

    // Art pack soft bindings (Batch 4): AstrawildArtPack is the single source of
    // truth — the designer can still override per-archetype in a BP subclass.
    {
        const AstrawildArtPack::FSurvivorArt& Art = AstrawildArtPack::GetSurvivorArt();
        SurvivorSkeletalMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(Art.MeshPath));
        SurvivorIdleAnim = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Art.IdleAnimPath));
        SurvivorWalkAnim = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Art.WalkAnimPath));
        SurvivorRunAnim = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Art.RunAnimPath));
        SurvivorJumpAnim = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Art.JumpAnimPath));
        SurvivorAimAnim = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Art.AimAnimPath));
        SurvivorFireAnim = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Art.FireAnimPath));
        SurvivorGatherAnim = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Art.GatherAnimPath));
    }

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

    // Production V2 Batch 2: procedural survivor body + held weapon mesh.
    BodyMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("BodyMesh"));
    BodyMesh->SetupAttachment(RootComponent);
    BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BodyMesh->bVisibleInRayTracing = false;

    WeaponMesh = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(RootComponent);
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponMesh->SetCastShadow(false);
    WeaponMesh->bVisibleInRayTracing = false;
    WeaponMesh->SetRelativeLocation(FVector(38.0f, 24.0f, 58.0f));
    WeaponMesh->SetVisibility(false); // shown once a weapon is equipped

    InventoryComponent = CreateDefaultSubobject<UAstrawildInventoryComponent>(TEXT("Inventory"));
    CraftingComponent = CreateDefaultSubobject<UAstrawildCraftingComponent>(TEXT("Crafting"));
    CaptureComponent = CreateDefaultSubobject<UAstrawildCaptureComponent>(TEXT("Capture"));
    SurvivalComponent = CreateDefaultSubobject<UAstrawildSurvivalComponent>(TEXT("Survival"));
    CombatComponent = CreateDefaultSubobject<UAstrawildCombatComponent>(TEXT("Combat"));
    BuildingComponent = CreateDefaultSubobject<UAstrawildBuildingComponent>(TEXT("Building"));

    // Audit C-3: broad navmesh generation around the player covers the camp, the
    // arena interior and the dungeon approach in the zero-asset world.
    NavInvoker = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavInvoker"));
    NavInvoker->SetGenerationRadii(12000.0f, 16000.0f);

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

    // Production V2 Batch 2: survivor silhouette (local build — runs on server
    // AND owning client so the body reads in every netmode).
    BuildProceduralBody();

    // Art pack (Batch 4): swap to the skinned exosuit when the pack is imported.
    // The registry warm pass pre-loads the soft refs, so this is one synchronous
    // resolution on a warmed pointer in the common case (and a cheap miss when
    // the pack is absent — the PMC silhouette stays live, zero-asset rule).
    bSkeletalBodyActive = TryActivateSkeletalBody();

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

    // Production V2 Batch 2: poll equipped weapons at a gentle cadence so the
    // held-weapon silhouette follows equipment changes without plumbing every
    // equip path through the inventory.
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(HeldWeaponTimerHandle, this,
            &AAstrawildPlayerCharacter::RefreshHeldWeaponVisual, 0.5f, true);
        RefreshHeldWeaponVisual();
    }
}

// ---------------------------------------------------------------------------
// Art pack animation driver (Batch 4, CP-08) — code-driven locomotion over the
// imported AM_Survivor_* clips (single-node mode: no AnimBP required).
// ---------------------------------------------------------------------------
bool AAstrawildPlayerCharacter::TryActivateSkeletalBody()
{
    USkeletalMesh* Mesh = SurvivorSkeletalMesh.LoadSynchronous();
    if (!Mesh)
    {
        return false; // pack not imported — PMC silhouette stays live
    }

    SurvivorBody = NewObject<USkeletalMeshComponent>(this, TEXT("SurvivorBody"));
    if (!SurvivorBody)
    {
        return false;
    }
    SurvivorBody->SetupAttachment(GetCapsuleComponent());
    SurvivorBody->SetSkeletalMesh(Mesh);
    SurvivorBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SurvivorBody->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    SurvivorBody->SetRelativeLocation(FVector::ZeroVector);
    SurvivorBody->SetRelativeRotation(FRotator::ZeroRotator);
    SurvivorBody->RegisterComponent();

    // The PMC body + placeholder cylinder retire while the skinned body lives.
    if (BodyMesh)
    {
        BodyMesh->SetVisibility(false);
    }
    if (PlaceholderMesh)
    {
        PlaceholderMesh->SetVisibility(false);
    }

    PrimaryActorTick.bCanEverTick = true;
    SetActorTickEnabled(true);

    // Warm the locomotion clips (registry warm pass usually already did).
    SurvivorIdleAnim.LoadSynchronous();
    SurvivorWalkAnim.LoadSynchronous();
    SurvivorRunAnim.LoadSynchronous();
    SurvivorAimAnim.LoadSynchronous();
    SurvivorJumpAnim.LoadSynchronous();
    SurvivorFireAnim.LoadSynchronous();
    SurvivorGatherAnim.LoadSynchronous();

    UE_LOG(LogAstrawild, Log,
        TEXT("Survivor art pack active: skinned exosuit %s replaces the PMC silhouette."),
        *SurvivorSkeletalMesh.ToSoftObjectPath().ToString());
    return true;
}

void AAstrawildPlayerCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bSkeletalBodyActive)
    {
        UpdateSurvivorAnimation();
    }
}

void AAstrawildPlayerCharacter::UpdateSurvivorAnimation()
{
    if (!SurvivorBody)
    {
        return;
    }

    UAnimSequenceBase* Target = nullptr;
    const float Speed = GetVelocity().Size();
    if (bGuardPose && Speed < 450.0f)
    {
        Target = SurvivorAimAnim.Get();
    }
    else if (Speed < 60.0f)
    {
        Target = SurvivorIdleAnim.Get();
    }
    else if (Speed < 480.0f)
    {
        Target = SurvivorWalkAnim.Get();
    }
    else
    {
        Target = SurvivorRunAnim.Get();
    }

    if (Target && Target != CurrentLoopAnimation)
    {
        SurvivorBody->PlayAnimation(Target, true);
        CurrentLoopAnimation = Target;
    }
    else if (!Target && CurrentLoopAnimation)
    {
        // Clips absent — hold the bind pose rather than stutter.
        SurvivorBody->Stop();
        CurrentLoopAnimation = nullptr;
    }
}

void AAstrawildPlayerCharacter::PlaySurvivorOneShot(UAnimSequenceBase* Sequence, float Duration)
{
    if (!bSkeletalBodyActive || !SurvivorBody || !Sequence)
    {
        return;
    }
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(SurvivorOneShotTimer);
        World->GetTimerManager().SetTimer(SurvivorOneShotTimer, this,
            &AAstrawildPlayerCharacter::OnSurvivorOneShotFinished, FMath::Max(0.1f, Duration), false);
    }
    SurvivorBody->PlayAnimation(Sequence, false);
    CurrentLoopAnimation = nullptr;
}

void AAstrawildPlayerCharacter::OnSurvivorOneShotFinished()
{
    CurrentLoopAnimation = nullptr;
    UpdateSurvivorAnimation();
}

void AAstrawildPlayerCharacter::SetGuardPose(bool bEnabled)
{
    if (bGuardPose == bEnabled)
    {
        return;
    }
    bGuardPose = bEnabled;
    if (bSkeletalBodyActive && bEnabled && SurvivorAimAnim.Get())
    {
        CurrentLoopAnimation = nullptr; // force stance change next tick
        UpdateSurvivorAnimation();
    }
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
    // Batch 8 — skiff descend: CTRL held (SPACE climbs through JumpAction).
    DescendAction = MakeRuntimeAction(TEXT("AWD_Descend"), static_cast<uint8>(EInputActionValueType::Boolean));
    Context->MapKey(DescendAction, EKeys::LeftControl);

    DefaultMappingContext = Context;
    UE_LOG(LogAstrawild, Log, TEXT("Runtime default input mapping built (26 actions, WASD+mouse+wheel+UI+skiff)."));
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
    // Batch 8 — LS click = skiff descend (A button climbs through JumpAction).
    if (DescendAction)
    {
        Context->MapKey(DescendAction, EKeys::Gamepad_LeftThumbstick);
    }
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
        EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::OnJumpPressed);
        EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &AAstrawildPlayerCharacter::OnJumpReleased);
        EnhancedInput->BindAction(JumpAction, ETriggerEvent::Canceled, this, &AAstrawildPlayerCharacter::OnJumpReleased);
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
    if (DescendAction)
    {
        // Batch 8 — skiff descend (CTRL / LS click).
        EnhancedInput->BindAction(DescendAction, ETriggerEvent::Started, this, &AAstrawildPlayerCharacter::StartDescend);
        EnhancedInput->BindAction(DescendAction, ETriggerEvent::Completed, this, &AAstrawildPlayerCharacter::StopDescend);
        EnhancedInput->BindAction(DescendAction, ETriggerEvent::Canceled, this, &AAstrawildPlayerCharacter::StopDescend);
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

    // Batch 8 — piloting a skiff: WASD drives the aircraft instead of the pawn.
    if (AAstrawildSkiffActor* Skiff = PilotedSkiff.Get())
    {
        Skiff->ReceivePilotMove(MovementVector.Y, MovementVector.X);
        return;
    }

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

    // Batch 8 — SHIFT while piloting = resonance boost.
    if (AAstrawildSkiffActor* Skiff = PilotedSkiff.Get())
    {
        Skiff->ReceivePilotBoost(true);
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
    // Batch 8 — boost release routes to the skiff even mid-flight.
    if (AAstrawildSkiffActor* Skiff = PilotedSkiff.Get())
    {
        Skiff->ReceivePilotBoost(false);
        return;
    }

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
    if (!IsAlive())
    {
        return;
    }

    // Batch 8 — SPACE while piloting = climb.
    if (AAstrawildSkiffActor* Skiff = PilotedSkiff.Get())
    {
        Skiff->ReceivePilotVertical(1.0f);
        return;
    }

    // Art pack (Batch 4): jump one-shot (skips if the clip is not imported).
    PlaySurvivorOneShot(SurvivorJumpAnim.Get(), 0.9f);
    Jump();
}

void AAstrawildPlayerCharacter::OnJumpPressed(const FInputActionValue& Value)
{
    HandleJump(Value);
}

void AAstrawildPlayerCharacter::OnJumpReleased(const FInputActionValue& Value)
{
    // Batch 8 — climb release (only meaningful while piloting).
    if (AAstrawildSkiffActor* Skiff = PilotedSkiff.Get())
    {
        Skiff->ReceivePilotVertical(0.0f);
        return;
    }

    StopJumping();
}

void AAstrawildPlayerCharacter::StartDescend(const FInputActionValue& Value)
{
    // Batch 8 — CTRL while piloting = descend (no-op on foot).
    if (AAstrawildSkiffActor* Skiff = PilotedSkiff.Get())
    {
        Skiff->ReceivePilotVertical(-1.0f);
    }
}

void AAstrawildPlayerCharacter::StopDescend(const FInputActionValue& Value)
{
    if (AAstrawildSkiffActor* Skiff = PilotedSkiff.Get())
    {
        Skiff->ReceivePilotVertical(0.0f);
    }
}

void AAstrawildPlayerCharacter::Interact(const FInputActionValue& Value)
{
    if (!IsAlive())
    {
        return;
    }

    // Art pack (Batch 4): gather/interact one-shot before the trace (skipped
    // while piloting and when the clip is not imported).
    if (!PilotedSkiff.IsValid())
    {
        PlaySurvivorOneShot(SurvivorGatherAnim.Get(), 1.0f);
    }

    // Batch 8 — E while piloting = dismount (before any trace).
    if (AAstrawildSkiffActor* Skiff = PilotedSkiff.Get())
    {
        Skiff->DismountPilot();
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

    // Art pack (Batch 4): fire recoil one-shot (fires on melee too — reads as a
    // swing; skips when the clip is absent).
    PlaySurvivorOneShot(SurvivorFireAnim.Get(), 0.35f);
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
        SetGuardPose(true);
    }
}

void AAstrawildPlayerCharacter::StopBlock(const FInputActionValue& Value)
{
    SetGuardPose(false);
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

namespace
{
    /** Player body part builder helpers — the Echo body idiom, local variant. */
    struct FAstrawildPlayerBodyPart
    {
        TArray<FVector> Vertices;
        TArray<int32> Triangles;
        TArray<FVector> Normals;
        TArray<FVector2D> UVs;
        TArray<FColor> Colors;
    };

    void PushPlayerQuad(FAstrawildPlayerBodyPart& Part, const int32 A, const int32 B, const int32 C, const int32 D)
    {
        Part.Triangles.Append({ A, B, C, A, C, D });
    }

    void AddPlayerBox(FAstrawildPlayerBodyPart& Part, const FVector& Center, const FVector& HalfSize, const FColor& Color)
    {
        const int32 Base = Part.Vertices.Num();
        const FVector Corners[8] = {
            Center + FVector(-HalfSize.X, -HalfSize.Y, -HalfSize.Z),
            Center + FVector( HalfSize.X, -HalfSize.Y, -HalfSize.Z),
            Center + FVector( HalfSize.X,  HalfSize.Y, -HalfSize.Z),
            Center + FVector(-HalfSize.X,  HalfSize.Y, -HalfSize.Z),
            Center + FVector(-HalfSize.X, -HalfSize.Y,  HalfSize.Z),
            Center + FVector( HalfSize.X, -HalfSize.Y,  HalfSize.Z),
            Center + FVector( HalfSize.X,  HalfSize.Y,  HalfSize.Z),
            Center + FVector(-HalfSize.X,  HalfSize.Y,  HalfSize.Z),
        };
        for (const FVector& Corner : Corners)
        {
            Part.Vertices.Add(Corner);
            Part.Normals.Add((Corner - Center).GetSafeNormal());
            Part.UVs.Add(FVector2D(0.0f, 0.0f));
            Part.Colors.Add(Color);
        }
        PushPlayerQuad(Part, Base + 0, Base + 1, Base + 2, Base + 3); // bottom
        PushPlayerQuad(Part, Base + 4, Base + 7, Base + 6, Base + 5); // top
        PushPlayerQuad(Part, Base + 0, Base + 4, Base + 5, Base + 1); // front
        PushPlayerQuad(Part, Base + 3, Base + 2, Base + 6, Base + 7); // back
        PushPlayerQuad(Part, Base + 1, Base + 5, Base + 6, Base + 2); // right
        PushPlayerQuad(Part, Base + 0, Base + 3, Base + 7, Base + 4); // left
    }

    void AddPlayerSphere(FAstrawildPlayerBodyPart& Part, const FVector& Center, const float Radius, const FColor& Color,
        const int32 Segments = 9)
    {
        const int32 Rings = FMath::Max(4, Segments);
        const int32 Slices = FMath::Max(4, Segments);
        const int32 Base = Part.Vertices.Num();
        for (int32 Ring = 0; Ring <= Rings; ++Ring)
        {
            const float Phi = PI * static_cast<float>(Ring) / static_cast<float>(Rings);
            for (int32 Slice = 0; Slice <= Slices; ++Slice)
            {
                const float Theta = 2.0f * PI * static_cast<float>(Slice) / static_cast<float>(Slices);
                const FVector Normal(
                    FMath::Sin(Phi) * FMath::Cos(Theta),
                    FMath::Sin(Phi) * FMath::Sin(Theta),
                    FMath::Cos(Phi));
                Part.Vertices.Add(Center + Normal * Radius);
                Part.Normals.Add(Normal);
                Part.UVs.Add(FVector2D(static_cast<float>(Slice) / Slices, static_cast<float>(Ring) / Rings));
                Part.Colors.Add(Color);
            }
        }
        for (int32 Ring = 0; Ring < Rings; ++Ring)
        {
            for (int32 Slice = 0; Slice < Slices; ++Slice)
            {
                const int32 A = Base + Ring * (Slices + 1) + Slice;
                const int32 B = Base + (Ring + 1) * (Slices + 1) + Slice;
                const int32 C = Base + (Ring + 1) * (Slices + 1) + Slice + 1;
                const int32 D = Base + Ring * (Slices + 1) + Slice + 1;
                Part.Triangles.Append({ A, B, C, A, C, D });
            }
        }
    }

    void AddPlayerCylinder(FAstrawildPlayerBodyPart& Part, const FVector& Center, const float Radius, const float HalfLength,
        const FColor& Color, const int32 Sides = 8)
    {
        const int32 Base = Part.Vertices.Num();
        for (int32 Side = 0; Side <= Sides; ++Side)
        {
            const float Theta = 2.0f * PI * static_cast<float>(Side) / static_cast<float>(Sides);
            const FVector Dir(FMath::Cos(Theta), FMath::Sin(Theta), 0.0f);
            Part.Vertices.Add(Center + FVector(0, 0, -HalfLength) + Dir * Radius);
            Part.Normals.Add(Dir);
            Part.UVs.Add(FVector2D(0.0f, 0.0f));
            Part.Colors.Add(Color);
            Part.Vertices.Add(Center + FVector(0, 0, HalfLength) + Dir * Radius);
            Part.Normals.Add(Dir);
            Part.UVs.Add(FVector2D(0.0f, 1.0f));
            Part.Colors.Add(Color);
        }
        for (int32 Side = 0; Side < Sides; ++Side)
        {
            const int32 A = Base + Side * 2;
            const int32 B = Base + Side * 2 + 1;
            const int32 C = Base + Side * 2 + 3;
            const int32 D = Base + Side * 2 + 2;
            Part.Triangles.Append({ A, B, C, A, C, D });
        }
    }

    UMaterial* LoadPlayerBodyMaterial()
    {
        UMaterial* Material = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineDebugMaterials/DebugMeshMaterial.DebugMeshMaterial"));
        if (!Material)
        {
            Material = LoadObject<UMaterial>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial"));
        }
        return Material;
    }
}

void AAstrawildPlayerCharacter::BuildProceduralBody()
{
    if (!BodyMesh)
    {
        return;
    }

    // ASTRAWILD frontier-survivor palette: graphite suit, amber accents, teal visor.
    const FColor SuitDark(58, 60, 66, 255);
    const FColor SuitMid(78, 80, 88, 255);
    const FColor Amber(232, 152, 48, 255);
    const FColor VisorTeal(74, 220, 200, 255);
    const FColor HelmetGrey(128, 130, 134, 255);

    FAstrawildPlayerBodyPart Body;

    // Torso + chest plate.
    AddPlayerBox(Body, FVector(0, 0, 100), FVector(17, 12, 26), SuitDark);
    AddPlayerBox(Body, FVector(9, 0, 108), FVector(8, 9, 13), Amber);

    // Head + visor.
    AddPlayerSphere(Body, FVector(0, 0, 148), 13.5f, HelmetGrey);
    AddPlayerBox(Body, FVector(10, 0, 149), FVector(4, 9, 6), VisorTeal);

    // Backpack (scavenger gear) + strap.
    AddPlayerBox(Body, FVector(-19, 0, 106), FVector(7, 13, 17), SuitMid);
    AddPlayerBox(Body, FVector(-12, 0, 120), FVector(2, 11, 3), Amber);

    // Legs + arms.
    AddPlayerBox(Body, FVector(0, 8, 55), FVector(7, 7, 20), SuitMid);
    AddPlayerBox(Body, FVector(0, -8, 55), FVector(7, 7, 20), SuitMid);
    AddPlayerBox(Body, FVector(0, 21, 100), FVector(6, 5, 23), SuitMid);
    AddPlayerBox(Body, FVector(0, -21, 100), FVector(6, 5, 23), SuitMid);

    if (Body.Vertices.Num() > 0)
    {
        BodyMesh->CreateMeshSection(0, Body.Vertices, Body.Triangles, Body.Normals, Body.UVs, Body.Colors,
            TArray<FProcMeshTangent>(), false);
        if (UMaterial* Material = LoadPlayerBodyMaterial())
        {
            BodyMesh->SetMaterial(0, Material);
        }

        // The silhouette replaces the legacy grey cylinder.
        if (PlaceholderMesh)
        {
            PlaceholderMesh->SetVisibility(false);
        }
    }
}

void AAstrawildPlayerCharacter::RefreshHeldWeaponVisual()
{
    if (!WeaponMesh || !CombatComponent)
    {
        return;
    }

    const UAstrawildWeaponDefinition* WeaponDef = CombatComponent->GetEquippedWeaponDefinition();
    const FName WeaponId = WeaponDef ? WeaponDef->WeaponId : NAME_None;
    if (WeaponId == LastHeldWeaponId)
    {
        return; // nothing changed since the last poll
    }
    LastHeldWeaponId = WeaponId;

    // Art pack (Batch 4, CP-03): real weapon static mesh on the hand socket when
    // both the skinned body and the weapon mesh resolve. Falls through to the
    // PMC silhouette otherwise (zero-asset rule).
    if (WeaponDef && bSkeletalBodyActive && SurvivorBody)
    {
        UStaticMesh* WeaponMeshAsset = WeaponDef->Mesh.Get();
        if (WeaponMeshAsset)
        {
            if (WeaponMesh)
            {
                WeaponMesh->SetVisibility(false); // retire the PMC gun
            }
            if (!HeldWeaponMesh)
            {
                HeldWeaponMesh = NewObject<UStaticMeshComponent>(this, TEXT("HeldWeaponMesh"));
                HeldWeaponMesh->SetupAttachment(SurvivorBody);
                HeldWeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                HeldWeaponMesh->SetCastShadow(true);
                HeldWeaponMesh->RegisterComponent();
            }
            HeldWeaponMesh->AttachToComponent(SurvivorBody,
                FAttachmentTransformRules::SnapToTargetNotIncludingScale, TEXT("Weapon_R"));
            HeldWeaponMesh->SetStaticMesh(WeaponMeshAsset);

            float TierScale = 1.0f;
            switch (WeaponDef->Tier)
            {
            case EAstrawildTechTier::Field:         TierScale = 1.0f; break;
            case EAstrawildTechTier::Mk1:           TierScale = 1.05f; break;
            case EAstrawildTechTier::Mk2:           TierScale = 1.15f; break;
            case EAstrawildTechTier::Mk3:           TierScale = 1.25f; break;
            case EAstrawildTechTier::Experimental:  TierScale = 1.35f; break;
            default: break;
            }
            HeldWeaponMesh->SetRelativeScale3D(FVector(TierScale));
            HeldWeaponMesh->SetVisibility(true);
            return;
        }
    }
    if (HeldWeaponMesh)
    {
        HeldWeaponMesh->SetVisibility(false);
    }

    if (!WeaponDef)
    {
        WeaponMesh->SetVisibility(false);
        WeaponMesh->ClearAllMeshSections();
        return;
    }

    // Family identity tint + tier scale — the held silhouette telegraphs the
    // weapon's firing behavior (kinetic grey, plasma magenta, arc electric...).
    const FLinearColor Tint = FAstrawildVfxPalette::GetWeaponFamilyTint(WeaponDef->Family);
    const FColor Body = FLinearColor(Tint.R * 0.55f + 0.10f, Tint.G * 0.55f + 0.10f, Tint.B * 0.55f + 0.10f, 1.0f).ToFColor(false);
    const FColor Accents = Tint.ToFColor(false);

    float TierScale = 1.0f;
    switch (WeaponDef->Tier)
    {
    case EAstrawildTechTier::Mk1:          TierScale = 1.1f; break;
    case EAstrawildTechTier::Mk2:          TierScale = 1.25f; break;
    case EAstrawildTechTier::Mk3:          TierScale = 1.4f; break;
    case EAstrawildTechTier::Experimental: TierScale = 1.6f; break;
    default: break;
    }

    FAstrawildPlayerBodyPart Gun;
    // Receiver, grip, energy cell, barrel — sized in tier scale.
    AddPlayerBox(Gun, FVector(0, 0, 0), FVector(15 * TierScale, 4.5f * TierScale, 6 * TierScale), Body);
    AddPlayerBox(Gun, FVector(-9 * TierScale, 0, -7 * TierScale), FVector(3.5f * TierScale, 3.5f * TierScale, 7 * TierScale), Body);
    AddPlayerBox(Gun, FVector(-2 * TierScale, 0, 8 * TierScale), FVector(5 * TierScale, 3.5f * TierScale, 3.5f * TierScale), Accents);
    AddPlayerCylinder(Gun, FVector(17 * TierScale, 0, 0), 3.2f * TierScale, 13 * TierScale, Accents, 7);

    WeaponMesh->ClearAllMeshSections();
    if (Gun.Vertices.Num() > 0)
    {
        WeaponMesh->CreateMeshSection(0, Gun.Vertices, Gun.Triangles, Gun.Normals, Gun.UVs, Gun.Colors,
            TArray<FProcMeshTangent>(), false);
        if (UMaterial* Material = LoadPlayerBodyMaterial())
        {
            WeaponMesh->SetMaterial(0, Material);
        }
        WeaponMesh->SetVisibility(true);
    }
}

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

    // Production V2 Batch 2: world-space pulse ring — the scan now reads as an
    // action (tier-tinted; radius tracks the scanner's effective range).
    if (World)
    {
        const FLinearColor PulseTint = FAstrawildVfxPalette::GetScannerTint(InventoryComponent->EquippedScannerItemId);
        const float RangeMultiplier = InventoryComponent->GetEquippedScannerRangeMultiplier();
        AAstrawildScannerPulseActor::SpawnPulse(World, GetActorLocation(), PulseTint,
            2400.0f * FMath::Max(1.0f, RangeMultiplier));
    }
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

bool AAstrawildPlayerCharacter::SpawnUtilityRobot(FName RobotDefinitionOverride)
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
    // Production V2: specialist chassis resolve their profile from the registry.
    if (!RobotDefinitionOverride.IsNone())
    {
        if (UAstrawildItemRegistrySubsystem* Registry = World->GetSubsystem<UAstrawildItemRegistrySubsystem>())
        {
            if (UAstrawildRobotDefinition* RobotDef = Registry->FindRobot(RobotDefinitionOverride))
            {
                Robot->InitializeFromDefinition(RobotDef);
            }
        }
    }
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

    // Production V2 (Master Plan §12): specialist chassis deploy first when
    // carried (mining/farming/defense frames), the general frame is the fallback.
    static const FName RobotItemIds[] =
    {
        TEXT("Item_RobotBorebot"),
        TEXT("Item_RobotCultivator"),
        TEXT("Item_RobotSentinel"),
        TEXT("Item_UtilityRobot")
    };

    FName ChosenItem = NAME_None;
    FName ChosenDefinition = NAME_None;
    for (const FName ItemId : RobotItemIds)
    {
        if (InventoryComponent->HasItem(ItemId, 1))
        {
            ChosenItem = ItemId;
            ChosenDefinition = ResolveRobotDefinitionIdForItem(ItemId);
            break;
        }
    }

    if (ChosenItem.IsNone())
    {
        if (AAstrawildPlayerController* PC = Cast<AAstrawildPlayerController>(GetController()))
        {
            PC->Notify(FText::FromString(TEXT("No utility robot in the pack (craft one at the Auto-Forge).")));
        }
        return;
    }

    FAstrawildItemStack Cost;
    Cost.ItemId = ChosenItem;
    Cost.Quantity = 1;
    if (InventoryComponent->ConsumeItems(TArray<FAstrawildItemStack>{Cost}))
    {
        SpawnUtilityRobot(ChosenDefinition);
    }
}

FName AAstrawildPlayerCharacter::ResolveRobotDefinitionIdForItem(const FName RobotItemId) const
{
    // Robot chassis id comes from the item definition (data-driven mapping).
    const UWorld* World = GetWorld();
    const UAstrawildItemRegistrySubsystem* Registry = World ? World->GetSubsystem<UAstrawildItemRegistrySubsystem>() : nullptr;
    const UAstrawildItemDefinition* ItemDef = Registry ? Registry->FindItem(RobotItemId) : nullptr;
    return (ItemDef && !ItemDef->RobotDefinitionId.IsNone()) ? ItemDef->RobotDefinitionId : NAME_None;
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

AAstrawildSkiffActor* AAstrawildPlayerCharacter::GetPilotedSkiff() const
{
    return PilotedSkiff.Get();
}

void AAstrawildPlayerCharacter::SetPilotedSkiff(AAstrawildSkiffActor* Skiff)
{
    PilotedSkiff = Skiff;
}
