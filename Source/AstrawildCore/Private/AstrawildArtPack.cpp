#include "AstrawildArtPack.h"

namespace AstrawildArtPack
{
	namespace Vfx
	{
		const TCHAR* MuzzleFlash = TEXT("/Game/VFX/NS_AW_MuzzleFlash");
		const TCHAR* ImpactBurst = TEXT("/Game/VFX/NS_AW_Weap_Impact");
		const TCHAR* ProjectileTrail = TEXT("/Game/VFX/NS_AW_Weap_Trail");
	}

	namespace Sfx
	{
		// DP-5: reuses the existing CP-06 energy impact binding verbatim
		// (no new /Game/ path — validator check 8 reads the same ref).
		const TCHAR* WeaknessHitImpact = TEXT("/Game/Audio/A_Weapon_Impact_Energy");
	}

	namespace Paths
	{
		// Meshes (mirrored from ArtSource/manifest.json ue_path values)
		static const TCHAR* WeaponScrapRifle = TEXT("/Game/Weapons/Meshes/SM_Weapon_ScrapRifle");
		static const TCHAR* WeaponPlasmaCarbine = TEXT("/Game/Weapons/Meshes/SM_Weapon_PlasmaCarbine");
		static const TCHAR* WeaponArcCannon = TEXT("/Game/Weapons/Meshes/SM_Weapon_ArcCannon");
		static const TCHAR* WeaponRailgun = TEXT("/Game/Weapons/Meshes/SM_Weapon_Railgun");
		static const TCHAR* WeaponSingularity = TEXT("/Game/Weapons/Meshes/SM_Weapon_SingularityCannon");
		static const TCHAR* VehicleDawnSkiff = TEXT("/Game/Vehicles/SM_Vehicle_DawnSkiff");

		static const TCHAR* TreeBroadleaf = TEXT("/Game/Environment/SM_Tree_Broadleaf");
		static const TCHAR* TreeConifer = TEXT("/Game/Environment/SM_Tree_Conifer");
		static const TCHAR* TreeSporeCanopy = TEXT("/Game/Environment/SM_Tree_SporeCanopy");
		static const TCHAR* RockGraniteL = TEXT("/Game/Environment/SM_Rock_Granite_L");
		static const TCHAR* RockGraniteM = TEXT("/Game/Environment/SM_Rock_Granite_M");
		static const TCHAR* RockGraniteS = TEXT("/Game/Environment/SM_Rock_Granite_S");
		static const TCHAR* RockBoulderMoss = TEXT("/Game/Environment/SM_Rock_Boulder_Moss");
		static const TCHAR* CliffShard = TEXT("/Game/Environment/SM_Cliff_Shard");
		static const TCHAR* GrassTuft = TEXT("/Game/Environment/SM_Grass_Tuft");
		static const TCHAR* Fern = TEXT("/Game/Environment/SM_Fern");
		static const TCHAR* SporeBush = TEXT("/Game/Environment/SM_SporeBush");
		static const TCHAR* GlowReed = TEXT("/Game/Environment/SM_GlowReed");

		static const TCHAR* NodeAstraite = TEXT("/Game/Environment/ResourceNodes/SM_Node_Astraite");
		static const TCHAR* NodePyronite = TEXT("/Game/Environment/ResourceNodes/SM_Node_Pyronite");
		static const TCHAR* NodeVoidstone = TEXT("/Game/Environment/ResourceNodes/SM_Node_Voidstone");
		static const TCHAR* NodeAncientVein = TEXT("/Game/Environment/ResourceNodes/SM_Node_AncientVein");

		// Materials / audio
		static const TCHAR* LandscapeMaterial = TEXT("/Game/Materials/M_Landscape_SciFiFrontier");
		static const TCHAR* AmbWind = TEXT("/Game/Audio/A_Amb_Wind_Gentle");
		static const TCHAR* AmbForest = TEXT("/Game/Audio/A_Amb_Forest_Dawn");
		static const TCHAR* AmbMarsh = TEXT("/Game/Audio/A_Amb_Marsh_Dusk");
		static const TCHAR* AmbNight = TEXT("/Game/Audio/A_Amb_Night_Crystal");
		static const TCHAR* AmbWater = TEXT("/Game/Audio/A_Amb_Water_Lake");
		static const TCHAR* FireScrap = TEXT("/Game/Audio/A_Weapon_Scrap_Fire");
		static const TCHAR* FirePlasma = TEXT("/Game/Audio/A_Weapon_Plasma_Fire");
		static const TCHAR* FireArc = TEXT("/Game/Audio/A_Weapon_Arc_Fire");
		static const TCHAR* FireRail = TEXT("/Game/Audio/A_Weapon_Rail_Fire");
		static const TCHAR* FireSingularity = TEXT("/Game/Audio/A_Weapon_Singularity_Fire");
		static const TCHAR* ImpactKinetic = TEXT("/Game/Audio/A_Weapon_Impact_Kinetic");
		static const TCHAR* ImpactEnergy = TEXT("/Game/Audio/A_Weapon_Impact_Energy");
	}

	const TArray<FWeaponArt>& GetWeaponArt()
	{
		static const TArray<FWeaponArt> Table =
		{
			{ TEXT("Weapon_Scrapshot"), Paths::WeaponScrapRifle, Vfx::MuzzleFlash, Vfx::ImpactBurst, Vfx::ProjectileTrail, Paths::FireScrap, Paths::ImpactKinetic },
			{ TEXT("Weapon_PulseLance"), Paths::WeaponPlasmaCarbine, Vfx::MuzzleFlash, Vfx::ImpactBurst, Vfx::ProjectileTrail, Paths::FirePlasma, Paths::ImpactEnergy },
			{ TEXT("Weapon_PlasmaCharger"), Paths::WeaponPlasmaCarbine, Vfx::MuzzleFlash, Vfx::ImpactBurst, Vfx::ProjectileTrail, Paths::FirePlasma, Paths::ImpactEnergy },
			{ TEXT("Weapon_LumenBeam"), Paths::WeaponPlasmaCarbine, Vfx::MuzzleFlash, Vfx::ImpactBurst, TEXT(""), Paths::FirePlasma, Paths::ImpactEnergy },
			{ TEXT("Weapon_ArcCaster"), Paths::WeaponArcCannon, Vfx::MuzzleFlash, Vfx::ImpactBurst, TEXT(""), Paths::FireArc, Paths::ImpactEnergy },
			{ TEXT("Weapon_MagrailDriver"), Paths::WeaponRailgun, Vfx::MuzzleFlash, Vfx::ImpactBurst, TEXT(""), Paths::FireRail, Paths::ImpactEnergy },
			{ TEXT("Weapon_SkysingerLauncher"), Paths::WeaponSingularity, Vfx::MuzzleFlash, Vfx::ImpactBurst, Vfx::ProjectileTrail, Paths::FireSingularity, Paths::ImpactEnergy },
			{ TEXT("Weapon_StarlancePrototype"), Paths::WeaponSingularity, Vfx::MuzzleFlash, Vfx::ImpactBurst, Vfx::ProjectileTrail, Paths::FireSingularity, Paths::ImpactEnergy },
		};
		return Table;
	}

	const TArray<FEchoArt>& GetEchoArt()
	{
		static const TArray<FEchoArt> Table =
		{
			{ TEXT("Echo_Terraquill"), TEXT("/Game/Characters/Echoes/SK_Echo_Terraquill"), TEXT("/Game/Characters/Echoes/AM_Terraquill_Idle"), TEXT("/Game/Characters/Echoes/AM_Terraquill_Move") },
			{ TEXT("Echo_Cindermule"), TEXT("/Game/Characters/Echoes/SK_Echo_Cindermule"), TEXT("/Game/Characters/Echoes/AM_Cindermule_Idle"), TEXT("/Game/Characters/Echoes/AM_Cindermule_Move") },
			{ TEXT("Echo_Voltpylon"), TEXT("/Game/Characters/Echoes/SK_Echo_Voltpylon"), TEXT("/Game/Characters/Echoes/AM_Voltpylon_Idle"), TEXT("/Game/Characters/Echoes/AM_Voltpylon_Move") },
			{ TEXT("Echo_Bastionbeetle"), TEXT("/Game/Characters/Echoes/SK_Echo_Bastionbeetle"), TEXT("/Game/Characters/Echoes/AM_Bastionbeetle_Idle"), TEXT("/Game/Characters/Echoes/AM_Bastionbeetle_Move") },
			{ TEXT("Echo_Mistmender"), TEXT("/Game/Characters/Echoes/SK_Echo_Mistmender"), TEXT("/Game/Characters/Echoes/AM_Mistmender_Idle"), TEXT("/Game/Characters/Echoes/AM_Mistmender_Move") },
			{ TEXT("Echo_Deepdelver"), TEXT("/Game/Characters/Echoes/SK_Echo_Deepdelver"), TEXT("/Game/Characters/Echoes/AM_Deepdelver_Idle"), TEXT("/Game/Characters/Echoes/AM_Deepdelver_Move") },
		};
		return Table;
	}

	const TArray<FBiomeArt>& GetBiomeArt()
	{
		static const TArray<FBiomeArt> Table =
		{
			{ TEXT("Zone_DawnFields"), { Paths::TreeBroadleaf, Paths::TreeConifer }, { Paths::RockGraniteL, Paths::RockGraniteM }, { Paths::GrassTuft, Paths::Fern }, Paths::LandscapeMaterial, Paths::AmbForest },
			{ TEXT("Zone_DuskMarsh"), { Paths::TreeSporeCanopy }, { Paths::RockGraniteS }, { Paths::GlowReed, Paths::SporeBush }, Paths::LandscapeMaterial, Paths::AmbMarsh },
			{ TEXT("Zone_EmberRidge"), { Paths::TreeConifer }, { Paths::CliffShard, Paths::RockGraniteM }, { Paths::GrassTuft }, Paths::LandscapeMaterial, Paths::AmbWind },
			{ TEXT("Zone_FrostveilExpanse"), { Paths::TreeConifer }, { Paths::RockGraniteL }, { }, Paths::LandscapeMaterial, Paths::AmbWind },
			{ TEXT("Zone_Glimmerwood"), { Paths::TreeSporeCanopy, Paths::TreeBroadleaf }, { Paths::RockGraniteS }, { Paths::Fern, Paths::GlowReed }, Paths::LandscapeMaterial, Paths::AmbNight },
			{ TEXT("Zone_HollowApproach"), { }, { Paths::CliffShard, Paths::RockGraniteM }, { Paths::SporeBush }, Paths::LandscapeMaterial, Paths::AmbNight },
			{ TEXT("Zone_AzureShallows"), { }, { Paths::RockGraniteS }, { Paths::GlowReed, Paths::GrassTuft }, Paths::LandscapeMaterial, Paths::AmbWater },
			{ TEXT("Zone_TidebreakerIsles"), { Paths::TreeConifer }, { Paths::RockGraniteM, Paths::CliffShard }, { Paths::GlowReed }, Paths::LandscapeMaterial, Paths::AmbWater },
			{ TEXT("Zone_SunscarDesert"), { }, { Paths::CliffShard, Paths::RockGraniteM }, { }, Paths::LandscapeMaterial, Paths::AmbWind },
			{ TEXT("Zone_StormcrestHighlands"), { Paths::TreeConifer }, { Paths::CliffShard, Paths::RockGraniteL }, { Paths::GrassTuft }, Paths::LandscapeMaterial, Paths::AmbWind },
			{ TEXT("Zone_VerdantReach"), { Paths::TreeBroadleaf, Paths::TreeConifer }, { Paths::RockBoulderMoss, Paths::RockGraniteM }, { Paths::GrassTuft, Paths::Fern, Paths::SporeBush }, Paths::LandscapeMaterial, Paths::AmbForest },
			{ TEXT("Zone_PearlseaReef"), { }, { Paths::RockGraniteS }, { Paths::GlowReed }, Paths::LandscapeMaterial, Paths::AmbWater },
		};
		return Table;
	}

	const TArray<FNodeArt>& GetNodeArt()
	{
		static const TArray<FNodeArt> Table =
		{
			{ TEXT("Node_Dawnwood"), Paths::RockGraniteS },
			{ TEXT("Node_Fieldstone"), Paths::RockGraniteM },
			{ TEXT("Node_Sunfiber"), Paths::SporeBush },
			{ TEXT("Node_DawnCrystal"), Paths::NodeAstraite },
			{ TEXT("Node_EmberAsh"), Paths::NodePyronite },
			{ TEXT("Node_SeaPearl"), Paths::NodeAstraite },
			{ TEXT("Node_CoralShard"), Paths::NodeAstraite },
			{ TEXT("Node_DuneGlass"), Paths::NodeVoidstone },
			{ TEXT("Node_StormSilver"), Paths::NodeVoidstone },
			{ TEXT("Node_AncientVein"), Paths::NodeAncientVein },
		};
		return Table;
	}

	const FSurvivorArt& GetSurvivorArt()
	{
		static const FSurvivorArt Art =
		{
			TEXT("/Game/Characters/Survivor/SK_Survivor_Exosuit"),
			TEXT("/Game/Characters/Survivor/AM_Survivor_Idle"),
			TEXT("/Game/Characters/Survivor/AM_Survivor_Walk"),
			TEXT("/Game/Characters/Survivor/AM_Survivor_Run"),
			TEXT("/Game/Characters/Survivor/AM_Survivor_Jump"),
			TEXT("/Game/Characters/Survivor/AM_Survivor_Aim"),
			TEXT("/Game/Characters/Survivor/AM_Survivor_Fire"),
			TEXT("/Game/Characters/Survivor/AM_Survivor_Gather"),
		};
		return Art;
	}

	const FWeaponArt* FindWeaponArt(const FName& WeaponId)
	{
		for (const FWeaponArt& Entry : GetWeaponArt())
		{
			if (Entry.WeaponId == WeaponId)
			{
				return &Entry;
			}
		}
		return nullptr;
	}

	const FEchoArt* FindEchoArt(const FName& EchoId)
	{
		for (const FEchoArt& Entry : GetEchoArt())
		{
			if (Entry.EchoId == EchoId)
			{
				return &Entry;
			}
		}
		return nullptr;
	}

	const FBiomeArt* FindBiomeArt(const FName& BiomeId)
	{
		for (const FBiomeArt& Entry : GetBiomeArt())
		{
			if (Entry.BiomeId == BiomeId)
			{
				return &Entry;
			}
		}
		return nullptr;
	}

	const FNodeArt* FindNodeArt(const FName& NodeId)
	{
		for (const FNodeArt& Entry : GetNodeArt())
		{
			if (Entry.NodeId == NodeId)
			{
				return &Entry;
			}
		}
		return nullptr;
	}
}

namespace AstrawildArtPack
{

// ---------------------------------------------------------------------------
// PCR-4/PCR-5 — Tier-B archetype mesh library (strategy §6)
// ---------------------------------------------------------------------------

namespace
{
	// The 39 Tier-B species (baked by Tools/ArtSourceGen/gen_tier_b.py from the
	// ACTUAL source tables: zone-wildlife spawn rows + dungeon creature pools +
	// event species boosts + Huge size + the monolith/colossus name family,
	// minus the 14 Tier-A bespoke species). Mirrors the ArtSource manifest —
	// ASTRAWILD.PCR4.TierBLibrary + the static validator verify every entry.
	const TArray<FName>& GetTierBSpeciesTable()
	{
		static const TArray<FName> Table =
		{
			TEXT("Echo_Abyssjelly"), TEXT("Echo_Astralmonolith"), TEXT("Echo_Brinefin"),
			TEXT("Echo_Coralray"), TEXT("Echo_Duskmoth"), TEXT("Echo_Eldermonolith"),
			TEXT("Echo_Emberfang"), TEXT("Echo_Embershade"), TEXT("Echo_Fernthorn"),
			TEXT("Echo_Forgottencolossus"), TEXT("Echo_Frostblaze"), TEXT("Echo_Geargolem"),
			TEXT("Echo_Ghostshade"), TEXT("Echo_Glimmerhornet"), TEXT("Echo_Hallowedcolossus"),
			TEXT("Echo_Lagoonfin"), TEXT("Echo_Magmawing"), TEXT("Echo_Mistwing"),
			TEXT("Echo_Monolithcolossus"), TEXT("Echo_Monolithprimarch"), TEXT("Echo_Pearlcrest"),
			TEXT("Echo_Pistongolem"), TEXT("Echo_Primemonolith"), TEXT("Echo_Pyreblaze"),
			TEXT("Echo_Reliccolossus"), TEXT("Echo_Rimefang"), TEXT("Echo_Saltcrest"),
			TEXT("Echo_Saltray"), TEXT("Echo_Stonehide"), TEXT("Echo_Sunhide"),
			TEXT("Echo_Sunhorn"), TEXT("Echo_Sunpaw"), TEXT("Echo_Tidewyrm"),
			TEXT("Echo_Undertowray"), TEXT("Echo_Verdantbloom"), TEXT("Echo_Vespermonolith"),
			TEXT("Echo_Voidwing"), TEXT("Echo_Voltmaw"), TEXT("Echo_Wavecrest"),
		};
		return Table;
	}
}

const TArray<FName>& GetTierBSpeciesIds()
{
	return GetTierBSpeciesTable();
}

FString BuildTierBMechPath(const FName& EchoId)
{
	// Convention path — derived, never a literal (validator check 8 unaffected).
	// "/Game/Characters/Echoes/SK_Echo_<Name>" resolves only after the engine
	// import pass lands the .uassets; until then the PMC body stays (opt-in).
	const FString Species = EchoId.ToString().StartsWith(TEXT("Echo_"))
		? EchoId.ToString().RightChop(5)
		: EchoId.ToString();
	return FString::Printf(TEXT("/Game/Characters/Echoes/SK_Echo_%s"), *Species);
}

FString BuildTierBAnimPath(const FName& EchoId, const bool bMoveClip)
{
	const FString Species = EchoId.ToString().StartsWith(TEXT("Echo_"))
		? EchoId.ToString().RightChop(5)
		: EchoId.ToString();
	return FString::Printf(TEXT("/Game/Characters/Echoes/AM_%s_%s"), *Species, bMoveClip ? TEXT("Move") : TEXT("Idle"));
}

bool IsTierBSpecies(const FName& EchoId)
{
	return GetTierBSpeciesTable().Contains(EchoId);
}

} // namespace AstrawildArtPack
