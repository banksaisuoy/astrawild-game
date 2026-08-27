#include "Components/AstrawildSanComponent.h"

#include "Math/UnrealMathUtility.h"

UAstrawildSanComponent::UAstrawildSanComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UAstrawildSanComponent::BeginPlay()
{
    Super::BeginPlay();
    MaxSAN = FMath::Max(1.0f, MaxSAN);
    CurrentSAN = FMath::Clamp(CurrentSAN, 0.0f, MaxSAN);
    CurrentWorkStress = FMath::Clamp(CurrentWorkStress, 0.0f, 1.0f);
    RefreshCriticalState();
}

void UAstrawildSanComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (DeltaTime <= 0.0f)
    {
        return;
    }

    const float RecoveryDelta = RecoveryPerSecond * (1.0f - CurrentWorkStress) * DeltaTime;
    const float WorkDecayDelta = WorkDecayPerSecond * CurrentWorkStress * DeltaTime;
    ModifySAN(RecoveryDelta - WorkDecayDelta);
}

float UAstrawildSanComponent::ModifySAN(const float DeltaSAN)
{
    const float OldSAN = CurrentSAN;
    MaxSAN = FMath::Max(1.0f, MaxSAN);
    CurrentSAN = FMath::Clamp(CurrentSAN + DeltaSAN, 0.0f, MaxSAN);
    if (!FMath::IsNearlyEqual(OldSAN, CurrentSAN))
    {
        OnSANChanged.Broadcast(CurrentSAN, MaxSAN);
    }
    RefreshCriticalState();
    return CurrentSAN - OldSAN;
}

void UAstrawildSanComponent::SetWorkStress(const float Stress)
{
    CurrentWorkStress = FMath::Clamp(Stress, 0.0f, 1.0f);
}

float UAstrawildSanComponent::GetSANPercent() const
{
    return MaxSAN > 0.0f ? CurrentSAN / MaxSAN : 0.0f;
}

void UAstrawildSanComponent::RefreshCriticalState()
{
    const bool bWasCritical = bIsSANCritical;
    bIsSANCritical = GetSANPercent() <= FMath::Clamp(CriticalThreshold, 0.0f, 1.0f);
    if (bWasCritical != bIsSANCritical)
    {
        OnSANStateChanged.Broadcast(bIsSANCritical);
    }
}
