// Copyright Epic Games, Inc. All Rights Reserved.

#include "AstrawildCore.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "FAstrawildCoreModule"

void FAstrawildCoreModule::StartupModule()
{
	UE_LOG(LogTemp, Log, TEXT("AstrawildCore: Module Initialized."));
}

void FAstrawildCoreModule::ShutdownModule()
{
	UE_LOG(LogTemp, Log, TEXT("AstrawildCore: Module Shutdown."));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_PRIMARY_GAME_MODULE(FAstrawildCoreModule, AstrawildCore, "AstrawildCore");