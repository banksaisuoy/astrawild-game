// Copyright Epic Games, Inc. All Rights Reserved.

#include "Data/AstrawildItemDataAsset.h"

UAstrawildItemDataAsset::UAstrawildItemDataAsset()
	: Category(EAstrawildItemCategory::Resource)
	, MaxStackSize(99)
	, Weight(0.5f)
	, MaxDurability(100.0f)
	, PreferredHarvestType(EAstrawildHarvestType::Lumber)
	, HarvestEfficiencyPower(1.0f)
	, WeaponDamageBonus(0.0f)
{
}