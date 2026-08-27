#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildSurvivalComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSurvivalValueChangedSignature, float, CurrentValue, float, MaxValue, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSurvivalWarningSignature, FName, WarningId);

UCLASS(ClassGroup=(Astrawild), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildSurvivalComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildSurvivalComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival", meta=(ClampMin="1.0"))
    float MaxHunger = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival", meta=(ClampMin="1.0"))
    float MaxThirst = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival", meta=(ClampMin="0.0"))
    float HungerDrainPerMinute = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival", meta=(ClampMin="0.0"))
    float ThirstDrainPerMinute = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival")
    float ComfortableTemperature = 21.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival")
    float TemperatureTolerance = 12.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival")
    float CurrentHunger = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival")
    float CurrentThirst = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival")
    float CurrentTemperature = 21.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Survival")
    float CarryWeight = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Survival")
    float CarryWeightCapacity = 100.0f;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Survival|Events")
    FOnSurvivalValueChangedSignature OnHungerChanged;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Survival|Events")
    FOnSurvivalValueChangedSignature OnThirstChanged;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Survival|Events")
    FOnSurvivalValueChangedSignature OnTemperatureChanged;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Survival|Events")
    FOnSurvivalWarningSignature OnSurvivalWarning;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Survival")
    bool ConsumeFood(float HungerRestored);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Survival")
    bool DrinkWater(float ThirstRestored);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Survival")
    void SetTemperature(float NewTemperature);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Survival")
    void SetCarryWeight(float NewWeight);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Survival")
    float GetHungerPercent() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Survival")
    float GetThirstPercent() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Survival")
    float GetTemperatureStress() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Survival")
    bool IsOverburdened() const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Save")
    void ExportToProfile(FAstrawildPlayerProfile& OutProfile) const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Save")
    void ImportFromProfile(const FAstrawildPlayerProfile& InProfile);

private:
    float WarningCooldownRemaining = 0.0f;
    void BroadcastWarnings();
};
