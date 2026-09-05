#include "AstrawildGameState.h"

#include "AstrawildResearchSubsystem.h" // LCP-5: research mirror import

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
    DOREPLIFETIME(AAstrawildGameState, bWorldSeedSynced);
    DOREPLIFETIME(AAstrawildGameState, ResearchMirror); // LCP-5
    DOREPLIFETIME(AAstrawildGameState, EndingState);
    DOREPLIFETIME(AAstrawildGameState, bPostGameActive);
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
    bWorldSeedSynced = true; // LCP-2: clients may now build the deterministic world copy
}

void AAstrawildGameState::SetEndingState(const EAstrawildEndingState InState)
{
    if (!HasAuthority())
    {
        return;
    }

    // One-way verdict: None can become an ending, but a chosen ending never
    // changes back or switches sides (FR-6 story integrity + save v5 truth).
    if (EndingState != EAstrawildEndingState::None || InState == EAstrawildEndingState::None ||
        InState >= EAstrawildEndingState::Count || InState == EndingState)
    {
        return;
    }

    const EAstrawildEndingState OldEnding = EndingState;
    EndingState = InState;
    bPostGameActive = true; // Both endings unlock post-game free roam.

    // Ending A — "The Dawn That Stays": the storm crown is broken; the sky is
    // pinned to Clear forever (the weather subsystem also stops rolling).
    if (InState == EAstrawildEndingState::TheDawnThatStays)
    {
        SetWeatherState(EAstrawildWeatherState::Clear);
    }
    // Ending B — "The Storm That Sleeps": the crown sleeps beneath the waves;
    // the living sky keeps rolling, only tamed.

    UE_LOG(LogAstrawildWorld, Log, TEXT("Ending triggered: %d (day %d) — post-game free roam unlocked."),
        static_cast<int32>(InState), DayNumber);
    OnEndingTriggered.Broadcast(InState, OldEnding);
}

FText AAstrawildGameState::GetEndingBannerText() const
{
    switch (EndingState)
    {
    case EAstrawildEndingState::TheDawnThatStays:
        return FText::FromString(TEXT("THE DAWN THAT STAYS — the storm crown is broken. The Vale is yours. [Post-game: free roam]"));
    case EAstrawildEndingState::TheStormThatSleeps:
        return FText::FromString(TEXT("THE STORM THAT SLEEPS — the crown sleeps beneath the waves. The Vale endures. [Post-game: free roam]"));
    default:
        return FText::GetEmpty();
    }
}

void AAstrawildGameState::OnRep_EndingState()
{
    // Client hook for the ending banner presentation (HUD polls the replicated
    // state directly; this stays available for sequencer/audio polish).
}

void AAstrawildGameState::OnRep_TimeOfDayMinutes()
{
    // Hook for client-side presentation (skybox, audio). No-op by default.
}

void AAstrawildGameState::OnRep_WeatherState()
{
    // Hook for client-side presentation (VFX, audio). No-op by default.
}

void AAstrawildGameState::OnRep_ResearchMirror()
{
    // LCP-5: clients mirror the host's authoritative research pool into their
    // LOCAL GameInstance subsystem — every read path (HUD, research screen)
    // keeps working unchanged. Clients never mutate the authoritative pool.
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() != NM_Client)
    {
        return;
    }
    if (UGameInstance* GameInstance = World->GetGameInstance())
    {
        if (UAstrawildResearchSubsystem* Research = GameInstance->GetSubsystem<UAstrawildResearchSubsystem>())
        {
            Research->ImportFromSave(ResearchMirror);
        }
    }
}
