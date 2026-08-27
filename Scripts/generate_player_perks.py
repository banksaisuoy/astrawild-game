"""Generate the 50-entry ASTRAWILD player perk tree."""
from __future__ import annotations

import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "Content/Astrawild/Data/Source/DT_PlayerPerks.csv"
HEADERS = ["Name", "PerkTag", "DisplayName", "Description", "Tier", "PrerequisitePerkTags", "StatType", "StatBonus", "SprintStaminaMultiplier", "FoodNutritionMultiplier", "RepairRefundMultiplier", "CaptureOddsBonus", "ReloadSpeedMultiplier", "CriticalDamageMultiplier"]
BASE = [
    ("MarathonRunner", "Marathon Runner", "Reduces sprint stamina drain.", "0.70", "1", "0", "0", "1", "1"),
    ("IronStomach", "Iron Stomach", "Improves nutrition from every meal.", "1", "1.50", "0", "0", "1", "1"),
    ("MasterBlacksmith", "Master Blacksmith", "Returns more resources when repairing gear.", "1", "1", "1", "0", "1", "1"),
    ("BeastWhisperer", "Beast Whisperer", "Improves base capture odds.", "1", "1", "0", "0.25", "1", "1"),
    ("Gunslinger", "Gunslinger", "Improves ranged reloads and critical damage.", "1", "1", "0", "0", "1.20", "1.20"),
]
STAT_PERKS = [
    ("VitalCore", "Vital Core", "Adds maximum health.", "MaxHealth", "100"),
    ("IronLungs", "Iron Lungs", "Adds maximum stamina.", "MaxStamina", "50"),
    ("PowerGrip", "Power Grip", "Adds attack power.", "AttackPower", "2"),
    ("ColonyForeman", "Colony Foreman", "Adds work speed.", "WorkSpeed", "50"),
    ("PackMule", "Pack Mule", "Adds weight capacity.", "WeightCapacity", "50"),
    ("VitalCoreII", "Vital Core II", "Adds more maximum health.", "MaxHealth", "150"),
    ("IronLungsII", "Iron Lungs II", "Adds more maximum stamina.", "MaxStamina", "75"),
    ("PowerGripII", "Power Grip II", "Adds more attack power.", "AttackPower", "3"),
    ("ColonyForemanII", "Colony Foreman II", "Adds more work speed.", "WorkSpeed", "75"),
    ("PackMuleII", "Pack Mule II", "Adds more weight capacity.", "WeightCapacity", "75"),
    ("VitalCoreIII", "Vital Core III", "Adds substantial maximum health.", "MaxHealth", "250"),
    ("IronLungsIII", "Iron Lungs III", "Adds substantial maximum stamina.", "MaxStamina", "100"),
    ("PowerGripIII", "Power Grip III", "Adds substantial attack power.", "AttackPower", "5"),
    ("ColonyForemanIII", "Colony Foreman III", "Adds substantial work speed.", "WorkSpeed", "100"),
    ("PackMuleIII", "Pack Mule III", "Adds substantial weight capacity.", "WeightCapacity", "100"),
]
MOD_PERKS = [
    ("TrailSense", "Trail Sense", "Improves exploration movement efficiency.", "MoveSpeed", "0.08"),
    ("ColdBlooded", "Cold Blooded", "Improves resistance to heat exposure.", "HeatResistance", "0.12"),
    ("WarmHeart", "Warm Heart", "Improves resistance to cold exposure.", "ColdResistance", "0.12"),
    ("FieldMedic", "Field Medic", "Improves health recovery from food.", "HealthRecovery", "0.15"),
    ("NightScout", "Night Scout", "Improves nighttime awareness.", "NightVision", "1"),
    ("EfficientCook", "Efficient Cook", "Improves nutrition output from cooked meals.", "FoodNutritionMultiplier", "1.15"),
    ("QuickHands", "Quick Hands", "Improves reload speed.", "ReloadSpeedMultiplier", "1.15"),
    ("CriticalEye", "Critical Eye", "Improves critical damage.", "CriticalDamageMultiplier", "1.15"),
    ("ResonanceStudent", "Resonance Student", "Improves capture odds against weakened Echoes.", "CaptureOddsBonus", "0.08"),
    ("ResonanceMaster", "Resonance Master", "Further improves capture odds.", "CaptureOddsBonus", "0.12"),
    ("BuilderMind", "Builder Mind", "Reduces building material waste.", "RepairRefundMultiplier", "0.20"),
    ("StormRunner", "Storm Runner", "Improves movement in weather hazards.", "WeatherMoveSpeed", "0.10"),
    ("MinerFocus", "Miner Focus", "Improves mining output.", "MiningYield", "0.15"),
    ("GreenThumb", "Green Thumb", "Improves crop yield.", "FarmingYield", "0.15"),
    ("EchoEmpathy", "Echo Empathy", "Improves companion trust gain.", "TrustGain", "0.20"),
    ("PackTactics", "Pack Tactics", "Improves damage while a companion is active.", "CompanionDamage", "0.10"),
    ("ShieldBearer", "Shield Bearer", "Improves damage mitigation while mounted.", "MountedDefense", "0.12"),
    ("AstraScholar", "Astra Scholar", "Improves experience gained from discoveries.", "DiscoveryEXP", "0.20"),
    ("BreederPatience", "Breeder Patience", "Improves breeding trait inheritance.", "TraitInheritance", "0.10"),
    ("SanctuaryKeeper", "Sanctuary Keeper", "Improves Echo SAN recovery at camp.", "SANRecovery", "0.18"),
    ("LastStand", "Last Stand", "Improves defense at low health.", "LowHealthDefense", "0.20"),
    ("DodgeRhythm", "Dodge Rhythm", "Reduces dodge stamina cost.", "DodgeStamina", "0.20"),
    ("HarvestSurge", "Harvest Surge", "Improves resource gathering speed.", "GatherSpeed", "0.18"),
    ("Logistics", "Logistics", "Improves transport capacity for colony tasks.", "TransportCapacity", "0.25"),
    ("PowerTechnician", "Power Technician", "Improves powered machine efficiency.", "PowerEfficiency", "0.15"),
    ("AstraArmorer", "Astra Armorer", "Improves equipment durability.", "DurabilityLoss", "-0.20"),
    ("TowerVeteran", "Tower Veteran", "Improves damage against tower bosses.", "BossDamage", "0.10"),
    ("DawnChampion", "Dawn Champion", "Improves all core combat output.", "AllCoreCombat", "0.08"),
    ("VoidWalker", "Void Walker", "Improves Abyssal hazard resistance.", "AbyssalResistance", "0.20"),
    ("ElementalScholar", "Elemental Scholar", "Improves elemental effect duration.", "StatusDuration", "0.18"),
]


def generate() -> None:
    rows = []
    for index, (slug, display, description, sprint, food, refund, capture, reload, crit) in enumerate(BASE, start=1):
        rows.append({"Name": f"Perk_{index:02d}_{slug}", "PerkTag": f"Perk.{slug}", "DisplayName": display, "Description": description, "Tier": 1, "PrerequisitePerkTags": "()", "StatType": "MaxHealth", "StatBonus": 0, "SprintStaminaMultiplier": sprint, "FoodNutritionMultiplier": food, "RepairRefundMultiplier": refund, "CaptureOddsBonus": capture, "ReloadSpeedMultiplier": reload, "CriticalDamageMultiplier": crit})
    for index, (slug, display, description, stat, bonus) in enumerate(STAT_PERKS, start=6):
        tier = 2 if slug.endswith("II") else (3 if slug.endswith("III") else 1)
        previous = slug[:-2] if slug.endswith("II") else (slug[:-3] + "II" if slug.endswith("III") else "")
        rows.append({"Name": f"Perk_{index:02d}_{slug}", "PerkTag": f"Perk.{slug}", "DisplayName": display, "Description": description, "Tier": tier, "PrerequisitePerkTags": f"(Perk.{previous})" if previous else "()", "StatType": stat, "StatBonus": bonus, "SprintStaminaMultiplier": 1, "FoodNutritionMultiplier": 1, "RepairRefundMultiplier": 0, "CaptureOddsBonus": 0, "ReloadSpeedMultiplier": 1, "CriticalDamageMultiplier": 1})
    for index, (slug, display, description, modifier, magnitude) in enumerate(MOD_PERKS, start=21):
        rows.append({"Name": f"Perk_{index:02d}_{slug}", "PerkTag": f"Perk.{slug}", "DisplayName": display, "Description": description, "Tier": 2 if index < 36 else 3, "PrerequisitePerkTags": "()", "StatType": "MaxHealth", "StatBonus": 0, "SprintStaminaMultiplier": 1, "FoodNutritionMultiplier": 1, "RepairRefundMultiplier": 0, "CaptureOddsBonus": 0, "ReloadSpeedMultiplier": 1, "CriticalDamageMultiplier": 1})
        if modifier == "FoodNutritionMultiplier": rows[-1]["FoodNutritionMultiplier"] = magnitude
        elif modifier == "RepairRefundMultiplier": rows[-1]["RepairRefundMultiplier"] = magnitude
        elif modifier == "CaptureOddsBonus": rows[-1]["CaptureOddsBonus"] = magnitude
        elif modifier == "ReloadSpeedMultiplier": rows[-1]["ReloadSpeedMultiplier"] = magnitude
        elif modifier == "CriticalDamageMultiplier": rows[-1]["CriticalDamageMultiplier"] = magnitude
    if len(rows) != 50:
        raise RuntimeError(f"Expected 50 perks, generated {len(rows)}")
    with OUTPUT.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=HEADERS)
        writer.writeheader()
        writer.writerows(rows)
    print(f"Generated {len(rows)} player perks: {OUTPUT}")


if __name__ == "__main__":
    generate()
