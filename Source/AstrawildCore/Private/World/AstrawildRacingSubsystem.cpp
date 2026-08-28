#include "World/AstrawildRacingSubsystem.h"

#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"

UAstrawildRacingSubsystem::UAstrawildRacingSubsystem()
{
    TickIntervalSeconds = 0.05f;
    DefaultCheckpointRadius = 400.0f;
}

void UAstrawildRacingSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().SetTimer(
            RaceTimerHandle,
            this,
            &UAstrawildRacingSubsystem::HandleRaceTick,
            FMath::Max(0.01f, TickIntervalSeconds),
            true,
            FMath::Max(0.01f, TickIntervalSeconds));
    }
}

void UAstrawildRacingSubsystem::Deinitialize()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(RaceTimerHandle);
    }
    ParticipantStates.Reset();
    TrackDefinitions.Reset();
    Super::Deinitialize();
}

bool UAstrawildRacingSubsystem::HasAuthorityForRacing() const
{
    const UWorld* World = GetWorld();
    return World && World->GetNetMode() != NM_Client;
}

bool UAstrawildRacingSubsystem::RegisterTrack(const FAstrawildRaceTrackDefinition& TrackDefinition)
{
    if (!HasAuthorityForRacing() || !TrackDefinition.TrackTag.IsValid() || TrackDefinition.Checkpoints.Num() < 2)
    {
        return false;
    }

    FAstrawildRaceTrackDefinition Sanitized = TrackDefinition;
    Sanitized.LapCount = FMath::Max(1, Sanitized.LapCount);
    for (FAstrawildRaceCheckpoint& Checkpoint : Sanitized.Checkpoints)
    {
        Checkpoint.Radius = FMath::Max(1.0f, Checkpoint.Radius > 0.0f ? Checkpoint.Radius : DefaultCheckpointRadius);
    }
    TrackDefinitions.Add(Sanitized.TrackTag, MoveTemp(Sanitized));
    return true;
}

bool UAstrawildRacingSubsystem::StartRace(const FGameplayTag& TrackTag)
{
    if (!HasAuthorityForRacing() || bRaceActive || !TrackTag.IsValid() || !TrackDefinitions.Contains(TrackTag))
    {
        return false;
    }

    ActiveTrackTag = TrackTag;
    RaceElapsedSeconds = 0.0f;
    bRaceActive = true;
    for (TPair<TWeakObjectPtr<AActor>, FAstrawildRaceParticipantState>& Pair : ParticipantStates)
    {
        Pair.Value = FAstrawildRaceParticipantState();
    }
    OnRaceStarted.Broadcast(ActiveTrackTag);
    return true;
}

void UAstrawildRacingSubsystem::EndRace()
{
    if (!HasAuthorityForRacing())
    {
        return;
    }
    bRaceActive = false;
    ActiveTrackTag = FGameplayTag();
    RaceElapsedSeconds = 0.0f;
    ParticipantStates.Reset();
}

bool UAstrawildRacingSubsystem::RegisterParticipant(AActor* Participant)
{
    if (!HasAuthorityForRacing() || !Participant || !bRaceActive || ParticipantStates.Contains(TWeakObjectPtr<AActor>(Participant)))
    {
        return false;
    }
    ParticipantStates.Add(TWeakObjectPtr<AActor>(Participant), FAstrawildRaceParticipantState());
    return true;
}

bool UAstrawildRacingSubsystem::UnregisterParticipant(AActor* Participant)
{
    if (!HasAuthorityForRacing() || !Participant)
    {
        return false;
    }
    return ParticipantStates.Remove(Participant) > 0;
}

const FAstrawildRaceTrackDefinition* UAstrawildRacingSubsystem::FindActiveTrack() const
{
    return TrackDefinitions.Find(ActiveTrackTag);
}

bool UAstrawildRacingSubsystem::SubmitCheckpoint(
    AActor* Participant,
    const int32 CheckpointIndex,
    const FVector ReportedWorldLocation)
{
    if (!HasAuthorityForRacing() || !bRaceActive || !Participant)
    {
        return false;
    }

    const FAstrawildRaceTrackDefinition* Track = FindActiveTrack();
    FAstrawildRaceParticipantState* State = ParticipantStates.Find(TWeakObjectPtr<AActor>(Participant));
    if (!Track || !State || State->bFinished || !Track->Checkpoints.IsValidIndex(CheckpointIndex))
    {
        return false;
    }
    if (CheckpointIndex != State->NextCheckpointIndex)
    {
        return false;
    }

    const FAstrawildRaceCheckpoint& Checkpoint = Track->Checkpoints[CheckpointIndex];
    const float Radius = FMath::Max(1.0f, Checkpoint.Radius);
    const FVector ActualLocation = Participant->GetActorLocation();
    const FVector TrustedLocation = ActualLocation;
    if (FVector::DistSquared(TrustedLocation, Checkpoint.WorldLocation) > FMath::Square(Radius))
    {
        return false;
    }
    if (FVector::DistSquared(ReportedWorldLocation, ActualLocation) > FMath::Square(Radius * 2.0f))
    {
        return false;
    }

    State->NextCheckpointIndex++;
    if (State->NextCheckpointIndex >= Track->Checkpoints.Num())
    {
        State->NextCheckpointIndex = 0;
        State->CompletedLaps++;
    }
    OnCheckpointValidated.Broadcast(Participant, CheckpointIndex, State->CompletedLaps);

    if (State->CompletedLaps >= Track->LapCount)
    {
        State->bFinished = true;
        State->FinishTimeSeconds = RaceElapsedSeconds;
        OnRaceFinished.Broadcast(Participant, State->FinishTimeSeconds);
    }
    return true;
}

bool UAstrawildRacingSubsystem::ActivateBoostPad(
    AActor* Participant,
    const FGameplayTag& PadTag,
    const FVector ReportedWorldLocation)
{
    if (!HasAuthorityForRacing() || !bRaceActive || !Participant || !PadTag.IsValid())
    {
        return false;
    }

    const FAstrawildRaceTrackDefinition* Track = FindActiveTrack();
    FAstrawildRaceParticipantState* State = ParticipantStates.Find(TWeakObjectPtr<AActor>(Participant));
    if (!Track || !State || State->bFinished)
    {
        return false;
    }

    const FAstrawildRaceBoostPad* MatchingPad = nullptr;
    for (const FAstrawildRaceBoostPad& Pad : Track->BoostPads)
    {
        if (Pad.PadTag == PadTag)
        {
            MatchingPad = &Pad;
            break;
        }
    }
    if (!MatchingPad)
    {
        return false;
    }

    const FVector ActualLocation = Participant->GetActorLocation();
    const float Radius = FMath::Max(1.0f, MatchingPad->Radius);
    if (FVector::DistSquared(ActualLocation, MatchingPad->WorldLocation) > FMath::Square(Radius) ||
        FVector::DistSquared(ReportedWorldLocation, ActualLocation) > FMath::Square(Radius * 2.0f))
    {
        return false;
    }

    State->BoostMultiplier = FMath::Max(1.0f, MatchingPad->SpeedMultiplier);
    State->BoostRemainingSeconds = FMath::Max(0.1f, MatchingPad->DurationSeconds);
    OnBoostPadActivated.Broadcast(Participant, PadTag);
    return true;
}

void UAstrawildRacingSubsystem::AdvanceRace(const float DeltaSeconds)
{
    if (!HasAuthorityForRacing() || !bRaceActive || DeltaSeconds <= 0.0f)
    {
        return;
    }
    RaceElapsedSeconds += FMath::Min(DeltaSeconds, 0.25f);
    for (TPair<TWeakObjectPtr<AActor>, FAstrawildRaceParticipantState>& Pair : ParticipantStates)
    {
        Pair.Value.BoostRemainingSeconds = FMath::Max(0.0f, Pair.Value.BoostRemainingSeconds - DeltaSeconds);
        if (Pair.Value.BoostRemainingSeconds <= 0.0f)
        {
            Pair.Value.BoostMultiplier = 1.0f;
        }
    }
}

bool UAstrawildRacingSubsystem::GetParticipantState(AActor* Participant, FAstrawildRaceParticipantState& OutState) const
{
    OutState = FAstrawildRaceParticipantState();
    if (!Participant)
    {
        return false;
    }
    const FAstrawildRaceParticipantState* FoundState = ParticipantStates.Find(TWeakObjectPtr<AActor>(Participant));
    if (!FoundState)
    {
        return false;
    }
    OutState = *FoundState;
    return true;
}

float UAstrawildRacingSubsystem::GetParticipantSpeedMultiplier(AActor* Participant) const
{
    const FAstrawildRaceParticipantState* State = ParticipantStates.Find(TWeakObjectPtr<AActor>(Participant));
    return State ? FMath::Max(1.0f, State->BoostMultiplier) : 1.0f;
}

bool UAstrawildRacingSubsystem::IsTrackRegistered(const FGameplayTag& TrackTag) const
{
    return TrackDefinitions.Contains(TrackTag);
}

void UAstrawildRacingSubsystem::HandleRaceTick()
{
    AdvanceRace(FMath::Max(0.01f, TickIntervalSeconds));
    RemoveInvalidParticipants();
}

void UAstrawildRacingSubsystem::RemoveInvalidParticipants()
{
    for (auto It = ParticipantStates.CreateIterator(); It; ++It)
    {
        if (!It.Key().IsValid())
        {
            It.RemoveCurrent();
        }
    }
}
