#include "World/AstrawildUnderwaterSubsystem.h"

#include "Engine/DataTable.h"

UAstrawildUnderwaterSubsystem::UAstrawildUnderwaterSubsystem()
{
    OxygenTankCapacitySeconds = 300.0f;
    SurfaceDepthMeters = 0.0f;
    AbyssalTrenchMinDepthMeters = 100.0f;
    AbyssalTrenchMaxDepthMeters = 1000.0f;
    PressureResistanceDepthMeters = 100.0f;
    BasePressureDamagePerSecond = 5.0f;
    BaseOxygenDrainPerSecond = 1.0f;
    SurfaceOxygenRefillPerSecond = 20.0f;
}

void UAstrawildUnderwaterSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    ActiveZoneTable = nullptr;
}

void UAstrawildUnderwaterSubsystem::Deinitialize()
{
    ActiveZoneTable = nullptr;
    Super::Deinitialize();
}

bool UAstrawildUnderwaterSubsystem::IsAbyssalTrenchDepth(const float DepthMeters) const
{
    const float SafeMinDepth = FMath::Max(SurfaceDepthMeters, AbyssalTrenchMinDepthMeters);
    const float SafeMaxDepth = FMath::Max(SafeMinDepth, AbyssalTrenchMaxDepthMeters);
    return FMath::IsWithinInclusive(DepthMeters, SafeMinDepth, SafeMaxDepth);
}

float UAstrawildUnderwaterSubsystem::CalculatePressureDamagePerSecond(const float DepthMeters, const bool bHasPressureProtection) const
{
    if (bHasPressureProtection || DepthMeters <= PressureResistanceDepthMeters)
    {
        return 0.0f;
    }

    const float SafeMaxDepth = FMath::Max(PressureResistanceDepthMeters + 1.0f, AbyssalTrenchMaxDepthMeters);
    const float PressureAlpha = FMath::Clamp(
        (DepthMeters - PressureResistanceDepthMeters) / (SafeMaxDepth - PressureResistanceDepthMeters),
        0.0f,
        1.0f);
    return FMath::Max(0.0f, BasePressureDamagePerSecond) * PressureAlpha;
}

float UAstrawildUnderwaterSubsystem::CalculateOxygenDrainPerSecond(const float DepthMeters, const bool bHasPressureProtection) const
{
    if (DepthMeters <= SurfaceDepthMeters)
    {
        return 0.0f;
    }

    const float PressureMultiplier = bHasPressureProtection ? 0.75f : 1.0f;
    const float AbyssMultiplier = IsAbyssalTrenchDepth(DepthMeters) ? 1.5f : 1.0f;
    return FMath::Max(0.0f, BaseOxygenDrainPerSecond) * PressureMultiplier * AbyssMultiplier;
}

float UAstrawildUnderwaterSubsystem::CalculateBuoyancyMultiplier(const float DepthMeters) const
{
    const float SafeDepth = FMath::Max(0.0f, DepthMeters);
    const float DepthAlpha = FMath::Clamp(SafeDepth / FMath::Max(1.0f, AbyssalTrenchMaxDepthMeters), 0.0f, 1.0f);
    return FMath::Lerp(1.0f, 0.85f, DepthAlpha);
}

FAstrawildUnderwaterState UAstrawildUnderwaterSubsystem::EvaluateDiverState(
    const float DepthMeters,
    const float OxygenRemainingSeconds,
    const bool bHasPressureProtection,
    const float DeltaSeconds) const
{
    FAstrawildUnderwaterState State;
    const float SafeDepth = FMath::Max(SurfaceDepthMeters, DepthMeters);
    const float SafeCapacity = FMath::Max(1.0f, OxygenTankCapacitySeconds);
    const float SafeDelta = FMath::Max(0.0f, DeltaSeconds);
    const bool bSubmerged = SafeDepth > SurfaceDepthMeters;
    const float OxygenDrain = CalculateOxygenDrainPerSecond(SafeDepth, bHasPressureProtection);

    State.DepthMeters = SafeDepth;
    State.bIsSubmerged = bSubmerged;
    State.bHasPressureProtection = bHasPressureProtection;
    State.PressureDamagePerSecond = CalculatePressureDamagePerSecond(SafeDepth, bHasPressureProtection);
    State.BuoyancyMultiplier = CalculateBuoyancyMultiplier(SafeDepth);
    State.OxygenRemainingSeconds = bSubmerged
        ? FMath::Clamp(OxygenRemainingSeconds - (OxygenDrain * SafeDelta), 0.0f, SafeCapacity)
        : FMath::Min(SafeCapacity, FMath::Max(0.0f, OxygenRemainingSeconds) + (FMath::Max(0.0f, SurfaceOxygenRefillPerSecond) * SafeDelta));
    State.OxygenNormalized = FMath::Clamp(State.OxygenRemainingSeconds / SafeCapacity, 0.0f, 1.0f);

    if (!bSubmerged)
    {
        State.MovementMode = EAstrawildUnderwaterMovementMode::Surface;
    }
    else if (State.OxygenRemainingSeconds <= KINDA_SMALL_NUMBER)
    {
        State.MovementMode = EAstrawildUnderwaterMovementMode::PressureEmergency;
    }
    else if (SafeDepth >= AbyssalTrenchMinDepthMeters)
    {
        State.MovementMode = EAstrawildUnderwaterMovementMode::Diving;
    }
    else
    {
        State.MovementMode = EAstrawildUnderwaterMovementMode::Swimming;
    }

    return State;
}

void UAstrawildUnderwaterSubsystem::SetActiveZoneTable(UDataTable* InZoneTable)
{
    ActiveZoneTable = InZoneTable;
}

bool UAstrawildUnderwaterSubsystem::GetActiveZoneRow(const FName RowName, FAstrawildUnderwaterZoneRow& OutRow) const
{
    OutRow = FAstrawildUnderwaterZoneRow();
    if (!ActiveZoneTable || RowName.IsNone())
    {
        return false;
    }

    const FAstrawildUnderwaterZoneRow* FoundRow = ActiveZoneTable->FindRow<FAstrawildUnderwaterZoneRow>(RowName, TEXT("UAstrawildUnderwaterSubsystem::GetActiveZoneRow"), false);
    if (!FoundRow)
    {
        return false;
    }

    OutRow = *FoundRow;
    return true;
}
