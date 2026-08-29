// Copyright Epic Games, Inc. All Rights Reserved.

// Force-linker to keep these reflected structs registered in the DLL.
// Without explicit references in a compiled translation unit, the linker
// may strip the .gen.cpp registration code for the AstrawildWorldData /
// AstrawildUnderwaterData / DataTable row structs and `unreal.AstrawildXxx`
// attribute lookups will return None in the Python commandlet.
//
// This file is intentionally tiny — it just touches the struct types so
// the linker keeps the registration symbols.

#include "World/AstrawildWorldData.h"
#include "World/AstrawildUnderwaterData.h"
#include "Data/AstrawildBossData.h"
#include "Data/AstrawildBreedingData.h"
#include "Data/AstrawildBreedingFusionData.h"
#include "Data/AstrawildCampaignData.h"
#include "Data/AstrawildCookingData.h"
#include "Data/AstrawildCraftingData.h"
#include "Data/AstrawildDungeonData.h"
#include "Data/AstrawildDyeData.h"
#include "Data/AstrawildEchoDexRow.h"
#include "Data/AstrawildEcosystemData.h"
#include "Data/AstrawildEvolutionData.h"
#include "Data/AstrawildFishingData.h"
#include "Data/AstrawildFoliageData.h"
#include "Data/AstrawildMasterEchoData.h"
#include "Data/AstrawildMechaAnimationData.h"
#include "Data/AstrawildMechaData.h"
#include "Data/AstrawildMechaVFXData.h"
#include "Data/AstrawildMountData.h"
#include "Data/AstrawildPlayerProgressionData.h"
#include "Data/AstrawildPowerGridData.h"
#include "Data/AstrawildRangedWeaponData.h"
#include "Data/AstrawildTechnologyData.h"
#include "Data/AstrawildVehicleData.h"
#include "Data/AstrawildWeatherData.h"
#include "Data/AstrawildWorldEventData.h"
#include "Data/AstrawildWorldKaijuBossData.h"

namespace AstrawildDataStructRegistry
{
	// Static instance references — the linker must keep these.
	static FAstrawildBiomeDefinition BiomeAnchor;
	static FAstrawildUnderwaterZoneRow UnderwaterAnchor;
	static FAstrawildBossAttackRow BossAttackAnchor;
	static FAstrawildBossEncounterRow BossEncounterAnchor;
	static FAstrawildBreedingGroupRow BreedingGroupAnchor;
	static FAstrawildBreedingFusionRow BreedingFusionAnchor;
	static FAstrawildCampaignChapterRow CampaignAnchor;
	static FAstrawildCookingRecipeRow CookingAnchor;
	static FAstrawildCraftingRecipeRow CraftingAnchor;
	static FAstrawildCyberneticEvolutionRow CyberneticAnchor;
	static FAstrawildDungeonRow DungeonAnchor;
	static FAstrawildDyeRow DyeAnchor;
	static FAstrawildEchoDexRow EchoDexAnchor;
	static FAstrawildEchoTraitRow EchoTraitAnchor;
	static FAstrawildEcosystemBehaviorRow EcosystemAnchor;
	static FAstrawildEvolutionRow EvolutionAnchor;
	static FAstrawildFastTravelSpire FastTravelAnchor;
	static FAstrawildFishRow FishAnchor;
	static FAstrawildFoliageRuleRow FoliageAnchor;
	static FAstrawildMasterEchoRow MasterEchoAnchor;
	static FAstrawildMechaAnimationProfileRow MechaAnimAnchor;
	static FAstrawildMechaFrameRow MechaFrameAnchor;
	static FAstrawildMechaVFXBindingRow MechaVfxAnchor;
	static FAstrawildMechaWeaponRow MechaWeaponAnchor;
	static FAstrawildMountProfile MountAnchor;
	static FAstrawildPlayerPerkRow PlayerPerkAnchor;
	static FAstrawildPowerGridNodeRow PowerGridAnchor;
	static FAstrawildRangedWeaponRow RangedWeaponAnchor;
	static FAstrawildTechnologyNodeRow TechnologyAnchor;
	static FAstrawildVehicleRow VehicleAnchor;
	static FAstrawildVehiclePartRow VehiclePartAnchor;
	static FAstrawildWeatherRow WeatherAnchor;
	static FAstrawildWorldCell WorldCellAnchor;
	static FAstrawildWorldEventRow WorldEventAnchor;
	static FAstrawildWorldKaijuBossRow WorldKaijuAnchor;
	static FAstrawildWorldSpawnRule WorldSpawnAnchor;
}
