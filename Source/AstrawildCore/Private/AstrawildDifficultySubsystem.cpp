#include "AstrawildDifficultySubsystem.h"

#include "AstrawildEventBusSubsystem.h"
#include "AstrawildGameplayTags.h"
#include "AstrawildLog.h"
#include "Engine/World.h"

UAstrawildDifficultySubsystem::UAstrawildDifficultySubsystem()
{
}

TStatId UAstrawildDifficultySubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UAstrawildDifficultySubsystem, STATGROUP_Tickables);
}

bool UAstrawildDifficultySubsystem::IsTickable() const
{
    const UWorld* World = GetWorld();
    return World && World->IsGameWorld() && World->GetNetMode() != NM_Client;
}

int32 UAstrawildDifficultySubsystem::ComputeSkillBand(int32 HostileDefeats, int32 Captures, int32 Deaths, int32 PartyLosses)
{
    // Weighted skill metric: a capture demonstrates the full loop (weaken,
    // feed, throw) so it counts double; deaths cost triple because they
    // represent a full reset of pressure.
    // FCR-1-d (M-d8): party losses pull the metric down at half the weight of a
    // player death (pressure without a full reset).
    const int32 Metric = HostileDefeats + (Captures * 2) - (Deaths * 3) - (PartyLosses * 2);

    // Hysteresis band: [-1..+1] stays Standard so the difficulty does not
    // oscillate on single events.
    if (Metric <= -2)
    {
        return 0; // Struggling — the game leans in to help.
    }
    if (Metric >= 2)
    {
        return 2; // Thriving — the world pushes back.
    }
    return 1;
}

float UAstrawildDifficultySubsystem::GetHostileStrengthMultiplier(int32 Band)
{
    switch (Band)
    {
    case 0:
        return 0.85f;
    case 2:
        return 1.15f;
    default:
        return 1.0f;
    }
}

float UAstrawildDifficultySubsystem::GetResourceYieldMultiplier(int32 Band)
{
    switch (Band)
    {
    case 0:
        return 1.15f;
    case 2:
        return 0.9f;
    default:
        return 1.0f;
    }
}

void UAstrawildDifficultySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    // DDA listens to the world's own event bus — no per-caller wiring needed.
    if (UAstrawildEventBusSubsystem* EventBus = InWorld.GetSubsystem<UAstrawildEventBusSubsystem>())
    {
        EventBus->OnGameplayEvent.AddUniqueDynamic(this, &UAstrawildDifficultySubsystem::HandleGameplayEvent);
    }
}

void UAstrawildDifficultySubsystem::HandleGameplayEvent(const FAstrawildGameplayEvent& Event)
{
    if (Event.EventTag == TAG_Astrawild_Event_HostileDefeated)
    {
        NotifyHostileDefeated();
    }
    else if (Event.EventTag == TAG_Astrawild_Event_EchoCaptured)
    {
        NotifyCapture();
    }
    else if (Event.EventTag == TAG_Astrawild_Event_PartyEchoDefeated)
    {
        // FCR-1-d fix (M-d8): a captured echo dying is PRESSURE — the world leans
        // in to help (metric down), not a skill demonstration pushing it up.
        NotifyPartyLoss();
    }
    else if (Event.EventTag == TAG_Astrawild_Event_EchoDefeated)
    {
        // Wild (uncaptured) echo defeats remain a skill signal — hunting is play.
        NotifyHostileDefeated();
    }
}

void UAstrawildDifficultySubsystem::Tick(float DeltaTime)
{
    if (!IsTickable())
    {
        return;
    }

    RefreshAccumulator += DeltaTime;
    if (RefreshAccumulator < BandRefreshSeconds)
    {
        return;
    }
    RefreshAccumulator = 0.0f;

    RefreshBand();
}

void UAstrawildDifficultySubsystem::RefreshBand()
{
    const int32 NewBand = ComputeSkillBand(HostileDefeatCount, CaptureCount, DeathCount, PartyLossCount);
    if (NewBand != SkillBand)
    {
        const TCHAR* BandNames[] = { TEXT("Struggling"), TEXT("Standard"), TEXT("Thriving") };
        const int32 NameIndex = FMath::Clamp(NewBand, 0, 2);
        UE_LOG(LogAstrawildAI, Log, TEXT("DDA: band %d -> %d (%s) — hostile x%.2f, resources x%.2f"),
            SkillBand, NewBand, BandNames[NameIndex],
            GetHostileStrengthMultiplier(NewBand), GetResourceYieldMultiplier(NewBand));
        SkillBand = NewBand;
    }
}
