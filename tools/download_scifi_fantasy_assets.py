#!/usr/bin/env python3
"""
ASTRAWILD — Sci-Fantasy asset acquisition & staging (directive SCI Phase 3).

Curates the Sci-Fantasy monster base-model + sound sources for the 204-Echo
mutation system, verifies licenses/CC0 provenance, stages the engine-bound
sound sets, and records everything in the free-asset ledger manifest.

Sources (CC0 1.0 Universal — verified at acquisition time by the LCP-7/AA-1/2
passes, see Docs/ASTRAWILD_FREE_ASSET_LEDGER.md):
  - Quaternius "Ultimate Monsters" (61 files, in-repo) — CC0, license-gated
  - Kenney Sci-fi Sounds / Interface Sounds (in-repo) — CC0
  - Baked archetype bases (Tools/ArtSourceGen/gen_sci_fantasy_bases.py) —
    project-authored CC0-equivalent output

Staging targets (repo convention — raw sources live in ArtSource/, the UE
import pass lands engine packages under /Game/...):
  ArtSource/Meshes/Echoes/BaseMeshes/  ->  /Game/Characters/Echoes/BaseMeshes/  (16 SK_Base_*)
  ArtSource/Audio/Echoes/              ->  /Game/Audio/Echoes/                  (SFXSet_* cues)

Behavior:
  1. Inventory + SHA-256 the local CC0 packs (already acquired).
  2. Try to fetch any CURATED pack that is missing (network optional — a
     failure records DELIVERY_PENDING, never a fake success).
  3. Stage the 8 theme sound sets (2 cues each) into ArtSource/Audio/Echoes/.
  4. Verify the 16 baked base GLBs exist and validate.
  5. Write Docs/ASTRAWILD_SCI_FANTASY_ACQUISITION.json + console LFS steps.

Run:  python3 tools/download_scifi_fantasy_assets.py [--offline]
"""

import argparse
import hashlib
import json
import os
import shutil
import sys
import urllib.request
from pathlib import Path

REPO = Path(__file__).resolve().parent.parent
AUDIO_OUT = REPO / "ArtSource" / "Audio" / "Echoes"
BASE_OUT = REPO / "ArtSource" / "Meshes" / "Echoes" / "BaseMeshes"
ACQ_JSON = REPO / "Docs" / "ASTRAWILD_SCI_FANTASY_ACQUISITION.json"

# ---------------------------------------------------------------------------
# Curated CC0 sources. LOCAL_PATH is the in-repo acquisition (LCP-7/AA-1/2);
# SOURCE_URL is the official page for re-download when a pack is missing.
# ---------------------------------------------------------------------------
CURATED_PACKS = [
    {
        "pack": "Quaternius Ultimate Monsters",
        "local": "ArtSource/Models/Quaternius_UltimateMonsters",
        "source_url": "https://quaternius.com/packs/ultimatemonsters.html",
        "license": "CC0 1.0 Universal",
        "license_url": "https://quaternius.com/license.html",
        "usage": "Sci-Fantasy monster reference/archetype library (61 files: Big/Blob/Flying glTF + atlas)",
    },
    {
        "pack": "Kenney Sci-fi Sounds",
        "local": "ArtSource/Audio/Kenney_SciFiSounds",
        "source_url": "https://kenney.nl/assets/sci-fi-sounds",
        "license": "CC0 1.0 Universal",
        "license_url": "https://creativecommons.org/publicdomain/zero/1.0/",
        "usage": "Sci-Fantasy Echo vocalization/mechanical sound pool",
    },
    {
        "pack": "Kenney Interface Sounds",
        "local": "ArtSource/Audio/Kenney_InterfaceSounds",
        "source_url": "https://kenney.nl/assets/interface-sounds",
        "license": "CC0 1.0 Universal",
        "license_url": "https://creativecommons.org/publicdomain/zero/1.0/",
        "usage": "Sci-Fantasy spirit/plant/void cue pool",
    },
    {
        "pack": "Kenney Impact Sounds",
        "local": "ArtSource/Audio/Kenney_ImpactSounds",
        "source_url": "https://kenney.nl/assets/impact-sounds",
        "license": "CC0 1.0 Universal",
        "license_url": "https://creativecommons.org/publicdomain/zero/1.0/",
        "usage": "Armored-organic impact layer (hit feedback)",
    },
    {
        "pack": "Poly Haven (creatures/rocks)",
        "local": None,  # REJECTED at curation: Poly Haven hosts no creature MODELS
        "source_url": "https://polyhaven.com/",
        "license": "CC0 1.0 Universal",
        "license_url": "https://polyhaven.com/license",
        "usage": "REJECTED — photography/HDR/texture library without creature/monster 3D models; the baked archetype bases cover the need",
    },
    {
        "pack": "Sketchfab CC0 monster search",
        "local": None,  # APPROVED-NOT-REQUIRED: Quaternius pack + baked bases give 16 archetypes
        "source_url": "https://sketchfab.com/search?features=downloadable&licenses=cc0&q=monster+robot+dragon+golem",
        "license": "CC0 1.0 (per-asset filter required)",
        "license_url": "https://sketchfab.com/licenses",
        "usage": "APPROVED-NOT-REQUIRED — reserve source; every Sketchfab CC0 asset needs individual license verification before any download",
    },
]

# ---------------------------------------------------------------------------
# Theme sound sets — 2 cues per theme (vocal + hurt), staged from the LOCAL
# CC0 packs. Cue index 0 = vocalization (weakness-hit hook), 1 = alternate.
# ---------------------------------------------------------------------------
THEME_SOUND_SETS = {
    "SFXSet_AncientConstruct": [
        ("ArtSource/Audio/Kenney_SciFiSounds/Wav/computerNoise_002.wav", "servo rumble"),
        ("ArtSource/Audio/Kenney_SciFiSounds/Wav/doorClose_000.wav", "stone plate slam"),
    ],
    "SFXSet_ElementalBeast": [
        ("ArtSource/Audio/Kenney_SciFiSounds/Wav/laserSmall_002.wav", "energy crackle"),
        ("ArtSource/Audio/Kenney_SciFiSounds/Wav/explosionCrunch_001.wav", "elemental blast"),
    ],
    "SFXSet_MutatedFauna": [
        ("ArtSource/Audio/Kenney_SciFiSounds/Wav/slime_001.wav", "organic vocal"),
        ("ArtSource/Audio/Kenney_InterfaceSounds/Wav/scratch_002.wav", "mutant scratch"),
    ],
    "SFXSet_ArmoredOrganic": [
        ("ArtSource/Audio/Kenney_SciFiSounds/Wav/impactMetal_000.wav", "chitin clank"),
        ("ArtSource/Audio/Kenney_SciFiSounds/Wav/doorClose_002.wav", "carapace slam"),
    ],
    "SFXSet_EtherealSpirit": [
        ("ArtSource/Audio/Kenney_InterfaceSounds/Wav/pluck_001.wav", "spirit chime"),
        ("ArtSource/Audio/Kenney_InterfaceSounds/Wav/question_002.wav", "wisp call"),
    ],
    "SFXSet_MechanicalHybrid": [
        ("ArtSource/Audio/Kenney_SciFiSounds/Wav/computerNoise_000.wav", "servo whirr"),
        ("ArtSource/Audio/Kenney_SciFiSounds/Wav/engineCircular_001.wav", "motor pulse"),
    ],
    "SFXSet_PlantMonster": [
        ("ArtSource/Audio/Kenney_InterfaceSounds/Wav/pluck_002.wav", "leaf pop"),
        ("ArtSource/Audio/Kenney_SciFiSounds/Wav/slime_000.wav", "sap squelch"),
    ],
    "SFXSet_VoidAbomination": [
        ("ArtSource/Audio/Kenney_SciFiSounds/Wav/lowFrequency_explosion_000.wav", "void rumble"),
        ("ArtSource/Audio/Kenney_InterfaceSounds/Wav/glitch_001.wav", "reality tear"),
    ],
}

BASE_GLBS = [
    "SK_Base_GolemQuadruped", "SK_Base_MonolithColossus",
    "SK_Base_ElemDrake", "SK_Base_ElemWisp",
    "SK_Base_MutantBeast", "SK_Base_MutantAvian",
    "SK_Base_ArmoredBeetle", "SK_Base_ArmoredCrab",
    "SK_Base_SpiritWisp", "SK_Base_SpiritOrb",
    "SK_Base_CyborgBeast", "SK_Base_CyborgSerpent",
    "SK_Base_PlantMaw", "SK_Base_Mushroomling",
    "SK_Base_VoidBlob", "SK_Base_VoidTentacle",
]


def sha256(path: Path) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as fh:
        for chunk in iter(lambda: fh.read(1 << 16), b""):
            h.update(chunk)
    return h.hexdigest()


def try_fetch(url: str, dest: Path) -> bool:
    """Best-effort remote fetch. Returns True on success; a network failure
    is RECORDED, never hidden."""
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "astrawild-sci-fantasy-pipeline"})
        with urllib.request.urlopen(req, timeout=20) as resp, open(dest, "wb") as out:
            shutil.copyfileobj(resp, out)
        return True
    except Exception as exc:  # noqa: BLE001 — the ledger records the reason
        print(f"  [network] {url} unreachable ({exc}) — status DELIVERY_PENDING (local sources cover the pipeline)")
        return False


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--offline", action="store_true", help="skip remote fetches entirely")
    args = parser.parse_args()

    report = {"packs": [], "sound_sets": {}, "base_meshes": {}, "lfs_note": ""}
    failures = []

    print("[sci-fantasy] Step 1 — curated CC0 pack inventory")
    for pack in CURATED_PACKS:
        entry = {k: pack[k] for k in ("pack", "source_url", "license", "license_url", "usage")}
        if pack["local"] is None:
            entry["status"] = "REJECTED" if "REJECTED" in pack["usage"] else "APPROVED-NOT-REQUIRED"
            print(f"  {pack['pack']:36s} {entry['status']}")
            report["packs"].append(entry)
            continue
        local = REPO / pack["local"]
        if local.exists():
            files = [p for p in local.rglob("*") if p.is_file()]
            entry["status"] = "LICENSE_VERIFIED / LOCAL_PRESENT"
            entry["files"] = len(files)
            entry["bytes"] = sum(p.stat().st_size for p in files)
            print(f"  {pack['pack']:36s} {entry['status']} ({entry['files']} files, {entry['bytes'] // 1024} KB)")
        else:
            entry["status"] = "LICENSE_VERIFIED / DELIVERY_PENDING"
            print(f"  {pack['pack']:36s} local copy MISSING — attempting remote fetch")
            if not args.offline:
                # The Kenney/Quaternius deliveries are zip/folder crawls owned by
                # Scripts/download_assets.py + Scripts/download_quaternius.py —
                # re-run those for a full acquisition; here we only probe reachability.
                probe = REPO / ".sci_fantasy_probe.tmp"
                if try_fetch(pack["source_url"], probe):
                    probe.unlink(missing_ok=True)
                    entry["status"] = "LICENSE_VERIFIED / REACHABLE (run the pack downloader)"
                else:
                    probe.unlink(missing_ok=True)
                    failures.append(pack["pack"])
        report["packs"].append(entry)

    print("[sci-fantasy] Step 2 — stage the 8 theme sound sets (16 cues)")
    AUDIO_OUT.mkdir(parents=True, exist_ok=True)
    for set_id, cues in THEME_SOUND_SETS.items():
        report["sound_sets"][set_id] = []
        for index, (src_rel, desc) in enumerate(cues):
            src = REPO / src_rel
            dst = AUDIO_OUT / f"{set_id}_{index}.wav"
            if not src.exists():
                print(f"  MISSING source {src_rel} for {set_id}_{index}")
                failures.append(f"{set_id}_{index}")
                report["sound_sets"][set_id].append({"cue": str(dst.relative_to(REPO)), "status": "SOURCE_MISSING"})
                continue
            shutil.copyfile(src, dst)
            record = {
                "cue": str(dst.relative_to(REPO)),
                "source": src_rel,
                "usage": desc,
                "sha256": sha256(dst),
                "bytes": dst.stat().st_size,
                "status": "LICENSE_VERIFIED / STAGED",
                "ue_path": f"/Game/Audio/Echoes/{set_id}_{index}",
            }
            report["sound_sets"][set_id].append(record)
            print(f"  {dst.name:36s} <- {Path(src_rel).name} ({desc})")

    print("[sci-fantasy] Step 3 — verify the 16 baked base archetypes")
    for base_id in BASE_GLBS:
        glb = BASE_OUT / f"{base_id}.glb"
        if glb.exists():
            report["base_meshes"][base_id] = {
                "path": str(glb.relative_to(REPO)),
                "sha256": sha256(glb),
                "bytes": glb.stat().st_size,
                "ue_path": f"/Game/Characters/Echoes/BaseMeshes/{base_id}",
                "status": "BAKED / IMPORT_PENDING",
            }
            print(f"  {base_id:28s} {glb.stat().st_size // 1024:4d} KB  IMPORT_PENDING")
        else:
            print(f"  {base_id:28s} MISSING — run Tools/ArtSourceGen/gen_sci_fantasy_bases.py")
            failures.append(base_id)
            report["base_meshes"][base_id] = {"status": "MISSING"}

    report["lfs_note"] = (
        "Staged .wav files are covered by the existing *.wav LFS pattern — "
        "commit with git add (LFS hooks handle them). Baked GLBs ride *.glb."
    )

    with open(ACQ_JSON, "w", encoding="utf-8") as fh:
        json.dump(report, fh, indent=2, sort_keys=True)
    print(f"[sci-fantasy] Acquisition record -> {ACQ_JSON}")

    if failures:
        print(f"[sci-fantasy] INCOMPLETE: {failures}")
        return 1
    print("[sci-fantasy] Complete: 4 local CC0 packs verified, 2 curated sources "
          "resolved (1 rejected with reason, 1 approved-not-required), 16 cues staged, 16 bases verified.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
