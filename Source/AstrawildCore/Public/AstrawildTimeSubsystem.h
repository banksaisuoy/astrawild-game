#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstrawildTimeSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildDayChanged, int32, NewDayNumber, bool, bWasAutomatic);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildHourChanged, int32, HourOfDay);

/**
 * Deterministic, multiplayer-safe day/night clock (directive §13).
 * Server-only simulation; writes replicated state on the game state.
 * Default: 1 real second = 1 in-world minute (24-minute full day).
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildTimeSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    UAstrawildTimeSubsystem();

    /** Fires on server when the day number increments. */
    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|World|Time")
    FAstrawildDayChanged OnDayChanged;

    /** Fires on server at each in-world hour boundary. */
    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|World|Time")
    FAstrawildHourChanged OnHourChanged;

    /** In-world minutes advanced per real second. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|World|Time", meta=(ClampMin="0.0"))
    float MinutesPerRealSecond = 1.0f;

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World|Time")
    int32 GetCurrentMinute() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World|Time")
    int32 GetCurrentDay() const;

    /** Debug/cheat: jump the clock (server only). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|World|Time")
    void SetTimeOfDay(int32 Hour, int32 Minute);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|World|Time")
    void AdvanceDays(int32 NumDays);

protected:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
    float FractionalMinuteAccumulator = 0.0f;
    int32 LastBroadcastHour = -1;
    int32 LastBroadcastDay = -1;

    void ApplyMinutes(float InWorldMinutes);
    class AAstrawildGameState* GetAstrawildGameState() const;
};
