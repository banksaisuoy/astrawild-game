# ASTRAWILD ART & ASSET PLAN — LONG-RUN DIRECTIVE L7

> Consolidates the existing free-asset ledgers with the L6 placeholder
> ledger into ONE actionable binding plan. Every mapping below names a
> SPECIFIC asset that is either (a) already staged in this repo with
> per-asset license provenance, or (b) an already-approved free source with
> a citable license basis. **No license is claimed that is not cited.**

## 1. Sources of truth (existing ledgers — unchanged, referenced)

| Ledger | What it proves |
|---|---|
| `ArtSource/manifest.json` | 189 staged assets, **all CC0 1.0 Universal** with `license` + `license_url` per entry; 189/189 have `ue_path` (engine import staged) |
| `Docs/ASTRAWILD_REAL_ASSET_CREDITS.json` | Per-mesh author provenance (Quaternius / Kenney), generator `Scripts/fetch_free_assets.py`, 1:1 source-uniqueness rule |
| `Docs/ThirdPartyLicenses.md` | Human-readable license table incl. Kenney Modular Space Kit (41 GLB, CC0, serving the 3 dungeons + Hollow Approach) |
| `Docs/ASTRAWILD_FREE_ASSET_LEDGER.md` | The binding acquisition record: 15 Kenney packs (3,678 files), six Quaternius Ultimate packs, approved-not-required sources (Poly Haven, Freesound, OpenGameArt) |

License basis for everything below: **CC0 1.0 Universal**
(`https://creativecommons.org/publicdomain/zero/1.0/`) — verified per asset
in the manifest; Kenney packs additionally carry on-disk `License.txt`.

## 2. Placeholder → asset mapping (the L6 REPLACE_BEFORE_RELEASE ledger, closed)

All 20 code sites from `Docs/ASTRAWILD_SOURCE_INVENTORY.md` resolve to 9
visual targets; every target names its concrete asset:

| # | Placeholder target (code sites) | Bound asset (already staged unless noted) | Binding point |
|---|---|---|---|
| P-1 | Resource node visuals (`AstrawildResourceNode.cpp:32,189`) | `SM_Node_AncientVein`, `SM_Node_Astraite`, `SM_Node_Pyronite`, `SM_Node_Voidstone` (manifest `mesh`, CC0) | shape-by-rarity kits: the 4 rarity tiers map 1:1 to the 4 staged ore-node meshes; bind at the engine import pass (`Tools/ArtSourceGen` conventions) |
| P-2 | Dungeon room shells + floors (`AstrawildDungeonRoomActor.cpp:70,293`) | `Kenney_ModularDungeonKit` (40 GLB tiles on disk, CC0, `ArtSource/Models/Kenney_ModularDungeonKit/`) | wall/floor/corner tiles per the 3 dungeon themes (ResolveDungeonTheme switch) |
| P-3 | Dungeon gate pillars (`AstrawildDungeonGateActor.cpp:26`) | `Kenney_ModularDungeonKit` pillar/corner tiles (same pack as P-2 — kit coherence) | gate frame composition |
| P-4 | Dungeon portal pads (`AstrawildDungeonPortalActor.cpp:20`) | `Kenney_ModularSpaceKit` floor-panel GLBs (41 tiles, CC0 — already the ledgered dungeon dressing pack) | flat pad + emissive rim via `T_Crystal_E` (staged, CC0) |
| P-5 | Resonance pillars (`AstrawildResonancePillarActor.cpp:25`) | `SM_Ruin_Pillar` (manifest `mesh`, CC0) + `SM_Ruin_Arch`/`SM_Ruin_Block` for the gate cluster | tall-stone identity, tint through the existing per-zone tint pipeline |
| P-6 | POI markers (`AstrawildPOIMarkerActor.h:16`) | `SM_Ruin_Pillar` variants + `SM_Ruin_Arch` for landmark POIs (staged, CC0) | keep the runtime tint+light behavior; swap only the stand-in mesh |
| P-7 | Boss hazard field (`AstrawildBossHazardActor.cpp:16`, `.h:15`) | In-engine Niagara system (authored, zero external dependency); flipbook/emissive textures from staged `T_FX_Flare_E`, `T_FX_Sparks_FB`, `T_FX_Smoke_FB` (CC0) | hazard sphere swaps to an element-tinted Niagara field |
| P-8 | Boss telegraph ring (`AstrawildBossTelegraphActor.cpp:13`, `.h:22`) | In-engine decal ring (material-authored) using `T_FX_Smoke_FB`/`T_FX_Sparks_FB` frames; optional **Kenney Particle Pack** (CC0, `https://kenney.nl/assets/particle-pack`) for extra ring flipbooks — ledger §3 AS-NEEDED rule | flat cylinder → animated decal/Niagara ring |
| P-9 | Projectile visual (`AstrawildProjectileActor.cpp:40`) | Niagara trail + `T_FX_Flare_E` emissive (staged, CC0) | element-tinted bolt/trail |
| P-10 | Utility drone body (`AstrawildUtilityDroneActor.cpp:24`) | `Kenney_SpaceKit` prop GLBs (on disk, CC0) — small hull/antenna pieces | placeholder box → kit-built micro-hull |
| P-11 | Utility robot chassis (`AstrawildUtilityRobotActor.cpp:17`) | `Kenney_AnimatedCharactersSurvivors` / `Kenney_BlasterKit` bot pieces (on disk, CC0) | chassis silhouette per work type |
| P-12 | Boss glow material (`AstrawildEchoBossCharacter.cpp:45,55`) | Material-authored (engine); emissive from `T_Crystal_E` (staged, CC0); boss bodies already have real meshes (14 `SK_Boss_*` + Tier-B binds) | cone → existing `SK_Boss_*`/Tier-B mesh + pulse material |

**Rule**: every binding above is an ENGINE-PASS task (import + assign); no
source-code value changes are required (the REPLACE markers are visual
swap points by design — fail-closed stand-ins).

## 3. Unfilled art slots beyond the placeholders (engine-import queue)

From the asset-truth record (L2/L6 re-verified against the manifest):

| Slot family | Count | Staged assets ready | Remaining action |
|---|---|---|---|
| Echo base archetypes | 16 `SK_Base_*` | all 16 staged (CC0) | engine import via `import_echo_bases.py` (V2-35 queue row) |
| Tier-B echo meshes | 47 `SK_Echo_*` GLB-backed ids | all staged | engine import + binding (opt-in path) |
| Boss meshes | 14 `SK_Boss_*` | all staged | engine import (showcase + encounters) |
| Survivor rigs | 3 tiers | staged | engine import + anim clip map (Idle/Walk/Run/Roll→Jump/…) |
| Nature/ruin props | 29 `SM_*` props | staged | engine import (zone dressing) |
| Ore nodes | 4 `SM_Node_*` | staged | engine import + rarity bind (P-1) |
| Vehicles | `SM_Vehicle_DawnSkiff` + rover | staged | engine import |
| Landscape/environment/fx/survivor textures | 41 `T_*` | staged | engine import (materials) |
| Audio | 36 entries (ambience/ui/weapon/footstep/creature/player) | staged, CC0 | engine import (sound bases) |

**63 pending ue_paths** are all GLB-backed Echo/SK_Base rows — by design
opt-in engine imports (asset-truth: TRUE_MISSING 0).

## 4. Approved sources for any FUTURE gap (carried from ledger §3 — nothing new claimed)

| Source | License basis | Standing decision |
|---|---|---|
| Poly Haven (models/textures/HDRI) | site-wide CC0 (`https://polyhaven.com/license`) | APPROVED-NOT-REQUIRED — P2 realism upgrade path only |
| Kenney remaining catalog incl. Particle Pack | CC0 (`https://kenney.nl/assets/particle-pack` for P-8) | AS-NEEDED with matrix gap |
| Freesound | CC0 filter | APPROVED-NOT-REQUIRED (Kenney audio covers the matrix) |
| OpenGameArt | per-asset CC0 (machine-parseable) | DEFERRED |

## 5. What this plan does NOT claim

- No asset is claimed IMPORTED — every row above is staged-on-disk with
  license provenance; import happens on the machine (R3/R7).
- No license beyond CC0 1.0 Universal is relied upon anywhere in the
  current art surface; Mixamo/Fab/Sketchfab appear in NO binding because
  nothing staged requires them (recorded to keep the plan honest — those
  sources were not needed and are not cited as evidence).
- Content canon is untouched (Directive Q2): this plan only swaps visual
  stand-ins; no species/zone/quest/recipe/building is added or removed.
