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
    ROOT / "Content/Astrawild/Data/Source/DT_SpawnRules.csv": {"Name", "SpawnRuleId", "SpeciesTag", "BiomeId", "MinLevel", "MaxLevel", "Weight", "MaxActive"},
    ROOT / "Content/Astrawild/Data/Source/DT_FastTravelSpires.csv": {"Name", "SpireId", "DisplayName", "BiomeId", "QuestTargetTag", "WorldTransform", "bUnlockedByDefault"},
    ROOT / "Content/Astrawild/Data/Source/DT_EchoDex.csv": {"Name", "SpeciesTag", "SpeciesName", "SpeciesTitle", "LoreDescription", "PrimaryElement", "ElementalAffinities", "Role", "BaseMaxHealth", "BaseAttackPower", "BaseDefensePower", "BaseWalkSpeed", "BaseRunSpeed", "CaptureDifficultyModifier", "PassiveTraitTags", "WorkSuitabilityTags", "PartnerSkillTag", "MountProfileId", "bCanBeMounted", "BreedingGroupId", "EvolutionTargetId", "EvolutionLevel", "DexOrder"},
    ROOT / "Content/Astrawild/Data/Source/DT_MountProfiles.csv": {"Name", "MountProfileId", "SaddleSocketName", "SpeedMultiplier", "StaminaCostPerSecond", "JumpMultiplier", "bAllowsCombatFromMount", "MountFamilyTag"},
    ROOT / "Content/Astrawild/Data/Source/DT_BreedingGroups.csv": {"Name", "BreedingGroupId", "CompatibleSpeciesTags", "IncubationDurationSeconds", "MutationChance", "MaxInheritedTraits"},
    ROOT / "Content/Astrawild/Data/Source/DT_EchoTraits.csv": {"Name", "TraitTag", "DisplayName", "Description", "HealthMultiplier", "AttackMultiplier", "DefenseMultiplier", "WorkSpeedMultiplier"},
    ROOT / "Content/Astrawild/Data/Source/DT_TechnologyNodes.csv": {"Name", "TechnologyTag", "DisplayName", "Description", "Tier", "PrerequisiteTechnologyTags", "UnlockRecipeTags", "ResearchCost"},
    ROOT / "Content/Astrawild/Data/Source/DT_Recipes.csv": {"Name", "RecipeTag", "DisplayName", "Description", "IngredientTags", "IngredientQuantities", "OutputItemTag", "OutputQuantity", "CraftTimeSeconds", "RequiredStation", "RequiredTechnologyTag"},
    ROOT / "Content/Astrawild/Data/Source/DT_RangedWeapons.csv": {"Name", "WeaponTag", "DisplayName", "WeaponType", "DamageElement", "AmmoTag", "BaseDamage", "RangeCentimeters", "FireIntervalSeconds", "MagazineSize", "ReloadDurationSeconds", "bUseHitscan", "RequiredTechnologyTag"},
    ROOT / "Content/Astrawild/Data/Source/DT_Dungeons.csv": {"Name", "DungeonId", "DisplayName", "RegionTag", "RequiredKeyTag", "bConsumeRequiredKey", "BossSpeciesTag", "BossElement", "RecommendedLevel", "TimeLimitSeconds", "bSupportsCoop", "RewardItemTags", "RewardQuantities"},
}
REQUIRED_PATHS = [
    "Source/AstrawildCore/Public/Animation/AstrawildAnimInstance.h",
    "Source/AstrawildCore/Private/Animation/AstrawildAnimInstance.cpp",
    "Source/AstrawildCore/Public/Components/AstrawildFeedbackComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildFeedbackComponent.cpp",
    "Source/AstrawildCore/Public/Components/AstrawildQuestComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildQuestComponent.cpp",
    "Source/AstrawildCore/Public/Components/AstrawildSurvivalComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildSurvivalComponent.cpp",
    "Source/AstrawildCore/Public/UI/AstrawildGameplayWidgets.h",
    "Source/AstrawildCore/Private/UI/AstrawildGameplayWidgets.cpp",
    "Source/AstrawildCore/Public/Echoes/AstrawildAlphaEcho.h",
    "Source/AstrawildCore/Private/Echoes/AstrawildAlphaEcho.cpp",
    "Source/AstrawildCore/Public/World/AstrawildWorldData.h",
    "Source/AstrawildCore/Public/World/AstrawildWorldPartitionSubsystem.h",
    "Source/AstrawildCore/Private/World/AstrawildWorldPartitionSubsystem.cpp",
    "Source/AstrawildCore/Public/World/AstrawildEnvironmentHazardComponent.h",
    "Source/AstrawildCore/Private/World/AstrawildEnvironmentHazardComponent.cpp",
    "Source/AstrawildCore/Public/Data/AstrawildEchoDexRow.h",
    "Source/AstrawildCore/Private/Tests/AstrawildElementalMatrixTests.cpp",
    "Source/AstrawildCore/Public/Components/AstrawildBreedingComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildBreedingComponent.cpp",
    "Source/AstrawildCore/Public/Components/AstrawildMountComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildMountComponent.cpp",
    "Source/AstrawildCore/Public/Data/AstrawildMountData.h",
    "Source/AstrawildCore/Public/Data/AstrawildBreedingData.h",
    "Source/AstrawildCore/Public/Data/AstrawildTechnologyData.h",
    "Source/AstrawildCore/Public/Data/AstrawildCraftingData.h",
    "Source/AstrawildCore/Public/Data/AstrawildRangedWeaponData.h",
    "Source/AstrawildCore/Public/Data/AstrawildDungeonData.h",
    "Source/AstrawildCore/Public/World/AstrawildDungeonSubsystem.h",
    "Source/AstrawildCore/Private/World/AstrawildDungeonSubsystem.cpp",
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
    "Docs/M3_M5_COLONY_TECHNOLOGY_HANDOFF.md",
    "Docs/M6_M8_COMBAT_DUNGEON_HANDOFF.md",
    "Docs/M9_M10_UI_PACKAGING_HANDOFF.md",
    "Docs/BUILD_STATUS.md",
    "Tools/Package_Astrawild.ps1",
    "Scripts/validate_runtime_contracts.py",
    "Config/AstrawildWorldPartition.ini",
]

errors: list[str] = []
loaded_rows: dict[pathlib.Path, list[dict[str, str]]] = {}
for relative in REQUIRED_PATHS:
    if not (ROOT / relative).is_file():
        errors.append(f"missing required source/contract: {relative}")

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
