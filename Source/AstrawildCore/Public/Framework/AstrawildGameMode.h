// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/SoftObjectPtr.h"
#include "AstrawildGameMode.generated.h"

class UDataTable;
class APlayerController;

UCLASS()
class ASTRAWILDCORE_API AAstrawildGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AAstrawildGameMode();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASTRAWILD|First Loop")
	TSoftObjectPtr<UDataTable> QuestTable = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Astrawild/Data/Imported/DT_Quests.DT_Quests")));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASTRAWILD|First Loop")
	TSoftObjectPtr<UDataTable> QuestObjectiveTable = TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/Astrawild/Data/Imported/DT_QuestObjectives.DT_QuestObjectives")));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASTRAWILD|First Loop")
	FName StartingQuestId = TEXT("Quest.Awakening");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ASTRAWILD|First Loop")
	bool bBootstrapFirstQuest = true;

protected:
	virtual void StartPlay() override;
	virtual void PostLogin(APlayerController* NewPlayer) override;

private:
	void BootstrapPlayerQuest(APlayerController* NewPlayer);
};