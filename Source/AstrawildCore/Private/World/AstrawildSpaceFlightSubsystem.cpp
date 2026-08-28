#include "World/AstrawildSpaceFlightSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

UAstrawildSpaceFlightSubsystem::UAstrawildSpaceFlightSubsystem()
{
    TickIntervalSeconds = 0.05f;
    LowGravityScale = 0.16f;
    VacuumPressureLossKPaPerSecond = 18.0f;
    MinimumSafeCabinPressureKPa = 40.0f;
}

void UAstrawildSpaceFlightSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(FlightTimerHandle, this, &UAstrawildSpaceFlightSubsystem::HandleFlightTick, FMath::Max(0.01f, TickIntervalSeconds), true, FMath::Max(0.01f, TickIntervalSeconds));
    }
}

void UAstrawildSpaceFlightSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(FlightTimerHandle);
    }
    LaunchPads.Reset();
    PilotStates.Reset();
    LaunchTimers.Reset();
    RegisteredLaunchPadCount = 0;
    Super::Deinitialize();
}

bool UAstrawildSpaceFlightSubsystem::HasAuthorityForFlight() const
{
    const UWorld* World = GetWorld();
    return World && World->GetNetMode() != NM_Client;
}

bool UAstrawildSpaceFlightSubsystem::RegisterLaunchPad(const FAstrawildLaunchPadDefinition& Definition)
{
    if (!HasAuthorityForFlight() || !Definition.PadTag.IsValid() || !Definition.DestinationBiomeTag.IsValid())
    {
        return false;
    }
    FAstrawildLaunchPadDefinition Sanitized = Definition;
    Sanitized.InteractionRadius = FMath::Max(1.0f, Sanitized.InteractionRadius);
    Sanitized.LaunchDurationSeconds = FMath::Max(0.1f, Sanitized.LaunchDurationSeconds);
    LaunchPads.Add(Sanitized.PadTag, MoveTemp(Sanitized));
    RegisteredLaunchPadCount = LaunchPads.Num();
    return true;
}

bool UAstrawildSpaceFlightSubsystem::UnregisterLaunchPad(const FGameplayTag& PadTag)
{
    if (!HasAuthorityForFlight())
    {
        return false;
    }
    const int32 Removed = LaunchPads.Remove(PadTag);
    RegisteredLaunchPadCount = LaunchPads.Num();
    return Removed > 0;
}

bool UAstrawildSpaceFlightSubsystem::RequestLaunch(AActor* Pilot, const FGameplayTag& PadTag)
{
    if (!HasAuthorityForFlight() || !Pilot || !LaunchPads.Contains(PadTag))
    {
        return false;
    }
    const FAstrawildLaunchPadDefinition& Pad = LaunchPads[PadTag];
    if (FVector::DistSquared(Pilot->GetActorLocation(), Pad.WorldLocation) > FMath::Square(Pad.InteractionRadius))
    {
        return false;
    }
    FAstrawildSpaceFlightState& State = PilotStates.FindOrAdd(TWeakObjectPtr<AActor>(Pilot));
    if (State.FlightState != EAstrawildFlightState::Docked && State.FlightState != EAstrawildFlightState::Returning)
    {
        return false;
    }
    State.LowGravityScale = FMath::Clamp(LowGravityScale, 0.0f, 1.0f);
    State.LaunchProgressNormalized = 0.0f;
    State.CabinPressureKPa = 101.325f;
    State.bVacuumEmergency = false;
    LaunchTimers.Add(TWeakObjectPtr<AActor>(Pilot), Pad.LaunchDurationSeconds);
    SetPilotState(Pilot, EAstrawildFlightState::Launching);
    OnLaunchStarted.Broadcast(Pilot, Pad.DestinationBiomeTag);
    return true;
}

bool UAstrawildSpaceFlightSubsystem::UpdateFlightInput(AActor* Pilot, const FVector InputAcceleration, const float DeltaSeconds)
{
    if (!HasAuthorityForFlight() || !Pilot || DeltaSeconds <= 0.0f)
    {
        return false;
    }
    FAstrawildSpaceFlightState* State = PilotStates.Find(TWeakObjectPtr<AActor>(Pilot));
    if (!State || (State->FlightState != EAstrawildFlightState::InOrbit && State->FlightState != EAstrawildFlightState::VacuumEmergency))
    {
        return false;
    }

    const float SafeDelta = FMath::Min(DeltaSeconds, 0.1f);
    const FVector ClampedInput = InputAcceleration.GetClampedToMaxSize(1.0f);
    State->Velocity += ClampedInput * 1000.0f * SafeDelta;
    State->Velocity *= FMath::Pow(0.985f, SafeDelta * 60.0f);
    Pilot->SetActorLocation(Pilot->GetActorLocation() + State->Velocity * SafeDelta, true);
    return true;
}

bool UAstrawildSpaceFlightSubsystem::ReturnToSurface(AActor* Pilot)
{
    if (!HasAuthorityForFlight() || !Pilot)
    {
        return false;
    }
    FAstrawildSpaceFlightState* State = PilotStates.Find(TWeakObjectPtr<AActor>(Pilot));
    if (!State || (State->FlightState != EAstrawildFlightState::InOrbit && State->FlightState != EAstrawildFlightState::VacuumEmergency))
    {
        return false;
    }
    State->Velocity = FVector::ZeroVector;
    State->LaunchProgressNormalized = 0.0f;
    State->CabinPressureKPa = 101.325f;
    State->bVacuumEmergency = false;
    SetPilotState(Pilot, EAstrawildFlightState::Returning);
    return true;
}

void UAstrawildSpaceFlightSubsystem::AdvanceFlight(const float DeltaSeconds)
{
    if (!HasAuthorityForFlight() || DeltaSeconds <= 0.0f)
    {
        return;
    }
    const float SafeDelta = FMath::Min(DeltaSeconds, 0.25f);
    for (TPair<TWeakObjectPtr<AActor>, float>& Pair : LaunchTimers)
    {
        if (FAstrawildSpaceFlightState* State = PilotStates.Find(Pair.Key))
        {
            if (State->FlightState == EAstrawildFlightState::Launching)
            {
                const float LaunchDuration = FMath::Max(0.1f, Pair.Value);
                State->LaunchProgressNormalized = FMath::Clamp(State->LaunchProgressNormalized + SafeDelta / LaunchDuration, 0.0f, 1.0f);
                if (State->LaunchProgressNormalized >= 1.0f)
                {
                    SetPilotState(Pair.Key.Get(), EAstrawildFlightState::InOrbit);
                    Pair.Value = 0.0f;
                }
            }
        }
    }
    for (TPair<TWeakObjectPtr<AActor>, FAstrawildSpaceFlightState>& Pair : PilotStates)
    {
        FAstrawildSpaceFlightState& State = Pair.Value;
        if (State.FlightState == EAstrawildFlightState::InOrbit || State.FlightState == EAstrawildFlightState::VacuumEmergency)
        {
            State.CabinPressureKPa = FMath::Max(0.0f, State.CabinPressureKPa - VacuumPressureLossKPaPerSecond * SafeDelta);
            if (!State.bVacuumEmergency && State.CabinPressureKPa <= MinimumSafeCabinPressureKPa)
            {
                State.bVacuumEmergency = true;
                State.FlightState = EAstrawildFlightState::VacuumEmergency;
                if (AActor* Pilot = Pair.Key.Get())
                {
                    OnVacuumEmergency.Broadcast(Pilot);
                    OnFlightStateChanged.Broadcast(Pilot, State.FlightState);
                }
            }
        }
    }
}

bool UAstrawildSpaceFlightSubsystem::GetFlightState(AActor* Pilot, FAstrawildSpaceFlightState& OutState) const
{
    OutState = FAstrawildSpaceFlightState();
    if (!Pilot)
    {
        return false;
    }
    const FAstrawildSpaceFlightState* State = PilotStates.Find(TWeakObjectPtr<AActor>(Pilot));
    if (!State)
    {
        return false;
    }
    OutState = *State;
    return true;
}

bool UAstrawildSpaceFlightSubsystem::IsLaunchPadRegistered(const FGameplayTag& PadTag) const
{
    return LaunchPads.Contains(PadTag);
}

void UAstrawildSpaceFlightSubsystem::HandleFlightTick()
{
    AdvanceFlight(FMath::Max(0.01f, TickIntervalSeconds));
    RemoveInvalidPilots();
}

void UAstrawildSpaceFlightSubsystem::RemoveInvalidPilots()
{
    for (auto It = PilotStates.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            LaunchTimers.Remove(It.Key());
            It.RemoveCurrent();
        }
    }
}

void UAstrawildSpaceFlightSubsystem::SetPilotState(AActor* Pilot, const EAstrawildFlightState NewState)
{
    if (!Pilot)
    {
        return;
    }
    FAstrawildSpaceFlightState& State = PilotStates.FindOrAdd(TWeakObjectPtr<AActor>(Pilot));
    if (State.FlightState != NewState)
    {
        State.FlightState = NewState;
        OnFlightStateChanged.Broadcast(Pilot, NewState);
    }
}
