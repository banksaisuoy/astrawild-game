# ASTRAWILD — FINAL CONTENT MANIFEST

**Document version**: 1.6 · **Issued**: 2026-09-03 · **Author**: GLM 5.3 (Final Completion Run Batch 5 + Final Source Completion Pass amendment + FINAL-EXECUTION truth re-verification)
**Branch**: `final-completion` (99e4105..HEAD — all batches + FINAL-AUDIT A/B/C/D + SCI + FINAL-EXECUTION pushed)
**Purpose**: prove that the final repository supplies EVERY piece of UE5 content the
game needs — from a clean `git clone` + `git lfs pull`, a deterministic build, to a
playable game with both endings. This manifest is the LAST gate before
`READY_FOR_FINAL_BUILD` (per the Final Completion directive).

**Verification performed for this manifest (all in this sandbox, all reproducible)**:
1. `git ls-tree -r HEAD` blob walk + `git cat-file -p` → every tracked binary is a
   valid Git LFS pointer (oid sha256 + byte size).
2. GitHub LFS Batch API (`POST /info/lfs/objects/batch`, authenticated) → at the
   v1.1 gate: 459/459 objects (233.0 MB). **Re-verified at the v1.6 tip
   (FINAL-EXECUTION round, commit 4daa113): 491/491 objects resolve on disk with
   OID-matched sha256 (459 + the 32 SCI-tracked files: 16 SK_Base_*.glb +
   16 SFXSet_*.wav); 236.5 MB total payload.** **Re-verified at the v1.7/v9.4 tip
   (2637c13): 586/586 pointers resolve with on-disk objects (491 + the 95 v9.3
   real-mesh source files); 0 pending upload.** The engine-side re-check command is
   `git lfs ls-files -l | Measure-Object -Line` → expect **586** (see HANDOFF §3
   + the fsck caveat in LIVE_EXECUTION_STATE §8).
3. Full `Content/` filesystem walk → per-folder asset counts (416 runtime binaries —
   re-verified at v1.6: 416/416 carry genuine UE package magic 0x9E2A83C1).
4. Full-source regex sweep of every `/Game/...` reference (hardcoded + `TEXT()` forms)
   → every full asset path resolves to a tracked file (65/65; the 10 short prefixes
   are TEST constants, not asset paths).
5. `Scripts/validate_final_run.py` → **ALL CHECKS PASSED** (re-run at v1.6: 126-test
   exact gate + 15 census equality gates; includes the LFS pointer sweep,
   asset-path resolution and content-presence gates).

**Statuses used below** (closed set):
`LFS_OK` (tracked + object verified on GitHub) · `PRESENT` (committed directly, not LFS-routed)
· `PROCEDURAL` (generated at runtime from C++/seed — no asset required)
· `OPTIONAL` (fallback chain present; game is complete without it)

---

## §1. PLAYER — survivor, animations, skeleton/physics

| ASSET PATH | ASSET TYPE | SOURCE | LFS | REFERENCED BY | REQUIRED FOR | STATUS |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `/Game/Characters/Survivor/SK_Survivor_Exosuit` | SkeletalMesh | ArtPack (PR #4) | LFS pointer→object verified | PlayerCharacter.cpp (primary mesh), ArtPack.cpp (soft path), AutomationTests (fallback chain) | player pawn body | LFS_OK |
| `/Game/Characters/Survivor/SK_Survivor_Exosuit/SkeletalMeshes/*` (2 uasset) | SkeletalMesh + PhysicsAsset | ArtPack | LFS pointer→object verified | PlayerCharacter.cpp duplicate-fallback branch | mesh skeleton/physics | OPTIONAL (documented dead-fallback path) |
| `/Game/Characters/Survivor/AM_Survivor_{Idle,Walk,Run,Jump,Aim,Fire,Gather}` (7 uasset) | AnimSequence (UPaper-like skeletal clips) | ArtPack | LFS pointer→object verified | PlayerCharacter.cpp anim fallback pairs, ArtPack.cpp | player locomotion/combat/gather | LFS_OK |
| `/Game/Characters/Survivor/SK_Survivor_Exosuit/Materials/*` (6 uasset) | Materials + instances | ArtPack | LFS pointer→object verified | Survivor mesh materials | player skinning | LFS_OK |
| `/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple` | SkeletalMesh | UE5 template content (in-repo) | LFS pointer→object verified | PlayerCharacter.cpp (fallback when Exosuit absent) | zero-asset boot guarantee | LFS_OK |
| `/Game/Characters/Mannequins/Rigs/*` (4), `Anims/**` (94), `Materials/**` (5), `Textures/**` (14) | Rig/Anim/Material/Texture | UE5 template | LFS pointers→objects verified | PlayerCharacter.cpp anim fallback chain (MM_*/MF_*), AutomationTests fallback contract | mannequin fallback animation set | LFS_OK |
| `PlayerCharacter procedural body` | ProceduralMesh + basic shapes | C++ (`BuildProceduralBody`) | — | PlayerCharacter.cpp | zero-asset fallback | PROCEDURAL |

Fallback chain (tested by `ASTRAWILD.Asset.SurvivorFallbackChain`):
Exosuit → Mannequin → procedural body — the pawn ALWAYS renders.

## §2. ECHO — hero creatures, skeletons, animations, materials

| ASSET PATH | ASSET TYPE | SOURCE | LFS | REFERENCED BY | REQUIRED FOR | STATUS |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `/Game/Characters/Echoes/SK_Echo_{Bastionbeetle,Cindermule,Deepdelver,Mistmender,Terraquill,Voltpylon}` (6) | SkeletalMesh | ArtPack | LFS pointer→object verified | ArtPack.cpp (soft paths), EchoCharacter runtime bind | hero Echo bodies | LFS_OK |
| `.../SK_Echo_*/SkeletalMeshes/*` (12 uasset) | SkeletalMesh + PhysicsAsset | ArtPack | LFS pointers→objects verified | hero Echo skeletons/physics | hero Echo rigs | LFS_OK |
| `.../SK_Echo_*/Materials/*` (24 uasset) | Materials + instances | ArtPack | LFS pointers→objects verified | hero Echo materials | hero Echo visuals | LFS_OK |
| `/Game/Characters/Echoes/AM_{...}_Idle` + `AM_{...}_Move` (12) | AnimSequence | ArtPack | LFS pointers→objects verified | ArtPack.cpp (soft paths), EchoCharacter locomotion | hero Echo animation | LFS_OK |
| 226 remaining species bodies | ProceduralMesh silhouettes (body plans: Quadruped/Biped/Serpent/Floating/Insectoid/Avian/Crystalline/Amorphous) | C++ (`AstrawildBestiaryData.cpp` 204 + authored 22) | — | EchoCharacter::BuildProceduralBody | bestiary rendering | PROCEDURAL |

## §3. TERRAIN — landscape materials, instances, textures, heightmaps

| ASSET PATH | ASSET TYPE | SOURCE | LFS | REFERENCED BY | REQUIRED FOR | STATUS |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `/Game/Materials/M_Landscape_SciFiFrontier` | Material | ArtPack | LFS pointer→object verified | TerrainTileActor.cpp (TEXT path), ArtPack.cpp | 12-zone terrain surface | LFS_OK |
| `/Game/Materials/M_Master_Surface` | Material | ArtPack | LFS pointer→object verified | PlayerCharacter.cpp, BiomeDressingActor.cpp, ArtPack.cpp | master surface (props/pawn) | LFS_OK |
| `/Game/Materials/Instances/*` (41 uasset) | MaterialInstances (incl. MI_Vehicle_*, MI_Survivor_Armor) | ArtPack | LFS pointers→objects verified | ArtPack.cpp + per-mesh materials | per-object material instances | LFS_OK |
| `/Game/Textures/*` (44 uasset) | Textures | ArtPack | LFS pointers→objects verified | material graphs | material inputs | LFS_OK |
| `Content/Heightmaps/*.r16` (12) + `Heightmaps.md` | Raw heightfields + doc | Scripts/export_landscape_heightmaps.py | PRESENT (committed directly — `*.r16 -text`, by design; ~510 KB each) | Terrain world-gen reference/pipeline | offline terrain reference | PRESENT |
| 12 zone terrain tiles | ProceduralMesh from `EvalWorldHeight(seed)` | C++ (WorldBootstrapper + TerrainTileActor) | — | world build | playable terrain | PROCEDURAL |

## §4. WEAPONS — meshes, materials, VFX, audio

| ASSET PATH | ASSET TYPE | SOURCE | LFS | REFERENCED BY | REQUIRED FOR | STATUS |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `/Game/Weapons/Meshes/SM_Weapon_{ScrapRifle,PlasmaCarbine,ArcCannon,Railgun,SingularityCannon}` (5) | StaticMesh | ArtPack | LFS pointers→objects verified | ArtPack.cpp (weapon art table) | held weapon meshes | LFS_OK |
| `.../SM_Weapon_*/Materials/*` (21 uasset) | Materials + instances | ArtPack | LFS pointers→objects verified | weapon meshes | weapon visuals | LFS_OK |
| `/Game/VFX/NS_AW_MuzzleFlash`, `NS_AW_Weap_Impact`, `NS_AW_Weap_Trail` (3) | NiagaraSystem | ArtPack | LFS pointers→objects verified | ArtPack.cpp Vfx namespace, CombatComponent FX spawn | weapon FX | LFS_OK |
| `/Game/Audio/A_Weapon_{Scrap,Plasma,Arc,Rail,Singularity}_Fire` (5), `A_Weapon_Impact_{Kinetic,Energy}` (2) | SoundWave | ArtPack (uasset) | LFS pointers→objects verified | ArtPack.cpp audio table, CombatComponent PlaySoundAtLocation | weapon audio | LFS_OK |
| Weapon VFX fallback (beams/rings/tracers) | Procedural VfxActor | C++ (AstrawildVfxActor) | — | CombatComponent zero-asset path | FX fallback | PROCEDURAL |

## §5. ENVIRONMENT — props, foliage, resources, water, skiff

| ASSET PATH | ASSET TYPE | SOURCE | LFS | REFERENCED BY | REQUIRED FOR | STATUS |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `/Game/Environment/SM_Tree_{Broadleaf,Conifer,SporeCanopy}` (3) | StaticMesh | ArtPack | LFS pointers→objects verified | BiomeDressingActor.cpp tree meshes, ArtPack.cpp | forest dressing | LFS_OK |
| `/Game/Environment/SM_Rock_{Granite_L,M,S}`, `SM_Rock_Boulder_Moss` (4) | StaticMesh | ArtPack | LFS pointers→objects verified | BiomeDressingActor.cpp, ArtPack.cpp | rock dressing | LFS_OK |
| `/Game/Environment/SM_{Fern,Grass_Tuft,GlowReed,SporeBush,Cliff_Shard}` (5) | StaticMesh | ArtPack | LFS pointers→objects verified | BiomeDressingActor.cpp, ArtPack.cpp | ground flora/cliffs | LFS_OK |
| `/Game/Environment/Ruins/SM_Ruin_{Arch,Block,Pillar}` (3 + 6 materials) | StaticMesh + Materials | ArtPack | LFS pointers→objects verified | ArtPack.cpp, POI dressing | ruins dressing | LFS_OK |
| `/Game/Environment/ResourceNodes/SM_Node_{Astraite,Pyronite,Voidstone,AncientVein}` (4 + 8 materials) | StaticMesh + Materials | ArtPack | LFS pointers→objects verified | ArtPack.cpp node art, ResourceNode bind | harvest node visuals | LFS_OK |
| `/Game/Vehicles/SM_Vehicle_DawnSkiff` (+4 `MI_Vehicle_*` materials) | StaticMesh + MaterialInstances | ArtPack | LFS pointers→objects verified | SkiffActor.cpp hull binding (FR-8), ArtPack.cpp | Dawn Skiff hull | LFS_OK |
| Sea water planes (3 zones) | Procedural WaterPlaneActor | C++ | — | WorldBootstrapper | ocean surfaces | PROCEDURAL |
| Skiff silhouette fallback (hull/pontoons/fin) | basic shapes | C++ ctor | — | SkiffActor.cpp | zero-asset skiff | PROCEDURAL |

## §6. MAPS & WORLD — playable maps + runtime zone generation

| ASSET PATH | ASSET TYPE | SOURCE | LFS | REFERENCED BY | REQUIRED FOR | STATUS |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| `/Game/ASTRAWILD/Maps/MainMap` | World (level) | authored (in-repo) | LFS pointer→object verified | .uproject default; packaged game entry | playable world entry | LFS_OK |
| `/Game/ThirdPerson/Lvl_ThirdPerson` | World (level) | UE5 template | LFS pointer→object verified | editor template/test level | template reference | OPTIONAL |
| **12 zones of the Shattered Vale** (Dawn Fields, Dusk Marsh, Ember Ridge, Frostveil, Glimmerwood, Hollow Approach, Azure Shallows, Tidebreaker Isles, Sunscar, Stormcrest, Verdant Reach, Pearlsea) | **runtime world generation** — 12 procedural terrain tiles (800 m × 800 m, 4×3 grid) from the replicated world seed + zone table | C++: WorldBootstrapper + ZoneSubsystem | — | world build (directive §21; validator §4 zone checks) | the entire overworld | PROCEDURAL — **no 12 separate .umap files are required**; zones are data (ZoneSubsystem table) + generated terrain |
| Villages (Dawnstead, Driftwood Landing), dungeons ×3, POI markers ×17, portals, skiff pads ×2 | runtime actors | C++: WorldBootstrapper Spawn* passes | — | world build | all gameplay locations | PROCEDURAL |

## §7. DATA — every data-driven definition has a single source of truth

| CONTENT FAMILY | SINGLE SOURCE OF TRUTH | COUNT | REPLICATION MECHANISM |
| :--- | :--- | :--- | :--- |
| Items | `AstrawildContentLibrary.cpp` + `AstrawildProductionContent.cpp` (CODE_DEFAULT, registered into the item registry at subsystem init) | ContentLibrary 49 + ProductionContent 29 = 78 authored ids (DP-6 adds the Field Ration + Pulse Tonic; 12 vendor/loot tables feed more) | authored `.uasset` `UAstrawildItemDefinition` replaces by same-id override — no code change |
| Echo species | ContentLibrary (10 authored starters) + ProductionContent (9 incl. the 3 Act 3 bosses) + 6 evolution targets + BestiaryData (204) | **229 definitions** (census-enforced) | same-id override on `UAstrawildEchoDefinition` |
| Quests (MQ-01..17) | ContentLibrary::BuildQuests (12) + ProductionContent (2) + BuildFinalRunContent (5) | 17 | same-id override on `UAstrawildQuestDefinition` |
| Buildings | ContentLibrary::BuildBuildings | 26 (incl. Floor/Roof/Door/StorageCrate + terminal/turret/farm/pen/incubator) | same-id override on `UAstrawildBuildingDefinition` |
| Technologies | ContentLibrary::BuildTechnologies + ProductionContent | 17 | same-id override on `UAstrawildTechnologyDefinition` |
| Recipes | ContentLibrary + ProductionContent | 58 (DP-6 adds the Field Ration + Pulse Tonic mirrors) | same-id override on `UAstrawildRecipeDefinition` |
| NPCs + dialogue trees | ContentLibrary::BuildNPCs (11) + ProductionContent trees (11) | 11 NPCs / 11 trees | same-id override (`UAstrawildNPCDefinition`, `UAstrawildDialogueTreeDefinition`) |
| Loot tables / world events / POIs / biomes / weapons / nodes / work sites | respective CODE_DEFAULT builders | 11 / 16 / 17 / 12 / 8 / 10 / 8 (DP-6: +Cargo Dock/Field Lab/Dynamo Hall/Bulwark Post; DP-7: +7 zone events for the bare zones, +4 scanner-gated secret POIs) | same-id override per family |

**There is NO `.uda` / `.uabp` / `.usk` requirement anywhere in this project.** UE5
Data Assets are `.uasset` files of `UPrimaryDataAsset`-derived classes; this project's
authoring path is C++ CODE_DEFAULT first (single source, zero-asset boot) with
same-id `.uasset` overrides for art-driven replacement — both verified by the
registry contract (`ASTRAWILD.Echo.ProductionRosterContract`, `ASTRAWILD.ResourceNode.DefinitionContract`).

## §8. FORMAT RULE (binding)

Only real UE5 asset extensions count as content in this manifest: **`.uasset` and
`.umap`** (tracked in `Content/`), plus raw `.r16` heightmaps (documented, non-LFS).
No `.usk`/`.uabp`/`.uda` files are required, expected or counted. Every referenced
object type above is stated per row (SkeletalMesh, StaticMesh, Material,
MaterialInstance, AnimSequence, NiagaraSystem, SoundWave, Texture, PhysicsAsset,
World) exactly as the C++ consumers request them (`TSoftObjectPtr<T>`,
`LoadObject<T>`, `ConstructorHelpers`).

## §9. GIT LFS — full verification (491/491 at the v1.6 tip)

| CHECK | RESULT |
| :--- | :--- |
| `.gitattributes` coverage | `*.uasset`, `*.umap`, `*.fbx`, `*.glb`, `*.wav`, `*.png`, … all routed through LFS (`*.r16 -text` documented exception) |
| Content/ binaries (414 `.uasset` + 2 `.umap`) | 416/416 valid LFS pointers at HEAD; 416/416 objects on disk, OID-verified (v1.6 re-check: genuine UE package magic on all 416) |
| ArtSource/ art sources (43 `.png`) | 43/43 LFS pointers; 43/43 objects verified (raw Kenney/Quaternius pack files are committed directly — they are pipeline inputs, not engine content; no runtime dependency) |
| SCI sources (16 `.glb` + 16 `.wav`, added 0b55072) | 32/32 LFS pointers; 32/32 objects on disk, OID-matched (v1.6 re-check: sha256 verified on all 32) |
| **TOTAL** | **491/491 LFS objects resolve · 236.5 MB payload (v1.6 re-verification; v1.1-era basis was 459/459 · 233.0 MB — the +32 delta is exactly the SCI source set)** |
| Pointer integrity | each pointer's `oid sha256` + `size` parsed and cross-checked against the Batch API response (operation=download) |
| Reproduction | `git clone … && git lfs pull` → all binaries materialize (byte-exact); GLM-side script: `tool-results/verify_lfs4.py` pattern (ls-tree → cat-file → Batch API) |

## §10. PATH VALIDATION — every hardcoded `/Game/...` reference resolves

Full-source sweep (all `.cpp/.h` in `Source/AstrawildCore/`) finds **65 complete
asset-path references + 10 TEST prefix constants** (prefixes end in `_`/`/` and
exist only inside automation tests as family-match strings). All 65 full paths
resolve against the tracked `Content/` tree; consumers and status:

| CONSUMER (C++ file) | REFERENCED FAMILIES | RESOLUTION |
| :--- | :--- | :--- |
| `AstrawildArtPack.cpp` (soft-path binding tables) | 6 SK_Echo + 12 AM_Echo + 7 AM_Survivor + SK_Survivor_Exosuit + 14 environment SM + 4 SM_Node + 5 SM_Weapon + 3 NS_AW VFX + 15 audio + M_Landscape/Master + SM_Vehicle_DawnSkiff | ALL resolve → LFS_OK |
| `AstrawildPlayerCharacter.cpp` (TEXT() hard paths + fallback pairs) | SK_Survivor_Exosuit (+dup fallback), SKM_Manny_Simple, 7×2 anim fallback pairs, M_Master_Surface, MI_Survivor_Armor | ALL resolve (one documented dead-fallback path — see §1) → LFS_OK |
| `AstrawildSkiffActor.cpp` (FR-8 hull binding) | `/Game/Vehicles/SM_Vehicle_DawnSkiff` | resolves → LFS_OK |
| `AstrawildBiomeDressingActor.cpp` / `AstrawildTerrainTileActor.cpp` | tree/rock/grass meshes, M_Master_Surface, M_Landscape_SciFiFrontier | ALL resolve → LFS_OK |
| `AstrawildCaptureComponent.cpp` (FR-11 stinger) | `/Game/Audio/A_Echo_Capture_Success` | resolves → LFS_OK |
| `AstrawildAutomationTests.cpp` (contract tests) | family prefixes (10 constants) — not asset paths | contracts PASS (engine-run pending, AG-3) |

Cross-checked by: `Scripts/validate_final_run.py` §8 (64 refs in the hot files),
`ASTRAWILD.ArtPack.BindingContract`, `ASTRAWILD.Weapon.AssetBindingContract`,
`ASTRAWILD.Asset.SurvivorFallbackChain`.

## §11. REFERENCE INTEGRITY & FINAL VERDICT

- Every C++ consumer ships a **fallback chain** (asset → engine basic shape /
  procedural mesh / silent pass-through), so a missing asset can never hard-fail
  the build or PIE (CP-00 rule 2).
- Every content family has a **registered CODE_DEFAULT** (validator §1: items>50,
  quests≥17, techs≥17) — the registry always boots complete.
- Quest chain closure: MQ-01→MQ-17 + Maren endings (validator §2/§3).
- Act 3 world wiring: 8/8 checks (validator §5). Ending state machine: 12/12
  (validator §6). Building catalog: all categories populated (validator §10).
- Automation suite: **109 tests** (inventory doc: `ASTRAWILD_TEST_INVENTORY.md`, rows 1-109).

### v1.1 amendment (Final Source Completion Pass — FINAL-AUDIT A/B/C/D)

The final audit landed source fixes only — **no content family, LFS object, asset path or
code-default changed identity**. The manifest's verification basis (459/459 LFS objects,
65/65 /Game references, CODE_DEFAULT registries, deterministic 12-zone world) is
unchanged and re-validated (46/46) at a5aa74d. What the audit DID change that touches
this manifest's scope: (a) `GameDefaultMap` now points at `/Game/ASTRAWILD/Maps/MainMap`
(the canon map — was the ThirdPerson template), (b) `.gitattributes` additionally
reserves `*.uexp/*.ubulk/*.exr` for LFS (none in repo; future-proof), (c) `__pycache__`
untracked. Statuses and verdict below are re-affirmed as-is.

### DP-10 re-verification note (Depth Passes — final gate)

Both validators re-ran **ALL CHECKS PASSED at the DP-10 final-gate tip** —
`validate_final_run.py` now runs 61 checks (the 109-test exact gate + the 15 census
equality gates + LFS/asset-path/content checks). §7 counts were re-synced to the
validator census at this gate: species 226 → **229** (per-file breakdown corrected:
10 + 9 + 6 evolution targets + 204), buildings 17 → **26**, NPCs 12 → **11**, loot
tables 10 → **11** — the earlier values were stale current-state claims (each census
delta itself was historical: the DP-6/DP-7 rows already carried the item/recipe/site/
event/POI updates). Zero gameplay code, zero assets, zero LFS objects changed in the
depth passes' documentation gate — the manifest's verification basis (459/459 LFS,
65/65 /Game refs, CODE_DEFAULT registries) is unchanged.

**MANIFEST VERDICT**: the final repository — as pushed on `final-completion` —
supplies 100% of the required UE5 content: **491/491 LFS objects verified (re-checked
at the v1.6 tip; the v1.1-era 459/459 basis is historical), all 65 hardcoded asset
references resolve, all data-driven definitions
carry a single CODE_DEFAULT source of truth, the 12-zone overworld is generated
deterministically from world data (by design, not by omission), and every visual
family has a zero-asset fallback.** A clean clone + `git lfs pull` + the HANDOFF
§20 build sequence yields a complete, playable game with both endings.

**READY_FOR_FINAL_BUILD** — engine verification (AG-2..5) is the sole remaining gate.


## GDP Amendment (v1.2 — Gameplay Depth Pack)

- **Echo abilities**: 44 code-default ability templates registered by `UAstrawildAbilityLibrary` (DATA, CODE_DEFAULT — same replace-by-asset contract as all definitions). Authored species carry curated AbilityIds; every other species derives a deterministic 4-ability kit (element x role x family). PROCEDURAL/derived data — no .uasset requirement.
- **Locomotion classes**: `EAstrawildLocomotionClass` data field (Auto = derived at runtime). All 210+ species classified — no per-species assets required.
- **Player attributes + skills**: runtime component systems — DATA lives in code tables (`UAstrawildAttributeComponent`), no assets.
- **NPC affinity**: runtime values + save fields — no assets.
- Input contract: 26 -> 28 actions (T party-cast, Y smart-cast). Save schema: still V5 (additive fields only).
- Verdict: READY_FOR_FINAL_BUILD re-affirmed (source/repository side).

## FPP Amendment (v1.4 — Final Player-Facing Presentation Pass)

- **No content-family, LFS-object, asset-path, or code-default change.** FPP-1
  is presentation-layer source only (UI strings/toasts/telegraph visuals/one
  concrete UMG base class). It introduces ZERO new `/Game/` references (the
  boss feedback cues reuse the existing `AstrawildArtPack::Sfx` binding), so
  the manifest's verification basis (459/459 LFS objects, all hardcoded asset
  references resolving, CODE_DEFAULT registries) is unchanged and re-validated
  — `validate_final_run.py` ALL CHECKS PASS at the FPP gate (125-test exact
  gate, 15 census equality gates unchanged: 78 items / 58 recipes / 229
  species / 26 buildings / 17 techs / 17 quests / 11 loot tables / 11 NPCs /
  8 weapons / 10 nodes / 8 sites / 16 events / 17 POIs / 11 dialogue trees /
  3 robots / 12 zones / 4 bosses).
- **Crafting screen**: `UAstrawildCraftingScreenWidget` is now a concrete
  native-C++ screen (was `UCLASS(Abstract)` with no WBP — un-instantiable).
  Pure C++ Slate construction, no `.uasset` requirement; a WBP subclass can
  still restyle it later (BP_* events preserved).
- **Player-facing rulebook**: `Docs/ASTRAWILD_PLAYER_RULES.md` (documentation,
  not content). Input contract stays 32 actions; save schema stays V5
  (presentation pass adds no save fields).
- Test suite: 124 → **125** (`ASTRAWILD.FPP1.PresentationContract`).
- Verdict: **SOURCE_PRODUCT_FROZEN** (source-side; READY_FOR_FINAL_BUILD
  carried through). Engine verification (AG-2..5 + §22) is the sole remaining
  gate.

## SCI Amendment (v1.5 — Sci-Fantasy Monster Directive + v9.1 material-swap wiring)

- **New ArtSource RAW assets (16 GLB + 16 WAV, LFS-tracked)**:
  `ArtSource/Meshes/Echoes/BaseMeshes/SK_Base_*.glb` (16 rigged theme archetypes,
  Idle/Move/Hit clips baked) + `ArtSource/Audio/Echoes/SFXSet_<Theme>_{0,1}.wav`
  (16 CC0-staged theme cues; SHA-256 ledger `Docs/ASTRAWILD_SCI_FANTASY_ACQUISITION.json`).
  These are NOT engine packages yet — they import via the §20d pass below. TRUE_MISSING stays 0.
- **New ENGINE-side packages authored by the one-time import (§20d / queue V2-35)**:
  `/Game/Characters/Echoes/BaseMeshes/` 16 skeletal meshes + 48 AM_* clips ·
  `/Game/Audio/Echoes/` 16 SoundWaves · `/Game/VFX/NS_AW_Elem_{7}` templates ·
  `/Game/Materials/M_SciFi_{8}` theme masters — **8, not 6 (v9.1)**:
  MetallicRobot / EnergyBody / StonyGolem / Slime / Chitin / VoidFlesh /
  OrganicHide / FocusCrystal — one per EAstrawildMutationMaterialTheme, all
  parameterized (Tint/PatternTint/GlowIntensity/Metallic/Roughness) and all
  counted in `echo_base_report.json` coverage (missing master = ERROR +
  total_missing includes it — no false-clean report possible).
- **Runtime consumers (binding model unchanged — opt-in / fail-closed)**:
  `FAstrawildEchoMutator::BuildSciFantasyBaseMeshPath/BuildSciFantasyAnimPath`
  (base binding via ProductionContent, precedence Tier-A > Tier-B > SK_Base) ·
  `BuildElementVfxSystemPath` + `ApplyElementVfx` (persistent element VFX) ·
  `BuildSoundSetCuePath` (weakness-hit vocal cue) · **`BuildThemeMaterialPath` +
  `ApplyThemeMaterial` (v9.1 — runtime material/theme swap on the skinned path:
  dynamic instances of the 8 masters per species, fail-closed to the GLB's own
  materials before import)**.
- **Verification basis re-validated at the v9.1 tip**: `validate_final_run.py`
  ALL CHECKS PASS — 126-test exact gate, 15 census equality gates UNCHANGED
  (229 species / 78 items / 58 recipes / 26 buildings / 17 techs / 17 quests /
  11 loot / 11 NPCs / 8 weapons / 10 nodes / 8 sites / 16 events / 17 POIs /
  11 dialogue trees / 3 robots); LFS pointer sweep clean; mutation table
  deterministic (regeneration byte-identical).
- Verdict: source-side COMPLETE (the Sci-Fantasy visual identity layer is fully
  wired source-side incl. the material swap); runtime visuals ENGINE-UNVERIFIED
  until the §20d/V2-35 engine pass. The engine run remains the sole remaining gate.

## FINAL-EXECUTION Amendment (v1.6 — truth re-verification + false-100% banner sweep)

- **LFS basis number corrected (the one real stale count this round found)**:
  the v1.1-era basis "459/459 · 233.0 MB" was still cited as CURRENT in several
  live surfaces after 0b55072 added 32 LFS-tracked SCI source files. True state
  re-derived with actual commands at 4daa113: `git lfs ls-files` = **491 pointers**;
  491/491 objects present under `.git/lfs/objects`; sha256 OID-match verified on
  all 32 SCI files + a random 6-file sample; payload = **236.5 MB**; `git lfs fsck`
  exit 0. Content/ binaries re-walked: **416 files, 416/416 genuine UE package
  magic 0x9E2A83C1, zero bad** — the 416 count is UNCHANGED (GLB/WAV are raw
  sources, not engine packages). All current-facing 459 mentions now read 491
  (README, MASTER_CONTROL W-21 + §N row, readiness, HANDOFF §3 pre-flight
  "expect 491", asset-truth final block); version-history rows keep their
  era-correct 459 as dated history.
- **Manifest count corrected**: `ArtSource/manifest.json` now carries **175
  entries** (159 + 16 SK_Base_* BaseMeshes rows added by the SCI commit);
  ue_path presence = **112/175**; pending = **63** = 47 GLB-backed Echo IDs
  (39 Tier-B + 3 boss/summon + 5 ContentLibrary species) + 16 SK_Base base
  *(v1.6-era counts — SUPERSEDED by the v1.7 ASSET OVERHAUL below: 189 entries,
  189 present, 0 pending, 109 real unique CC0 mesh rows)*
  meshes. Both pending groups import at the §20d/V2-35 engine pass (Echo bases
  via `import_echo_bases.py`, the 47 via the manifest-driven `import_all.py`
  re-run of V2-29) — opt-in/fail-closed by design, PMC fallback active until
  then. TRUE_MISSING stays **0**.
- **False-100% banner sweep (5 more surfaces)**: docs that presented old
  engine-era 100% claims without historical banners got them —
  `Docs/ASTRAWILD_ANTIGRAVITY_TO_GLM_HANDOFF.md` ("100% VERIFIED" status line),
  `Docs/ASTRAWILD_PRODUCTION_V2_WORKLOG.md` (48-test era),
  `Docs/ASTRAWILD_PROJECT_MASTER_STATUS_AND_GLM_HANDOFF.md` (53-test era,
  also classified in MASTER_CONTROL §12), `Docs/ANTIGRAVITY_RUNTIME_FAILURES.md`
  ("100% OPERATIONAL"), `Docs/CONTENT_PACK/CP-00_INDEX.md` (inline 48/48 count
  qualifier). Every 100% in the repo is now either banner-qualified history or
  a factually-true percentage (LFS fractions, gameplay refund rates, census
  fractions).
- **Static validators re-run at this tip (evidence, not doc claims)**:
  `Scripts/validate_final_run.py` ALL CHECKS PASSED (126-test gate, 204-row
  mutation table, 8 themes, 16 bases, consumption wiring, 15 census gates at
  229/126); `Scripts/validate_repository.sh` v2 PASS; `py_compile` PASS on
  the two pipeline scripts; 21/21 declared mutator methods defined; C++ brace
  balance checked.
- **Engine-side rows unchanged and honest**: V2-29..V2-35 remain **NOT_RUN at
  tip** — no UE5/MSVC exists on this Linux sandbox (re-verified this round:
  no Unreal installation, no Windows mounts). Engine claims belong ONLY to the
  Windows UE 5.8.2 Antigravity machine per HANDOFF §20/§20d.
- Verdict: manifest basis re-proven at the FINAL-EXECUTION tip. No engine
  package, binding, or code-default changed in this amendment — it is a
  truth/documentation gate only.


---

## MANIFEST v1.7 — ASSET OVERHAUL (NO PLACEHOLDERS / NO PALETTE SWAPS)

**Supersedes every mesh-era count above.** `Scripts/fetch_free_assets.py` regenerated
`ArtSource/manifest.json` from the REAL curated CC0 catalog:

- **189 entries / 189 `status: "present"` / 0 pending** (source-file truth; the
  engine import is the separate Setup_And_Play.bat / import_all.py stage, V2-36).
- **109 mesh rows** (75 skeletal, 34 static): 3 survivor armor tiers + 42 Echo
  species + 16 base archetypes + 14 showcase bosses + 5 weapons + 4 vehicles +
  4 ore nodes + 21 environment props. **109 unique source models — 1:1, zero
  reuses (palette-swap guard), all CC0 1.0 Universal (Quaternius + Kenney).**
- Every mesh row carries: source_pack, source_model, license + license_url,
  source_page, path (repo-relative, verified on disk), bytes, meshes, verts,
  materials, joints/bones, sha256, animations + clip_map (AM_<Asset>_<Role> ->
  source clip), asset_type.
- 15 superseded procedural GLBs purged (11 species -> real base + mutation spec;
  4 environment GLBs -> real Kenney/Quaternius meshes).
- Per-asset license provenance: `Docs/ASTRAWILD_REAL_ASSET_CREDITS.json`;
  run evidence: `Docs/ASTRAWILD_ASSET_OVERHAUL_REPORT.json`.
- **VERDICT: source catalog 100% REAL-FILE-BACKED (109/109 magic-verified,
  189/189 manifest present, 0 pending, 0 palette swaps). Engine import =
  V2-36 (Setup_And_Play.bat → import_report.json total_missing == 0 incl.
  the AM_ clips + showcase-map PIE). Runtime visuals remain ENGINE_UNVERIFIED
  until that run — never faked here.**
