# ASTRAWILD — Production V2, Batch 2: Visual Vertical Slice Runtime Support

**Status:** SOURCE_IMPLEMENTED (compile validation pending on the UE 5.8.2 target machine)
**Date:** 2026-08-30
**Scope driver:** `Docs/ASTRAWILD_PRODUCTION_V2_MASTER_PLAN.md` §31 (Immediate Next Milestone — Visual Vertical Slice) + user priority P1
**Baseline:** `cbdbd82` (V2 Batch 1 — data foundation) → this batch

---

## 1. Batch objective

Close the five biggest *runtime visual* gaps between the verified graybox prototype and the Master Plan §31 Visual Vertical Slice — without duplicating a single existing system and without touching any working gameplay pipeline:

1. the twelve zones were **empty vertex-tinted terrain** (no dressing),
2. the atmosphere rig was **spawned-but-untuned** (default fog, colorless sun, no post-process),
3. Beam/Arc weapons resolved damage with **zero visual feedback**,
4. the scanner had **no world-space feedback** at all,
5. the player was a **plain grey cylinder**, Echoes had **no rarity/element identity**.

Everything is zero-asset (procedural meshes + vertex colors + engine DebugMeshMaterial — the same guaranteed path as the terrain tiles and Echo bodies) and every placeholder has a **data-only Antigravity upgrade path**.

## 2. What was built

### 2.1 Biome dressing system — `AAstrawildBiomeDressingActor` (P1 core)

The **first runtime consumer of `UAstrawildBiomeDefinition`** (the binding surface Batch 1 registered but nothing consumed):

- **Deterministic scatter** per zone: `FRandomStream` seeded from the world seed → identical layout every run/load (world seed replicates, so clients could re-derive it too).
- **Gameplay-aware placement gates** (pure statics, automation-tested):
  - below `GetSeaLevelZ() + 60cm` → rejected (sea zones dress only their islets),
  - slope limits per kind — trees 150cm/360cm, grass 210, rocks 260 → **natural banding** (rocks read on cliffs, trees hold the valleys),
  - **exclusion bubbles** around the camp (3000cm), both villages (2800), both dungeons (2600), every portal (1400), every POI marker (1600) and both skiff pads (1800) — gathered by actor iteration at the end of `BeginPlay`, so dressing **never buries a gameplay space**.
- **Zone personality budgets** (`GetDressingProfile`): 6 canopy archetypes — Broadleaf (Dawn/Verdant), Conifer with **snow-blend** (Frostveil/Stormcrest/Glimmerwood), Palm (islands/reef), Dead (marsh/hollow), Cactus (Sunscar), Spire (Ember) — with per-zone tree/rock/grass counts (jungle 85 trees vs desert 22, Master Plan §5 "regions read differently").
- **Placeholder geometry**: merged per-kind ProceduralMesh sections (trees / rocks / grass) — 3 sections per zone actor, ~430 trees + ~330 rocks + ~480 grass tufts world-wide at ~22k vertices total (trivial cost).
- **Antigravity upgrade path**: when `BiomeDefinition.TreeMeshes/RockMeshes/GrassMeshes` soft refs resolve, the SAME scatter transforms feed `UInstancedStaticMeshComponent`s with the real meshes and the matching placeholder section is skipped. Mixed binding (real trees + placeholder rocks) works. `bEnablePlaceholderDressing=false` = pure-asset mode for editor shots.
- **New `UAstrawildBiomeDefinition` fields (additive)**: `TreeCanopyTint` / `RockTint` / `GrassTuftTint` (White = derive from zone `GroundTint`), `DressingDensity`, `bEnablePlaceholderDressing`. All 12 biomes registered with **explicit production tints** in `AstrawildProductionContent.cpp` (Dawn meadow green → Ember obsidian/ember-glass → Frostveil blue-grey → Glimmerwood violet → Sunscar sandstone…).
- Dressing is **NoCollision by design** (terrain owns collision; real meshes bring their own) — documented limitation.

### 2.2 Atmosphere pass — day/night grading + weather coupling (Master Plan §5/§7/§31)

The lighting rig existed (sun cycle, SkyLight, SkyAtmosphere, fog) but the fog was **spawned with engine defaults and never touched again**:

- **`EvalAtmosphereRamp(SunAlpha, IsNight, Visibility)`** — pure keyframe ramp (automation-tested):
  - sun color: dawn gold → neutral noon → ember dusk; night = cool moonlight (0.55, 0.65, 0.90),
  - fog color: dawn warm → noon neutral-blue → dusk violet-ember; night near-black,
  - fog density + SkyLight intensity by time-of-day; air thickens toward horizon hours.
- **Weather coupling**: `WeatherSubsystem::GetVisibilityMultiplier()` drives fog density ×(1 + visLoss·1.8), sun dimming ×(0.45 + 0.55·vis) and fog desaturation — **Storm Surge (world event) now visibly changes the sky**, not just a HUD label.
- **Sun intensity bug-proofing**: `EvalSunBaseIntensity()` shared by `UpdateSunRotation` and `UpdateAtmosphere` so the weather multiplier **never compounds across ticks** (recomputed from the base curve every tick).
- **Post-process volume** (unbound): restrained production grade — bloom 0.6, vignette 0.35, saturation 1.05, contrast 0.96, exposure clamps 0.06..3.2 + bias +0.3 (night stays playable, noon doesn't blow out).
- Fog initial state now tuned (density 0.00012, height falloff 0.22, start distance 1500) instead of defaults.

### 2.3 Weapon VFX — the Beam/ArcChain gap closed (Master Plan §8/§21)

- **`AAstrawildBeamVfxActor`**: bright vertex-colored stretched prism between muzzle and impact with a point light — **Beam weapons now visibly fire**. Fades by collapsing thickness over 0.16s, then self-destructs. The beam draws to the furthest contact (pierce weapons read full-length).
- **Arc Chain lightning**: one actor per shot renders the whole hop chain (muzzle → first impact → each zapped target) as **jagged multi-segment bolts** — deterministic per-hop quantized seed, ±34cm perpendicular jitter, clean final arrival. The Arc Caster's 4-target chain is finally *readable*.
- **Muzzle flash** on every ranged archetype (projectile/beam/arc): small bright octahedron core + 12k-lumen point light, 0.09s.
- **Element-tinted projectile cores**: `AAstrawildProjectileActor::VisualBody` — low-poly vertex-colored sphere (element tint, hot leading pole). Server/listen view; remote clients keep the constructor sphere (PMC never replicates — documented, Niagara trail is the art-pass target). `TrailVfxId` finally **assigned and consumed** at runtime (was a dead contract field).
- **Shared palette** `FAstrawildVfxPalette` (pure statics, tested): element tints, rarity ladder (grey/green/cyan/violet/amber/crimson), weapon family tints, scanner tier tints — one color language across beams, projectiles, held weapons, rings and pulses.
- Cosmetic-only: spawned on server/listen/standalone (NM_DedicatedServer skipped); dedicated-MP replication deferred to the H-12 RPC pass. Damage pipelines untouched.

### 2.4 Scanner pulse — `AAstrawildScannerPulseActor` (Master Plan §10)

Hold-V now spawns an **expanding ground ring** (48-segment annulus, ease-out to the scanner's effective range ×2.4km scale) tinted by tier — Field teal, Array amber, **Oracle violet with visibly larger radius** (sells the upgrade path) + a fading point light. The scan now reads as an *action* (Master Plan: "scanner is a signature exploration tool").

### 2.5 Echo visual identity (Master Plan §6 "visibly meaningful")

- **Rarity ring**: Rare+ species wear a flattened annulus at their feet in the rarity color (Common/Uncommon stay clean to avoid noise) — appended as BodyMesh section 1, same vertex-color material.
- **Element glow light**: elemental species get a small unshadowed point light (element tint) — **captured party members always glow (2.4), wild elementals only within 3200cm of a player (1.9)** → the active light count stays bounded with 200+ species roaming. 1s cadence, local cosmetic (runs against replicated `bCaptured`).

### 2.6 Player silhouette + held weapon (Master Plan §4 art hierarchy #1)

- **Procedural survivor body** (torso, amber chest plate, helmet + teal visor, scavenger backpack, limbs — graphite/amber ASTRAWILD palette) replaces the grey cylinder. Built in `BeginPlay` → **runs on server AND owning client** (reads in every netmode).
- **Held weapon silhouette**: family-tinted gun (receiver/grip/energy cell/barrel) scaled by tech tier (Field 1.0 → Experimental 1.6), rebuilt on equipment change via a gentle 0.5s poll (`RefreshHeldWeaponVisual`) — **the weapon in your hands telegraphs its firing behavior** (kinetic gunmetal, plasma magenta, arc electric…).

## 3. Changed files

**New (4):** `Public/AstrawildVfxActor.h`, `Private/AstrawildVfxActor.cpp` (BeamVfx + ScannerPulse + shared palette), `Public/AstrawildBiomeDressingActor.h`, `Private/AstrawildBiomeDressingActor.cpp`, `Docs/ASTRAWILD_PRODUCTION_V2_BATCH_2.md` (this file)

**Modified (12):** `AstrawildDataAssets.h` (+5 BiomeDefinition dressing fields), `AstrawildProductionContent.cpp` (explicit tints ×12 biomes), `AstrawildWorldBootstrapper.h/.cpp` (atmosphere ramp + PPV + fog tuning + SpawnBiomeDressing + exclusions + sun-base fix), `AstrawildCombatComponent.cpp` (beam/arc/muzzle VFX spawns + TrailVfxId wiring), `AstrawildProjectileActor.h/.cpp` (VisualBody core + BuildElementCore), `AstrawildPlayerCharacter.h/.cpp` (body + held weapon + scan pulse), `AstrawildEchoCharacter.h/.cpp` (rarity ring + element glow), `AstrawildAutomationTests.cpp` (+8 tests → 47)

## 4. Tests added (39 → 47)

`ASTRAWILD.BiomeDressing.ZoneProfiles` (12 budgets, jungle>desert>reef, frost snow-blend, isles palms) · `.PointRejection` (camp bubble, edges, mismatched-array safety) · `.DeterministicScatter` (same seed → identical layout; sea-zone waterline; camp exclusion) · `ASTRAWILD.Atmosphere.DayRamp` (warm dawn/dusk vs neutral noon, cool night, storm thickening/dimming, clear-never-dims, sun base curve) · `ASTRAWILD.Vfx.Palette` (element/rarity/family/scanner distinctness) · `.BeamMath` (transform/length/orientation, midpoint, diagonal) · `.ArcJitter` (determinism, 5-waypoint structure, bounded jitter, seed variance) · `.RingGeometry` (98 verts/96 tris, radii bands, flatness, clamp safety)

## 5. What Antigravity must do in UE5

1. **Pull + compile** (`git pull origin main` → build `ASTRAWILDEditor Win64 Development`): 4 new files + 12 modified, UHT re-runs. Watch the two brand-new translation units (`AstrawildVfxActor.cpp`, `AstrawildBiomeDressingActor.cpp`) and the bootstrapper's new includes (`Engine/PostProcessVolume.h`, `Components/PointLightComponent.h`, `Components/InstancedStaticMeshComponent.h`, `ProceduralMeshComponent.h`).
2. **Run the 47 automation tests** (`Automation RunTests ASTRAWILD`): 39 prior + 8 new.
3. **First-look VVS checks** (the P1 acceptance test, Master Plan §31: *"a screenshot of the starting area should no longer look like a default UE5 test map"*):
   - Dawn Fields at 08:00 — **dressed terrain** (broadleaf trees, boulders, grass tufts in meadow green), warm dawn fog, amber sun.
   - Walk the camp → **no dressing inside the camp bubble**; walk 60m out → trees around you.
   - Fly/drive to Frostveil → **snow-dusted conifers + blue-grey rocks + cool fog**; Sunscar → cacti + sandstone; Ember Ridge → ember-glass spires + obsidian.
   - Let a day pass: **dawn gold → noon neutral → dusk ember → cool dark night** fog/sun cycle; force a Storm (`ForceWeather` cheat) → **sky visibly darkens + fog thickens within 0.25s**.
   - Fire the **Arc Caster** into a pack → jagged lightning chains through up to 4 targets + muzzle flashes; **Lumen Beam** → bright piercing beam to the furthest target; every projectile shot leaves a tinted core + flash.
   - Hold **V** with each scanner tier → **teal/amber/violet expanding pulse rings** (Oracle visibly largest).
   - Look down at the player → **survivor silhouette with amber accents + held weapon tinted by family** (equip different weapons → the gun changes within 0.5s).
   - Find a Rare+ wild Echo → **rarity ring at its feet**; capture an elemental → it **glows** (party light); a wild Emberfang near you lights up faintly.
4. **Perf sanity** (V2-19 below): dressing is ~22k verts across 36 PMC sections + ≤8 concurrent active element lights + transient VFX lights — `stat unit` should hold 60fps; if the dressing cost ever matters, drop the per-biome `DressingDensity` data value (no code needed).

## 6. Antigravity asset-binding quick reference (unchanged contracts, now with live fallbacks)

| Contract | Field | Placeholder until bound |
|---|---|---|
| Real trees | `BiomeDefinition.TreeMeshes` | merged PMC broadleaf/conifer/palm/dead/cactus/spire |
| Real rocks | `BiomeDefinition.RockMeshes` | rotated irregular boxes |
| Real grass | `BiomeDefinition.GrassMeshes` | 3-blade tinted tufts |
| Landscape material | `BiomeDefinition.LandscapeMaterial` | vertex-colored terrain (unchanged Batch 7/8 path) |
| Beam/trail/muzzle/impact | `WeaponDefinition.*VfxId` (`NS_AW_Weap_*`) | BeamVfxActor prisms/octahedron + light |
| Projectile trail | `TrailVfxId` (now assigned per weapon) | tinted PMC core sphere |
| Ambient audio | `BiomeDefinition.AmbientAudio` | silent |

## 7. Known limitations / honest status

- **Compile status: NOT_RUN in this sandbox** (no UE5 on Linux). Source follows every established compile-safe pattern; `validate_repository.sh` + brace/include/paren audits pass; one defensive API choice (`FMath` quantized hash instead of `GetTypeHash(FVector)`).
- Dressing is visual-only (NoCollision) by design — trees don't block movement until real meshes land.
- PMC visuals (dressing, rings, tinted cores, player body on remote clients) render on the machine that builds them: **listen-server/standalone = everything; dedicated MP = partial** (H-12 multicast pass remains open for the VFX actors).
- The PPV exposure/bloom values are first-pass defaults — tune freely in the volume; the ramp values live in one pure function (`EvalAtmosphereRamp`).
- Scanner pulse spawns on key press (once per hold), not continuously while held — acceptable feedback; continuous pulse trains are a candidate for the audio/VFX pass.

## 8. Verification expectations for this batch

- `Automation RunTests ASTRAWILD` → **47/47 pass**.
- New log line on boot: `Biome dressing placed for 12/12 zones (N exclusion bubbles)` + per-zone dressing counts.
- If anything fails: **exact file:line + full error text** into `Docs/ENGINE_LOGS/` per the protocol — GLM fixes source-side next batch.
