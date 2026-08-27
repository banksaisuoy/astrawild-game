// Copyright Epic Games, Inc. All Rights Reserved.

#include "SaveSystem/AstrawildSaveGame.h"

UAstrawildSaveGame::UAstrawildSaveGame()
	: SaveSlotName(TEXT("Slot_01"))
	, SaveTimestamp(FDateTime::Now())
	, PlayerTransform(FTransform::Identity)
	, PlayerHealth(100.0f)
	, PlayerMaxHealth(100.0f)
	, PlayerStamina(100.0f)
	, PlayerMaxStamina(100.0f)
	, PlayerLevel(1)
	, PlayerCurrentEXP(0.0f)
	, WorldTimeOfDay(8.0f)
{
}