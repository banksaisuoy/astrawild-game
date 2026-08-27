#include "World/AstrawildWeatherSubsystem.h"

#include "Engine/DataTable.h"
#include "Engine/World.h"

bool UAstrawildWeatherSubsystem::SetWeather(const FGameplayTag& WeatherTag)
{
    if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
    {
        return false;
    }
    const FAstrawildWeatherRow* Row = FindWeatherByTag(WeatherTag);
    if (!Row)
    {
        return false;
    }
    if (CurrentWeatherTag == WeatherTag)
    {
        return true;
    }
    CurrentWeatherTag = WeatherTag;
    OnWeatherChanged.Broadcast(CurrentWeatherTag);
    return true;
}

bool UAstrawildWeatherSubsystem::SetWeatherByRow(const FName RowName)
{
    const FAstrawildWeatherRow* Row = FindWeatherByRow(RowName);
    return Row && SetWeather(Row->WeatherTag);
}

bool UAstrawildWeatherSubsystem::GetCurrentWeather(FAstrawildWeatherRow& OutWeather) const
{
    const FAstrawildWeatherRow* Row = FindWeatherByTag(CurrentWeatherTag);
    if (!Row)
    {
        return false;
    }
    OutWeather = *Row;
    return true;
}

int32 UAstrawildWeatherSubsystem::GetTemperatureModifier() const
{
    FAstrawildWeatherRow Weather;
    return GetCurrentWeather(Weather) ? Weather.TemperatureModifier : 0;
}

float UAstrawildWeatherSubsystem::GetVisibilityMultiplier() const
{
    FAstrawildWeatherRow Weather;
    return GetCurrentWeather(Weather) ? FMath::Clamp(Weather.VisibilityMultiplier, 0.0f, 1.0f) : 1.0f;
}

bool UAstrawildWeatherSubsystem::IsRaining() const
{
    FAstrawildWeatherRow Weather;
    return GetCurrentWeather(Weather) && Weather.RainIntensity > KINDA_SMALL_NUMBER;
}

const FAstrawildWeatherRow* UAstrawildWeatherSubsystem::FindWeatherByTag(const FGameplayTag& WeatherTag) const
{
    if (!WeatherTable || !WeatherTag.IsValid())
    {
        return nullptr;
    }
    for (const TPair<FName, uint8*>& Pair : WeatherTable->GetRowMap())
    {
        const FAstrawildWeatherRow* Row = reinterpret_cast<const FAstrawildWeatherRow*>(Pair.Value);
        if (Row && Row->WeatherTag == WeatherTag)
        {
            return Row;
        }
    }
    return nullptr;
}

const FAstrawildWeatherRow* UAstrawildWeatherSubsystem::FindWeatherByRow(const FName RowName) const
{
    return WeatherTable && !RowName.IsNone() ? WeatherTable->FindRow<FAstrawildWeatherRow>(RowName, TEXT("WeatherLookup")) : nullptr;
}
