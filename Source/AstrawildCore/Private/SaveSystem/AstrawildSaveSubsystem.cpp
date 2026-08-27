// Copyright Epic Games, Inc. All Rights Reserved.

#include "SaveSystem/AstrawildSaveSubsystem.h"
#include "SaveSystem/AstrawildSaveGame.h"
#include "Characters/AstrawildCharacter.h"
#include "Components/AstrawildAttributeComponent.h"
#include "Components/AstrawildInventoryComponent.h"
#include "Components/AstrawildCaptureComponent.h"
#include "Environment/AstrawildBuildingPiece.h"
#include "Environment/AstrawildHarvestableNode.h"
#include "AstrawildLogChannels.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Engine/World.h"

UAstrawildSaveSubsystem::UAstrawildSaveSubsystem()
	: bEnableAutosave(true)
	, AutosaveIntervalSeconds(300.0f) // 5 minutes
	, bIsCurrentlySaving(false)
{
}

void UAstrawildSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogAstrawildSave, Log, TEXT("AstrawildSaveSubsystem Initialized."));

	UWorld* World = GetWorld();
	if (World && bEnableAutosave)
	{
		World->GetTimerManager().SetTimer(AutosaveTimerHandle, this, &UAstrawildSaveSubsystem::OnAutosaveTimerFired, AutosaveIntervalSeconds, true);
	}
}

void UAstrawildSaveSubsystem::Deinitialize()
{
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(AutosaveTimerHandle);
	}
	Super::Deinitialize();
}

void UAstrawildSaveSubsystem::OnAutosaveTimerFired()
{
	if (bEnableAutosave)
	{
		TriggerAutosave();
	}
}

bool UAstrawildSaveSubsystem::TriggerAutosave()
{
	UE_LOG(LogAstrawildSave, Log, TEXT("Triggering periodic Autosave..."));
	return SaveGameToSlot(TEXT("Autosave_Slot"));
}

bool UAstrawildSaveSubsystem::CaptureWorldState(UAstrawildSaveGame* SaveObject)
{
	if (!SaveObject)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	// 1. Capture Player Profile
	AAstrawildCharacter* PlayerChar = Cast<AAstrawildCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
	if (PlayerChar)
	{
		SaveObject->PlayerProfile.PlayerTransform = PlayerChar->GetActorTransform();

		if (PlayerChar->Attributes)
		{
			SaveObject->PlayerProfile.CurrentHealth = PlayerChar->Attributes->CurrentHealth;
			SaveObject->PlayerProfile.MaxHealth = PlayerChar->Attributes->MaxHealth;
			SaveObject->PlayerProfile.CurrentStamina = PlayerChar->Attributes->CurrentStamina;
			SaveObject->PlayerProfile.MaxStamina = PlayerChar->Attributes->MaxStamina;
			SaveObject->PlayerProfile.PlayerLevel = PlayerChar->Attributes->Level;
			SaveObject->PlayerProfile.CurrentEXP = PlayerChar->Attributes->CurrentEXP;
		}

		if (PlayerChar->Inventory)
		{
			SaveObject->PlayerProfile.InventorySlots = PlayerChar->Inventory->GetSlots();
		}

		if (PlayerChar->Capture)
		{
			SaveObject->PlayerProfile.ActiveParty = PlayerChar->Capture->ActiveParty;
			SaveObject->PlayerProfile.ReserveStorage = PlayerChar->Capture->ReserveStorage;
		}
	}

	// 2. Capture Placed Buildings
	SaveObject->WorldSnapshot.PlacedBuildings.Empty();
	for (TActorIterator<AAstrawildBuildingPiece> It(World); It; ++It)
	{
		SaveObject->WorldSnapshot.PlacedBuildings.Add(It->GetSaveData());
	}

	// 3. Capture Harvestable Nodes
	SaveObject->WorldSnapshot.HarvestNodes.Empty();
	for (TActorIterator<AAstrawildHarvestableNode> It(World); It; ++It)
	{
		SaveObject->WorldSnapshot.HarvestNodes.Add(It->GetSaveData());
	}

	SaveObject->WorldSnapshot.SnapshotTimestamp = FDateTime::Now();
	SaveObject->ValidateAndSanitize();
	return true;
}

bool UAstrawildSaveSubsystem::RestoreWorldState(UAstrawildSaveGame* SaveObject)
{
	if (!SaveObject)
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	SaveObject->ValidateAndSanitize();

	// 1. Restore Player Character
	AAstrawildCharacter* PlayerChar = Cast<AAstrawildCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
	if (PlayerChar)
	{
		PlayerChar->SetActorTransform(SaveObject->PlayerProfile.PlayerTransform);

		if (PlayerChar->Attributes)
		{
			PlayerChar->Attributes->MaxHealth = SaveObject->PlayerProfile.MaxHealth;
			PlayerChar->Attributes->CurrentHealth = SaveObject->PlayerProfile.CurrentHealth;
			PlayerChar->Attributes->MaxStamina = SaveObject->PlayerProfile.MaxStamina;
			PlayerChar->Attributes->CurrentStamina = SaveObject->PlayerProfile.CurrentStamina;
			PlayerChar->Attributes->Level = SaveObject->PlayerProfile.PlayerLevel;
			PlayerChar->Attributes->CurrentEXP = SaveObject->PlayerProfile.CurrentEXP;
		}

		if (PlayerChar->Inventory)
		{
			PlayerChar->Inventory->LoadInventorySlots(SaveObject->PlayerProfile.InventorySlots);
		}

		if (PlayerChar->Capture)
		{
			PlayerChar->Capture->LoadPartyData(SaveObject->PlayerProfile.ActiveParty, SaveObject->PlayerProfile.ReserveStorage);
		}
	}

	// 2. Restore Harvest Nodes
	TMap<FGuid, FAstrawildHarvestNodeSaveData> HarvestDataMap;
	for (const FAstrawildHarvestNodeSaveData& NodeData : SaveObject->WorldSnapshot.HarvestNodes)
	{
		HarvestDataMap.Add(NodeData.NodeGuid, NodeData);
	}

	for (TActorIterator<AAstrawildHarvestableNode> It(World); It; ++It)
	{
		if (FAstrawildHarvestNodeSaveData* Found = HarvestDataMap.Find(It->NodeUniqueId))
		{
			It->LoadSaveData(*Found);
		}
	}

	return true;
}

bool UAstrawildSaveSubsystem::SaveGameToSlot(const FString& SlotName)
{
	if (bIsCurrentlySaving)
	{
		return false;
	}

	bIsCurrentlySaving = true;
	LastSaveStatusBanner = FText::FromString(TEXT("Saving game..."));

	UAstrawildSaveGame* SaveObject = Cast<UAstrawildSaveGame>(UGameplayStatics::CreateSaveGameObject(UAstrawildSaveGame::StaticClass()));
	if (!SaveObject)
	{
		bIsCurrentlySaving = false;
		LastSaveStatusBanner = FText::FromString(TEXT("Save Failed: Object creation error."));
		OnSaveStatusChanged.Broadcast(false, SlotName, LastSaveStatusBanner);
		return false;
	}

	SaveObject->SaveSlotName = SlotName;
	SaveObject->SaveTimestamp = FDateTime::Now();
	CaptureWorldState(SaveObject);

	// 1. Create Backup copy of existing save if it exists
	const FString BackupSlotName = SlotName + TEXT("_Backup");
	if (DoesSaveSlotExist(SlotName))
	{
		UAstrawildSaveGame* OldSave = Cast<UAstrawildSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
		if (OldSave)
		{
			UGameplayStatics::SaveGameToSlot(OldSave, BackupSlotName, 0);
		}
	}

	// 2. Write new save safely
	const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveObject, SlotName, 0);

	bIsCurrentlySaving = false;
	if (bSaved)
	{
		LastSaveStatusBanner = FText::FromString(TEXT("Game Saved Successfully."));
		UE_LOG(LogAstrawildSave, Log, TEXT("Saved to slot '%s' successfully (Backup synced: %s)."), *SlotName, *BackupSlotName);
	}
	else
	{
		LastSaveStatusBanner = FText::FromString(TEXT("Save Failed: Disk write error."));
		UE_LOG(LogAstrawildSave, Error, TEXT("Failed to save to slot '%s'!"), *SlotName);
	}

	OnSaveStatusChanged.Broadcast(bSaved, SlotName, LastSaveStatusBanner);
	return bSaved;
}

bool UAstrawildSaveSubsystem::LoadGameFromSlot(const FString& SlotName)
{
	const FString BackupSlotName = SlotName + TEXT("_Backup");

	// 1. Try loading primary slot
	UAstrawildSaveGame* SaveObject = nullptr;
	if (DoesSaveSlotExist(SlotName))
	{
		SaveObject = Cast<UAstrawildSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	}

	// 2. Fallback to backup slot if primary slot is missing or corrupted
	if (!SaveObject && DoesSaveSlotExist(BackupSlotName))
	{
		UE_LOG(LogAstrawildSave, Warning, TEXT("Primary save slot '%s' failed. Attempting fallback to backup '%s'..."), *SlotName, *BackupSlotName);
		SaveObject = Cast<UAstrawildSaveGame>(UGameplayStatics::LoadGameFromSlot(BackupSlotName, 0));
		if (SaveObject)
		{
			LastSaveStatusBanner = FText::FromString(TEXT("Primary Save Corrupt: Restored from Backup!"));
		}
	}

	if (!SaveObject)
	{
		LastSaveStatusBanner = FText::FromString(TEXT("Load Failed: Save slot not found."));
		UE_LOG(LogAstrawildSave, Warning, TEXT("Cannot load save: Slot '%s' and backup not found."), *SlotName);
		OnSaveStatusChanged.Broadcast(false, SlotName, LastSaveStatusBanner);
		return false;
	}

	// Schema migration if necessary
	SaveObject->MigrateSchema(1);

	// Restore world and player
	const bool bRestored = RestoreWorldState(SaveObject);
	if (bRestored)
	{
		if (LastSaveStatusBanner.IsEmpty())
		{
			LastSaveStatusBanner = FText::FromString(TEXT("Game Loaded Successfully."));
		}
		UE_LOG(LogAstrawildSave, Log, TEXT("Loaded save slot '%s' successfully."), *SlotName);
	}

	OnSaveStatusChanged.Broadcast(bRestored, SlotName, LastSaveStatusBanner);
	return bRestored;
}

bool UAstrawildSaveSubsystem::DoesSaveSlotExist(const FString& SlotName) const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

void UAstrawildSaveSubsystem::DeleteSaveSlot(const FString& SlotName)
{
	UGameplayStatics::DeleteGameInSlot(SlotName, 0);
	UGameplayStatics::DeleteGameInSlot(SlotName + TEXT("_Backup"), 0);
	UE_LOG(LogAstrawildSave, Log, TEXT("Deleted save slot '%s' and backup."), *SlotName);
}