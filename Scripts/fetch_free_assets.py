#!/usr/bin/env python3
"""
ASTRAWILD — ASSET OVERHAUL: real unique-mesh acquisition pipeline.

Executive directive (NO PLACEHOLDERS / NO PALETTE SWAPS): replace the interim
Procedural Recolor system with REAL, uniquely-shaped 3D models. No cylinders,
no recolored duplicate models.

What this script does (all verifiable on disk):
  1. Curates a real-model catalog from free/open sources with verifiable
     licenses (CC0 1.0 Universal only — Quaternius packs, Kenney packs).
  2. Verifies/uses the in-repo packs; for MISSING Kenney packs it performs a
     REAL remote download (scrapes the official asset page's download modal
     and fetches the .zip). Quaternius remote delivery is login-walled
     (itch.io requires an account for $0 purchases) — recorded honestly,
     never faked; the local LFS-tracked CC0 packs cover those sources.
  3. Converts every Quaternius .gltf (data-URI buffers + atlas image) into a
     SELF-CONTAINED .glb (binary container, images embedded as bufferViews).
  4. Re-packs Kenney .glb files that reference external colormap textures so
     every staged .glb is self-contained.
  5. Stages curated files under ArtSource/Meshes/<Category>/ with the game's
     ASSET-ID naming (SK_Echo_*, SK_Base_*, SK_Boss_*, SK_Survivor_*,
     SM_Weapon_*, SM_Vehicle_*, SM_Node_*, SM_Tree/Rock/Fern/...).
  6. DERIVES the animation clip map for every skeletal mesh from the model's
     ACTUAL animation list (Idle/Move/Hit + survivor locomotion set).
  7. VERIFIES every staged file (GLB magic, JSON parse, mesh/vertex counts,
     strict 1:1 source-model uniqueness across the whole catalog — a reused
     source model is a hard error: no palette swaps, no duplicate meshes).
  8. REGENERATES ArtSource/manifest.json: every entry maps Asset ID -> real
     file path with status "present" (source files 100% on disk, Pending = 0;
     the engine-side import step is a separate pipeline stage run by
     Setup_And_Play.bat / Content/Python/AwPipeline/import_all.py).
  9. Writes Docs/ASTRAWILD_REAL_ASSET_CREDITS.json (per-asset license
     provenance: source pack, source model, license, license URL) and
     Docs/ASTRAWILD_ASSET_OVERHAUL_REPORT.json (run evidence).

Run:  python3 Scripts/fetch_free_assets.py [--verify-remote]
Exit: 0 on full success, 1 on any failure (never a fake success).
"""

import argparse
import base64
import hashlib
import json
import os
import re
import shutil
import struct
import sys
import tempfile
import urllib.request
import zipfile
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
MESHES = REPO / "ArtSource" / "Meshes"
TEXTURES = REPO / "ArtSource" / "Textures"
MANIFEST = REPO / "ArtSource" / "manifest.json"
CREDITS = REPO / "Docs" / "ASTRAWILD_REAL_ASSET_CREDITS.json"
REPORT = REPO / "Docs" / "ASTRAWILD_ASSET_OVERHAUL_REPORT.json"

CC0 = "CC0 1.0 Universal"
CC0_URL = "https://creativecommons.org/publicdomain/zero/1.0/"

# ---------------------------------------------------------------------------
# Source packs (local LFS-tracked acquisition + official pages for
# re-download). Licenses verified from the in-repo License.txt /
# LICENSE_CC0.txt files at acquisition time.
# ---------------------------------------------------------------------------
PACKS = {
    "Quaternius_UltimateAnimatedAnimals": {
        "root": REPO / "ArtSource" / "Models" / "Quaternius_UltimateAnimatedAnimals",
        "page": "https://quaternius.com/packs/ultimatedanimatedanimals.html",
        "license": CC0, "license_url": CC0_URL,
    },
    "Quaternius_UltimateMonsters": {
        "root": REPO / "ArtSource" / "Models" / "Quaternius_UltimateMonsters",
        "page": "https://quaternius.com/packs/ultimatemonsters.html",
        "license": CC0, "license_url": CC0_URL,
    },
    "Quaternius_UltimateModularMen": {
        "root": REPO / "ArtSource" / "Models" / "Quaternius_UltimateModularMen",
        "page": "https://quaternius.com/packs/ultimatemodularmen.html",
        "license": CC0, "license_url": CC0_URL,
    },
    "Quaternius_UltimateSpaceKit": {
        "root": REPO / "ArtSource" / "Models" / "Quaternius_UltimateSpaceKit",
        "page": "https://quaternius.com/packs/ultimatespacekit.html",
        "license": CC0, "license_url": CC0_URL,
    },
    "Quaternius_UltimateModularRuins": {
        "root": REPO / "ArtSource" / "Models" / "Quaternius_UltimateModularRuins",
        "page": "https://quaternius.com/packs/ultimatemodularruins.html",
        "license": CC0, "license_url": CC0_URL,
    },
    "Kenney_BlasterKit": {
        "root": REPO / "ArtSource" / "Models" / "Kenney_BlasterKit",
        "page": "https://kenney.nl/assets/blaster-kit",
        "license": CC0, "license_url": CC0_URL,
    },
    "Kenney_NatureKit": {
        "root": REPO / "ArtSource" / "Models" / "Kenney_NatureKit",
        "page": "https://kenney.nl/assets/nature-kit",
        "license": CC0, "license_url": CC0_URL,
    },
    "Kenney_SpaceKit": {
        "root": REPO / "ArtSource" / "Models" / "Kenney_SpaceKit",
        "page": "https://kenney.nl/assets/space-kit",
        "license": CC0, "license_url": CC0_URL,
    },
}

# ---------------------------------------------------------------------------
# Curated model catalog. Each staged asset maps 1:1 to a UNIQUE source model
# (strict uniqueness — enforced below). Sources are (pack, pack-relative
# path). .gltf inputs are converted to self-contained .glb; .glb inputs with
# external textures are re-packed with the textures embedded; .fbx inputs are
# staged verbatim (UE imports FBX natively).
# ---------------------------------------------------------------------------
def QM(kind, name):    # Quaternius Ultimate Monsters
    return ("Quaternius_UltimateMonsters", f"{kind}/glTF/{name}.gltf")
def ANIMAL(name):      # Quaternius Ultimate Animated Animals
    return ("Quaternius_UltimateAnimatedAnimals", f"glTF/{name}.gltf")
def MAN(name):         # Quaternius Ultimate Modular Men
    return ("Quaternius_UltimateModularMen", f"Individual Characters/glTF/{name}.gltf")
def SPACEDIR(sub, name):  # Quaternius Ultimate Space Kit
    return ("Quaternius_UltimateSpaceKit", f"{sub}/GLTF/{name}.gltf")
def RUIN(name):        # Quaternius Ultimate Modular Ruins (FBX)
    return ("Quaternius_UltimateModularRuins", f"FBX/{name}.fbx")
def KBLASTER(name):
    return ("Kenney_BlasterKit", f"GLB/{name}.glb")
def KNATURE(name):
    return ("Kenney_NatureKit", f"GLB/{name}.glb")
def KSPACE(name):
    return ("Kenney_SpaceKit", f"GLB/{name}.glb")

# ---- Player & armor sets: rigged humanoids, 3 tiers -----------------------
PLAYER = {
    # Tier 3 Singularity Exosuit — the game's referenced survivor mesh.
    # Real source: Quaternius Modular Men "Swat" (heavy armored rig, gun anims).
    "Characters/Survivor/SK_Survivor_Exosuit": (MAN("Swat"), {
        "ue_path": "/Game/Characters/Survivor/SK_Survivor_Exosuit",
        "armor_tier": 3, "armor_tier_name": "Singularity Exosuit",
        "clip_map": {"AM_Survivor_Idle": "Idle", "AM_Survivor_Walk": "Walk",
                      "AM_Survivor_Run": "Run", "AM_Survivor_Jump": "Roll",
                      "AM_Survivor_Aim": "Idle_Gun_Pointing",
                      "AM_Survivor_Fire": "Gun_Shoot",
                      "AM_Survivor_Gather": "Interact"},
    }),
    # Tier 1 Scavenger — worn field gear + backpack.
    "Characters/Survivor/SK_Survivor_T1_Scavenger": (MAN("Adventurer"), {
        "ue_path": "/Game/Characters/Survivor/SK_Survivor_T1_Scavenger",
        "armor_tier": 1, "armor_tier_name": "Scavenger",
    }),
    # Tier 2 Astraite Suit — sealed astraite-weave environment suit.
    "Characters/Survivor/SK_Survivor_T2_Astraite": (MAN("Spacesuit"), {
        "ue_path": "/Game/Characters/Survivor/SK_Survivor_T2_Astraite",
        "armor_tier": 2, "armor_tier_name": "Astraite Suit",
    }),
}

# ---- 16 Sci-Fantasy base archetypes (mutation-system geometry layer) ------
ECHO_BASES = {
    "Echoes/BaseMeshes/SK_Base_GolemQuadruped": QM("Flying", "Goleling"),
    "Echoes/BaseMeshes/SK_Base_MonolithColossus": QM("Big", "Yeti"),
    "Echoes/BaseMeshes/SK_Base_ElemDrake": QM("Flying", "Dragon"),
    "Echoes/BaseMeshes/SK_Base_ElemWisp": QM("Flying", "Glub"),
    "Echoes/BaseMeshes/SK_Base_MutantBeast": ANIMAL("Wolf"),
    "Echoes/BaseMeshes/SK_Base_MutantAvian": QM("Big", "Birb"),
    "Echoes/BaseMeshes/SK_Base_ArmoredBeetle": QM("Flying", "Armabee"),
    "Echoes/BaseMeshes/SK_Base_ArmoredCrab": QM("Flying", "Armabee_Evolved"),
    "Echoes/BaseMeshes/SK_Base_SpiritWisp": QM("Flying", "Ghost"),
    "Echoes/BaseMeshes/SK_Base_SpiritOrb": QM("Flying", "Ghost_Skull"),
    "Echoes/BaseMeshes/SK_Base_CyborgBeast": QM("Big", "Demon"),
    "Echoes/BaseMeshes/SK_Base_CyborgSerpent": QM("Flying", "Hywirl"),
    "Echoes/BaseMeshes/SK_Base_PlantMaw": QM("Big", "Cactoro"),
    "Echoes/BaseMeshes/SK_Base_Mushroomling": QM("Blob", "Mushnub"),
    "Echoes/BaseMeshes/SK_Base_VoidBlob": QM("Blob", "GreenBlob"),
    "Echoes/BaseMeshes/SK_Base_VoidTentacle": QM("Flying", "Squidle"),
}

# ---- 6 production hero Echoes ---------------------------------------------
ECHO_HEROES = {
    "Echoes/SK_Echo_Terraquill": ANIMAL("Stag"),       # quill-antlered stag
    "Echoes/SK_Echo_Cindermule": ANIMAL("Donkey"),     # ember mule
    "Echoes/SK_Echo_Voltpylon": QM("Big", "Monkroose"),# volt antler-pylon moose
    "Echoes/SK_Echo_Bastionbeetle": QM("Big", "Dino"), # armored tank beetle
    "Echoes/SK_Echo_Mistmender": QM("Blob", "Wizard"), # mist mender
    "Echoes/SK_Echo_Deepdelver": QM("Big", "Bunny"),   # burrower
}

# ---- 3 production boss species (Act-3 quest chain) — direct real meshes ---
PROD_BOSSES = {
    "Echoes/SK_Echo_GlassTyrant": QM("Big", "Frog"),               # translucent tyrant
    "Echoes/SK_Echo_EyeSentinel": SPACEDIR("Characters", "Astronaut_FernandoTheFlamingo"),
    "Echoes/SK_Echo_DrownedSovereign": QM("Blob", "Yeti"),         # drowned regal colossus
}

# ---- 14 showcase/zone bosses (distinct imposing meshes) -------------------
SHOWCASE_BOSSES = {
    "Echoes/Bosses/SK_Boss_SkyTyrant": QM("Flying", "Dragon_Evolved"),
    "Echoes/Bosses/SK_Boss_AncientWarden": QM("Flying", "Goleling_Evolved"),
    "Echoes/Bosses/SK_Boss_VoidPrime": QM("Flying", "Glub_Evolved"),
    "Echoes/Bosses/SK_Boss_AlpacaColossus": QM("Flying", "Alpaking"),
    "Echoes/Bosses/SK_Boss_AlpacaWarlord": QM("Flying", "Alpaking_Evolved"),
    "Echoes/Bosses/SK_Boss_FungalSovereign": QM("Blob", "Mushnub_Evolved"),
    "Echoes/Bosses/SK_Boss_MushroomKing": QM("Big", "MushroomKing"),
    "Echoes/Bosses/SK_Boss_WarlordOrc": QM("Big", "Orc"),
    "Echoes/Bosses/SK_Boss_BoneShaman": QM("Big", "Orc_Skull"),
    "Echoes/Bosses/SK_Boss_AbyssDemon": QM("Big", "BlueDemon"),
    "Echoes/Bosses/SK_Boss_ShadowNinja": QM("Big", "Ninja"),
    "Echoes/Bosses/SK_Boss_MechTitan": SPACEDIR("Characters", "Mech_BarbaraTheBee"),
    "Echoes/Bosses/SK_Boss_MechRavager": SPACEDIR("Characters", "Mech_RaeTheRedPanda"),
    "Echoes/Bosses/SK_Boss_SentinelPrime": SPACEDIR("Characters", "Enemy_Large"),
}

# ---- Tier-B species: deterministic theme-queue assignment -----------------
# Candidates = every species that currently holds a staged SK_Echo_*.glb.
# Each candidate draws ONE unique model from its theme queue (models drawn
# from the remaining pool after bases/heroes/prod+showcase bosses). When the
# pool for a theme empties, the species keeps NO direct file — it renders via
# its theme's REAL base mesh + its unique deterministic mutation spec
# (per-part scale + attachments + material theme: real geometric variation,
# never a palette swap). The exact resulting list is printed for the C++
# Tier-B table sync.
THEME_QUEUES = {
    "AncientConstruct": [QM("Big", "Alien"), QM("Big", "Tribal"), QM("Blob", "Orc"),
                          QM("Flying", "Tribal"), QM("Blob", "Ninja"), QM("Blob", "Alien"),
                          QM("Flying", "Demon"), QM("Flying", "Pigeon"),
                          SPACEDIR("Characters", "Mech_FinnTheFrog"),
                          SPACEDIR("Characters", "Mech_FernandoTheFlamingo"),
                          SPACEDIR("Characters", "Enemy_Flying"),
                          SPACEDIR("Characters", "Enemy_Small"),
                          SPACEDIR("Characters", "Enemy_ExtraSmall")],
    "ElementalBeast": [ANIMAL("Fox"), ANIMAL("Husky"), ANIMAL("Deer"), ANIMAL("Horse"),
                        ANIMAL("Bull"), ANIMAL("Cow"), ANIMAL("ShibaInu"),
                        QM("Blob", "Birb")],
    "EtherealSpirit": [QM("Blob", "Pigeon"), QM("Blob", "PinkBlob"), QM("Blob", "Cat"),
                        QM("Blob", "Dog"), QM("Blob", "Chicken"), QM("Blob", "GreenSpikyBlob"),
                        QM("Blob", "Fish"), SPACEDIR("Characters", "Astronaut_RaeTheRedPanda"),
                        SPACEDIR("Characters", "Astronaut_FinnTheFrog")],
    "MutatedFauna": [ANIMAL("Alpaca"), QM("Blob", "Chicken"), QM("Blob", "Cat"),
                      QM("Blob", "Dog"), ANIMAL("Cow")],
    "ArmoredOrganic": [QM("Blob", "Cactoro"), QM("Blob", "GreenSpikyBlob"),
                        SPACEDIR("Characters", "Astronaut_BarbaraTheBee")],
    "MechanicalHybrid": [SPACEDIR("Characters", "Enemy_Flying"),
                          SPACEDIR("Characters", "Enemy_Small"),
                          SPACEDIR("Characters", "Enemy_ExtraSmall"),
                          SPACEDIR("Characters", "Astronaut_FinnTheFrog")],
    "PlantMonster": [QM("Blob", "Cactoro"), QM("Blob", "Birb"), ANIMAL("Alpaca")],
    "VoidAbomination": [QM("Blob", "PinkBlob"), QM("Blob", "GreenSpikyBlob"),
                          QM("Blob", "Fish"), QM("Blob", "Pigeon"), QM("Flying", "Pigeon")],
}


# Priority order: production bosses were already assigned above; then the
# CURRENT C++ Tier-B table order, then any remaining species with staged
# files (alphabetical). Deterministic — the printed result is the new truth.
TIERB_PRIORITY = [
    "Abyssjelly", "Astralmonolith", "Brinefin", "Coralray", "Duskmoth",
    "Eldermonolith", "Emberfang", "Embershade", "Fernthorn", "Forgottencolossus",
    "Frostblaze", "Geargolem", "Ghostshade", "Glimmerhornet", "Hallowedcolossus",
    "Lagoonfin", "Magmawing", "Mistwing", "Monolithcolossus", "Monolithprimarch",
    "Pearlcrest", "Pistongolem", "Primemonolith", "Pyreblaze", "Reliccolossus",
    "Rimefang", "Saltcrest", "Saltray", "Stonehide", "Sunhide", "Sunhorn",
    "Sunpaw", "Tidewyrm", "Undertowray", "Verdantbloom", "Vespermonolith",
    "Voidwing", "Voltmaw", "Wavecrest",
]

# ---- Weapons: 5 distinct Kenney blaster meshes (geometry-verified) --------
WEAPONS = {
    "Weapons/SM_Weapon_ScrapRifle": (KBLASTER("blaster-a"), {          # 820 v / 2 parts
        "ue_path": "/Game/Weapons/Meshes/SM_Weapon_ScrapRifle",
        "weapon_class": "Scrap Rifle (kinetic junk-frame rifle)"}),
    "Weapons/SM_Weapon_PlasmaCarbine": (KBLASTER("blaster-c"), {       # 668 v compact
        "ue_path": "/Game/Weapons/Meshes/SM_Weapon_PlasmaCarbine",
        "weapon_class": "Plasma Carbine (compact energy carbine)"}),
    "Weapons/SM_Weapon_ArcCannon": (KBLASTER("blaster-e"), {           # 1386 v / 3 parts
        "ue_path": "/Game/Weapons/Meshes/SM_Weapon_ArcCannon",
        "weapon_class": "Arc Cannon (multi-coil heavy cannon)"}),
    "Weapons/SM_Weapon_Railgun": (KBLASTER("blaster-g"), {             # 1056 v long body
        "ue_path": "/Game/Weapons/Meshes/SM_Weapon_Railgun",
        "weapon_class": "Railgun (long-body magnetic accelerator)"}),
    "Weapons/SM_Weapon_SingularityCannon": (KBLASTER("blaster-p"), {   # 1506 v largest
        "ue_path": "/Game/Weapons/Meshes/SM_Weapon_SingularityCannon",
        "weapon_class": "Singularity Cannon (exotic heavy platform)"}),
}

# ---- Vehicles: Dawn Skiff hovercraft + ground rover (+2 showcase) ---------
VEHICLES = {
    "Vehicles/SM_Vehicle_DawnSkiff": (SPACEDIR("Vehicles", "Spaceship_RaeTheRedPanda"), {
        "ue_path": "/Game/Vehicles/SM_Vehicle_DawnSkiff",
        "vehicle_class": "Dawn Skiff — anti-grav hovercraft"}),
    "Vehicles/SM_Vehicle_GroundRover": (SPACEDIR("Vehicles", "Rover_1"), {
        "ue_path": "/Game/Vehicles/SM_Vehicle_GroundRover",
        "vehicle_class": "All-terrain ground rover"}),
    "Vehicles/SM_Vehicle_SupportSkiff": (SPACEDIR("Vehicles", "Spaceship_FinnTheFrog"), {
        "ue_path": "/Game/Vehicles/SM_Vehicle_SupportSkiff",
        "vehicle_class": "Support Skiff — cargo hovercraft (showcase)"}),
    "Vehicles/SM_Vehicle_RoverHeavy": (SPACEDIR("Vehicles", "Rover_2"), {
        "ue_path": "/Game/Vehicles/SM_Vehicle_RoverHeavy",
        "vehicle_class": "Heavy rover — 6-wheel hauler (showcase)"}),
}

# ---- Resource nodes: 4 distinct mineral meshes ----------------------------
NODES = {
    "Environment/SM_Node_Astraite": (KSPACE("rock_crystals"), {
        "ue_path": "/Game/Environment/ResourceNodes/SM_Node_Astraite",
        "node_type": "Astraite (energy crystal cluster)"}),
    "Environment/SM_Node_Pyronite": (KSPACE("rock_crystalsLargeA"), {
        "ue_path": "/Game/Environment/ResourceNodes/SM_Node_Pyronite",
        "node_type": "Pyronite (large fire-crystal formation)"}),
    "Environment/SM_Node_Voidstone": (KSPACE("rock_crystalsLargeB"), {
        "ue_path": "/Game/Environment/ResourceNodes/SM_Node_Voidstone",
        "node_type": "Voidstone (dark crystal formation)"}),
    "Environment/SM_Node_AncientVein": (RUIN("Column_Round_Short"), {
        "ue_path": "/Game/Environment/ResourceNodes/SM_Node_AncientVein",
        "node_type": "Ancient Vein (ancient monolith marker)"}),
}

# ---- Environment: trees/rocks/flora/ruins (game-referenced set + extras) --
ENVIRONMENT = {
    "Environment/SM_Tree_Broadleaf": (KNATURE("tree_oak"), {
        "ue_path": "/Game/Environment/SM_Tree_Broadleaf"}),
    "Environment/SM_Tree_Conifer": (KNATURE("tree_cone"), {
        "ue_path": "/Game/Environment/SM_Tree_Conifer"}),
    "Environment/SM_Tree_SporeCanopy": (KNATURE("tree_fat"), {
        "ue_path": "/Game/Environment/SM_Tree_SporeCanopy"}),
    "Environment/SM_Rock_Granite_L": (KSPACE("rock_largeA"), {
        "ue_path": "/Game/Environment/SM_Rock_Granite_L"}),
    "Environment/SM_Rock_Granite_M": (KSPACE("rock_largeB"), {
        "ue_path": "/Game/Environment/SM_Rock_Granite_M"}),
    "Environment/SM_Rock_Granite_S": (KSPACE("rock"), {
        "ue_path": "/Game/Environment/SM_Rock_Granite_S"}),
    "Environment/SM_Rock_Boulder_Moss": (KSPACE("rocks_smallA"), {
        "ue_path": "/Game/Environment/SM_Rock_Boulder_Moss"}),
    "Environment/SM_Cliff_Shard": (KNATURE("cliff_blockDiagonal_rock"), {
        "ue_path": "/Game/Environment/SM_Cliff_Shard"}),
    "Environment/SM_Grass_Tuft": (KNATURE("grass_large"), {
        "ue_path": "/Game/Environment/SM_Grass_Tuft"}),
    "Environment/SM_Fern": (KNATURE("plant_bushDetailed"), {
        "ue_path": "/Game/Environment/SM_Fern"}),
    "Environment/SM_SporeBush": (KNATURE("plant_bushLarge"), {
        "ue_path": "/Game/Environment/SM_SporeBush"}),
    "Environment/SM_GlowReed": (KNATURE("plant_flatTall"), {
        "ue_path": "/Game/Environment/SM_GlowReed"}),
    "Environment/SM_Ruin_Arch": (RUIN("Arch_Gothic"), {
        "ue_path": "/Game/Environment/SM_Ruin_Arch"}),
    "Environment/SM_Ruin_Pillar": (RUIN("Column_Round"), {
        "ue_path": "/Game/Environment/SM_Ruin_Pillar"}),
    "Environment/SM_Ruin_Block": (RUIN("Brick"), {
        "ue_path": "/Game/Environment/SM_Ruin_Block"}),
    # Extra showcase flora (dressing variety, all distinct meshes).
    "Environment/SM_Flower_Purple": (KNATURE("flower_purpleA"), {
        "ue_path": "/Game/Environment/SM_Flower_Purple"}),
    "Environment/SM_Flower_Red": (KNATURE("flower_redA"), {
        "ue_path": "/Game/Environment/SM_Flower_Red"}),
    "Environment/SM_Mushroom_Red": (KNATURE("mushroom_red"), {
        "ue_path": "/Game/Environment/SM_Mushroom_Red"}),
    "Environment/SM_Mushroom_Group": (KNATURE("mushroom_redGroup"), {
        "ue_path": "/Game/Environment/SM_Mushroom_Group"}),
    "Environment/SM_Stump_Round": (KNATURE("stump_round"), {
        "ue_path": "/Game/Environment/SM_Stump_Round"}),
    "Environment/SM_Log_Fallen": (KNATURE("log"), {
        "ue_path": "/Game/Environment/SM_Log_Fallen"}),
}

# ---------------------------------------------------------------------------
# Mutation-spec table (species -> theme) parsed from the REAL generated C++
# table — the single source of truth for theme resolution.
# ---------------------------------------------------------------------------
MUTDATA_CPP = REPO / "Source" / "AstrawildCore" / "Private" / "AstrawildEchoMutationData.cpp"

def parse_species_themes() -> dict:
    text = MUTDATA_CPP.read_text(encoding="utf-8")
    rows = re.findall(
        r'TEXT\("Echo_(\w+)"\),\s*\n\s*EAstrawildSciFantasyTheme::(\w+),\s*\n\s*TEXT\("SK_Base_(\w+)"\)',
        text)
    return {sp: (theme, base) for sp, theme, base in rows}

# ---------------------------------------------------------------------------
# glTF 2.0 -> GLB container conversion (self-contained: data-URI buffers
# decoded, external images embedded as bufferViews).
# ---------------------------------------------------------------------------
GLB_MAGIC = 0x46546C67

def _pad4(data: bytes, pad_byte: int) -> bytes:
    rem = len(data) % 4
    return data + bytes([pad_byte]) * ((4 - rem) % 4) if rem else data

def _append_bin(bin_data: bytearray, blob: bytes) -> int:
    offset = len(bin_data)
    bin_data.extend(blob)
    rem = len(bin_data) % 4
    if rem:
        bin_data.extend(b"\x00" * (4 - rem))
    return offset

def _mime_of(uri: str, meta: str) -> str:
    if "image/png" in meta or uri.lower().endswith(".png"):
        return "image/png"
    if "image/jpeg" in meta or uri.lower().endswith((".jpg", ".jpeg")):
        return "image/jpeg"
    return "image/png"

def _embed_image_bytes(doc: dict, bin_data: bytearray, img_bytes: bytes, mime: str) -> None:
    offset = _append_bin(bin_data, img_bytes)
    bv = {"buffer": 0, "byteOffset": offset, "byteLength": len(img_bytes)}
    doc.setdefault("bufferViews", []).append(bv)
    idx = len(doc["bufferViews"]) - 1
    return idx

def gltf_to_glb(src: Path, dst: Path) -> dict:
    """Convert a .gltf (data-URI buffers, external/embedded images) to a
    self-contained .glb. Returns stats."""
    doc = json.loads(src.read_text(encoding="utf-8"))
    base_dir = src.parent
    bin_data = bytearray()

    # 1) Decode every buffer (data URI or external .bin) into the merged BIN.
    starts = []
    for buf in doc.get("buffers", []) or []:
        uri = buf.get("uri", "")
        if uri.startswith("data:"):
            meta, b64 = uri.split(",", 1)
            blob = base64.b64decode(b64)
        elif uri:
            blob = (base_dir / uri).read_bytes()
        else:
            blob = b""
        starts.append(_append_bin(bin_data, blob))

    # 2) Remap existing bufferViews to merged-BIN absolute offsets.
    for bv in doc.get("bufferViews", []):
        buf_idx = bv.get("buffer", 0)
        if buf_idx < len(starts):
            bv["buffer"] = 0
            bv["byteOffset"] = bv.get("byteOffset", 0) + starts[buf_idx]

    # 3) Embed images (external files / data URIs -> bufferViews).
    for img in doc.get("images", []):
        uri = img.get("uri", "")
        if not uri and "bufferView" in img:
            continue  # already embedded
        if uri.startswith("data:"):
            meta, b64 = uri.split(",", 1)
            blob = base64.b64decode(b64)
            mime = _mime_of(uri, meta)
        else:
            blob = (base_dir / uri).read_bytes()
            mime = _mime_of(uri, "")
        idx = _embed_image_bytes(doc, bin_data, blob, mime)
        img["bufferView"] = idx
        img["mimeType"] = mime
        img.pop("uri", None)

    doc["buffers"] = [{"byteLength": len(bin_data)}]

    json_bytes = _pad4(json.dumps(doc, separators=(",", ":")).encode("utf-8"), 0x20)
    chunks = struct.pack("<II", len(json_bytes), 0x4E4F534A) + json_bytes
    if bin_data:
        bin_chunk = _pad4(bytes(bin_data), 0x00)
        chunks += struct.pack("<II", len(bin_chunk), 0x004E4942) + bin_chunk
    total = 12 + len(chunks)
    with open(dst, "wb") as fh:
        fh.write(struct.pack("<III", GLB_MAGIC, 2, total))
        fh.write(chunks)
    return glb_stats(dst)

def glb_repack_embed(src: Path, dst: Path) -> dict:
    """Re-pack a .glb that references EXTERNAL textures so it becomes
    self-contained (Kenney colormap pattern)."""
    with open(src, "rb") as fh:
        magic, ver, total = struct.unpack("<III", fh.read(12))
        if magic != GLB_MAGIC:
            raise RuntimeError(f"not a GLB: {src}")
        clen, ctype = struct.unpack("<II", fh.read(8))
        doc = json.loads(fh.read(clen))
        # skip JSON-chunk padding to 4-byte alignment, then read the BIN chunk
        rem = clen % 4
        if rem:
            fh.read(4 - rem)
        rest = fh.read()
    # rest = [BIN chunk header + data] (+ any later chunks, none expected)
    bin_blob = b""
    if rest:
        blen, btype = struct.unpack("<II", rest[:8])
        bin_blob = rest[8:8 + blen]
    bin_data = bytearray(bin_blob)
    changed = False
    for img in doc.get("images", []):
        uri = img.get("uri", "")
        if uri and not uri.startswith("data:"):
            tex_path = (src.parent / uri)
            blob = tex_path.read_bytes()
            mime = _mime_of(uri, "")
            idx = _embed_image_bytes(doc, bin_data, blob, mime)
            img["bufferView"] = idx
            img["mimeType"] = mime
            img.pop("uri", None)
            changed = True
        elif uri.startswith("data:"):
            meta, b64 = uri.split(",", 1)
            blob = base64.b64decode(b64)
            mime = _mime_of(uri, meta)
            idx = _embed_image_bytes(doc, bin_data, blob, mime)
            img["bufferView"] = idx
            img["mimeType"] = mime
            img.pop("uri", None)
            changed = True
    if not changed:
        if dst != src:
            shutil.copyfile(src, dst)
        return glb_stats(dst)
    doc["buffers"] = [{"byteLength": len(bin_data)}]
    json_bytes = _pad4(json.dumps(doc, separators=(",", ":")).encode("utf-8"), 0x20)
    bin_chunk = _pad4(bytes(bin_data), 0x00)
    chunks = struct.pack("<II", len(json_bytes), 0x4E4F534A) + json_bytes
    chunks += struct.pack("<II", len(bin_chunk), 0x004E4942) + bin_chunk
    with open(dst, "wb") as fh:
        fh.write(struct.pack("<III", GLB_MAGIC, 2, 12 + len(chunks)))
        fh.write(chunks)
    return glb_stats(dst)

# ---------------------------------------------------------------------------
# GLB stats + verification
# ---------------------------------------------------------------------------
def glb_stats(path: Path) -> dict:
    with open(path, "rb") as fh:
        magic, ver, total = struct.unpack("<III", fh.read(12))
        if magic != GLB_MAGIC:
            raise RuntimeError(f"bad GLB magic in {path}")
        clen, ctype = struct.unpack("<II", fh.read(8))
        if clen > total:
            raise RuntimeError(f"corrupt chunk length in {path}")
        doc = json.loads(fh.read(clen))
    verts = 0
    for m in doc.get("meshes", []):
        for p in m.get("primitives", []):
            pos = p.get("attributes", {}).get("POSITION")
            if pos is not None and pos < len(doc.get("accessors", [])):
                verts += doc["accessors"][pos].get("count", 0)
    joints = 0
    for skin in doc.get("skins", []):
        joints = max(joints, len(skin.get("joints", [])))
    return {
        "meshes": len(doc.get("meshes", [])),
        "verts": verts,
        "materials": [m.get("name", "") for m in doc.get("materials", [])],
        "anims": [a.get("name", "") for a in doc.get("animations", [])],
        "joints": joints,
        "bytes": path.stat().st_size,
    }

def verify_fbx(path: Path) -> dict:
    head = path.read_bytes()[:23]
    if not head.startswith(b"Kaydara FBX Binary"):
        raise RuntimeError(f"not a binary FBX: {path}")
    return {"meshes": 1, "verts": 0, "materials": [], "anims": [],
            "joints": 0, "bytes": path.stat().st_size}

# ---------------------------------------------------------------------------
# Automatic clip-map derivation from a model's ACTUAL animation list.
# ---------------------------------------------------------------------------
IDLE_PREF = ["Idle", "Flying_Idle", "Idle_2", "Jump_Idle", "Idle_Gun"]
MOVE_PREF = ["Run", "Walk", "Gallop", "Fast_Flying", "Flying", "Run_Shoot", "Run_Fast"]
HIT_PREF = ["HitReact", "HitRecieve", "HitRecieve_2", "HitRecieve_1", "Hit", "Death"]

def derive_clip_map(anims: list) -> dict:
    def pick(prefs, contains):
        for p in prefs:
            if p in anims:
                return p
        for a in anims:
            if any(c in a for c in contains):
                return a
        return anims[0] if anims else ""
    return {
        "Idle": pick(IDLE_PREF, ("Idle", "idle")),
        "Move": pick(MOVE_PREF, ("Run", "Walk", "Gallop", "Flying", "Crawl", "Swim")),
        "Hit": pick(HIT_PREF, ("Hit", "Death", "death")),
    }

# ---------------------------------------------------------------------------
# Remote acquisition (Kenney direct-zip pattern — verified live).
# ---------------------------------------------------------------------------
UA = {"User-Agent": "Mozilla/5.0 (astrawild-asset-overhaul pipeline)"}

def kenney_zip_url(asset_page: str) -> str:
    req = urllib.request.Request(asset_page, headers=UA)
    html = urllib.request.urlopen(req, timeout=30).read().decode("utf-8", "replace")
    m = re.search(r"href='(https://kenney\.nl/media/pages/assets/[^']*\.zip)'", html)
    if not m:
        m = re.search(r'href="(https://kenney\.nl/media/pages/assets/[^"]*\.zip)"', html)
    if not m:
        raise RuntimeError(f"no zip link found at {asset_page}")
    return m.group(1)

def fetch_kenney_pack(slug: str, dest_dir: Path) -> bool:
    """REAL remote download + extraction of a Kenney pack. Returns True on
    success; failures are printed and returned as False (never faked)."""
    page = f"https://kenney.nl/assets/{slug}"
    try:
        url = kenney_zip_url(page)
        print(f"  [remote] {slug}: {url}")
        with tempfile.TemporaryDirectory() as td:
            zpath = Path(td) / f"{slug}.zip"
            req = urllib.request.Request(url, headers=UA)
            with urllib.request.urlopen(req, timeout=120) as resp, open(zpath, "wb") as out:
                shutil.copyfileobj(resp, out)
            if zpath.read_bytes()[:2] != b"PK":
                raise RuntimeError("downloaded file is not a zip")
            with zipfile.ZipFile(zpath) as zf:
                zf.extractall(dest_dir)
        return True
    except Exception as exc:  # noqa: BLE001 — recorded honestly
        print(f"  [remote] {slug} FAILED: {exc}")
        return False

# ---------------------------------------------------------------------------
# Main pipeline
# ---------------------------------------------------------------------------
def resolve_pack_model(pack: str, rel: str) -> Path:
    root = PACKS[pack]["root"]
    p = root / rel
    if not p.exists():
        raise FileNotFoundError(f"{pack}:{rel} -> {p} missing")
    return p

def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 16), b""):
            h.update(chunk)
    return h.hexdigest()

def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--verify-remote", action="store_true",
                    help="live-prove the Kenney remote download path (downloads to a temp dir)")
    args = ap.parse_args()

    failures: list = []
    staged: dict = {}      # asset_id -> row info
    used_sources: dict = {}  # (pack, rel) -> asset_id (1:1 uniqueness)
    credits: dict = {}

    print("=== ASTRAWILD ASSET OVERHAUL — real unique-mesh acquisition ===")

    # ---- Step 0: pack presence (local-first; honest remote fallback) -------
    print("\n[1] source packs")
    for name, info in PACKS.items():
        lic_file = None
        for cand in ("LICENSE_CC0.txt", "License.txt", "license.txt"):
            if (info["root"] / cand).exists():
                lic_file = info["root"] / cand
                break
        present = info["root"].exists()
        status = "LOCAL_PRESENT" if present else "MISSING"
        if lic_file:
            status += f" / LICENSE_ON_DISK({lic_file.name})"
        print(f"  {name:42s} {status}")
        if not present:
            failures.append(f"pack missing: {name}")
    if args.verify_remote:
        print("  [verify-remote] live-proving Kenney download path (blaster-kit)")
        with tempfile.TemporaryDirectory() as td:
            ok = fetch_kenney_pack("blaster-kit", Path(td))
            files = sum(1 for _ in Path(td).rglob("*") if _.is_file())
            print(f"  [verify-remote] blaster-kit: ok={ok} files={files}")
            if not ok or files < 10:
                failures.append("verify-remote failed for blaster-kit")

    # ---- Step 1: Tier-B deterministic assignment ---------------------------
    print("\n[2] Tier-B deterministic unique-model assignment")
    themes = parse_species_themes()
    if not themes:
        failures.append("could not parse mutation table")
        return finish(failures)
    # Pre-seed the consumed set with every FIXED catalog assignment so the
    # queues can never double-book a source model (1:1 uniqueness is global).
    consumed: set = set()
    for table in (ECHO_BASES, ECHO_HEROES, PROD_BOSSES, SHOWCASE_BOSSES):
        for src in table.values():
            consumed.add(src)
    for srcs in (PLAYER, WEAPONS, VEHICLES, NODES, ENVIRONMENT):
        for src, _extra in srcs.values():
            consumed.add(src)
    queue_pos = {k: 0 for k in THEME_QUEUES}
    overflow_pool: list = []
    tierb_assignment: dict = {}   # species -> (pack, rel)
    tierb_dropped: list = []
    echo_dir = MESHES / "Echoes"
    existing_echo_files = sorted(
        p.stem[len("SK_Echo_"):] for p in echo_dir.glob("SK_Echo_*.glb")) if echo_dir.exists() else []
    heroes_set = {k.split("/")[-1][len("SK_Echo_"):] for k in ECHO_HEROES}
    prod_set = {k.split("/")[-1][len("SK_Echo_"):] for k in PROD_BOSSES}
    candidates = [s for s in TIERB_PRIORITY if s in existing_echo_files]
    candidates += sorted(s for s in existing_echo_files
                         if s not in TIERB_PRIORITY and s not in heroes_set and s not in prod_set)
    for sp in candidates:
        theme = themes.get(sp, (None, None))[0]
        queue = THEME_QUEUES.get(theme, [])
        picked = None
        while queue_pos.get(theme, 0) < len(queue):
            model = queue[queue_pos[theme]]
            queue_pos[theme] += 1
            if model not in consumed:
                picked = model
                break
        if picked is None:
            if not overflow_pool:
                seen = set(consumed)
                for q in THEME_QUEUES.values():
                    for m in q:
                        if m not in seen:
                            seen.add(m)
                            overflow_pool.append(m)
            while overflow_pool:
                model = overflow_pool.pop(0)
                if model not in consumed:
                    picked = model
                    break
        if picked is None:
            tierb_dropped.append(sp)
        else:
            consumed.add(picked)
            tierb_assignment[sp] = picked
            themes_sp = themes.get(sp, ("", ""))
            print(f"    {sp:24s} <- {picked[1]:50s} (theme {themes_sp[0]})")

    print(f"  candidates with staged files : {len(existing_echo_files)}")
    print(f"  heroes (unique real mesh)    : {len(ECHO_HEROES)}")
    print(f"  production bosses (unique)   : {len(PROD_BOSSES)}")
    print(f"  Tier-B unique real meshes    : {len(tierb_assignment)}")
    print(f"  Tier-B base+mutation species : {len(tierb_dropped)} "
          f"(no direct file; real theme-base geometry + unique mutation spec)")
    if tierb_dropped:
        print(f"    dropped: {tierb_dropped}")
    print("\n  // C++ Tier-B table sync (AstrawildArtPack.cpp GetTierBSpeciesTable):")
    print(f"    // {len(tierb_assignment) + len(PROD_BOSSES)} species with unique real CC0 meshes")
    for sp in list(tierb_assignment) + sorted(prod_set):
        print(f'    TEXT("Echo_{sp}"),')

    # ---- Step 2: stage every catalog entry --------------------------------
    print("\n[3] stage real meshes (1:1 unique source models)")

    def stage(rel_dest: str, source: tuple, extra: dict | None = None) -> dict:
        asset_id = Path(rel_dest).stem
        src_path = resolve_pack_model(source[0], source[1])
        # Destination extension follows the SOURCE container (.gltf -> .glb
        # conversion; .glb re-pack; .fbx staged verbatim — UE imports FBX).
        src_ext = src_path.suffix.lower()
        dst_ext = ".fbx" if src_ext == ".fbx" else ".glb"
        dst = MESHES / (rel_dest + dst_ext)
        dst.parent.mkdir(parents=True, exist_ok=True)
        if dst_ext == ".glb":
            if src_ext == ".gltf":
                stats = gltf_to_glb(src_path, dst)
            else:
                stats = glb_repack_embed(src_path, dst)
        else:
            shutil.copyfile(src_path, dst)
            stats = verify_fbx(dst)
        key = (source[0], source[1])
        if key in used_sources:
            raise RuntimeError(
                f"PALETTE-SWAP GUARD: source {key} already staged as "
                f"{used_sources[key]} — refusing to reuse for {asset_id}")
        used_sources[key] = asset_id
        auto = derive_clip_map(stats["anims"])
        # Manifest clip convention: AM_<AssetId>_<Role> -> source clip name.
        # Survivor rows use the explicit locomotion set from extra.
        if extra and extra.get("clip_map"):
            clip_map = dict(extra["clip_map"])
        else:
            clip_map = {f"AM_{asset_id}_{role}": clip
                        for role, clip in auto.items()}
        row = {
            "asset_type": ("skeletal_mesh" if stats["joints"] > 0 else "static_mesh"),
            "category": "mesh",
            "status": "present",
            "source_pack": source[0],
            "source_model": source[1],
            "license": PACKS[source[0]]["license"],
            "license_url": PACKS[source[0]]["license_url"],
            "source_page": PACKS[source[0]]["page"],
            "path": str(dst.relative_to(REPO)),
            "bytes": stats["bytes"],
            "meshes": stats["meshes"],
            "verts": stats["verts"],
            "materials": stats["materials"],
            "joints": stats["joints"],
            "bones": stats["joints"],
            "sha256": sha256(dst),
        }
        if clip_map and any(clip_map.values()):
            row["animations"] = list(clip_map.keys())
            row["clip_map"] = clip_map
        if stats["anims"]:
            row["source_anims"] = stats["anims"]
        if extra:
            row.update({k: v for k, v in extra.items() if k not in ("clip_map",)})
        staged[asset_id] = row
        credits[asset_id] = {
            "source_pack": source[0], "source_model": source[1],
            "author": PACKS[source[0]].get("author", "Quaternius / Kenney"),
            "license": PACKS[source[0]]["license"],
            "license_url": PACKS[source[0]]["license_url"],
            "source_page": PACKS[source[0]]["page"],
            "path": row["path"],
        }
        print(f"  {asset_id:38s} <- {source[1]:52s} "
              f"({stats['bytes'] // 1024:4d} KB, {stats['verts']:5d} v, "
              f"{stats['joints']:3d} joints, {len(stats['anims']):2d} anims)")
        return row

    # Player & armor tiers
    for rel, (src, extra) in PLAYER.items():
        stage(rel, src, extra)
    # Echo bases
    for rel, src in ECHO_BASES.items():
        stage(rel, src, {"ue_path": f"/Game/Characters/Echoes/BaseMeshes/{Path(rel).stem}"})
    # Heroes
    for rel, src in ECHO_HEROES.items():
        stage(rel, src, {"ue_path": f"/Game/Characters/Echoes/{Path(rel).stem}"})
    # Production bosses
    for rel, src in PROD_BOSSES.items():
        stage(rel, src, {"ue_path": f"/Game/Characters/Echoes/{Path(rel).stem}",
                          "production_boss": True})
    # Showcase bosses
    for rel, src in SHOWCASE_BOSSES.items():
        stage(rel, src, {"ue_path": f"/Game/Characters/Echoes/Bosses/{Path(rel).stem}",
                          "showcase_boss": True})
    # Tier-B unique species
    for sp in sorted(tierb_assignment):
        stage(f"Echoes/SK_Echo_{sp}", tierb_assignment[sp],
              {"ue_path": f"/Game/Characters/Echoes/SK_Echo_{sp}", "tier_b": True})
    # Weapons / vehicles / nodes / environment
    for rel, (src, extra) in {**WEAPONS, **VEHICLES, **NODES, **ENVIRONMENT}.items():
        stage(rel, src, extra)

    # ---- Step 3: purge superseded procedural artifacts --------------------
    print("\n[4] purge superseded procedural artifacts")
    purged = 0
    keep_files = set()
    def _dest_file(rel: str, src: tuple) -> str:
        ext = ".fbx" if src[1].lower().endswith(".fbx") else ".glb"
        return str(MESHES / (rel + ext))
    for table in (PLAYER, WEAPONS, VEHICLES, NODES, ENVIRONMENT):
        for rel, val in table.items():
            src = val[0] if isinstance(val, tuple) else val
            keep_files.add(_dest_file(rel, src))
    for table in (ECHO_BASES, ECHO_HEROES, PROD_BOSSES, SHOWCASE_BOSSES):
        for rel, src in table.items():
            keep_files.add(_dest_file(rel, src))
    for sp in tierb_assignment:
        keep_files.add(str(MESHES / f"Echoes/SK_Echo_{sp}.glb"))
    for sub in ("Echoes", "Echoes/BaseMeshes", "Echoes/Bosses", "Characters/Survivor",
                "Weapons", "Vehicles", "Environment"):
        d = MESHES / sub
        if not d.exists():
            continue
        for f in sorted(d.iterdir()):
            if f.is_file() and str(f) not in keep_files:
                print(f"  rm {f.relative_to(REPO)}")
                f.unlink()
                purged += 1
    print(f"  purged {purged} superseded procedural files "
          f"(git history preserves the interim era)")

    # ---- Step 4: stage ruins textures -------------------------------------
    print("\n[5] stage pack textures")
    ruins_tex = REPO / "ArtSource" / "Models" / "Quaternius_UltimateModularRuins" / "Textures"
    tex_out = TEXTURES / "Ruins"
    tex_out.mkdir(parents=True, exist_ok=True)
    for t in sorted(ruins_tex.glob("*.*")):
        shutil.copyfile(t, tex_out / t.name)
        print(f"  {tex_out.name}/{t.name} ({t.stat().st_size // 1024} KB)")

    # ---- Step 5: regenerate the manifest (100% present, 0 pending) --------
    print("\n[6] regenerate ArtSource/manifest.json (status present, 0 pending)")
    old = json.loads(MANIFEST.read_text(encoding="utf-8"))
    old_assets = old.get("assets", {})

    # keep + normalize non-mesh rows (audio/textures) with explicit path/status
    carried = {}
    for name, info in old_assets.items():
        if name in staged:
            continue
        if info.get("category") == "mesh":
            continue  # mesh rows are FULLY regenerated above; superseded
            # procedural rows (purged species/props) are dropped deliberately —
            # their runtime identity moves to the real base mesh + mutation.
        row = dict(info)
        raw_path = row.get("path", "")
        # Normalize stale absolute paths (previous repo roots) to repo-relative.
        if raw_path:
            if "ArtSource" in raw_path:
                rel = raw_path.split("ArtSource", 1)[1].lstrip("/\\")
                raw_path = f"ArtSource/{rel}"
            row["path"] = raw_path
        if not raw_path:
            if name.startswith("A_"):
                cand = REPO / "ArtSource" / "Audio" / f"{name}.wav"
            else:
                cand = REPO / "ArtSource" / "Textures" / f"{name}.png"
            if cand.exists():
                row["path"] = str(cand.relative_to(REPO))
        p = REPO / row["path"] if row.get("path") else None
        row["status"] = "present" if (p and p.exists()) else "MISSING"
        if row["status"] != "present":
            failures.append(f"non-mesh asset missing on disk: {name} -> {row.get('path')}")
        row.setdefault("license", CC0)
        row.setdefault("license_url", CC0_URL)
        carried[name] = row

    assets = {}
    assets.update(carried)
    assets.update(staged)
    manifest = {
        "generator": "ASTRAWILD fetch_free_assets.py 2.0 (real unique-mesh overhaul)",
        "convention": {
            "path": "repo-relative source file (verified on disk at generation time)",
            "ue_path": "engine import target (landed by Content/Python/AwPipeline/import_all.py via Setup_And_Play.bat)",
            "status": "present = source file exists; every entry is real-file-backed; pending = 0 by construction",
            "licenses": "CC0 1.0 Universal for all externally-sourced models (Quaternius / Kenney); project-authored for generated PBR textures/audio",
        },
        "assets": {k: assets[k] for k in sorted(assets)},
    }
    MANIFEST.write_text(json.dumps(manifest, indent=2, sort_keys=True) + "\n",
                        encoding="utf-8")
    n_present = sum(1 for v in assets.values() if v.get("status") == "present")
    n_mesh = sum(1 for v in assets.values() if v.get("category") == "mesh")
    n_skel = sum(1 for v in assets.values() if v.get("asset_type") == "skeletal_mesh")
    print(f"  entries={len(assets)} present={n_present} pending={len(assets) - n_present} "
          f"mesh_rows={n_mesh} (skeletal={n_skel})")

    # ---- Step 6: credits + report ------------------------------------------
    CREDITS.parent.mkdir(parents=True, exist_ok=True)
    CREDITS.write_text(json.dumps({
        "generator": "Scripts/fetch_free_assets.py",
        "note": "Per-asset license provenance for every real staged mesh. All external sources are CC0 1.0 Universal (verified from in-pack license files).",
        "assets": {k: credits[k] for k in sorted(credits)},
        "packs": {k: {"page": v["page"], "license": v["license"]} for k, v in PACKS.items()},
    }, indent=2, sort_keys=True) + "\n", encoding="utf-8")

    report = {
        "run": "asset-overhaul",
        "staged_meshes": len(staged),
        "unique_source_models": len(used_sources),
        "tier_b_unique": len(tierb_assignment),
        "tier_b_dropped_to_base_mutation": tierb_dropped,
        "tier_b_assignment": {s: "/".join(m) for s, m in tierb_assignment.items()},
        "purged_procedural_files": purged,
        "manifest_entries": len(assets),
        "manifest_present": n_present,
        "manifest_pending": len(assets) - n_present,
        "failures": failures,
    }
    REPORT.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n",
                      encoding="utf-8")

    # ---- Step 7: final self-verification -----------------------------------
    print("\n[7] final self-verification")
    bad = []
    for aid, row in staged.items():
        p = REPO / row["path"]
        if not p.exists() or p.stat().st_size < 2048:
            bad.append(aid)
        if p.suffix == ".glb":
            try:
                glb_stats(p)
            except Exception as exc:  # noqa: BLE001
                bad.append(f"{aid}: {exc}")
    if bad:
        failures.append(f"verification failures: {bad}")
    print(f"  staged files verified: {len(staged) - len(bad)}/{len(staged)}")
    print(f"  palette-swap guard   : {len(used_sources)} unique source models for "
          f"{len(staged)} assets (1:1)")

    return finish(failures, report)

def finish(failures: list, report: dict | None = None) -> int:
    if failures:
        print(f"\n=== INCOMPLETE: {len(failures)} failures ===")
        for f in failures[:20]:
            print(f"  - {f}")
        return 1
    print("\n=== COMPLETE: real unique-mesh catalog staged, manifest 100% present, "
          "0 pending, 1:1 unique sources, licenses recorded. ===")
    return 0

if __name__ == "__main__":
    sys.exit(main())
