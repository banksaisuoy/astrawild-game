#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/AstrawildMountData.h"
#include "AstrawildMountComponent.generated.h"

class AAstrawildEchoBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEchoMountedSignature, AAstrawildEchoBase*, Mount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEchoDismountedSignature);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMountFailedSignature, const FText&, FailureReason);

UCLASS(ClassGroup=(Astrawild), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildMountComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildMountComponent();

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Mount")
    TMap<FName, FAstrawildMountProfile> MountProfiles;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Mount")
    TWeakObjectPtr<AAstrawildEchoBase> ActiveMount;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Mount")
    float ActiveSpeedMultiplier = 1.0f;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Mount|Events")
    FOnEchoMountedSignature OnMounted;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Mount|Events")
    FOnEchoDismountedSignature OnDismounted;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Mount|Events")
    FOnMountFailedSignature OnMountFailed;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Mount")
    bool CanMount(AAstrawildEchoBase* Echo, FText& OutFailureReason) const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Mount")
    bool TryMount(AAstrawildEchoBase* Echo);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Mount")
    void Dismount();

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Mount")
    bool IsMounted() const { return ActiveMount.IsValid(); }

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Mount")
    float GetActiveSpeedMultiplier() const { return ActiveSpeedMultiplier; }

private:
    TWeakObjectPtr<AActor> ActiveRider;
    void ApplyProfile(const FAstrawildMountProfile& Profile);
};
