#include "AstrawildEventBusSubsystem.h"

#include "AstrawildLog.h"
#include "Engine/World.h"

bool UAstrawildEventBusSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UAstrawildEventBusSubsystem::Publish(const FAstrawildGameplayEvent& Event)
{
    if (!Event.EventTag.IsValid())
    {
        UE_LOG(LogAstrawildWorld, Warning, TEXT("Event bus rejected event with invalid tag."));
        return;
    }

    UE_LOG(LogAstrawildWorld, Verbose, TEXT("Event %s target=%s amount=%d"),
        *Event.EventTag.ToString(), *Event.TargetId.ToString(), Event.Amount);

    OnGameplayEvent.Broadcast(Event);
}

void UAstrawildEventBusSubsystem::PublishEvent(const FGameplayTag& EventTag, AActor* Instigator, const FName TargetId, const int32 Amount, const FVector& Location)
{
    FAstrawildGameplayEvent Event;
    Event.EventTag = EventTag;
    Event.Instigator = Instigator;
    Event.TargetId = TargetId;
    Event.Amount = Amount;
    Event.Location = Location;
    Publish(Event);
}
