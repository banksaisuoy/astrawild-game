#include "UI/AstrawildHUD.h"
#include "Characters/AstrawildCharacter.h"
#include "Components/AstrawildAttributeComponent.h"
#include "Components/AstrawildCaptureComponent.h"
#include "Components/AstrawildInventoryComponent.h"
#include "Components/AstrawildCraftingComponent.h"
#include "Components/AstrawildBuildingComponent.h"
#include "SaveSystem/AstrawildSaveSubsystem.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "GameFramework/CharacterMovementComponent.h"

AAstrawildHUD::AAstrawildHUD()
	: bShowHUD(true)
	, bShowDebugOverlay(true) // Enabled by default for vertical slice prototype testing
	, bShowInventoryMenu(false)
	, HealthBarColor(FColor(46, 204, 113)) // Emerald green
	, StaminaBarColor(FColor(241, 196, 15)) // Sun gold
	, CrosshairColor(FColor(236, 240, 241, 180)) // Soft white
	, CrosshairHighlightColor(FColor(230, 126, 34)) // Vibrant orange
{
}

void AAstrawildHUD::DrawHUD()
{
	Super::DrawHUD();

	if (!bShowHUD || !Canvas)
	{
		return;
	}

	AAstrawildCharacter* PlayerChar = Cast<AAstrawildCharacter>(GetOwningPawn());
	if (!PlayerChar)
	{
		return;
	}

	DrawHealthAndStaminaBars(PlayerChar);
	DrawCenterCrosshair(PlayerChar);
	DrawInteractionPrompt(PlayerChar);
	DrawActiveCompanionBadge(PlayerChar);

	if (bShowInventoryMenu)
	{
		DrawInventoryAndCraftingMenu(PlayerChar);
	}

	DrawSaveStatusBanner();

	if (bShowDebugOverlay)
	{
		DrawDebugOverlay(PlayerChar);
	}
}

void AAstrawildHUD::ToggleDebugOverlay()
{
	bShowDebugOverlay = !bShowDebugOverlay;
}

void AAstrawildHUD::ToggleInventoryMenu()
{
	bShowInventoryMenu = !bShowInventoryMenu;
}

void AAstrawildHUD::DrawHealthAndStaminaBars(AAstrawildCharacter* PlayerChar)
{
	if (!PlayerChar || !PlayerChar->Attributes)
	{
		return;
	}

	const float BarWidth = 260.0f;
	const float BarHeight = 18.0f;
	const float MarginX = 30.0f;
	const float MarginY = 30.0f;

	// 1. Health Bar
	const float HealthPct = FMath::Clamp(PlayerChar->Attributes->GetHealthPercent(), 0.0f, 1.0f);
	DrawRect(FColor(30, 30, 30, 200), MarginX, MarginY, BarWidth, BarHeight);
	DrawRect(HealthBarColor, MarginX + 2.0f, MarginY + 2.0f, (BarWidth - 4.0f) * HealthPct, BarHeight - 4.0f);
	const FString HealthText = FString::Printf(TEXT("HP: %.0f / %.0f"), PlayerChar->Attributes->CurrentHealth, PlayerChar->Attributes->MaxHealth);
	DrawText(HealthText, FColor::White, MarginX + 8.0f, MarginY + 2.0f, nullptr, 0.9f);

	// 2. Stamina Bar
	const float StaminaPct = FMath::Clamp(PlayerChar->Attributes->GetStaminaPercent(), 0.0f, 1.0f);
	const float StaminaY = MarginY + BarHeight + 8.0f;
	DrawRect(FColor(30, 30, 30, 200), MarginX, StaminaY, BarWidth * 0.85f, BarHeight * 0.75f);
	DrawRect(StaminaBarColor, MarginX + 2.0f, StaminaY + 2.0f, (BarWidth * 0.85f - 4.0f) * StaminaPct, (BarHeight * 0.75f) - 4.0f);
	const FString StaminaText = FString::Printf(TEXT("SP: %.0f / %.0f"), PlayerChar->Attributes->CurrentStamina, PlayerChar->Attributes->MaxStamina);
	DrawText(StaminaText, FColor::White, MarginX + 8.0f, StaminaY + 1.0f, nullptr, 0.8f);
}

void AAstrawildHUD::DrawCenterCrosshair(AAstrawildCharacter* PlayerChar)
{
	const FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	const float CrosshairSize = 4.0f;
	const FColor ColorToUse = (PlayerChar && PlayerChar->bHasFocusedInteractable) ? CrosshairHighlightColor : CrosshairColor;

	DrawRect(ColorToUse, Center.X - CrosshairSize * 0.5f, Center.Y - CrosshairSize * 0.5f, CrosshairSize, CrosshairSize);
	DrawRect(ColorToUse, Center.X - 12.0f, Center.Y - 1.0f, 6.0f, 2.0f);
	DrawRect(ColorToUse, Center.X + 6.0f, Center.Y - 1.0f, 6.0f, 2.0f);
	DrawRect(ColorToUse, Center.X - 1.0f, Center.Y - 12.0f, 2.0f, 6.0f);
	DrawRect(ColorToUse, Center.X - 1.0f, Center.Y + 6.0f, 2.0f, 6.0f);
}

void AAstrawildHUD::DrawInteractionPrompt(AAstrawildCharacter* PlayerChar)
{
	if (!PlayerChar || !PlayerChar->bHasFocusedInteractable || PlayerChar->CachedInteractionPrompt.IsEmpty())
	{
		return;
	}

	const FString PromptString = PlayerChar->CachedInteractionPrompt.ToString();
	const float CenterX = Canvas->ClipX * 0.5f;
	const float PromptY = Canvas->ClipY * 0.65f;

	const float BoxWidth = FMath::Max(220.0f, (float)PromptString.Len() * 9.5f);
	const float BoxHeight = 36.0f;
	const float BoxX = CenterX - (BoxWidth * 0.5f);

	DrawRect(FColor(15, 20, 25, 220), BoxX, PromptY, BoxWidth, BoxHeight);
	DrawRect(FColor(243, 156, 18), BoxX, PromptY, 4.0f, BoxHeight);
	DrawText(PromptString, FColor::White, BoxX + 16.0f, PromptY + 10.0f, nullptr, 1.1f);
}

void AAstrawildHUD::DrawActiveCompanionBadge(AAstrawildCharacter* PlayerChar)
{
	if (!PlayerChar || !PlayerChar->Capture)
	{
		return;
	}

	const float PosX = 30.0f;
	const float PosY = Canvas->ClipY - 110.0f;

	// 1. Capture Feedback Banner
	if (!PlayerChar->Capture->LastCaptureFeedback.IsEmpty())
	{
		const FString FeedbackStr = PlayerChar->Capture->LastCaptureFeedback.ToString();
		const float BannerWidth = FMath::Max(320.0f, FeedbackStr.Len() * 9.0f);
		const float BannerX = (Canvas->ClipX - BannerWidth) * 0.5f;
		const float BannerY = 120.0f;

		DrawRect(FColor(15, 20, 25, 230), BannerX, BannerY, BannerWidth, 36.0f);
		DrawRect(FColor(241, 196, 15), BannerX, BannerY, BannerWidth, 3.0f);
		DrawText(FeedbackStr, FColor::White, BannerX + 16.0f, BannerY + 10.0f, nullptr, 0.95f);
	}

	// 2. Active Party Roster
	DrawRect(FColor(20, 25, 30, 210), PosX, PosY, 280.0f, 85.0f);
	DrawRect(FColor(52, 152, 219), PosX, PosY, 4.0f, 85.0f);

	if (PlayerChar->Capture->ActiveParty.Num() > 0 && PlayerChar->Capture->ActiveParty.IsValidIndex(PlayerChar->Capture->SelectedPartyIndex))
	{
		const FAstrawildCapturedEchoData& Echo = PlayerChar->Capture->ActiveParty[PlayerChar->Capture->SelectedPartyIndex];
		const FString EchoStatus = PlayerChar->Capture->ActiveSummonedEcho.IsValid() ? TEXT("[SUMMONED]") : TEXT("[READY]");
		const FString SpeciesName = Echo.SpeciesTag.ToString();
		const FString Line1 = FString::Printf(TEXT("Slot %d/%d: %s (Lv.%d) %s"),
			PlayerChar->Capture->SelectedPartyIndex + 1, PlayerChar->Capture->ActiveParty.Num(), *SpeciesName, Echo.Level, *EchoStatus);
		const FString Line2 = FString::Printf(TEXT("Trust: %.0f%% | HP: %.0f/%.0f | Element: %s"),
			Echo.TrustScore, Echo.CurrentHealth, Echo.MaxHealth, *UEnum::GetValueAsString(Echo.Element));
		const FString Line3 = TEXT("Press [T] Summon/Recall  [MWheel] Cycle Party");

		DrawText(Line1, FColor(241, 196, 15), PosX + 10.0f, PosY + 8.0f, nullptr, 0.9f);
		DrawText(Line2, FColor(46, 204, 113), PosX + 10.0f, PosY + 28.0f, nullptr, 0.85f);
		DrawText(Line3, FColor(189, 195, 199), PosX + 10.0f, PosY + 48.0f, nullptr, 0.75f);
	}
	else
	{
		DrawText(TEXT("No Echo in Party (Throw [Q] Resonator to capture)"), FColor(149, 165, 166), PosX + 10.0f, PosY + 20.0f, nullptr, 0.85f);
		DrawText(TEXT("Loot Monolith to obtain Astra Resonators!"), FColor(243, 156, 18), PosX + 10.0f, PosY + 45.0f, nullptr, 0.75f);
	}
}

void AAstrawildHUD::DrawInventoryAndCraftingMenu(AAstrawildCharacter* PlayerChar)
{
	if (!PlayerChar)
	{
		return;
	}

	const float MenuWidth = 720.0f;
	const float MenuHeight = 420.0f;
	const float PosX = (Canvas->ClipX - MenuWidth) * 0.5f;
	const float PosY = (Canvas->ClipY - MenuHeight) * 0.5f;

	// Background & Header
	DrawRect(FColor(15, 20, 25, 240), PosX, PosY, MenuWidth, MenuHeight);
	DrawRect(FColor(241, 196, 15), PosX, PosY, MenuWidth, 4.0f);
	DrawText(TEXT("=== SURVIVAL BAG & CRAFTING WORKSTATION [Press I to Close] ==="), FColor(241, 196, 15), PosX + 16.0f, PosY + 12.0f, nullptr, 0.95f);

	// 1. Left Column: Inventory
	const float LeftX = PosX + 20.0f;
	float LeftY = PosY + 45.0f;
	DrawText(TEXT("--- INVENTORY STORAGE ---"), FColor(52, 152, 219), LeftX, LeftY, nullptr, 0.85f);
	LeftY += 22.0f;

	if (PlayerChar->Inventory)
	{
		int32 Displayed = 0;
		for (int32 i = 0; i < PlayerChar->Inventory->Slots.Num(); ++i)
		{
			const FAstrawildItemSlot& Slot = PlayerChar->Inventory->Slots[i];
			if (Slot.IsValid())
			{
				Displayed++;
				const FString ItemLine = FString::Printf(TEXT("[%d] %s x%d"), Displayed, *Slot.ItemTag.ToString(), Slot.Quantity);
				DrawText(ItemLine, FColor::White, LeftX, LeftY, nullptr, 0.8f);
				LeftY += 18.0f;
				if (Displayed >= 15) break;
			}
		}

		if (Displayed == 0)
		{
			DrawText(TEXT("(Bag is empty. Chop trees or mine rocks!)"), FColor(149, 165, 166), LeftX, LeftY, nullptr, 0.8f);
		}
	}

	// 2. Right Column: Crafting & Building Station
	const float RightX = PosX + 370.0f;
	float RightY = PosY + 45.0f;
	DrawText(TEXT("--- CRAFTING & REST POINT BUILDING ---"), FColor(46, 204, 113), RightX, RightY, nullptr, 0.85f);
	RightY += 22.0f;

	const FString RecipesText[] = {
		TEXT("1. Primal Stone Axe (5 Sunwood, 3 LumenStone)"),
		TEXT("2. Primal Stone Pick (5 Sunwood, 3 LumenStone)"),
		TEXT("3. Astra Resonator T1 (1 Shard, 2 Stone, 3 Wood)"),
		TEXT("4. Astra Resonator T2 (3 Shard, 5 Stone)"),
		TEXT("5. Campfire / Rest Point (4 Sunwood, 2 LumenStone)"),
		TEXT("6. Rest Shelter Bed (6 Sunwood)")
	};

	for (int32 r = 0; r < 6; ++r)
	{
		DrawText(RecipesText[r], FColor(230, 230, 230), RightX, RightY, nullptr, 0.8f);
		RightY += 22.0f;
	}

	DrawText(TEXT("Use Console commands or Building wheel to construct!"), FColor(243, 156, 18), RightX, RightY + 20.0f, nullptr, 0.78f);
}

void AAstrawildHUD::DrawDebugOverlay(AAstrawildCharacter* PlayerChar)
{
	const float OverlayWidth = 360.0f;
	const float OverlayHeight = 180.0f;
	const float PosX = Canvas->ClipX - OverlayWidth - 20.0f;
	const float PosY = 20.0f;

	DrawRect(FColor(10, 15, 20, 220), PosX, PosY, OverlayWidth, OverlayHeight);
	DrawRect(FColor(46, 204, 113), PosX, PosY, OverlayWidth, 3.0f);

	const FString StateStr = UEnum::GetValueAsString(PlayerChar->GetCurrentMovementState());
	const FVector Vel = PlayerChar->GetVelocity();
	const float Speed = Vel.Size();
	const FVector Loc = PlayerChar->GetActorLocation();

	FString TargetName = TEXT("None");
	float TargetDist = 0.0f;
	if (PlayerChar->FocusedInteractableActor.IsValid())
	{
		TargetName = PlayerChar->FocusedInteractableActor->GetName();
		TargetDist = FVector::Dist(PlayerChar->GetActorLocation(), PlayerChar->FocusedInteractableActor->GetActorLocation());
	}

	const FString DebugTitle = TEXT("=== ASTRAWILD VERTICAL SLICE DEBUG ===");
	const FString DebugState = FString::Printf(TEXT("State: %s | Speed: %.1f cm/s"), *StateStr, Speed);
	const FString DebugLoc = FString::Printf(TEXT("Location: X=%.0f Y=%.0f Z=%.0f"), Loc.X, Loc.Y, Loc.Z);
	const FString DebugTarget = FString::Printf(TEXT("Focus Target: %s (Dist: %.0f cm)"), *TargetName, TargetDist);
	const FString DebugStamina = FString::Printf(TEXT("Stamina: %.1f / %.1f (Dodging: %d)"),
		PlayerChar->Attributes ? PlayerChar->Attributes->CurrentStamina : 0.0f,
		PlayerChar->Attributes ? PlayerChar->Attributes->MaxStamina : 0.0f,
		PlayerChar->bIsDodging);
	const FString DebugHelp = TEXT("Press [F1/Tab] Debug | [I] Inventory/Crafting | WASD/E/LMB/Q/T");

	float LineY = PosY + 10.0f;
	DrawText(DebugTitle, FColor(46, 204, 113), PosX + 12.0f, LineY, nullptr, 0.9f);
	LineY += 22.0f;
	DrawText(DebugState, FColor::White, PosX + 12.0f, LineY, nullptr, 0.85f);
	LineY += 20.0f;
	DrawText(DebugLoc, FColor(200, 200, 200), PosX + 12.0f, LineY, nullptr, 0.85f);
	LineY += 20.0f;
	DrawText(DebugTarget, FColor(241, 196, 15), PosX + 12.0f, LineY, nullptr, 0.85f);
	LineY += 20.0f;
	DrawText(DebugStamina, FColor::Cyan, PosX + 12.0f, LineY, nullptr, 0.85f);
	LineY += 24.0f;
	DrawText(DebugHelp, FColor(149, 165, 166), PosX + 12.0f, LineY, nullptr, 0.72f);
}

void AAstrawildHUD::DrawSaveStatusBanner()
{
	UGameInstance* GI = GetGameInstance();
	if (!GI)
	{
		return;
	}

	UAstrawildSaveSubsystem* SaveSys = GI->GetSubsystem<UAstrawildSaveSubsystem>();
	if (!SaveSys || SaveSys->LastSaveStatusBanner.IsEmpty())
	{
		return;
	}

	const FString StatusStr = SaveSys->LastSaveStatusBanner.ToString();
	const float BannerWidth = FMath::Max(320.0f, StatusStr.Len() * 9.5f);
	const float BannerHeight = 32.0f;
	const float PosX = (Canvas->ClipX - BannerWidth) * 0.5f;
	const float PosY = 40.0f;

	FColor AccentColor = FColor(46, 204, 113); // Green for success
	if (SaveSys->bIsCurrentlySaving || StatusStr.Contains(TEXT("Saving")))
	{
		AccentColor = FColor(241, 196, 15); // Yellow for saving
	}
	else if (StatusStr.Contains(TEXT("Failed")) || StatusStr.Contains(TEXT("Corrupt")) || StatusStr.Contains(TEXT("Error")))
	{
		AccentColor = FColor(231, 76, 60); // Red for failure
	}

	DrawRect(FColor(15, 20, 25, 230), PosX, PosY, BannerWidth, BannerHeight);
	DrawRect(AccentColor, PosX, PosY, BannerWidth, 3.0f);
	DrawText(StatusStr, FColor::White, PosX + 14.0f, PosY + 8.0f, nullptr, 0.9f);
}