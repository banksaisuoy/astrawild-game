// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "AstrawildTypes.h"
#include "AstrawildSaveGame.generated.h"

UCLASS()
class ASTRAWILDCORE_API UAstrawildSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	UAstrawildSaveGame();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save Metadata")
	int32 SchemaVersion;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save Metadata")
	FString SaveSlotName;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save Metadata")
	FDateTime SaveTimestamp;

	// --- Modular Profiles ---
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save Player")
	FAstrawildPlayerProfile PlayerProfile;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save World")
	FAstrawildWorldSnapshot WorldSnapshot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Save Settings")
	FAstrawildSettingsProfile SettingsProfile;

	// Migration & Integrity Methods
	UFUNCTION(BlueprintCallable, Category = "Save System")
	bool ValidateAndSanitize();

	UFUNCTION(BlueprintCallable, Category = "Save System")
	bool MigrateSchema(int32 TargetVersion);
};