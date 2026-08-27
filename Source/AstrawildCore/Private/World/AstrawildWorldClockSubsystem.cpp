#include "World/AstrawildWorldClockSubsystem.h"

#include "Engine/World.h"
#include "TimerManager.h"

UAstrawildWorldClockSubsystem::UAstrawildWorldClockSubsystem()
{
    bLastNightState = false;
}

void UAstrawildWorldClockSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(TimeTickHandle, this, &UAstrawildWorldClockSubsystem::HandleTimeTick, 1.0f, true, 1.0f);
    }
    BroadcastTimeStateIfChanged(true);
}

void UAstrawildWorldClockSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TimeTickHandle);
    }
    Super::Deinitialize();
}

void UAstrawildWorldClockSubsystem::AdvanceTime(const float DeltaSeconds)
{
    if (DeltaSeconds <= 0.0f || (GetWorld() && GetWorld()->GetNetMode() == NM_Client))
    {
        return;
    }
    const float SafeDayLength = FMath::Max(60.0f, DayLengthSeconds);
    const float PreviousDay = FMath::FloorToFloat(WorldTimeSeconds / SafeDayLength);
    WorldTimeSeconds = FMath::Max(0.0f, WorldTimeSeconds + DeltaSeconds);
    DayIndex = FMath::Max(0, FMath::FloorToInt(WorldTimeSeconds / SafeDayLength));
    if (FMath::FloorToFloat(WorldTimeSeconds / SafeDayLength) != PreviousDay)
    {
        BroadcastTimeStateIfChanged(true);
    }
    else
    {
        BroadcastTimeStateIfChanged(false);
    }
}

void UAstrawildWorldClockSubsystem::SetWorldTime(const float NewWorldTimeSeconds)
{
    if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
    {
        return;
    }
    WorldTimeSeconds = FMath::Max(0.0f, NewWorldTimeSeconds);
    DayIndex = FMath::Max(0, FMath::FloorToInt(WorldTimeSeconds / FMath::Max(60.0f, DayLengthSeconds)));
    BroadcastTimeStateIfChanged(true);
}

float UAstrawildWorldClockSubsystem::GetNormalizedTime() const
{
    const float SafeDayLength = FMath::Max(60.0f, DayLengthSeconds);
    return FMath::Fmod(FMath::Max(0.0f, WorldTimeSeconds), SafeDayLength) / SafeDayLength;
}

bool UAstrawildWorldClockSubsystem::IsNight() const
{
    const float NormalizedTime = GetNormalizedTime();
    return NormalizedTime < 0.25f || NormalizedTime >= 0.75f;
}

void UAstrawildWorldClockSubsystem::HandleTimeTick()
{
    AdvanceTime(1.0f);
}

void UAstrawildWorldClockSubsystem::BroadcastTimeStateIfChanged(const bool bForce)
{
    const bool bNight = IsNight();
    if (bForce || bNight != bLastNightState)
    {
        bLastNightState = bNight;
        OnTimeOfDayChanged.Broadcast(GetNormalizedTime(), bNight);
    }
}
