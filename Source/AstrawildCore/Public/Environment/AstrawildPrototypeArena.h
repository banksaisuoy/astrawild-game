// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildPrototypeArena.generated.h"

class UBoxComponent;

/**
 * Procedural Test Arena Actor that generates ramps, stairs, elevated ledges,
 * corridors, harvest nodes, and interactables for vertical slice movement & collision testing.
 */
UCLASS()
class ASTRAWILDCORE_API AAstrawildPrototypeArena : public AActor
{
	GENERATED_BODY()

public:
	AAstrawildPrototypeArena();

protected:
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena Settings")
	bool bAutoGenerateOnBeginPlay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arena Settings")
	float ArenaSize;

	UFUNCTION(BlueprintCallable, Category = "Arena Generation")
	void GenerateTestArena();

private:
	void SpawnTestPlatform(const FVector& Location, const FVector& Extent, const FRotator& Rotation = FRotator::ZeroRotator, const FColor& DebugColor = FColor::White);
	void SpawnTestEntities();
};