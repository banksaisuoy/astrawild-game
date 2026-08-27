#include "Components/AstrawildColonyWorkComponent.h"

#include "Components/AstrawildSanComponent.h"
#include "Echoes/AstrawildEchoBase.h"
#include "GameFramework/Actor.h"

UAstrawildColonyWorkComponent::UAstrawildColonyWorkComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UAstrawildColonyWorkComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (DeltaTime <= 0.0f || !GetOwner() || !GetOwner()->HasAuthority())
    {
        return;
    }

    PruneInvalidWorkers();
    for (int32 OrderIndex = 0; OrderIndex < WorkOrders.Num(); ++OrderIndex)
    {
        FAstrawildWorkOrder& Order = WorkOrders[OrderIndex];
        if (!Order.AssignedWorkerId.IsValid())
        {
            if (AAstrawildEchoBase* Worker = FindWorkerForOrder(Order))
            {
                Order.AssignedWorkerId = Worker->InstanceData.UniqueEchoId;
                Worker->SetEchoState(EAstrawildEchoState::Working);
            }
        }

        AAstrawildEchoBase* AssignedWorker = nullptr;
        for (const TWeakObjectPtr<AAstrawildEchoBase>& Candidate : RegisteredWorkers)
        {
            if (Candidate.IsValid() && Candidate->InstanceData.UniqueEchoId == Order.AssignedWorkerId)
            {
                AssignedWorker = Candidate.Get();
                break;
            }
        }
        if (!AssignedWorker)
        {
            continue;
        }

        float WorkMultiplier = FMath::Max(0.1f, AssignedWorker->InstanceData.WorkEfficiencyMultiplier);
        if (AssignedWorker->San)
        {
            const float SanRatio = AssignedWorker->San->GetSANPercent();
            AssignedWorker->San->SetWorkStress(1.0f);
            if (SanRatio <= 0.2f)
            {
                WorkMultiplier *= 0.5f;
            }
        }
        Order.ProgressSeconds = FMath::Min(Order.RequiredWorkSeconds, Order.ProgressSeconds + DeltaTime * WorkMultiplier);
        if (Order.ProgressSeconds >= Order.RequiredWorkSeconds)
        {
            OnWorkOrderCompleted.Broadcast(Order.OutputItemTag, FMath::Max(1, Order.OutputQuantity));
            if (AssignedWorker->San)
            {
                AssignedWorker->San->SetWorkStress(0.0f);
            }
            WorkOrders.RemoveAt(OrderIndex);
            break;
        }
    }
}

bool UAstrawildColonyWorkComponent::RegisterWorker(AAstrawildEchoBase* Worker)
{
    if (!Worker || RegisteredWorkers.Contains(TWeakObjectPtr<AAstrawildEchoBase>(Worker)))
    {
        return false;
    }

    RegisteredWorkers.Add(Worker);
    return true;
}

void UAstrawildColonyWorkComponent::UnregisterWorker(AAstrawildEchoBase* Worker)
{
    RegisteredWorkers.Remove(Worker);
    if (!Worker)
    {
        return;
    }

    for (FAstrawildWorkOrder& Order : WorkOrders)
    {
        if (Order.AssignedWorkerId == Worker->InstanceData.UniqueEchoId)
        {
            Order.AssignedWorkerId.Invalidate();
        }
    }
    Worker->SetEchoState(EAstrawildEchoState::SummonedCompanion);
}

bool UAstrawildColonyWorkComponent::QueueWorkOrder(const FGameplayTag& WorkTypeTag, const FGameplayTag& OutputItemTag, const int32 OutputQuantity, const float RequiredWorkSeconds)
{
    if (!WorkTypeTag.IsValid() || !OutputItemTag.IsValid() || OutputQuantity <= 0 || RequiredWorkSeconds <= 0.0f)
    {
        OnWorkOrderFailed.Broadcast(FText::FromString(TEXT("Work order requires valid tags, quantity, and duration.")));
        return false;
    }
    if (WorkOrders.Num() >= FMath::Max(1, MaxConcurrentOrders))
    {
        OnWorkOrderFailed.Broadcast(FText::FromString(TEXT("The colony work queue is full.")));
        return false;
    }

    FAstrawildWorkOrder& NewOrder = WorkOrders.AddDefaulted_GetRef();
    NewOrder.WorkTypeTag = WorkTypeTag;
    NewOrder.OutputItemTag = OutputItemTag;
    NewOrder.OutputQuantity = OutputQuantity;
    NewOrder.RequiredWorkSeconds = RequiredWorkSeconds;
    return true;
}

bool UAstrawildColonyWorkComponent::HasAvailableWorker(const FGameplayTag& WorkTypeTag) const
{
    if (!WorkTypeTag.IsValid())
    {
        return false;
    }
    for (const TWeakObjectPtr<AAstrawildEchoBase>& Worker : RegisteredWorkers)
    {
        if (Worker.IsValid() && Worker->InstanceData.WorkSuitabilityTags.HasTag(WorkTypeTag) && Worker->San && !Worker->San->IsCritical())
        {
            return true;
        }
    }
    return false;
}

float UAstrawildColonyWorkComponent::GetWorkOrderPercent(const FGuid& WorkOrderId) const
{
    for (const FAstrawildWorkOrder& Order : WorkOrders)
    {
        if (Order.WorkOrderId == WorkOrderId)
        {
            return Order.RequiredWorkSeconds > 0.0f ? Order.ProgressSeconds / Order.RequiredWorkSeconds : 0.0f;
        }
    }
    return 0.0f;
}

AAstrawildEchoBase* UAstrawildColonyWorkComponent::FindWorkerForOrder(const FAstrawildWorkOrder& Order) const
{
    for (const TWeakObjectPtr<AAstrawildEchoBase>& Worker : RegisteredWorkers)
    {
        if (Worker.IsValid() && Worker->InstanceData.WorkSuitabilityTags.HasTag(Order.WorkTypeTag) && Worker->San && !Worker->San->IsCritical())
        {
            return Worker.Get();
        }
    }
    return nullptr;
}

void UAstrawildColonyWorkComponent::PruneInvalidWorkers()
{
    for (int32 Index = RegisteredWorkers.Num() - 1; Index >= 0; --Index)
    {
        if (!RegisteredWorkers[Index].IsValid())
        {
            RegisteredWorkers.RemoveAt(Index);
        }
    }
}
