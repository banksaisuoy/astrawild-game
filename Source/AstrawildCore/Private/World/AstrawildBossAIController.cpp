#include "World/AstrawildBossAIController.h"

#include "Components/AstrawildAttributeComponent.h"
#include "Components/AstrawildCombatComponent.h"
#include "Echoes/AstrawildEchoBase.h"
#include "Echoes/AstrawildAlphaEcho.h"
#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "World/AstrawildAudioSubsystem.h"

AAstrawildBossAIController::AAstrawildBossAIController()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AAstrawildBossAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    ControlledBoss = Cast<AAstrawildEchoBase>(InPawn);
}

void AAstrawildBossAIController::OnUnPossess()
{
    ControlledBoss.Reset();
    Super::OnUnPossess();
}

void AAstrawildBossAIController::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (IsEncounterActive())
    {
        UpdateEncounter(DeltaSeconds);
    }
}

bool AAstrawildBossAIController::StartEncounter(const FName EncounterId)
{
    if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
    {
        return false;
    }
    if (!ControlledBoss.IsValid() || IsEncounterActive())
    {
        return false;
    }
    const FAstrawildBossEncounterRow* Encounter = FindEncounterRow(EncounterId);
    if (!Encounter || !GetWorld())
    {
        return false;
    }

    ActiveEncounterId = EncounterId;
    EncounterStartTimeSeconds = GetWorld()->GetTimeSeconds();
    EncounterElapsedSeconds = 0.0f;
    NextAttackTimeSeconds = Encounter->IntroDurationSeconds;
    TelegraphEndTimeSeconds = 0.0;
    PendingAttackId = NAME_None;
    LastAudioPhaseIndex = 1;
    SetArenaLocked(Encounter->bLockArena);
    if (AAstrawildAlphaEcho* AlphaBoss = Cast<AAstrawildAlphaEcho>(ControlledBoss.Get()))
    {
        AlphaBoss->StartEncounter();
    }
    ControlledBoss->SetEchoState(EAstrawildEchoState::WildHostile);
    if (UAstrawildAudioSubsystem* AudioSubsystem = GetWorld()->GetSubsystem<UAstrawildAudioSubsystem>())
    {
        AudioSubsystem->EnterBossCombat(ActiveEncounterId, 1, false);
    }
    EnterState(Encounter->IntroDurationSeconds > 0.0f ? EAstrawildBossControllerState::IntroCutscene : EAstrawildBossControllerState::PhaseOneCombat);
    return true;
}

void AAstrawildBossAIController::StopEncounter(const bool bVictory)
{
    if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
    {
        return;
    }
    if (!IsEncounterActive() && CurrentState != EAstrawildBossControllerState::IntroCutscene)
    {
        return;
    }
    SetArenaLocked(false);
    if (GetWorld())
    {
        if (UAstrawildAudioSubsystem* AudioSubsystem = GetWorld()->GetSubsystem<UAstrawildAudioSubsystem>())
        {
            AudioSubsystem->ExitCombat();
        }
    }
    if (ControlledBoss.IsValid())
    {
        if (AAstrawildAlphaEcho* AlphaBoss = Cast<AAstrawildAlphaEcho>(ControlledBoss.Get()))
        {
            AlphaBoss->StopEncounter();
        }
        if (!bVictory)
        {
            ControlledBoss->SetEchoState(EAstrawildEchoState::WildPassive);
        }
    }
    EnterState(bVictory ? EAstrawildBossControllerState::DefeatedLoot : EAstrawildBossControllerState::Failed);
    PendingAttackId = NAME_None;
    ActiveEncounterId = NAME_None;
    EncounterStartTimeSeconds = 0.0;
    TelegraphEndTimeSeconds = 0.0;
    NextAttackTimeSeconds = 0.0;
    LastAudioPhaseIndex = 0;
    OnEncounterEnded.Broadcast(bVictory);
}

bool AAstrawildBossAIController::TriggerAttack(const FName AttackId)
{
    if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
    {
        return false;
    }
    if (!IsEncounterActive() || !ControlledBoss.IsValid())
    {
        return false;
    }
    const FAstrawildBossAttackRow* Attack = FindAttackRow(AttackId);
    if (!Attack || Attack->EncounterId != ActiveEncounterId || Attack->PhaseIndex < 1)
    {
        return false;
    }

    const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    if (PendingAttackId != NAME_None || Now < NextAttackTimeSeconds)
    {
        return false;
    }
    PendingAttackId = AttackId;
    TelegraphEndTimeSeconds = Now + FMath::Max(0.0f, Attack->TelegraphDurationSeconds);
    EnterState(Attack->bIsUltimate ? EAstrawildBossControllerState::SupermoveTelegraph : (Attack->PhaseIndex >= 3 ? EAstrawildBossControllerState::PhaseThreeDawn : (Attack->PhaseIndex == 2 ? EAstrawildBossControllerState::PhaseTwoEnrage : EAstrawildBossControllerState::PhaseOneCombat)));
    OnAttackTelegraph.Broadcast(Attack->AttackId, Attack->TelegraphDurationSeconds, Attack->TelegraphRadius);
    if (Attack->bIsUltimate)
    {
        if (UAstrawildAudioSubsystem* AudioSubsystem = GetWorld()->GetSubsystem<UAstrawildAudioSubsystem>())
        {
            AudioSubsystem->EnterBossCombat(ActiveEncounterId, Attack->PhaseIndex, true);
        }
    }
    if (UAstrawildCombatComponent* Combat = ControlledBoss->Combat)
    {
        Combat->ShowAttackTelegraph(ControlledBoss->GetActorLocation(), Attack->TelegraphRadius, Attack->TelegraphDurationSeconds, Attack->bIsUltimate ? FColor::Yellow : FColor::Red);
    }
    return true;
}

void AAstrawildBossAIController::SetArenaLocked(const bool bLocked)
{
    for (AActor* Barrier : ArenaBarrierActors)
    {
        if (Barrier)
        {
            Barrier->SetActorEnableCollision(bLocked);
            Barrier->SetActorHiddenInGame(!bLocked);
        }
    }
}

bool AAstrawildBossAIController::IsEncounterActive() const
{
    return !ActiveEncounterId.IsNone() && CurrentState != EAstrawildBossControllerState::DefeatedLoot && CurrentState != EAstrawildBossControllerState::Failed;
}

float AAstrawildBossAIController::GetEncounterTimeRemaining() const
{
    const FAstrawildBossEncounterRow* Encounter = FindEncounterRow(ActiveEncounterId);
    if (!Encounter || !GetWorld() || !IsEncounterActive())
    {
        return 0.0f;
    }
    return FMath::Max(0.0f, Encounter->EncounterTimeLimitSeconds - EncounterElapsedSeconds);
}

AAstrawildEchoBase* AAstrawildBossAIController::GetControlledBoss() const
{
    return ControlledBoss.Get();
}

void AAstrawildBossAIController::EnterState(const EAstrawildBossControllerState NewState)
{
    if (CurrentState == NewState)
    {
        return;
    }
    CurrentState = NewState;
    const float HealthNormalized = ControlledBoss.IsValid() ? ControlledBoss->GetHealthNormalized() : 0.0f;
    OnBossStateChanged.Broadcast(CurrentState, HealthNormalized);
}

void AAstrawildBossAIController::UpdateEncounter(const float DeltaSeconds)
{
    if (!GetWorld() || !ControlledBoss.IsValid())
    {
        StopEncounter(false);
        return;
    }
    const FAstrawildBossEncounterRow* Encounter = FindEncounterRow(ActiveEncounterId);
    if (!Encounter)
    {
        StopEncounter(false);
        return;
    }
    EncounterElapsedSeconds = FMath::Max(0.0f, static_cast<float>(GetWorld()->GetTimeSeconds() - EncounterStartTimeSeconds));
    if (EncounterElapsedSeconds >= Encounter->EncounterTimeLimitSeconds)
    {
        StopEncounter(false);
        return;
    }
    if (ControlledBoss->GetHealthNormalized() <= 0.0f)
    {
        StopEncounter(true);
        return;
    }

    if (CurrentState == EAstrawildBossControllerState::IntroCutscene && EncounterElapsedSeconds >= Encounter->IntroDurationSeconds)
    {
        EnterState(EAstrawildBossControllerState::PhaseOneCombat);
    }
    const float Health = ControlledBoss->GetHealthNormalized();
    const bool bPhaseThree = Encounter->PhaseCount >= 3 && Health <= Encounter->PhaseThreeHealthThreshold;
    const bool bPhaseTwo = Health <= Encounter->PhaseTwoHealthThreshold;
    const int32 PhaseIndex = bPhaseThree ? 3 : (bPhaseTwo ? 2 : 1);
    if (PhaseIndex != LastAudioPhaseIndex)
    {
        LastAudioPhaseIndex = PhaseIndex;
        if (UAstrawildAudioSubsystem* AudioSubsystem = GetWorld()->GetSubsystem<UAstrawildAudioSubsystem>())
        {
            AudioSubsystem->EnterBossCombat(ActiveEncounterId, PhaseIndex, false);
        }
    }
    if (PendingAttackId != NAME_None && GetWorld()->GetTimeSeconds() >= TelegraphEndTimeSeconds)
    {
        ResolveTelegraph();
    }
    if (PendingAttackId == NAME_None && GetWorld()->GetTimeSeconds() >= NextAttackTimeSeconds && CurrentState != EAstrawildBossControllerState::IntroCutscene)
    {
        SelectAndTelegraphAttack(PhaseIndex);
    }
    (void)DeltaSeconds;
}

void AAstrawildBossAIController::ResolveTelegraph()
{
    const FAstrawildBossAttackRow* Attack = FindAttackRow(PendingAttackId);
    if (Attack && ControlledBoss.IsValid() && GetWorld())
    {
        APawn* Target = nullptr;
        float BestDistanceSquared = TNumericLimits<float>::Max();
        for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
        {
            APlayerController* PlayerController = Iterator->Get();
            APawn* Candidate = PlayerController ? PlayerController->GetPawn() : nullptr;
            if (!Candidate)
            {
                continue;
            }
            if (const UAstrawildAttributeComponent* CandidateAttributes = Candidate->FindComponentByClass<UAstrawildAttributeComponent>())
            {
                if (!CandidateAttributes->IsAlive())
                {
                    continue;
                }
            }
            const float DistanceSquared = FVector::DistSquared(ControlledBoss->GetActorLocation(), Candidate->GetActorLocation());
            if (DistanceSquared < BestDistanceSquared)
            {
                BestDistanceSquared = DistanceSquared;
                Target = Candidate;
            }
        }
        if (Target)
        {
            ControlledBoss->CastAbility(Attack->SpeciesAbilityIndex, Target);
        }
        NextAttackTimeSeconds = GetWorld()->GetTimeSeconds() + FMath::Max(0.1f, Attack->CooldownSeconds);
        const EAstrawildBossControllerState NextState = Attack->PhaseIndex >= 3 ? EAstrawildBossControllerState::PhaseThreeDawn : (Attack->PhaseIndex == 2 ? EAstrawildBossControllerState::PhaseTwoEnrage : EAstrawildBossControllerState::PhaseOneCombat);
        PendingAttackId = NAME_None;
        EnterState(NextState);
        return;
    }
    PendingAttackId = NAME_None;
    EnterState(EAstrawildBossControllerState::PhaseOneCombat);
}

void AAstrawildBossAIController::SelectAndTelegraphAttack(const int32 PhaseIndex)
{
    const FAstrawildBossAttackRow* Attack = FindNextAttackForPhase(PhaseIndex);
    if (Attack)
    {
        TriggerAttack(Attack->AttackId);
    }
}

const FAstrawildBossEncounterRow* AAstrawildBossAIController::FindEncounterRow(const FName EncounterId) const
{
    if (!BossEncounterTable || EncounterId.IsNone())
    {
        return nullptr;
    }
    for (const TPair<FName, uint8*>& Pair : BossEncounterTable->GetRowMap())
    {
        const FAstrawildBossEncounterRow* Row = reinterpret_cast<const FAstrawildBossEncounterRow*>(Pair.Value);
        if (Row && Row->EncounterId == EncounterId)
        {
            return Row;
        }
    }
    return nullptr;
}

const FAstrawildBossAttackRow* AAstrawildBossAIController::FindAttackRow(const FName AttackId) const
{
    if (!BossAttackTable || AttackId.IsNone())
    {
        return nullptr;
    }
    for (const TPair<FName, uint8*>& Pair : BossAttackTable->GetRowMap())
    {
        const FAstrawildBossAttackRow* Row = reinterpret_cast<const FAstrawildBossAttackRow*>(Pair.Value);
        if (Row && Row->AttackId == AttackId)
        {
            return Row;
        }
    }
    return nullptr;
}

const FAstrawildBossAttackRow* AAstrawildBossAIController::FindNextAttackForPhase(const int32 PhaseIndex) const
{
    if (!BossAttackTable || ActiveEncounterId.IsNone())
    {
        return nullptr;
    }
    for (const TPair<FName, uint8*>& Pair : BossAttackTable->GetRowMap())
    {
        const FAstrawildBossAttackRow* Row = reinterpret_cast<const FAstrawildBossAttackRow*>(Pair.Value);
        if (Row && Row->EncounterId == ActiveEncounterId && Row->PhaseIndex == PhaseIndex)
        {
            return Row;
        }
    }
    return nullptr;
}
