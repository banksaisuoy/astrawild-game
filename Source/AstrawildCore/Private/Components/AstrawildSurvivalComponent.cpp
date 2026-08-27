#include "Components/AstrawildSurvivalComponent.h"

#include "Math/UnrealMathUtility.h"

UAstrawildSurvivalComponent::UAstrawildSurvivalComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UAstrawildSurvivalComponent::BeginPlay()
{
    Super::BeginPlay();
    CurrentHunger = FMath::Clamp(CurrentHunger, 0.0f, MaxHunger);
    CurrentThirst = FMath::Clamp(CurrentThirst, 0.0f, MaxThirst);
    CurrentTemperature = ComfortableTemperature;
}

void UAstrawildSurvivalComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    const float HungerDelta = -(HungerDrainPerMinute / 60.0f) * DeltaTime;
    const float ThirstDelta = -(ThirstDrainPerMinute / 60.0f) * DeltaTime;
    CurrentHunger = FMath::Clamp(CurrentHunger + HungerDelta, 0.0f, MaxHunger);
    CurrentThirst = FMath::Clamp(CurrentThirst + ThirstDelta, 0.0f, MaxThirst);
    OnHungerChanged.Broadcast(CurrentHunger, MaxHunger, HungerDelta);
    OnThirstChanged.Broadcast(CurrentThirst, MaxThirst, ThirstDelta);

    WarningCooldownRemaining = FMath::Max(0.0f, WarningCooldownRemaining - DeltaTime);
    BroadcastWarnings();
}

bool UAstrawildSurvivalComponent::ConsumeFood(const float HungerRestored)
{
    if (HungerRestored <= 0.0f)
    {
        return false;
    }

    const float OldValue = CurrentHunger;
    CurrentHunger = FMath::Clamp(CurrentHunger + HungerRestored, 0.0f, MaxHunger);
    OnHungerChanged.Broadcast(CurrentHunger, MaxHunger, CurrentHunger - OldValue);
    return !FMath::IsNearlyEqual(OldValue, CurrentHunger);
}

bool UAstrawildSurvivalComponent::DrinkWater(const float ThirstRestored)
{
    if (ThirstRestored <= 0.0f)
    {
        return false;
    }

    const float OldValue = CurrentThirst;
    CurrentThirst = FMath::Clamp(CurrentThirst + ThirstRestored, 0.0f, MaxThirst);
    OnThirstChanged.Broadcast(CurrentThirst, MaxThirst, CurrentThirst - OldValue);
    return !FMath::IsNearlyEqual(OldValue, CurrentThirst);
}

void UAstrawildSurvivalComponent::SetTemperature(const float NewTemperature)
{
    const float OldValue = CurrentTemperature;
    CurrentTemperature = NewTemperature;
    OnTemperatureChanged.Broadcast(CurrentTemperature, ComfortableTemperature, CurrentTemperature - OldValue);
    BroadcastWarnings();
}

void UAstrawildSurvivalComponent::SetCarryWeight(const float NewWeight)
{
    CarryWeight = FMath::Max(0.0f, NewWeight);
    if (IsOverburdened())
    {
        OnSurvivalWarning.Broadcast(TEXT("Overburdened"));
    }
}

float UAstrawildSurvivalComponent::GetHungerPercent() const
{
    return MaxHunger > 0.0f ? CurrentHunger / MaxHunger : 0.0f;
}

float UAstrawildSurvivalComponent::GetThirstPercent() const
{
    return MaxThirst > 0.0f ? CurrentThirst / MaxThirst : 0.0f;
}

float UAstrawildSurvivalComponent::GetTemperatureStress() const
{
    return TemperatureTolerance > 0.0f ? FMath::Clamp(FMath::Abs(CurrentTemperature - ComfortableTemperature) / TemperatureTolerance, 0.0f, 1.0f) : 1.0f;
}

bool UAstrawildSurvivalComponent::IsOverburdened() const
{
    return CarryWeight > CarryWeightCapacity;
}

void UAstrawildSurvivalComponent::ExportToProfile(FAstrawildPlayerProfile& OutProfile) const
{
    OutProfile.Hunger = CurrentHunger;
    OutProfile.Thirst = CurrentThirst;
    OutProfile.BodyTemperature = CurrentTemperature;
    OutProfile.CarryWeight = CarryWeight;
}

void UAstrawildSurvivalComponent::ImportFromProfile(const FAstrawildPlayerProfile& InProfile)
{
    CurrentHunger = FMath::Clamp(InProfile.Hunger, 0.0f, MaxHunger);
    CurrentThirst = FMath::Clamp(InProfile.Thirst, 0.0f, MaxThirst);
    CurrentTemperature = InProfile.BodyTemperature;
    CarryWeight = FMath::Max(0.0f, InProfile.CarryWeight);
}

void UAstrawildSurvivalComponent::BroadcastWarnings()
{
    if (WarningCooldownRemaining > 0.0f)
    {
        return;
    }

    if (GetHungerPercent() <= 0.15f)
    {
        OnSurvivalWarning.Broadcast(TEXT("LowHunger"));
        WarningCooldownRemaining = 5.0f;
    }
    else if (GetThirstPercent() <= 0.15f)
    {
        OnSurvivalWarning.Broadcast(TEXT("LowThirst"));
        WarningCooldownRemaining = 5.0f;
    }
    else if (GetTemperatureStress() >= 0.85f)
    {
        OnSurvivalWarning.Broadcast(TEXT("TemperatureStress"));
        WarningCooldownRemaining = 5.0f;
    }
}
