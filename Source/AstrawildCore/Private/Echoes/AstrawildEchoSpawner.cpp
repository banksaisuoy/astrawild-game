// Copyright Epic Games, Inc. All Rights Reserved.

#include "Echoes/AstrawildEchoSpawner.h"
#include "Echoes/AstrawildEchoBase.h"
#include "Data/AstrawildEchoDataAsset.h"
#include "Components/AstrawildAttributeComponent.h"
#include "AstrawildLogChannels.h"
#include "Engine/World.h"
#include "NavigationSystem.h"

AAstrawildEchoSpawner::AAstrawildEchoSpawner()
	: MaxConcurrentEchoes(3)
	, SpawnRadius(1500.0f)
	, RespawnIntervalSeconds(15.0f)
	, MinSpawnLevel(1)
	, MaxSpawnLevel(5)
	, RespawnTimer(0.0f)
{
	PrimaryActorTick.bCanEverTick = true;
	USceneComponent* SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;
}

void AAstrawildEchoSpawner::BeginPlay()
{
	Super::BeginPlay();

	// Initial population
	for (int32 i = 0; i < MaxConcurrentEchoes; ++i)
	{
		SpawnEcho();
	}
}

void AAstrawildEchoSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	CleanupDeadInstances();

	if (ActiveSpawnedEchoes.Num() < MaxConcurrentEchoes)
	{
		RespawnTimer += DeltaTime;
		if (RespawnTimer >= RespawnIntervalSeconds)
		{
			RespawnTimer = 0.0f;
			SpawnEcho();
		}
	}
}

void AAstrawildEchoSpawner::CleanupDeadInstances()
{
	for (int32 i = ActiveSpawnedEchoes.Num() - 1; i >= 0; --i)
	{
		if (!ActiveSpawnedEchoes[i].IsValid() || 
			!ActiveSpawnedEchoes[i]->Attributes || 
			!ActiveSpawnedEchoes[i]->Attributes->IsAlive() ||
			ActiveSpawnedEchoes[i]->CurrentState == EAstrawildEchoState::Captured)
		{
			ActiveSpawnedEchoes.RemoveAt(i);
		}
	}
}

AAstrawildEchoBase* AAstrawildEchoSpawner::SpawnEcho(UAstrawildEchoDataAsset* SpeciesData, int32 Level)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UAstrawildEchoDataAsset* ChosenData = SpeciesData;
	if (!ChosenData && AvailableSpeciesPool.Num() > 0)
	{
		const int32 RandIndex = FMath::RandRange(0, AvailableSpeciesPool.Num() - 1);
		ChosenData = AvailableSpeciesPool[RandIndex];
	}

	const int32 SpawnLvl = (Level > 0) ? Level : FMath::RandRange(MinSpawnLevel, MaxSpawnLevel);

	// Find random point in radius
	FVector SpawnLocation = GetActorLocation() + FVector(FMath::RandRange(-SpawnRadius, SpawnRadius), FMath::RandRange(-SpawnRadius, SpawnRadius), 50.0f);
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (NavSys)
	{
		FNavLocation NavLoc;
		if (NavSys->GetRandomReachablePointInRadius(GetActorLocation(), SpawnRadius, NavLoc))
		{
			SpawnLocation = NavLoc.Location + FVector(0, 0, 50.0f);
		}
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	TSubclassOf<AAstrawildEchoBase> ClassToSpawn = (ChosenData && !ChosenData->EchoPawnClass.IsNull()) ? 
		ChosenData->EchoPawnClass.LoadSynchronous() : AAstrawildEchoBase::StaticClass();

	if (!ClassToSpawn)
	{
		ClassToSpawn = AAstrawildEchoBase::StaticClass();
	}

	AAstrawildEchoBase* NewEcho = World->SpawnActor<AAstrawildEchoBase>(ClassToSpawn, SpawnLocation, FRotator(0, FMath::RandRange(0.0f, 360.0f), 0), SpawnParams);
	if (NewEcho)
	{
		if (ChosenData)
		{
			NewEcho->InitializeFromSpeciesData(ChosenData, SpawnLvl);
		}
		ActiveSpawnedEchoes.Add(NewEcho);
		UE_LOG(LogAstrawildEcho, Log, TEXT("Spawner %s spawned %s at %s"), *GetName(), *NewEcho->GetName(), *SpawnLocation.ToString());
		return NewEcho;
	}

	return nullptr;
}

void AAstrawildEchoSpawner::DespawnAllEchoes()
{
	for (TWeakObjectPtr<AAstrawildEchoBase> EchoPtr : ActiveSpawnedEchoes)
	{
		if (EchoPtr.IsValid())
		{
			EchoPtr->Destroy();
		}
	}
	ActiveSpawnedEchoes.Empty();
	UE_LOG(LogAstrawildEcho, Log, TEXT("Spawner %s despawned all active echoes."), *GetName());
}

int32 AAstrawildEchoSpawner::GetActiveEchoCount() const
{
	return ActiveSpawnedEchoes.Num();
}