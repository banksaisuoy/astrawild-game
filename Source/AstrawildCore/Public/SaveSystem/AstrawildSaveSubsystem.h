// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AstrawildSaveSubsystem.generated.h"

class UAstrawildSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSaveGameResultSignature, bool, bSuccess, const FString&, SlotName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLoadGameResultSignature, bool, bSuccess, const FString&, SlotName);

UCLASS()
class ASTRAWILDCORE_API UAstrawildSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	UPROPERTY(BlueprintAssignable, Category = "Save|Events")
	FOnSaveGameResultSignature OnSaveGameCompleted;

	UPROPERTY(BlueprintAssignable, Category = "Save|Events")
	FOnLoadGameResultSignature OnLoadGameCompleted;

public:
	UFUNCTION(BlueprintCallable, Category = "Save System")
	bool SaveGameToSlot(const FString& SlotName = TEXT("Slot_01"));

	UFUNCTION(BlueprintCallable, Category = "Save System")
	bool LoadGameFromSlot(const FString& SlotName = TEXT("Slot_01"));

	UFUNCTION(BlueprintPure, Category = "Save System")
	bool DoesSaveSlotExist(const FString& SlotName = TEXT("Slot_01")) const;

	UFUNCTION(BlueprintCallable, Category = "Save System")
	void DeleteSaveSlot(const FString& SlotName = TEXT("Slot_01"));
};