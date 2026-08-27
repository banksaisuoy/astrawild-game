#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "World/AstrawildEnvironmentHazardComponent.generated.h"

class UAstrawildAttributeComponent;
class UAstrawildSurvivalComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHazardStateChangedSignature, int32, TemperatureLevel, float, Stress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHazardDamageSignature, FName, HazardId, float, Damage);

UCLASS(ClassGroup=(Astrawild), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildEnvironmentHazardComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildEnvironmentHazardComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Hazard", meta=(ClampMin="-5", ClampMax="5"))
    int32 AmbientTemperatureLevel = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Hazard", meta=(ClampMin="-5", ClampMax="5"))
    int32 InsulationLevel = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Hazard", meta=(ClampMin="-5", ClampMax="5"))
    int32 CampProtectionLevel = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Hazard", meta=(ClampMin="0.0"))
    float DamagePerStressPerSecond = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Hazard", meta=(ClampMin="0.1"))
    float DamageTickInterval = 2.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Hazard")
    float CurrentStress = 0.0f;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Hazard|Events")
    FOnHazardStateChangedSignature OnHazardStateChanged;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Hazard|Events")
    FOnHazardDamageSignature OnHazardDamage;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Hazard")
    void SetAmbientTemperature(int32 NewTemperatureLevel);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Hazard")
    void SetInsulationLevel(int32 NewInsulationLevel);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Hazard")
    void SetCampProtectionLevel(int32 NewProtectionLevel);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Hazard")
    float GetEffectiveTemperature() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Hazard")
    bool IsDangerous() const;

private:
    float TimeUntilNextDamage = 0.0f;
    TWeakObjectPtr<UAstrawildAttributeComponent> CachedAttributes;
    TWeakObjectPtr<UAstrawildSurvivalComponent> CachedSurvival;
    UAstrawildAttributeComponent* GetAttributes();
    UAstrawildSurvivalComponent* GetSurvival();
    void RecalculateStress();
    void SyncSurvivalTemperature();
};
