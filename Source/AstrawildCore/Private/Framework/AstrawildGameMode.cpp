// Copyright Epic Games, Inc. All Rights Reserved.

#include "Framework/AstrawildGameMode.h"
#include "Framework/AstrawildGameState.h"
#include "Characters/AstrawildCharacter.h"
#include "Characters/AstrawildPlayerController.h"
#include "AstrawildLogChannels.h"

AAstrawildGameMode::AAstrawildGameMode()
{
	DefaultPawnClass = AAstrawildCharacter::StaticClass();
	PlayerControllerClass = AAstrawildPlayerController::StaticClass();
	GameStateClass = AAstrawildGameState::StaticClass();
}

void AAstrawildGameMode::StartPlay()
{
	Super::StartPlay();
	UE_LOG(LogAstrawild, Log, TEXT("ASTRAWILD: Echoes of the First Dawn - GameMode Started."));
}