#include "AstrawildGameState.h"

#include "Net/UnrealNetwork.h"
#include "AstrawildCore.h"

AAstrawildGameState::AAstrawildGameState()
{
    // GameState replication is already enabled by default.
}

void AAstrawildGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AAstrawildGameState, TimeOfDayMinutes);
    DOREPLIFETIME(AAstrawildGameState, DayNumber);
    DOREPLIFETIME(AAstrawildGameState, WeatherState);
    DOREPLIFETIME(AAstrawildGameState, WorldSeed);
}

float AAstrawildGameState::GetTimeOfDayNormalized() const
{
    return static_cast<float>(TimeOfDayMinutes) / (60.0f * 24.0f);
}

float AAstrawildGameState::GetTimeOfDayHours() const
{
    return static_cast<float>(TimeOfDayMinutes) / 60.0f;
}

bool AAstrawildGameState::IsNight() const
{
    const float Hour = GetTimeOfDayHours();
    return Hour < 5.5f || Hour >= 19.5f;
}

float AAstrawildGameState::GetSunCycleAlpha() const
{
    // 06:00 sunrise -> 19:00 sunset mapped to 0..1 daylight arc.
    const float Hour = GetTimeOfDayHours();
    const float Arc = FMath::GetRangePct(6.0f, 19.0f, Hour);
    return FMath::Clamp(Arc, 0.0f, 1.0f);
}

FText AAstrawildGameState::GetTimeOfDayText() const
{
    const int32 Hour24 = TimeOfDayMinutes / 60;
    const int32 Minute = TimeOfDayMinutes % 60;
    return FText::FromString(FString::Printf(TEXT("%02d:%02d"), Hour24, Minute));
}

void AAstrawildGameState::SetTimeOfDayMinutes(const int32 InMinutes)
{
    if (!HasAuthority())
    {
        UE_LOG(LogAstrawildNetwork, Warning, TEXT("SetTimeOfDayMinutes rejected on client."));
        return;
    }
    TimeOfDayMinutes = FMath::Clamp(InMinutes, 0, 24 * 60 - 1);
}

void AAstrawildGameState::AdvanceDay()
{
    if (!HasAuthority())
    {
        return;
    }
    ++DayNumber;
}

void AAstrawildGameState::SetWeatherState(const EAstrawildWeatherState InState)
{
    if (!HasAuthority())
    {
        return;
    }
    if (WeatherState != InState)
    {
        WeatherState = InState;
        UE_LOG(LogAstrawildWorld, Log, TEXT("Weather transitioned to %d on day %d."), static_cast<int32>(InState), DayNumber);
    }
}

void AAstrawildGameState::SetWorldSeed(const int32 InSeed)
{
    if (!HasAuthority())
    {
        return;
    }
    WorldSeed = InSeed;
}

void AAstrawildGameState::OnRep_TimeOfDayMinutes()
{
    // Hook for client-side presentation (skybox, audio). No-op by default.
}

void AAstrawildGameState::OnRep_WeatherState()
{
    // Hook for client-side presentation (VFX, audio). No-op by default.
}
