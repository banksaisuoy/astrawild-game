// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AstrawildPlayerController.generated.h"

UCLASS()
class ASTRAWILDCORE_API AAstrawildPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AAstrawildPlayerController();

protected:
	virtual void BeginPlay() override;

public:
	// --- In-Game Debug Console Commands ---
	UFUNCTION(Exec, Category = "Astrawild Debug")
	void Astrawild_GiveItem(const FString& ItemTagName, int32 Quantity = 1);

	UFUNCTION(Exec, Category = "Astrawild Debug")
	void Astrawild_Heal();

	UFUNCTION(Exec, Category = "Astrawild Debug")
	void Astrawild_SaveGame(const FString& SlotName = TEXT("Slot_01"));

	UFUNCTION(Exec, Category = "Astrawild Debug")
	void Astrawild_LoadGame(const FString& SlotName = TEXT("Slot_01"));
};