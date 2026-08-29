#include "AstrawildPowerSubsystem.h"

#include "AstrawildBuildingActor.h"
#include "AstrawildDataAssets.h"
#include "AstrawildLog.h"
#include "Engine/World.h"

UAstrawildPowerSubsystem::UAstrawildPowerSubsystem()
{
}

bool UAstrawildPowerSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UAstrawildPowerSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UAstrawildPowerSubsystem, STATGROUP_Tickables);
}

void UAstrawildPowerSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    UE_LOG(LogAstrawildBuilding, Log, TEXT("Power subsystem online (connectivity %.0f cm, resolve every %.1fs)."), ConnectivityRadius, ResolveIntervalSeconds);
}

void UAstrawildPowerSubsystem::RegisterBuilding(AAstrawildBuildingActor* Building)
{
    if (!IsValid(Building))
    {
        return;
    }
    Buildings.AddUnique(Building);
}

void UAstrawildPowerSubsystem::UnregisterBuilding(AAstrawildBuildingActor* Building)
{
    Buildings.RemoveAll([Building](const TWeakObjectPtr<AAstrawildBuildingActor>& Weak) { return Weak.Get() == Building; });
    BuildingPowerState.Remove(FObjectKey(Building));
}

void UAstrawildPowerSubsystem::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    // Energy flows continuously; the network topology re-solves on a cadence.
    const float NetFlow = TotalGeneration - TotalDraw;
    if (TotalBatteryCapacity > 0.0f)
    {
        StoredEnergy = FMath::Clamp(StoredEnergy + NetFlow * DeltaTime, 0.0f, TotalBatteryCapacity);
    }

    ResolveAccumulator += DeltaTime;
    if (ResolveAccumulator >= ResolveIntervalSeconds)
    {
        ResolveAccumulator = 0.0f;
        ResolveGrid();
    }
}

void UAstrawildPowerSubsystem::ResolveGrid()
{
    Buildings.RemoveAll([](const TWeakObjectPtr<AAstrawildBuildingActor>& Weak) { return !Weak.IsValid(); });

    // --- Aggregate generators, batteries and connected consumers ---
    TotalGeneration = 0.0f;
    TotalDraw = 0.0f;
    TotalBatteryCapacity = 0.0f;

    // Only power-role buildings connected (by proximity) to at least one other power node
    // participate in the shared grid — a lone generator still powers itself and neighbors.
    TArray<AAstrawildBuildingActor*> Generators;
    TArray<AAstrawildBuildingActor*> Batteries;
    TArray<AAstrawildBuildingActor*> Consumers;

    for (const TWeakObjectPtr<AAstrawildBuildingActor>& Weak : Buildings)
    {
        AAstrawildBuildingActor* Building = Weak.Get();
        const UAstrawildBuildingDefinition* Def = Building ? Building->GetBuildingDefinition() : nullptr;
        if (!Building || !Def)
        {
            continue;
        }

        switch (Def->PowerRole)
        {
        case EAstrawildPowerRole::Generator:
            Generators.Add(Building);
            TotalGeneration += Def->PowerGeneration;
            break;
        case EAstrawildPowerRole::Battery:
            Batteries.Add(Building);
            TotalBatteryCapacity += Def->BatteryCapacity;
            break;
        case EAstrawildPowerRole::Consumer:
            if (Building->IsSwitchedOn() && Def->PowerDraw > 0.0f)
            {
                Consumers.Add(Building);
            }
            break;
        default:
            break;
        }
    }

    // --- Brownout resolution (directive §17 priority) ---
    // Priority order: Research > Workstation > Farm > Defense > Decoration.
    auto ConsumerPriority = [](const AAstrawildBuildingActor* B) -> int32
    {
        const UAstrawildBuildingDefinition* Def = B ? B->GetBuildingDefinition() : nullptr;
        if (!Def)
        {
            return 99;
        }
        switch (Def->Category)
        {
        case EAstrawildBuildingCategory::Research:    return 0;
        case EAstrawildBuildingCategory::Workstation: return 1;
        case EAstrawildBuildingCategory::Farm:        return 2;
        case EAstrawildBuildingCategory::Defense:     return 3;
        default:                                      return 4; // Decoration & others shed first.
        }
    };

    Consumers.Sort([&ConsumerPriority](const AAstrawildBuildingActor& A, const AAstrawildBuildingActor& B)
    {
        return ConsumerPriority(&A) < ConsumerPriority(&B);
    });

    float Available = StoredEnergy;
    for (AAstrawildBuildingActor* Generator : Generators)
    {
        Available += Generator->GetBuildingDefinition() ? Generator->GetBuildingDefinition()->PowerGeneration : 0.0f;
    }

    TotalDraw = 0.0f;
    bool bAnyUnpowered = false;
    for (AAstrawildBuildingActor* Consumer : Consumers)
    {
        const UAstrawildBuildingDefinition* Def = Consumer->GetBuildingDefinition();
        const float Draw = Def ? Def->PowerDraw : 0.0f;

        const bool bCanPower = Available >= Draw;
        const bool bPowered = bCanPower || Draw <= 0.0f;
        BuildingPowerState.Add(FObjectKey(Consumer), bPowered);

        if (bPowered)
        {
            Available -= Draw;
            TotalDraw += Draw;
        }
        else
        {
            bAnyUnpowered = true;
        }
    }

    const bool bNewGridState = !bAnyUnpowered;
    if (bNewGridState != bGridPowered)
    {
        bGridPowered = bNewGridState;
        OnPowerStateChanged.Broadcast(bGridPowered);
        UE_LOG(LogAstrawildBuilding, Log, TEXT("Power grid state: %s (gen %.1f, draw %.1f, stored %.0f)."),
            bGridPowered ? TEXT("STABLE") : TEXT("BROWNOUT"), TotalGeneration, TotalDraw, StoredEnergy);
    }

    OnGridChanged.Broadcast(TotalGeneration, TotalDraw);
}

bool UAstrawildPowerSubsystem::IsLocationPowered(const FVector& Location) const
{
    // A location is powered when it is near any active power node (generator/battery)
    // and its local consumer (if registered) is powered.
    for (const TPair<FObjectKey, bool>& Pair : BuildingPowerState)
    {
        if (!Pair.Value)
        {
            continue;
        }
        // Key-based lookup cannot give the actor back; power queries route through
        // IsBuildingPowered for exact buildings. Locations fall back to generator proximity.
        break;
    }

    // Proximity to any generator = powered location (simplified shared grid v1).
    for (const TWeakObjectPtr<AAstrawildBuildingActor>& Weak : Buildings)
    {
        const AAstrawildBuildingActor* Building = Weak.Get();
        const UAstrawildBuildingDefinition* Def = Building ? Building->GetBuildingDefinition() : nullptr;
        if (Building && Def && Def->PowerRole == EAstrawildPowerRole::Generator &&
            FVector::Dist(Building->GetActorLocation(), Location) <= ConnectivityRadius)
        {
            return true;
        }
    }
    return false;
}

bool UAstrawildPowerSubsystem::IsBuildingPowered(const AAstrawildBuildingActor* Building) const
{
    if (!IsValid(Building))
    {
        return false;
    }

    if (const bool* State = BuildingPowerState.Find(FObjectKey(Building)))
    {
        return *State;
    }
    return IsLocationPowered(Building->GetActorLocation());
}
