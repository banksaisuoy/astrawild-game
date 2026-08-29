#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "AstrawildTypes.h"
#include "AstrawildGameState.generated.h"

/**
 * Replicated world state — single source of truth for time-of-day, day number and weather
 * (directive §13/§12/§28). Server-authoritative: only the server (via subsystems) writes;
 * clients read for rendering, audio and UI.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AAstrawildGameState();

    /** Minutes since midnight, 0..1439. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|World|Time", ReplicatedUsing=OnRep_TimeOfDayMinutes)
    int32 TimeOfDayMinutes = 8 * 60;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|World|Time", Replicated)
    int32 DayNumber = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|World|Weather", ReplicatedUsing=OnRep_WeatherState)
    EAstrawildWeatherState WeatherState = EAstrawildWeatherState::Clear;

    /** World generation seed — replicated so client-side procedural logic matches. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|World", Replicated)
    int32 WorldSeed = 1337;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World|Time")
    float GetTimeOfDayNormalized() const;

    /** Hour in 0..23 (e.g. 14.5 = 14:30). */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World|Time")
    float GetTimeOfDayHours() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World|Time")
    bool IsNight() const;

    /** 0 = dawn, 0.5 = noon, 1 = midnight phases mapped to sun curve. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World|Time")
    float GetSunCycleAlpha() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World|Time")
    FText GetTimeOfDayText() const;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Server-side setters (called by subsystems only). */
    void SetTimeOfDayMinutes(int32 InMinutes);
    void AdvanceDay();
    void SetWeatherState(EAstrawildWeatherState InState);
    void SetWorldSeed(int32 InSeed);

protected:
    UFUNCTION()
    void OnRep_TimeOfDayMinutes();

    UFUNCTION()
    void OnRep_WeatherState();
};
