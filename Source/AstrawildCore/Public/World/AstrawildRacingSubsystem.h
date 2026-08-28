#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/AstrawildRacingData.h"
#include "AstrawildRacingSubsystem.generated.h"

class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAstrawildRaceStartedSignature, FGameplayTag, TrackTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAstrawildRaceFinishedSignature, AActor*, Participant, float, FinishTimeSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAstrawildCheckpointValidatedSignature, AActor*, Participant, int32, CheckpointIndex, int32, CompletedLaps);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAstrawildBoostPadActivatedSignature, AActor*, Participant, FGameplayTag, PadTag);

UCLASS()
class ASTRAWILDCORE_API UAstrawildRacingSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UAstrawildRacingSubsystem();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Racing|Rules", meta=(ClampMin="0.01"))
    float TickIntervalSeconds = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Racing|Rules", meta=(ClampMin="0.1"))
    float DefaultCheckpointRadius = 400.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Racing|State")
    bool bRaceActive = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Racing|State")
    FGameplayTag ActiveTrackTag;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Racing|State")
    float RaceElapsedSeconds = 0.0f;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Racing|Events")
    FOnAstrawildRaceStartedSignature OnRaceStarted;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Racing|Events")
    FOnAstrawildRaceFinishedSignature OnRaceFinished;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Racing|Events")
    FOnAstrawildCheckpointValidatedSignature OnCheckpointValidated;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Racing|Events")
    FOnAstrawildBoostPadActivatedSignature OnBoostPadActivated;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Racing|Authority")
    bool RegisterTrack(const FAstrawildRaceTrackDefinition& TrackDefinition);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Racing|Authority")
    bool StartRace(const FGameplayTag& TrackTag);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Racing|Authority")
    void EndRace();

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Racing|Authority")
    bool RegisterParticipant(AActor* Participant);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Racing|Authority")
    bool UnregisterParticipant(AActor* Participant);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Racing|Authority")
    bool SubmitCheckpoint(AActor* Participant, int32 CheckpointIndex, FVector ReportedWorldLocation);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Racing|Authority")
    bool ActivateBoostPad(AActor* Participant, const FGameplayTag& PadTag, FVector ReportedWorldLocation);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Racing|Authority")
    void AdvanceRace(float DeltaSeconds);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Racing")
    bool GetParticipantState(AActor* Participant, FAstrawildRaceParticipantState& OutState) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Racing")
    float GetParticipantSpeedMultiplier(AActor* Participant) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Racing")
    bool IsTrackRegistered(const FGameplayTag& TrackTag) const;

private:
    TMap<FGameplayTag, FAstrawildRaceTrackDefinition> TrackDefinitions;
    TMap<TWeakObjectPtr<AActor>, FAstrawildRaceParticipantState> ParticipantStates;
    FTimerHandle RaceTimerHandle;

    bool HasAuthorityForRacing() const;
    const FAstrawildRaceTrackDefinition* FindActiveTrack() const;
    void HandleRaceTick();
    void RemoveInvalidParticipants();
};
