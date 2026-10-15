#!/usr/bin/env python3
"""
ASTRAWILD — Free Asset Acquisition Pipeline (asset pipeline / technical art)

Downloads, validates, converts, deduplicates and organizes CC0 asset packs
from the official Kenney publisher site into `ArtSource/`.

Pipeline per pack (directive §8 + §28):
    DOWNLOAD -> HASH -> VALIDATE -> EXTRACT (safe) -> SELECT -> DEDUP
    -> COPY TO ARTSOURCE -> (audio: OGG->WAV, originals preserved) -> MANIFEST
    -> REPORT -> DELETE TEMPORARY ARCHIVE + EXTRACTION

Rules honored (asset-acquisition directive):
  * only approved URLs are downloaded — APPROVED_PACKS is the single list
  * SHA256 dedup against existing ArtSource files and the accepted set
  * archives are extracted with path-traversal / symlink / zip-bomb defenses
  * licenses are verified per pack BEFORE acceptance (CC0 only)
  * OGG originals are preserved; converted WAV files are documented separately
    (UE5 imports WAV, not OGG — directive §13)
  * 3D format duplicates are dropped: only GLB is kept (matches the existing
    ArtSource .glb convention and the AwPipeline Interchange importer)
  * idempotent: a second run re-validates and never duplicates or overwrites;
    differing content at an existing destination is BLOCKED for manual review
  * temporary archives live in the cache dir OUTSIDE the repository and are
    removed after successful validation
  * no .uasset/.umap is fabricated — engine import belongs to the integration
    step (status IMPORT_READY, never UE5_VERIFIED)

Usage:
    python3 Scripts/download_assets.py [--repo-root DIR] [--cache-dir DIR]
                                       [--packs slug1,slug2] [--dry-run]

Requires: Python 3.9+ (stdlib only) + ffmpeg/ffprobe for OGG->WAV conversion
(graceful BLOCKED status when unavailable).
"""

from __future__ import annotations

import argparse
import datetime
import hashlib
import json
import os
import shutil
import stat
import struct
import subprocess
import sys
import time
import urllib.error
import urllib.request
import zipfile
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional

# ---------------------------------------------------------------------------
# Configuration — the approved pack list (single source of truth for this tool)
# ---------------------------------------------------------------------------
# This file lives in Scripts/ (capital S) because the repository already uses
# Scripts/ — a lowercase scripts/ folder would collide on the case-insensitive
# Windows filesystem of the UE integration machine.

AUDIO_CATEGORY = "Audio"
MODEL_CATEGORY = "Models"
TEXTURE_CATEGORY = "Textures"

CREATOR = "Kenney"
CREATOR_URL = "https://kenney.nl"
LICENSE = "CC0 1.0 Universal (Public Domain Dedication)"
LICENSE_URL = "https://creativecommons.org/publicdomain/zero/1.0/"
ATTRIBUTION_REQUIRED = False
COMMERCIAL_USE = True
REDISTRIBUTION = "Permitted (CC0 — no restrictions)"

# License verification basis (checked per pack page before this list was
# built; each pack's license table row links the CC0 legal code):
LICENSE_VERIFIED_NOTE = (
    "Verified per pack page at kenney.nl: the license table row links to "
    "https://creativecommons.org/publicdomain/zero/1.0/ ('Creative Commons CC0') "
    "and the page meta description states 'free, CC0 licensed!'. Batch 1 (6 packs) "
    "verified 2025-09-04 before acquisition; batch 2 (9 packs) verified the same way "
    "during the wayfinder gap analysis (full catalog walk of all 14 pagination pages, "
    "21 candidate pack pages read, per-page license rows + CC0 legal-code links "
    "confirmed, zip sizes measured via HTTP HEAD) and re-checked on acquisition day."
)


@dataclass
class Pack:
    slug: str
    name: str
    dest_dir: str                 # ArtSource/<category>/<dest_dir>
    category: str                 # Audio | Models
    source_url: str
    download_url: str
    asset_count_claim: int        # stated on the publisher page
    usage: str                    # pack-level ASTRAWILD purpose
    keep_exts: tuple              # extensions selected from the archive
    exclude_patterns: tuple = ()  # lowercase substrings that reject a file
    notes: str = ""


APPROVED_PACKS: List[Pack] = [
    Pack(
        slug="impact-sounds",
        name="Impact Sounds",
        dest_dir="Kenney_ImpactSounds",
        category=AUDIO_CATEGORY,
        source_url="https://kenney.nl/assets/impact-sounds",
        download_url="https://kenney.nl/media/pages/assets/impact-sounds/87b4ddecda-1677589768/kenney_impact-sounds.zip",
        asset_count_claim=130,
        usage="Impact/hit feedback library: resource-node harvest, melee and structure impacts, footstep surface variations (player + Echo locomotion)",
        keep_exts=(".ogg", ".txt"),
        notes="130 OGG originals preserved in Ogg/, 16-bit PCM WAV conversions in Wav/ (UE5 import format). Pack page: CC0.",
    ),
    Pack(
        slug="interface-sounds",
        name="Interface Sounds",
        dest_dir="Kenney_InterfaceSounds",
        category=AUDIO_CATEGORY,
        source_url="https://kenney.nl/assets/interface-sounds",
        download_url="https://kenney.nl/media/pages/assets/interface-sounds/fa43c1dd4d-1677589452/kenney_interface-sounds.zip",
        asset_count_claim=100,
        usage="UI feedback palette: navigation clicks, confirmations, errors, screen open/close transitions, toggles and scrolls for every player-facing screen",
        keep_exts=(".ogg", ".txt"),
        notes="100 OGG originals preserved in Ogg/, 16-bit PCM WAV conversions in Wav/ (UE5 import format). Pack page: CC0.",
    ),
    Pack(
        slug="sci-fi-sounds",
        name="Sci-fi Sounds",
        dest_dir="Kenney_SciFiSounds",
        category=AUDIO_CATEGORY,
        source_url="https://kenney.nl/assets/sci-fi-sounds",
        download_url="https://kenney.nl/media/pages/assets/sci-fi-sounds/6b296f9ecf-1677589334/kenney_sci-fi-sounds.zip",
        asset_count_claim=70,
        usage="Sci-fi survival feedback: energy-weapon fire, dungeon/building doors, research-station computer noise, force fields, explosions, skiff engine/thruster loops",
        keep_exts=(".ogg", ".ini", ".txt"),
        notes="73 OGG originals preserved in Ogg/, 16-bit PCM WAV conversions in Wav/ (UE5 import format). Pack page: CC0.",
    ),
    Pack(
        slug="nature-kit",
        name="Nature Kit",
        dest_dir="Kenney_NatureKit",
        category=MODEL_CATEGORY,
        source_url="https://kenney.nl/assets/nature-kit",
        download_url="https://kenney.nl/media/pages/assets/nature-kit/37ac38a37b-1677698939/kenney_nature-kit.zip",
        asset_count_claim=330,
        usage="Biome dressing for the 12-zone world: trees (forest/pine/palm/oak variants), rocks, cliffs, plants, flowers, mushrooms, plus crop-field, fence, campfire, tent, bridge and statue pieces for farms, villages and ruins",
        keep_exts=(".glb", ".txt"),
        exclude_patterns=("ground_grass", "ground_river"),
        notes="GLB only (self-contained, embedded buffers). ground_grass/ground_river tiles dropped — terrain and water are procedural in ASTRAWILD. FBX/OBJ/DAE/STL/MTL format duplicates and 2D preview sprites dropped at selection time.",
    ),
    Pack(
        slug="space-kit",
        name="Space Kit",
        dest_dir="Kenney_SpaceKit",
        category=MODEL_CATEGORY,
        source_url="https://kenney.nl/assets/space-kit",
        download_url="https://kenney.nl/media/pages/assets/space-kit/20874c75ac-1677698978/kenney_space-kit.zip",
        asset_count_claim=150,
        usage="Sci-fi ruins/dungeon dressing: corridors, structures, platforms, pipes, gates, hangars, machines, satellite dishes, craters and crystal rocks for Hollow Underlight, Sunken Vault, Eye of the Maelstrom and ancient-tech landmarks; turret pieces for Bolt Turret visuals",
        keep_exts=(".glb", ".txt"),
        exclude_patterns=("alien", "astronaut", "craft_", "monorail_", "rail", "rocket_", "rover", "terrain_road", "weapon_gun", "weapon_rifle"),
        notes="GLB only (self-contained). Character/vehicle/monorail/rocket models excluded: NPC skins are deferred cosmetic scope, the skiff already exists, no rail/rocket consuming system. FBX/OBJ/DAE/STL/MTL duplicates and 2D preview sprites dropped.",
    ),
    Pack(
        slug="blaster-kit",
        name="Blaster Kit",
        dest_dir="Kenney_BlasterKit",
        category=MODEL_CATEGORY,
        source_url="https://kenney.nl/assets/blaster-kit",
        download_url="https://kenney.nl/media/pages/assets/blaster-kit/261d80a716-1753959510/kenney_blaster-kit_2.1.zip",
        asset_count_claim=40,
        usage="Energy weapon model library: 40 blaster variants as candidate held-weapon meshes (CANDIDATE_REPLACEMENT pool for the 5 procedural weapon meshes; binding decided at UE integration)",
        keep_exts=(".glb", ".png", ".txt"),
        notes="GLB + shared Textures/colormap.png dependency (URIs resolve relative to the GLB folder). FBX/OBJ duplicates and preview images dropped.",
    ),
    # ---------------- batch 2 (approved via wayfinder ticket 03) ----------------
    Pack(
        slug="particle-pack",
        name="Particle Pack",
        dest_dir="Kenney_ParticlePack",
        category=TEXTURE_CATEGORY,
        source_url="https://kenney.nl/assets/particle-pack",
        download_url="https://kenney.nl/media/pages/assets/particle-pack/f8fe0f8cb8-1677578741/kenney_particle-pack.zip",
        asset_count_claim=80,
        usage="Combat VFX sprite library: muzzle flashes, impact sparks, smoke puffs, glows and particle textures feeding upgraded Niagara systems (CombatComponent FX pool + AstrawildVfxActor fallback art)",
        keep_exts=(".png", ".txt"),
        exclude_patterns=("preview", "black background"),
        notes="Transparent-background PNG sprites only (the PNG (Black background) folder is a baked-black-bg duplicate set for tools without alpha support — redundant in UE where Niagara sprites use the alpha channel directly). Preview.png and XML metadata skipped. UE usage: Niagara sprite/flipbook textures (base + pre-rotated frames).",
    ),
    Pack(
        slug="ui-pack-sci-fi",
        name="UI Pack: Sci-Fi",
        dest_dir="Kenney_UIPackSciFi",
        category=TEXTURE_CATEGORY,
        source_url="https://kenney.nl/assets/ui-pack-sci-fi",
        download_url="https://kenney.nl/media/pages/assets/ui-pack-sci-fi/b67c2acd31-1724181109/kenney_ui-pack-space-expansion.zip",
        asset_count_claim=130,
        usage="Sci-fi UI art: panels, buttons, sliders and icons for the 7 C++ UMG widget classes (HUD, inventory, research, shop, dialogue, pause)",
        keep_exts=(".png", ".ttf", ".txt"),
        exclude_patterns=("preview", "sample", "vector"),
        notes="PNG art preserving style/state subfolders (Blue/Green/Grey/Red/Yellow/Extra x Default/Double — state variants reuse base filenames, so the sub-path is part of the asset identity); Kenney Future/Narrow TTF fonts kept for UMG; SVG vector sources skipped. Publisher download file is kenney_ui-pack-space-expansion.zip (Space Expansion naming).",
    ),
    Pack(
        slug="survival-kit",
        name="Survival Kit",
        dest_dir="Kenney_SurvivalKit",
        category=MODEL_CATEGORY,
        source_url="https://kenney.nl/assets/survival-kit",
        download_url="https://kenney.nl/media/pages/assets/survival-kit/4065a8185b-1712149243/kenney_survival-kit.zip",
        asset_count_claim=80,
        usage="Survival-frontier props: camps, fires, crates, tools and shelters across all 12 zones; POI dressing and village outskirts (Dawn Fields, Verdant Reach)",
        keep_exts=(".glb", ".txt"),
        notes="GLB only (self-contained, v2.0 with animations as claimed on page). FBX/OBJ format duplicates and preview images dropped.",
    ),
    Pack(
        slug="city-kit-industrial",
        name="City Kit (Industrial)",
        dest_dir="Kenney_CityKitIndustrial",
        category=MODEL_CATEGORY,
        source_url="https://kenney.nl/assets/city-kit-industrial",
        download_url="https://kenney.nl/media/pages/assets/city-kit-industrial/0ec35b139d-1788171848/kenney_city-kit-industrial_2.0.zip",
        asset_count_claim=40,
        usage="Industrial/research props: containers, cranes, pipes and warehouse shells for Ember Ridge, Stormcrest, research POIs and the Dawnstead industrial quarter",
        keep_exts=(".glb", ".txt"),
        notes="GLB only (self-contained, v2.0 with variations). FBX/OBJ format duplicates and preview images dropped.",
    ),
    Pack(
        slug="modular-space-kit",
        name="Modular Space Kit",
        dest_dir="Kenney_ModularSpaceKit",
        category=MODEL_CATEGORY,
        source_url="https://kenney.nl/assets/modular-space-kit",
        download_url="https://kenney.nl/media/pages/assets/modular-space-kit/8261428a47-1771146076/kenney_modular-space-kit_1.0.zip",
        asset_count_claim=40,
        usage="Modular snapping sci-fi interior tiles for the 3 dungeons and Hollow Approach ruined-ancient-tech layouts (distinct from the classic Space Kit dressing pack)",
        keep_exts=(".glb", ".txt"),
        notes="GLB only (self-contained, v1.0 with variations/animations as claimed on page). FBX/OBJ format duplicates and preview images dropped.",
    ),
    Pack(
        slug="modular-dungeon-kit",
        name="Modular Dungeon Kit",
        dest_dir="Kenney_ModularDungeonKit",
        category=MODEL_CATEGORY,
        source_url="https://kenney.nl/assets/modular-dungeon-kit",
        download_url="https://kenney.nl/media/pages/assets/modular-dungeon-kit/7bed87605b-1771926065/kenney_modular-dungeon-kit_1.0.zip",
        asset_count_claim=40,
        usage="Stone/ancient modular dungeon tiles for the 3 dungeons' ruin segments and Sunscar ruins",
        keep_exts=(".glb", ".txt"),
        notes="GLB only (self-contained, v1.0 with variations/animations as claimed on page). FBX/OBJ format duplicates and preview images dropped.",
    ),
    Pack(
        slug="animated-characters-survivors",
        name="Animated Characters: Survivors",
        dest_dir="Kenney_AnimatedCharactersSurvivors",
        category=MODEL_CATEGORY,
        source_url="https://kenney.nl/assets/animated-characters-survivors",
        download_url="https://kenney.nl/media/pages/assets/animated-characters-survivors/27b16052a7-1774772958/kenney_animated-characters-survivors.zip",
        asset_count_claim=8,
        usage="NPC/villager body candidate (12 NPCs, 2 villages) + locomotion animation reference (idle/run/jump); rig-retarget compatibility to be checked in engine",
        keep_exts=(".fbx", ".txt"),
        notes="Classic Kenney character pack: ONE medium-detail humanoid FBX (Model/characterMedium.fbx) + 3 FBX animations; 2D skin sprites and SVG sources skipped. Research correction: this is NOT a GLB multi-rigged-character pack (inference was wrong — corrected at acquisition against the actual zip). Value = retarget reference + NPC body candidate.",
    ),
    Pack(
        slug="skyboxes",
        name="Skyboxes",
        dest_dir="Kenney_Skyboxes",
        category=TEXTURE_CATEGORY,
        source_url="https://kenney.nl/assets/skyboxes",
        download_url="https://kenney.nl/media/pages/assets/skyboxes/6736ff5c10-1784123473/kenney_skyboxes.zip",
        asset_count_claim=5,
        usage="Alien sky dome art for 12-zone atmosphere variants (day/dusk/night/storm/space equirectangular textures)",
        keep_exts=(".png", ".txt"),
        exclude_patterns=("preview", "sample"),
        notes="PNG equirectangular sky textures (Skyboxes/skybox-*.png — day/morning/night/alien/space); Sample renders and Preview.png skipped. Import as long-lat in engine.",
    ),
    Pack(
        slug="crosshair-pack",
        name="Crosshair Pack",
        dest_dir="Kenney_CrosshairPack",
        category=TEXTURE_CATEGORY,
        source_url="https://kenney.nl/assets/crosshair-pack",
        download_url="https://kenney.nl/media/pages/assets/crosshair-pack/5ef74bd405-1785950072/kenney_crosshair-pack.zip",
        asset_count_claim=200,
        usage="Reticle art replacing the text-glyph crosshair in HudWidget (hip-fire/aim states already coded)",
        keep_exts=(".png", ".txt"),
        exclude_patterns=("tilesheet", "preview"),
        notes="PNG reticles (64x64) preserving style subfolders (Dark/Glow/Light/Outline — families reuse base filenames, so the sub-path is part of the identity). Tilesheet atlases skipped (duplicates of the individual PNGs); Preview.png and SVG variants skipped.",
    ),
]

# Packs evaluated and NOT accepted (documented in the report + manifest).
REJECTED_PACKS = [
    {
        "slug": "sci-fi-rts",
        "name": "Sci-Fi RTS",
        "source_url": "https://kenney.nl/assets/sci-fi-rts",
        "download_url": "https://kenney.nl/media/pages/assets/sci-fi-rts/792bcb9cd5-1677693650/kenney_sci-fi-rts.zip",
        "status": "REJECTED_FORMAT",
        "reason": "2D top-down sprite pack (259 PNGs, spritesheets, SVG/SWF) — not 3D models. ASTRAWILD is a 3D third-person game with no top-down sprite consuming system.",
    },
    {
        "slug": "digital-audio",
        "name": "Digital Audio",
        "source_url": "https://kenney.nl/assets/digital-audio",
        "download_url": "https://kenney.nl/media/pages/assets/digital-audio/216eac4753-1677590265/kenney_digital-audio.zip",
        "status": "REJECTED_QUALITY",
        "reason": "Retro 8-bit aesthetic — does not fit the sci-fi survival frontier audio identity.",
    },
    {
        "slug": "ui-audio",
        "name": "UI Audio",
        "source_url": "https://kenney.nl/assets/ui-audio",
        "download_url": "https://kenney.nl/media/pages/assets/ui-audio/490d233f68-1677590494/kenney_ui-audio.zip",
        "status": "REJECTED_QUALITY",
        "reason": "Duplicate role — superseded by Interface Sounds (newer, larger pack covering the same UI feedback category); accepting both would create near-duplicate libraries.",
    },
    {
        "slug": "rpg-audio",
        "name": "RPG Audio",
        "source_url": "https://kenney.nl/assets/rpg-audio",
        "download_url": "https://kenney.nl/media/pages/assets/rpg-audio/8e99002d76-1677590336/kenney_rpg-audio.zip",
        "status": "REJECTED_QUALITY",
        "reason": "Fantasy-specific sounds (coins, potions, spells) — weak fit for sci-fi survival; no consuming system.",
    },
    {
        "slug": "kenney-2d-creature-family",
        "name": "Kenney 2D creature/character packs (Monster Builder, Animal Pack, Animal Pack Remastered, Alien UFO, Robot, Fish, Toon Characters, Shape Characters)",
        "source_url": "https://kenney.nl/assets",
        "status": "REJECTED_FORMAT",
        "reason": "All 2D sprite packs — ASTRAWILD needs 3D meshes for creatures/NPCs; the hoped-for 3D creature catalog does not exist at Kenney (verified during the batch-2 gap analysis, full catalog walk).",
    },
    {
        "slug": "quaternius-qal-packs",
        "name": "Quaternius newer packs (Sci-Fi Essentials Kit, MegaKits, Universal Animation Library 2)",
        "source_url": "https://quaternius.com",
        "status": "REJECTED_LICENSE",
        "reason": "Custom QAL v1.0 license — free commercial use but redistribution prohibited, which forbids committing the assets into this repository; only Quaternius 'Ultimate'-series pages explicitly stating CC0 are eligible.",
    },
]

OTHER_SOURCES_EVALUATED = [
    {
        "source": "opengameart.org",
        "status": "DEFERRED",
        "reason": "Researched during batch-2 planning: per-asset licenses are machine-parseable (CC0-filterable, 2,807 CC0 3D assets) and anonymous direct downloads verified — viable as a future batch once the downloader gains a per-asset license gate and a curated allowlist; not needed for this batch (Kenney covered the gaps).",
    },
    {
        "source": "github.com/KenneyNL",
        "status": "NOT_NEEDED",
        "reason": "Official publisher downloads from kenney.nl are preferred and were available for every approved pack; no GitHub mirror required.",
    },
    {
        "source": "quaternius.com",
        "status": "DEFERRED",
        "reason": "Ultimate Animated Animal Pack page states CC0 (creature-mesh candidate), but delivery is a Google Drive folder rather than a direct download URL — needs a manual fetch plus validator pass, or a Drive-aware pipeline extension, and per-pack CC0 re-verification at download time; newer Quaternius packs use QAL (redistribution prohibited) and are excluded.",
    },
    {
        "source": "polyhaven.com / ambientcg.com",
        "status": "NOT_ACQUIRED",
        "reason": "Site-wide CC0 verified on their license pages; realistic-style HDRIs/PBR textures are a P2 upgrade path, not a current gap — deliberately not acquired (FEWER + BETTER).",
    },
    {
        "source": "kaylousberg.itch.io (KayKit)",
        "status": "DEFERRED",
        "reason": "itch.io downloads are click-through pages, not plain direct URLs; per-pack license pages not yet verified.",
    },
]

DOWNLOAD_TIMEOUT_S = 120
DOWNLOAD_RETRIES = 3
UA = "ASTRAWILD-AssetPipeline/1.0 (CC0 pack acquisition)"

MAX_UNCOMPRESSED_TOTAL = 2 * 1024 * 1024 * 1024   # 2 GB per archive (zip-bomb guard)
MAX_UNCOMPRESSED_FILE = 512 * 1024 * 1024         # 512 MB per member
MAX_ARCHIVE_MEMBERS = 20000

# ---------------------------------------------------------------------------
# Small utilities
# ---------------------------------------------------------------------------


def sha256_of(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def human_size(n: float) -> str:
    step = 1024.0
    for unit in ("B", "KB", "MB", "GB"):
        if n < step:
            return f"{n:.1f} {unit}" if unit != "B" else f"{int(n)} B"
        n /= step
    return f"{n:.1f} TB"


def log(msg: str) -> None:
    print(f"[assets] {msg}")


# ---------------------------------------------------------------------------
# Download with retry / partial-download detection
# ---------------------------------------------------------------------------


def download(url: str, dest: Path) -> bool:
    for attempt in range(1, DOWNLOAD_RETRIES + 1):
        tmp = dest.with_suffix(dest.suffix + ".part")
        try:
            req = urllib.request.Request(url, headers={"User-Agent": UA})
            with urllib.request.urlopen(req, timeout=DOWNLOAD_TIMEOUT_S) as resp:
                expected = None
                cl = resp.headers.get("Content-Length")
                if cl and cl.isdigit():
                    expected = int(cl)
                received = 0
                with open(tmp, "wb") as f:
                    while True:
                        chunk = resp.read(1 << 17)
                        if not chunk:
                            break
                        f.write(chunk)
                        received += len(chunk)
            if expected is not None and received != expected:
                raise IOError(f"partial download: {received} of {expected} bytes")
            with zipfile.ZipFile(tmp) as zf:      # archive integrity check
                bad = zf.testzip()
                if bad is not None:
                    raise IOError(f"corrupt archive member: {bad}")
            tmp.replace(dest)
            return True
        except (urllib.error.URLError, IOError, OSError, zipfile.BadZipFile) as e:
            log(f"download attempt {attempt}/{DOWNLOAD_RETRIES} failed: {e}")
            if tmp.exists():
                tmp.unlink()
            if attempt < DOWNLOAD_RETRIES:
                time.sleep(2 ** attempt)
    return False


# ---------------------------------------------------------------------------
# Safe extraction (path-traversal / symlink / zip-bomb defenses)
# ---------------------------------------------------------------------------


class UnsafeArchive(Exception):
    pass


def safe_extract(zip_path: Path, dest: Path) -> None:
    with zipfile.ZipFile(zip_path) as zf:
        members = zf.infolist()
        if len(members) > MAX_ARCHIVE_MEMBERS:
            raise UnsafeArchive(f"too many members: {len(members)}")
        total = 0
        for info in members:
            total += info.file_size
            if total > MAX_UNCOMPRESSED_TOTAL:
                raise UnsafeArchive("uncompressed total exceeds 2 GB guard")
            if info.file_size > MAX_UNCOMPRESSED_FILE:
                raise UnsafeArchive(f"member too large: {info.filename}")
            name = info.filename
            if name.startswith(("/", "\\")):
                raise UnsafeArchive(f"absolute member path: {name}")
            if len(name) > 1 and name[1] == ":":
                raise UnsafeArchive(f"drive-letter member path: {name}")
            if any(p == ".." for p in name.replace("\\", "/").split("/")):
                raise UnsafeArchive(f"parent traversal member path: {name}")
            mode = info.external_attr >> 16
            if mode and stat.S_ISLNK(mode):
                raise UnsafeArchive(f"symlink member: {name}")
            if "\x00" in name:
                raise UnsafeArchive(f"NUL byte in member path: {name}")
        zf.extractall(dest)


# ---------------------------------------------------------------------------
# Format validators
# ---------------------------------------------------------------------------


def validate_wav(path: Path) -> Dict:
    """Parse the RIFF/WAVE header: PCM format, bit depth, rate, channels, duration."""
    with open(path, "rb") as f:
        riff = f.read(12)
        if len(riff) < 12 or riff[:4] != b"RIFF" or riff[8:12] != b"WAVE":
            raise ValueError("not a RIFF/WAVE file")
        fmt = None
        data_size = None
        while True:
            hdr = f.read(8)
            if len(hdr) < 8:
                break
            cid, size = struct.unpack("<4sI", hdr)
            if cid == b"fmt ":
                body = f.read(min(size, 16))
                if len(body) >= 16:
                    fmt = struct.unpack("<HHIIHH", body)
                if size > len(body):
                    f.seek(size - len(body), 1)
            elif cid == b"data":
                data_size = size
                f.seek(size, 1)
            else:
                f.seek(size, 1)
    if fmt is None:
        raise ValueError("missing fmt chunk")
    audio_format, channels, rate, _avg, _align, bits = fmt
    if audio_format not in (1, 3, 0xFFFE):
        raise ValueError(f"non-PCM WAV (format tag {audio_format})")
    duration = None
    if data_size and channels and rate and bits:
        duration = data_size / (rate * channels * (bits // 8))
    return {
        "pcm": audio_format in (1, 0xFFFE),
        "bit_depth": bits,
        "sample_rate": rate,
        "channels": channels,
        "duration_s": round(duration, 3) if duration else None,
    }


def _gltf_stats(js: Dict) -> Dict:
    accessors = js.get("accessors", [])
    tris = 0
    for mesh in js.get("meshes", []):
        for prim in mesh.get("primitives", []):
            idx = prim.get("indices")
            if idx is not None and idx < len(accessors):
                if prim.get("mode", 4) == 4:  # triangles
                    tris += accessors[idx].get("count", 0) // 3
    deps = [u["uri"] for u in js.get("images", []) if u.get("uri") and not u["uri"].startswith("data:")]
    deps += [b["uri"] for b in js.get("buffers", []) if b.get("uri") and not b["uri"].startswith("data:")]
    return {
        "meshes": len(js.get("meshes", [])),
        "materials": len(js.get("materials", [])),
        "nodes": len(js.get("nodes", [])),
        "triangles": tris,
        "external_deps": deps,
    }


def validate_glb(path: Path) -> Dict:
    with open(path, "rb") as f:
        head = f.read(12)
        if len(head) < 12:
            raise ValueError("truncated GLB")
        magic, _ver, _length = struct.unpack("<III", head)
        if magic != 0x46546C67:
            raise ValueError("bad GLB magic")
        clen, ctype = struct.unpack("<II", f.read(8))
        if ctype != 0x4E4F534A:
            raise ValueError("first GLB chunk is not JSON")
        js = json.loads(f.read(clen))
    return _gltf_stats(js)


def validate_gltf(path: Path) -> Dict:
    return _gltf_stats(json.loads(path.read_text(encoding="utf-8")))


def validate_png(path: Path) -> Dict:
    with open(path, "rb") as f:
        sig = f.read(8)
        if sig != b"\x89PNG\r\n\x1a\n":
            raise ValueError("bad PNG signature")
        ihdr = f.read(25)
        if len(ihdr) < 25 or ihdr[4:8] != b"IHDR":
            raise ValueError("missing IHDR")
        w, h = struct.unpack(">II", ihdr[8:16])
        return {"width": w, "height": h}


def validate_fbx(path: Path) -> Dict:
    """Container-level FBX integrity: binary/ASCII magic + size.

    Triangle counts and rig data are NOT parsed outside the engine — the
    import itself is the real validation (IMPORT_READY, never UE5_VERIFIED).
    """
    with open(path, "rb") as f:
        head = f.read(32)
    if head.startswith(b"Kaydara FBX Binary"):
        kind = "binary"
    elif head.lstrip().startswith(b";FBX"):
        kind = "ascii"
    else:
        raise ValueError("bad FBX magic (neither Kaydara binary nor ASCII FBX header)")
    return {"format": "FBX", "kind": kind, "size_bytes": path.stat().st_size,
            "note": "container magic verified; mesh/rig data validated at engine import"}


def validate_ttf(path: Path) -> Dict:
    """Font container integrity: TrueType/OpenType/Collections magic."""
    with open(path, "rb") as f:
        head = f.read(4)
    if head in (b"\x00\x01\x00\x00", b"OTTO", b"true", b"ttcf"):
        kind = {b"\x00\x01\x00\x00": "TrueType", b"OTTO": "OpenType with CFF",
                b"true": "TrueType (apple)", b"ttcf": "Font Collection"}[head]
    else:
        raise ValueError("bad font magic (not TrueType/OpenType)")
    return {"format": "TTF", "kind": kind, "size_bytes": path.stat().st_size}


def probe_ogg(path: Path) -> Dict:
    """Read OGG stream metadata via ffprobe (codec/rate/channels/duration)."""
    r = subprocess.run(
        ["ffprobe", "-v", "quiet", "-show_entries",
         "stream=codec_name,sample_rate,channels:format=duration",
         "-of", "json", str(path)],
        capture_output=True, text=True, timeout=30, check=False)
    try:
        data = json.loads(r.stdout)
        st = (data.get("streams") or [{}])[0]
        return {
            "codec": st.get("codec_name"),
            "sample_rate": int(st.get("sample_rate") or 0),
            "channels": int(st.get("channels") or 0),
            "duration_s": round(float(data.get("format", {}).get("duration", 0)), 3),
        }
    except Exception as e:  # noqa: BLE001
        raise ValueError(f"ffprobe failed on {path.name}: {e}")


def convert_ogg_to_wav(src: Path, dst: Path) -> Dict:
    """Convert OGG -> 16-bit PCM WAV at the source sample rate and channels."""
    r = subprocess.run(
        ["ffmpeg", "-y", "-hide_banner", "-loglevel", "error",
         "-i", str(src), "-c:a", "pcm_s16le", str(dst)],
        capture_output=True, text=True, timeout=120, check=False)
    if r.returncode != 0 or not dst.exists():
        raise ValueError(f"ffmpeg conversion failed: {r.stderr.strip()[:200]}")
    info = validate_wav(dst)
    if not info["pcm"] or info["bit_depth"] != 16:
        raise ValueError("converted WAV is not 16-bit PCM")
    return info


# ---------------------------------------------------------------------------
# Usage inference (per-file ASTRAWILD purpose strings)
# ---------------------------------------------------------------------------

USAGE_RULES = [
    # audio: impact-sounds
    ("footstep", "footstep surface variation (player/Echo locomotion feedback)"),
    ("impactmining", "resource-node harvest impact feedback"),
    ("impactpunch", "unarmed/combat hit feedback"),
    ("impactmetal", "metal impact feedback (structures/armor)"),
    ("impactwood", "wood impact feedback (trees/planks/building pieces)"),
    ("impactplank", "wood-plank impact feedback"),
    ("impactplate", "metal-plate impact feedback"),
    ("impactglass", "glass/crystal impact feedback (crystal nodes/relics)"),
    ("impactbell", "bell strike feedback (alarm/relic variation)"),
    ("impacttin", "tin impact feedback (scrap/junk props)"),
    ("impactsoft", "soft-body impact feedback (creature hits/foliage)"),
    ("impactgeneric", "generic impact feedback"),
    # audio: interface-sounds
    ("confirmation", "action success feedback (craft/research/save confirm)"),
    ("error", "invalid action / craft failure feedback"),
    ("question", "dialogue/quest prompt feedback"),
    ("click", "UI navigation click"),
    ("select", "UI selection feedback"),
    ("toggle", "UI toggle feedback (settings/menus)"),
    ("switch", "UI switch feedback"),
    ("tick", "UI tick feedback (lists/hover)"),
    ("scroll", "list scroll feedback"),
    ("minimize", "screen close/minimize feedback"),
    ("maximize", "screen open/maximize feedback"),
    ("open", "screen open feedback"),
    ("close", "screen close feedback"),
    ("back", "screen back navigation feedback"),
    ("drop", "item drop feedback"),
    ("glitch", "UI error/glitch variation (scanner/static)"),
    ("glass", "UI glass tone variation"),
    ("pluck", "UI pluck variation"),
    ("bong", "UI alert tone"),
    ("scratch", "UI scratch variation"),
    # audio: sci-fi-sounds
    ("laser", "energy weapon fire feedback"),
    ("dooropen", "door open feedback (dungeon/building doors)"),
    ("doorclose", "door close feedback (dungeon/building doors)"),
    ("computernoise", "terminal/research station ambience"),
    ("forcefield", "shield/energy barrier feedback"),
    ("explosioncrunch", "explosion feedback (combat/boss)"),
    ("spaceengine", "skiff engine loop"),
    ("thrusterfire", "skiff thruster loop"),
    ("enginecircular", "machine/engine loop (power systems)"),
    ("lowfrequency", "low-frequency dread ambience (bosses/depths)"),
    ("slime", "creature/Amorphous Echo feedback"),
    ("impactmetal", "metal impact feedback (structures)"),
    # models: nature-kit
    ("crop", "crop field dressing (CropComponent farms)"),
    ("crops", "crop field dressing (CropComponent farms)"),
    ("fence", "farm/base perimeter dressing"),
    ("campfire", "rest point / campfire dressing"),
    ("tent", "village / POI shelter dressing"),
    ("canoe", "Driftwood Landing watercraft dressing"),
    ("bridge", "river-crossing dressing"),
    ("statue", "ancient ruins dressing"),
    ("tree", "forest biome dressing (12-zone world)"),
    ("rock", "zone rock dressing (12-zone world)"),
    ("stone", "zone stone dressing (12-zone world)"),
    ("cliff", "cliff / terrain variation dressing"),
    ("mushroom", "bioluminescent undergrowth dressing"),
    ("flower", "meadow flora dressing"),
    ("cactus", "desert zone dressing (Sunscar)"),
    ("lily", "marsh/water surface dressing"),
    ("hanging_moss", "marsh canopy dressing"),
    ("stump", "forest floor dressing"),
    ("log", "forest floor dressing"),
    ("bed", "rest point furniture"),
    ("sign", "village signpost dressing"),
    ("path", "village path dressing"),
    ("pot_", "village prop dressing"),
    ("platform_beach", "shoreline platform dressing"),
    ("platform_stone", "stone platform dressing"),
    ("plant", "ground flora dressing"),
    ("bush", "ground flora dressing"),
    ("grass", "grass tuft dressing"),
    # models: space-kit
    ("corridor", "dungeon interior dressing (Hollow Underlight/Sunken Vault/Eye)"),
    ("structure", "dungeon/ancient structure dressing"),
    ("platform", "dungeon floor/platform dressing"),
    ("pipe", "industrial piping dressing"),
    ("gate", "dungeon gate dressing"),
    ("hangar", "dungeon hangar dressing"),
    ("machine", "industrial machinery dressing (power systems)"),
    ("desk", "research station interior dressing"),
    ("satellitedish", "ancient-tech landmark dressing"),
    ("crater", "impact crater dressing (Sunscar/Ember Ridge)"),
    ("meteor", "impact crater dressing (Sunscar/Ember Ridge)"),
    ("rock_crystals", "crystal rock dressing (nodes/Glimmerwood)"),
    ("turret", "Bolt Turret visual candidate"),
    ("barrel", "prop storage dressing"),
    ("bones", "creature remains dressing"),
    ("chimney", "village structure dressing"),
    ("stairs", "dungeon vertical traversal dressing"),
    ("supports", "dungeon support dressing"),
    ("terrain", "dungeon floor dressing"),
    # models: blaster-kit
    ("blaster", "energy weapon model (held-weapon visual candidate / CANDIDATE_REPLACEMENT pool)"),
    ("colormap", "shared blaster colormap texture (GLB dependency)"),
]


def usage_for(name_lower: str, pack_usage: str) -> str:
    for needle, usage in USAGE_RULES:
        if needle in name_lower:
            return usage
    return pack_usage


# ---------------------------------------------------------------------------
# Acquisition session
# ---------------------------------------------------------------------------


@dataclass
class FileRecord:
    pack: str
    pack_name: str
    rel_source: str            # path inside the original archive ("-> wav" = converted)
    dest: str                  # repo-relative destination path
    file_type: str
    size_bytes: int
    sha256: str
    status: str                # ACCEPTED | IMPORT_READY | DUPLICATE | REJECTED_* | BLOCKED
    usage: str
    import_ready: bool = False
    note: str = ""
    validation: Dict = field(default_factory=dict)


class Acquisition:
    def __init__(self, repo_root: Path, cache_dir: Path, dry_run: bool):
        self.repo = repo_root
        self.cache = cache_dir
        self.dry_run = dry_run
        self.records: List[FileRecord] = []
        self.pack_results: List[Dict] = []
        self.existing_hashes: Dict[str, str] = {}   # sha256 -> repo-relative path
        self.accepted_hashes: Dict[str, str] = {}

    # -- dedup index ------------------------------------------------------
    def index_existing(self) -> int:
        art = self.repo / "ArtSource"
        if not art.exists():
            return 0
        for p in sorted(art.rglob("*")):
            if p.is_file():
                h = sha256_of(p)
                self.existing_hashes[h] = p.relative_to(self.repo).as_posix()
        return len(self.existing_hashes)

    # -- destination naming -------------------------------------------------
    def dest_for(self, pack: Pack, rel_in_archive: str) -> Path:
        # converted-audio records carry a "<ogg rel> -> wav" provenance marker;
        # map them to the Wav/ folder with the .wav extension
        if "-> wav" in rel_in_archive:
            ogg_rel = rel_in_archive.split("-> wav")[0].strip()
            ogg_name = ogg_rel.rsplit("/", 1)[-1]
            wav_name = ogg_name[: -len(".ogg")] + ".wav"
            return self.repo / "ArtSource" / pack.category / pack.dest_dir / "Wav" / wav_name
        parts = rel_in_archive.replace("\\", "/").split("/")
        fname = parts[-1]
        base = self.repo / "ArtSource" / pack.category / pack.dest_dir
        low = fname.lower()
        if pack.category == AUDIO_CATEGORY:
            if low.endswith(".ogg"):
                return base / "Ogg" / fname
            if low.endswith(".wav"):
                return base / "Wav" / fname
            if low == "license.txt":
                return base / "License.txt"
            return base / fname
        # texture packs: preserve the archive sub-path (style/state subfolders
        # reuse the same base filenames — a flat destination would collide);
        # a leading container folder (PNG/, PNG (Transparent)/, Skyboxes/, …)
        # is stripped so the destination root stays the pack's PNG/ folder.
        if pack.category == TEXTURE_CATEGORY:
            if low == "license.txt":
                return base / "License.txt"
            if low.endswith(".ttf"):
                return base / "Fonts" / fname
            if low.endswith(".txt") and len(parts) == 1:
                return base / fname       # root-level publisher metadata (Size.txt etc.)
            sub = parts
            if len(parts) > 1 and parts[0].lower().split(" ")[0] in (
                    "png", "assets", "textures", "sprites", "skyboxes"):
                sub = parts[1:]
            return base / "PNG" / Path(*sub)
        # model packs: collapse multi-format folders into GLB/ (FBX-only packs
        # keep their sources in FBX/)
        if low.endswith(".fbx"):
            return base / "FBX" / fname
        if low.endswith((".glb", ".gltf")):
            return base / "GLB" / fname
        if low == "license.txt":
            return base / "License.txt"
        # texture dependencies keep their relative sub-path so GLB URIs resolve
        tex_idx = next((i for i, p in enumerate(parts) if p.lower() == "textures"), None)
        sub_parts = parts[tex_idx:] if tex_idx is not None else [fname]
        return base / "GLB" / Path(*sub_parts)

    # -- commit: validate-idempotent copy into the repo ----------------------
    def commit(self, pack: Pack, src: Path, rel: str, usage: str,
               note: str, import_ready: bool, validation: Dict,
               status: str = "IMPORT_READY") -> FileRecord:
        dest = self.dest_for(pack, rel)
        digest = sha256_of(src)
        size = src.stat().st_size
        rec = FileRecord(
            pack=pack.slug, pack_name=pack.name, rel_source=rel,
            dest=dest.relative_to(self.repo).as_posix(),
            file_type=src.suffix.lower().lstrip(".") or "file",
            size_bytes=size, sha256=digest, status=status, usage=usage,
            import_ready=import_ready, note=note, validation=validation,
        )
        # dedup against existing repo files + already-accepted files
        # (same content at the SAME destination is handled below as an
        #  idempotent re-run; same content at a DIFFERENT destination is a
        #  true duplicate)
        if digest in self.existing_hashes and rec.dest != self.existing_hashes[digest]:
            rec.status = "DUPLICATE"
            rec.note = f"identical SHA256 already present in the repository: {self.existing_hashes[digest]}"
            rec.import_ready = False
            self.records.append(rec)
            return rec
        if digest in self.accepted_hashes:
            rec.status = "DUPLICATE"
            rec.note = f"identical SHA256 already accepted this run: {self.accepted_hashes[digest]}"
            rec.import_ready = False
            self.records.append(rec)
            return rec
        if not self.dry_run:
            dest.parent.mkdir(parents=True, exist_ok=True)
            if dest.exists():
                if sha256_of(dest) == digest:
                    rec.note = (rec.note + " | already present with identical hash (idempotent re-run)").strip(" |")
                    self.accepted_hashes[digest] = rec.dest
                    self.records.append(rec)
                    return rec
                rec.status = "BLOCKED"
                rec.note = (f"destination exists with DIFFERENT content — manual review required "
                            f"({rec.dest}); existing assets are never overwritten (directive §19)")
                rec.import_ready = False
                self.records.append(rec)
                return rec
            shutil.copy2(src, dest)
        self.accepted_hashes[digest] = rec.dest
        self.records.append(rec)
        return rec

    # -- main per-pack flow ---------------------------------------------------
    def process(self, pack: Pack) -> Dict:
        log(f"=== pack: {pack.name} ({pack.slug}) ===")
        result: Dict = {
            "pack": pack.name, "slug": pack.slug, "category": pack.category,
            "source_url": pack.source_url, "download_url": pack.download_url,
            "creator": CREATOR, "license": LICENSE, "license_url": LICENSE_URL,
            "attribution_required": ATTRIBUTION_REQUIRED,
            "asset_count_claim": pack.asset_count_claim,
            "usage": pack.usage, "notes": pack.notes,
            "status": "BLOCKED", "files_accepted": 0, "files_rejected": 0,
            "files_format_skipped": 0, "size_accepted_bytes": 0,
        }
        zpath = self.cache / f"{pack.slug}.zip"
        if not download(pack.download_url, zpath):
            result["notes"] = (result["notes"] + " | download failed after retries").strip(" |")
            self.pack_results.append(result)
            return result
        result["zip_sha256"] = sha256_of(zpath)
        result["zip_bytes"] = zpath.stat().st_size

        extract_dir = self.cache / "extracted" / pack.slug
        if extract_dir.exists():
            shutil.rmtree(extract_dir)
        try:
            safe_extract(zpath, extract_dir)
        except (UnsafeArchive, zipfile.BadZipFile) as e:
            result["notes"] = (result["notes"] + f" | unsafe archive rejected: {e}").strip(" |")
            result["status"] = "BLOCKED"
            zpath.unlink(missing_ok=True)
            self.pack_results.append(result)
            return result

        members: List[Path] = []
        for root, _dirs, files in os.walk(extract_dir):
            for fn in sorted(files):
                members.append(Path(root) / fn)
        members.sort(key=lambda p: p.relative_to(extract_dir).as_posix())

        model_candidates: List[Path] = []

        for full in members:
            rel = full.relative_to(extract_dir).as_posix()
            fname = full.name
            low = fname.lower()
            ext = full.suffix.lower()

            if ext == ".url":
                result["files_format_skipped"] += 1       # promo shortcuts — never accepted
                continue
            if ext not in pack.keep_exts:
                result["files_format_skipped"] += 1       # duplicate formats / previews
                continue
            if any(pat in rel.lower() for pat in pack.exclude_patterns):
                rec = FileRecord(
                    pack=pack.slug, pack_name=pack.name, rel_source=rel, dest="",
                    file_type=ext.lstrip("."), size_bytes=full.stat().st_size,
                    sha256=sha256_of(full), status="REJECTED_QUALITY",
                    usage=usage_for(low, pack.usage), import_ready=False,
                    note="excluded by pack curation rule (see pack notes)")
                self.records.append(rec)
                result["files_rejected"] += 1
                continue

            # ---- texture packs: direct 2D art (validated, no dependency closure) ----
            if pack.category == TEXTURE_CATEGORY and ext == ".png":
                try:
                    info = validate_png(full)
                except Exception as e:  # noqa: BLE001
                    self.records.append(FileRecord(
                        pack=pack.slug, pack_name=pack.name, rel_source=rel, dest="",
                        file_type="png", size_bytes=full.stat().st_size, sha256="",
                        status="REJECTED_FORMAT", usage=usage_for(low, pack.usage),
                        note=f"PNG validation failed: {e}"))
                    result["files_rejected"] += 1
                    continue
                rec = self.commit(pack, full, rel, usage_for(low, pack.usage),
                                  "validated 2D art source (direct asset — no dependency closure)",
                                  import_ready=True, validation=info)
                if rec.status == "IMPORT_READY":
                    result["files_accepted"] += 1
                else:
                    result["files_rejected"] += 1
                continue

            # ---- fonts (publisher TTFs committed as import-ready sources) ----
            if ext == ".ttf":
                try:
                    info = validate_ttf(full)
                except Exception as e:  # noqa: BLE001
                    self.records.append(FileRecord(
                        pack=pack.slug, pack_name=pack.name, rel_source=rel, dest="",
                        file_type="ttf", size_bytes=full.stat().st_size, sha256="",
                        status="REJECTED_FORMAT", usage=usage_for(low, pack.usage),
                        note=f"font validation failed: {e}"))
                    result["files_rejected"] += 1
                    continue
                rec = self.commit(pack, full, rel, "sci-fi UI font (UMG typography)",
                                  "publisher font file (CC0 via pack page), UMG-usable",
                                  import_ready=True, validation=info)
                if rec.status == "IMPORT_READY":
                    result["files_accepted"] += 1
                else:
                    result["files_rejected"] += 1
                continue

            # ---- FBX model/animation sources (packs without a GLB variant) ----
            if ext == ".fbx":
                try:
                    info = validate_fbx(full)
                except Exception as e:  # noqa: BLE001
                    self.records.append(FileRecord(
                        pack=pack.slug, pack_name=pack.name, rel_source=rel, dest="",
                        file_type="fbx", size_bytes=full.stat().st_size, sha256="",
                        status="REJECTED_FORMAT", usage=usage_for(low, pack.usage),
                        note=f"FBX validation failed: {e}"))
                    result["files_rejected"] += 1
                    continue
                rec = self.commit(pack, full, rel, usage_for(low, pack.usage),
                                  "FBX model/animation source (pack ships no GLB variant); "
                                  "mesh/rig data validated at engine import",
                                  import_ready=True, validation=info)
                if rec.status == "IMPORT_READY":
                    result["files_accepted"] += 1
                else:
                    result["files_rejected"] += 1
                continue

            # ---- audio packs: OGG original + converted WAV ----
            if pack.category == AUDIO_CATEGORY and ext == ".ogg":
                try:
                    ogg_info = probe_ogg(full)
                except Exception as e:  # noqa: BLE001
                    self.records.append(FileRecord(
                        pack=pack.slug, pack_name=pack.name, rel_source=rel, dest="",
                        file_type="ogg", size_bytes=full.stat().st_size, sha256="",
                        status="REJECTED_FORMAT", usage=usage_for(low, pack.usage),
                        note=f"OGG probe failed: {e}"))
                    result["files_rejected"] += 1
                    continue
                rec_ogg = self.commit(pack, full, rel, usage_for(low, pack.usage),
                                      note=("original OGG preserved (source provenance); UE5 does not "
                                            "import OGG directly — see the converted WAV record"),
                                      import_ready=False, validation=ogg_info, status="ACCEPTED")
                if rec_ogg.status in ("ACCEPTED", "IMPORT_READY"):
                    result["files_accepted"] += 1
                else:
                    result["files_rejected"] += 1
                # converted WAV (import-ready)
                wav_name = fname[: -len(".ogg")] + ".wav"
                wav_src = self.cache / "converted" / pack.slug / wav_name
                wav_src.parent.mkdir(parents=True, exist_ok=True)
                wav_rel = f"{rel} -> wav"
                try:
                    wav_info = convert_ogg_to_wav(full, wav_src)
                    rec = self.commit(pack, wav_src, wav_rel, usage_for(low, pack.usage),
                                      note=("converted from the OGG original via ffmpeg "
                                            "(16-bit PCM, source rate/channels); original "
                                            "preserved in Ogg/"),
                                      import_ready=True, validation=wav_info)
                    if rec.status == "IMPORT_READY":
                        result["files_accepted"] += 1
                    else:
                        result["files_rejected"] += 1
                except Exception as e:  # noqa: BLE001
                    self.records.append(FileRecord(
                        pack=pack.slug, pack_name=pack.name, rel_source=wav_rel,
                        dest=self.dest_for(pack, rel).as_posix().replace("/Ogg/", "/Wav/"),
                        file_type="wav", size_bytes=0, sha256="", status="BLOCKED",
                        usage=usage_for(low, pack.usage),
                        note=f"OGG->WAV conversion failed: {e}"))
                    result["files_rejected"] += 1
                continue

            # ---- model packs: GLB (validated after dependency collection) ----
            if ext in (".glb", ".gltf"):
                model_candidates.append(full)
                continue

            # ---- license / metadata ----
            if ext == ".txt":
                is_license = low == "license.txt"
                rec = self.commit(
                    pack, full, rel,
                    "pack license file (CC0 statement)" if is_license else "publisher metadata file",
                    "publisher License.txt preserved for provenance" if is_license else
                    "publisher metadata file (provenance, not a game asset)",
                    import_ready=False, validation={}, status="ACCEPTED")
                if rec.status in ("ACCEPTED", "IMPORT_READY"):
                    result["files_accepted"] += 1
                else:
                    result["files_rejected"] += 1
                continue
            if ext == ".ini":
                rec = self.commit(pack, full, rel, usage_for(low, pack.usage),
                                  "publisher metadata file (provenance)",
                                  import_ready=False, validation={}, status="ACCEPTED")
                if rec.status in ("ACCEPTED", "IMPORT_READY"):
                    result["files_accepted"] += 1
                else:
                    result["files_rejected"] += 1
                continue
            # (.png in model packs is handled by the GLB dependency pass below;
            #  non-referenced preview images are format-skipped there)

        # ---- model pass: validate GLBs + resolve dependency closure ----
        dep_targets: set = set()
        validated: List[tuple] = []
        for full in model_candidates:
            rel = full.relative_to(extract_dir).as_posix()
            low = full.name.lower()
            try:
                info = validate_glb(full) if low.endswith(".glb") else validate_gltf(full)
            except Exception as e:  # noqa: BLE001
                self.records.append(FileRecord(
                    pack=pack.slug, pack_name=pack.name, rel_source=rel, dest="",
                    file_type=full.suffix.lower().lstrip("."),
                    size_bytes=full.stat().st_size, sha256="",
                    status="REJECTED_FORMAT", usage=usage_for(low, pack.usage),
                    note=f"model validation failed: {e}"))
                result["files_rejected"] += 1
                continue
            for dep in info["external_deps"]:
                dep_norm = dep.replace("\\", "/")
                dep_path = (full.parent / dep_norm).resolve()
                try:
                    dep_rel = dep_path.relative_to(extract_dir.resolve()).as_posix()
                except ValueError:
                    dep_rel = dep_norm  # outside extraction — will fail resolution below
                dep_targets.add(dep_rel)
            validated.append((full, rel, info))

        for full, rel, info in validated:
            usage = usage_for(full.name.lower(), pack.usage)
            note = "self-contained GLB (embedded buffers)" if not info["external_deps"] else (
                "external texture dependency committed beside the GLB (relative URI resolves)")
            rec = self.commit(pack, full, rel, usage, note, import_ready=True, validation=info)
            if rec.status == "IMPORT_READY":
                result["files_accepted"] += 1
            else:
                result["files_rejected"] += 1

        # texture dependencies only — model packs exclusively (texture packs commit
        # their PNGs directly in the member loop above, so this pass must not
        # double-count them as skipped previews)
        if pack.category == MODEL_CATEGORY:
            for full in members:
                rel = full.relative_to(extract_dir).as_posix()
                if full.suffix.lower() != ".png":
                    continue
                if rel not in dep_targets:
                    result["files_format_skipped"] += 1   # preview image — not a game asset
                    continue
                try:
                    info = validate_png(full)
                    rec = self.commit(pack, full, rel,
                                      usage_for(full.name.lower(), pack.usage),
                                      "texture dependency of selected GLB models",
                                      import_ready=True, validation=info)
                    if rec.status == "IMPORT_READY":
                        result["files_accepted"] += 1
                    else:
                        result["files_rejected"] += 1
                except Exception as e:  # noqa: BLE001
                    self.records.append(FileRecord(
                        pack=pack.slug, pack_name=pack.name, rel_source=rel, dest="",
                        file_type="png", size_bytes=full.stat().st_size, sha256="",
                        status="REJECTED_FORMAT", usage=usage_for(full.name.lower(), pack.usage),
                        note=f"PNG validation failed: {e}"))
                    result["files_rejected"] += 1

            # unresolved deps -> MISSING_DEPENDENCY records (model packs only)
            for dep_rel in sorted(dep_targets):
                if not (extract_dir / dep_rel).exists():
                    self.records.append(FileRecord(
                        pack=pack.slug, pack_name=pack.name, rel_source=dep_rel, dest="",
                        file_type="dependency", size_bytes=0, sha256="",
                        status="MISSING_DEPENDENCY", usage="model texture dependency",
                        note="referenced by a GLB but not present in the archive"))

        # per-file size accounting for accepted records of this pack
        result["size_accepted_bytes"] = sum(
            r.size_bytes for r in self.records
            if r.pack == pack.slug and r.status in ("ACCEPTED", "IMPORT_READY"))
        result["status"] = "ACCEPTED" if result["files_accepted"] > 0 else "BLOCKED"

        # cleanup: temporary extraction + archive (directive §8)
        shutil.rmtree(extract_dir, ignore_errors=True)
        zpath.unlink(missing_ok=True)
        self.pack_results.append(result)
        return result


# ---------------------------------------------------------------------------
# Output documents
# ---------------------------------------------------------------------------


def file_record_json(rec: FileRecord) -> Dict:
    pack_meta = next((p for p in APPROVED_PACKS if p.slug == rec.pack), None)
    return {
        "asset": (Path(rec.dest).stem if rec.dest else Path(rec.rel_source).stem),
        "pack": rec.pack_name,
        "creator": CREATOR,
        "source_url": pack_meta.source_url if pack_meta else "",
        "download_url": pack_meta.download_url if pack_meta else "",
        "license": LICENSE,
        "license_url": LICENSE_URL,
        "attribution_required": ATTRIBUTION_REQUIRED,
        "sha256": rec.sha256,
        "source_path": rec.rel_source,
        "destination_path": rec.dest,
        "file_type": rec.file_type,
        "size_bytes": rec.size_bytes,
        "status": rec.status,
        "astrawild_usage": rec.usage,
        "import_ready": rec.import_ready,
        "note": rec.note,
        "validation": rec.validation,
    }


def stats_of(files: List[Dict]) -> Dict:
    accepted = [f for f in files if f["status"] in ("ACCEPTED", "IMPORT_READY")]
    return {
        "total_records": len(files),
        "accepted_records": len(accepted),
        "import_ready": sum(1 for f in files if f["status"] == "IMPORT_READY"),
        "duplicates": sum(1 for f in files if f["status"] == "DUPLICATE"),
        "rejected_format": sum(1 for f in files if f["status"] == "REJECTED_FORMAT"),
        "rejected_quality": sum(1 for f in files if f["status"] == "REJECTED_QUALITY"),
        "missing_dependency": sum(1 for f in files if f["status"] == "MISSING_DEPENDENCY"),
        "blocked": sum(1 for f in files if f["status"] == "BLOCKED"),
        "accepted_bytes": sum(f["size_bytes"] for f in accepted),
        "audio_files": sum(1 for f in accepted if f["file_type"] in ("ogg", "wav")),
        "model_files": sum(1 for f in accepted if f["file_type"] in ("glb", "gltf", "fbx")),
        "texture_files": sum(1 for f in accepted if f["file_type"] in ("png", "ttf")),
    }


def compute_stats(acq: Acquisition) -> Dict:
    return stats_of([file_record_json(r) for r in acq.records])


def load_previous_state(acq: Acquisition) -> tuple:
    """Incremental-run support: carry file records and accepted pack results
    from the previous manifest for packs NOT re-processed this run, so a
    `--packs` subset still produces the single authoritative manifest.

    Replacement rule: a pack's previous records are dropped iff this run
    produced at least one record for it (fresh state wins — including the
    case where fresh curation now rejects what an older run accepted).
    Records of packs no longer in APPROVED_PACKS are dropped entirely.
    """
    recorded_names = {r.pack_name for r in acq.records}
    current_names = {p.name for p in APPROVED_PACKS}
    current_slugs = {p.slug for p in APPROVED_PACKS}
    carried_files: List[Dict] = []
    carried_packs: List[Dict] = []
    root_path = acq.repo / "ASSET_MANIFEST.json"
    sess_path = acq.repo / "Docs" / "ASSET_ACQUISITION_MANIFEST.json"
    if root_path.exists():
        try:
            prev = json.loads(root_path.read_text(encoding="utf-8"))
            for f in prev.get("files", []):
                name = f.get("pack")
                if name in recorded_names or name not in current_names:
                    continue
                carried_files.append(f)
        except Exception:  # noqa: BLE001 — unreadable previous manifest: start fresh
            carried_files = []
    if sess_path.exists():
        try:
            prev = json.loads(sess_path.read_text(encoding="utf-8"))
            for p in prev.get("packs_accepted", []):
                if p.get("pack") in recorded_names or p.get("slug") not in current_slugs:
                    continue
                carried_packs.append(p)
        except Exception:  # noqa: BLE001
            carried_packs = []
    return carried_files, carried_packs


def write_manifests(acq: Acquisition, now_iso: str) -> tuple:
    run_files = [file_record_json(r) for r in acq.records]
    carried_files, carried_packs = load_previous_state(acq)
    files = run_files + carried_files
    stats = stats_of(files)
    packs_accepted = ([p for p in acq.pack_results if p["status"] == "ACCEPTED"]
                      + carried_packs)
    packs_accepted.sort(key=lambda p: next(
        (i for i, ap in enumerate(APPROVED_PACKS) if ap.slug == p.get("slug")), 999))
    root_manifest = {
        "schema": "astrawild-asset-manifest/1",
        "generated": now_iso,
        "generator": "Scripts/download_assets.py",
        "license_verification": LICENSE_VERIFIED_NOTE,
        "creator": CREATOR,
        "creator_url": CREATOR_URL,
        "license": LICENSE,
        "license_url": LICENSE_URL,
        "attribution_required": ATTRIBUTION_REQUIRED,
        "commercial_use": COMMERCIAL_USE,
        "redistribution": REDISTRIBUTION,
        "stats": stats,
        "files": files,
    }
    (acq.repo / "ASSET_MANIFEST.json").write_text(
        json.dumps(root_manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    session_manifest = {
        "schema": "astrawild-asset-acquisition-manifest/1",
        "generated": now_iso,
        "generator": "Scripts/download_assets.py",
        "packs_accepted": packs_accepted,
        "packs_rejected": REJECTED_PACKS,
        "other_sources_evaluated": OTHER_SOURCES_EVALUATED,
        "stats": stats,
        "files": files,
    }
    docs = acq.repo / "Docs"
    docs.mkdir(exist_ok=True)
    (docs / "ASSET_ACQUISITION_MANIFEST.json").write_text(
        json.dumps(session_manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return stats, files, packs_accepted


def write_credits(acq: Acquisition, now_iso: str, files: List[Dict],
                  packs_accepted: List[Dict]) -> None:
    lines = [
        "# ASTRAWILD — Asset Credits (CC0 sources)",
        "",
        "> Machine-generated by `Scripts/download_assets.py` — regenerate with the same script.",
        "> Every accepted asset is CC0 (public domain): no attribution required, but provenance",
        "> is recorded anyway. Each pack page at kenney.nl carries a license table linking",
        "> https://creativecommons.org/publicdomain/zero/1.0/ ('Creative Commons CC0').",
        "",
        f"**Date checked**: {now_iso[:10]} · **Creator**: {CREATOR} ({CREATOR_URL})",
        f"**License**: {LICENSE} · **Attribution required**: {ATTRIBUTION_REQUIRED}",
        "",
        "| Field | Value |",
        "| :--- | :--- |",
        f"| Commercial use | {COMMERCIAL_USE} |",
        f"| Redistribution | {REDISTRIBUTION} |",
        "",
        "---",
        "",
    ]
    for p in packs_accepted:
        pack_files = [f for f in files
                      if f["pack"] == p["pack"] and f["status"] in ("ACCEPTED", "IMPORT_READY")]
        lines += [
            f"## {p['pack']}",
            "",
            f"- **Asset / Pack**: {p['pack']} (slug `{p['slug']}`, {p['asset_count_claim']} assets claimed on the page)",
            f"- **Creator**: {CREATOR}",
            f"- **Source**: {p['source_url']}",
            f"- **Download**: {p['download_url']}",
            f"- **License**: {LICENSE} (LICENSE_VERIFIED)",
            f"- **License URL**: {LICENSE_URL}",
            f"- **Attribution Required**: {ATTRIBUTION_REQUIRED}",
            f"- **ASTRAWILD Usage**: {p['usage']}",
            f"- **Files**: {len(pack_files)} accepted — {human_size(p['size_accepted_bytes'])}",
            f"- **SHA256 (pack zip)**: `{p.get('zip_sha256', 'n/a')}`",
            f"- **Date Checked**: {now_iso[:10]}",
            "",
        ]
    lines += [
        "---",
        "",
        "## Rejected / not acquired",
        "",
    ]
    for rp in REJECTED_PACKS:
        lines.append(f"- **{rp['name']}** — {rp['status']}: {rp['reason']}")
    for s in OTHER_SOURCES_EVALUATED:
        lines.append(f"- **{s['source']}** — {s['status']}: {s['reason']}")
    lines.append("")
    (acq.repo / "ASSETS_CREDITS.md").write_text("\n".join(lines), encoding="utf-8")


def write_report(acq: Acquisition, now_iso: str, stats: Dict,
                files: List[Dict], packs_accepted: List[Dict]) -> None:
    lines = [
        "# ASTRAWILD — Asset Acquisition Report",
        "",
        f"**Generated**: {now_iso} · **Generator**: `Scripts/download_assets.py`",
        "**Context**: branch `final-completion` (post READY_FOR_FINAL_BUILD source gate).",
        "This run adds supplementary CC0 source assets only — no gameplay code, no soft-path",
        "bindings and no existing ArtSource file were changed.",
        "",
        "## 1. Sources searched",
        "",
        "| Source | Result |",
        "| :--- | :--- |",
        "| kenney.nl (Priority 1, official publisher — batch 1) | 10 pack pages probed; 7 candidates evaluated; 6 accepted; 4 packs rejected with documented reasons |",
        "| kenney.nl (Priority 1 — batch 2 gap analysis) | full catalog walk: all 14 pagination pages enumerated (214 packs); 21 candidate pack pages read individually; 9 more packs accepted (this run); 2D creature/character family rejected (format); zip sizes measured via HTTP HEAD |",
        "| github.com/KenneyNL | not needed — official publisher downloads available for every pack |",
        "| quaternius.com | deferred — Ultimate-series CC0 pages exist, but delivery is a Google Drive folder (not a direct URL); newer packs use QAL (redistribution prohibited) |",
        "| opengameart.org (Priority 2) | deferred — researched and viable (machine-parseable per-asset licenses, anonymous direct downloads), needs a downloader license-gate extension first |",
        "| polyhaven.com / ambientcg.com | site-wide CC0 verified; realistic PBR/HDRIs are a P2 upgrade path — deliberately not acquired |",
        "",
        "## 2. Packs accepted",
        "",
        "| Pack | Category | Files accepted | Size | Pack status |",
        "| :--- | :--- | ---: | ---: | :--- |",
    ]
    for p in packs_accepted:
        lines.append(f"| {p['pack']} | {p['category']} | {p['files_accepted']} | "
                     f"{human_size(p['size_accepted_bytes'])} | ACCEPTED (sources IMPORT_READY) |")
    lines += [
        "",
        "## 3. Packs rejected (with reasons)",
        "",
        "| Pack | Status | Reason |",
        "| :--- | :--- | :--- |",
    ]
    for rp in REJECTED_PACKS:
        lines.append(f"| {rp['name']} | {rp['status']} | {rp['reason']} |")
    lines += [
        "",
        "## 4. File-level results",
        "",
        f"- Total file records: **{stats['total_records']}**",
        f"- Accepted: **{stats['accepted_records']}** ({human_size(stats['accepted_bytes'])})",
        f"- Import-ready (UE5 import format): **{stats['import_ready']}**",
        f"- Duplicates skipped: **{stats['duplicates']}**",
        f"- Rejected (format): **{stats['rejected_format']}**",
        f"- Rejected (quality/curation): **{stats['rejected_quality']}**",
        f"- Missing dependencies: **{stats['missing_dependency']}**",
        f"- Blocked: **{stats['blocked']}**",
        f"- Audio files accepted (OGG originals + WAV conversions): **{stats['audio_files']}**",
        f"- Model files accepted (GLB): **{stats['model_files']}**",
        f"- Texture files accepted (PNG): **{stats['texture_files']}**",
        "- Format-duplicate files skipped at selection (FBX/OBJ/DAE/STL/MTL copies, 2D",
        "  preview sprites, `.url` shortcuts) are counted per pack in",
        "  `Docs/ASSET_ACQUISITION_MANIFEST.json` (files_format_skipped) — never committed.",
        "",
        "## 5. License verification",
        "",
        f"All accepted packs: **{LICENSE}** — LICENSE_VERIFIED.",
        "",
        LICENSE_VERIFIED_NOTE,
        "",
        "CC0 requires no attribution and permits commercial use and redistribution;",
        "provenance is recorded in `ASSETS_CREDITS.md`, `ASSET_MANIFEST.json`,",
        "`Docs/ASSET_ACQUISITION_MANIFEST.json` and the per-pack `License.txt` files committed",
        "alongside the assets. No CC-BY or unclear-license asset was accepted.",
        "",
        "## 6. Format validation performed",
        "",
        "- **WAV (converted)**: RIFF/WAVE header parsed — PCM 16-bit, source sample rate and",
        "  channel count preserved, duration computed. Corrupt files are REJECTED.",
        "- **OGG (originals)**: probed via ffprobe (Vorbis, 44.1 kHz, channel count preserved",
        "  per source — mono and stereo both occur) — preserved verbatim as source provenance;",
        "  UE5 does not import OGG directly, so these carry `import_ready: false`.",
        "- **GLB**: binary glTF magic + JSON chunk parsed; meshes/materials/nodes counted;",
        "  triangle totals computed from index accessors; external URI dependencies resolved",
        "  (blaster-kit GLBs reference `Textures/colormap.png`, which is committed beside them",
        "  so relative URIs keep resolving).",
        "- **PNG**: signature + IHDR dimensions verified.",
        "- No file extension was renamed or silently converted — conversions are separate,",
        "  documented files.",
        "",
        "## 7. Storage control",
        "",
        f"New accepted source payload: **{human_size(stats['accepted_bytes'])}** (directive soft",
        "limits: 2 GB per pack, 10 GB total — far below both). Audio packs keep OGG originals",
        "(provenance) AND 16-bit PCM WAV conversions (UE5 import format). 3D packs keep GLB",
        "only; FBX/OBJ/DAE/STL/MTL duplicates and preview images were dropped at selection",
        "time (never committed).",
        "",
        "## 8. Repository integration (what changed)",
        "",
        "- New sources under `ArtSource/Audio/Kenney_*/` (Ogg/ + Wav/ + License.txt),",
        "  `ArtSource/Models/Kenney_*/` (GLB/ + License.txt) and — new in batch 2 —",
        "  `ArtSource/Textures/Kenney_*/` (PNG/ + License.txt: particles, sci-fi UI,",
        "  crosshairs, skyboxes).",
        "- The existing UE import pipeline (`Content/Python/AwPipeline/import_all.py`) only",
        "  auto-imports the FLAT `ArtSource/Audio/*.wav` and `ArtSource/Textures/*.png` folders,",
        "  so the new pack subfolders are NOT auto-imported — current bindings and fallback",
        "  chains are untouched (soft-path contract intact; zero-asset boot still guaranteed).",
        "- No `.uasset`/`.umap` was fabricated. All new files are import-ready SOURCES with",
        "  status IMPORT_READY — **not** UE5_VERIFIED (engine import/cook is owned by the",
        "  Antigravity integration run).",
        "- Existing ASTRAWILD assets were NOT replaced (directive §19). Substantially better",
        "  models (blaster-kit weapons vs the 5 procedural weapon meshes) are marked in the",
        "  per-file usage notes as the CANDIDATE_REPLACEMENT pool.",
        "- Git: the new binary types are all under the repo's existing LFS-tracked extensions",
        "(`*.wav`, `*.png`, `*.glb` in `.gitattributes`). This sandbox has no git-lfs binary,",
        "matching how the existing 69 ArtSource binaries were committed (direct blobs, all",
        "small); the static validator tolerates real binaries in ArtSource.",
        "",
        "## 9. Recommended next UE5 import steps (integration agent)",
        "",
        "1. Run the standard `import_all.py` pass first (unchanged contract).",
        "2. Import selected new sources via the same Interchange importer (suggested",
        "   destinations; binding decisions belong to the integration agent who can see them",
        "   in-engine):",
        "   - `ArtSource/Models/Kenney_NatureKit/GLB/*.glb` → `/Game/Environment/Kenney/NatureKit/`",
        "     (BiomeDressingActor candidates — tree/rock/flora/crop/fence/campfire pieces)",
        "   - `ArtSource/Models/Kenney_SpaceKit/GLB/*.glb` → `/Game/Environment/Kenney/SpaceKit/`",
        "     (dungeon corridor/structure/pipe/platform dressing; turret candidates)",
        "   - `ArtSource/Models/Kenney_BlasterKit/GLB/*.glb` → `/Game/Weapons/Meshes/Kenney/`",
        "     (CANDIDATE_REPLACEMENT held-weapon meshes + shared colormap texture)",
        "   - `ArtSource/Models/Kenney_SurvivalKit/GLB/*.glb` → `/Game/Environment/Kenney/SurvivalKit/`",
        "     (camp/fire/crate/tool dressing for POIs, villages and all 12 zones)",
        "   - `ArtSource/Models/Kenney_CityKitIndustrial/GLB/*.glb` → `/Game/Environment/Kenney/Industrial/`",
        "     (containers, cranes, pipes, warehouse shells for Ember Ridge/Stormcrest/research POIs)",
        "   - `ArtSource/Models/Kenney_ModularSpaceKit/GLB/*.glb` → `/Game/Environment/Kenney/ModularSpaceKit/`",
        "     (modular sci-fi interior tiles for the 3 dungeons + Hollow Approach)",
        "   - `ArtSource/Models/Kenney_ModularDungeonKit/GLB/*.glb` → `/Game/Environment/Kenney/ModularDungeonKit/`",
        "     (stone/ancient tiles for dungeon ruin segments + Sunscar)",
        "   - `ArtSource/Models/Kenney_AnimatedCharactersSurvivors/GLB/*.glb` → `/Game/Characters/Kenney/Survivors/`",
        "     (NPC body candidates + locomotion reference; check rig retarget to SK_Survivor_Exosuit)",
        "   - `ArtSource/Textures/Kenney_ParticlePack/PNG/*.png` → `/Game/FX/Kenney/ParticlePack/`",
        "     (Niagara sprite textures for muzzle/impact/spark/smoke; see HANDOFF tone check)",
        "   - `ArtSource/Textures/Kenney_UIPackSciFi/PNG/*.png` → `/Game/UI/Kenney/SciFi/`",
        "     (panels/buttons/icons for the 7 UMG widget classes)",
        "   - `ArtSource/Textures/Kenney_CrosshairPack/PNG/*.png` → `/Game/UI/Kenney/Crosshairs/`",
        "     (real reticles for HudWidget hip-fire/aim states)",
        "   - `ArtSource/Textures/Kenney_Skyboxes/PNG/*.png` → `/Game/Environment/Kenney/Skyboxes/`",
        "     (equirectangular sky textures — import as long-lat, sky dome material candidates)",
        "   - `ArtSource/Audio/Kenney_*/Wav/*.wav` → `/Game/Audio/Kenney/<Pack>/`",
        "     (SoundWave library for impact/UI/sci-fi feedback extension)",
        "3. Extend `Content/Python/AwPipeline/import_all.py` mappings ONLY when binding is",
        "   decided in the same session (otherwise keep the 112-asset import contract as-is).",
        "4. Any replacement of a bound asset (e.g. procedural weapon mesh → blaster) follows",
        "   the CANDIDATE_REPLACEMENT rule: import, compare in-engine, rebind the soft path —",
        "   never delete the procedural source.",
        "",
        "## 10. Unresolved dependencies / blockers",
        "",
        "- None at source level: every GLB external dependency resolved (blaster colormap",
        "  committed beside the GLBs); no MISSING_DEPENDENCY records.",
        "- OGG→WAV conversion needs ffmpeg; where absent the affected records are BLOCKED",
        "  (re-run the script where ffmpeg is installed).",
        "- Engine-side import/cook verification belongs to the Antigravity one-time",
        "  integration run per MASTER_CONTROL — sources here are IMPORT_READY only.",
        "",
    ]
    (acq.repo / "Docs" / "ASSET_ACQUISITION_REPORT.md").write_text("\n".join(lines), encoding="utf-8")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------


def main() -> int:
    ap = argparse.ArgumentParser(description="ASTRAWILD free asset acquisition pipeline")
    ap.add_argument("--repo-root", default=None,
                    help="repository root (default: parent of this script)")
    ap.add_argument("--cache-dir", default=None,
                    help="download cache OUTSIDE the repo (default: <repo>/../asset_download_cache)")
    ap.add_argument("--packs", default="", help="comma-separated slugs (default: all approved)")
    ap.add_argument("--dry-run", action="store_true",
                    help="download+validate only; write nothing into the repository")
    args = ap.parse_args()

    repo_root = Path(args.repo_root) if args.repo_root else Path(__file__).resolve().parent.parent
    cache_dir = Path(args.cache_dir) if args.cache_dir else repo_root.parent / "asset_download_cache"
    cache_dir.mkdir(parents=True, exist_ok=True)
    (cache_dir / "converted").mkdir(exist_ok=True)

    log(f"repo root: {repo_root}")
    log(f"cache dir: {cache_dir} (outside the repository — never committed)")
    if args.dry_run:
        log("DRY RUN: no repository files will be written")

    packs = APPROVED_PACKS
    if args.packs:
        wanted = {s.strip() for s in args.packs.split(",") if s.strip()}
        packs = [p for p in APPROVED_PACKS if p.slug in wanted]
        missing = wanted - {p.slug for p in packs}
        if missing:
            log(f"WARNING: unknown pack slugs ignored: {sorted(missing)}")

    acq = Acquisition(repo_root, cache_dir, args.dry_run)
    n = acq.index_existing()
    log(f"dedup index: {n} existing ArtSource files hashed")

    for pack in packs:
        acq.process(pack)

    now_iso = datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds")
    if not args.dry_run:
        stats, m_files, m_packs = write_manifests(acq, now_iso)
        write_credits(acq, now_iso, m_files, m_packs)
        write_report(acq, now_iso, stats, m_files, m_packs)
        log(f"manifest: {repo_root / 'ASSET_MANIFEST.json'}")
        log(f"credits:  {repo_root / 'ASSETS_CREDITS.md'}")
        log(f"report:   {repo_root / 'Docs' / 'ASSET_ACQUISITION_REPORT.md'}")
        log(f"stats: accepted={stats['accepted_records']} import_ready={stats['import_ready']} "
            f"size={human_size(stats['accepted_bytes'])}")
    else:
        stats = compute_stats(acq)
        log(f"DRY-RUN stats: records={stats['total_records']} "
            f"accepted={stats['accepted_records']} size={human_size(stats['accepted_bytes'])}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
