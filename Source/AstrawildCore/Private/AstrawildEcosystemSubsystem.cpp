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
    BuildDefaultFoodChain();
    UE_LOG(LogAstrawildAI, Log, TEXT("Ecosystem subsystem online (LOD tiers: %.0f / %.0f / %.0f cm, %d predator chains, %d herding species)."),
        Tier0Distance, Tier1Distance, Tier2Distance, PredatorChains.Num(), HerdingSpecies.Num());
}

void UAstrawildEcosystemSubsystem::BuildDefaultFoodChain()
{
    // Dawn Fields food chain (directive §7/§21): Gloomfangs hunt the gentle species.
    AddPredatorPair(TEXT("Echo_Gloomfang"), TEXT("Echo_Lumewisp"));
    AddPredatorPair(TEXT("Echo_Gloomfang"), TEXT("Echo_Duskmoth"));
    AddPredatorPair(TEXT("Echo_Gloomfang"), TEXT("Echo_Sprigling"));

    // Ember predator (content wave 2): crepuscular stalker of Sprigling herds and Voltlings.
    AddPredatorPair(TEXT("Echo_Emberfang"), TEXT("Echo_Sprigling"));
    AddPredatorPair(TEXT("Echo_Emberfang"), TEXT("Echo_Voltling"));

    // Herding species — Social personalities anchor them together (directive §7).
    MarkHerdingSpecies(TEXT("Echo_Lumewisp"));
    MarkHerdingSpecies(TEXT("Echo_Sprigling"));
}

void UAstrawildEcosystemSubsystem::AddPredatorPair(const FName PredatorId, const FName PreyId)
{
    if (PredatorId.IsNone() || PreyId.IsNone() || PredatorId == PreyId)
    {
        return;
    }

    TArray<FName>& Chain = PredatorChains.FindOrAdd(PredatorId);
    if (!Chain.Contains(PreyId))
    {
        Chain.Add(PreyId);
    }
}

bool UAstrawildEcosystemSubsystem::IsPredator(const FName SpeciesId) const
{
    return PredatorChains.Contains(SpeciesId);
}

bool UAstrawildEcosystemSubsystem::IsPreyOf(const FName PredatorId, const FName PreyId) const
{
    const TArray<FName>* Chain = PredatorChains.Find(PredatorId);
    return Chain && Chain->Contains(PreyId);
}

TArray<FName> UAstrawildEcosystemSubsystem::GetPreySpecies(const FName PredatorId) const
{
    if (const TArray<FName>* Chain = PredatorChains.Find(PredatorId))
    {
        return *Chain;
    }
    return TArray<FName>();
}

void UAstrawildEcosystemSubsystem::MarkHerdingSpecies(const FName SpeciesId)
{
    if (!SpeciesId.IsNone())
    {
        HerdingSpecies.Add(SpeciesId);
    }
}

bool UAstrawildEcosystemSubsystem::IsHerdingSpecies(const FName SpeciesId) const
{
    return HerdingSpecies.Contains(SpeciesId);
}

AAstrawildEchoCharacter* UAstrawildEcosystemSubsystem::FindHerdAnchor(const AAstrawildEchoCharacter* Echo, const float MaxDistance) const
{
    if (!IsValid(Echo) || !IsValid(Echo->EchoDefinition))
    {
        return nullptr;
    }

    const FName SpeciesId = Echo->EchoDefinition->DefinitionId;
    if (!IsHerdingSpecies(SpeciesId))
    {
        return nullptr;
    }

    AAstrawildEchoCharacter* Best = nullptr;
    float BestDistance = MaxDistance;
    for (const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak : RegisteredEchoes)
    {
        AAstrawildEchoCharacter* Other = Weak.Get();
        if (!Other || Other == Echo || Other->bCaptured || Other->IsDefeated())
        {
            continue;
        }
        if (!IsValid(Other->EchoDefinition) || Other->EchoDefinition->DefinitionId != SpeciesId)
        {
            continue;
        }
        const float Distance = FVector::Dist(Echo->GetActorLocation(), Other->GetActorLocation());
        if (Distance > 100.0f && Distance < BestDistance) // Not stacked on itself.
        {
            BestDistance = Distance;
            Best = Other;
        }
    }
    return Best;
}

AAstrawildEchoCharacter* UAstrawildEcosystemSubsystem::FindPreyFor(const AAstrawildEchoCharacter* Predator, const float MaxDistance) const
{
    if (!IsValid(Predator) || !IsValid(Predator->EchoDefinition))
    {
        return nullptr;
    }

    const FName PredatorId = Predator->EchoDefinition->DefinitionId;
    const TArray<FName> PreyIds = GetPreySpecies(PredatorId);
    if (PreyIds.IsEmpty())
    {
        return nullptr;
    }

    AAstrawildEchoCharacter* Best = nullptr;
    float BestDistance = MaxDistance;
    for (const TWeakObjectPtr<AAstrawildEchoCharacter>& Weak : RegisteredEchoes)
    {
        AAstrawildEchoCharacter* Other = Weak.Get();
        if (!Other || Other == Predator || Other->bCaptured || Other->IsDefeated())
        {
            continue;
        }
        if (!IsValid(Other->EchoDefinition) || !PreyIds.Contains(Other->EchoDefinition->DefinitionId))
        {
            continue;
        }
        const float Distance = FVector::Dist(Predator->GetActorLocation(), Other->GetActorLocation());
        if (Distance < BestDistance)
        {
            BestDistance = Distance;
            Best = Other;
        }
    }
    return Best;
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
