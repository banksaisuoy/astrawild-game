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
    ROOT / "Content/Astrawild/Data/Source/DT_FastTravelSpires.csv": {"Name", "SpireId", "DisplayName", "BiomeId", "WorldTransform", "bUnlockedByDefault"},
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
    "Docs/M1_WORLD_PARTITION_HANDOFF.md",
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
