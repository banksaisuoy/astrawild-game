#!/usr/bin/env python3
"""Validate ASTRAWILD text content contracts without requiring Unreal Editor."""
from __future__ import annotations

import csv
import pathlib
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
REQUIRED_CSV = {
    ROOT / "Content/Astrawild/Data/Source/DT_Lore.csv": {"Name", "LoreId", "Title", "Body", "RegionTag", "SortOrder", "bUnlockedByDefault"},
    ROOT / "Content/Astrawild/Data/Source/DT_Quests.csv": {"Name", "QuestId", "Title", "Description", "RegionTag", "PrerequisiteQuestTag", "bMainQuest"},
    ROOT / "Content/Astrawild/Data/Source/DT_QuestObjectives.csv": {"Name", "QuestId", "ObjectiveId", "Type", "TargetTag", "RequiredQuantity", "Description"},
    ROOT / "Content/Astrawild/Data/Source/DT_Biomes.csv": {"Name", "BiomeId", "Biome", "DisplayName", "MinLevel", "MaxLevel", "TemperatureLevel", "DominantElements", "ResourceTags"},

    ROOT / "Content/Astrawild/Data/Source/DT_FoliageRules.csv": {"Name", "FoliageRuleId", "BiomeId", "RuleKind", "FoliageAssetId", "ResourceTag", "DensityScale", "MinSlopeDegrees", "MaxSlopeDegrees", "MinHeightMeters", "MaxHeightMeters", "WindResponse", "bRespondsToCharacters", "bUseNanite"},
    ROOT / "Content/Astrawild/Data/Source/DT_BossAttacks.csv": {"Name", "AttackId", "EncounterId", "DisplayName", "PhaseIndex", "SpeciesAbilityIndex", "Element", "TelegraphDurationSeconds", "TelegraphRadius", "CooldownSeconds", "DamageMultiplier", "bIsUltimate"},
    ROOT / "Content/Astrawild/Data/Source/DT_BossEncounters.csv": {"Name", "EncounterId", "DungeonId", "BossSpeciesTag", "PrimaryElement", "RecommendedLevel", "MaxHealth", "PhaseTwoHealthThreshold", "PhaseThreeHealthThreshold", "PhaseCount", "IntroDurationSeconds", "EncounterTimeLimitSeconds", "bLockArena", "MaxParticipants"},
    ROOT / "Content/Astrawild/Data/Source/DT_SpawnRules.csv": {"Name", "SpawnRuleId", "SpeciesTag", "BiomeId", "MinLevel", "MaxLevel", "Weight", "MaxActive"},
    ROOT / "Content/Astrawild/Data/Source/DT_FastTravelSpires.csv": {"Name", "SpireId", "DisplayName", "BiomeId", "QuestTargetTag", "WorldTransform", "bUnlockedByDefault"},
    ROOT / "Content/Astrawild/Data/Source/DT_EchoDex.csv": {"Name", "SpeciesTag", "SpeciesName", "SpeciesTitle", "LoreDescription", "PrimaryElement", "ElementalAffinities", "Role", "BaseMaxHealth", "BaseAttackPower", "BaseDefensePower", "BaseWalkSpeed", "BaseRunSpeed", "CaptureDifficultyModifier", "PassiveTraitTags", "WorkSuitabilityTags", "PartnerSkillTag", "MountProfileId", "bCanBeMounted", "BreedingGroupId", "EvolutionTargetId", "EvolutionLevel", "DexOrder"},
    ROOT / "Content/Astrawild/Data/Source/DT_EchoDex_200.csv": {"Name", "DexOrder", "SpeciesTag", "SpeciesName", "SpeciesTitle", "AnatomyConcept", "Diet", "SocialBehavior", "Temperament", "HabitatBiomeTag", "ActivityCycleTag", "PrimaryElement", "ElementalAffinities", "Role", "BaseMaxHealth", "BaseAttackPower", "BaseDefensePower", "BaseStamina", "BaseWalkSpeed", "BaseRunSpeed", "CaptureDifficultyModifier", "WorkSuitabilityLevels", "WorkSuitabilityTags", "PassiveTraitTags", "ActiveSkillTags", "ActiveSkillElementTags", "ActiveSkillCooldowns", "ActiveSkillDamageMultipliers", "ActiveSkillTelegraphs", "PartnerSkillTag", "MountedWeaponTag", "DropItemTags", "DropItemQuantities", "ParentSpeciesA", "ParentSpeciesB"},
    ROOT / "Content/Astrawild/Data/Source/DT_MountProfiles.csv": {"Name", "MountProfileId", "SaddleSocketName", "SpeedMultiplier", "StaminaCostPerSecond", "JumpMultiplier", "bAllowsCombatFromMount", "MountFamilyTag"},
    ROOT / "Content/Astrawild/Data/Source/DT_BreedingGroups.csv": {"Name", "BreedingGroupId", "CompatibleSpeciesTags", "IncubationDurationSeconds", "MutationChance", "MaxInheritedTraits"},
    ROOT / "Content/Astrawild/Data/Source/DT_BreedingFusions.csv": {"Name", "ParentSpeciesA", "ParentSpeciesB", "OffspringSpeciesTag", "OffspringElementalAffinities", "GuaranteedInheritedTraitTags", "TraitInheritanceChance", "HiddenPassiveUnlockChance", "FusionGroupTag"},
    ROOT / "Content/Astrawild/Data/Source/DT_PowerGrid.csv": {"Name", "NodeTag", "DisplayName", "NodeType", "GenerationWatts", "StorageCapacityWattHours", "ConsumptionWatts", "Priority", "RequiredTechnologyTag", "RequiredBiomeTag", "bGeneratesOnlyDuringDay", "bRequiresVolcanicVent", "bStopsFoodSpoilage", "CraftSpeedMultiplier"},
    ROOT / "Content/Astrawild/Data/Source/DT_CookingRecipes.csv": {"Name", "RecipeTag", "DisplayName", "Description", "IngredientTags", "IngredientQuantities", "OutputItemTag", "OutputQuantity", "HungerRestored", "ThirstRestored", "NutritionValue", "SpoilageDurationSeconds", "RefrigeratedSpoilageRate", "BuffTag", "BuffMagnitude", "BuffDurationSeconds", "RequiredStation"},
    ROOT / "Content/Astrawild/Data/Source/DT_PlayerPerks.csv": {"Name", "PerkTag", "DisplayName", "Description", "Tier", "PrerequisitePerkTags", "StatType", "StatBonus", "SprintStaminaMultiplier", "FoodNutritionMultiplier", "RepairRefundMultiplier", "CaptureOddsBonus", "ReloadSpeedMultiplier", "CriticalDamageMultiplier"},
    ROOT / "Content/Astrawild/Data/Source/DT_EchoTraits.csv": {"Name", "TraitTag", "DisplayName", "Description", "HealthMultiplier", "AttackMultiplier", "DefenseMultiplier", "WorkSpeedMultiplier"},
    ROOT / "Content/Astrawild/Data/Source/DT_TechnologyNodes.csv": {"Name", "TechnologyTag", "DisplayName", "Description", "Tier", "PrerequisiteTechnologyTags", "UnlockRecipeTags", "ResearchCost"},
    ROOT / "Content/Astrawild/Data/Source/DT_Recipes.csv": {"Name", "RecipeTag", "DisplayName", "Description", "IngredientTags", "IngredientQuantities", "OutputItemTag", "OutputQuantity", "CraftTimeSeconds", "RequiredStation", "RequiredTechnologyTag"},
    ROOT / "Content/Astrawild/Data/Source/DT_RangedWeapons.csv": {"Name", "WeaponTag", "DisplayName", "WeaponType", "DamageElement", "AmmoTag", "BaseDamage", "RangeCentimeters", "FireIntervalSeconds", "MagazineSize", "ReloadDurationSeconds", "bUseHitscan", "RequiredTechnologyTag"},
    ROOT / "Content/Astrawild/Data/Source/DT_Dungeons.csv": {"Name", "DungeonId", "DisplayName", "RegionTag", "RequiredKeyTag", "bConsumeRequiredKey", "BossSpeciesTag", "BossElement", "RecommendedLevel", "TimeLimitSeconds", "bSupportsCoop", "RewardItemTags", "RewardQuantities"},
    ROOT / "Content/Astrawild/Data/Source/DT_Evolutions.csv": {"Name", "EvolutionId", "SourceSpeciesTag", "TargetSpeciesTag", "TargetSpeciesData", "RequiredLevel", "RequiredItemTag", "RequiredItemQuantity"},
    ROOT / "Content/Astrawild/Data/Source/DT_Weather.csv": {"Name", "WeatherTag", "DisplayName", "TemperatureModifier", "VisibilityMultiplier", "WindStrength", "RainIntensity", "MinimumDurationSeconds", "MaximumDurationSeconds"},
    ROOT / "Content/Astrawild/Data/Source/DT_WorldEvents.csv": {"Name", "EventTag", "DisplayName", "EventType", "BiomeTag", "WeatherTag", "SpawnRuleTags", "RewardItemTags", "DurationSeconds", "CooldownSeconds", "MaxActiveSpecialSpawns", "bRequiresNight", "bRequiresStorm"},
    ROOT / "Content/Astrawild/Data/Source/DT_CampaignChapters.csv": {"Name", "ChapterId", "DisplayName", "Summary", "RequiredQuestIds", "OptionalQuestIds", "RequiredBossEncounterTag", "UnlockRegionTag", "UnlockSpireTag", "EndingChoiceTags", "bIsFinalChapter"},
    ROOT / "Content/Astrawild/Data/Source/DT_EcosystemBehavior.csv": {"Name", "SpeciesTag", "Temperament", "DietTag", "SocialGroupTag", "PerceptionRadius", "TerritoryRadius", "HungerSecondsUntilForage", "FleeHealthThreshold", "DefendHealthThreshold", "bCanMigrateDuringWorldEvents", "bFormsGroups", "bDefendsYoung"},
    ROOT / "Content/Astrawild/Data/Source/DT_MechaAnimationProfiles.csv": {"Name", "FrameTag", "AnimBlueprintPath", "GroundLocomotionPath", "FlightHoverPath", "FlightCruisePath", "OverboostMontagePath", "PlasmaEdgeMontagePath", "HeavyCannonMontagePath", "ShutdownMontagePath", "RequiredSocketNames"},
    ROOT / "Content/Astrawild/Data/Source/DT_MechaFrames.csv": {"Name", "FrameTag", "FrameName", "MechaClass", "MaxEnergy", "EnergyRechargeRate", "MaxShieldHP", "ArmorDefense", "GroundRunSpeed", "FlightCruiseSpeed", "OverboostSpeed", "DefaultWeaponTags"},
    ROOT / "Content/Astrawild/Data/Source/DT_MechaWeapons.csv": {"Name", "WeaponTag", "DisplayName", "HardpointSlot", "BaseDamage", "FireRate", "EnergyCostPerShot", "HeatGeneratedPerShot", "ProjectileSpeed", "bIsHomingMissile", "bIsContinuousBeam"},
    ROOT / "Content/Astrawild/Data/Source/DT_CyberneticEvolutions.csv": {"Name", "BaseEchoTag", "ResultingMechaEchoTag", "CyberVariantName", "RequiredPlayerLevel", "RequiredAstraCoreItemTag", "RequiredIngotsCount"},
    ROOT / "Content/Astrawild/Data/Source/DT_MechaVFX.csv": {"Name", "EffectTag", "DisplayName", "NiagaraSystemPath", "AttachSocket", "StartParameter", "EndParameter", "IntensityParameter", "bLooping", "bFallbackToEmitter"},
}
REQUIRED_PATHS = [
    "Source/AstrawildCore/Public/Animation/AstrawildAnimInstance.h",
    "Source/AstrawildCore/Private/Animation/AstrawildAnimInstance.cpp",
    "Source/AstrawildCore/Public/Components/AstrawildFeedbackComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildFeedbackComponent.cpp",
    "Source/AstrawildCore/Public/Components/AstrawildQuestComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildQuestComponent.cpp",
    "Source/AstrawildCore/Public/Components/AstrawildSurvivalComponent.h",
    "Source/AstrawildCore/Public/Data/AstrawildCookingData.h",
    "Source/AstrawildCore/Public/Data/AstrawildPlayerProgressionData.h",
    "Source/AstrawildCore/Public/Components/AstrawildPlayerProgressionComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildPlayerProgressionComponent.cpp",
    "Source/AstrawildCore/Private/Components/AstrawildSurvivalComponent.cpp",
    "Source/AstrawildCore/Public/Data/AstrawildEcosystemData.h",
    "Source/AstrawildCore/Public/Components/AstrawildEcosystemBehaviorComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildEcosystemBehaviorComponent.cpp",
    "Scripts/generate_ecosystem_behavior.py",
    "Source/AstrawildCore/Public/UI/AstrawildGameplayWidgets.h",
    "Source/AstrawildCore/Private/UI/AstrawildGameplayWidgets.cpp",
    "Source/AstrawildCore/Public/UI/AstrawildCockpitWidget.h",
    "Source/AstrawildCore/Private/UI/AstrawildCockpitWidget.cpp",
    "Source/AstrawildCore/Public/Echoes/AstrawildAlphaEcho.h",
    "Source/AstrawildCore/Public/Data/AstrawildBossData.h",
    "Source/AstrawildCore/Public/World/AstrawildBossAIController.h",
    "Source/AstrawildCore/Public/World/AstrawildLandscapeMaterialComponent.h",
    "Source/AstrawildCore/Private/World/AstrawildLandscapeMaterialComponent.cpp",
    "Source/AstrawildCore/Public/World/AstrawildAudioSubsystem.h",
    "Source/AstrawildCore/Private/World/AstrawildAudioSubsystem.cpp",
    "Source/AstrawildCore/Private/World/AstrawildBossAIController.cpp",
    "Source/AstrawildCore/Private/Echoes/AstrawildAlphaEcho.cpp",
    "Source/AstrawildCore/Public/World/AstrawildWorldData.h",
    "Source/AstrawildCore/Public/World/AstrawildWorldPartitionSubsystem.h",
    "Source/AstrawildCore/Private/World/AstrawildWorldPartitionSubsystem.cpp",
    "Source/AstrawildCore/Public/World/AstrawildEnvironmentHazardComponent.h",
    "Source/AstrawildCore/Private/World/AstrawildEnvironmentHazardComponent.cpp",
    "Source/AstrawildCore/Public/Data/AstrawildEchoDexRow.h",
    "Source/AstrawildCore/Public/Data/AstrawildMasterEchoData.h",
    "Source/AstrawildCore/Public/Data/AstrawildBreedingFusionData.h",
    "Source/AstrawildCore/Public/Data/AstrawildPartnerGearData.h",
    "Source/AstrawildCore/Public/Data/AstrawildGeneratedAssetRegistry.h",
    "Source/AstrawildCore/Private/Data/AstrawildGeneratedAssetRegistry.cpp",
    "Source/AstrawildCore/Public/Data/AstrawildPartnerGearData.h",
    "Source/AstrawildCore/Public/Data/AstrawildPowerGridData.h",
    "Source/AstrawildCore/Public/World/AstrawildPowerGridSubsystem.h",
    "Source/AstrawildCore/Private/World/AstrawildPowerGridSubsystem.cpp",
    "Source/AstrawildCore/Public/Data/AstrawildWorldEventData.h",
    "Source/AstrawildCore/Public/World/AstrawildWorldEventSubsystem.h",
    "Source/AstrawildCore/Private/World/AstrawildWorldEventSubsystem.cpp",
    "Source/AstrawildCore/Public/Data/AstrawildCampaignData.h",
    "Source/AstrawildCore/Public/World/AstrawildCampaignSubsystem.h",
    "Source/AstrawildCore/Private/World/AstrawildCampaignSubsystem.cpp",
    "Source/AstrawildCore/Public/World/AstrawildPowerGridSubsystem.h",
    "Source/AstrawildCore/Public/Characters/AstrawildMountedWeaponBase.h",
    "Source/AstrawildCore/Private/Characters/AstrawildMountedWeaponBase.cpp",
    "Source/AstrawildCore/Public/Data/AstrawildMasterEchoData.h",
    "Source/AstrawildCore/Public/Data/AstrawildBreedingFusionData.h",
    "Source/AstrawildCore/Private/Data/AstrawildGeneratedAssetRegistry.cpp",
    "Source/AstrawildCore/Private/Tests/AstrawildElementalMatrixTests.cpp",
    "Source/AstrawildCore/Public/Components/AstrawildBreedingComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildBreedingComponent.cpp",
    "Source/AstrawildCore/Public/Components/AstrawildMechaComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildMechaComponent.cpp",
    "Source/AstrawildCore/Public/Data/AstrawildMechaData.h",
    "Source/AstrawildCore/Public/Data/AstrawildMechaAnimationData.h",
    "Source/AstrawildCore/Public/Data/AstrawildMechaVFXData.h",
    "Source/AstrawildCore/Public/Components/AstrawildMechaVFXComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildMechaVFXComponent.cpp",
    "Source/AstrawildCore/Private/Components/AstrawildMountComponent.cpp",
    "Source/AstrawildCore/Public/Data/AstrawildMountData.h",
    "Source/AstrawildCore/Public/Data/AstrawildBreedingData.h",
    "Source/AstrawildCore/Public/Data/AstrawildTechnologyData.h",
    "Source/AstrawildCore/Public/Data/AstrawildCraftingData.h",
    "Source/AstrawildCore/Public/Data/AstrawildRangedWeaponData.h",
    "Source/AstrawildCore/Public/Data/AstrawildDungeonData.h",
    "Source/AstrawildCore/Public/World/AstrawildWeatherSubsystem.h",
    "Source/AstrawildCore/Public/World/AstrawildWorldClockSubsystem.h",
    "Source/AstrawildCore/Private/World/AstrawildWorldClockSubsystem.cpp",
    "Source/AstrawildCore/Public/Data/AstrawildEvolutionData.h",
    "Source/AstrawildCore/Public/Components/AstrawildEvolutionComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildEvolutionComponent.cpp",
    "Source/AstrawildCore/Private/World/AstrawildWeatherSubsystem.cpp",
    "Source/AstrawildCore/Public/Components/AstrawildRangedCombatComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildRangedCombatComponent.cpp",
    "Source/AstrawildCore/Public/UI/AstrawildMasterWidgets.h",
    "Source/AstrawildCore/Private/UI/AstrawildMasterWidgets.cpp",
    "Source/AstrawildCore/Public/Components/AstrawildSanComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildSanComponent.cpp",
    "Source/AstrawildCore/Public/Components/AstrawildColonyWorkComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildColonyWorkComponent.cpp",
    "Source/AstrawildCore/Public/Components/AstrawildTechnologyComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildTechnologyComponent.cpp",
    "Docs/M1_WORLD_PARTITION_HANDOFF.md",
    "Docs/M2_ELEMENT_COMPATIBILITY_TEST_PLAN.md",
    "Docs/M2_ECHODEX_MOUNT_BREEDING_HANDOFF.md",
    "Docs/M2_EVOLUTION_HANDOFF.md",
    "Docs/M3_M5_COLONY_TECHNOLOGY_HANDOFF.md",
    "Docs/M6_M8_COMBAT_DUNGEON_HANDOFF.md",
    "Docs/M9_M10_UI_PACKAGING_HANDOFF.md",
    "Docs/VISUAL_AND_WORLD_POLISH_HANDOFF.md",
    "Docs/UNREAL_EDITOR_AUTOMATION_HANDOFF.md",
    "Docs/P5_ASTRA_EXOSUIT_SYSTEM_SPEC.md",
    "Docs/VERTICAL_SLICE_MAP_20MIN_SPEC.md",
    "Docs/BUILD_STATUS.md",
    "Tools/Package_Astrawild.ps1",
    "Scripts/generate_3d_props.py",
    "Scripts/generate_master_echodex_200.py",
    "Scripts/generate_breeding_fusions.py",
    "Scripts/validate_master_echodex.py",
    "Scripts/generate_cooking_recipes.py",
    "Scripts/generate_player_perks.py",
    "Scripts/generate_game_audio.py",
    "Scripts/validate_generated_assets.py",
    "Scripts/validate_mecha_contracts.py",
    "Scripts/import_all_datatables.py",
    "Scripts/setup_project_assets.py",
    "Scripts/validate_runtime_contracts.py",
    "Scripts/validate_editor_automation.py",
    "Scripts/validate_generated_headers.py",
    "Source/AstrawildCore/Public/Data/AstrawildEvolutionData.h",
    "Config/AstrawildWorldPartition.ini",
    "Content/Astrawild/Meshes/Props/ASTRAWILD_Props_Manifest.json",
    "Content/Astrawild/Audio/SFX/ASTRAWILD_SFX_Manifest.json",
    "Content/Astrawild/Data/Source/DT_EchoDex_200.csv",
    "Content/Astrawild/Data/Source/DT_BreedingFusions.csv",
    "Content/Astrawild/Data/Source/DT_PowerGrid.csv",
    "Content/Astrawild/Data/Source/DT_CookingRecipes.csv",
    "Content/Astrawild/Data/Source/DT_PlayerPerks.csv",
    "Content/Astrawild/Data/Source/DT_WorldEvents.csv",
    "Content/Astrawild/Data/Source/DT_CampaignChapters.csv",
    "Content/Astrawild/Data/Source/DT_EcosystemBehavior.csv",
    "Content/Astrawild/Data/Source/DT_MechaAnimationProfiles.csv",
    "Content/Astrawild/Data/Source/DT_MechaFrames.csv",
    "Content/Astrawild/Data/Source/DT_MechaWeapons.csv",
    "Content/Astrawild/Data/Source/DT_CyberneticEvolutions.csv",
    "Content/Astrawild/Data/Source/DT_MechaVFX.csv",
]

errors: list[str] = []
loaded_rows: dict[pathlib.Path, list[dict[str, str]]] = {}
for relative in REQUIRED_PATHS:
    if not (ROOT / relative).is_file():
        errors.append(f"missing required source/contract: {relative}")

for config_relative, required_lines in {
    "Config/DefaultEngine.ini": ["r.DynamicGlobalIlluminationMethod=1", "r.ReflectionMethod=1", "r.Lumen.HardwareRayTracing=1", "r.VolumetricFog=1", "r.VolumetricCloud=1"],
    "Config/DefaultScalability.ini": ["[GlobalIlluminationQuality@3]", "[ReflectionQuality@3]", "[VolumetricFogQuality@3]", "[FoliageQuality@3]"],
}.items():
    config_path = ROOT / config_relative
    if config_path.is_file():
        config_text = config_path.read_text(encoding="utf-8", errors="replace")
        for required_line in required_lines:
            if required_line not in config_text:
                errors.append(f"{config_relative} missing visual polish setting {required_line}")
    else:
        errors.append(f"missing visual polish config: {config_relative}")

world_config_path = ROOT / "Config/AstrawildWorldPartition.ini"
if world_config_path.is_file():
    config_text = world_config_path.read_text(encoding="utf-8", errors="replace")
    expected_config = {
        "MapSizeCentimeters": "409600",
        "CellSizeCentimeters": "51200",
        "CellsPerAxis": "8",
        "TotalCellCount": "64",
        "BiomeCount": "4",
        "FastTravelSpireCount": "16",
    }
    for key, expected_value in expected_config.items():
        if not any(line.strip() == f"{key}={expected_value}" for line in config_text.splitlines()):
            errors.append(f"AstrawildWorldPartition.ini missing {key}={expected_value}")

for path, required_columns in REQUIRED_CSV.items():
    if not path.is_file():
        errors.append(f"missing CSV: {path.relative_to(ROOT)}")
        continue
    with path.open(newline="", encoding="utf-8-sig") as handle:
        reader = csv.DictReader(handle)
        columns = set(reader.fieldnames or [])
        missing = required_columns - columns
        if missing:
            errors.append(f"{path.relative_to(ROOT)} missing columns: {sorted(missing)}")
        rows = list(reader)
        loaded_rows[path] = rows
        if not rows:
            errors.append(f"{path.relative_to(ROOT)} has no rows")
        names = [row.get("Name", "") for row in rows]
        if any(not name.strip() for name in names):
            errors.append(f"{path.relative_to(ROOT)} contains an empty Name value")
        if len(names) != len(set(names)):
            errors.append(f"{path.relative_to(ROOT)} has duplicate Name values")

biomes_path = ROOT / "Content/Astrawild/Data/Source/DT_Biomes.csv"
biome_rows = loaded_rows.get(biomes_path, [])
expected_biomes = {
    "Biome.DawnMeadows",
    "Biome.SylvanRainforest",
    "Biome.ScorchedObsidianCaldera",
    "Biome.GlacialZenith",
}
actual_biomes = {row.get("BiomeId", "") for row in biome_rows}
if actual_biomes != expected_biomes:
    errors.append(f"DT_Biomes.csv must contain exactly the four canonical biome IDs; found {sorted(actual_biomes)}")
for row in biome_rows:
    try:
        minimum, maximum = int(row["MinLevel"]), int(row["MaxLevel"])
        temperature = int(row["TemperatureLevel"])
        if minimum < 1 or maximum < minimum or not -5 <= temperature <= 5:
            errors.append(f"DT_Biomes.csv invalid range in row {row.get('Name', '<unknown>')}")
    except (KeyError, ValueError):
        errors.append(f"DT_Biomes.csv non-numeric level/temperature in row {row.get('Name', '<unknown>')}")

foliage_path = ROOT / "Content/Astrawild/Data/Source/DT_FoliageRules.csv"
foliage_rows = loaded_rows.get(foliage_path, [])
valid_foliage_kinds = {"GroundCover", "Tree", "Shrub", "ResourceNode", "RockFormation"}
if len(foliage_rows) < 12:
    errors.append(f"DT_FoliageRules.csv must contain at least 12 authored foliage rows; found {len(foliage_rows)}")
foliage_ids = [row.get("FoliageRuleId", "") for row in foliage_rows]
if len(foliage_ids) != len(set(foliage_ids)):
    errors.append("DT_FoliageRules.csv has duplicate FoliageRuleId values")
for row in foliage_rows:
    if row.get("BiomeId", "") not in expected_biomes:
        errors.append(f"DT_FoliageRules.csv references unknown biome {row.get('BiomeId', '')}")
    if row.get("RuleKind", "") not in valid_foliage_kinds:
        errors.append(f"DT_FoliageRules.csv invalid RuleKind in row {row.get('Name', '<unknown>')}")
    if not row.get("FoliageAssetId", "").strip():
        errors.append(f"DT_FoliageRules.csv missing FoliageAssetId in row {row.get('Name', '<unknown>')}")
    try:
        min_slope, max_slope = float(row["MinSlopeDegrees"]), float(row["MaxSlopeDegrees"])
        min_height, max_height = float(row["MinHeightMeters"]), float(row["MaxHeightMeters"])
        if float(row["DensityScale"]) < 0 or not 0.0 <= min_slope <= max_slope <= 90.0 or min_height < 0 or max_height < min_height or not 0.0 <= float(row["WindResponse"]) <= 1.0:
            errors.append(f"DT_FoliageRules.csv invalid placement range in row {row.get('Name', '<unknown>')}")
    except (KeyError, ValueError):
        errors.append(f"DT_FoliageRules.csv non-numeric placement value in row {row.get('Name', '<unknown>')}")

spawn_path = ROOT / "Content/Astrawild/Data/Source/DT_SpawnRules.csv"
spawn_rows = loaded_rows.get(spawn_path, [])
for row in spawn_rows:
    if row.get("BiomeId", "") not in expected_biomes:
        errors.append(f"DT_SpawnRules.csv references unknown biome {row.get('BiomeId', '')}")
    try:
        minimum, maximum = int(row["MinLevel"]), int(row["MaxLevel"])
        weight, max_active = float(row["Weight"]), int(row["MaxActive"])
        if minimum < 1 or maximum < minimum or weight <= 0.0 or max_active < 1:
            errors.append(f"DT_SpawnRules.csv invalid numeric range in row {row.get('Name', '<unknown>')}")
    except (KeyError, ValueError):
        errors.append(f"DT_SpawnRules.csv non-numeric spawn value in row {row.get('Name', '<unknown>')}")

spire_path = ROOT / "Content/Astrawild/Data/Source/DT_FastTravelSpires.csv"
spire_rows = loaded_rows.get(spire_path, [])
if len(spire_rows) != 16:
    errors.append(f"DT_FastTravelSpires.csv must contain exactly 16 rows; found {len(spire_rows)}")
spire_ids = [row.get("SpireId", "") for row in spire_rows]
if len(spire_ids) != len(set(spire_ids)):
    errors.append("DT_FastTravelSpires.csv has duplicate SpireId values")
if sum(row.get("bUnlockedByDefault", "").lower() == "true" for row in spire_rows) != 1:
    errors.append("DT_FastTravelSpires.csv must have exactly one default-unlocked spire")
for row in spire_rows:
    if row.get("BiomeId", "") not in expected_biomes:
        errors.append(f"DT_FastTravelSpires.csv references unknown biome {row.get('BiomeId', '')}")

echo_dex_path = ROOT / "Content/Astrawild/Data/Source/DT_EchoDex.csv"
echo_dex_rows = loaded_rows.get(echo_dex_path, [])
if len(echo_dex_rows) != 30:
    errors.append(f"DT_EchoDex.csv must contain exactly 30 rows; found {len(echo_dex_rows)}")
else:
    dex_orders = sorted(int(row["DexOrder"]) for row in echo_dex_rows)
    if dex_orders != list(range(1, 31)):
        errors.append("DT_EchoDex.csv DexOrder values must be exactly 1 through 30")
echo_tags = [row.get("SpeciesTag", "") for row in echo_dex_rows]
if len(echo_tags) != len(set(echo_tags)):
    errors.append("DT_EchoDex.csv has duplicate SpeciesTag values")
valid_elements = {"Neutral", "Solar", "Torrent", "Geo", "Aether", "Volt", "Glacial", "Abyssal", "Astra"}
valid_roles = {"Exploration", "Combat", "BaseUtility"}
for row in echo_dex_rows:
    if row.get("PrimaryElement", "") not in valid_elements:
        errors.append(f"DT_EchoDex.csv invalid PrimaryElement in row {row.get('Name', '<unknown>')}")
    if row.get("Role", "") not in valid_roles:
        errors.append(f"DT_EchoDex.csv invalid Role in row {row.get('Name', '<unknown>')}")
    try:
        if float(row["BaseMaxHealth"]) <= 0 or float(row["BaseAttackPower"]) <= 0 or float(row["CaptureDifficultyModifier"]) <= 0:
            errors.append(f"DT_EchoDex.csv invalid base stat in row {row.get('Name', '<unknown>')}")
        if int(row["EvolutionLevel"]) < 0 or int(row["DexOrder"]) < 1:
            errors.append(f"DT_EchoDex.csv invalid evolution/order value in row {row.get('Name', '<unknown>')}")
    except (KeyError, ValueError):
        errors.append(f"DT_EchoDex.csv non-numeric stat in row {row.get('Name', '<unknown>')}")

mount_path = ROOT / "Content/Astrawild/Data/Source/DT_MountProfiles.csv"
mount_rows = loaded_rows.get(mount_path, [])
if len(mount_rows) < 1:
    errors.append("DT_MountProfiles.csv must contain at least one mount profile")
mount_ids = {row.get("MountProfileId", "") for row in mount_rows}
for row in mount_rows:
    try:
        if float(row["SpeedMultiplier"]) <= 0 or float(row["StaminaCostPerSecond"]) < 0 or float(row["JumpMultiplier"]) <= 0:
            errors.append(f"DT_MountProfiles.csv invalid numeric value in row {row.get('Name', '<unknown>')}")
    except (KeyError, ValueError):
        errors.append(f"DT_MountProfiles.csv non-numeric value in row {row.get('Name', '<unknown>')}")
for row in echo_dex_rows:
    if row.get("bCanBeMounted", "").lower() == "true" and row.get("MountProfileId", "") not in mount_ids:
        errors.append(f"DT_EchoDex.csv mounted species references missing profile {row.get('MountProfileId', '')}")

breeding_path = ROOT / "Content/Astrawild/Data/Source/DT_BreedingGroups.csv"
breeding_rows = loaded_rows.get(breeding_path, [])
breeding_ids = {row.get("BreedingGroupId", "") for row in breeding_rows}
for row in breeding_rows:
    try:
        if float(row["IncubationDurationSeconds"]) < 1.0 or not 0.0 <= float(row["MutationChance"]) <= 1.0 or int(row["MaxInheritedTraits"]) < 0:
            errors.append(f"DT_BreedingGroups.csv invalid numeric value in row {row.get('Name', '<unknown>')}")
    except (KeyError, ValueError):
        errors.append(f"DT_BreedingGroups.csv non-numeric value in row {row.get('Name', '<unknown>')}")
for row in echo_dex_rows:
    if row.get("BreedingGroupId", "") and row.get("BreedingGroupId", "") not in breeding_ids:
        errors.append(f"DT_EchoDex.csv references missing breeding group {row.get('BreedingGroupId', '')}")

trait_path = ROOT / "Content/Astrawild/Data/Source/DT_EchoTraits.csv"
trait_rows = loaded_rows.get(trait_path, [])
trait_tags = [row.get("TraitTag", "") for row in trait_rows]
if len(trait_tags) != len(set(trait_tags)):
    errors.append("DT_EchoTraits.csv has duplicate TraitTag values")
for row in trait_rows:
    try:
        for key in ("HealthMultiplier", "AttackMultiplier", "DefenseMultiplier", "WorkSpeedMultiplier"):
            if float(row[key]) < 0.0:
                errors.append(f"DT_EchoTraits.csv negative multiplier in row {row.get('Name', '<unknown>')}")
    except (KeyError, ValueError):
        errors.append(f"DT_EchoTraits.csv non-numeric multiplier in row {row.get('Name', '<unknown>')}")

recipe_path = ROOT / "Content/Astrawild/Data/Source/DT_Recipes.csv"
recipe_rows = loaded_rows.get(recipe_path, [])
if len(recipe_rows) != 64:
    errors.append(f"DT_Recipes.csv must contain exactly 64 rows; found {len(recipe_rows)}")
recipe_tags = [row.get("RecipeTag", "") for row in recipe_rows]
if len(recipe_tags) != len(set(recipe_tags)):
    errors.append("DT_Recipes.csv has duplicate RecipeTag values")
valid_stations = {"None", "Campfire", "RestBed", "CraftingBench", "StorageChest", "Structure", "HeatForge"}
for row in recipe_rows:
    if row.get("RequiredStation", "") not in valid_stations:
        errors.append(f"DT_Recipes.csv invalid station in row {row.get('Name', '<unknown>')}")
    try:
        if int(row["OutputQuantity"]) <= 0 or float(row["CraftTimeSeconds"]) <= 0:
            errors.append(f"DT_Recipes.csv invalid output/time in row {row.get('Name', '<unknown>')}")
        ingredient_tags = row["IngredientTags"].strip().strip("()")
        ingredient_quantities = row["IngredientQuantities"].strip().strip("()")
        if len([value for value in ingredient_tags.split(",") if value.strip()]) != len([value for value in ingredient_quantities.split(",") if value.strip()]):
            errors.append(f"DT_Recipes.csv ingredient arrays do not align in row {row.get('Name', '<unknown>')}")
    except (KeyError, ValueError):
        errors.append(f"DT_Recipes.csv non-numeric output/time in row {row.get('Name', '<unknown>')}")

evolution_path = ROOT / "Content/Astrawild/Data/Source/DT_Evolutions.csv"
evolution_rows = loaded_rows.get(evolution_path, [])
if len(evolution_rows) != 12:
    errors.append(f"DT_Evolutions.csv must contain exactly 12 rows; found {len(evolution_rows)}")
evolution_ids = [row.get("EvolutionId", "") for row in evolution_rows]
if len(evolution_ids) != len(set(evolution_ids)):
    errors.append("DT_Evolutions.csv has duplicate EvolutionId values")
for row in evolution_rows:
    if not row.get("SourceSpeciesTag", "") or not row.get("TargetSpeciesTag", "") or not row.get("TargetSpeciesData", ""):
        errors.append(f"DT_Evolutions.csv missing source/target/asset in row {row.get('Name', '<unknown>')}")
    try:
        if int(row["RequiredLevel"]) < 1 or int(row["RequiredItemQuantity"]) < 0:
            errors.append(f"DT_Evolutions.csv invalid gate in row {row.get('Name', '<unknown>')}")
    except (KeyError, ValueError):
        errors.append(f"DT_Evolutions.csv non-numeric gate in row {row.get('Name', '<unknown>')}")

weather_path = ROOT / "Content/Astrawild/Data/Source/DT_Weather.csv"
weather_rows = loaded_rows.get(weather_path, [])
if len(weather_rows) != 8:
    errors.append(f"DT_Weather.csv must contain exactly 8 rows; found {len(weather_rows)}")
weather_tags = [row.get("WeatherTag", "") for row in weather_rows]
if len(weather_tags) != len(set(weather_tags)):
    errors.append("DT_Weather.csv has duplicate WeatherTag values")
for row in weather_rows:
    try:
        if not -5 <= int(row["TemperatureModifier"]) <= 5:
            errors.append(f"DT_Weather.csv invalid temperature modifier in row {row.get('Name', '<unknown>')}")
        for field in ("VisibilityMultiplier", "WindStrength", "RainIntensity"):
            if not 0.0 <= float(row[field]) <= 1.0:
                errors.append(f"DT_Weather.csv invalid {field} in row {row.get('Name', '<unknown>')}")
        if float(row["MinimumDurationSeconds"]) < 30.0 or float(row["MaximumDurationSeconds"]) < float(row["MinimumDurationSeconds"]):
            errors.append(f"DT_Weather.csv invalid duration in row {row.get('Name', '<unknown>')}")
    except (KeyError, ValueError):
        errors.append(f"DT_Weather.csv non-numeric value in row {row.get('Name', '<unknown>')}")

dungeon_path = ROOT / "Content/Astrawild/Data/Source/DT_Dungeons.csv"
dungeon_rows = loaded_rows.get(dungeon_path, [])
if len(dungeon_rows) != 5:
    errors.append(f"DT_Dungeons.csv must contain exactly 5 tower rows; found {len(dungeon_rows)}")
dungeon_ids = [row.get("DungeonId", "") for row in dungeon_rows]
if len(dungeon_ids) != len(set(dungeon_ids)):
    errors.append("DT_Dungeons.csv has duplicate DungeonId values")
for row in dungeon_rows:
    if row.get("BossElement", "") not in {"Neutral", "Solar", "Torrent", "Geo", "Aether", "Volt", "Glacial", "Abyssal", "Astra"}:
        errors.append(f"DT_Dungeons.csv invalid boss element in row {row.get('Name', '<unknown>')}")
    try:
        if int(row["RecommendedLevel"]) < 1 or float(row["TimeLimitSeconds"]) < 60.0:
            errors.append(f"DT_Dungeons.csv invalid level/time in row {row.get('Name', '<unknown>')}")
        reward_tags = row["RewardItemTags"].strip().strip("()")
        reward_quantities = row["RewardQuantities"].strip().strip("()")
        if len([value for value in reward_tags.split(",") if value.strip()]) != len([value for value in reward_quantities.split(",") if value.strip()]):
            errors.append(f"DT_Dungeons.csv reward arrays do not align in row {row.get('Name', '<unknown>')}")
    except (KeyError, ValueError):
        errors.append(f"DT_Dungeons.csv non-numeric level/time in row {row.get('Name', '<unknown>')}")

boss_encounter_path = ROOT / "Content/Astrawild/Data/Source/DT_BossEncounters.csv"
boss_attack_path = ROOT / "Content/Astrawild/Data/Source/DT_BossAttacks.csv"
boss_encounter_rows = loaded_rows.get(boss_encounter_path, [])
boss_attack_rows = loaded_rows.get(boss_attack_path, [])
valid_boss_elements = {"Neutral", "Solar", "Torrent", "Geo", "Aether", "Volt", "Glacial", "Abyssal", "Astra"}
if len(boss_encounter_rows) != 5:
    errors.append(f"DT_BossEncounters.csv must contain exactly 5 tower encounter rows; found {len(boss_encounter_rows)}")
encounter_ids = [row.get("EncounterId", "") for row in boss_encounter_rows]
if len(encounter_ids) != len(set(encounter_ids)):
    errors.append("DT_BossEncounters.csv has duplicate EncounterId values")
if len(boss_attack_rows) < 15:
    errors.append(f"DT_BossAttacks.csv must contain at least 15 attack rows; found {len(boss_attack_rows)}")
attack_ids = [row.get("AttackId", "") for row in boss_attack_rows]
if len(attack_ids) != len(set(attack_ids)):
    errors.append("DT_BossAttacks.csv has duplicate AttackId values")
encounter_id_set = set(encounter_ids)
for row in boss_encounter_rows:
    if row.get("DungeonId", "") not in dungeon_ids:
        errors.append(f"boss encounter {row.get('EncounterId', '<unknown>')} references missing dungeon {row.get('DungeonId', '')}")
    if row.get("BossSpeciesTag", "") not in set(echo_tags):
        errors.append(f"boss encounter {row.get('EncounterId', '<unknown>')} references missing Echo {row.get('BossSpeciesTag', '')}")
    if row.get("PrimaryElement", "") not in valid_boss_elements:
        errors.append(f"DT_BossEncounters.csv invalid primary element in row {row.get('Name', '<unknown>')}")
    try:
        phase_count = int(row["PhaseCount"])
        phase_two = float(row["PhaseTwoHealthThreshold"])
        phase_three = float(row["PhaseThreeHealthThreshold"])
        if int(row["RecommendedLevel"]) < 1 or float(row["MaxHealth"]) <= 0 or phase_count < 1 or not 0.0 <= phase_three < phase_two < 1.0:
            errors.append(f"DT_BossEncounters.csv invalid phase/stat values in row {row.get('Name', '<unknown>')}")
        if float(row["IntroDurationSeconds"]) < 0 or float(row["EncounterTimeLimitSeconds"]) < 60 or int(row["MaxParticipants"]) < 1:
            errors.append(f"DT_BossEncounters.csv invalid timing/participant values in row {row.get('Name', '<unknown>')}")
    except (KeyError, ValueError):
        errors.append(f"DT_BossEncounters.csv non-numeric phase/stat value in row {row.get('Name', '<unknown>')}")
for row in boss_attack_rows:
    if row.get("EncounterId", "") not in encounter_id_set:
        errors.append(f"boss attack {row.get('AttackId', '<unknown>')} references missing encounter {row.get('EncounterId', '')}")
    if row.get("Element", "") not in valid_boss_elements:
        errors.append(f"DT_BossAttacks.csv invalid element in row {row.get('Name', '<unknown>')}")
    try:
        if int(row["PhaseIndex"]) < 1 or float(row["TelegraphDurationSeconds"]) < 0 or float(row["TelegraphRadius"]) < 50 or float(row["CooldownSeconds"]) <= 0 or float(row["DamageMultiplier"]) <= 0:
            errors.append(f"DT_BossAttacks.csv invalid combat value in row {row.get('Name', '<unknown>')}")
    except (KeyError, ValueError):
        errors.append(f"DT_BossAttacks.csv non-numeric combat value in row {row.get('Name', '<unknown>')}")

ranged_path = ROOT / "Content/Astrawild/Data/Source/DT_RangedWeapons.csv"
ranged_rows = loaded_rows.get(ranged_path, [])
if len(ranged_rows) != 8:
    errors.append(f"DT_RangedWeapons.csv must contain exactly 8 rows; found {len(ranged_rows)}")
ranged_tags = [row.get("WeaponTag", "") for row in ranged_rows]
if len(ranged_tags) != len(set(ranged_tags)):
    errors.append("DT_RangedWeapons.csv has duplicate WeaponTag values")
valid_weapon_types = {"Bow", "Repeater", "Beam"}
valid_elements = {"Neutral", "Solar", "Torrent", "Geo", "Aether", "Volt", "Glacial", "Abyssal", "Astra"}
for row in ranged_rows:
    if row.get("WeaponType", "") not in valid_weapon_types:
        errors.append(f"DT_RangedWeapons.csv invalid weapon type in row {row.get('Name', '<unknown>')}")
    if row.get("DamageElement", "") not in valid_elements:
        errors.append(f"DT_RangedWeapons.csv invalid damage element in row {row.get('Name', '<unknown>')}")
    try:
        if float(row["BaseDamage"]) <= 0 or float(row["RangeCentimeters"]) <= 0 or float(row["FireIntervalSeconds"]) <= 0 or int(row["MagazineSize"]) <= 0 or float(row["ReloadDurationSeconds"]) <= 0:
            errors.append(f"DT_RangedWeapons.csv invalid combat value in row {row.get('Name', '<unknown>')}")
    except (KeyError, ValueError):
        errors.append(f"DT_RangedWeapons.csv non-numeric combat value in row {row.get('Name', '<unknown>')}")

technology_path = ROOT / "Content/Astrawild/Data/Source/DT_TechnologyNodes.csv"
technology_rows = loaded_rows.get(technology_path, [])
technology_tags = [row.get("TechnologyTag", "") for row in technology_rows]
if len(technology_tags) != len(set(technology_tags)):
    errors.append("DT_TechnologyNodes.csv has duplicate TechnologyTag values")
for row in technology_rows:
    try:
        if int(row["Tier"]) < 0 or int(row["ResearchCost"]) < 0:
            errors.append(f"DT_TechnologyNodes.csv invalid tier/cost in row {row.get('Name', '<unknown>')}")
    except (KeyError, ValueError):
        errors.append(f"DT_TechnologyNodes.csv non-numeric tier/cost in row {row.get('Name', '<unknown>')}")

for header in (ROOT / "Source").rglob("*.h"):
    text = header.read_text(encoding="utf-8", errors="replace")
    if "UCLASS(" in text or "USTRUCT(" in text or "UENUM(" in text:
        if "generated.h" not in text:
            errors.append(f"reflection header missing generated include: {header.relative_to(ROOT)}")

for source in (ROOT / "Source").rglob("*.cpp"):
    text = source.read_text(encoding="utf-8", errors="replace")
    if text.count("{") != text.count("}"):
        errors.append(f"brace count mismatch: {source.relative_to(ROOT)}")
    if text.count("(") != text.count(")"):
        errors.append(f"parenthesis count mismatch: {source.relative_to(ROOT)}")

if errors:
    print("ASTRAWILD content contract validation failed:")
    print("\n".join(f"- {error}" for error in errors))
    sys.exit(1)

print("ASTRAWILD content contract validation passed.")
