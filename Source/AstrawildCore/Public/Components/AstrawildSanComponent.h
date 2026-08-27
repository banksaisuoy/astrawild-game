#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildSanComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSanChangedSignature, float, CurrentSAN, float, MaxSAN);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSanStateChangedSignature, bool, bIsCritical);

UCLASS(ClassGroup=(Astrawild), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildSanComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildSanComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|SAN", meta=(ClampMin="1.0"))
    float MaxSAN = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|SAN", meta=(ClampMin="0.0"))
    float CurrentSAN = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|SAN", meta=(ClampMin="0.0"))
    float RecoveryPerSecond = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|SAN", meta=(ClampMin="0.0"))
    float WorkDecayPerSecond = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|SAN", meta=(ClampMin="0.0", ClampMax="1.0"))
    float CriticalThreshold = 0.2f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|SAN")
    bool bIsSANCritical = false;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|SAN|Events")
    FOnSanChangedSignature OnSANChanged;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|SAN|Events")
    FOnSanStateChangedSignature OnSANStateChanged;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|SAN")
    float ModifySAN(float DeltaSAN);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|SAN")
    void SetWorkStress(float Stress);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|SAN")
    float GetSANPercent() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|SAN")
    bool IsCritical() const { return bIsSANCritical; }

private:
    float CurrentWorkStress = 0.0f;
    void RefreshCriticalState();
};
