# ASTRAWILD — CONTENT INTEGRATION MATRIX (DP-2)

**Document Version**: 1.1 (DP-10 re-verification — all 14 category rows re-checked against the post-depth-pass source)
**Custodian**: GLM 5.3 — Lead Programmer / Game Architect (source-side)
**Status**: SOURCE-VERIFIED readiness matrix. **Nothing in this document is `BOUND`, and no engine-verified or game-ready claim is made — those statuses are reserved for the Antigravity engine run and are not claimed anywhere below.** Statuses separate three trust levels: **SOURCE-VERIFIED** (acquired/authored + validated on disk in this sandbox), **STATIC-VALIDATED** (native content that is code-registered + machine-checked by the 109-test gate and the 15 census gates — but has never been rendered in an engine), and **ENGINE-UNVERIFIED** (applies to every row: import, binding, cook and PIE belong exclusively to the Antigravity one-time integration run per `ASTRAWILD_FINAL_BUILD_HANDOFF.md` §20/§20b/§20c).
**Origin**: user directive (2026-09-05) — *"For every acquired asset ask: WHERE IS THIS USED? If the answer is nowhere, do not prioritize importing it."* Registry §I row DP-2; re-verified at DP-10.
**Baseline**: `final-completion` — written at `981250d` (post DP-1 creature strategy); re-verified at the DP-10 final gate (tip `018a95a` + the DP-10 docs commit) after depth passes DP-3..DP-9 landed.

Companion docs (detail lives there, not here): `ASTRAWILD_FINAL_BUILD_HANDOFF.md` §20b (import destinations + engine checks) · `ASTRAWILD_CREATURE_VISUAL_STRATEGY.md` (DP-1 — Echo/boss tiers) · `ASTRAWILD_MASTER_TASK_REGISTRY.md` §H (acquisition ledger AA-1..AA-9) · `ASSETS_CREDITS.md` + `ASSET_MANIFEST.json` (per-file provenance + SHA256) · `Docs/ASSET_ACQUISITION_REPORT.md` (authoritative pack numbers).

## 0. Status vocabulary (only these)

| Status | Meaning |
| :--- | :--- |
| `MISSING` | No source AND no code-registered substitute — a real content gap. |
| `SOURCE_READY` | Acquired/authored + validated on disk, but NOT in a UE5-importable format (e.g. OGG provenance originals). |
| `IMPORT_READY` | On disk in a UE5 import format (WAV/GLB/PNG/FBX/TTF) with verified license; NOT imported, NOT bound. |
| `BOUND` | Imported into `/Game/` AND referenced by runtime code. **Nothing acquired is BOUND — binding is engine-side only.** |
| `STATIC_VALIDATED` | Native procedural/authored content: code-registered + validator-verified; visuals never rendered in an engine. |
| `ENGINE_UNVERIFIED` | Qualifier over every status above: no import/cook/PIE evidence exists yet (Antigravity-owned). |

**Acquisition baseline (authoritative — `ASSET_ACQUISITION_REPORT.md` §2/§4)**: 15 Kenney CC0 packs · **3,678 accepted / 3,360 IMPORT_READY / 75.8 MB**. IMPORT_READY composition on disk: **301 WAV + 657 GLB + 2,396 PNG + 4 FBX + 2 TTF**. Native project-authored ArtSource (flat folders, auto-imported by the unchanged flat-destination contract): **36 WAV + 44 PNG + 40 GLB = 120 files** (the 112-asset contract of DP-2 plus the 8 DP-1 Tier-A echo meshes; `ArtSource/manifest.json` records all 120 — disk and manifest cross-checked equal at DP-10).

> **Data correction (recorded, not silently fixed)**: HANDOFF §20b step 5 lists "602 WAV" — 602 is the *accepted audio record* count (301 OGG provenance originals + 301 WAV conversions, REPORT §4). Only the **301 WAVs are importable** (UE5 does not import OGG; `Ogg/` folders are provenance-only and must NOT be imported, §20b step 2). Total importable WAV library after integration = 301 Kenney + 36 native = **337**.

## 0b. Evidence & method

- **Pack numbers** re-verified on disk at `981250d`: 301 WAV + 657 GLB + 2,396 PNG + 4 FBX + 2 TTF = **3,360 exactly** (matches the manifest/import-ready census; per-pack splits in §2).
- **Native numbers** re-verified on disk at the DP-10 gate: 36 flat WAV + 44 flat PNG + 40 GLB (14 echo meshes + 26 props) = **120** (the flat auto-import contract; `manifest.json` = 120 records, zero drift); `Content/` carries 414 `.uasset` + 2 `.umap` committed via LFS.
- **Census facts** (229 species / 12 zones / 3 dungeons / 4 bosses / 11 NPCs / 2 villages / 8 weapons / 26 buildings / 78 items / 58 recipes / 17 techs / 17 quests / 11 loot tables / 10 resource nodes / 8 work sites / 16 world events / 17 POIs / 11 dialogue trees / 3 robots / **109 tests**) are the validator-enforced values — `validate_final_run.py` census gates re-ran PASS at the DP-10 tip (`018a95a`).
- **Placeholder evidence**: boss Cone (`AstrawildEchoBossCharacter.cpp:40-56`, opt-in skeletal path since DP-1b), NPC Cylinder+Sphere (`AstrawildNPCCharacter.cpp:48-61`), 3 `NS_AW_*` Niagara systems (`Content/VFX/`), text-glyph crosshair (pure-C++ `HudWidget`).
- Nothing was imported, bound, replaced or deleted while writing this matrix — docs-only, per the DP-2 scope.

---

## 1. Category readiness matrix (14 categories — "WHERE IS THIS USED?")

| CATEGORY | CURRENT | MISSING | SOURCE | LICENSE | IMPORT PATH | BINDING | FALLBACK | STATUS |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **PLAYER** | `SK_Survivor_Exosuit` GLB + 7 anim clips (Aim/Fire/Gather/Idle/Jump/Run/Walk) + `T_Survivor_*` texture set; `.uasset` committed (LFS) at `/Game/Characters/Survivor`; 28-action input map, 5 attributes, 7 milestone skills + **DP-4 skill loadout live** (3-slot bound actives via the ESC pause-menu SKILL LOADOUT section; empty loadout = legacy smart-cast; save v5 additive) | none source-side (player body art exists; ACS below is an NPC-side candidate, not a player replacement) | ArtSourceGen (self-authored) | project-authored (no third-party license) | flat contract → `/Game/Characters/Survivor` (part of the 120-asset import) | `ConstructorHelpers` hard refs in `PlayerCharacter` (code-registered) | capsule + `AssetFallback` chain (zero-asset boot guaranteed) | **STATIC_VALIDATED** |
| **ECHOS** | 229 species (census-locked): **14 bespoke Tier-A meshes** (`SK_Echo_*` GLB in `ArtSource/Meshes/Echoes/` — 6 pre-existing heroes + 8 DP-1 deliveries incl. the 4 bosses, each with Idle/Move/Hit clips + manifest record) + 215 procedural PMC bodies (8 body plans × 5 size classes × tint pairs + rarity ring + element point-light); **locomotion signature identity since DP-3** (Water/Flying species carry a 7th signature ability pick; 15-pair party element resonance; Aquatic mounts swim); weak-point windows on Large/Huge wild game (DP-5) | Tier-B archetype rig library (DP-1 P1 art backlog — ~55 species on procedural bodies with strengthened material identity until it lands) | ArtSourceGen (self); **no external 3D-creature source usable** (Kenney creature packs are 2D; Quaternius deferred/QAL — strategy §2) | project-authored | `ArtSource/Meshes/Echoes/` → `/Game/Characters/Echoes` (existing flat contract) | `AstrawildArtPack::GetEchoArt()` soft refs + the §20c binding-patch sequence (assets-first, binding-second) + automatic PMC fallback | procedural PMC body (spawn never breaks) | **STATIC_VALIDATED** (procedural path) + 14 Tier-A meshes **IMPORT_READY**; Tier-B library **MISSING (P1, graceful degradation)** |
| **WEAPONS** | 8 weapon profiles (Arc/Plasma/Rail/Scrap/Singularity + beam/arc/charge fire modes) on 5 procedural meshes + `T_Weapon_*` textures; `.uasset` committed at `/Game/Weapons/Meshes` | none source-side (visuals engine-unverified) | ArtSourceGen + Kenney Blaster Kit (40 GLB + colormap) | self + CC0 (LICENSE_VERIFIED) | native done; Blaster → `/Game/Weapons/Meshes/Kenney/` (§20b step 2) | `WeaponDefinition` refs today; Blaster = **CANDIDATE_REPLACEMENT** — compare-first per archetype in PIE, never auto-replace (ticket 06) | the 5 procedural meshes stay on disk AND bound regardless | **STATIC_VALIDATED** + Blaster Kit **IMPORT_READY** |
| **TERRAIN** | procedural heightmap system: 12 zone `.r16` heightmaps (`Content/Heightmaps`) + `T_Landscape_{Grass,Sand,Soil,Granite}_D/N` + `T_Landscape_ORM` (flat contract, imported) | none | ArtSourceGen (self) | project-authored | `/Game/Textures` + `/Game/Heightmaps` (flat contract) | `ZoneSubsystem` world-gen reads `.r16`; landscape material applied by code | default flat landscape material | **STATIC_VALIDATED** |
| **BIOMES** | 12 zones (per-zone tint/light/height/ridge/threat) + `BiomeDressingActor` (6 canopy styles, per-zone budgets) + 19 native env GLB (trees/rocks/foliage/ruins/4 resource nodes); **per-zone hazard identity + ≥1 anchored event per zone since DP-7** | per-zone flora variety (cosmetic depth, P2 — not a gameplay gap) | + Kenney Nature Kit (314 GLB) | self + CC0 | → `/Game/Environment/Kenney/NatureKit/` (§20b) | BiomeDressing/POI candidate pools — decided at engine AFTER tone check (ticket 05) | native 19 GLB dressing (already in the 120-asset contract) | **STATIC_VALIDATED** + Nature Kit **IMPORT_READY** |
| **VILLAGES** | 2 villages (Dawnstead, Driftwood Landing): 26 building types code-placed (doors/crates/levels 1-3) + NPC schedules/services; buildings procedural | village prop dressing (campfire/tent/crate variety — cosmetic) | + Kenney Survival Kit (80 GLB + colormap) | self + CC0 | → `/Game/Environment/Kenney/SurvivalKit/` | dressing pools for POIs + village outskirts (engine-side) | existing procedural buildings/props | **STATIC_VALIDATED** + Survival Kit **IMPORT_READY** |
| **DUNGEONS** | 3 dungeons (Hollow Underlight, Sunken Vault, Eye of the Maelstrom — 5 rooms, 400 m altitude) generated procedurally; differ by boss/creature-pool/loot/RP data; **per-dungeon themed rooms since DP-9** (tinted shells + side walls + deterministic ArtPack dressing + room-level hazards + resonance-pillar puzzle rooms — existing ArtPack mesh paths only, no new /Game/ refs) | authored room TILES (modular dungeon kit) — the visual upgrade half that pairs with the DP-9 mechanics pass | + Modular Space Kit (40 GLB), Modular Dungeon Kit (39 GLB), Space Kit (107 GLB) | self + CC0 | → `/Game/Environment/Kenney/{ModularSpaceKit, ModularDungeonKit, SpaceKit}/` | `DungeonGenerator` room-dressing candidates (engine-side; snap/layout decisions there) | current themed shell generator (DP-9) | **STATIC_VALIDATED** + 186 tile/dressing GLB **IMPORT_READY** |
| **VFX** | 3 committed Niagara systems (`NS_AW_MuzzleFlash`, `NS_AW_Weap_Impact`, `NS_AW_Weap_Trail`) + procedural Beam/ScannerPulse/Capture VFX actors | combat VFX sprite art (P0 gap — acquired, unbound) | + Kenney Particle Pack (96 transparent PNGs) | self + CC0 | → `/Game/FX/Kenney/ParticlePack/` | Niagara sprite renderers; **fitness check FIRST** (palette PNGs = indexed color + tRNS alpha — §20b step 2; if alpha is unusable, record and stop that binding) | existing `NS_AW_*` systems + procedural VFX actors (always-on) | **STATIC_VALIDATED** (3 NS + actors) + sprites **IMPORT_READY** |
| **UI** | 7 pure-C++ UMG classes (HUD, Inventory, Research, Shop, Dialogue, Pause, Crafting) — layout + logic live, **zero art**; text-glyph crosshair | panel/button/icon art + offline font + real reticle (P0 gap — acquired, unbound) | + UI Pack Sci-Fi (690 PNG + 2 TTF); Crosshair Pack (HUD subset) | self + CC0 | → `/Game/UI/Kenney/SciFi/` + `/Game/UI/Fonts/` + `/Game/UI/Kenney/Crosshairs/` | UMG brush overrides at engine; HudWidget hip/aim states already coded | current solid-color/text UMG (always-on) | widgets **STATIC_VALIDATED**; art **MISSING → IMPORT_READY** |
| **AUDIO** | 36 native WAV (UI/combat/ambience/footsteps + 7 creature calls) imported + wired | impact/interface/sci-fi feedback palette breadth (P0/P1 gap — acquired, unbound) | + Kenney 301 WAV (Impact 128 · Interface 100 · Sci-Fi 73); 301 OGG originals = SOURCE_READY provenance only | self + CC0 | → `/Game/Audio/Kenney/<Pack>/` (Wav/ only — never `Ogg/`) | SoundWave extension/swap at engine (AudioComponent pools exist) | current 36 wired sounds | **STATIC_VALIDATED** (36 native) + 301 WAV **IMPORT_READY** |
| **WATER** | `WaterPlaneActor` in the 3 sea zones (Azure Shallows, Tidebreaker Isles, Pearlsea Reef) + animated water material; zone tint system drives atmosphere | sky art (sky dome textures — P1 gap acquired, unbound; water itself exists) | + Kenney Skyboxes (5 equirect PNG: day/morning/night/alien/space) | self + CC0 | → `/Game/Environment/Kenney/Skyboxes/` (long-lat import) | sky dome material per zone mood (engine-side; 5 skies ≠ 12 zones — shared moods + zone tint) | procedural sky/gradient + zone tint (current) | **STATIC_VALIDATED** + skyboxes **IMPORT_READY** |
| **SKIFF** | `SM_Vehicle_DawnSkiff` GLB + `T_Vehicle_*` glow textures; `.uasset` committed; board/boost/descend + Stratos Coil 160 m ceiling gate (MQ-14) | none (orientation check is the known cosmetic risk — HANDOFF §18) | ArtSourceGen (self) | project-authored | flat contract → `/Game/Vehicles` (done) | `SkiffActor` HullMesh hard ref | capsule hull | **STATIC_VALIDATED** |
| **NPCS** | 11 NPCs (affinity tiers 0-100, 4 professions, schedules, shops, 11 dialogue trees) on **Cylinder + Sphere engine-shape placeholder bodies**; **affinity-gated dialogue evolution + regional knowledge lines since DP-8** (RequiredMinAffinity gates at 25/50/75 with fail-closed evaluation; evolved trees for Tam/Rowan/Nima/Sela; knowledge nodes for Tam/Rowan/Sela/Jori) | NPC body meshes (P1 gap — Kenney 2D rejected; candidate acquired, unbound) | + Kenney Animated Characters: Survivors (1 humanoid FBX + idle/run/jump clips) | self + CC0 | → `/Game/Characters/Kenney/Survivors/` (SkeletalMesh + 3 anims) | retarget compatibility vs `SK_Survivor_Exosuit` = **explicit §20b step-2 check** (rig mismatch is a valid finding, not a source defect) | cylinder+sphere placeholder (current) | systems **STATIC_VALIDATED**; ACS **IMPORT_READY**; bodies **ENGINE_UNVERIFIED** |
| **BOSSES** | 4 boss fights fully coded (Underlight Warden, Dawnfang, Glass Tyrant, Drowned Sovereign): 3 phases + enrage + summons + ×2 weak points + telegraphed AoE + arena hazards; **per-boss special sets since DP-5** (4 distinct tunings over the shared pipeline, default = legacy verbatim) | **boss meshes still render as the engine Cone placeholder until import** (visual half only — the meshes exist) | 4 bespoke ArtSourceGen meshes **delivered (V25-C1, IMPORT_READY)** in `ArtSource/Meshes/Echoes/` (DrownedSovereign/GlassTyrant/Dawnfang/EyeSentinel) | project-authored | `ArtSource/Meshes/Echoes/` → `/Game/Characters/Echoes` (same flat contract as the heroes) | **opt-in skeletal path already source-side (DP-1b)**: `BossBodyMesh` activates the skinned body and hides the cone the moment the mesh resolves — swap happens via the §20c verbatim binding-patch sequence (assets-first, binding-second); cone stays as fallback (same never-auto-replace rule as weapons) | Cone placeholder (current) | fight logic **STATIC_VALIDATED**; boss meshes **IMPORT_READY** (binding **ENGINE_UNVERIFIED**) |

---

### 1b. Rollup tally (the honest one-glance answer)

| Tally | Categories | Note |
| :--- | :--- | :--- |
| `BOUND` | **0 of 14** | by design — binding happens only in the Antigravity engine run |
| `STATIC_VALIDATED` (native systems, all 14) | PLAYER · ECHOS · WEAPONS · TERRAIN · BIOMES · VILLAGES · DUNGEONS · VFX · UI · AUDIO · WATER · SKIFF · NPCS · BOSSES | every gameplay system is code-registered + validator-verified; visuals never rendered in an engine |
| `STATIC_VALIDATED` **+** acquired `IMPORT_READY` layer | ECHOS (14 Tier-A GLB) · WEAPONS (Blaster) · BIOMES (Nature) · VILLAGES (Survival) · DUNGEONS (186 tiles) · VFX (96 sprites) · UI (690+2+reticle subset) · AUDIO (301 WAV) · WATER (5 skies) · NPCS (ACS candidate) | acquired packs sit beside working fallbacks — upgrades, not dependencies |
| `MISSING` (source-side backlog, no acquirable source) | **ECHOS Tier-B rig library (P1)** · NPCS bodies (P1, pending ACS retarget) · UI art until bound | Tier-A/boss meshes are NO LONGER missing (delivered IMPORT_READY, V25-C1/C2); the Tier-B library degrades gracefully — those ~55 species run on procedural bodies with strengthened material identity |
| Clean (nothing missing, nothing to acquire) | PLAYER · TERRAIN · SKIFF | self-authored, committed, code-referenced |

---

## 2. Per-pack integration table (15 packs — WHERE IS THIS USED)

Accepted counts are the authoritative REPORT §2 values (they include `License.txt`/metadata); import-ready counts are the files UE5 can actually import. All packs: license CC0 1.0, LICENSE_VERIFIED per pack page + in-archive `License.txt`. **No pack is force-imported: import happens per §20b only when a concrete game use exists below.**

| PACK | FILES | INTENDED ASTRAWILD USE | TARGET ZONES/SYSTEMS | IMPORT DESTINATION (§20b) | BINDING APPROACH | FALLBACK | INTEGRATION STATUS |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Impact Sounds | 257 acc (128 WAV + 128 OGG + lic) | impact/hit feedback: node harvest, melee/structure impacts, footstep surface variation (player + Echo locomotion) | combat + harvesting, all 12 zones | `/Game/Audio/Kenney/ImpactSounds/` | SoundWave extension of impact/footstep events (engine) | 36 native wired WAVs (incl. 5 footsteps) | **IMPORT_READY**; ~128 variants ≫ event count — unused variants are a palette, curate at binding (subset **NOT_INTEGRATION_SCOPE** until a consumer exists) |
| Interface Sounds | 201 acc (100 WAV + 100 OGG + lic) | UI feedback: clicks/confirms/errors/open-close/toggles for the 7 screens | all player-facing UI | `/Game/Audio/Kenney/InterfaceSounds/` | UMG event hooks (engine) | 12 native `A_UI_*` WAVs | **IMPORT_READY**; same palette note (subset **NOT_INTEGRATION_SCOPE**) |
| Sci-fi Sounds | 148 acc (73 WAV + 73 OGG + lic) | energy-weapon fire, doors, computers, force fields, explosions, skiff engine loops | combat + 3 dungeons + skiff | `/Game/Audio/Kenney/SciFiSounds/` | weapon fire-mode SFX + door/station ambience (engine) | 8 native weapon-fire WAVs + charge loop | **IMPORT_READY**; subset beyond the 8 fire modes + door/station events = palette (**NOT_INTEGRATION_SCOPE**) |
| Nature Kit | 315 acc (314 GLB) | biome dressing: trees/rocks/plants/mushrooms + crop/fence/campfire/tent/bridge/statue pieces | all 12 zones; farms/villages/ruins | `/Game/Environment/Kenney/NatureKit/` | BiomeDressingActor candidate pool — post tone-check (ticket 05) | 19 native env GLB | **IMPORT_READY** |
| Space Kit | 108 acc (107 GLB) | sci-fi ruins/dungeon dressing: corridors/platforms/pipes/gates/machines/crystals; turret pieces | 3 dungeons + ancient-tech landmarks; Bolt Turret | `/Game/Environment/Kenney/SpaceKit/` | DungeonGenerator + turret visuals (engine) | shell generator + procedural turret | **IMPORT_READY** (character/vehicle/rocket models already excluded at selection — never enter the repo) |
| Blaster Kit | 42 acc (40 GLB + colormap PNG) | energy-weapon held meshes — **CANDIDATE_REPLACEMENT pool** for the 5 procedural weapon meshes | 8 weapon profiles (combat) | `/Game/Weapons/Meshes/Kenney/` | compare-first per archetype in PIE; rebind ONLY if clearly better (ticket 06) | 5 procedural meshes stay bound | **IMPORT_READY** (CANDIDATE_REPLACEMENT — never auto-replace) |
| Particle Pack | 97 acc (96 PNG) | combat VFX sprites: muzzle/impact/spark/smoke/glow | CombatComponent FX pool + VfxActor fallback art | `/Game/FX/Kenney/ParticlePack/` | Niagara sprite renderers AFTER the tRNS-alpha fitness check (§20b step 2) | 3 `NS_AW_*` systems + procedural actors | **IMPORT_READY** |
| UI Pack: Sci-Fi | 693 acc (690 PNG + 2 TTF + lic) | panel/button/slider/icon art for the 7 UMG screens + Kenney Future/Narrow offline fonts | all player-facing UI | `/Game/UI/Kenney/SciFi/` + `/Game/UI/Fonts/` | UMG brush overrides + offline font import (engine) | solid-color C++ UMG | **IMPORT_READY**; 6 color families × Default/Double states — pick 1–2 families post tone check; the other ~4–5 families are a palette (subset **NOT_INTEGRATION_SCOPE**) |
| Survival Kit | 82 acc (80 GLB + colormap + lic) | camps/fires/crates/tools/shelters | all 12 zones; POIs; village outskirts (Dawn Fields, Verdant Reach) | `/Game/Environment/Kenney/SurvivalKit/` | POI/dressing pools (engine) | procedural props | **IMPORT_READY** |
| City Kit (Industrial) | 39 acc (37 GLB + colormap + lic) | containers/cranes/pipes/warehouse shells | Ember Ridge, Stormcrest, research POIs, Dawnstead industrial quarter | `/Game/Environment/Kenney/Industrial/` | POI dressing (engine) | procedural | **IMPORT_READY** |
| Modular Space Kit | 42 acc (40 GLB + colormap + lic) | modular sci-fi interior tiles (walls/floors/doors/corners) | 3 dungeons + Hollow Approach | `/Game/Environment/Kenney/ModularSpaceKit/` | DungeonGenerator room assembly (engine) | shell generator | **IMPORT_READY** |
| Modular Dungeon Kit | 41 acc (39 GLB + colormap + lic) | stone/ancient modular tiles | 3 dungeons' ruin segments + Sunscar ruins | `/Game/Environment/Kenney/ModularDungeonKit/` | DungeonGenerator room assembly (engine) | shell generator | **IMPORT_READY** |
| Animated Characters: Survivors | 5 acc (4 FBX + lic) | NPC body candidate + idle/run/jump locomotion reference | 11 NPCs / 2 villages | `/Game/Characters/Kenney/Survivors/` | import `characterMedium.fbx` as SkeletalMesh + 3 anims; **retarget check vs `SK_Survivor_Exosuit`** (§20b explicit check) | cylinder+sphere placeholder | **IMPORT_READY** (retarget risk explicit — mismatch is a finding, not a defect) |
| Skyboxes | 6 acc (5 PNG + lic) | equirect sky textures: day/morning/night/alien/space | 12-zone atmosphere (shared moods + zone tint) | `/Game/Environment/Kenney/Skyboxes/` (long-lat import) | sky dome material (engine) | procedural sky + zone tint | **IMPORT_READY**; `skybox-space.png` has **no committed consumer today** (Eye dungeon exterior is undecided) — that one file **NOT_INTEGRATION_SCOPE** until a consumer exists |
| Crosshair Pack | 1,602 acc (1,600 PNG + lic + size.txt) | real reticles replacing the text-glyph crosshair | HudWidget hip-fire/aim states | `/Game/UI/Kenney/Crosshairs/` | pick a handful (hip + aim, optionally element-tinted) at engine | text-glyph crosshair | **IMPORT_READY** (HUD subset only); the other ~1,594 = palette library, **NOT_INTEGRATION_SCOPE — never force-imported** |

**Palette rule** (the user's directive, operationalized): a pack or pack-subset whose answer to "WHERE IS THIS USED?" is "nowhere concrete" stays in `ArtSource` unimported. It is never deleted (CC0, cheap, may earn a consumer later) and never imported "just in case" (3,360 imports already lengthen the first cook — §20b step 5).

### 2b. Inverse index — zone/system → applicable packs (the world-side cross-check)

| Zone / system | Applicable Kenney packs (candidates) | Honest coverage note |
| :--- | :--- | :--- |
| Dawn Fields | Nature (village/farm pieces) · Survival (outskirts, camp) | strong |
| Verdant Reach | Nature · Survival | strong |
| Dusk Marsh | Nature (plants/reeds) | moderate (swamp-specific props thin — native dressing carries) |
| Glimmerwood | Nature (forest) | strong |
| Ember Ridge | City Kit Industrial (containers/cranes) · Nature (rocks/cliffs) | strong |
| Frostveil | Nature (pines) | moderate |
| Stormcrest Highlands | City Kit Industrial (research/warehouse) | strong |
| Sunscar | Modular Dungeon (ruin tiles) · Space Kit (ancient-tech) | strong |
| Hollow Approach | Modular Space · Space Kit | strong |
| Azure Shallows / Tidebreaker Isles / Pearlsea Reef (3 sea zones) | Skyboxes (sky mood) · Nature (palm/cliff candidates) | **thin** — sea zones are water-plane + sky dominated; land dressing stays native-only |
| Hollow Underlight (dungeon 1) | Modular Space · Modular Dungeon · Space Kit | strong |
| Sunken Vault (dungeon 2) | Modular Dungeon (stone) · Space Kit | strong |
| Eye of the Maelstrom (dungeon 3) | Space Kit · Modular Space · (skybox-space undecided) | strong; `skybox-space.png` consumer undecided → NOT_INTEGRATION_SCOPE |
| Global systems (no zone) | Particle Pack · UI Pack · Crosshair Pack · Impact/Interface/Sci-Fi Sounds · Blaster Kit | combat/UI/audio feedback everywhere |

No zone depends on any pack: every zone boots and plays with 100% native content (zero-asset boot guarantee). Packs are dressing/upgrade candidates only.

---

## 3. Batch-2 purpose mapping (9 packs — which P0/P1 gap each fills)

| ASSET | PURPOSE (gap filled) | GAME SYSTEM | TARGET ZONE/SCOPE | SOURCE | LICENSE | IMPORT PATH | BINDING | STATUS |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| Particle Pack (96 PNG) | **P0 combat VFX** sprites | CombatComponent FX pool + Niagara (`NS_AW_*`) | global combat | kenney.nl | CC0 | `/Game/FX/Kenney/ParticlePack/` | Niagara sprite renderers post fitness check | **IMPORT_READY** |
| UI Pack: Sci-Fi (690 PNG + 2 TTF) | **P0 UI art** + offline fonts | 7 UMG widget classes | all player-facing UI | kenney.nl | CC0 | `/Game/UI/Kenney/SciFi/` + `/Game/UI/Fonts/` | UMG brush overrides + font import | **IMPORT_READY** |
| Crosshair Pack (1,600 PNG) | **P0 reticle art** (HUD subset ~6 files) | HudWidget hip/aim states | HUD | kenney.nl | CC0 | `/Game/UI/Kenney/Crosshairs/` | dynamic brush per weapon/aim state | **IMPORT_READY** (subset; rest palette) |
| Survival Kit (80 GLB) | **P0 survival props** | POI/village dressing + camp systems | all 12 zones | kenney.nl | CC0 | `/Game/Environment/Kenney/SurvivalKit/` | dressing pools | **IMPORT_READY** |
| City Kit Industrial (37 GLB) | **P0/P1 industrial props** | POI dressing (research/industrial landmarks) | Ember Ridge, Stormcrest, Dawnstead | kenney.nl | CC0 | `/Game/Environment/Kenney/Industrial/` | dressing pools | **IMPORT_READY** |
| Modular Space Kit (40 GLB) | **P1 modular dungeon/space tiles** | DungeonGenerator | 3 dungeons + Hollow Approach | kenney.nl | CC0 | `/Game/Environment/Kenney/ModularSpaceKit/` | room assembly | **IMPORT_READY** |
| Modular Dungeon Kit (39 GLB) | **P1 modular dungeon tiles** (ancient/stone) | DungeonGenerator | 3 dungeons + Sunscar | kenney.nl | CC0 | `/Game/Environment/Kenney/ModularDungeonKit/` | room assembly | **IMPORT_READY** |
| Animated Characters: Survivors (4 FBX) | **P1 NPC bodies** + anim reference | NPCCharacter visuals | 11 NPCs, 2 villages | kenney.nl | CC0 | `/Game/Characters/Kenney/Survivors/` | skeletal retarget (explicit check) | **IMPORT_READY** (retarget risk) |
| Skyboxes (5 PNG) | **P1 sky art** | sky dome / zone atmosphere | 12 zones | kenney.nl | CC0 | `/Game/Environment/Kenney/Skyboxes/` | sky dome material (long-lat) | **IMPORT_READY** |

Batch-2 totals (registry §H): 9 packs / 2,607 accepted files / 32.4 MB. Every batch-2 pack maps to a named gap from the wayfinder gap analysis (ticket 01) — no pack was acquired "just in case".

---

## 4. Gap closure status (Wayfinder P0/P1 gaps)

| GAP | PRIORITY | BATCH-2 COVERAGE (ACQUIRED) | RESIDUAL ENGINE-SIDE WORK | RESIDUAL SOURCE-SIDE WORK | STATUS |
| :--- | :--- | :--- | :--- | :--- | :--- |
| Combat VFX | **P0** | Particle Pack 96 transparent sprites + pre-rotated frames (black-bg duplicates curated out) | Niagara fitness check (tRNS alpha, §20b step 2) → emitter sprite binding into the FX pool → tone check screenshot (ticket 05 evidence) | none | gap **source-closed**; binding **ENGINE_UNVERIFIED** |
| UI art | **P0** | UI Pack Sci-Fi 690 PNG + 2 TTF fonts; Crosshair Pack (HUD subset) | offline font import; UMG brush overrides on the 7 screens; pick reticles (hip/aim) + 1–2 UI color families | none | gap **source-closed**; binding **ENGINE_UNVERIFIED**; non-selected families/reticles **NOT_INTEGRATION_SCOPE** |
| Survival/industrial props | **P0/P1** | Survival Kit 80 GLB + City Kit Industrial 37 GLB | dressing-pool binding for POIs/villages/industrial landmarks; tone check | none | gap **source-closed**; binding **ENGINE_UNVERIFIED** |
| Modular dungeon/space tiles | **P1** | Modular Space Kit 40 + Modular Dungeon Kit 39 GLB (+ batch-1 Space Kit 107 as dressing) | DungeonGenerator room assembly + snap/layout decisions; per-dungeon visual differentiation (pairs with DP-9 mechanics pass) | none | gap **source-closed**; binding **ENGINE_UNVERIFIED** |
| Sky art | **P1** | Skyboxes 5 equirect PNG | long-lat import; sky dome material; zone-mood mapping (5 skies over 12 zones via tint) | none (space variant consumer undecided → that file NOT_INTEGRATION_SCOPE) | gap **source-closed**; binding **ENGINE_UNVERIFIED** |
| Creature/boss meshes | **P0 visual** (not closable by acquisition — structural, per DP-1) | none acquirable (Kenney 2D; Quaternius deferred/QAL) | Tier-A/boss import via `/Game/Characters/Echoes/` + the §20c binding-patch sequence (assets-first, binding-second) + cone replacement only after import confirmed; boss opt-in skeletal path already source-side (DP-1b) | **Tier-B archetype rig library only (~55 species) — ArtSourceGen backlog per `ASTRAWILD_CREATURE_STRATEGY` §10; graceful degradation: those species run on procedural bodies with strengthened material identity** | Tier-A/boss set **source-closed (14 meshes IMPORT_READY)**; Tier-B **MISSING (P1)**; binding **ENGINE_UNVERIFIED** |
| NPC bodies | **P1** | ACS 1 humanoid + 3 clips (candidate, retarget risk) | skeletal import + retarget check vs `SK_Survivor_Exosuit`; rebind NPC body if compatible | none (if retarget fails: placeholder stays — candidate, not a dependency) | candidate **IMPORT_READY**; bodies **ENGINE_UNVERIFIED** |

### 4b. Open decision queue this matrix feeds (engine-side / HITL — not agent-resolved)

| # | Decision | Owner | Evidence needed | Blocks |
| :--- | :--- | :--- | :--- | :--- |
| D1 | Kenney tone verdict (keep/constrain/reject low-poly vs bioluminescent frontier) | user (HITL) | PIE mix-scene screenshots (§20b step 3) | all Kenney dressing binding |
| D2 | Weapon replacement (procedural mesh vs Blaster Kit per archetype) | user (HITL) via Antigravity | side-by-side PIE comparison (§20b step 4) | Blaster Kit rebind — never auto |
| D3 | Particle sprite fitness (tRNS alpha usable?) | Antigravity | 2–3 sprites in a test Niagara emitter (§20b step 2) | combat-VFX binding |
| D4 | ACS retarget compatibility | Antigravity | skeletal import + retarget check | NPC body rebind |
| D5 | UI color family + reticle picks | user (HITL) via Antigravity | themed UMG screenshots | UI art binding (subset selection) |
| D6 | `skybox-space.png` consumer (Eye exterior? starfield sky?) | user (HITL) | PIE dungeon-exterior check | that one file only |
| D7 | ~~Boss mesh generation priority (4 bosses first per DP-1 P0)~~ **RESOLVED at DP-1** — all 4 boss meshes + 4 story meshes delivered IMPORT_READY (V25-C1/C2); binding rides §20c | GLM (source-side backlog) → delivered | none | cone replacement (after import per §20c) |

---

### Integration sequence reminder (unchanged contracts)

1. **Baseline first**: run the standard `import_all.py` pass (the flat-contract procedural import — 120 files on disk at DP-10: the 112-asset contract + the 8 DP-1 Tier-A echo meshes) BEFORE any Kenney import — soft-path fallbacks + zero-asset boot must stay intact (§20b step 1).
2. **Order**: audio → models → textures (§20b step 5). Import only the subsets with a concrete use in §2; leave palette subsets in `ArtSource`.
3. **Never auto-replace**: Blaster Kit (ticket 06) and boss meshes follow the compare-first rule — the procedural/cone originals stay on disk as fallback.
4. **Decisions route back**: tone verdict (ticket 05), weapon keep/rebind (ticket 06), particle fitness — these are Antigravity/HITL answers, not unilateral agent calls (§20b step 6).
5. **Nothing here upgrades any game status**: IMPORT_READY ≠ imported ≠ bound ≠ verified. The game remains READY_FOR_FINAL_BUILD (source gate); every visual/audio upgrade in this matrix lands during the one-time engine run.

*End of DP-2 (v1.0) / DP-10 re-verification (v1.1). One canonical file — per-pipe detail lives in the companion docs referenced in the header.*
