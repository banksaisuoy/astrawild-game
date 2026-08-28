#include "World/AstrawildDisasterSubsystem.h"

#include "Engine/World.h"
#include "TimerManager.h"

UAstrawildDisasterSubsystem::UAstrawildDisasterSubsystem()
{
    TickIntervalSeconds = 1.0f;
    MinimumRandomIntervalSeconds = 60.0f;
    MaximumRandomIntervalSeconds = 180.0f;
    RandomSeed = 19860417;
}

void UAstrawildDisasterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    RandomStream.Initialize(RandomSeed);
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(DisasterTimerHandle, this, &UAstrawildDisasterSubsystem::HandleDisasterTick, FMath::Max(0.1f, TickIntervalSeconds), true, FMath::Max(0.1f, TickIntervalSeconds));
    }
}

void UAstrawildDisasterSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(DisasterTimerHandle);
    }
    Definitions.Reset();
    ActiveDisasters.Reset();
    Cooldowns.Reset();
    Super::Deinitialize();
}

bool UAstrawildDisasterSubsystem::HasAuthorityForDisaster() const
{
    const UWorld* World = GetWorld();
    return World && World->GetNetMode() != NM_Client;
}

bool UAstrawildDisasterSubsystem::RegisterDisasterDefinition(const FAstrawildDisasterDefinition& Definition)
{
    if (!HasAuthorityForDisaster() || !Definition.DisasterTag.IsValid() || !Definition.BiomeTag.IsValid())
    {
        return false;
    }
    FAstrawildDisasterDefinition Sanitized = Definition;
    Sanitized.DurationSeconds = FMath::Max(0.1f, Sanitized.DurationSeconds);
    Sanitized.CooldownSeconds = FMath::Max(0.0f, Sanitized.CooldownSeconds);
    Sanitized.Intensity = FMath::Clamp(Sanitized.Intensity, 0.0f, 1.0f);
    Sanitized.EffectRadius = FMath::Max(1.0f, Sanitized.EffectRadius);
    Definitions.Add(Sanitized.DisasterTag, MoveTemp(Sanitized));
    return true;
}

bool UAstrawildDisasterSubsystem::StartDisaster(const FGameplayTag& DisasterTag)
{
    if (!HasAuthorityForDisaster())
    {
        return false;
    }
    const FAstrawildDisasterDefinition* Definition = Definitions.Find(DisasterTag);
    if (!Definition || ActiveDisasters.Contains(DisasterTag) || Cooldowns.FindRef(DisasterTag) > 0.0f)
    {
        return false;
    }
    FAstrawildDisasterState State;
    State.DisasterTag = Definition->DisasterTag;
    State.DisasterType = Definition->DisasterType;
    State.RemainingSeconds = Definition->DurationSeconds;
    State.Intensity = Definition->Intensity;
    State.bActive = true;
    ActiveDisasters.Add(DisasterTag, State);
    OnDisasterStarted.Broadcast(State);
    return true;
}

bool UAstrawildDisasterSubsystem::StopDisaster(const FGameplayTag& DisasterTag)
{
    if (!HasAuthorityForDisaster())
    {
        return false;
    }
    const FAstrawildDisasterDefinition* Definition = Definitions.Find(DisasterTag);
    if (!Definition || ActiveDisasters.Remove(DisasterTag) == 0)
    {
        return false;
    }
    Cooldowns.Add(DisasterTag, Definition->CooldownSeconds);
    OnDisasterEnded.Broadcast(DisasterTag);
    return true;
}

bool UAstrawildDisasterSubsystem::StartRandomDisaster(const FGameplayTag& BiomeTag)
{
    if (!HasAuthorityForDisaster() || !BiomeTag.IsValid())
    {
        return false;
    }
    TArray<FGameplayTag> Candidates;
    for (const TPair<FGameplayTag, FAstrawildDisasterDefinition>& Pair : Definitions)
    {
        if (Pair.Value.BiomeTag == BiomeTag && !ActiveDisasters.Contains(Pair.Key) && Cooldowns.FindRef(Pair.Key) <= 0.0f)
        {
            Candidates.Add(Pair.Key);
        }
    }
    if (Candidates.Num() == 0)
    {
        return false;
    }
    return StartDisaster(Candidates[RandomStream.RandRange(0, Candidates.Num() - 1)]);
}

void UAstrawildDisasterSubsystem::AdvanceDisasters(const float DeltaSeconds)
{
    if (!HasAuthorityForDisaster() || DeltaSeconds <= 0.0f)
    {
        return;
    }
    const float SafeDelta = FMath::Min(DeltaSeconds, 5.0f);
    TArray<FGameplayTag> Expired;
    for (TPair<FGameplayTag, FAstrawildDisasterState>& Pair : ActiveDisasters)
    {
        Pair.Value.RemainingSeconds = FMath::Max(0.0f, Pair.Value.RemainingSeconds - SafeDelta);
        if (Pair.Value.RemainingSeconds <= 0.0f)
        {
            Expired.Add(Pair.Key);
        }
    }
    for (const FGameplayTag& Tag : Expired)
    {
        StopDisaster(Tag);
    }
    for (TPair<FGameplayTag, float>& Pair : Cooldowns)
    {
        Pair.Value = FMath::Max(0.0f, Pair.Value - SafeDelta);
    }
}

bool UAstrawildDisasterSubsystem::GetActiveDisaster(FAstrawildDisasterState& OutState) const
{
    OutState = FAstrawildDisasterState();
    for (const TPair<FGameplayTag, FAstrawildDisasterState>& Pair : ActiveDisasters)
    {
        OutState = Pair.Value;
        return true;
    }
    return false;
}

bool UAstrawildDisasterSubsystem::IsDisasterActive(const FGameplayTag& DisasterTag) const
{
    return ActiveDisasters.Contains(DisasterTag);
}

void UAstrawildDisasterSubsystem::HandleDisasterTick()
{
    AdvanceDisasters(FMath::Max(0.1f, TickIntervalSeconds));
}

void UAstrawildDisasterSubsystem::ScheduleNextRandomEvent()
{
    const float Interval = RandomStream.FRandRange(FMath::Max(5.0f, MinimumRandomIntervalSeconds), FMath::Max(MinimumRandomIntervalSeconds, MaximumRandomIntervalSeconds));
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(DisasterTimerHandle, this, &UAstrawildDisasterSubsystem::HandleDisasterTick, FMath::Max(0.1f, TickIntervalSeconds), true, Interval);
    }
}
