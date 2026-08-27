// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AstrawildPlayerController.generated.h"

class AAstrawildHUD;

UCLASS()
class ASTRAWILDCORE_API AAstrawildPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAstrawildPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;

public:
	// --- UI & Input Mode Management ---
	UFUNCTION(BlueprintCallable, Category = "Astrawild UI")
	void SetUIMode(bool bEnableUI);

	UFUNCTION(BlueprintCallable, Category = "Astrawild UI")
	void ToggleDebugHUD();

	// --- In-Game Debug Console Commands ---
	UFUNCTION(Exec, Category = "Astrawild Debug")
	void Astrawild_GiveItem(const FString& ItemTagName, int32 Quantity = 1);

	UFUNCTION(Exec, Category = "Astrawild Debug")
	void Astrawild_Heal();

	UFUNCTION(Exec, Category = "Astrawild Debug")
	void Astrawild_SaveGame(const FString& SlotName = TEXT("Slot_01"));

	UFUNCTION(Exec, Category = "Astrawild Debug")
	void Astrawild_LoadGame(const FString& SlotName = TEXT("Slot_01"));

	UFUNCTION(Exec, Category = "Astrawild Debug")
	void Astrawild_ToggleDebugHUD();

	UFUNCTION(Exec, Category = "Astrawild Debug")
	void Astrawild_SpawnEcho(const FString& SpeciesTagName = TEXT("Echo.Pyrelite"), int32 Level = 1);

	UFUNCTION(Exec, Category = "Astrawild Debug")
	void Astrawild_ListEchoes();

	UFUNCTION(Exec, Category = "Astrawild Debug")
	void Astrawild_KillAllWildEchoes();
};