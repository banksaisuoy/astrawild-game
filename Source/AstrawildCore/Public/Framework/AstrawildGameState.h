// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "AstrawildGameState.generated.h"

UCLASS()
class ASTRAWILDCORE_API AAstrawildGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AAstrawildGameState();

protected:
	virtual void Tick(float DeltaSeconds) override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Time")
	float DayNightCycleDurationMinutes;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Time")
	float CurrentTimeOfDayHours;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World State")
	int32 ActiveWildEchoesCount;
};