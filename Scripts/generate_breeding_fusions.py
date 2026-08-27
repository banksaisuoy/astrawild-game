"""Generate validated, deterministic parent-pair fusion rows for ASTRAWILD."""
from __future__ import annotations

import csv
from itertools import combinations
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Content/Astrawild/Data/Source"
MASTER = SOURCE / "DT_EchoDex_200.csv"
OUTPUT = SOURCE / "DT_BreedingFusions.csv"

HEADERS = [
    "Name", "ParentSpeciesA", "ParentSpeciesB", "OffspringSpeciesTag", "OffspringElementalAffinities", "GuaranteedInheritedTraitTags", "TraitInheritanceChance", "HiddenPassiveUnlockChance", "FusionGroupTag"
]


def split_array(value: str) -> list[str]:
    value = value.strip().strip("()")
    return [item.strip() for item in value.split(",") if item.strip()]


def generate() -> None:
    with MASTER.open(encoding="utf-8-sig", newline="") as handle:
        master_rows = list(csv.DictReader(handle))
    species = [row["SpeciesTag"] for row in master_rows]
    elements = {row["SpeciesTag"]: split_array(row["ElementalAffinities"]) for row in master_rows}
    traits = {row["SpeciesTag"]: split_array(row["PassiveTraitTags"]) for row in master_rows}
    offspring = [row["SpeciesTag"] for row in master_rows if 181 <= int(row["DexOrder"]) <= 195]
    pairs = list(combinations(species[:80], 2))[:120]
    rows = []
    for index, (parent_a, parent_b) in enumerate(pairs, start=1):
        child = offspring[(index - 1) % len(offspring)]
        inherited = []
        for trait in traits[parent_a] + traits[parent_b]:
            if trait not in inherited:
                inherited.append(trait)
        affinity = []
        for element in elements[parent_a] + elements[parent_b]:
            if element not in affinity:
                affinity.append(element)
        rows.append({
            "Name": f"Fusion_{index:03d}",
            "ParentSpeciesA": parent_a,
            "ParentSpeciesB": parent_b,
            "OffspringSpeciesTag": child,
            "OffspringElementalAffinities": "(" + ",".join(affinity[:2]) + ")",
            "GuaranteedInheritedTraitTags": "(" + ",".join(inherited[:2]) + ")",
            "TraitInheritanceChance": f"{0.75 + (index % 5) * 0.05:.2f}",
            "HiddenPassiveUnlockChance": f"{0.03 + (index % 8) * 0.01:.2f}",
            "FusionGroupTag": f"Fusion.Group.{((index - 1) % 10) + 1:02d}",
        })
    if len(rows) != 120 or len({(row["ParentSpeciesA"], row["ParentSpeciesB"]) for row in rows}) != 120:
        raise RuntimeError("Fusion generation did not produce 120 unique ordered parent combinations")
    with OUTPUT.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=HEADERS)
        writer.writeheader()
        writer.writerows(rows)
    print(f"Generated {len(rows)} fusion rows: {OUTPUT}")


if __name__ == "__main__":
    generate()
