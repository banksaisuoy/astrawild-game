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

void UAstrawildSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UE_LOG(LogAstrawildSave, Log, TEXT("AstrawildSaveSubsystem Initialized."));
}

bool UAstrawildSaveSubsystem::SaveGameToSlot(const FString& SlotName)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	UAstrawildSaveGame* SaveObject = Cast<UAstrawildSaveGame>(UGameplayStatics::CreateSaveGameObject(UAstrawildSaveGame::StaticClass()));
	if (!SaveObject)
	{
		return false;
	}

	SaveObject->SaveSlotName = SlotName;
	SaveObject->SaveTimestamp = FDateTime::Now();

	// 1. Serialize Player Character
	AAstrawildCharacter* PlayerChar = Cast<AAstrawildCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
	if (PlayerChar)
	{
		SaveObject->PlayerTransform = PlayerChar->GetActorTransform();

		if (PlayerChar->Attributes)
		{
			SaveObject->PlayerHealth = PlayerChar->Attributes->CurrentHealth;
			SaveObject->PlayerMaxHealth = PlayerChar->Attributes->MaxHealth;
			SaveObject->PlayerStamina = PlayerChar->Attributes->CurrentStamina;
			SaveObject->PlayerMaxStamina = PlayerChar->Attributes->MaxStamina;
			SaveObject->PlayerLevel = PlayerChar->Attributes->Level;
			SaveObject->PlayerCurrentEXP = PlayerChar->Attributes->CurrentEXP;
		}

		if (PlayerChar->Inventory)
		{
			SaveObject->PlayerInventory = PlayerChar->Inventory->GetSlots();
		}

		if (PlayerChar->Capture)
		{
			SaveObject->ActivePartyEchoes = PlayerChar->Capture->ActiveParty;
			SaveObject->ReserveStorageEchoes = PlayerChar->Capture->ReserveStorage;
		}
	}

	// 2. Serialize Placed Buildings
	for (TActorIterator<AAstrawildBuildingPiece> It(World); It; ++It)
	{
		SaveObject->PlacedWorldBuildings.Add(It->GetSaveData());
	}

	// 3. Serialize Harvestable Nodes
	for (TActorIterator<AAstrawildHarvestableNode> It(World); It; ++It)
	{
		SaveObject->HarvestNodes.Add(It->GetSaveData());
	}

	const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveObject, SlotName, 0);
	UE_LOG(LogAstrawildSave, Log, TEXT("Save to slot '%s' result: %s (Buildings: %d, Party Echoes: %d)"),
		*SlotName, bSaved ? TEXT("SUCCESS") : TEXT("FAILED"), SaveObject->PlacedWorldBuildings.Num(), SaveObject->ActivePartyEchoes.Num());

	OnSaveGameCompleted.Broadcast(bSaved, SlotName);
	return bSaved;
}

bool UAstrawildSaveSubsystem::LoadGameFromSlot(const FString& SlotName)
{
	UWorld* World = GetWorld();
	if (!World || !DoesSaveSlotExist(SlotName))
	{
		UE_LOG(LogAstrawildSave, Warning, TEXT("Cannot load save: Slot '%s' not found."), *SlotName);
		OnLoadGameCompleted.Broadcast(false, SlotName);
		return false;
	}

	UAstrawildSaveGame* SaveObject = Cast<UAstrawildSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
	if (!SaveObject)
	{
		OnLoadGameCompleted.Broadcast(false, SlotName);
		return false;
	}

	// 1. Restore Player Character
	AAstrawildCharacter* PlayerChar = Cast<AAstrawildCharacter>(UGameplayStatics::GetPlayerCharacter(World, 0));
	if (PlayerChar)
	{
		PlayerChar->SetActorTransform(SaveObject->PlayerTransform);

		if (PlayerChar->Attributes)
		{
			PlayerChar->Attributes->MaxHealth = SaveObject->PlayerMaxHealth;
			PlayerChar->Attributes->CurrentHealth = SaveObject->PlayerHealth;
			PlayerChar->Attributes->MaxStamina = SaveObject->PlayerMaxStamina;
			PlayerChar->Attributes->CurrentStamina = SaveObject->PlayerStamina;
			PlayerChar->Attributes->Level = SaveObject->PlayerLevel;
			PlayerChar->Attributes->CurrentEXP = SaveObject->PlayerCurrentEXP;
		}

		if (PlayerChar->Inventory)
		{
			PlayerChar->Inventory->LoadInventorySlots(SaveObject->PlayerInventory);
		}

		if (PlayerChar->Capture)
		{
			PlayerChar->Capture->LoadPartyData(SaveObject->ActivePartyEchoes, SaveObject->ReserveStorageEchoes);
		}
	}

	// 2. Restore Harvest Nodes
	TMap<FGuid, FAstrawildHarvestNodeSaveData> HarvestDataMap;
	for (const FAstrawildHarvestNodeSaveData& NodeData : SaveObject->HarvestNodes)
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

	UE_LOG(LogAstrawildSave, Log, TEXT("Loaded save slot '%s' successfully."), *SlotName);
	OnLoadGameCompleted.Broadcast(true, SlotName);
	return true;
}

bool UAstrawildSaveSubsystem::DoesSaveSlotExist(const FString& SlotName) const
{
	return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}

void UAstrawildSaveSubsystem::DeleteSaveSlot(const FString& SlotName)
{
	UGameplayStatics::DeleteGameInSlot(SlotName, 0);
	UE_LOG(LogAstrawildSave, Log, TEXT("Deleted save slot '%s'."), *SlotName);
}