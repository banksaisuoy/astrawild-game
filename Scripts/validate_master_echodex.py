"""Validate the 200-entry ASTRAWILD master Echo lexicon."""
from __future__ import annotations

import csv
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Content/Astrawild/Data/Source"
MASTER = SOURCE / "DT_EchoDex_200.csv"
EXPECTED_HEADERS = {
    "Name", "DexOrder", "SpeciesTag", "SpeciesName", "SpeciesTitle", "AnatomyConcept", "Diet", "SocialBehavior", "Temperament", "HabitatBiomeTag", "ActivityCycleTag", "PrimaryElement", "ElementalAffinities", "Role", "BaseMaxHealth", "BaseAttackPower", "BaseDefensePower", "BaseStamina", "BaseWalkSpeed", "BaseRunSpeed", "CaptureDifficultyModifier", "WorkSuitabilityLevels", "WorkSuitabilityTags", "PassiveTraitTags", "ActiveSkillTags", "ActiveSkillElementTags", "ActiveSkillCooldowns", "ActiveSkillDamageMultipliers", "ActiveSkillTelegraphs", "PartnerSkillTag", "MountedWeaponTag", "DropItemTags", "DropItemQuantities", "ParentSpeciesA", "ParentSpeciesB"
}
VALID_ELEMENTS = {"Neutral", "Solar", "Torrent", "Geo", "Volt", "Glacial", "Abyssal", "Astra", "Aether"}
VALID_ROLES = {"Exploration", "Combat", "BaseUtility"}
ARRAY_FIELDS = ("ElementalAffinities", "WorkSuitabilityLevels", "WorkSuitabilityTags", "PassiveTraitTags", "ActiveSkillTags", "ActiveSkillElementTags", "ActiveSkillCooldowns", "ActiveSkillDamageMultipliers", "ActiveSkillTelegraphs", "DropItemTags", "DropItemQuantities")
# These are kept out of new authored data because they are strongly associated
# with external creature properties, not ASTRAWILD original terminology.
FORBIDDEN_NAMES = {"Cattiva", "Palworld", "Pokemon", "Pokémon", "ARK", "Pocketpair", "Nintendo"}


def split_array(value: str) -> list[str]:
    value = value.strip()
    if value.startswith("(") and value.endswith(")"):
        value = value[1:-1]
    return [item.strip() for item in value.split(",") if item.strip()]


def main() -> int:
    errors: list[str] = []
    if not MASTER.is_file():
        print(f"missing {MASTER}")
        return 1
    with MASTER.open(encoding="utf-8-sig", newline="") as handle:
        reader = csv.DictReader(handle)
        headers = set(reader.fieldnames or [])
        rows = list(reader)
    if headers != EXPECTED_HEADERS:
        errors.append(f"header mismatch: missing={sorted(EXPECTED_HEADERS - headers)} extra={sorted(headers - EXPECTED_HEADERS)}")
    if len(rows) != 200:
        errors.append(f"expected 200 rows, found {len(rows)}")
    orders: list[int] = []
    species_tags: list[str] = []
    names: list[str] = []
    biome_ids = set()
    biome_path = SOURCE / "DT_Biomes.csv"
    if biome_path.is_file():
        with biome_path.open(encoding="utf-8-sig", newline="") as handle:
            biome_ids = {row.get("BiomeId", "") for row in csv.DictReader(handle)}
    for row in rows:
        label = row.get("Name", "<unknown>")
        try:
            orders.append(int(row.get("DexOrder", "0")))
        except ValueError:
            errors.append(f"{label} has non-numeric DexOrder")
        species_tags.append(row.get("SpeciesTag", ""))
        names.append(row.get("SpeciesName", ""))
        searchable = row.get("SpeciesName", "") + " " + row.get("AnatomyConcept", "")
        if any(re.search(rf"(?<![A-Za-z0-9]){re.escape(forbidden)}(?![A-Za-z0-9])", searchable, re.IGNORECASE) for forbidden in FORBIDDEN_NAMES):
            errors.append(f"{label} contains forbidden external/IP-associated terminology")
        if "original" not in row.get("AnatomyConcept", "").lower() or "palette variant" in row.get("AnatomyConcept", "").lower() and "not a palette variant" not in row.get("AnatomyConcept", "").lower():
            errors.append(f"{label} lacks original anatomy declaration")
        if row.get("PrimaryElement") not in VALID_ELEMENTS:
            errors.append(f"{label} has invalid element {row.get('PrimaryElement')}")
        if row.get("Role") not in VALID_ROLES:
            errors.append(f"{label} has invalid role {row.get('Role')}")
        if row.get("HabitatBiomeTag") not in biome_ids:
            errors.append(f"{label} references missing habitat biome {row.get('HabitatBiomeTag')}")
        try:
            for value in ("BaseMaxHealth", "BaseAttackPower", "BaseDefensePower", "BaseStamina", "BaseWalkSpeed", "BaseRunSpeed", "CaptureDifficultyModifier"):
                if float(row.get(value, "0")) <= 0.0:
                    errors.append(f"{label} has non-positive {value}")
        except ValueError:
            errors.append(f"{label} has non-numeric base stat")
        for field in ARRAY_FIELDS:
            values = split_array(row.get(field, ""))
            if not values:
                errors.append(f"{label} has empty {field}")
            if field in ("ActiveSkillTags", "ActiveSkillElementTags", "ActiveSkillCooldowns", "ActiveSkillDamageMultipliers", "ActiveSkillTelegraphs") and len(values) != 3:
                errors.append(f"{label} must have exactly 3 {field}, found {len(values)}")
            if field == "WorkSuitabilityLevels":
                if len(values) != 12:
                    errors.append(f"{label} must have exactly 12 work levels, found {len(values)}")
                for value in values:
                    try:
                        if not 0 <= int(value) <= 4:
                            errors.append(f"{label} has work level outside 0..4")
                    except ValueError:
                        errors.append(f"{label} has non-numeric work level")
        if len(split_array(row.get("DropItemTags", ""))) != len(split_array(row.get("DropItemQuantities", ""))):
            errors.append(f"{label} drop arrays are not aligned")
    if sorted(orders) != list(range(1, 201)):
        errors.append("DexOrder values must be exactly 1 through 200")
    if len(set(species_tags)) != len(species_tags):
        errors.append("SpeciesTag values must be unique")
    if len(set(names)) != len(names):
        errors.append("SpeciesName values must be unique")
    if errors:
        print("ASTRAWILD master Echo validation failed:")
        for error in errors:
            print(f"- {error}")
        return 1
    print("ASTRAWILD master Echo validation passed (200 unique rows, 3 skills and 12 work levels per row).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
