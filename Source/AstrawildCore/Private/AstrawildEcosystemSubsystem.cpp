#include "AstrawildEcosystemSubsystem.h"

#include "AstrawildEchoCharacter.h"
#include "AstrawildLog.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

UAstrawildEcosystemSubsystem::UAstrawildEcosystemSubsystem()
{
}

bool UAstrawildEcosystemSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UAstrawildEcosystemSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UAstrawildEcosystemSubsystem, STATGROUP_Tickables);
}

void UAstrawildEcosystemSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    UE_LOG(LogAstrawildAI, Log, TEXT("Ecosystem subsystem online (LOD tiers: %.0f / %.0f / %.0f cm)."), Tier0Distance, Tier1Distance, Tier2Distance);
}

void UAstrawildEcosystemSubsystem::RegisterEcho(AAstrawildEchoCharacter* Echo)
{
    if (!IsValid(Echo))
    {
        return;
    }

    if (!RegisteredEchoes.ContainsByPredicate([&Echo](const TWeakObjectPtr<AAstrawildEchoCharacter>& Entry) { return Entry.Get() == Echo; }))
    {
        RegisteredEchoes.Add(Echo);
        EchoTiers.Add(FObjectKey(Echo), EAstrawildSimulationTier::Tier0_Full);
    }

    if (IsValid(Echo->EchoDefinition) && !Echo->EchoDefinition->DefinitionId.IsNone())
    {
        const FName SpeciesId = Echo->EchoDefinition->DefinitionId;
        FAstrawildSpeciesPopulation& Population = Populations.FindOrAdd(SpeciesId);
        Population.DefinitionId = SpeciesId;
        Population.WildCount += 1;
        OnPopulationChanged.Broadcast(SpeciesId, Population.WildCount);
    }
}

void UAstrawildEcosystemSubsystem::UnregisterEcho(AAstrawildEchoCharacter* Echo)
{
    if (!IsValid(Echo))
    {
        return;
    }

    RegisteredEchoes.RemoveAll([&Echo](const TWeakObjectPtr<AAstrawildEchoCharacter>& Entry) { return Entry.Get() == Echo; });
    EchoTiers.Remove(FObjectKey(Echo));
}

void UAstrawildEcosystemSubsystem::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    TierSweepAccumulator += DeltaTime;
    if (TierSweepAccumulator >= TierUpdateIntervalSeconds)
    {
        TierSweepAccumulator = 0.0f;
        RunTierSweep();
    }
}

void UAstrawildEcosystemSubsystem::RunTierSweep()
{
    // Drop stale weak pointers, then assign tiers by nearest-player distance.
    RegisteredEchoes.RemoveAll([](const TWeakObjectPtr<AAstrawildEchoCharacter>& Entry) { return !Entry.IsValid(); });

    for (const TWeakObjectPtr<AAstrawildEchoCharacter>& WeakEcho : RegisteredEchoes)
    {
        AAstrawildEchoCharacter* Echo = WeakEcho.Get();
        if (!IsValid(Echo))
        {
            continue;
        }

        const float Distance = FindNearestPlayerDistance(Echo->GetActorLocation());
        const EAstrawildSimulationTier NewTier = DistanceToTier(Distance, Tier0Distance, Tier1Distance, Tier2Distance);
        EchoTiers.Add(FObjectKey(Echo), NewTier);
    }
}

float UAstrawildEcosystemSubsystem::FindNearestPlayerDistance(const FVector& Location) const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return TNumericLimits<float>::Max();
    }

    float Best = TNumericLimits<float>::Max();
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
    {
        const APlayerController* PC = It->Get();
        if (!PC)
        {
            continue;
        }
        const APawn* Pawn = PC->GetPawn();
        if (!Pawn)
        {
            continue;
        }
        Best = FMath::Min(Best, static_cast<float>(FVector::Dist(Location, Pawn->GetActorLocation())));
    }
    return Best;
}

EAstrawildSimulationTier UAstrawildEcosystemSubsystem::DistanceToTier(const float Distance, const float Tier0, const float Tier1, const float Tier2)
{
    if (Distance <= Tier0)
    {
        return EAstrawildSimulationTier::Tier0_Full;
    }
    if (Distance <= Tier1)
    {
        return EAstrawildSimulationTier::Tier1_Reduced;
    }
    if (Distance <= Tier2)
    {
        return EAstrawildSimulationTier::Tier2_Statistical;
    }
    return EAstrawildSimulationTier::Tier3_World;
}

EAstrawildSimulationTier UAstrawildEcosystemSubsystem::GetTierForEcho(const AAstrawildEchoCharacter* Echo) const
{
    if (!IsValid(Echo))
    {
        return EAstrawildSimulationTier::Tier3_World;
    }

    if (const EAstrawildSimulationTier* Tier = EchoTiers.Find(FObjectKey(Echo)))
    {
        return *Tier;
    }
    return EAstrawildSimulationTier::Tier0_Full;
}

float UAstrawildEcosystemSubsystem::GetRecommendedUpdateInterval(const EAstrawildSimulationTier Tier)
{
    switch (Tier)
    {
    case EAstrawildSimulationTier::Tier0_Full:
        return 0.0f;  // Every frame — AI owns its own tick budget.
    case EAstrawildSimulationTier::Tier1_Reduced:
        return 0.25f; // 4 Hz.
    case EAstrawildSimulationTier::Tier2_Statistical:
        return 1.0f;  // 1 Hz, movement disabled.
    case EAstrawildSimulationTier::Tier3_World:
        return 5.0f;  // Slow statistical bookkeeping.
    default:
        return 1.0f;
    }
}

int32 UAstrawildEcosystemSubsystem::GetWildPopulation(const FName DefinitionId) const
{
    if (const FAstrawildSpeciesPopulation* Population = Populations.Find(DefinitionId))
    {
        return Population->WildCount;
    }
    return 0;
}

TArray<FAstrawildSpeciesPopulation> UAstrawildEcosystemSubsystem::GetPopulations() const
{
    TArray<FAstrawildSpeciesPopulation> Out;
    Populations.GenerateValueArray(Out);
    return Out;
}

void UAstrawildEcosystemSubsystem::NotifyCaptured(const FName DefinitionId)
{
    FAstrawildSpeciesPopulation& Population = Populations.FindOrAdd(DefinitionId);
    Population.DefinitionId = DefinitionId;
    Population.WildCount = FMath::Max(0, Population.WildCount - 1);
    Population.CapturedCount += 1;
    OnPopulationChanged.Broadcast(DefinitionId, Population.WildCount);
}

void UAstrawildEcosystemSubsystem::NotifyDefeated(const FName DefinitionId)
{
    FAstrawildSpeciesPopulation& Population = Populations.FindOrAdd(DefinitionId);
    Population.DefinitionId = DefinitionId;
    Population.WildCount = FMath::Max(0, Population.WildCount - 1);
    Population.DefeatedCount += 1;
    OnPopulationChanged.Broadcast(DefinitionId, Population.WildCount);
}
