// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "AstrawildHUD.generated.h"

class AAstrawildCharacter;

UCLASS()
class ASTRAWILDCORE_API AAstrawildHUD : public AHUD
{
	GENERATED_BODY()

public:
	AAstrawildHUD();

	virtual void DrawHUD() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Display")
	bool bShowHUD;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Display")
	bool bShowDebugOverlay;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Display")
	bool bShowInventoryMenu;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Display")
	FColor HealthBarColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Display")
	FColor StaminaBarColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Display")
	FColor CrosshairColor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "HUD Display")
	FColor CrosshairHighlightColor;

public:
	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleDebugOverlay();

	UFUNCTION(BlueprintCallable, Category = "HUD")
	void ToggleInventoryMenu();

private:
	void DrawHealthAndStaminaBars(AAstrawildCharacter* PlayerChar);
	void DrawCenterCrosshair(AAstrawildCharacter* PlayerChar);
	void DrawInteractionPrompt(AAstrawildCharacter* PlayerChar);
	void DrawActiveCompanionBadge(AAstrawildCharacter* PlayerChar);
	void DrawDebugOverlay(AAstrawildCharacter* PlayerChar);
	void DrawInventoryAndCraftingMenu(AAstrawildCharacter* PlayerChar);
	void DrawSaveStatusBanner();
};