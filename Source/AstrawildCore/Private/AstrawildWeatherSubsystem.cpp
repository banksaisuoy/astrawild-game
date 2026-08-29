#include "AstrawildWeatherSubsystem.h"

#include "AstrawildCore.h"
#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameState.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildLog.h"
#include "AstrawildTimeSubsystem.h"
#include "Engine/World.h"

namespace
{
    FAstrawildWeatherProfile MakeProfile(const float TempOffset, const float Weight, const float Visibility)
    {
        FAstrawildWeatherProfile Profile;
        Profile.TemperatureOffset = TempOffset;
        Profile.SelectionWeight = Weight;
        Profile.VisibilityMultiplier = Visibility;
        return Profile;
    }
}

UAstrawildWeatherSubsystem::UAstrawildWeatherSubsystem()
{
    // Configuration defaults live in the header; migrate to data assets later (asset pipeline).
}

bool UAstrawildWeatherSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UAstrawildWeatherSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UAstrawildWeatherSubsystem, STATGROUP_Tickables);
}

void UAstrawildWeatherSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
}

void UAstrawildWeatherSubsystem::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    UAstrawildTimeSubsystem* TimeSubsystem = World->GetSubsystem<UAstrawildTimeSubsystem>();
    AAstrawildGameState* GameState = GetAstrawildGameState();
    if (!TimeSubsystem || !GameState)
    {
        return;
    }

    // Weather decisions run on an in-world cadence, not real time.
    const int64 AbsoluteMinutes = static_cast<int64>(TimeSubsystem->GetCurrentDay()) * 24 * 60 + TimeSubsystem->GetCurrentMinute();
    if (LastDecisionAbsoluteMinutes < 0)
    {
        LastDecisionAbsoluteMinutes = AbsoluteMinutes;
        return;
    }

    if (AbsoluteMinutes - LastDecisionAbsoluteMinutes >= static_cast<int64>(WeatherChangeIntervalMinutes))
    {
        LastDecisionAbsoluteMinutes = AbsoluteMinutes;
        RollNextWeather();
    }
}

const FAstrawildWeatherProfile& UAstrawildWeatherSubsystem::GetProfile(const EAstrawildWeatherState State)
{
    static const TMap<EAstrawildWeatherState, FAstrawildWeatherProfile> Profiles = {
        { EAstrawildWeatherState::Clear,     MakeProfile( 2.0f, 3.0f, 1.00f) },
        { EAstrawildWeatherState::Cloudy,    MakeProfile( 0.0f, 2.5f, 1.00f) },
        { EAstrawildWeatherState::Rain,      MakeProfile(-4.0f, 1.8f, 0.85f) },
        { EAstrawildWeatherState::HeavyRain, MakeProfile(-7.0f, 1.0f, 0.70f) },
        { EAstrawildWeatherState::Storm,     MakeProfile(-9.0f, 0.6f, 0.55f) },
        { EAstrawildWeatherState::Fog,       MakeProfile(-3.0f, 1.2f, 0.45f) },
        { EAstrawildWeatherState::Heat,      MakeProfile(10.0f, 1.0f, 1.10f) },
        { EAstrawildWeatherState::Cold,      MakeProfile(-12.0f, 0.8f, 1.00f) }
    };
    return Profiles.FindRef(State);
}

EAstrawildWeatherState UAstrawildWeatherSubsystem::GetCurrentWeather() const
{
    const AAstrawildGameState* GameState = GetAstrawildGameState();
    return GameState ? GameState->WeatherState : EAstrawildWeatherState::Clear;
}

float UAstrawildWeatherSubsystem::GetTemperatureOffsetCelsius() const
{
    return GetProfile(GetCurrentWeather()).TemperatureOffset;
}

float UAstrawildWeatherSubsystem::GetVisibilityMultiplier() const
{
    return GetProfile(GetCurrentWeather()).VisibilityMultiplier;
}

void UAstrawildWeatherSubsystem::ForceWeather(const EAstrawildWeatherState NewState)
{
    AAstrawildGameState* GameState = GetAstrawildGameState();
    if (!GameState || !GameState->HasAuthority())
    {
        return;
    }

    const EAstrawildWeatherState OldState = GameState->WeatherState;
    if (OldState == NewState)
    {
        return;
    }

    GameState->SetWeatherState(NewState);
    OnWeatherChanged.Broadcast(NewState, OldState);
    UE_LOG(LogAstrawildWorld, Log, TEXT("Weather forced: %d -> %d."), static_cast<int32>(OldState), static_cast<int32>(NewState));
}

void UAstrawildWeatherSubsystem::RollNextWeather()
{
    AAstrawildGameState* GameState = GetAstrawildGameState();
    if (!GameState)
    {
        return;
    }

    // Weighted pick that avoids repeating the current regime twice in a row.
    const EAstrawildWeatherState Current = GameState->WeatherState;

    float TotalWeight = 0.0f;
    const TArray<EAstrawildWeatherState> AllStates = {
        EAstrawildWeatherState::Clear, EAstrawildWeatherState::Cloudy, EAstrawildWeatherState::Rain,
        EAstrawildWeatherState::HeavyRain, EAstrawildWeatherState::Storm, EAstrawildWeatherState::Fog,
        EAstrawildWeatherState::Heat, EAstrawildWeatherState::Cold
    };

    TArray<float> Weights;
    Weights.Reserve(AllStates.Num());
    for (const EAstrawildWeatherState State : AllStates)
    {
        float Weight = GetProfile(State).SelectionWeight;
        if (State == Current)
        {
            Weight *= 0.25f; // Strongly discourage repeats.
        }
        Weights.Add(Weight);
        TotalWeight += Weight;
    }

    float Roll = FMath::FRandRange(0.0f, TotalWeight);
    for (int32 i = 0; i < AllStates.Num(); ++i)
    {
        Roll -= Weights[i];
        if (Roll <= 0.0f)
        {
            ForceWeather(AllStates[i]);
            return;
        }
    }

    ForceWeather(EAstrawildWeatherState::Clear);
}

AAstrawildGameState* UAstrawildWeatherSubsystem::GetAstrawildGameState() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetGameState<AAstrawildGameState>() : nullptr;
}
