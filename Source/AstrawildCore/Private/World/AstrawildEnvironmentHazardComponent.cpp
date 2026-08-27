#include "World/AstrawildEnvironmentHazardComponent.h"

#include "Components/AstrawildAttributeComponent.h"
#include "Components/AstrawildSurvivalComponent.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "World/AstrawildWeatherSubsystem.h"

UAstrawildEnvironmentHazardComponent::UAstrawildEnvironmentHazardComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UAstrawildEnvironmentHazardComponent::BeginPlay()
{
    Super::BeginPlay();
    RecalculateStress();
    SyncSurvivalTemperature();
}

void UAstrawildEnvironmentHazardComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    RecalculateStress();
    SyncSurvivalTemperature();
    if (!IsDangerous() || !GetOwner() || !GetOwner()->HasAuthority())
    {
        TimeUntilNextDamage = FMath::Max(0.0f, TimeUntilNextDamage - DeltaTime);
        return;
    }

    TimeUntilNextDamage -= DeltaTime;
    if (TimeUntilNextDamage > 0.0f)
    {
        return;
    }

    TimeUntilNextDamage = DamageTickInterval;
    if (UAstrawildAttributeComponent* Attributes = GetAttributes())
    {
        const float Damage = DamagePerStressPerSecond * CurrentStress * DamageTickInterval;
        if (Damage > 0.0f)
        {
            Attributes->ModifyHealth(-Damage, GetOwner());
            OnHazardDamage.Broadcast(GetEffectiveTemperature() < 0.0f ? FName(TEXT("Hazard.Hypothermia")) : FName(TEXT("Hazard.Heatstroke")), Damage);
        }
    }
}

void UAstrawildEnvironmentHazardComponent::SetAmbientTemperature(const int32 NewTemperatureLevel)
{
    AmbientTemperatureLevel = FMath::Clamp(NewTemperatureLevel, -5, 5);
    RecalculateStress();
}

void UAstrawildEnvironmentHazardComponent::SetInsulationLevel(const int32 NewInsulationLevel)
{
    InsulationLevel = FMath::Clamp(NewInsulationLevel, -5, 5);
    RecalculateStress();
}

void UAstrawildEnvironmentHazardComponent::SetCampProtectionLevel(const int32 NewProtectionLevel)
{
    CampProtectionLevel = FMath::Clamp(NewProtectionLevel, -5, 5);
    RecalculateStress();
}

float UAstrawildEnvironmentHazardComponent::GetEffectiveTemperature() const
{
    int32 WeatherModifier = 0;
    if (const UWorld* World = GetWorld())
    {
        if (const UAstrawildWeatherSubsystem* Weather = World->GetSubsystem<UAstrawildWeatherSubsystem>())
        {
            WeatherModifier = Weather->GetTemperatureModifier();
        }
    }
    return static_cast<float>(AmbientTemperatureLevel + WeatherModifier - InsulationLevel - CampProtectionLevel);
}

bool UAstrawildEnvironmentHazardComponent::IsDangerous() const
{
    return CurrentStress > 0.25f;
}

UAstrawildAttributeComponent* UAstrawildEnvironmentHazardComponent::GetAttributes()
{
    if (!CachedAttributes.IsValid())
    {
        CachedAttributes = GetOwner() ? GetOwner()->FindComponentByClass<UAstrawildAttributeComponent>() : nullptr;
    }
    return CachedAttributes.Get();
}

UAstrawildSurvivalComponent* UAstrawildEnvironmentHazardComponent::GetSurvival()
{
    if (!CachedSurvival.IsValid())
    {
        CachedSurvival = GetOwner() ? GetOwner()->FindComponentByClass<UAstrawildSurvivalComponent>() : nullptr;
    }
    return CachedSurvival.Get();
}

void UAstrawildEnvironmentHazardComponent::SyncSurvivalTemperature()
{
    if (UAstrawildSurvivalComponent* Survival = GetSurvival())
    {
        const float TargetTemperature = Survival->ComfortableTemperature + GetEffectiveTemperature();
        if (!FMath::IsNearlyEqual(Survival->CurrentTemperature, TargetTemperature))
        {
            Survival->SetTemperature(TargetTemperature);
        }
    }
}

void UAstrawildEnvironmentHazardComponent::RecalculateStress()
{
    const float OldStress = CurrentStress;
    CurrentStress = FMath::Clamp(FMath::Abs(GetEffectiveTemperature()) / 5.0f, 0.0f, 1.0f);
    if (!FMath::IsNearlyEqual(OldStress, CurrentStress))
    {
        OnHazardStateChanged.Broadcast(FMath::Clamp(FMath::RoundToInt(GetEffectiveTemperature()), -5, 5), CurrentStress);
    }
}
