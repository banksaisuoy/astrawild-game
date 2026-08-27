// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "AstrawildTypes.h"
#include "AstrawildSaveSubsystem.generated.h"

class UAstrawildSaveGame;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnSaveGameStatusChangedSignature, bool, bSuccess, const FString&, SlotName, const FText&, StatusMessage);

UCLASS()
class ASTRAWILDCORE_API UAstrawildSaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UAstrawildSaveSubsystem();

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Config")
	bool bEnableAutosave;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Config")
	float AutosaveIntervalSeconds;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|State")
	bool bIsCurrentlySaving;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save|State")
	FText LastSaveStatusBanner;

	UPROPERTY(BlueprintAssignable, Category = "Save|Events")
	FOnSaveGameStatusChangedSignature OnSaveStatusChanged;

public:
	UFUNCTION(BlueprintCallable, Category = "Save System")
	bool SaveGameToSlot(const FString& SlotName = TEXT("Slot_01"));

	UFUNCTION(BlueprintCallable, Category = "Save System")
	bool LoadGameFromSlot(const FString& SlotName = TEXT("Slot_01"));

	UFUNCTION(BlueprintCallable, Category = "Save System")
	bool TriggerAutosave();

	UFUNCTION(BlueprintPure, Category = "Save System")
	bool DoesSaveSlotExist(const FString& SlotName = TEXT("Slot_01")) const;

	UFUNCTION(BlueprintCallable, Category = "Save System")
	void DeleteSaveSlot(const FString& SlotName = TEXT("Slot_01"));

private:
	FTimerHandle AutosaveTimerHandle;
	void OnAutosaveTimerFired();

	bool CaptureWorldState(UAstrawildSaveGame* SaveObject);
	bool RestoreWorldState(UAstrawildSaveGame* SaveObject);
};