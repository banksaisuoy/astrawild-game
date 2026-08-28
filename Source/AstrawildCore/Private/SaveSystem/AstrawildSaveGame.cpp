// Copyright Epic Games, Inc. All Rights Reserved.

#include "SaveSystem/AstrawildSaveGame.h"
#include "AstrawildLogChannels.h"

UAstrawildSaveGame::UAstrawildSaveGame()
	: SchemaVersion(1)
	, SaveSlotName(TEXT("Slot_01"))
	, SaveTimestamp(FDateTime::Now())
{
}

bool UAstrawildSaveGame::ValidateAndSanitize()
{
	// 1. Sanitize Player Profile
	PlayerProfile.CurrentHealth = FMath::Clamp(PlayerProfile.CurrentHealth, 1.0f, PlayerProfile.MaxHealth);
	PlayerProfile.CurrentStamina = FMath::Clamp(PlayerProfile.CurrentStamina, 0.0f, PlayerProfile.MaxStamina);
	PlayerProfile.PlayerLevel = FMath::Max(1, PlayerProfile.PlayerLevel);

	// Sanitize inventory slot counts and negative values
	for (FAstrawildItemSlot& Slot : PlayerProfile.InventorySlots)
	{
		if (Slot.Quantity < 0)
		{
			Slot.Clear();
		}
		else if (Slot.Quantity > 999)
		{
			Slot.Quantity = 999;
		}
	}
	PlayerProfile.ResearchPoints = FMath::Max(0, PlayerProfile.ResearchPoints);
	PlayerProfile.UnlockedTechnologyTags.RemoveAll([](const FGameplayTag& Tag)
	{
		return !Tag.IsValid();
	});

	for (int32 i = PlayerProfile.TrackedFood.Num() - 1; i >= 0; --i)
	{
		FAstrawildFoodSaveState& Food = PlayerProfile.TrackedFood[i];
		if (!Food.FoodItemTag.IsValid() || Food.RemainingQuantity <= 0)
		{
			PlayerProfile.TrackedFood.RemoveAt(i);
			continue;
		}
		if (!Food.ItemInstanceId.IsValid())
		{
			Food.ItemInstanceId = FGuid::NewGuid();
		}
		Food.RemainingQuantity = FMath::Clamp(Food.RemainingQuantity, 1, 999);
		Food.RemainingFreshnessSeconds = FMath::Max(0.0f, Food.RemainingFreshnessSeconds);
	}

	for (int32 i = PlayerProfile.ActiveFoodBuffs.Num() - 1; i >= 0; --i)
	{
		FAstrawildFoodBuffSaveState& Buff = PlayerProfile.ActiveFoodBuffs[i];
		if (!Buff.BuffTag.IsValid() || Buff.RemainingDurationSeconds <= 0.0f)
		{
			PlayerProfile.ActiveFoodBuffs.RemoveAt(i);
			continue;
		}
		Buff.RemainingDurationSeconds = FMath::Max(0.0f, Buff.RemainingDurationSeconds);
	}

	// 2. Sanitize Echoes
	for (FAstrawildCapturedEchoData& Echo : PlayerProfile.ActiveParty)
	{
		Echo.CurrentHealth = FMath::Clamp(Echo.CurrentHealth, 1.0f, Echo.MaxHealth);
		Echo.TrustScore = FMath::Clamp(Echo.TrustScore, 0.0f, 100.0f);
		Echo.Level = FMath::Max(1, Echo.Level);
	}

	for (FAstrawildCapturedEchoData& Echo : PlayerProfile.ReserveStorage)
	{
		Echo.CurrentHealth = FMath::Clamp(Echo.CurrentHealth, 1.0f, Echo.MaxHealth);
		Echo.TrustScore = FMath::Clamp(Echo.TrustScore, 0.0f, 100.0f);
		Echo.Level = FMath::Max(1, Echo.Level);
	}

	for (FAstrawildEchoEggData& Egg : WorldSnapshot.IncubatingEggs)
	{
		if (!Egg.EggId.IsValid())
		{
			Egg.EggId = FGuid::NewGuid();
		}
		Egg.IncubationProgress = FMath::Clamp(Egg.IncubationProgress, 0.0f, 1.0f);
		Egg.IncubationDurationSeconds = FMath::Max(1.0f, Egg.IncubationDurationSeconds);
		Egg.Generation = FMath::Max(1, Egg.Generation);
	}
	WorldSnapshot.DiscoveredSpireIds.RemoveAll([](const FName& SpireId)
	{
		return SpireId.IsNone();
	});

	// 3. Sanitize World Buildings
	for (int32 i = WorldSnapshot.PlacedBuildings.Num() - 1; i >= 0; --i)
	{
		if (WorldSnapshot.PlacedBuildings[i].CurrentHealth <= 0.0f)
		{
			WorldSnapshot.PlacedBuildings.RemoveAt(i);
		}
	}

	UE_LOG(LogAstrawildSave, Log, TEXT("Save data validated and sanitized successfully."));
	return true;
}

bool UAstrawildSaveGame::MigrateSchema(int32 TargetVersion)
{
	if (SchemaVersion >= TargetVersion)
	{
		return true;
	}

	UE_LOG(LogAstrawildSave, Log, TEXT("Migrating Save Data from Schema v%d to v%d..."), SchemaVersion, TargetVersion);

	// Schema v0 -> v1 Migration example
	if (SchemaVersion == 0)
	{
		PlayerProfile.SchemaVersion = 1;
		WorldSnapshot.SchemaVersion = 1;
		SettingsProfile.SchemaVersion = 1;
		SchemaVersion = 1;
	}

	return true;
}