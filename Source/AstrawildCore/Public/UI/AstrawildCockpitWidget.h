#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "AstrawildCockpitWidget.generated.h"

class AActor;
class UAstrawildMechaComponent;

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildCockpitState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Cockpit")
    bool bHasTargetLock = false;

    UPROPERTY(BlueprintReadOnly, Category="Cockpit")
    FText TargetName;

    UPROPERTY(BlueprintReadOnly, Category="Cockpit")
    float TargetDistance = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Cockpit")
    float EnergyNormalized = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Cockpit")
    float HeatNormalized = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Cockpit")
    float ShieldNormalized = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Cockpit")
    bool bIsOverboosting = false;

    UPROPERTY(BlueprintReadOnly, Category="Cockpit")
    bool bIsOverheated = false;

    UPROPERTY(BlueprintReadOnly, Category="Cockpit")
    bool bIsFlying = false;

    UPROPERTY(BlueprintReadOnly, Category="Cockpit")
    FGameplayTag EquippedWeaponTag;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAstrawildCockpitStateChangedSignature, const FAstrawildCockpitState&, State);

UCLASS(Blueprintable)
class ASTRAWILDCORE_API UAstrawildCockpitWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|UI|Cockpit")
    FAstrawildCockpitState CockpitState;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|UI|Cockpit|Events")
    FOnAstrawildCockpitStateChangedSignature OnCockpitStateChanged;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|UI|Cockpit")
    void RefreshCockpitState();

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|UI|Cockpit")
    void SetTargetLock(AActor* TargetActor, bool bLocked);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|UI|Cockpit")
    bool HasTargetLock() const { return CockpitState.bHasTargetLock; }

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|UI|Cockpit")
    float GetEnergyPercent() const { return CockpitState.EnergyNormalized; }

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|UI|Cockpit")
    float GetHeatPercent() const { return CockpitState.HeatNormalized; }

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|UI|Cockpit")
    float GetShieldPercent() const { return CockpitState.ShieldNormalized; }

private:
    TWeakObjectPtr<AActor> LockedTarget;
    TWeakObjectPtr<UAstrawildMechaComponent> BoundMecha;
    float RefreshAccumulator = 0.0f;
    void BindMechaIfNeeded();
};
