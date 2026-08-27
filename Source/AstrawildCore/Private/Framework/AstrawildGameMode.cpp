// Copyright Epic Games, Inc. All Rights Reserved.

#include "Framework/AstrawildGameMode.h"
#include "Framework/AstrawildGameState.h"
#include "Characters/AstrawildCharacter.h"
#include "Characters/AstrawildPlayerController.h"
#include "UI/AstrawildHUD.h"
#include "Environment/AstrawildPrototypeArena.h"
#include "AstrawildLogChannels.h"
#include "EngineUtils.h"

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

	// Auto-spawn prototype testing arena if not already in level
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
			World->SpawnActor<AAstrawildPrototypeArena>(AAstrawildPrototypeArena::StaticClass(), FVector(0.0f, 0.0f, 0.0f), FRotator::ZeroRotator, SpawnParams);
			UE_LOG(LogAstrawild, Log, TEXT("Auto-spawned AAstrawildPrototypeArena in level."));
		}
	}
}