// Copyright Epic Games, Inc. All Rights Reserved.

#include "Framework/AstrawildGameState.h"

AAstrawildGameState::AAstrawildGameState()
	: DayNightCycleDurationMinutes(24.0f)
	, CurrentTimeOfDayHours(8.0f) // Start at 8:00 AM (Dawn/Morning)
	, ActiveWildEchoesCount(0)
{
	PrimaryActorTick.bCanEverTick = true;
}

void AAstrawildGameState::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Advance time of day
	const float HoursPerSecond = 24.0f / (DayNightCycleDurationMinutes * 60.0f);
	CurrentTimeOfDayHours += HoursPerSecond * DeltaSeconds;
	if (CurrentTimeOfDayHours >= 24.0f)
	{
		CurrentTimeOfDayHours -= 24.0f;
	}
}