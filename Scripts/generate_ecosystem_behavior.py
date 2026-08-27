"""Generate one ecosystem behavior row for every master Echo."""
from __future__ import annotations

import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Content/Astrawild/Data/Source"
MASTER = SOURCE / "DT_EchoDex_200.csv"
OUTPUT = SOURCE / "DT_EcosystemBehavior.csv"
HEADERS = ["Name", "SpeciesTag", "Temperament", "DietTag", "SocialGroupTag", "PerceptionRadius", "TerritoryRadius", "HungerSecondsUntilForage", "FleeHealthThreshold", "DefendHealthThreshold", "bCanMigrateDuringWorldEvents", "bFormsGroups", "bDefendsYoung"]


def generate() -> None:
    with MASTER.open(encoding="utf-8-sig", newline="") as handle:
        master_rows = list(csv.DictReader(handle))
    rows = []
    for index, master in enumerate(master_rows, start=1):
        role = master["Role"]
        if role == "BaseUtility":
            temperament = "Docile" if index % 3 else "Curious"
            diet = "Diet.Herbivore" if index % 2 else "Diet.Omnivore"
            group = f"Group.Herd.{((index - 1) % 8) + 1:02d}"
            forms_groups = True
        elif role == "Exploration":
            temperament = "NocturnalScavenger" if index % 4 == 0 else "Curious"
            diet = "Diet.Omnivore"
            group = f"Group.Flock.{((index - 1) % 6) + 1:02d}"
            forms_groups = index % 4 != 0
        else:
            temperament = "SolitaryApex" if index % 5 == 0 else ("PackHunter" if index % 2 else "Territorial")
            diet = "Diet.Carnivore"
            group = f"Group.Pack.{((index - 1) % 7) + 1:02d}"
            forms_groups = temperament == "PackHunter"
        rows.append({
            "Name": f"Ecosystem_{index:03d}_{master['SpeciesName'].replace(' ', '')}",
            "SpeciesTag": master["SpeciesTag"],
            "Temperament": temperament,
            "DietTag": diet,
            "SocialGroupTag": group,
            "PerceptionRadius": 2200 + (index % 7) * 250,
            "TerritoryRadius": 700 + (index % 8) * 150,
            "HungerSecondsUntilForage": 600 + (index % 6) * 90,
            "FleeHealthThreshold": f"{0.18 + (index % 5) * 0.03:.2f}",
            "DefendHealthThreshold": f"{0.45 + (index % 4) * 0.05:.2f}",
            "bCanMigrateDuringWorldEvents": "true" if index % 3 == 0 else "false",
            "bFormsGroups": "true" if forms_groups else "false",
            "bDefendsYoung": "true" if forms_groups and index % 5 != 0 else "false",
        })
    if len(rows) != 200:
        raise RuntimeError(f"Expected 200 ecosystem rows, generated {len(rows)}")
    with OUTPUT.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=HEADERS)
        writer.writeheader()
        writer.writerows(rows)
    print(f"Generated {len(rows)} ecosystem rows: {OUTPUT}")


if __name__ == "__main__":
    generate()
