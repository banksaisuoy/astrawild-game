#include "AstrawildEchoCharacter.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoAIController.h"
#include "AstrawildEcosystemSubsystem.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildGameState.h"
#include "AstrawildLog.h"
#include "AstrawildTimeSubsystem.h"
#include "AstrawildWorkSiteActor.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "NavigationInvokerComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

AAstrawildEchoCharacter::AAstrawildEchoCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    InstanceId = FGuid::NewGuid();

    bReplicates = true;
    SetReplicatingMovement(true);

    // Server-driven C++ AI that works without Behavior Tree assets (directive §6);
    // future BT/StateTree assets can possess through the same controller.
    AIControllerClass = AAstrawildEchoAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    PlaceholderMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PlaceholderMesh"));
    PlaceholderMesh->SetupAttachment(GetCapsuleComponent());
    PlaceholderMesh->SetCollisionProfileName(TEXT("NoCollision"));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMesh.Succeeded())
    {
        PlaceholderMesh->SetStaticMesh(SphereMesh.Object);
        PlaceholderMesh->SetWorldScale3D(FVector(0.8f));
    }

    // Audit C-3: runtime navmesh generation anchor — without an authored navmesh in
    // the zero-asset world, this invoker makes tiles generate around the creature so
    // all MoveTo* pathfinding works (project setting: navigation generation around
    // invokers only).
    NavInvoker = CreateDefaultSubobject<UNavigationInvokerComponent>(TEXT("NavInvoker"));
    NavInvoker->SetRadii(5000.0f, 7000.0f);
}

void AAstrawildEchoCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAstrawildEchoCharacter, Personality);
    DOREPLIFETIME(AAstrawildEchoCharacter, Needs);
    DOREPLIFETIME(AAstrawildEchoCharacter, Experience);
    DOREPLIFETIME(AAstrawildEchoCharacter, CurrentAIState);
    DOREPLIFETIME(AAstrawildEchoCharacter, ActiveCommand);
    DOREPLIFETIME(AAstrawildEchoCharacter, OwnerPlayerId);
}

void AAstrawildEchoCharacter::BeginPlay()
{
    Super::BeginPlay();

    RegisterWithEcosystem();

    if (EchoDefinition)
    {
        InitializeFromDefinition(EchoDefinition, InstanceId);
    }
    else
    {
        UE_LOG(LogAstrawildAI, Warning, TEXT("Echo %s has no EchoDefinition assigned."), *GetName());
    }
}

void AAstrawildEchoCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UnregisterFromEcosystem();
    Super::EndPlay(EndPlayReason);
}

void AAstrawildEchoCharacter::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Needs + bond simulate server-side only (directive §28), throttled by LOD tier.
    if (GetLocalRole() == ROLE_Authority && !IsDefeated())
    {
        HandleNeedsDecay(DeltaTime);
    }
}

UAstrawildEcosystemSubsystem* AAstrawildEchoCharacter::GetEcosystem() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UAstrawildEcosystemSubsystem>() : nullptr;
}

void AAstrawildEchoCharacter::RegisterWithEcosystem()
{
    if (GetLocalRole() == ROLE_Authority)
    {
        if (UAstrawildEcosystemSubsystem* Ecosystem = GetEcosystem())
        {
            Ecosystem->RegisterEcho(this);
        }
    }
}

void AAstrawildEchoCharacter::UnregisterFromEcosystem()
{
    if (GetLocalRole() == ROLE_Authority)
    {
        if (UAstrawildEcosystemSubsystem* Ecosystem = GetEcosystem())
        {
            Ecosystem->UnregisterEcho(this);
        }
    }
}

bool AAstrawildEchoCharacter::InitializeFromDefinition(UAstrawildEchoDefinition* InDefinition, const FGuid& OptionalInstanceId)
{
    if (!IsValid(InDefinition) || InDefinition->DefinitionId.IsNone())
    {
        UE_LOG(LogAstrawildAI, Warning, TEXT("Echo initialization rejected: invalid definition."));
        return false;
    }

    EchoDefinition = InDefinition;
    CachedStats = InDefinition->BaseStats;
    CurrentHealth = FMath::Max(1.0f, CachedStats.MaxHealth);
    Trust = FMath::Max(0.0f, Trust);
    InstanceId = OptionalInstanceId.IsValid() ? OptionalInstanceId : FGuid::NewGuid();

    if (Personality == EAstrawildPersonality::Curious && !bCaptured)
    {
        RollPersonalityFromDefinition();
    }

    GetCharacterMovement()->MaxWalkSpeed = FMath::Max(0.0f, CachedStats.MoveSpeed);
    return true;
}

bool AAstrawildEchoCharacter::InitializeFromDefinitionWithPersonality(UAstrawildEchoDefinition* InDefinition, const EAstrawildPersonality InPersonality, const FGuid& OptionalInstanceId)
{
    Personality = InPersonality;
    return InitializeFromDefinition(InDefinition, OptionalInstanceId);
}

void AAstrawildEchoCharacter::RollPersonalityFromDefinition()
{
    if (!IsValid(EchoDefinition))
    {
        return;
    }

    // 70% species-dominant personality, 30% random archetype — creatures feel varied (directive §5).
    if (FMath::FRand() < 0.7f)
    {
        Personality = EchoDefinition->DominantPersonality;
    }
    else
    {
        const int32 Roll = FMath::RandRange(0, 9);
        Personality = static_cast<EAstrawildPersonality>(Roll);
    }
}

void AAstrawildEchoCharacter::SetAIState(const EAstrawildEchoAIState NewState)
{
    // Public entry (audit H-8): broadcast on every transition so UI/audio observers
    // can react to AI state changes.
    if (CurrentAIState != NewState)
    {
        CurrentAIState = NewState;
        OnAIStateChanged.Broadcast(this, NewState);
    }
}

bool AAstrawildEchoCharacter::ApplyDamage(const float DamageAmount)
{
    return ApplyElementalDamage(DamageAmount, EAstrawildElementType::None) > 0.0f;
}

float AAstrawildEchoCharacter::ApplyElementalDamage(const float DamageAmount, const EAstrawildElementType InElement)
{
    if (GetLocalRole() != ROLE_Authority || DamageAmount <= 0.0f || IsDefeated())
    {
        return 0.0f;
    }

    float Damage = DamageAmount;

    // Elemental interactions (directive §9): weakness x1.5, matching element resisted.
    if (IsValid(EchoDefinition))
    {
        if (InElement != EAstrawildElementType::None && InElement == EchoDefinition->WeaknessElement)
        {
            Damage *= 1.5f;
        }
        else if (InElement != EAstrawildElementType::None && InElement == EchoDefinition->Element)
        {
            Damage *= (1.0f - EchoDefinition->ElementalResistance);
        }
    }

    const float MitigatedDamage = FMath::Max(0.0f, Damage - CachedStats.Defense);
    if (MitigatedDamage <= 0.0f)
    {
        return 0.0f;
    }

    CurrentHealth = FMath::Max(0.0f, CurrentHealth - MitigatedDamage);
    OnDamaged.Broadcast(this, CurrentHealth);

    // Aggressive/Brave personalities fight back harder; the AI controller listens to OnDamaged.
    if (IsDefeated())
    {
        OnDefeated.Broadcast(this);

        // Loot + events (server-side).
        if (IsValid(EchoDefinition))
        {
            if (UAstrawildEcosystemSubsystem* Ecosystem = GetEcosystem())
            {
                Ecosystem->NotifyDefeated(EchoDefinition->DefinitionId);
            }

            if (UWorld* World = GetWorld())
            {
                if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
                {
                    const bool bWasHostile = IsValid(EchoDefinition) && EchoDefinition->bHostileToPlayers;
                    EventBus->PublishEvent(
                        bWasHostile ? TAG_Astrawild_Event_HostileDefeated : TAG_Astrawild_Event_EchoDefeated,
                        GetInstigator() ? GetInstigator() : nullptr,
                        EchoDefinition->DefinitionId,
                        1,
                        GetActorLocation());
                }
            }
        }
    }

    return MitigatedDamage;
}

float AAstrawildEchoCharacter::GetHealthFraction() const
{
    const float MaxHealth = FMath::Max(1.0f, GetMaxHealth());
    return FMath::Clamp(CurrentHealth / MaxHealth, 0.0f, 1.0f);
}

float AAstrawildEchoCharacter::GetMaxHealth() const
{
    // +10% per level above 1 (growth, directive §4).
    return FMath::Max(1.0f, CachedStats.MaxHealth * (1.0f + 0.1f * (Level - 1)));
}

float AAstrawildEchoCharacter::GetAttackPower() const
{
    return CachedStats.AttackPower * (1.0f + 0.08f * (Level - 1));
}

float AAstrawildEchoCharacter::ComputeCaptureChance() const
{
    if (IsDefeated() || !IsValid(EchoDefinition))
    {
        return 0.0f;
    }

    const float HealthFraction = GetHealthFraction();
    const float Resilience = FMath::Clamp(CachedStats.CaptureResilience, 0.0f, 1.0f);
    const float Difficulty = FMath::Clamp(EchoDefinition->CaptureDifficulty, 0.0f, 1.0f);

    // Base: species difficulty reduces the floor; weaken bonus scales with missing health.
    const float Base = 0.05f * (1.0f - 0.5f * Difficulty);
    const float WeakenBonus = (1.0f - HealthFraction) * (1.0f - Resilience) * (1.0f - 0.5f * Difficulty);
    const float TrustBonus = FMath::Clamp(Trust / 100.0f, 0.0f, 1.0f) * 0.5f;

    // Situational bonuses (directive §8): preferred weather + activity window.
    float SituationalBonus = 0.0f;
    if (UWorld* World = GetWorld())
    {
        if (const AAstrawildGameState* GameState = World->GetGameState<AAstrawildGameState>())
        {
            if (EchoDefinition->PreferredWeather.Contains(GameState->WeatherState))
            {
                SituationalBonus += 0.10f;
            }
        }
    }
    if (IsCurrentlyActiveTime())
    {
        SituationalBonus += 0.05f;
    }

    return FMath::Clamp(Base + WeakenBonus + TrustBonus + SituationalBonus, 0.02f, 0.95f);
}

bool AAstrawildEchoCharacter::IsCurrentlyActiveTime() const
{
    if (!IsValid(EchoDefinition))
    {
        return true;
    }

    const UWorld* World = GetWorld();
    const AAstrawildGameState* GameState = World ? World->GetGameState<AAstrawildGameState>() : nullptr;
    if (!GameState)
    {
        return true;
    }

    const float Hour = GameState->GetTimeOfDayHours();
    switch (EchoDefinition->ActivityPattern)
    {
    case EAstrawildActivityPattern::Diurnal:
        return Hour >= 5.5f && Hour < 19.5f;
    case EAstrawildActivityPattern::Nocturnal:
        return Hour < 5.5f || Hour >= 19.5f;
    case EAstrawildActivityPattern::Crepuscular:
        return (Hour >= 5.0f && Hour < 8.0f) || (Hour >= 17.0f && Hour < 20.5f);
    default:
        return true;
    }
}

float AAstrawildEchoCharacter::GetFleeHealthThresholdMultiplier() const
{
    switch (Personality)
    {
    case EAstrawildPersonality::Timid:    return 1.8f;
    case EAstrawildPersonality::Brave:    return 0.4f;
    case EAstrawildPersonality::Aggressive: return 0.5f;
    case EAstrawildPersonality::Protective: return 0.6f;
    default: return 1.0f;
    }
}

float AAstrawildEchoCharacter::GetAggroRadiusMultiplier() const
{
    switch (Personality)
    {
    case EAstrawildPersonality::Aggressive: return 1.5f;
    case EAstrawildPersonality::Brave:      return 1.2f;
    case EAstrawildPersonality::Timid:      return 0.5f;
    default: return 1.0f;
    }
}

float AAstrawildEchoCharacter::GetWorkSpeedMultiplier() const
{
    switch (Personality)
    {
    case EAstrawildPersonality::Lazy:      return 0.6f;
    case EAstrawildPersonality::Energetic: return 1.4f;
    case EAstrawildPersonality::Loyal:     return 1.15f;
    default: return 1.0f;
    }
}

float AAstrawildEchoCharacter::GetCommandObedience() const
{
    float Obedience = 0.8f;
    switch (Personality)
    {
    case EAstrawildPersonality::Loyal:        Obedience = 1.0f; break;
    case EAstrawildPersonality::Independent:  Obedience = 0.5f; break;
    case EAstrawildPersonality::Lazy:         Obedience = 0.6f; break;
    case EAstrawildPersonality::Protective:   Obedience = 0.95f; break;
    default: break;
    }
    // Trust and bond raise obedience (directive §5/§10).
    Obedience += FMath::Clamp(Trust / 200.0f, 0.0f, 0.25f) + FMath::Clamp(Bond / 400.0f, 0.0f, 0.25f);
    return FMath::Clamp(Obedience, 0.1f, 1.0f);
}

bool AAstrawildEchoCharacter::Capture(const float InitialTrust)
{
    if (GetLocalRole() != ROLE_Authority || bCaptured || IsDefeated() || !IsValid(EchoDefinition))
    {
        return false;
    }

    bCaptured = true;
    Trust = FMath::Max(0.0f, InitialTrust) + EchoDefinition->TrustGainOnCapture;
    ActiveCommand = EAstrawildEchoCommand::Follow;
    SetAIState(EAstrawildEchoAIState::Follow);

    if (UAstrawildEcosystemSubsystem* Ecosystem = GetEcosystem())
    {
        Ecosystem->NotifyCaptured(EchoDefinition->DefinitionId);
    }

    if (UWorld* World = GetWorld())
    {
        if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
        {
            EventBus->PublishEvent(TAG_Astrawild_Event_EchoCaptured, GetInstigator(), EchoDefinition->DefinitionId, 1, GetActorLocation());
        }
    }

    OnCaptured.Broadcast(this);
    return true;
}

void AAstrawildEchoCharacter::AddTrust(const float Amount)
{
    if (GetLocalRole() == ROLE_Authority)
    {
        Trust = FMath::Max(0.0f, Trust + Amount);
    }
}

float AAstrawildEchoCharacter::Feed(const FName FoodItemId, const float FeedValue)
{
    if (GetLocalRole() != ROLE_Authority || !IsValid(EchoDefinition) || IsDefeated())
    {
        return 0.0f;
    }

    const bool bPreferred = EchoDefinition->PreferredFoodIds.Contains(FoodItemId);
    const float Multiplier = bPreferred ? 2.0f : 1.0f;
    const float TrustGain = FMath::Max(0.0f, FeedValue) * Multiplier;

    Trust += TrustGain;
    Bond = FMath::Clamp(Bond + TrustGain * 0.25f, 0.0f, 100.0f);
    Needs.Hunger = FMath::Clamp(Needs.Hunger + 30.0f * Multiplier, 0.0f, 100.0f);
    Needs.Mood = FMath::Clamp(Needs.Mood + 10.0f * Multiplier, 0.0f, 100.0f);

    if (UWorld* World = GetWorld())
    {
        if (UAstrawildEventBusSubsystem* EventBus = World->GetSubsystem<UAstrawildEventBusSubsystem>())
        {
            EventBus->PublishEvent(TAG_Astrawild_Event_EchoFed, GetInstigator(), FoodItemId, 1, GetActorLocation());
        }
    }

    return TrustGain;
}

void AAstrawildEchoCharacter::AddExperience(const float Amount)
{
    if (GetLocalRole() != ROLE_Authority || Amount <= 0.0f || !IsValid(EchoDefinition))
    {
        return;
    }

    Experience += FMath::Max(0.0f, Amount);

    // Level curve: BaseExperienceToLevel * level (directive §4 Growth).
    const float Required = EchoDefinition->BaseExperienceToLevel * Level;
    if (Experience >= Required)
    {
        Experience -= Required;
        ++Level;
        CurrentHealth = GetMaxHealth(); // Full heal on level up.
        OnLevelUp.Broadcast(this, Level);
        UE_LOG(LogAstrawildAI, Log, TEXT("Echo %s reached level %d."), *GetName(), Level);
    }
}

bool AAstrawildEchoCharacter::IssueCommand(const EAstrawildEchoCommand Command)
{
    if (GetLocalRole() != ROLE_Authority || !bCaptured || IsDefeated())
    {
        return false;
    }

    // Obedience roll — disloyal Echoes sometimes ignore commands (directive §5/§10).
    if (FMath::FRand() > GetCommandObedience())
    {
        UE_LOG(LogAstrawildAI, Verbose, TEXT("Echo %s ignored command %d (obedience %.2f)."), *GetName(), static_cast<int32>(Command), GetCommandObedience());
        return false;
    }

    ActiveCommand = Command;
    OnCommandReceived.Broadcast(this, Command);
    return true;
}

void AAstrawildEchoCharacter::HandleNeedsDecay(const float DeltaSeconds)
{
    if (!IsValid(EchoDefinition))
    {
        return;
    }

    // Throttle by simulation tier (directive §34): far creatures update less often.
    float UpdateInterval = 0.0f;
    if (const UAstrawildEcosystemSubsystem* Ecosystem = GetEcosystem())
    {
        UpdateInterval = UAstrawildEcosystemSubsystem::GetRecommendedUpdateInterval(Ecosystem->GetTierForEcho(this));
    }

    if (UpdateInterval > 0.0f)
    {
        NeedsDecayAccumulator += DeltaSeconds;
        if (NeedsDecayAccumulator < UpdateInterval)
        {
            return;
        }
        DeltaSeconds = NeedsDecayAccumulator;
        NeedsDecayAccumulator = 0.0f;
    }

    // Convert decay-per-in-world-hour into per-second using the time subsystem rate.
    const UWorld* World = GetWorld();
    const UAstrawildTimeSubsystem* TimeSubsystem = World ? World->GetSubsystem<UAstrawildTimeSubsystem>() : nullptr;
    const float WorldMinutesPerSecond = TimeSubsystem ? FMath::Max(0.001f, TimeSubsystem->MinutesPerRealSecond) : 1.0f;
    const float InWorldHoursThisTick = (DeltaSeconds * WorldMinutesPerSecond) / 60.0f;

    const float HungerDecay = EchoDefinition->HungerDecayPerHour * InWorldHoursThisTick;
    const float EnergyDecay = EchoDefinition->EnergyDecayPerHour * InWorldHoursThisTick;

    // Captured Echoes in the player party burn needs slower (they are cared for).
    const float CareMultiplier = bCaptured ? 0.6f : 1.0f;

    Needs.Hunger = FMath::Clamp(Needs.Hunger - HungerDecay * CareMultiplier, 0.0f, 100.0f);
    Needs.Energy = FMath::Clamp(Needs.Energy - EnergyDecay * CareMultiplier, 0.0f, 100.0f);
    Needs.Mood = FMath::Clamp(Needs.Mood - (Needs.Hunger < 30.0f ? 2.0f : -0.5f) * InWorldHoursThisTick, 0.0f, 100.0f);

    // Bond grows slowly while traveling with the player (directive §4 Relationship).
    if (bCaptured)
    {
        Bond = FMath::Clamp(Bond + 0.2f * InWorldHoursThisTick, 0.0f, 100.0f);
    }

    // Critical needs injure the creature (soft pressure, directive §11 philosophy).
    if (Needs.IsCritical())
    {
        CurrentHealth = FMath::Max(1.0f, CurrentHealth - 1.0f * DeltaSeconds);
    }
}

FAstrawildEchoInstanceSaveData AAstrawildEchoCharacter::ToSaveData() const
{
    FAstrawildEchoInstanceSaveData Data;
    Data.InstanceId = InstanceId;
    Data.DefinitionId = EchoDefinition ? EchoDefinition->DefinitionId : NAME_None;
    Data.Level = FMath::Max(1, Level);
    Data.Trust = Trust;
    Data.Experience = 0;
    Data.bInRoster = bCaptured;
    Data.LastKnownTransform = GetActorTransform();
    return Data;
}

FAstrawildEchoInstanceV2 AAstrawildEchoCharacter::ToSaveDataV2() const
{
    FAstrawildEchoInstanceV2 Data;
    Data.InstanceId = InstanceId;
    Data.DefinitionId = EchoDefinition ? EchoDefinition->DefinitionId : NAME_None;
    Data.Personality = Personality;
    Data.Level = FMath::Max(1, Level);
    Data.Experience = Experience;
    Data.Trust = Trust;
    Data.Bond = Bond;
    Data.Needs = Needs;
    Data.LastKnownTransform = GetActorTransform();
    Data.bInParty = bCaptured; // Roster membership == captured in v2 schema.
    return Data;
}

bool AAstrawildEchoCharacter::FromSaveDataV2(const FAstrawildEchoInstanceV2& Data)
{
    if (!Data.InstanceId.IsValid() || Data.DefinitionId.IsNone())
    {
        return false;
    }

    InstanceId = Data.InstanceId;
    Personality = Data.Personality;
    Level = FMath::Max(1, Data.Level);
    Experience = FMath::Max(0.0f, Data.Experience);
    Trust = FMath::Max(0.0f, Data.Trust);
    Bond = FMath::Clamp(Data.Bond, 0.0f, 100.0f);
    Needs = Data.Needs;
    bCaptured = Data.bInParty;
    SetActorTransform(Data.LastKnownTransform);

    if (IsValid(EchoDefinition))
    {
        CachedStats = EchoDefinition->BaseStats;
        CurrentHealth = FMath::Min(FMath::Max(1.0f, CurrentHealth > 0.0f ? CurrentHealth : GetMaxHealth()), GetMaxHealth());
        GetCharacterMovement()->MaxWalkSpeed = CachedStats.MoveSpeed;
    }
    return true;
}
