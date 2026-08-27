#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildColonyWorkComponent.generated.h"

class AAstrawildEchoBase;

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildWorkOrder
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Colony Work")
    FGuid WorkOrderId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Colony Work")
    FGameplayTag WorkTypeTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Colony Work")
    FGameplayTag OutputItemTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Colony Work", meta=(ClampMin="1"))
    int32 OutputQuantity = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Colony Work", meta=(ClampMin="1.0"))
    float RequiredWorkSeconds = 10.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colony Work")
    float ProgressSeconds = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Colony Work")
    FGuid AssignedWorkerId;

    FAstrawildWorkOrder()
        : WorkOrderId(FGuid::NewGuid())
        , OutputQuantity(1)
        , RequiredWorkSeconds(10.0f)
        , ProgressSeconds(0.0f)
    {
    }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWorkOrderCompletedSignature, FGameplayTag, OutputItemTag, int32, OutputQuantity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnWorkOrderFailedSignature, const FText&, FailureReason);

UCLASS(ClassGroup=(Astrawild), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildColonyWorkComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildColonyWorkComponent();

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Colony")
    TArray<FAstrawildWorkOrder> WorkOrders;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Colony")
    TArray<TWeakObjectPtr<AAstrawildEchoBase>> RegisteredWorkers;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Colony", meta=(ClampMin="1", ClampMax="32"))
    int32 MaxConcurrentOrders = 8;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Colony|Events")
    FOnWorkOrderCompletedSignature OnWorkOrderCompleted;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Colony|Events")
    FOnWorkOrderFailedSignature OnWorkOrderFailed;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Colony")
    bool RegisterWorker(AAstrawildEchoBase* Worker);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Colony")
    void UnregisterWorker(AAstrawildEchoBase* Worker);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Colony")
    bool QueueWorkOrder(const FGameplayTag& WorkTypeTag, const FGameplayTag& OutputItemTag, int32 OutputQuantity, float RequiredWorkSeconds);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Colony")
    bool HasAvailableWorker(const FGameplayTag& WorkTypeTag) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Colony")
    float GetWorkOrderPercent(const FGuid& WorkOrderId) const;

private:
    AAstrawildEchoBase* FindWorkerForOrder(const FAstrawildWorkOrder& Order) const;
    void PruneInvalidWorkers();
};
