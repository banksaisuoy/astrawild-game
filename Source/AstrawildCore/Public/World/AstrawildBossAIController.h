#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Data/AstrawildBossData.h"
#include "AstrawildBossAIController.generated.h"

class AActor;
class AAstrawildEchoBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAstrawildBossStateChangedSignature, EAstrawildBossControllerState, NewState, float, HealthNormalized);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnAstrawildBossTelegraphSignature, FName, AttackId, float, DurationSeconds, float, Radius);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAstrawildBossEncounterEndedSignature, bool, bVictory);

UCLASS()
class ASTRAWILDCORE_API AAstrawildBossAIController : public AAIController
{
    GENERATED_BODY()

public:
    AAstrawildBossAIController();

    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void Tick(float DeltaSeconds) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Boss")
    TObjectPtr<UDataTable> BossEncounterTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Boss")
    TObjectPtr<UDataTable> BossAttackTable;

    UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category="ASTRAWILD|Boss|Arena")
    TArray<TObjectPtr<AActor>> ArenaBarrierActors;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Boss")
    EAstrawildBossControllerState CurrentState = EAstrawildBossControllerState::Idle;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Boss")
    FName ActiveEncounterId = NAME_None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Boss")
    float EncounterElapsedSeconds = 0.0f;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Boss|Events")
    FOnAstrawildBossStateChangedSignature OnBossStateChanged;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Boss|Events")
    FOnAstrawildBossTelegraphSignature OnAttackTelegraph;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Boss|Events")
    FOnAstrawildBossEncounterEndedSignature OnEncounterEnded;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Boss")
    bool StartEncounter(FName EncounterId);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Boss")
    void StopEncounter(bool bVictory);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Boss")
    bool TriggerAttack(FName AttackId);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Boss")
    void SetArenaLocked(bool bLocked);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Boss")
    bool IsEncounterActive() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Boss")
    float GetEncounterTimeRemaining() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Boss")
    AAstrawildEchoBase* GetControlledBoss() const;

protected:
    void EnterState(EAstrawildBossControllerState NewState);
    void UpdateEncounter(float DeltaSeconds);
    void ResolveTelegraph();
    void SelectAndTelegraphAttack(int32 PhaseIndex);

private:
    TWeakObjectPtr<AAstrawildEchoBase> ControlledBoss;
    const FAstrawildBossEncounterRow* FindEncounterRow(FName EncounterId) const;
    const FAstrawildBossAttackRow* FindAttackRow(FName AttackId) const;
    const FAstrawildBossAttackRow* FindNextAttackForPhase(int32 PhaseIndex) const;
    double EncounterStartTimeSeconds = 0.0;
    double NextAttackTimeSeconds = 0.0;
    double TelegraphEndTimeSeconds = 0.0;
    FName PendingAttackId = NAME_None;
    int32 LastAudioPhaseIndex = 0;
};
