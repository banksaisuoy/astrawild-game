#include "AstrawildSurvivalComponent.h"

#include "AstrawildAttributeComponent.h"
#include "AstrawildCore.h"
#include "AstrawildInventoryComponent.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "AstrawildWeatherSubsystem.h"
#include "GameFramework/Controller.h"
#include "Net/UnrealNetwork.h"

UAstrawildSurvivalComponent::UAstrawildSurvivalComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

void UAstrawildSurvivalComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UAstrawildSurvivalComponent, Stats);
    DOREPLIFETIME(UAstrawildSurvivalComponent, StatusEffects);
}

void UAstrawildSurvivalComponent::BeginPlay()
{
    Super::BeginPlay();

    // GDP-3: Vigor scales max health — subscribe to level-ups so the bar grows
    // live, and take the current level into account right away.
    if (AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(GetOwner()))
    {
        if (UAstrawildAttributeComponent* Attributes = Player->AttributeComponent)
        {
            Attributes->OnAttributeLevelUp.AddDynamic(this, &UAstrawildSurvivalComponent::HandleAttributeLevelUp);
        }
    }
    RefreshVigorMaxHealth();
}

void UAstrawildSurvivalComponent::HandleAttributeLevelUp(const EAstrawildAttributeType Attribute, const int32 NewLevel)
{
    if (Attribute == EAstrawildAttributeType::Vigor)
    {
        RefreshVigorMaxHealth();
    }
}

void UAstrawildSurvivalComponent::RefreshVigorMaxHealth()
{
    if (GetOwnerRole() != ROLE_Authority)
    {
        return;
    }

    float Multiplier = 1.0f;
    if (const AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(GetOwner()))
    {
        if (const UAstrawildAttributeComponent* Attributes = Player->AttributeComponent)
        {
            Multiplier = Attributes->GetMaxHealthMultiplier();
        }
    }

    const float NewMax = BaseMaxHealth * Multiplier;
    if (!FMath::IsNearlyEqual(Stats.MaxHealth, NewMax))
    {
        Stats.MaxHealth = NewMax;
        Stats.Health = FMath::Min(Stats.Health, Stats.MaxHealth);
        OnStatsChanged.Broadcast(Stats.Health, Stats.Stamina);
    }
}

void UAstrawildSurvivalComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Server-authoritative simulation (directive §28). Clients see replicated Stats.
    if (GetOwnerRole() != ROLE_Authority || Stats.bIsDead)
    {
        return;
    }

    // --- Needs decay ---
    Stats.Hunger = FMath::Max(0.0f, Stats.Hunger - HungerDecayPerSecond * DeltaTime);
    Stats.Thirst = FMath::Max(0.0f, Stats.Thirst - ThirstDecayPerSecond * DeltaTime);

    // --- Stamina: sprint drain (M-2a) or passive regen (drains are requested by actions) ---
    // Batch 4 — M-2a: sprinting previously never drained stamina (gate-only), making
    // the exhaustion rule in RefreshMovementSpeed unreachable in normal play. While
    // the sprint-drain request is active AND the owner actually moves, drain instead
    // of regenerating (regen 14/s would otherwise out-pace drain 7/s → free sprint).
    if (bSprintDrainActive && IsOwnerMoving())
    {
        Stats.Stamina = FMath::Max(0.0f, Stats.Stamina - SprintStaminaDrainPerSecond * DeltaTime);
        if (Stats.Stamina <= 0.0f)
        {
            // Exhausted — stop draining (regen resumes next tick) and tell the owner
            // so it can drop out of sprint speed immediately.
            bSprintDrainActive = false;
            OnSprintExhausted.Broadcast();
        }
    }
    else
    {
        // Final production run: exosuit stamina-regen bonus stacks on the base rate.
        // GDP-3: Agility speeds the regen itself (1 + 4% per level above 1).
        float Regen = StaminaRegenPerSecond + GetExosuitStaminaRegenBonus();
        if (const AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(GetOwner()))
        {
            if (const UAstrawildAttributeComponent* Attributes = Player->AttributeComponent)
            {
                Regen *= Attributes->GetStaminaRegenMultiplier();
            }
        }
        Stats.Stamina = FMath::Min(Stats.MaxStamina, Stats.Stamina + Regen * DeltaTime);
    }

    // --- Starvation / dehydration ---
    if (Stats.Hunger <= 0.0f || Stats.Thirst <= 0.0f)
    {
        const float Factor = (Stats.Hunger <= 0.0f ? 1.0f : 0.0f) + (Stats.Thirst <= 0.0f ? 1.0f : 0.0f);
        Stats.Health = FMath::Max(0.0f, Stats.Health - StarvationHealthDamagePerSecond * Factor * DeltaTime);
    }

    // --- Environment temperature ---
    UpdateTemperature();
    // Production V2 (Master Plan §9): split thermal bands — cold-side and
    // heat-side insulation now resolve independently so frost/heat armor sets
    // can specialize. Legacy InsulationRating still counts on both sides.
    const float ColdInsulation = GetEquippedColdInsulation();
    const float HeatInsulation = GetEquippedHeatInsulation();
    if (Stats.Temperature <= ColdThresholdCelsius - ColdInsulation || Stats.Temperature >= HeatThresholdCelsius + HeatInsulation)
    {
        Stats.Health = FMath::Max(0.0f, Stats.Health - ExposureHealthDamagePerSecond * DeltaTime);
    }

    ApplyStatusTicks(DeltaTime);

    if (Stats.Health <= 0.0f)
    {
        Die();
    }
}

void UAstrawildSurvivalComponent::SetSprintDrainActive(const bool bActive)
{
    // Server-side simulation state (the drain ticks in the authority-gated Tick).
    // Harmless no-op on clients; the sprint SPEED itself is handled locally by
    // RefreshMovementSpeed, this flag only feeds the server-side stamina economy.
    bSprintDrainActive = bActive;
}

bool UAstrawildSurvivalComponent::IsOwnerMoving() const
{
    const AActor* Owner = GetOwner();
    // Walk 450 / sprint 700 cm/s — anything above 25 cm/s counts as real movement,
    // so holding the sprint key while standing still drains nothing.
    return Owner && Owner->GetVelocity().SizeSquared() > (25.0f * 25.0f);
}

void UAstrawildSurvivalComponent::UpdateTemperature()
{
    UAstrawildWeatherSubsystem* Weather = GetWeatherSubsystem();
    const float WeatherOffset = Weather ? Weather->GetTemperatureOffsetCelsius() : 0.0f;
    // Base temperate climate 20C + weather offset (time-of-day modulation could be added).
    Stats.Temperature = 20.0f + WeatherOffset;
}

UAstrawildWeatherSubsystem* UAstrawildSurvivalComponent::GetWeatherSubsystem() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetSubsystem<UAstrawildWeatherSubsystem>() : nullptr;
}

float UAstrawildSurvivalComponent::GetEquippedInsulation() const
{
    // Final production run (PHASE 12): helmet + exosuit insulation (0 when unequipped).
    const AActor* Owner = GetOwner();
    const UAstrawildInventoryComponent* Inventory = Owner ? Owner->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr;
    return Inventory ? Inventory->GetEquippedInsulationRating() : 0.0f;
}

float UAstrawildSurvivalComponent::GetEquippedColdInsulation() const
{
    const AActor* Owner = GetOwner();
    const UAstrawildInventoryComponent* Inventory = Owner ? Owner->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr;
    return Inventory ? Inventory->GetEquippedColdInsulationRating() : 0.0f;
}

float UAstrawildSurvivalComponent::GetEquippedHeatInsulation() const
{
    const AActor* Owner = GetOwner();
    const UAstrawildInventoryComponent* Inventory = Owner ? Owner->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr;
    return Inventory ? Inventory->GetEquippedHeatInsulationRating() : 0.0f;
}

float UAstrawildSurvivalComponent::GetExosuitStaminaRegenBonus() const
{
    const AActor* Owner = GetOwner();
    const UAstrawildInventoryComponent* Inventory = Owner ? Owner->FindComponentByClass<UAstrawildInventoryComponent>() : nullptr;
    return Inventory ? Inventory->GetEquippedStaminaRegenBonus() : 0.0f;
}

void UAstrawildSurvivalComponent::ApplyStatusTicks(const float DeltaTime)
{
    if (StatusEffects.IsEmpty())
    {
        return;
    }

    for (int32 i = StatusEffects.Num() - 1; i >= 0; --i)
    {
        FAstrawildStatusEffect& Effect = StatusEffects[i];
        Effect.RemainingSeconds -= DeltaTime;
        if (Effect.DamagePerSecond > 0.0f)
        {
            Stats.Health = FMath::Max(0.0f, Stats.Health - Effect.DamagePerSecond * DeltaTime);
        }
        if (Effect.RemainingSeconds <= 0.0f)
        {
            // Batch 3 — Item A: broadcast expiry so listeners (movement speed) refresh.
            const FName ExpiredId = Effect.StatusId;
            StatusEffects.RemoveAt(i);
            OnStatusEffectRemoved.Broadcast(ExpiredId);
        }
    }
}

float UAstrawildSurvivalComponent::GetHealthFraction() const
{
    const float MaxHealth = FMath::Max(1.0f, Stats.MaxHealth);
    return FMath::Clamp(Stats.Health / MaxHealth, 0.0f, 1.0f);
}

float UAstrawildSurvivalComponent::GetStaminaFraction() const
{
    const float MaxStamina = FMath::Max(1.0f, Stats.MaxStamina);
    return FMath::Clamp(Stats.Stamina / MaxStamina, 0.0f, 1.0f);
}

float UAstrawildSurvivalComponent::ApplyDamage(const float DamageAmount)
{
    if (GetOwnerRole() != ROLE_Authority || Stats.bIsDead || DamageAmount <= 0.0f)
    {
        return 0.0f;
    }

    if (bGodMode)
    {
        return 0.0f;
    }

    const float Applied = FMath::Min(Stats.Health, DamageAmount);
    Stats.Health = FMath::Max(0.0f, Stats.Health - DamageAmount);
    OnStatsChanged.Broadcast(Stats.Health, Stats.Stamina);

    // GDP-3: Vigor grows by surviving real hits (anything that hurts at least 5).
    if (Applied >= 5.0f)
    {
        if (const AAstrawildPlayerCharacter* Player = Cast<AAstrawildPlayerCharacter>(GetOwner()))
        {
            if (Player->AttributeComponent)
            {
                Player->AttributeComponent->AddAttributeXP(EAstrawildAttributeType::Vigor, 1.0f);
            }
        }
    }

    UE_LOG(LogAstrawildCombat, Verbose, TEXT("Player took %.1f damage (%.1f remaining)."), DamageAmount, Stats.Health);

    if (Stats.Health <= 0.0f)
    {
        Die();
    }
    return Applied;
}

void UAstrawildSurvivalComponent::ApplyConsumption(const float FoodValue, const float WaterValue, const float HealValue)
{
    if (GetOwnerRole() != ROLE_Authority || Stats.bIsDead)
    {
        return;
    }

    Stats.Hunger = FMath::Clamp(Stats.Hunger + FoodValue, 0.0f, 100.0f);
    Stats.Thirst = FMath::Clamp(Stats.Thirst + WaterValue, 0.0f, 100.0f);
    Stats.Health = FMath::Clamp(Stats.Health + HealValue, 0.0f, Stats.MaxHealth);
    OnStatsChanged.Broadcast(Stats.Health, Stats.Stamina);
}

bool UAstrawildSurvivalComponent::TryConsumeStamina(const float Amount)
{
    if (GetOwnerRole() != ROLE_Authority)
    {
        return true; // Client prediction path — server validates on authoritative call.
    }

    if (Amount <= 0.0f)
    {
        return true;
    }

    if (Stats.Stamina < Amount)
    {
        return false;
    }

    Stats.Stamina = FMath::Max(0.0f, Stats.Stamina - Amount);
    return true;
}

void UAstrawildSurvivalComponent::FullRestore()
{
    if (GetOwnerRole() != ROLE_Authority)
    {
        return;
    }

    Stats.Health = Stats.MaxHealth;
    Stats.Stamina = Stats.MaxStamina;
    Stats.Hunger = 100.0f;
    Stats.Thirst = 100.0f;
    Stats.bIsDead = false;
    // REVIEW-3 (M-2): broadcast removal before clearing so listeners (movement speed)
    // refresh — a Chilled/Shocked player used to keep the stale slow after a full rest.
    for (const FAstrawildStatusEffect& Effect : StatusEffects)
    {
        OnStatusEffectRemoved.Broadcast(Effect.StatusId);
    }
    StatusEffects.Reset();
    OnStatsChanged.Broadcast(Stats.Health, Stats.Stamina);
}

void UAstrawildSurvivalComponent::SetStatsForRestore(const FAstrawildSurvivalStats& InStats)
{
    // Audit H-1: load-path vitals restore — clamp everything to safe ranges and keep
    // MaxHealth/MaxStamina from the component so tunable changes survive save/load.
    if (GetOwnerRole() != ROLE_Authority)
    {
        return;
    }

    Stats.Health = FMath::Clamp(InStats.Health, 0.0f, Stats.MaxHealth);
    Stats.Stamina = FMath::Clamp(InStats.Stamina, 0.0f, Stats.MaxStamina);
    Stats.Hunger = FMath::Clamp(InStats.Hunger, 0.0f, 100.0f);
    Stats.Thirst = FMath::Clamp(InStats.Thirst, 0.0f, 100.0f);
    Stats.Temperature = InStats.Temperature;
    // A snapshot with zero health was saved mid-death — treat it as alive-at-minimum so
    // loading never instant-kills the player.
    Stats.bIsDead = false;
    if (Stats.Health <= 0.0f)
    {
        Stats.Health = 1.0f;
    }
    // REVIEW-3 (M-2): same removal-broadcast-before-clear as FullRestore — loading a
    // save while Chilled/Shocked must not leave a permanent stale slow.
    for (const FAstrawildStatusEffect& Effect : StatusEffects)
    {
        OnStatusEffectRemoved.Broadcast(Effect.StatusId);
    }
    StatusEffects.Reset();
    OnStatsChanged.Broadcast(Stats.Health, Stats.Stamina);
}

void UAstrawildSurvivalComponent::AddStatusEffect(const FAstrawildStatusEffect& Effect)
{
    if (GetOwnerRole() != ROLE_Authority || Effect.StatusId.IsNone() || Effect.RemainingSeconds <= 0.0f)
    {
        return;
    }

    // Refresh if already applied, otherwise append.
    if (FAstrawildStatusEffect* Existing = StatusEffects.FindByPredicate(
        [&Effect](const FAstrawildStatusEffect& Item) { return Item.StatusId == Effect.StatusId; }))
    {
        *Existing = Effect;
    }
    else
    {
        StatusEffects.Add(Effect);
    }
    OnStatusEffectApplied.Broadcast(Effect.StatusId);
}

bool UAstrawildSurvivalComponent::HasStatusEffect(const FName StatusId) const
{
    return StatusEffects.ContainsByPredicate(
        [&StatusId](const FAstrawildStatusEffect& Item) { return Item.StatusId == StatusId; });
}

float UAstrawildSurvivalComponent::GetStatusSpeedMultiplier() const
{
    // Batch 3 — Item A: combine every active speed-affecting status multiplicatively
    // (e.g. Chill 0.5 alone → 0.5; Chill + Shock would be 0.15 — rare but consistent).
    float Multiplier = 1.0f;
    for (const FAstrawildStatusEffect& Effect : StatusEffects)
    {
        if (Effect.SpeedMultiplier > 0.0f && Effect.SpeedMultiplier < 1.0f)
        {
            Multiplier *= Effect.SpeedMultiplier;
        }
    }
    return Multiplier;
}

void UAstrawildSurvivalComponent::SetGodMode(const bool bEnabled)
{
    bGodMode = bEnabled;
    UE_LOG(LogAstrawildCombat, Log, TEXT("Player god mode %s."), bEnabled ? TEXT("ENABLED") : TEXT("disabled"));
}

void UAstrawildSurvivalComponent::Die()
{
    if (Stats.bIsDead)
    {
        return;
    }

    Stats.bIsDead = true;
    Stats.Health = 0.0f;
    OnStatsChanged.Broadcast(0.0f, Stats.Stamina);
    OnDied.Broadcast();
    UE_LOG(LogAstrawildCombat, Log, TEXT("Player died."));
}

void UAstrawildSurvivalComponent::OnRep_Stats()
{
    OnStatsChanged.Broadcast(Stats.Health, Stats.Stamina);
}

void UAstrawildSurvivalComponent::RestoreStamina(const float Amount)
{
    // Production V2: aura-driven regen (Rhythm Aura party passive). Server-only,
    // broadcast keeps HUD/clients in sync through the standard stats delegate.
    if (Amount <= 0.0f || GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    Stats.Stamina = FMath::Min(Stats.MaxStamina, Stats.Stamina + Amount);
    OnStatsChanged.Broadcast(Stats.Health, Stats.Stamina);
}

void UAstrawildSurvivalComponent::RestoreHealth(const float Amount)
{
    if (Amount <= 0.0f || GetOwnerRole() != ROLE_Authority)
    {
        return;
    }
    Stats.Health = FMath::Clamp(Stats.Health + Amount, 0.0f, Stats.MaxHealth);
    OnStatsChanged.Broadcast(Stats.Health, Stats.Stamina);
}
