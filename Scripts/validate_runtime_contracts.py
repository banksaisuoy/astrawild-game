"""Validate cross-table references that CSV shape validation cannot prove."""
from __future__ import annotations

import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Content/Astrawild/Data/Source"


def rows(name: str) -> list[dict[str, str]]:
    with (SOURCE / name).open(encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle))


def list_field(value: str) -> list[str]:
    value = value.strip().strip("()")
    return [item.strip() for item in value.split(",") if item.strip()]


errors: list[str] = []
quests = rows("DT_Quests.csv")
objectives = rows("DT_QuestObjectives.csv")
quest_ids = {row["QuestId"] for row in quests}
for row in quests:
    prerequisite = row.get("PrerequisiteQuestTag", "").strip()
    if prerequisite and prerequisite not in quest_ids:
        errors.append(f"quest {row['QuestId']} references missing prerequisite {prerequisite}")
for row in objectives:
    if row["QuestId"] not in quest_ids:
        errors.append(f"objective {row['ObjectiveId']} references missing quest {row['QuestId']}")

traits = {row["TraitTag"] for row in rows("DT_EchoTraits.csv")}
breeding_groups = {row["BreedingGroupId"] for row in rows("DT_BreedingGroups.csv")}
mount_profiles = {row["MountProfileId"] for row in rows("DT_MountProfiles.csv")}
echo_rows = rows("DT_EchoDex.csv")
echo_species = {row["SpeciesTag"] for row in echo_rows}
for row in echo_rows:
    for trait in list_field(row.get("PassiveTraitTags", "")):
        if trait not in traits:
            errors.append(f"Echo {row['SpeciesTag']} references missing trait {trait}")
    if row.get("BreedingGroupId", "") not in breeding_groups:
        errors.append(f"Echo {row['SpeciesTag']} references missing breeding group {row.get('BreedingGroupId', '')}")
    if row.get("bCanBeMounted", "").lower() == "true" and row.get("MountProfileId", "") not in mount_profiles:
        errors.append(f"mounted Echo {row['SpeciesTag']} references missing mount profile {row.get('MountProfileId', '')}")

for row in rows("DT_SpawnRules.csv"):
    if row["SpeciesTag"] not in echo_species:
        errors.append(f"spawn rule {row['SpawnRuleId']} references missing Echo {row['SpeciesTag']}")

evolution_rows = rows("DT_Evolutions.csv")
for row in evolution_rows:
    if row["SourceSpeciesTag"] not in echo_species:
        errors.append(f"evolution {row['EvolutionId']} references missing source Echo {row['SourceSpeciesTag']}")
    if row["TargetSpeciesTag"] not in echo_species:
        errors.append(f"evolution {row['EvolutionId']} references missing target Echo {row['TargetSpeciesTag']}")
    if not row.get("TargetSpeciesData", "").startswith("/Game/"):
        errors.append(f"evolution {row['EvolutionId']} has invalid target DataAsset path")

technology_tags = {row["TechnologyTag"] for row in rows("DT_TechnologyNodes.csv")}
for row in rows("DT_Recipes.csv"):
    required_tech = row.get("RequiredTechnologyTag", "").strip()
    if required_tech and required_tech not in technology_tags:
        errors.append(f"recipe {row['RecipeTag']} references missing technology {required_tech}")

dungeon_rows = rows("DT_Dungeons.csv")
dungeon_ids = {row["DungeonId"] for row in dungeon_rows}
for row in dungeon_rows:
    if row["BossSpeciesTag"] not in echo_species:
        errors.append(f"dungeon {row['DungeonId']} references missing boss Echo {row['BossSpeciesTag']}")
    if len(list_field(row["RewardItemTags"])) != len(list_field(row["RewardQuantities"])):
        errors.append(f"dungeon {row['DungeonId']} reward arrays are not aligned")

boss_encounter_rows = rows("DT_BossEncounters.csv")
boss_encounter_ids = {row["EncounterId"] for row in boss_encounter_rows}
for row in boss_encounter_rows:
    if row["DungeonId"] not in dungeon_ids:
        errors.append(f"boss encounter {row['EncounterId']} references missing dungeon {row['DungeonId']}")
    if row["BossSpeciesTag"] not in echo_species:
        errors.append(f"boss encounter {row['EncounterId']} references missing Echo {row['BossSpeciesTag']}")
    try:
        phase_count = int(row["PhaseCount"])
        phase_two = float(row["PhaseTwoHealthThreshold"])
        phase_three = float(row["PhaseThreeHealthThreshold"])
        if phase_count < 1 or not 0.0 <= phase_three < phase_two < 1.0:
            errors.append(f"boss encounter {row['EncounterId']} has invalid phase thresholds")
    except (KeyError, ValueError):
        errors.append(f"boss encounter {row['EncounterId']} has invalid numeric phase data")

for row in rows("DT_BossAttacks.csv"):
    if row["EncounterId"] not in boss_encounter_ids:
        errors.append(f"boss attack {row['AttackId']} references missing encounter {row['EncounterId']}")
    try:
        phase_index = int(row["PhaseIndex"])
        encounter = next((item for item in boss_encounter_rows if item["EncounterId"] == row["EncounterId"]), None)
        if encounter and phase_index > int(encounter["PhaseCount"]):
            errors.append(f"boss attack {row['AttackId']} exceeds phase count for {row['EncounterId']}")
    except (KeyError, ValueError):
        errors.append(f"boss attack {row['AttackId']} has invalid phase index")

spire_rows = rows("DT_FastTravelSpires.csv")
spire_targets = [row.get("QuestTargetTag", "").strip() for row in spire_rows]
if any(not target for target in spire_targets):
    errors.append("every fast-travel spire must have a QuestTargetTag")
if len(spire_targets) != len(set(spire_targets)):
    errors.append("fast-travel spire QuestTargetTag values must be unique")

if errors:
    print("ASTRAWILD runtime contract validation failed:")
    for error in errors:
        print(f"- {error}")
    raise SystemExit(1)

print("ASTRAWILD runtime contract validation passed.")
