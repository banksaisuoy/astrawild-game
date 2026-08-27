#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildLandscapeMaterialComponent.generated.h"

class UMaterialParameterCollection;

/**
 * Runtime bridge between ASTRAWILD world/weather data and the authored
 * MPC_AstrawildLandscape material graph. The MPC itself remains an Editor asset.
 */
UCLASS(ClassGroup=(Astrawild), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildLandscapeMaterialComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildLandscapeMaterialComponent();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Landscape|Material")
    TObjectPtr<UMaterialParameterCollection> LandscapeParameterCollection;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Landscape|Material")
    FName WetnessParameterName = TEXT("AW_LandscapeWetness");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Landscape|Material")
    FName RainIntensityParameterName = TEXT("AW_RainIntensity");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Landscape|Material")
    FName WindStrengthParameterName = TEXT("AW_WindStrength");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Landscape|Material")
    FName GrassSlopeMaxDegreesParameterName = TEXT("AW_GrassSlopeMaxDegrees");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Landscape|Material")
    FName RockSlopeStartDegreesParameterName = TEXT("AW_RockSlopeStartDegrees");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Landscape|Material")
    FName MeadowHeightMetersParameterName = TEXT("AW_MeadowHeightMeters");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Landscape|Material")
    FName MountainHeightMetersParameterName = TEXT("AW_MountainHeightMeters");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Landscape|Material", meta=(ClampMin="0.0", ClampMax="1.0"))
    float WetnessResponseSpeed = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Landscape|Material", meta=(ClampMin="0.0"))
    float WindStrength = 0.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Landscape|Material", meta=(ClampMin="0.0"))
    float GrassSlopeMaxDegrees = 18.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Landscape|Material", meta=(ClampMin="0.0"))
    float RockSlopeStartDegrees = 48.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Landscape|Material", meta=(ClampMin="0.0"))
    float MeadowHeightMeters = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Landscape|Material", meta=(ClampMin="0.0"))
    float MountainHeightMeters = 300.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Landscape|Material")
    float CurrentWetness = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Landscape|Material")
    float TargetWetness = 0.0f;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Landscape|Material")
    void RefreshWeatherParameters();

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Landscape|Material")
    void PushMaterialContract();

private:
    UFUNCTION()
    void HandleWeatherChanged(FGameplayTag WeatherTag);

    bool bWarnedMissingCollection = false;
};
