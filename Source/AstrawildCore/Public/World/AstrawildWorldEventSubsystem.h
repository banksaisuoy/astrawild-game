#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/AstrawildWorldEventData.h"
#include "AstrawildWorldEventSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAstrawildWorldEventStartedSignature, FGameplayTag, EventTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAstrawildWorldEventEndedSignature, FGameplayTag, EventTag);

UCLASS()
class ASTRAWILDCORE_API UAstrawildWorldEventSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|World Events")
    TObjectPtr<UDataTable> EventTable;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|World Events")
    FGameplayTag ActiveEventTag;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|World Events")
    float ActiveEventRemainingSeconds = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|World Events")
    TMap<FGameplayTag, float> EventCooldownRemaining;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|World Events|Events")
    FOnAstrawildWorldEventStartedSignature OnEventStarted;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|World Events|Events")
    FOnAstrawildWorldEventEndedSignature OnEventEnded;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|World Events")
    bool StartEvent(FGameplayTag EventTag);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|World Events")
    void EndActiveEvent();

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|World Events")
    void AdvanceEvents(float DeltaSeconds);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World Events")
    bool IsEventActive(FGameplayTag EventTag) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World Events")
    bool CanStartEvent(FGameplayTag EventTag) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World Events")
    bool GetEventData(FGameplayTag EventTag, FAstrawildWorldEventRow& OutEvent) const;

private:
    const FAstrawildWorldEventRow* FindEvent(FGameplayTag EventTag) const;
    bool HasAuthorityForEvents() const;
};
