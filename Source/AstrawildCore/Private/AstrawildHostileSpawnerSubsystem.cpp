#include "AstrawildHostileSpawnerSubsystem.h"

#include "AstrawildCore.h"
#include "AstrawildDataAssets.h"
#include "AstrawildEchoCharacter.h"
#include "AstrawildEcosystemSubsystem.h"
#include "AstrawildGameState.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildPlayerCharacter.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

UAstrawildHostileSpawnerSubsystem::UAstrawildHostileSpawnerSubsystem()
{
}

bool UAstrawildHostileSpawnerSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UAstrawildHostileSpawnerSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UAstrawildHostileSpawnerSubsystem, STATGROUP_Tickables);
}

void UAstrawildHostileSpawnerSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    // Seed the spawn stream from the replicated world seed (deterministic across clients in SP).
    if (const AAstrawildGameState* GS = InWorld.GetGameState<AAstrawildGameState>())
    {
        SpawnStream.Initialize(GS->WorldSeed);
    }
    else
    {
        SpawnStream.Initialize(FMath::Rand());
    }

    UE_LOG(LogAstrawildBuilding, Log,
        TEXT("Hostile spawner online (gloomfang target=%d, emberfang target=%d, every %.1fs)."),
        TargetGloomfangPopulation, TargetEmberfangPopulation, RespawnIntervalSeconds);
}

void UAstrawildHostileSpawnerSubsystem::Tick(const float DeltaTime)
{
    Super::Tick(DeltaTime);

    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    RespawnAccumulator += DeltaTime;
    if (RespawnAccumulator < RespawnIntervalSeconds)
    {
        return;
    }
    RespawnAccumulator = 0.0f;

    AAstrawildPlayerCharacter* Player = FindLocalPlayer();
    if (!Player)
    {
        return;
    }

    UAstrawildItemRegistrySubsystem* Registry = World->GetSubsystem<UAstrawildItemRegistrySubsystem>();
    UAstrawildEcosystemSubsystem* Ecosystem = World->GetSubsystem<UAstrawildEcosystemSubsystem>();
    if (!Registry || !Ecosystem)
    {
        return;
    }

    const FVector Origin = Player->GetActorLocation();

    // Refill each tracked species up to its target population.
    UAstrawildEchoDefinition* GloomfangDef = Registry->FindEcho(GloomfangId);
    if (GloomfangDef)
    {
        const int32 Current = Ecosystem->GetWildPopulation(GloomfangId);
        const int32 Deficit = TargetGloomfangPopulation - Current;
        for (int32 i = 0; i < FMath::Max(0, Deficit); ++i)
        {
            SpawnOneHostile(GloomfangDef, Origin);
        }
    }

    UAstrawildEchoDefinition* EmberfangDef = Registry->FindEcho(EmberfangId);
    if (EmberfangDef)
    {
        const int32 Current = Ecosystem->GetWildPopulation(EmberfangId);
        const int32 Deficit = TargetEmberfangPopulation - Current;
        for (int32 i = 0; i < FMath::Max(0, Deficit); ++i)
        {
            SpawnOneHostile(EmberfangDef, Origin);
        }
    }
}

void UAstrawildHostileSpawnerSubsystem::SpawnOneHostile(UAstrawildEchoDefinition* Definition, const FVector& Origin)
{
    if (!Definition)
    {
        return;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    // Random ring placement inside SpawnRadius, biased outward (minimum 30% radius so
    // hostiles never spawn directly on top of the player).
    const float Angle = SpawnStream.FRandRange(0.0f, 2.0f * PI);
    const float MinRadius = SpawnRadius * 0.3f;
    const float Radius = SpawnStream.FRandRange(MinRadius, SpawnRadius);
    const FVector Location(
        Origin.X + FMath::Cos(Angle) * Radius,
        Origin.Y + FMath::Sin(Angle) * Radius,
        Origin.Z + 150.0f);

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    AAstrawildEchoCharacter* Echo = World->SpawnActor<AAstrawildEchoCharacter>(
        AAstrawildEchoCharacter::StaticClass(), Location, FRotator::ZeroRotator, Params);
    if (Echo)
    {
        Echo->InitializeFromDefinition(Definition);
        // REVIEW-2 medium-risk fix: RegisterWithEcosystem ran in BeginPlay BEFORE
        // EchoDefinition was set, so the WildCount bump at EcosystemSubsystem::RegisterEcho
        // was skipped. Re-register now that the definition is populated, so the next
        // Tick's GetWildPopulation sees this hostile and the population clamp works.
        if (UAstrawildEcosystemSubsystem* Eco = World->GetSubsystem<UAstrawildEcosystemSubsystem>())
        {
            Eco->RegisterEcho(Echo);
        }
        UE_LOG(LogAstrawildBuilding, Verbose,
            TEXT("Hostile spawner: spawned %s near player at %s."),
            *Definition->DefinitionId.ToString(), *Location.ToCompactString());
    }
}

AAstrawildPlayerCharacter* UAstrawildHostileSpawnerSubsystem::FindLocalPlayer() const
{
    const UWorld* World = GetWorld();
    if (!World)
    {
        return nullptr;
    }
    if (APlayerController* PC = World->GetFirstPlayerController())
    {
        return Cast<AAstrawildPlayerCharacter>(PC->GetPawn());
    }
    return nullptr;
}
