// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildTypes.h"
#include "AstrawildEchoSpawner.generated.h"

class UAstrawildEchoDataAsset;
class AAstrawildEchoBase;

UCLASS()
class ASTRAWILDCORE_API AAstrawildEchoSpawner : public AActor
{
	GENERATED_BODY()

public:
	AAstrawildEchoSpawner();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Configuration")
	TArray<TObjectPtr<UAstrawildEchoDataAsset>> AvailableSpeciesPool;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Configuration")
	int32 MaxConcurrentEchoes;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Configuration")
	float SpawnRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Configuration")
	float RespawnIntervalSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Configuration")
	int32 MinSpawnLevel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spawner Configuration")
	int32 MaxSpawnLevel;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Spawner State")
	TArray<TWeakObjectPtr<AAstrawildEchoBase>> ActiveSpawnedEchoes;

public:
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	AAstrawildEchoBase* SpawnEcho(UAstrawildEchoDataAsset* SpeciesData = nullptr, int32 Level = 1);

	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void DespawnAllEchoes();

	UFUNCTION(BlueprintPure, Category = "Spawner")
	int32 GetActiveEchoCount() const;

private:
	float RespawnTimer;
	void CleanupDeadInstances();
};