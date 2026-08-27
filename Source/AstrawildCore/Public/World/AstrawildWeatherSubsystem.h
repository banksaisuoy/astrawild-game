#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/AstrawildWeatherData.h"
#include "AstrawildWeatherSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWeatherChangedSignature, FGameplayTag, WeatherTag);

UCLASS()
class ASTRAWILDCORE_API UAstrawildWeatherSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Weather")
    TObjectPtr<UDataTable> WeatherTable;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Weather")
    FGameplayTag CurrentWeatherTag;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Weather|Events")
    FOnWeatherChangedSignature OnWeatherChanged;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Weather")
    bool SetWeather(const FGameplayTag& WeatherTag);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Weather")
    bool SetWeatherByRow(FName RowName);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Weather")
    bool GetCurrentWeather(FAstrawildWeatherRow& OutWeather) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Weather")
    int32 GetTemperatureModifier() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Weather")
    float GetVisibilityMultiplier() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Weather")
    bool IsRaining() const;

private:
    const FAstrawildWeatherRow* FindWeatherByTag(const FGameplayTag& WeatherTag) const;
    const FAstrawildWeatherRow* FindWeatherByRow(FName RowName) const;
};
