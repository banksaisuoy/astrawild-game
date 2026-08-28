#include "World/AstrawildLandscapeMaterialComponent.h"

#include "Engine/World.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Materials/MaterialParameterCollection.h"
#include "World/AstrawildWeatherSubsystem.h"

UAstrawildLandscapeMaterialComponent::UAstrawildLandscapeMaterialComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
}

void UAstrawildLandscapeMaterialComponent::BeginPlay()
{
    Super::BeginPlay();
    if (UWorld* World = GetWorld())
    {
        if (UAstrawildWeatherSubsystem* WeatherSubsystem = World->GetSubsystem<UAstrawildWeatherSubsystem>())
        {
            WeatherSubsystem->OnWeatherChanged.AddDynamic(this, &UAstrawildLandscapeMaterialComponent::HandleWeatherChanged);
        }
    }
    RefreshWeatherParameters();
    PushMaterialContract();
}

void UAstrawildLandscapeMaterialComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        if (UAstrawildWeatherSubsystem* WeatherSubsystem = World->GetSubsystem<UAstrawildWeatherSubsystem>())
        {
            WeatherSubsystem->OnWeatherChanged.RemoveDynamic(this, &UAstrawildLandscapeMaterialComponent::HandleWeatherChanged);
        }
    }
    Super::EndPlay(EndPlayReason);
}

void UAstrawildLandscapeMaterialComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    CurrentWetness = FMath::FInterpTo(CurrentWetness, TargetWetness, DeltaTime, FMath::Max(0.0f, WetnessResponseSpeed));
    PushMaterialContract();
}

void UAstrawildLandscapeMaterialComponent::RefreshWeatherParameters()
{
    TargetWetness = 0.0f;
    if (UWorld* World = GetWorld())
    {
        if (const UAstrawildWeatherSubsystem* WeatherSubsystem = World->GetSubsystem<UAstrawildWeatherSubsystem>())
        {
            FAstrawildWeatherRow Weather;
            if (WeatherSubsystem->GetCurrentWeather(Weather))
            {
                TargetWetness = FMath::Clamp(Weather.RainIntensity, 0.0f, 1.0f);
                WindStrength = FMath::Max(WindStrength, FMath::Clamp(Weather.WindStrength, 0.0f, 1.0f));
            }
        }
    }
}

void UAstrawildLandscapeMaterialComponent::PushMaterialContract()
{
    if (!LandscapeParameterCollection)
    {
        if (!bWarnedMissingCollection)
        {
            UE_LOG(LogTemp, Warning, TEXT("[ASTRAWILD][Landscape] LandscapeParameterCollection is not assigned; material graph remains unbound until MPC_AstrawildLandscape is authored in Editor."));
            bWarnedMissingCollection = true;
        }
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    UMaterialParameterCollectionInstance* Instance = World->GetParameterCollectionInstance(LandscapeParameterCollection);
    if (!Instance)
    {
        return;
    }
    Instance->SetScalarParameterValue(WetnessParameterName, CurrentWetness);
    Instance->SetScalarParameterValue(RainIntensityParameterName, TargetWetness);
    Instance->SetScalarParameterValue(WindStrengthParameterName, WindStrength);
    Instance->SetScalarParameterValue(GrassSlopeMaxDegreesParameterName, GrassSlopeMaxDegrees);
    Instance->SetScalarParameterValue(RockSlopeStartDegreesParameterName, RockSlopeStartDegrees);
    Instance->SetScalarParameterValue(MeadowHeightMetersParameterName, MeadowHeightMeters);
    Instance->SetScalarParameterValue(MountainHeightMetersParameterName, MountainHeightMeters);
}

void UAstrawildLandscapeMaterialComponent::HandleWeatherChanged(FGameplayTag WeatherTag)
{
    (void)WeatherTag;
    RefreshWeatherParameters();
}
