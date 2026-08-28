// Copyright Epic Games, Inc. All Rights Reserved.

#include "Characters/AstrawildPlayerController.h"
#include "Characters/AstrawildCharacter.h"
#include "Echoes/AstrawildEchoBase.h"
#include "Echoes/AstrawildEchoAIController.h"
#include "Data/AstrawildEchoDataAsset.h"
#include "Components/AstrawildInventoryComponent.h"
#include "Components/AstrawildAttributeComponent.h"
#include "Components/AstrawildBuildingComponent.h"
#include "SaveSystem/AstrawildSaveSubsystem.h"
#include "UI/AstrawildHUD.h"
#include "AstrawildLogChannels.h"
#include "EngineUtils.h"
#include "Engine/World.h"

AAstrawildPlayerController::AAstrawildPlayerController()
{
	bShowMouseCursor = false;
}

void AAstrawildPlayerController::BeginPlay()
{
	Super::BeginPlay();
	SetUIMode(false);
}

void AAstrawildPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (InputComponent)
	{
		InputComponent->BindKey(EKeys::F1, IE_Pressed, this, &AAstrawildPlayerController::ToggleDebugHUD);
		InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &AAstrawildPlayerController::ToggleDebugHUD);
		InputComponent->BindKey(EKeys::I, IE_Pressed, this, &AAstrawildPlayerController::ToggleInventoryMenu);
		InputComponent->BindKey(EKeys::B, IE_Pressed, this, &AAstrawildPlayerController::Astrawild_BuildCampfire);
	}
}

void AAstrawildPlayerController::SetUIMode(bool bEnableUI)
{
	if (bEnableUI)
	{
		bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
	}
	else
	{
		bShowMouseCursor = false;
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
	}
}

void AAstrawildPlayerController::ToggleDebugHUD()
{
	AAstrawildHUD* AstrawildHUD = Cast<AAstrawildHUD>(GetHUD());
	if (AstrawildHUD)
	{
		AstrawildHUD->ToggleDebugOverlay();
		UE_LOG(LogAstrawild, Log, TEXT("Toggled Debug HUD Overlay: %d"), AstrawildHUD->bShowDebugOverlay);
	}
}

void AAstrawildPlayerController::ToggleInventoryMenu()
{
	AAstrawildHUD* AstrawildHUD = Cast<AAstrawildHUD>(GetHUD());
	if (AstrawildHUD)
	{
		AstrawildHUD->ToggleInventoryMenu();
		SetUIMode(AstrawildHUD->bShowInventoryMenu);
		UE_LOG(LogAstrawild, Log, TEXT("Toggled Inventory Menu: %d"), AstrawildHUD->bShowInventoryMenu);
	}
}

void AAstrawildPlayerController::Astrawild_ToggleInventory()
{
	ToggleInventoryMenu();
}

void AAstrawildPlayerController::Astrawild_BuildCampfire()
{
	AAstrawildCharacter* Char = Cast<AAstrawildCharacter>(GetPawn());
	if (Char && Char->Building)
	{
		if (Char->Building->bIsBuildModeActive)
		{
			Char->Building->ExitBuildMode();
		}
		else
		{
			TArray<FAstrawildRecipeIngredient> Cost;
			Cost.Add(FAstrawildRecipeIngredient{ FGameplayTag::RequestGameplayTag(FName("Item.Resource.Sunwood"), false), 4 });
			Cost.Add(FAstrawildRecipeIngredient{ FGameplayTag::RequestGameplayTag(FName("Item.Resource.LumenStone"), false), 2 });

			Char->Building->EnterBuildMode(nullptr, FGameplayTag::RequestGameplayTag(FName("Building.Campfire"), false), Cost);
		}
	}
}

void AAstrawildPlayerController::Astrawild_BuildBed()
{
	AAstrawildCharacter* Char = Cast<AAstrawildCharacter>(GetPawn());
	if (Char && Char->Building)
	{
		if (Char->Building->bIsBuildModeActive)
		{
			Char->Building->ExitBuildMode();
		}
		else
		{
			TArray<FAstrawildRecipeIngredient> Cost;
			Cost.Add(FAstrawildRecipeIngredient{ FGameplayTag::RequestGameplayTag(FName("Item.Resource.Sunwood"), false), 6 });

			Char->Building->EnterBuildMode(nullptr, FGameplayTag::RequestGameplayTag(FName("Building.RestBed"), false), Cost);
		}
	}
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

void AAstrawildPlayerController::Astrawild_ToggleDebugHUD()
{
	ToggleDebugHUD();
}

void AAstrawildPlayerController::Astrawild_SpawnEcho(const FString& SpeciesTagName, int32 Level)
{
	APawn* PlayerPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!PlayerPawn || !World)
	{
		return;
	}

	const FVector SpawnLoc = PlayerPawn->GetActorLocation() + (PlayerPawn->GetActorForwardVector() * 300.0f) + FVector(0, 0, 50.0f);
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	AAstrawildEchoBase* SpawnedEcho = World->SpawnActor<AAstrawildEchoBase>(AAstrawildEchoBase::StaticClass(), SpawnLoc, PlayerPawn->GetActorRotation(), SpawnParams);
	if (SpawnedEcho)
	{
		// Create temporary inline data asset if not loaded from disk
		UAstrawildEchoDataAsset* TempData = NewObject<UAstrawildEchoDataAsset>();
		TempData->SpeciesTag = FGameplayTag::RequestGameplayTag(FName(*SpeciesTagName), false);
		TempData->SpeciesName = FText::FromString(SpeciesTagName);

		if (SpeciesTagName.Contains(TEXT("Pyrelite")))
		{
			TempData->SpeciesName = FText::FromString(TEXT("Pyrelite"));
			TempData->SpeciesTitle = FText::FromString(TEXT("The Ember Fawn"));
			TempData->ElementalAffinity = EAstrawildElement::Solar;
			TempData->Role = EAstrawildEchoRole::Exploration;
			TempData->BaseMaxHealth = 280.0f;
			TempData->BaseAttackPower = 42.0f;
			TempData->BaseDefensePower = 22.0f;
			TempData->BaseWalkSpeed = 300.0f;
			TempData->BaseRunSpeed = 620.0f;
		}
		else if (SpeciesTagName.Contains(TEXT("Thornback")))
		{
			TempData->SpeciesName = FText::FromString(TEXT("Thornback"));
			TempData->SpeciesTitle = FText::FromString(TEXT("The Terra Bastion"));
			TempData->ElementalAffinity = EAstrawildElement::Geo;
			TempData->Role = EAstrawildEchoRole::Combat;
			TempData->BaseMaxHealth = 450.0f;
			TempData->BaseAttackPower = 32.0f;
			TempData->BaseDefensePower = 48.0f;
			TempData->BaseWalkSpeed = 220.0f;
			TempData->BaseRunSpeed = 420.0f;
		}
		else if (SpeciesTagName.Contains(TEXT("Aquavine")))
		{
			TempData->SpeciesName = FText::FromString(TEXT("Aquavine"));
			TempData->SpeciesTitle = FText::FromString(TEXT("The Dew Serpent"));
			TempData->ElementalAffinity = EAstrawildElement::Torrent;
			TempData->Role = EAstrawildEchoRole::BaseUtility;
			TempData->BaseMaxHealth = 340.0f;
			TempData->BaseAttackPower = 36.0f;
			TempData->BaseDefensePower = 28.0f;
			TempData->BaseWalkSpeed = 260.0f;
			TempData->BaseRunSpeed = 500.0f;
		}

		SpawnedEcho->InitializeFromSpeciesData(TempData, Level);
		UE_LOG(LogAstrawildEcho, Log, TEXT("Console Exec: Spawned %s (Level %d) at %s"), *SpeciesTagName, Level, *SpawnLoc.ToString());
	}
}

void AAstrawildPlayerController::Astrawild_ListEchoes()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	int32 Count = 0;
	UE_LOG(LogAstrawildEcho, Log, TEXT("=== ACTIVE WORLD ECHOES ==="));
	for (TActorIterator<AAstrawildEchoBase> It(World); It; ++It)
	{
		Count++;
		const FString Name = It->SpeciesData ? It->SpeciesData->SpeciesName.ToString() : It->GetName();
		const FString State = UEnum::GetValueAsString(It->CurrentState);
		const float HP = It->Attributes ? It->Attributes->CurrentHealth : 0.0f;
		UE_LOG(LogAstrawildEcho, Log, TEXT("[%d] %s | Level %d | HP: %.0f | State: %s"), Count, *Name, It->InstanceData.Level, HP, *State);
	}
	UE_LOG(LogAstrawildEcho, Log, TEXT("Total Active Echoes: %d"), Count);
}

void AAstrawildPlayerController::Astrawild_KillAllWildEchoes()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	int32 Destroyed = 0;
	for (TActorIterator<AAstrawildEchoBase> It(World); It; ++It)
	{
		if (It->CurrentState == EAstrawildEchoState::WildPassive || It->CurrentState == EAstrawildEchoState::WildHostile)
		{
			It->Destroy();
			Destroyed++;
		}
	}
	UE_LOG(LogAstrawildEcho, Log, TEXT("Destroyed %d wild Echoes."), Destroyed);
}

void AAstrawildPlayerController::Astrawild_ToggleAIDebug()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AAstrawildEchoAIController> It(World); It; ++It)
	{
		It->ToggleAIDebug();
	}
	UE_LOG(LogAstrawildEcho, Log, TEXT("Toggled AI Perception & Territory Debug Visuals."));
}