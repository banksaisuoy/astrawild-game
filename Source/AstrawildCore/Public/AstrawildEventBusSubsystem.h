#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstrawildEventBusSubsystem.generated.h"

/**
 * Decoupled gameplay event bus (directive §38 event-driven communication).
 * Gameplay systems publish domain events (Event.* gameplay tags); quests, journal,
 * ecosystem and audio subscribe without knowing publishers.
 *
 * Server is the authority for gameplay-affecting events. In single player the same
 * call path serves both.
 */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildGameplayEvent
{
    GENERATED_BODY()

    /** Event.* gameplay tag identifying the event type. */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Event")
    FGameplayTag EventTag;

    /**
     * Actor that caused the event (player, echo...).
     * Audit C-5 (final run): TObjectPtr instead of TWeakObjectPtr — UHT 5.8 policy on
     * Blueprint-exposed weak pointers is stricter, and event payloads are short-lived
     * so a strong ref is semantically fine (nothing currently reads this member).
     */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Event")
    TObjectPtr<AActor> Instigator;

    /** Domain id: item id, echo definition, tech id, location name... */
    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Event")
    FName TargetId = NAME_None;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Event")
    int32 Amount = 0;

    UPROPERTY(BlueprintReadWrite, Category="ASTRAWILD|Event")
    FVector Location = FVector::ZeroVector;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildGameplayEventReceived, const FAstrawildGameplayEvent&, Event);

UCLASS()
class ASTRAWILDCORE_API UAstrawildEventBusSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    /** Fired for every published event. Filter by Event.EventTag. */
    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Event")
    FAstrawildGameplayEventReceived OnGameplayEvent;

    /** Publish a gameplay event. Server-side gameplay events should only be published on the server. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Event")
    void Publish(const FAstrawildGameplayEvent& Event);

    /** Convenience overload used by C++ systems. */
    void PublishEvent(const FGameplayTag& EventTag, AActor* Instigator, const FName TargetId = NAME_None, const int32 Amount = 0, const FVector& Location = FVector::ZeroVector);

    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
};
