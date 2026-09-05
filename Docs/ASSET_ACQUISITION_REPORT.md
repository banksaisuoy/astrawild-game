# ASTRAWILD — Asset Acquisition Report

**Generated**: 2026-09-04T15:14:30+00:00 · **Generator**: `Scripts/download_assets.py`
**Context**: branch `final-completion` (post READY_FOR_FINAL_BUILD source gate).
This run adds supplementary CC0 source assets only — no gameplay code, no soft-path
bindings and no existing ArtSource file were changed.

## 1. Sources searched

| Source | Result |
| :--- | :--- |
| kenney.nl (Priority 1, official publisher — batch 1) | 10 pack pages probed; 7 candidates evaluated; 6 accepted; 4 packs rejected with documented reasons |
| kenney.nl (Priority 1 — batch 2 gap analysis) | full catalog walk: all 14 pagination pages enumerated (214 packs); 21 candidate pack pages read individually; 9 more packs accepted (this run); 2D creature/character family rejected (format); zip sizes measured via HTTP HEAD |
| github.com/KenneyNL | not needed — official publisher downloads available for every pack |
| quaternius.com | deferred — Ultimate-series CC0 pages exist, but delivery is a Google Drive folder (not a direct URL); newer packs use QAL (redistribution prohibited) |
| opengameart.org (Priority 2) | deferred — researched and viable (machine-parseable per-asset licenses, anonymous direct downloads), needs a downloader license-gate extension first |
| polyhaven.com / ambientcg.com | site-wide CC0 verified; realistic PBR/HDRIs are a P2 upgrade path — deliberately not acquired |

## 2. Packs accepted

| Pack | Category | Files accepted | Size | Pack status |
| :--- | :--- | ---: | ---: | :--- |
| Impact Sounds | Audio | 257 | 9.7 MB | ACCEPTED (sources IMPORT_READY) |
| Interface Sounds | Audio | 201 | 4.0 MB | ACCEPTED (sources IMPORT_READY) |
| Sci-fi Sounds | Audio | 148 | 24.4 MB | ACCEPTED (sources IMPORT_READY) |
| Nature Kit | Models | 315 | 2.8 MB | ACCEPTED (sources IMPORT_READY) |
| Space Kit | Models | 108 | 1.3 MB | ACCEPTED (sources IMPORT_READY) |
| Blaster Kit | Models | 42 | 1.2 MB | ACCEPTED (sources IMPORT_READY) |
| Particle Pack | Textures | 97 | 5.1 MB | ACCEPTED (sources IMPORT_READY) |
| UI Pack: Sci-Fi | Textures | 693 | 376.8 KB | ACCEPTED (sources IMPORT_READY) |
| Survival Kit | Models | 82 | 1.2 MB | ACCEPTED (sources IMPORT_READY) |
| City Kit (Industrial) | Models | 39 | 3.0 MB | ACCEPTED (sources IMPORT_READY) |
| Modular Space Kit | Models | 42 | 7.7 MB | ACCEPTED (sources IMPORT_READY) |
| Modular Dungeon Kit | Models | 41 | 7.1 MB | ACCEPTED (sources IMPORT_READY) |
| Animated Characters: Survivors | Models | 5 | 1.8 MB | ACCEPTED (sources IMPORT_READY) |
| Skyboxes | Textures | 6 | 4.5 MB | ACCEPTED (sources IMPORT_READY) |
| Crosshair Pack | Textures | 1602 | 1.5 MB | ACCEPTED (sources IMPORT_READY) |

## 3. Packs rejected (with reasons)

| Pack | Status | Reason |
| :--- | :--- | :--- |
| Sci-Fi RTS | REJECTED_FORMAT | 2D top-down sprite pack (259 PNGs, spritesheets, SVG/SWF) — not 3D models. ASTRAWILD is a 3D third-person game with no top-down sprite consuming system. |
| Digital Audio | REJECTED_QUALITY | Retro 8-bit aesthetic — does not fit the sci-fi survival frontier audio identity. |
| UI Audio | REJECTED_QUALITY | Duplicate role — superseded by Interface Sounds (newer, larger pack covering the same UI feedback category); accepting both would create near-duplicate libraries. |
| RPG Audio | REJECTED_QUALITY | Fantasy-specific sounds (coins, potions, spells) — weak fit for sci-fi survival; no consuming system. |
| Kenney 2D creature/character packs (Monster Builder, Animal Pack, Animal Pack Remastered, Alien UFO, Robot, Fish, Toon Characters, Shape Characters) | REJECTED_FORMAT | All 2D sprite packs — ASTRAWILD needs 3D meshes for creatures/NPCs; the hoped-for 3D creature catalog does not exist at Kenney (verified during the batch-2 gap analysis, full catalog walk). |
| Quaternius newer packs (Sci-Fi Essentials Kit, MegaKits, Universal Animation Library 2) | REJECTED_LICENSE | Custom QAL v1.0 license — free commercial use but redistribution prohibited, which forbids committing the assets into this repository; only Quaternius 'Ultimate'-series pages explicitly stating CC0 are eligible. |

## 4. File-level results

- Total file records: **3904**
- Accepted: **3678** (75.8 MB)
- Import-ready (UE5 import format): **3360**
- Duplicates skipped: **54**
- Rejected (format): **0**
- Rejected (quality/curation): **172**
- Missing dependencies: **0**
- Blocked: **0**
- Audio files accepted (OGG originals + WAV conversions): **602**
- Model files accepted (GLB): **661**
- Texture files accepted (PNG): **2398**
- Format-duplicate files skipped at selection (FBX/OBJ/DAE/STL/MTL copies, 2D
  preview sprites, `.url` shortcuts) are counted per pack in
  `Docs/ASSET_ACQUISITION_MANIFEST.json` (files_format_skipped) — never committed.

## 5. License verification

All accepted packs: **CC0 1.0 Universal (Public Domain Dedication)** — LICENSE_VERIFIED.

Verified per pack page at kenney.nl: the license table row links to https://creativecommons.org/publicdomain/zero/1.0/ ('Creative Commons CC0') and the page meta description states 'free, CC0 licensed!'. Batch 1 (6 packs) verified 2025-09-04 before acquisition; batch 2 (9 packs) verified the same way during the wayfinder gap analysis (full catalog walk of all 14 pagination pages, 21 candidate pack pages read, per-page license rows + CC0 legal-code links confirmed, zip sizes measured via HTTP HEAD) and re-checked on acquisition day.

CC0 requires no attribution and permits commercial use and redistribution;
provenance is recorded in `ASSETS_CREDITS.md`, `ASSET_MANIFEST.json`,
`Docs/ASSET_ACQUISITION_MANIFEST.json` and the per-pack `License.txt` files committed
alongside the assets. No CC-BY or unclear-license asset was accepted.

## 6. Format validation performed

- **WAV (converted)**: RIFF/WAVE header parsed — PCM 16-bit, source sample rate and
  channel count preserved, duration computed. Corrupt files are REJECTED.
- **OGG (originals)**: probed via ffprobe (Vorbis, 44.1 kHz, channel count preserved
  per source — mono and stereo both occur) — preserved verbatim as source provenance;
  UE5 does not import OGG directly, so these carry `import_ready: false`.
- **GLB**: binary glTF magic + JSON chunk parsed; meshes/materials/nodes counted;
  triangle totals computed from index accessors; external URI dependencies resolved
  (blaster-kit GLBs reference `Textures/colormap.png`, which is committed beside them
  so relative URIs keep resolving).
- **PNG**: signature + IHDR dimensions verified.
- No file extension was renamed or silently converted — conversions are separate,
  documented files.

## 7. Storage control

New accepted source payload: **75.8 MB** (directive soft
limits: 2 GB per pack, 10 GB total — far below both). Audio packs keep OGG originals
(provenance) AND 16-bit PCM WAV conversions (UE5 import format). 3D packs keep GLB
only; FBX/OBJ/DAE/STL/MTL duplicates and preview images were dropped at selection
time (never committed).

## 8. Repository integration (what changed)

- New sources under `ArtSource/Audio/Kenney_*/` (Ogg/ + Wav/ + License.txt),
  `ArtSource/Models/Kenney_*/` (GLB/ + License.txt) and — new in batch 2 —
  `ArtSource/Textures/Kenney_*/` (PNG/ + License.txt: particles, sci-fi UI,
  crosshairs, skyboxes).
- The existing UE import pipeline (`Content/Python/AwPipeline/import_all.py`) only
  auto-imports the FLAT `ArtSource/Audio/*.wav` and `ArtSource/Textures/*.png` folders,
  so the new pack subfolders are NOT auto-imported — current bindings and fallback
  chains are untouched (soft-path contract intact; zero-asset boot still guaranteed).
- No `.uasset`/`.umap` was fabricated. All new files are import-ready SOURCES with
  status IMPORT_READY — **not** UE5_VERIFIED (engine import/cook is owned by the
  Antigravity integration run).
- Existing ASTRAWILD assets were NOT replaced (directive §19). Substantially better
  models (blaster-kit weapons vs the 5 procedural weapon meshes) are marked in the
  per-file usage notes as the CANDIDATE_REPLACEMENT pool.
- Git: the new binary types are all under the repo's existing LFS-tracked extensions
(`*.wav`, `*.png`, `*.glb` in `.gitattributes`). This sandbox has no git-lfs binary,
matching how the existing 69 ArtSource binaries were committed (direct blobs, all
small); the static validator tolerates real binaries in ArtSource.

## 9. Recommended next UE5 import steps (integration agent)

1. Run the standard `import_all.py` pass first (unchanged contract).
2. Import selected new sources via the same Interchange importer (suggested
   destinations; binding decisions belong to the integration agent who can see them
   in-engine):
   - `ArtSource/Models/Kenney_NatureKit/GLB/*.glb` → `/Game/Environment/Kenney/NatureKit/`
     (BiomeDressingActor candidates — tree/rock/flora/crop/fence/campfire pieces)
   - `ArtSource/Models/Kenney_SpaceKit/GLB/*.glb` → `/Game/Environment/Kenney/SpaceKit/`
     (dungeon corridor/structure/pipe/platform dressing; turret candidates)
   - `ArtSource/Models/Kenney_BlasterKit/GLB/*.glb` → `/Game/Weapons/Meshes/Kenney/`
     (CANDIDATE_REPLACEMENT held-weapon meshes + shared colormap texture)
   - `ArtSource/Models/Kenney_SurvivalKit/GLB/*.glb` → `/Game/Environment/Kenney/SurvivalKit/`
     (camp/fire/crate/tool dressing for POIs, villages and all 12 zones)
   - `ArtSource/Models/Kenney_CityKitIndustrial/GLB/*.glb` → `/Game/Environment/Kenney/Industrial/`
     (containers, cranes, pipes, warehouse shells for Ember Ridge/Stormcrest/research POIs)
   - `ArtSource/Models/Kenney_ModularSpaceKit/GLB/*.glb` → `/Game/Environment/Kenney/ModularSpaceKit/`
     (modular sci-fi interior tiles for the 3 dungeons + Hollow Approach)
   - `ArtSource/Models/Kenney_ModularDungeonKit/GLB/*.glb` → `/Game/Environment/Kenney/ModularDungeonKit/`
     (stone/ancient tiles for dungeon ruin segments + Sunscar)
   - `ArtSource/Models/Kenney_AnimatedCharactersSurvivors/GLB/*.glb` → `/Game/Characters/Kenney/Survivors/`
     (NPC body candidates + locomotion reference; check rig retarget to SK_Survivor_Exosuit)
   - `ArtSource/Textures/Kenney_ParticlePack/PNG/*.png` → `/Game/FX/Kenney/ParticlePack/`
     (Niagara sprite textures for muzzle/impact/spark/smoke; see HANDOFF tone check)
   - `ArtSource/Textures/Kenney_UIPackSciFi/PNG/*.png` → `/Game/UI/Kenney/SciFi/`
     (panels/buttons/icons for the 7 UMG widget classes)
   - `ArtSource/Textures/Kenney_CrosshairPack/PNG/*.png` → `/Game/UI/Kenney/Crosshairs/`
     (real reticles for HudWidget hip-fire/aim states)
   - `ArtSource/Textures/Kenney_Skyboxes/PNG/*.png` → `/Game/Environment/Kenney/Skyboxes/`
     (equirectangular sky textures — import as long-lat, sky dome material candidates)
   - `ArtSource/Audio/Kenney_*/Wav/*.wav` → `/Game/Audio/Kenney/<Pack>/`
     (SoundWave library for impact/UI/sci-fi feedback extension)
3. Extend `Content/Python/AwPipeline/import_all.py` mappings ONLY when binding is
   decided in the same session (otherwise keep the 112-asset import contract as-is).
4. Any replacement of a bound asset (e.g. procedural weapon mesh → blaster) follows
   the CANDIDATE_REPLACEMENT rule: import, compare in-engine, rebind the soft path —
   never delete the procedural source.

## 10. Unresolved dependencies / blockers

- None at source level: every GLB external dependency resolved (blaster colormap
  committed beside the GLBs); no MISSING_DEPENDENCY records.
- OGG→WAV conversion needs ffmpeg; where absent the affected records are BLOCKED
  (re-run the script where ffmpeg is installed).
- Engine-side import/cook verification belongs to the Antigravity one-time
  integration run per MASTER_CONTROL — sources here are IMPORT_READY only.
