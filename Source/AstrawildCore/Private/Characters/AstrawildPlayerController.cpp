// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/AstrawildPlayerController.h"
#include "Characters/AstrawildCharacter.h"
#include "Components/AstrawildInventoryComponent.h"
#include "Components/AstrawildAttributeComponent.h"
#include "SaveSystem/AstrawildSaveSubsystem.h"
#include "AstrawildLogChannels.h"

AAstrawildPlayerController::AAstrawildPlayerController()
{
	bShowMouseCursor = false;
}

void AAstrawildPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetInputMode(FInputModeGameOnly());
}

void AAstrawildPlayerController::Astrawild_GiveItem(const FString& ItemTagName, int32 Quantity)
{
	AAstrawildCharacter* Char = Cast<AAstrawildCharacter>(GetPawn());
	if (!Char || !Char->Inventory)
	{
		return;
	}

	const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*ItemTagName), false);
	if (Tag.IsValid())
	{
		Char->Inventory->AddItem(Tag, Quantity);
		UE_LOG(LogAstrawildInventory, Log, TEXT("Console Exec: Added %s x%d"), *ItemTagName, Quantity);
	}
	else
	{
		UE_LOG(LogAstrawildInventory, Warning, TEXT("Console Exec: Invalid Item Tag '%s'"), *ItemTagName);
	}
}

void AAstrawildPlayerController::Astrawild_Heal()
{
	AAstrawildCharacter* Char = Cast<AAstrawildCharacter>(GetPawn());
	if (Char && Char->Attributes)
	{
		Char->Attributes->ResetToMax();
		UE_LOG(LogAstrawild, Log, TEXT("Console Exec: Player healed to full."));
	}
}

void AAstrawildPlayerController::Astrawild_SaveGame(const FString& SlotName)
{
	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		UAstrawildSaveSubsystem* SaveSys = GI->GetSubsystem<UAstrawildSaveSubsystem>();
		if (SaveSys)
		{
			SaveSys->SaveGameToSlot(SlotName);
		}
	}
}

void AAstrawildPlayerController::Astrawild_LoadGame(const FString& SlotName)
{
	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		UAstrawildSaveSubsystem* SaveSys = GI->GetSubsystem<UAstrawildSaveSubsystem>();
		if (SaveSys)
		{
			SaveSys->LoadGameFromSlot(SlotName);
		}
	}
}