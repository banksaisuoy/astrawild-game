#include "World/AstrawildWorldEventSubsystem.h"

#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "World/AstrawildWeatherSubsystem.h"
#include "World/AstrawildWorldClockSubsystem.h"

void UAstrawildWorldEventSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UAstrawildWorldEventSubsystem::Deinitialize()
{
    ActiveEventTag = FGameplayTag::EmptyTag;
    ActiveEventRemainingSeconds = 0.0f;
    EventCooldownRemaining.Reset();
    Super::Deinitialize();
}

bool UAstrawildWorldEventSubsystem::StartEvent(const FGameplayTag EventTag)
{
    if (!HasAuthorityForEvents() || !CanStartEvent(EventTag))
    {
        return false;
    }
    const FAstrawildWorldEventRow* Event = FindEvent(EventTag);
    if (!Event)
    {
        return false;
    }
    ActiveEventTag = EventTag;
    ActiveEventRemainingSeconds = FMath::Max(30.0f, Event->DurationSeconds);
    OnEventStarted.Broadcast(EventTag);
    return true;
}

void UAstrawildWorldEventSubsystem::EndActiveEvent()
{
    if (!HasAuthorityForEvents() || !ActiveEventTag.IsValid())
    {
        return;
    }
    const FGameplayTag endedTag = ActiveEventTag;
    if (const FAstrawildWorldEventRow* Event = FindEvent(endedTag))
    {
        EventCooldownRemaining.FindOrAdd(endedTag) = FMath::Max(0.0f, Event->CooldownSeconds);
    }
    ActiveEventTag = FGameplayTag::EmptyTag;
    ActiveEventRemainingSeconds = 0.0f;
    OnEventEnded.Broadcast(endedTag);
}

void UAstrawildWorldEventSubsystem::AdvanceEvents(const float DeltaSeconds)
{
    if (!HasAuthorityForEvents() || DeltaSeconds <= 0.0f)
    {
        return;
    }
    for (auto& cooldown : EventCooldownRemaining)
    {
        cooldown.Value = FMath::Max(0.0f, cooldown.Value - DeltaSeconds);
    }
    if (ActiveEventTag.IsValid())
    {
        ActiveEventRemainingSeconds = FMath::Max(0.0f, ActiveEventRemainingSeconds - DeltaSeconds);
        if (ActiveEventRemainingSeconds <= 0.0f)
        {
            EndActiveEvent();
        }
    }
}

bool UAstrawildWorldEventSubsystem::IsEventActive(const FGameplayTag EventTag) const
{
    return EventTag.IsValid() && ActiveEventTag == EventTag && ActiveEventRemainingSeconds > 0.0f;
}

bool UAstrawildWorldEventSubsystem::CanStartEvent(const FGameplayTag EventTag) const
{
    const FAstrawildWorldEventRow* Event = FindEvent(EventTag);
    if (!Event || ActiveEventTag.IsValid() || EventCooldownRemaining.FindRef(EventTag) > 0.0f)
    {
        return false;
    }
    if (Event->bRequiresNight)
    {
        const UAstrawildWorldClockSubsystem* clock = GetWorld() ? GetWorld()->GetSubsystem<UAstrawildWorldClockSubsystem>() : nullptr;
        if (!clock || !clock->IsNight())
        {
            return false;
        }
    }
    if (Event->bRequiresStorm)
    {
        const UAstrawildWeatherSubsystem* weather = GetWorld() ? GetWorld()->GetSubsystem<UAstrawildWeatherSubsystem>() : nullptr;
        if (!weather || weather->CurrentWeatherTag != FGameplayTag::RequestGameplayTag(FName(TEXT("Weather.Storm")), false))
        {
            return false;
        }
    }
    return true;
}

bool UAstrawildWorldEventSubsystem::GetEventData(const FGameplayTag EventTag, FAstrawildWorldEventRow& OutEvent) const
{
    const FAstrawildWorldEventRow* Event = FindEvent(EventTag);
    if (!Event)
    {
        OutEvent = FAstrawildWorldEventRow();
        return false;
    }
    OutEvent = *Event;
    return true;
}

const FAstrawildWorldEventRow* UAstrawildWorldEventSubsystem::FindEvent(const FGameplayTag EventTag) const
{
    if (!EventTable || !EventTag.IsValid())
    {
        return nullptr;
    }
    TArray<FAstrawildWorldEventRow*> rows;
    EventTable->GetAllRows<FAstrawildWorldEventRow>(TEXT("AstrawildWorldEventLookup"), rows);
    FAstrawildWorldEventRow** Found = rows.FindByPredicate([EventTag](const FAstrawildWorldEventRow* row)
    {
        return row && row->EventTag == EventTag;
    });
    return Found ? *Found : nullptr;
}

bool UAstrawildWorldEventSubsystem::HasAuthorityForEvents() const
{
    return !GetWorld() || GetWorld()->GetNetMode() != NM_Client;
}
