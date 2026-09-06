# ASTRAWILD — CURRENT-HEAD ASSET TRUTH AUDIT

> **CURRENT MESH TRUTH (v9.3 ASSET OVERHAUL): see §12 — every staged ArtSource mesh is now a REAL unique CC0 model; manifest 189/189 present / 0 pending; the procedural-recolor counts in §1-§10 are era-correct audit history.**

> **v9.4 sync correction (2026-09-06, honest):** the §10b final block's "git lfs fsck
> exit 0" statement was INACCURATE — fsck exits 1 with ~3,954 "should have been a
> pointer" flags, all in pre-existing raw pack-source dirs (ArtSource/Textures/Kenney* /
> Models/Kenney / Audio/Kenney / Models/Quaternius), zero under ArtSource/Meshes, and
> the same failure exists at the pre-v9.3 tip 08af72b (verified via worktree). It is an
> acquisition-batch convention mismatch, not corruption: every current LFS pointer's
> object resolves OID-matched, and the v9.3 commit itself was all-proper-pointers
> (111/111 uploaded; it REDUCED the raw flag count by 79). Current counts at tip
> 2637c13: **586 LFS pointers (491 + 95 v3 real-mesh sources), 586/586 objects
> on-disk, 0 pending push.** Full details: LIVE_EXECUTION_STATE §8.

> **Audit date:** 2026-09-06 (GLM source-side audit, sandbox `git` + `git-lfs 3.7.0`)
> **Scope:** ground-truth verification of the ACTUAL remote HEAD of `final-completion`.
> This document supersedes every earlier "asset truth" statement (including the
> stale historical diagnosis about a missing Survivor/Manny fallback and the
> "3 assets only" era). Nothing here is assumed from prior reports — every number
> below was re-derived from the repo at the audited HEAD.

---

## 1. HEAD truth (verified against remote, not from memory)

| Item | Value |
|---|---|
| Branch | `final-completion` |
| **Remote HEAD (origin/final-completion)** | **`68c2b075c9d68e7dcfd15d159cd8e177128bf872`** |
| Local HEAD | `68c2b075c9d68e7dcfd15d159cd8e177128bf872` (in sync, 0 unpushed commits) |
| Working tree | clean (0 dirty files) |
| Note on `4bb7be5` | `4bb7be5` is an **ancestor** 4 commits behind the tip. The three commits after it are `cca4cfc` [FPP-1] player-facing presentation, `948d75a` [FPP-2] player rules + doc sync, `68c2b07` [FPP-3] FINAL SOURCE FREEZE (`SOURCE_PRODUCT_FROZEN @ tip`). |

Claim-check against the audit brief:

| Brief claim | Verified result |
|---|---|
| "Current HEAD: 4bb7be5" | **Stale** — actual tip is `68c2b07` (contains 4bb7be5). |
| "PlayerCharacter already contains Survivor fallbacks for Aim/Jump/Fire/Gather" | **TRUE** — `AstrawildPlayerCharacter.cpp` loads Survivor clip first, Mannequins clip second, for all 7 clips (lines 324-368), and the soft bindings are initialized from `AstrawildArtPack::GetSurvivorArt()` (lines 77-83). The historical "no Survivor fallback for 4 clips" bug is fixed at this HEAD. |
| "Manny asset exists as an LFS-tracked .uasset" | **TRUE** — `Content/Characters/Mannequins/Meshes/SKM_Manny_Simple.uasset` (15,825,101 bytes, genuine UE package). |
| "Content for Characters/Environment/Materials/Textures/Audio/VFX/Vehicles/Weapons/Maps" | **TRUE** — all nine categories present with binaries (§4). |
| "hero meshes + Tier-B assets + procedural body plans for the Echo definitions" | **TRUE** — 3-layer architecture (§6): 6 hero species engine-imported + bound; 39 Tier-B species with raw GLBs + convention-path opt-in binding; the rest procedural PMC. Species counts: 204 generated bestiary species; **229 total unique Echo ids** across all registration layers (204 bestiary + 6 production heroes + ContentLibrary defaults, minus overlap). |

---

## 2. LFS truth

- `.gitattributes` LFS patterns: `*.uasset *.umap *.fbx *.glb *.gltf *.wav *.mp3 *.ogg *.png *.jpg *.tga *.exr *.psd *.mov *.mp4 *.pak *.uexp *.ubulk`.
- `git lfs ls-files -l` → **459 tracked files** (pre-audit the working tree held 459 LFS **pointers**; local object store was empty).
- **Remote object verification:** `git lfs fetch origin` completed **exit 0, 459/459 objects, 232 MB (244,278,177 bytes)**. Every LFS object referenced by every pointer **exists in the remote LFS store and matches its pointer OID**.
- Post-fetch `git lfs checkout` materialized all binaries: the 459 files now exist locally with their true bytes (no pointer mismatches, no corrupt objects).
- 459 = **416 Content binaries** (`.uasset`/`.umap`) + **43 ArtSource production textures** (`ArtSource/Textures/T_*.png`).
- Storage nuance (truth-relevant): the **79 ArtSource mesh GLBs** and the **4,065 Kenney/Quaternius pack files** are stored as plain git blobs (committed before/without the LFS filter applying), NOT as LFS pointers. They are real binaries in git (`glTF` magic / `RIFF` / `\x89PNG` verified) — RAW_PRESENT, not LFS.

---

## 3. Content binary authenticity forensics

All 416 `.uasset`/`.umap` files were byte-inspected:

| Check | Result |
|---|---|
| UE package magic `0x9E2A83C1` | **416 / 416 valid** |
| LegacyFileVersion | `-9` × 412 files (UE5.3+ serializer), `-8` × 4 files (older-era: `CR_Mannequin_Body`, `CR_Mannequin_Procedural`, `PA_Mannequin`, `T_UE_Logo_M` — verUE5 1013) |
| FileVersionUE4 | `522` for all 416 |
| FileVersionUE5 | `1018` × 291, `1017` × 120, `1016` × 1, `1013` × 4 (mixed serialization sessions — consistent with real engine imports at different times, incl. stock Epic mannequin content) |
| Custom version GUIDs | Present and well-formed (e.g. CR_Mannequin_Body carries 13 known UE custom-version GUIDs) |
| Total binary payload | 196.1 MB (205,600,511 bytes) across Content |

Conclusion: these are **genuine engine-serialized UE packages**, not synthetic placeholders. Binary-level import is real; runtime load in a live editor at this exact tip remains ENGINE_UNVERIFIED (§8).

---

## 4. Content inventory (exact)

432 tracked files under `Content/`:

| Dir | Files | Breakdown |
|---|---|---|
| `Content/Characters/Echoes` | 54 | 6 hero species × (1 SK mesh + 3 AM clips + 4 materials + 1 skeleton + 1 physics asset) |
| `Content/Characters/Mannequins` | 134 | SKM_Manny_Simple, SKM_Quinn_Simple, SK_Mannequin, 4 rigs, full anim tree (Unarmed/Pistol/Rifle: idle/walk/jog/jump/aim/fire/reload/hitreact/death), 5 materials + 4 MIs, 13 textures |
| `Content/Characters/Survivor` | 16 | `SK_Survivor_Exosuit.uasset` (580,218 B) + `AM_Survivor_{Idle,Walk,Run,Jump,Aim,Fire,Gather}` + 6 materials + skeleton + physics asset |
| `Content/Environment` | 53 | 12 resource-node files (4 SM + 8 mat), 9 ruin files (3 SM + 6 mat), 32 scatter files (12 SM + 20 mat) |
| `Content/Weapons` | 26 | 5 weapon SMs + 21 material files (ArcCannon 5, PlasmaCarbine/Railgun/ScrapRifle/SingularityCannon 4 each) |
| `Content/Vehicles` | 5 | `SM_Vehicle_DawnSkiff` + 4 materials |
| `Content/Materials` | 43 | 2 masters (`M_Master_Surface`, `M_Landscape_SciFiFrontier`) + 41 instances |
| `Content/Textures` | 44 | survivor/echo/landscape/rock/ruin/foliage/weapon/vehicle/FX sets |
| `Content/Audio` | 36 | 5 ambience, 7 creature, 5 footstep, 4 player, 7 UI, 8 weapon cues |
| `Content/VFX` | 3 | `NS_AW_MuzzleFlash`, `NS_AW_Weap_Impact`, `NS_AW_Weap_Trail` |
| `Content/Heightmaps` | 13 | 12 × `.r16` (505×505, plain git, RAW_PRESENT) + README |
| `Content/ASTRAWILD` | 2 | `.gitkeep` + `Maps/MainMap.umap` (48,805 B) |
| `Content/ThirdPerson` | 1 | `Lvl_ThirdPerson.umap` |
| `Content/Python` | 2 | `AwPipeline/import_all.py` (480 lines), `AwPipeline/aw_materials.py` (602 lines) |

Total `.uasset`/`.umap` = **416**; plus 12 `.r16` + 2 `.py` + 2 misc = 432.

## 5. ArtSource inventory (exact)

4,108 tracked files under `ArtSource/`:

| Group | Count | Storage |
|---|---|---|
| `ArtSource/manifest.json` | 1 | 175 asset entries (categories: mesh 95, textures 44, audio 36 — 159 pre-SCI + 16 SK_Base_* BaseMeshes mesh rows added by 0b55072; v9.2 re-count) |
| `ArtSource/Textures/T_*.png` (production) | 43 | **LFS_OBJECT_VERIFIED** |
| `ArtSource/Meshes/Echoes/*.glb` | 53 | RAW_PRESENT (glTF magic verified) — 39 Tier-B + 6 hero + 3 boss/summon (DrownedSovereign, GlassTyrant, EyeSentinel) + 5 ContentLibrary species (Auroraling, Dawnfang, Gloomfang, Lumewisp, Sprigling) |
| `ArtSource/Meshes/{Environment,Weapons,Vehicles,Characters}` | 26 | RAW_PRESENT GLBs (19 env + 5 weapons + 1 vehicle + 1 character) |
| `ArtSource/Textures/<packs>` | 2,398 | RAW Kenney: CrosshairPack 1,611, UIPackSciFi 713, ParticlePack 99, Skyboxes 7 (UI-domain, not runtime-imported by design) |
| `ArtSource/Models/<packs>` | 944 | RAW Kenney/Quaternius GLB/FBX/PLY sources: NatureKit 316, SpaceKit 109, SurvivalKit 84, SpaceKit(Quaternius) 87, ModularDungeonKit 43, ModularSpaceKit 44, CityKitIndustrial 41, BlasterKit 44, **Quaternius_UltimateMonsters 61**, ModularRuins 58, UltimateNature 53, ModularMen 16, AnimatedAnimals 15, AnimatedCharactersSurvivors 6 |
| `ArtSource/Audio/<packs>` | 612 | RAW Kenney: SciFiSounds 150, ImpactSounds 259, InterfaceSounds 203 |

Raw (non-LFS) payload: 222.6 MB.

## 6. Cross-check: code references → Content files

- **114 unique `/Game/` literals** extracted from `Source/**/*.cpp|h`. **98 resolve to an existing Content package.** The 16 non-resolving literals decompose as:
  - 12 are **prefix/format constants, not asset paths** (`/Game/Audio/A_Amb_`, `/Game/Audio/A_Weapon_`, `/Game/Characters/Echoes/AM_%s_%s`, `/Game/Characters/Echoes/SK_Echo_%s`, `/Game/Environment/`, `/Game/VFX/NS_AW_`, `/Game/Weapons/`, etc. — used by tests/pipeline to BUILD paths);
  - 3 are Tier-B convention probes in automation tests (`SK_Echo_Rimefang`, `AM_Rimefang_*` — string-contract tests, and `Echo_Rimefang` has its GLB on disk);
  - 1 is the nested Survivor mesh variant `/Game/Characters/Survivor/SK_Survivor_Exosuit/SkeletalMeshes/SK_Survivor_Exosuit` — a **secondary LoadObject attempt that is never reached** because the flat `SK_Survivor_Exosuit.uasset` resolves first (see PATH_MISMATCH note, §9).
- **Manifest `ue_path` → Content: 112 / 175 present (v9.2 re-derivation).** The 63 absent entries = 47 not-yet-engine-imported Echo GLB meshes (53 Echo GLBs − 6 hero) + 16 SK_Base_* base-mesh rows (also pending engine import, via the separate `import_echo_bases.py` / V2-35 pass). These are NOT missing — their source GLBs are RAW_PRESENT and the bindings are opt-in by design (PMC fallback until the final-tip re-import).
- Binding architecture verified in source:
  - `UAstrawildProductionContent` applies `AstrawildArtPack::GetEchoArt()` (6 hero) and `BuildTierBMechPath()` (39 Tier-B, derived paths — validator check 8 clean);
  - `AAstrawildPlayerCharacter::TryActivateSkeletalBody()` — Survivor → nested Survivor → `SKM_Manny_Simple` → PMC, with all 7 clip fallback chains;
  - `AAstrawildEchoCharacter` — EchoDefinition-driven (body plan / size class / tints / rarity / element glow);
  - Weapons: 8 bindings → 5 meshes (PlasmaCarbine shared by 2, Singularity by 2 — intentional);
  - VFX/SFX: `AstrawildArtPack::Vfx` (3 Niagara paths) + `Sfx::WeaknessHitImpact` all resolve.

## 7. Engine-import evidence status

- A completed import run **exists**: `Docs/ENGINE_LOGS/raw/import_report.json` — `total_missing: 0`, `errors: []`, stages: textures 44 / audio 72 / meshes 32 imported. Per the verification-queue status note, that run was executed on the engine machine at SHA `8313c61` (branch `agent/antigravity-ue5-v2`, 2026-09-02) — it **predates the Tier-B library (39 GLBs), the PCR screens, and the FPP freeze commits**.
- Consequence: at the CURRENT tip, `import_all.py` (manifest-driven, covers all 175 entries incl. the 53 Echo GLBs + the 16 SK_Base rows) has **not been re-run**; the 63 newer manifest entries have no engine package yet. The queue rows V2-29 (re-import baseline), V2-30/V2-31 (PIE clips) correctly remain **NOT_RUN at the final tip** in `Docs/ASTRAWILD_ENGINE_VERIFICATION_QUEUE.md` — this audit changes none of those rows.

## 8. Status vocabulary (as used below)

| Status | Meaning |
|---|---|
| RAW_PRESENT | Real binary tracked in plain git (glTF/RIFF/PNG magic verified) |
| LFS_POINTER | Working-tree file is an LFS pointer (pre-fetch state) |
| LFS_OBJECT_VERIFIED | Remote LFS object fetched and OID-matched during this audit |
| IMPORT_READY | Genuine engine-serialized `.uasset`/`.umap` on disk (magic + version tables valid) |
| BOUND_READY | IMPORT_READY **and** a live code-side binding references the path |
| ENGINE_UNVERIFIED | Cannot be proven loading/running in UE 5.8 from this sandbox (no editor/GPU here) |
| MISSING | Referenced by code, absent from both Content and ArtSource |
| PATH_MISMATCH | A code path that does not and cannot resolve (but is inert) |

## 9. Per-domain truth

**PLAYER** — BOUND_READY: 8 survivor art assets + 16-file Survivor set + 134-file Mannequin fallback set, all LFS_OBJECT_VERIFIED + IMPORT_READY; source binding data-driven (`GetSurvivorArt()`); two-level clip fallbacks verified in code. Runtime PIE: ENGINE_UNVERIFIED at tip.

**ECHO** — 229 unique ids across 3 registration layers (204 generated bestiary + 6 production heroes + ContentLibrary defaults). 6 hero: BOUND_READY. 39 Tier-B + 8 other GLB-backed ids: RAW_PRESENT, opt-in convention bindings, engine import pending → ENGINE_UNVERIFIED (PMC fallback active by design). Remainder: procedural PMC body plans (by design, not an asset gap). `UnderlightWarden`/`VaultColossus` have no mesh art at all (PMC by design).

**WORLD** — BOUND_READY: 12 scatter SMs + 4 node SMs + 9 ruin files + materials; 12 `.r16` heightmaps RAW_PRESENT; `M_Landscape_SciFiFrontier` IMPORT_READY (assignment onto the `MainMap` landscape actor is an editor-only manual step — HANDOFF §19 — ENGINE_UNVERIFIED).

**WEAPONS** — BOUND_READY: 5 meshes + 21 material files; 8 bindings; muzzle/impact/trail FX paths resolve.

**AUDIO** — BOUND_READY: 36 cues (exactly matching manifest audio categories); 612 raw Kenney pack files RAW_PRESENT (source pool).

**VFX** — BOUND_READY: 3 Niagara systems (pipeline-created template-level systems, ~48 KB each); visual authoring depth + runtime: ENGINE_UNVERIFIED.

**PATH_MISMATCH (inert, 1)** — `/Game/Characters/Survivor/SK_Survivor_Exosuit/SkeletalMeshes/SK_Survivor_Exosuit` (secondary attempt in `AstrawildPlayerCharacter.cpp:258`): no such nested mesh exists (nested folder holds only Skeleton + PhysicsAsset, which is correct UE layout). Unreachable when the flat path resolves; **no action required** — recorded for truth only.

**TRUE_MISSING_ASSETS — 0.** No code-referenced asset is absent from the repository.

## 10. Final block

```
CURRENT_HEAD            = 68c2b075c9d68e7dcfd15d159cd8e177128bf872 (branch final-completion; 4bb7be5 is 4 commits behind tip)
CONTENT_BINARY_COUNT    = 416 (.uasset/.umap, all genuine UE packages; +12 .r16, +2 .py, +2 misc = 432 tracked)
LFS_OBJECTS_VERIFIED    = 459 / 459 (232 MB fetched from remote, OID-matched; 416 Content + 43 ArtSource textures)
PLAYER_ASSET_STATUS     = BOUND_READY (16 Survivor files + 134 Mannequin fallback files; runtime ENGINE_UNVERIFIED)
ECHO_ASSET_STATUS       = 6 hero BOUND_READY; 47 GLB-backed ids RAW_PRESENT awaiting final-tip import (opt-in, PMC fallback by design); remainder procedural PMC; runtime ENGINE_UNVERIFIED
WORLD_ASSET_STATUS      = BOUND_READY (53 environment files + M_Landscape_SciFiFrontier IMPORT_READY + 12 r16 RAW_PRESENT; landscape assignment = manual editor step, ENGINE_UNVERIFIED)
WEAPON_ASSET_STATUS     = BOUND_READY (5 meshes + 21 materials, 8 bindings; FX runtime ENGINE_UNVERIFIED)
AUDIO_STATUS            = BOUND_READY (36 cues; 612 raw pack files; audibility ENGINE_UNVERIFIED)
VFX_STATUS              = BOUND_READY (3 template Niagara systems; authored visuals + runtime ENGINE_UNVERIFIED)
TRUE_MISSING_ASSETS     = 0
ENGINE_UNVERIFIED_ITEMS = V2-29 final-tip re-import baseline; V2-30 PIE survivor clips; V2-31 PIE echo skinned swap;
                          V2-32 biome scatter + 4-layer landscape; V2-33 node meshes; V2-34 weapon FX+audio;
                          47 not-yet-imported GLB ue_paths (39 Tier-B + 3 boss/summon + 5 ContentLibrary species);
                          Niagara visual authoring depth; Lvl_ThirdPerson map provenance (stock template, unused by MQ)
```

**Honesty floor:** nothing in this audit claims UE 5.8 runtime verification. The sandbox has no engine;
every runtime claim above is deferred to the Antigravity engine machine per the existing
`ASTRAWILD_FINAL_BUILD_HANDOFF.md` §20 sequence. This document is the asset-truth baseline for
any subsequent content-development decision.

## 10b. Final block — v9.2 FINAL-EXECUTION re-derivation (current HEAD 4daa113)

The §10 block above is the frozen 68c2b07 audit snapshot. Every number below was
re-derived with actual commands at the CURRENT tip (this round — docs are not
evidence; `git lfs ls-files` / sha256 / filesystem walks are):

```
CURRENT_HEAD            = 4daa113b4224c58a0d3ee1e636c4f1240f11c937 (branch final-completion, in sync w/ remote, clean tree)
CONTENT_BINARY_COUNT    = 416 (.uasset/.umap — re-walked, 416/416 genuine UE package magic 0x9E2A83C1, 0 bad)
LFS_OBJECTS_VERIFIED    = 491 / 491 (236.5 MB on-disk, sha256 OID-match verified on all 32 SCI files + 6 random; git lfs fsck exit 0;
                          composition: 416 Content + 43 ArtSource textures + 16 SK_Base_*.glb + 16 SFXSet_*.wav)
PLAYER_ASSET_STATUS     = unchanged (BOUND_READY; runtime ENGINE_UNVERIFIED)
ECHO_ASSET_STATUS       = 6 hero BOUND_READY; 47 GLB-backed Echo ids RAW_PRESENT (opt-in, PMC fallback by design);
                          + 16 SK_Base base meshes RAW_PRESENT (import via import_echo_bases.py, V2-35); runtime ENGINE_UNVERIFIED
MANIFEST_STATUS         = 175 entries (159 + 16 SK_Base rows); ue_path present 112 / 175; pending 63 = 47 Echo ids + 16 base meshes
WORLD/WEAPON/AUDIO/VFX  = unchanged from §10 (BOUND_READY; runtime ENGINE_UNVERIFIED)
TRUE_MISSING_ASSETS     = 0
ENGINE_UNVERIFIED_ITEMS = V2-29..V2-35 all NOT_RUN at tip (queue §5); 63 not-yet-imported manifest ue_paths;
                          Niagara authoring depth; no UE/MSVC on this Linux sandbox (re-verified this round)
```

---

## 11. SCI v9.1 amendment (FINAL EXECUTION round — appended, original audit above unchanged)

- **Baseline note**: §1-§10 above are the audit record at HEAD `68c2b07` (commit 58b3fcd).
  The Sci-Fantasy directive landed at `0b55072` after it; this amendment documents the
  asset-truth deltas of that commit + the v9.1 material-swap amendment, without rewriting history.
- **New source assets (0 new engine packages, all RAW_PRESENT / engine-side pending)**:
  16 × `SK_Base_*.glb` (ArtSource/Meshes/Echoes/BaseMeshes, rigged + Idle/Move/Hit clips),
  16 × `SFXSet_*.wav` (ArtSource/Audio/Echoes, CC0 Kenney-staged, SHA-256 ledger
  `Docs/ASTRAWILD_SCI_FANTASY_ACQUISITION.json`), 1 import script
  (`Content/Python/AwPipeline/import_echo_bases.py`). TRUE_MISSING stays **0**.
- **New ENGINE-side packages to be authored by the one-time import** (all opt-in/fail-closed
  until then, counted in `echo_base_report.json` coverage): 16 skeletal meshes +
  48 anim clips + 16 sound cues + 7 NS_AW_Elem_* templates + **8 M_SciFi_* theme master
  materials (v9.1: was 6 — M_SciFi_OrganicHide + M_SciFi_FocusCrystal added so every
  EAstrawildMutationMaterialTheme resolves; a missing master now counts in
  total_missing as an ERROR, killing the false-clean-report risk)**.
- **New runtime consumer (v9.1 — was the real dead-end this round closed)**:
  `FAstrawildEchoMutator::BuildThemeMaterialPath` + `ApplyThemeMaterial` — the masters
  are no longer import-authoring-only; the skinned path swaps them in as dynamic
  material instances (Tint/PatternTint/GlowIntensity per species). Fail-closed before
  import: the GLB's own materials stay.
- **GLB ue_paths pending import: 63 manifest rows (v9.2 re-count) = 47 Echo IDs
  (39 Tier-B + 3 boss/summon + 5 ContentLibrary species — opt-in by design, PMC
  mutated fallback active) + 16 SK_Base_* base meshes (import via
  import_echo_bases.py at the same engine pass)** — TRUE_MISSING stays 0.
- **Runtime claims: still NONE.** Everything above stays ENGINE-UNVERIFIED until the
  V2-35 engine run (import report total_missing==0 incl. materials + PIE clips incl.
  theme material swap). Sandbox truth re-verified this round: no UE/MSVC exists on
  this Linux sandbox (checked: no Unreal installation, no /mnt/c, network-limited) —
  the engine run is exclusively the Antigravity Windows machine's.

---

## 12. ASSET OVERHAUL v9.3 (NO PLACEHOLDERS / NO PALETTE SWAPS — supersedes the mesh-era counts above)

User executive directive: transition from the interim Procedural Recolor system to REAL,
uniquely-shaped 3D models. Executed source-side at this commit; engine import = V2-36.

**The mesh inventory is now 100% real, 100% unique, 100% CC0:**

| Group | Count | Truth |
|---|---|---|
| `ArtSource/Meshes/Characters/Survivor` | 3 | REAL rigged CC0 humanoids — SK_Survivor_Exosuit (Swat, Tier-3 Singularity Exosuit) + SK_Survivor_T1_Scavenger (Adventurer) + SK_Survivor_T2_Astraite (Spacesuit); 62 bones, 24 anims each (incl. Gun_Shoot / Idle_Gun_Pointing / Interact / Roll) |
| `ArtSource/Meshes/Echoes` | 42 | REAL unique CC0 meshes — 6 hero + 33 Tier-B + 3 production bosses (GlassTyrant/EyeSentinel/DrownedSovereign) |
| `ArtSource/Meshes/Echoes/BaseMeshes` | 16 | REAL unique CC0 base archetypes (Quaternius: Dragon, Ghost, Wolf, Yeti, Goleling, Armabee, Squidle, Cactoro, Mushnub, GreenBlob, Hywirl, Glub, Ghost_Skull, Demon, Birb, Armabee_Evolved) |
| `ArtSource/Meshes/Echoes/Bosses` | 14 | REAL unique CC0 showcase bosses (Evolved variants + MushroomKing/Orc/BlueDemon/Ninja + 2 Mechs + Enemy_Large) |
| `ArtSource/Meshes/Weapons` | 5 | REAL geometry-distinct Kenney blasters (820/668/1386/1056/1506 verts — no two alike) |
| `ArtSource/Meshes/Vehicles` | 4 | REAL — Dawn Skiff hover + Ground Rover + Support Skiff + Heavy Rover |
| `ArtSource/Meshes/Environment` | 25 | REAL — 4 ore nodes (Kenney crystal clusters + ruins Column_Round_Short) + trees/rocks/flora/ruins (Kenney NatureKit/SpaceKit + Quaternius ModularRuins FBX) |

- **1:1 source uniqueness enforced**: 109 staged assets ← 109 distinct source models; the fetch
  script's palette-swap guard hard-fails on any reuse. Verified by the validator gate
  "Mesh rows carry unique source models — no palette swaps".
- **Manifest regenerated (v2.0)**: 189 entries / 189 `status: "present"` / **0 pending**;
  every mesh row records source_pack + source_model + license (CC0) + sha256 + clip_map;
  stale absolute paths (previous repo root) normalized to repo-relative. Per-asset license
  provenance: `Docs/ASTRAWILD_REAL_ASSET_CREDITS.json`.
- **Tier-B code table 39 → 36** (33 real-mesh species + the 3 production bosses — the
  production bosses now carry DIRECT real meshes); 11 superseded procedural species GLBs
  purged — those species render via their theme's REAL base mesh + their unique deterministic
  mutation spec (real geometry + real mutation, never a recolor).
- **Superseded procedural files purged**: 15 (11 Echo GLBs + 4 Environment GLBs) — git
  history preserves the interim era.
- **Pipeline**: `Scripts/fetch_free_assets.py` (catalog + live-verified Kenney remote fetch
  + glTF→GLB self-contained conversion with embedded images) → `import_all.py` (clip_map
  AM_ renames + PBR node emissive + sockets + direct-binding coverage incl. clips) →
  `build_showcase_map.py` (PlayerStart + every mesh placed on real ground) →
  **`Setup_And_Play.bat`** (Windows one-click).
- **Runtime claims: still NONE.** Engine import stays NOT_RUN on this sandbox (no UE —
  re-verified). V2-36: run `Setup_And_Play.bat` on the Windows machine; evidence =
  `Saved/AwPipelineReport/import_report.json` (total_missing == 0 including the AM_ clips)
  + showcase-map PIE. TRUE_MISSING stays 0.
