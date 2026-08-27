// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AstrawildTypes.h"
#include "AstrawildSaveGame.generated.h"

UCLASS()
class ASTRAWILDCORE_API UAstrawildSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UAstrawildSaveGame();

	UPROPERTY(VisibleAnywhere, Category = "Save Metadata")
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, Category = "Save Metadata")
	FDateTime SaveTimestamp;

	// --- Player State ---
	UPROPERTY(VisibleAnywhere, Category = "Save Player")
	FTransform PlayerTransform;

	UPROPERTY(VisibleAnywhere, Category = "Save Player")
	float PlayerHealth;

	UPROPERTY(VisibleAnywhere, Category = "Save Player")
	float PlayerMaxHealth;

	UPROPERTY(VisibleAnywhere, Category = "Save Player")
	float PlayerStamina;

	UPROPERTY(VisibleAnywhere, Category = "Save Player")
	float PlayerMaxStamina;

	UPROPERTY(VisibleAnywhere, Category = "Save Player")
	int32 PlayerLevel;

	UPROPERTY(VisibleAnywhere, Category = "Save Player")
	float PlayerCurrentEXP;

	UPROPERTY(VisibleAnywhere, Category = "Save Player")
	TArray<FAstrawildItemSlot> PlayerInventory;

	// --- Echoes ---
	UPROPERTY(VisibleAnywhere, Category = "Save Echoes")
	TArray<FAstrawildCapturedEchoData> ActivePartyEchoes;

	UPROPERTY(VisibleAnywhere, Category = "Save Echoes")
	TArray<FAstrawildCapturedEchoData> ReserveStorageEchoes;

	// --- World Structures & Nodes ---
	UPROPERTY(VisibleAnywhere, Category = "Save World")
	TArray<FAstrawildBuildingSaveData> PlacedWorldBuildings;

	UPROPERTY(VisibleAnywhere, Category = "Save World")
	TArray<FAstrawildHarvestNodeSaveData> HarvestNodes;

	UPROPERTY(VisibleAnywhere, Category = "Save World")
	float WorldTimeOfDay;
};