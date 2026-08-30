#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstrawildTypes.h"
#include "AstrawildWorldEventSubsystem.generated.h"

class UAstrawildWorldEventDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildWorldEventStateChanged, FName, EventId, bool, bStarted);

/**
 * Production V2 (Master Plan §19): the data-driven world-event scheduler.
 *
 * Nothing is hardcoded per event — every effect (research rewards, loot drops,
 * species boosts, night raids, meteor craters, forced weather) resolves from
 * UAstrawildWorldEventDefinition data registered in the content library.
 *
 * Scheduling is deterministic: the next roll time + per-event cooldowns are
 * absolute in-world minutes (seeded from the world seed), so a save/load
 * resumes the exact schedule. Active events + the roll clock persist in save
 * schema v4 (FAstrawildWorldEventScheduleSaveData).
 *
 * Server-authoritative: rolls, effect resolution and expiry run on the server;
 * players hear about events through toasts (Notify) and the event bus
 * (Event.WorldEventStarted / Event.WorldEventEnded — quest hookable).
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildWorldEventSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    /** Fired when an event starts or ends (server-side broadcast). */
    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|WorldEvent")
    FAstrawildWorldEventStateChanged OnWorldEventStateChanged;

    /** In-world minutes between scheduling rolls (default ~5 hours). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ASTRAWILD|WorldEvent")
    int32 RollIntervalMinutes = 300;

    /** Max events running at once (raid + surge together is plenty). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="ASTRAWILD|WorldEvent", meta=(ClampMin="1", ClampMax="3"))
    int32 MaxConcurrentEvents = 2;

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override
    {
        RETURN_QUICK_DECLARE_CYCLE_STAT(UAstrawildWorldEventSubsystem, STATGROUP_Tickables);
    }
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override
    {
        return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
    }

    /** Events currently running (id + definition). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|WorldEvent")
    TArray<FName> GetActiveEventIds() const;

    /** Display names of the running events (HUD banner). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|WorldEvent")
    FText GetActiveEventSummaryText() const;

    /** Absolute in-world minute of the next roll (exposed for tests/debug HUD). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|WorldEvent")
    int32 GetNextRollAbsoluteMinute() const { return NextRollAbsoluteMinute; }

    /** Minutes an event id still has on cooldown (0 = ready). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|WorldEvent")
    int32 GetCooldownRemainingMinutes(FName EventId) const;

    /** Save/load (schema v4). */
    void ExportForSave(FAstrawildWorldEventScheduleSaveData& OutData) const;
    void ImportFromSave(const FAstrawildWorldEventScheduleSaveData& InData);

    /**
     * Deterministic eligibility test (pure — unit-tested): day gate, night gate,
     * cooldown, concurrency and weight pool membership.
     */
    static bool IsEventEligible(const UAstrawildWorldEventDefinition* Definition, int32 CurrentAbsoluteMinute,
        int32 CurrentDay, int32 CurrentHour, int32 ActiveEventCount,
        const TMap<FName, int32>& CooldownEndMinutes, int32 MaxConcurrent);

    /** Deterministic weighted pick (pure — unit-tested). */
    static FName PickWeightedEvent(const TArray<UAstrawildWorldEventDefinition*>& Pool,
        const TMap<FName, int32>& CooldownEndMinutes, int32 CurrentAbsoluteMinute, int32 CurrentDay,
        int32 CurrentHour, int32 ActiveEventCount, int32 MaxConcurrent, FRandomStream& Stream);

    /** Force-start an event (cheat manager / tests). Resolves effects immediately. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|WorldEvent")
    bool ForceStartEvent(FName EventId);

private:
    /** Runtime active list (mirrors what gets saved). */
    TArray<FAstrawildWorldEventSaveData> ActiveEvents;

    TMap<FName, int32> CooldownEndMinutes;

    int32 NextRollAbsoluteMinute = 0;

    bool bScheduleInitialized = false;

    void InitializeSchedule();
    void RunRoll();
    bool StartEvent(const UAstrawildWorldEventDefinition* Definition);
    void ResolveEventEffects(const UAstrawildWorldEventDefinition* Definition, const FAstrawildWorldEventSaveData& Runtime);
    void EndExpiredEvents(int32 CurrentAbsoluteMinute);
    void EndEvent(FName EventId);
    int32 GetAbsoluteMinute() const;
    class UAstrawildTimeSubsystem* GetTime() const;
    class AAstrawildGameState* GetGameState() const;
    void BroadcastToast(const FText& Message) const;
};
