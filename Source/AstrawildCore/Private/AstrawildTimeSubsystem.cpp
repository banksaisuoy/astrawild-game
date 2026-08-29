#include "AstrawildTimeSubsystem.h"

#include "AstrawildCore.h"
#include "AstrawildGameState.h"
#include "AstrawildLog.h"
#include "Engine/World.h"

UAstrawildTimeSubsystem::UAstrawildTimeSubsystem()
{
    // Defaults set in header for data-asset migration later.
}

bool UAstrawildTimeSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UAstrawildTimeSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UAstrawildTimeSubsystem, STATGROUP_Tickables);
}

void UAstrawildTimeSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    if (InWorld.GetNetMode() != NM_Client)
    {
        UE_LOG(LogAstrawildWorld, Log, TEXT("Time subsystem online — %.1f in-world minutes per real second."), MinutesPerRealSecond);
    }
}

void UAstrawildTimeSubsystem::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    ApplyMinutes(DeltaTime * MinutesPerRealSecond);
}

void UAstrawildTimeSubsystem::ApplyMinutes(const float InWorldMinutes)
{
    AAstrawildGameState* GameState = GetAstrawildGameState();
    if (!GameState)
    {
        return;
    }

    FractionalMinuteAccumulator += InWorldMinutes;
    const int32 WholeMinutes = FMath::FloorToInt32(FractionalMinuteAccumulator);
    if (WholeMinutes <= 0)
    {
        return;
    }
    FractionalMinuteAccumulator -= WholeMinutes;

    int32 NewMinutes = GameState->TimeOfDayMinutes + WholeMinutes;
    int32 DayCarry = 0;
    const int32 MinutesPerDay = 24 * 60;
    while (NewMinutes >= MinutesPerDay)
    {
        NewMinutes -= MinutesPerDay;
        ++DayCarry;
    }

    GameState->SetTimeOfDayMinutes(NewMinutes);

    const int32 CurrentHour = NewMinutes / 60;
    if (CurrentHour != LastBroadcastHour)
    {
        LastBroadcastHour = CurrentHour;
        OnHourChanged.Broadcast(CurrentHour);
    }

    for (int32 i = 0; i < DayCarry; ++i)
    {
        GameState->AdvanceDay();
    }
    if (DayCarry > 0 && GameState->DayNumber != LastBroadcastDay)
    {
        LastBroadcastDay = GameState->DayNumber;
        OnDayChanged.Broadcast(GameState->DayNumber, true);
    }
}

int32 UAstrawildTimeSubsystem::GetCurrentMinute() const
{
    const AAstrawildGameState* GameState = GetAstrawildGameState();
    return GameState ? GameState->TimeOfDayMinutes : 0;
}

int32 UAstrawildTimeSubsystem::GetCurrentDay() const
{
    const AAstrawildGameState* GameState = GetAstrawildGameState();
    return GameState ? GameState->DayNumber : 1;
}

void UAstrawildTimeSubsystem::SetTimeOfDay(const int32 Hour, const int32 Minute)
{
    AAstrawildGameState* GameState = GetAstrawildGameState();
    if (!GameState || !GameState->HasAuthority())
    {
        return;
    }

    const int32 ClampedHour = FMath::Clamp(Hour, 0, 23);
    const int32 ClampedMinute = FMath::Clamp(Minute, 0, 59);
    GameState->SetTimeOfDayMinutes(ClampedHour * 60 + ClampedMinute);
    UE_LOG(LogAstrawildWorld, Log, TEXT("Time set to %02d:%02d."), ClampedHour, ClampedMinute);
}

void UAstrawildTimeSubsystem::AdvanceDays(const int32 NumDays)
{
    AAstrawildGameState* GameState = GetAstrawildGameState();
    if (!GameState || !GameState->HasAuthority() || NumDays <= 0)
    {
        return;
    }

    for (int32 i = 0; i < NumDays; ++i)
    {
        GameState->AdvanceDay();
    }
    OnDayChanged.Broadcast(GameState->DayNumber, false);
}

AAstrawildGameState* UAstrawildTimeSubsystem::GetAstrawildGameState() const
{
    const UWorld* World = GetWorld();
    return World ? World->GetGameState<AAstrawildGameState>() : nullptr;
}
