#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstrawildTypes.h"
#include "AstrawildWeatherSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildWeatherChanged, EAstrawildWeatherState, NewState, EAstrawildWeatherState, OldState);

/** Per-weather gameplay modifiers (directive §12). */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildWeatherProfile
{
    GENERATED_BODY()

    /** Ambient temperature offset in Celsius. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Weather")
    float TemperatureOffset = 0.0f;

    /** Relative weight in the random transition pool. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Weather", meta=(ClampMin="0.0"))
    float SelectionWeight = 1.0f;

    /** Visibility/sight perception multiplier (fog lowers it). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Weather", meta=(ClampMin="0.1", ClampMax="2.0"))
    float VisibilityMultiplier = 1.0f;
};

/**
 * Dynamic weather simulation (directive §12).
 * Server-only decision making on an in-world-hour cadence; state is replicated through
 * the game state. Weather feeds: temperature, creature behavior, capture bonuses, events.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildWeatherSubsystem : public UTickableWorldSubsystem
{
    GENERATED_BODY()

public:
    UAstrawildWeatherSubsystem();

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|World|Weather")
    FAstrawildWeatherChanged OnWeatherChanged;

    /** Minutes of in-world time each weather regime holds before a transition roll. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|World|Weather", meta=(ClampMin="10.0"))
    float WeatherChangeIntervalMinutes = 90.0f;

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World|Weather")
    EAstrawildWeatherState GetCurrentWeather() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World|Weather")
    float GetTemperatureOffsetCelsius() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World|Weather")
    float GetVisibilityMultiplier() const;

    /** Cheat/debug: force a weather state (server only). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|World|Weather")
    void ForceWeather(EAstrawildWeatherState NewState);

protected:
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

private:
    /** Audit H-4: returns by value — the old const& signature bound FindRef's by-value
     *  result to a dangling reference (undefined behaviour). */
    static FAstrawildWeatherProfile GetProfile(EAstrawildWeatherState State);

    /** Absolute in-world minutes (day*1440 + minute) of the last transition decision. */
    int64 LastDecisionAbsoluteMinutes = -1;

    void RollNextWeather();
    class AAstrawildGameState* GetAstrawildGameState() const;
};
