#include "Echoes/AstrawildAlphaEcho.h"

#include "Components/AstrawildAttributeComponent.h"
#include "Components/AstrawildCombatComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

AAstrawildAlphaEcho::AAstrawildAlphaEcho()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AAstrawildAlphaEcho::BeginPlay()
{
    Super::BeginPlay();
    EvaluatePhaseTransition();
}

void AAstrawildAlphaEcho::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bIsEncounterActive)
    {
        EvaluatePhaseTransition();
    }
}

void AAstrawildAlphaEcho::StartEncounter()
{
    bIsEncounterActive = true;
    EvaluatePhaseTransition();
}

void AAstrawildAlphaEcho::StopEncounter()
{
    bIsEncounterActive = false;
}

float AAstrawildAlphaEcho::GetHealthNormalized() const
{
    if (!Attributes || Attributes->MaxHealth <= 0.0f)
    {
        return 0.0f;
    }
    return FMath::Clamp(Attributes->CurrentHealth / Attributes->MaxHealth, 0.0f, 1.0f);
}

const TArray<FAstrawildBossAttackPattern>& AAstrawildAlphaEcho::GetActivePatterns() const
{
    return CurrentBossPhase == EAstrawildBossPhase::PhaseTwo ? PhaseTwoPatterns : PhaseOnePatterns;
}

void AAstrawildAlphaEcho::EvaluatePhaseTransition()
{
    const float HealthNormalized = GetHealthNormalized();
    if (CurrentBossPhase == EAstrawildBossPhase::PhaseOne && HealthNormalized <= PhaseTwoHealthThreshold)
    {
        EnterPhaseTwo();
    }
}

void AAstrawildAlphaEcho::EnterPhaseTwo()
{
    CurrentBossPhase = EAstrawildBossPhase::PhaseTwo;
    PatternCooldownEndTimes.Reset();
    SetEchoState(EAstrawildEchoState::WildHostile);
    OnPhaseChanged.Broadcast(CurrentBossPhase, GetHealthNormalized());
}

bool AAstrawildAlphaEcho::ExecuteAttackPattern(const int32 PatternIndex)
{
    const TArray<FAstrawildBossAttackPattern>& Patterns = GetActivePatterns();
    if (!Patterns.IsValidIndex(PatternIndex) || !GetWorld())
    {
        return false;
    }

    const FAstrawildBossAttackPattern& Pattern = Patterns[PatternIndex];
    const double Now = GetWorld()->GetTimeSeconds();
    const double* CooldownEnd = PatternCooldownEndTimes.Find(Pattern.PatternId);
    if (CooldownEnd && Now < *CooldownEnd)
    {
        return false;
    }

    PatternCooldownEndTimes.FindOrAdd(Pattern.PatternId) = Now + Pattern.Cooldown;
    OnAttackTelegraph.Broadcast(Pattern.PatternId);

    if (Combat)
    {
        Combat->ShowAttackTelegraph(GetActorLocation(), 260.0f, Pattern.TelegraphDuration, FColor::Red);
    }

    APawn* TargetPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (Pattern.SpeciesAbilityIndex != INDEX_NONE && TargetPawn)
    {
        CastAbility(Pattern.SpeciesAbilityIndex, TargetPawn);
    }
    return true;
}
