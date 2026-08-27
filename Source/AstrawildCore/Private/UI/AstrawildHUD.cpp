// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/AstrawildHUD.h"
#include "Characters/AstrawildCharacter.h"
#include "Components/AstrawildAttributeComponent.h"
#include "Components/AstrawildCaptureComponent.h"
#include "Components/AstrawildInventoryComponent.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "GameFramework/CharacterMovementComponent.h"

AAstrawildHUD::AAstrawildHUD()
	: bShowHUD(true)
	, bShowDebugOverlay(true) // Enabled by default for vertical slice prototype testing
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

	if (bShowDebugOverlay)
	{
		DrawDebugOverlay(PlayerChar);
	}
}

void AAstrawildHUD::ToggleDebugOverlay()
{
	bShowDebugOverlay = !bShowDebugOverlay;
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
	
	// Background
	DrawRect(FColor(30, 30, 30, 200), MarginX, MarginY, BarWidth, BarHeight);
	// Fill
	DrawRect(HealthBarColor, MarginX + 2.0f, MarginY + 2.0f, (BarWidth - 4.0f) * HealthPct, BarHeight - 4.0f);
	// Text
	const FString HealthText = FString::Printf(TEXT("HP: %.0f / %.0f"), PlayerChar->Attributes->CurrentHealth, PlayerChar->Attributes->MaxHealth);
	DrawText(HealthText, FColor::White, MarginX + 8.0f, MarginY + 2.0f, nullptr, 0.9f);

	// 2. Stamina Bar
	const float StaminaPct = FMath::Clamp(PlayerChar->Attributes->GetStaminaPercent(), 0.0f, 1.0f);
	const float StaminaY = MarginY + BarHeight + 8.0f;

	// Background
	DrawRect(FColor(30, 30, 30, 200), MarginX, StaminaY, BarWidth * 0.85f, BarHeight * 0.75f);
	// Fill
	DrawRect(StaminaBarColor, MarginX + 2.0f, StaminaY + 2.0f, (BarWidth * 0.85f - 4.0f) * StaminaPct, (BarHeight * 0.75f) - 4.0f);
	// Text
	const FString StaminaText = FString::Printf(TEXT("SP: %.0f / %.0f"), PlayerChar->Attributes->CurrentStamina, PlayerChar->Attributes->MaxStamina);
	DrawText(StaminaText, FColor::White, MarginX + 8.0f, StaminaY + 1.0f, nullptr, 0.8f);
}

void AAstrawildHUD::DrawCenterCrosshair(AAstrawildCharacter* PlayerChar)
{
	const FVector2D Center(Canvas->ClipX * 0.5f, Canvas->ClipY * 0.5f);
	const float CrosshairSize = 4.0f;

	const FColor ColorToUse = (PlayerChar && PlayerChar->bHasFocusedInteractable) ? CrosshairHighlightColor : CrosshairColor;

	// Center dot & subtle reticle
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

	// Estimate text width
	const float BoxWidth = FMath::Max(220.0f, (float)PromptString.Len() * 9.5f);
	const float BoxHeight = 36.0f;
	const float BoxX = CenterX - (BoxWidth * 0.5f);

	// Semi-transparent stylish badge background
	DrawRect(FColor(15, 20, 25, 220), BoxX, PromptY, BoxWidth, BoxHeight);
	DrawRect(FColor(243, 156, 18), BoxX, PromptY, 4.0f, BoxHeight); // Left accent bar

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

	// 1. Capture Feedback Banner (if active)
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
	DrawRect(FColor(52, 152, 219), PosX, PosY, 4.0f, 85.0f); // Blue accent

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

void AAstrawildHUD::DrawDebugOverlay(AAstrawildCharacter* PlayerChar)
{
	const float OverlayWidth = 360.0f;
	const float OverlayHeight = 180.0f;
	const float PosX = Canvas->ClipX - OverlayWidth - 20.0f;
	const float PosY = 20.0f;

	// Background
	DrawRect(FColor(10, 15, 20, 220), PosX, PosY, OverlayWidth, OverlayHeight);
	DrawRect(FColor(46, 204, 113), PosX, PosY, OverlayWidth, 3.0f); // Top accent

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
	const FString DebugHelp = TEXT("Press [F1] or [Tab] to toggle overlay | Controls: WASD/Shift/Space/Alt/E/LMB/Q/T");

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