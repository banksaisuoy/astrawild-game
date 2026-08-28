#include "World/AstrawildPowerGridSubsystem.h"

#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "World/AstrawildWorldClockSubsystem.h"

void UAstrawildPowerGridSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    RebuildDefaultsFromTable();

    if (GetWorld())
    {
        GetWorld()->GetTimerManager().SetTimer(
            SimulationTimerHandle,
            this,
            &UAstrawildPowerGridSubsystem::HandleSimulationTick,
            1.0f,
            true);
    }
}

void UAstrawildPowerGridSubsystem::Deinitialize()
{
    if (GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(SimulationTimerHandle);
    }
    GeneratorOutputWatts.Reset();
    ConsumerDemandWatts.Reset();
    ConsumerPriorities.Reset();
    ConsumerPowered.Reset();
    Super::Deinitialize();
}

bool UAstrawildPowerGridSubsystem::RegisterGenerator(const FGameplayTag GeneratorTag, const float OutputWatts)
{
    if (!HasAuthorityForGrid() || !GeneratorTag.IsValid() || OutputWatts < 0.0f)
    {
        return false;
    }
    GeneratorOutputWatts.Add(GeneratorTag, OutputWatts);
    return true;
}

bool UAstrawildPowerGridSubsystem::RegisterConsumer(const FGameplayTag ConsumerTag, const float DemandWatts, const float Priority)
{
    if (!HasAuthorityForGrid() || !ConsumerTag.IsValid() || DemandWatts < 0.0f || Priority < 0.0f)
    {
        return false;
    }
    ConsumerDemandWatts.Add(ConsumerTag, DemandWatts);
    ConsumerPriorities.Add(ConsumerTag, Priority);
    ConsumerPowered.FindOrAdd(ConsumerTag) = false;
    return true;
}

bool UAstrawildPowerGridSubsystem::UnregisterNode(const FGameplayTag NodeTag)
{
    if (!HasAuthorityForGrid() || !NodeTag.IsValid())
    {
        return false;
    }
    const bool bRemoved = GeneratorOutputWatts.Remove(NodeTag) > 0 || ConsumerDemandWatts.Remove(NodeTag) > 0;
    ConsumerPriorities.Remove(NodeTag);
    ConsumerPowered.Remove(NodeTag);
    return bRemoved;
}

void UAstrawildPowerGridSubsystem::SimulatePowerStep(const float DeltaSeconds)
{
    if (!HasAuthorityForGrid() || DeltaSeconds <= 0.0f)
    {
        return;
    }

    float generation = GetAvailableGenerationWatts();
    float demand = 0.0f;
    for (const TPair<FGameplayTag, float>& Consumer : ConsumerDemandWatts)
    {
        demand += FMath::Max(0.0f, Consumer.Value);
    }

    const float netWatts = generation - demand;
    float batteryDischargeWatts = 0.0f;
    if (netWatts >= 0.0f)
    {
        BatteryChargeWattHours = FMath::Min(BatteryCapacityWattHours, BatteryChargeWattHours + netWatts * DeltaSeconds / 3600.0f * FMath::Clamp(BatteryChargeEfficiency, 0.0f, 1.0f));
    }
    else
    {
        const float neededWattHours = -netWatts * DeltaSeconds / 3600.0f;
        const float suppliedWattHours = FMath::Min(BatteryChargeWattHours, neededWattHours);
        batteryDischargeWatts = DeltaSeconds > 0.0f ? suppliedWattHours * 3600.0f / DeltaSeconds : 0.0f;
        BatteryChargeWattHours = FMath::Max(0.0f, BatteryChargeWattHours - suppliedWattHours);
    }

    // Use the energy discharged during this step to determine consumer power;
    // do not derive available watts from the already-depleted charge.
    float availableWatts = generation + batteryDischargeWatts;
    TArray<FGameplayTag> consumerTags;
    ConsumerDemandWatts.GetKeys(consumerTags);
    consumerTags.Sort([this](const FGameplayTag& left, const FGameplayTag& right)
    {
        return ConsumerPriorities.FindRef(left) > ConsumerPriorities.FindRef(right);
    });
    for (const FGameplayTag& consumerTag : consumerTags)
    {
        const float required = ConsumerDemandWatts.FindRef(consumerTag);
        const bool bWasPowered = ConsumerPowered.FindRef(consumerTag);
        const bool bNowPowered = required <= availableWatts + KINDA_SMALL_NUMBER;
        ConsumerPowered.FindOrAdd(consumerTag) = bNowPowered;
        if (bNowPowered)
        {
            availableWatts -= required;
        }
        if (bWasPowered != bNowPowered)
        {
            OnConsumerStateChanged.Broadcast(consumerTag);
        }
    }
    OnPowerGridStateChanged.Broadcast(GetAvailableGenerationWatts(), BatteryChargeWattHours);
}

float UAstrawildPowerGridSubsystem::GetAvailableGenerationWatts() const
{
    float total = 0.0f;
    for (const TPair<FGameplayTag, float>& generator : GeneratorOutputWatts)
    {
        total += FMath::Max(0.0f, generator.Value);
    }
    return total;
}

float UAstrawildPowerGridSubsystem::GetNetPowerWatts() const
{
    float totalDemand = 0.0f;
    for (const TPair<FGameplayTag, float>& consumer : ConsumerDemandWatts)
    {
        totalDemand += FMath::Max(0.0f, consumer.Value);
    }
    return GetAvailableGenerationWatts() - totalDemand;
}

bool UAstrawildPowerGridSubsystem::IsConsumerPowered(const FGameplayTag ConsumerTag) const
{
    return ConsumerPowered.FindRef(ConsumerTag);
}

float UAstrawildPowerGridSubsystem::GetBatteryNormalized() const
{
    return BatteryCapacityWattHours > 0.0f ? FMath::Clamp(BatteryChargeWattHours / BatteryCapacityWattHours, 0.0f, 1.0f) : 0.0f;
}

bool UAstrawildPowerGridSubsystem::IsDaytime() const
{
    return !GetWorld() || !GetWorld()->GetSubsystem<UAstrawildWorldClockSubsystem>() || !GetWorld()->GetSubsystem<UAstrawildWorldClockSubsystem>()->IsNight();
}

void UAstrawildPowerGridSubsystem::HandleSimulationTick()
{
    SimulatePowerStep(1.0f);
}

void UAstrawildPowerGridSubsystem::RebuildDefaultsFromTable()
{
    if (!PowerNodeTable)
    {
        return;
    }
    TArray<FAstrawildPowerGridNodeRow*> rows;
    PowerNodeTable->GetAllRows<FAstrawildPowerGridNodeRow>(TEXT("AstrawildPowerGrid"), rows);
    for (const FAstrawildPowerGridNodeRow* row : rows)
    {
        if (!row || !row->NodeTag.IsValid())
        {
            continue;
        }
        if (row->NodeType == EAstrawildPowerNodeType::Generator)
        {
            RegisterGenerator(row->NodeTag, row->GenerationWatts);
        }
        else if (row->NodeType == EAstrawildPowerNodeType::Consumer)
        {
            RegisterConsumer(row->NodeTag, row->ConsumptionWatts, row->Priority);
        }
        else if (row->NodeType == EAstrawildPowerNodeType::Battery)
        {
            BatteryCapacityWattHours = FMath::Max(BatteryCapacityWattHours, row->StorageCapacityWattHours);
        }
    }
}

bool UAstrawildPowerGridSubsystem::HasAuthorityForGrid() const
{
    return !GetWorld() || GetWorld()->GetNetMode() != NM_Client;
}
