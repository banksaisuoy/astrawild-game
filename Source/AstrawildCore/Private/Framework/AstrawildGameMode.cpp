// Copyright Epic Games, Inc. All Rights Reserved.

#include "Framework/AstrawildGameMode.h"
#include "Framework/AstrawildGameState.h"
#include "Characters/AstrawildCharacter.h"
#include "Characters/AstrawildPlayerController.h"
#include "UI/AstrawildHUD.h"
#include "Environment/AstrawildPrototypeArena.h"
#include "Components/AstrawildQuestComponent.h"
#include "AstrawildLogChannels.h"
#include "Engine/DataTable.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

AAstrawildGameMode::AAstrawildGameMode()
{
	DefaultPawnClass = AAstrawildCharacter::StaticClass();
	PlayerControllerClass = AAstrawildPlayerController::StaticClass();
	GameStateClass = AAstrawildGameState::StaticClass();
	HUDClass = AAstrawildHUD::StaticClass();
}

void AAstrawildGameMode::StartPlay()
{
	Super::StartPlay();
	UE_LOG(LogAstrawild, Log, TEXT("ASTRAWILD: Echoes of the First Dawn - GameMode Started."));

	// Auto-spawn prototype testing arena if not already in level.
	UWorld* World = GetWorld();
	if (World)
	{
		bool bFoundArena = false;
		for (TActorIterator<AAstrawildPrototypeArena> It(World); It; ++It)
		{
			bFoundArena = true;
			break;
		}

		if (!bFoundArena)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			World->SpawnActor<AAstrawildPrototypeArena>(AAstrawildPrototypeArena::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
			UE_LOG(LogAstrawild, Log, TEXT("Auto-spawned AAstrawildPrototypeArena in level."));
		}
	}
}

void AAstrawildGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);
	BootstrapPlayerQuest(NewPlayer);
}

void AAstrawildGameMode::BootstrapPlayerQuest(APlayerController* NewPlayer)
{
	if (!bBootstrapFirstQuest || !NewPlayer)
	{
		return;
	}

	AAstrawildCharacter* Character = Cast<AAstrawildCharacter>(NewPlayer->GetPawn());
	if (!Character)
	{
		if (GetWorld())
		{
			GetWorld()->GetTimerManager().SetTimerForNextTick(
				FTimerDelegate::CreateUObject(this, &AAstrawildGameMode::BootstrapPlayerQuest, NewPlayer));
		}
		return;
	}

	if (!Character->Quest)
	{
		return;
	}
	if (!QuestTable.IsNull())
	{
		Character->Quest->QuestTable = QuestTable.LoadSynchronous();
	}
	if (!QuestObjectiveTable.IsNull())
	{
		Character->Quest->ObjectiveTable = QuestObjectiveTable.LoadSynchronous();
	}
	if (Character->Quest->QuestTable && !StartingQuestId.IsNone())
	{
		Character->Quest->StartQuest(StartingQuestId);
	}
}
