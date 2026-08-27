"""Generate the ASTRAWILD 200-species master lexicon.

The legacy DT_EchoDex.csv remains the compatibility table for the current
vertical slice. This generator creates DT_EchoDex_200.csv with a new
FAstrawildMasterEchoRow schema, preserving old row names and saves while making
all 200 master entries available for a later Editor/DataAsset migration.
"""
from __future__ import annotations

import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "Content/Astrawild/Data/Source/DT_EchoDex_200.csv"

FAMILIES: list[tuple[str, list[str], str, str, str, str]] = [
    ("PrimordialMegafauna", ["Terradon", "Glacisaur", "Ignisaur", "Titanus", "Verdantor", "Craghorn", "Thunderhide", "Stegocryst", "Frostbrunt", "Cinderbeast", "Gravegorg", "Amberhorn", "Spirebehemoth", "Mossback", "Ironhide", "Rootcolossus", "Quakehorn", "Dawnmammoth", "Emberback", "Stonegrazer"], "Geo", "Combat", "Biome.ScorchedObsidianCaldera", "HerdHerbivore"),
    ("ApexPredators", ["Volthound", "Cinderclaw", "Umbralon", "Frostfox", "Dreadstalker", "Pyrehound", "Gladefang", "Stormstalker", "Bloodjaw", "Dunehunter", "Gloomclaw", "Frostlynx", "Obsidianfang", "Tempestwolf", "Cavernstalker", "Ashprowler", "Mirepanther", "Riftjackal", "Thornlion", "Nightpelt", "Voltlynx", "Scorchmaul", "Duskhunter", "Boulderfang", "Rainstalker"], "Abyssal", "Combat", "Biome.SylvanRainforest", "PackHunter"),
    ("AvianWyverns", ["Zephyros", "Voltrix", "Solarix", "Cryovex", "Aquaphoenix", "Shadowcrow", "Dreadfalcon", "Stormswallow", "Magmapen", "Galeclaw", "Tempestbird", "Mistgryphon", "Suncondor", "Nightowl", "Cloudray", "Emberkite", "Frostwing", "Razorcrest", "Astraheron", "Voltwing", "Cinderkite", "Skydrake", "Moonsoar", "Galegryph", "Dawnroc"], "Volt", "Exploration", "Biome.DawnMeadows", "SolitarySkyHunter"),
    ("AquaticLeviathans", ["Leviathan", "Aquavine", "Torrentail", "Depthmaw", "Frostfin", "Shockeel", "Mudcroc", "Coralclad", "Toxictoad", "Geyserspout", "Abyssalray", "Riverotter", "Tsunamishark", "Sporefrog", "Glaciertoad", "Reefstalker", "Brineback", "Kelpdrifter", "Miregator", "Tidecrawler", "Siltwhale", "Moonjelly", "Stormray", "Pearlmaw", "Dewrunner"], "Torrent", "BaseUtility", "Biome.SylvanRainforest", "SchoolingOmnivore"),
    ("ChitinousArthropods", ["Dreadmantis", "Sparkwasp", "Terrascarab", "Cinderant", "Frostmoth", "Venomweaver", "Lumifly", "Armorscorp", "Chlorophyte", "Dynastoceros", "Sporeweevil", "Dunecrawler", "Magmacaterpillar", "Abyssalsentinel", "Razorbeetle", "Miretick", "Crystalcicada", "Emberhornet", "Glacialgrub", "Voltbeetle", "Rootweevil", "Ashsilkworm", "Gloomhopper", "Ironmantid", "Astrapupa"], "Abyssal", "BaseUtility", "Biome.ScorchedObsidianCaldera", "HiveSwarm"),
    ("FloraFungiSpirits", ["Lambit", "MischiefCat", "Sporecap", "Florafox", "Treantling", "Bloomdeer", "Cactusbrawler", "Bramblewolf", "Shroombeast", "Sunpetal", "Groveranger", "Rootgolem", "Sproutling", "Thornhare", "Mossmender", "Funglow", "Vinewarden", "Petalimp", "Barksprite", "Dawnshroom"], "Neutral", "BaseUtility", "Biome.DawnMeadows", "DocileHerd"),
    ("AncientGolemsRelics", ["GeoTitan", "Electromon", "Voidcaller", "ClockworkHound", "AstraSentinel", "ForgeGolem", "FrostAutomaton", "RuinCrusher", "TeslaWard", "ChronoScout", "ObsidianOracle", "Magnetarch", "Sunforged", "Cryovault", "RiftEngine", "StoneArchivist", "DawnGearling", "VoltMonolith", "AetherClock", "LumenColossus"], "Geo", "BaseUtility", "Biome.ScorchedObsidianCaldera", "SolitaryGuardian"),
    ("ReptilesDrakesHydras", ["Basilisk", "MagmaDrake", "Hydra", "ThunderScale", "GlacialSerpent", "SandWyrm", "VenomGila", "SolarChameleon", "AbyssalHydra", "SpineDrake", "ObsidianCobra", "FrostBasilisk", "VoltRaptor", "CanyonDrake", "MireHydra", "AshBasilisk", "CrystalViper", "StormDrake", "DawnHydra", "RuinSerpent"], "Solar", "Combat", "Biome.ScorchedObsidianCaldera", "SolitaryApex"),
    ("FusionOffspring", ["Steamwyrm", "Magnetite", "EclipseFox", "StormRoc", "Volcanodon", "FrostThorn", "ElectroPlant", "ShadowPhoenix", "MagmaHydra", "CrystalYeti", "PlasmaWolf", "VoidWyrm", "MudBehemoth", "SolarMantis", "GlacialGolem"], "Astra", "Combat", "Biome.DawnMeadows", "HybridSolitary"),
    ("CelestialOverlords", ["SolarixAlpha", "LeviathanPrime", "IgnisaurOverlord", "CryovexGlaciar", "AstralosTheFirstDawn"], "Astra", "Combat", "Biome.DawnMeadows", "RaidSovereign"),
]

HEADERS = [
    "Name", "DexOrder", "SpeciesTag", "SpeciesName", "SpeciesTitle", "AnatomyConcept", "Diet", "SocialBehavior", "Temperament", "HabitatBiomeTag", "ActivityCycleTag", "PrimaryElement", "ElementalAffinities", "Role", "BaseMaxHealth", "BaseAttackPower", "BaseDefensePower", "BaseStamina", "BaseWalkSpeed", "BaseRunSpeed", "CaptureDifficultyModifier", "WorkSuitabilityLevels", "WorkSuitabilityTags", "PassiveTraitTags", "ActiveSkillTags", "ActiveSkillElementTags", "ActiveSkillCooldowns", "ActiveSkillDamageMultipliers", "ActiveSkillTelegraphs", "PartnerSkillTag", "MountedWeaponTag", "DropItemTags", "DropItemQuantities", "ParentSpeciesA", "ParentSpeciesB"
]

ELEMENTS = ("Neutral", "Solar", "Torrent", "Geo", "Volt", "Glacial", "Abyssal", "Astra")
WORK_TAGS = ("Work.Mining", "Work.Gathering", "Work.Farming", "Work.Kindling", "Work.Building", "Work.Cooling", "Work.Crafting", "Work.Transport", "Work.Generator", "Work.Medicine", "Work.Watering", "Work.Handiwork")
SPECIAL_PARENT_PAIRS = {
    "Steamwyrm": ("Echo.Pyrelite", "Echo.Aquavine"),
    "Magnetite": ("Echo.Thornback", "Echo.Sparkfin"),
    "EclipseFox": ("Echo.Frostfox", "Echo.Pyrelite"),
    "StormRoc": ("Echo.Voltrix", "Echo.Zephyros"),
    "Volcanodon": ("Echo.Terradon", "Echo.Ignisaur"),
    "FrostThorn": ("Echo.Glacisaur", "Echo.Thornback"),
}
SPECIAL_SIGNATURES = {
    "Steamwyrm": "Skill.SuperheatedScaldFog",
    "Magnetite": "Skill.RailgunCrystalSpike",
    "EclipseFox": "Skill.ThermalShockExplosion",
    "StormRoc": "Skill.TornadoLightningStrike",
    "Volcanodon": "Skill.VolcanicEruptionStomp",
    "SolarixAlpha": "Skill.SupernovaCleave",
    "LeviathanPrime": "Skill.MaelstromTsunami",
    "IgnisaurOverlord": "Skill.CataclysmicEarthbreak",
    "CryovexGlaciar": "Skill.AbsoluteZeroFreeze",
    "AstralosTheFirstDawn": "Skill.CelestialRadiance",
}


def tag_list(values: list[str]) -> str:
    return "(" + ",".join(values) + ")"


def element_pair(primary: str, order: int, family_index: int) -> tuple[str, list[str]]:
    if order % 5 == 0:
        secondary = ELEMENTS[(family_index + order) % len(ELEMENTS)]
        if secondary == primary:
            secondary = "Neutral"
        return primary, [primary, secondary]
    return primary, [primary]


def make_row(order: int, name: str, family_index: int, family_name: str, primary: str, role: str, biome: str, social: str) -> dict[str, str | int | float]:
    element, affinities = element_pair(primary, order, family_index)
    if family_name == "FusionOffspring":
        element = ("Solar", "Geo", "Glacial", "Volt", "Abyssal", "Torrent")[order % 6]
        affinities = [element, "Astra" if order % 2 else "Neutral"]
    if family_name == "CelestialOverlords":
        exact = {"SolarixAlpha": "Solar", "LeviathanPrime": "Torrent", "IgnisaurOverlord": "Solar", "CryovexGlaciar": "Glacial", "AstralosTheFirstDawn": "Astra"}
        element = exact[name]
        affinities = [element, "Astra"] if element != "Astra" else ["Astra"]

    tier = 1.0 + (order - 1) / 199.0 * 4.0
    is_boss = family_name == "CelestialOverlords"
    boss_hp = {"SolarixAlpha": 45000, "LeviathanPrime": 85000, "IgnisaurOverlord": 140000, "CryovexGlaciar": 200000, "AstralosTheFirstDawn": 350000}
    health = boss_hp.get(name, round(180.0 * (1.0 + tier * tier * 0.8), 1))
    attack = round(18.0 + tier * (8.0 if not is_boss else 14.0), 1)
    defense = round(12.0 + tier * (7.0 if not is_boss else 10.0), 1)
    stamina = round(90.0 + tier * 30.0, 1)
    walk = round(180.0 + (order % 9) * 13.0, 1)
    run = round(walk * (1.65 if family_name in {"ApexPredators", "AvianWyverns"} else 1.45), 1)
    level_a = 1 + (order + family_index) % 4
    work_levels = [max(0, min(4, level_a if index in ((order + family_index) % 12, (order * 3 + family_index) % 12) else (1 if role == "BaseUtility" and index % 4 == family_index % 4 else 0))) for index in range(12)]
    active_tags = [f"Skill.{name}.Signature", f"Skill.{name}.Utility", f"Skill.{name}.Ultimate"]
    signature = SPECIAL_SIGNATURES.get(name, f"Skill.{name}.Signature")
    active_tags[0] = signature
    element_tags = [f"Element.{element}", f"Element.{affinities[-1]}", f"Element.{element}"]
    cooldowns = [round(2.0 + (order % 5) * 0.5, 2), round(5.0 + (order % 7) * 0.8, 2), round(12.0 + (order % 9) * 1.5, 2)]
    damage = [round(0.65 + tier * 0.10, 2), round(0.90 + tier * 0.12, 2), round(1.40 + tier * 0.18, 2)]
    telegraphs = [round(0.20 + (order % 4) * 0.05, 2), round(0.45 + (order % 3) * 0.10, 2), round(0.90 + (order % 4) * 0.15, 2)]
    drops = ["Item.Meat", "Item.Fiber", f"Item.{element}Organ"]
    quantities = [str(1 + order % 3), str(2 + order % 5), str(1 + order % 2)]
    if is_boss:
        drops.append("Item.AncientAstraSchematic")
        quantities.append("1")
    parents = SPECIAL_PARENT_PAIRS.get(name, ("", ""))
    mount = f"MountWeapon.{name}" if role in {"Combat", "Exploration"} else ""
    return {
        "Name": f"MasterEcho_{order:03d}_{name}",
        "DexOrder": order,
        "SpeciesTag": f"Echo.{name}",
        "SpeciesName": name.replace("TheFirstDawn", " The First Dawn"),
        "SpeciesTitle": f"{family_name} {social}",
        "AnatomyConcept": f"Original {family_name.lower()} anatomy built around {name.lower()} ecological silhouette; not a palette variant.",
        "Diet": "Herbivore" if role == "BaseUtility" else ("Omnivore" if order % 3 == 0 else "Carnivore"),
        "SocialBehavior": social,
        "Temperament": "Territorial" if role == "Combat" else ("Curious" if role == "Exploration" else "Docile"),
        "HabitatBiomeTag": biome,
        "ActivityCycleTag": "Activity.Nocturnal" if order % 4 == 0 else ("Activity.Diurnal" if order % 2 else "Activity.Crepscular"),
        "PrimaryElement": element,
        "ElementalAffinities": tag_list(affinities),
        "Role": role,
        "BaseMaxHealth": health,
        "BaseAttackPower": attack,
        "BaseDefensePower": defense,
        "BaseStamina": stamina,
        "BaseWalkSpeed": walk,
        "BaseRunSpeed": run,
        "CaptureDifficultyModifier": round(0.85 + tier * 0.10, 2),
        "WorkSuitabilityLevels": tag_list([str(value) for value in work_levels]),
        "WorkSuitabilityTags": tag_list([WORK_TAGS[index] for index, value in enumerate(work_levels) if value > 0]),
        "PassiveTraitTags": tag_list([f"Trait.{('Alpha' if is_boss else 'Sturdy')}", f"Trait.{('Mythic' if tier >= 4.2 else 'Alert')}" ]),
        "ActiveSkillTags": tag_list(active_tags),
        "ActiveSkillElementTags": tag_list(element_tags),
        "ActiveSkillCooldowns": tag_list([str(value) for value in cooldowns]),
        "ActiveSkillDamageMultipliers": tag_list([str(value) for value in damage]),
        "ActiveSkillTelegraphs": tag_list([str(value) for value in telegraphs]),
        "PartnerSkillTag": f"Partner.{name}",
        "MountedWeaponTag": mount,
        "DropItemTags": tag_list(drops),
        "DropItemQuantities": tag_list(quantities),
        "ParentSpeciesA": parents[0],
        "ParentSpeciesB": parents[1],
    }


def generate() -> None:
    rows: list[dict[str, str | int | float]] = []
    order = 1
    for family_index, (family_name, names, primary, role, biome, social) in enumerate(FAMILIES):
        for name in names:
            rows.append(make_row(order, name, family_index, family_name, primary, role, biome, social))
            order += 1
    if len(rows) != 200:
        raise RuntimeError(f"Expected 200 master Echo rows, generated {len(rows)}")
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=HEADERS)
        writer.writeheader()
        writer.writerows(rows)
    print(f"Generated {len(rows)} rows: {OUTPUT}")


if __name__ == "__main__":
    generate()
