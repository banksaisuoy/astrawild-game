// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "AstrawildGameMode.generated.h"

UCLASS()
class ASTRAWILDCORE_API AAstrawildGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAstrawildGameMode();

protected:
	virtual void StartPlay() override;
};