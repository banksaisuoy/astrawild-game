#include "AstrawildSurvivalComponent.h"

#include "AstrawildCore.h"
#include "AstrawildLog.h"
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

    // --- Passive stamina regen (drains are requested by actions) ---
    Stats.Stamina = FMath::Min(Stats.MaxStamina, Stats.Stamina + StaminaRegenPerSecond * DeltaTime);

    // --- Starvation / dehydration ---
    if (Stats.Hunger <= 0.0f || Stats.Thirst <= 0.0f)
    {
        const float Factor = (Stats.Hunger <= 0.0f ? 1.0f : 0.0f) + (Stats.Thirst <= 0.0f ? 1.0f : 0.0f);
        Stats.Health = FMath::Max(0.0f, Stats.Health - StarvationHealthDamagePerSecond * Factor * DeltaTime);
    }

    // --- Environment temperature ---
    UpdateTemperature();
    if (Stats.Temperature <= ColdThresholdCelsius || Stats.Temperature >= HeatThresholdCelsius)
    {
        Stats.Health = FMath::Max(0.0f, Stats.Health - ExposureHealthDamagePerSecond * DeltaTime);
    }

    ApplyStatusTicks(DeltaTime);

    if (Stats.Health <= 0.0f)
    {
        Die();
    }
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
            StatusEffects.RemoveAt(i);
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
