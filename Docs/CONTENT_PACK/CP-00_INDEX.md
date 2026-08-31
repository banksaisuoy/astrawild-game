# ASTRAWILD — PRODUCTION CONTENT PACK (CP-00 MASTER INDEX)

**Purpose:** The complete asset/content specification that turns the verified zero-asset runtime
(48/48 automation tests green on UE 5.8.2 — see `Docs/ENGINE_LOGS/BATCH8_PLAYTEST_LOG.md`) into a
produced game. Every spec below binds to **existing C++ data contracts** — no new systems, no
dangling architecture. Assets slot into soft-reference fields that already fall back to the
procedural Batch-2 visuals, so **the game keeps running identically before, during and after the
art pass**.

**Consumers:**
- **Antigravity (UE5 host):** implements/imports assets per spec, binds them on `.uasset`
  definitions, verifies against each pack's acceptance criteria.
- **GLM 5.3 (GitHub):** owns the source/data side; extends definitions when a spec needs a
  binding point that does not exist yet (this batch already added the ones marked **NEW**).

---

## Pack contents

| Pack | Area | Primary binding (C++) | Status |
|---|---|---|---|
| [CP-01](CP-01_PLAYER.md) | Player — exosuit, armor tiers, materials, equipment visuals | `UAstrawildItemDefinition::EquipMeshOverride/EquipMaterialOverride` **NEW**, tier fields | MESH+ANIMS DELIVERED (Batch 4: SK_Survivor_Exosuit + 7 clips; EquipMeshOverride consumption next) |
| [CP-02](CP-02_ECHO.md) | Echo — 8 body plans, evolution, elements, rarity, animations | `UAstrawildEchoDefinition::EvolveToDefinitionId` + gates **NEW**, `UAstrawildEchoRosterSubsystem::EvolveInstance` **NEW** | 6/6 STARTER MESHES DELIVERED (Batch 4) + SOURCE_TESTED |
| [CP-03](CP-03_WEAPONS.md) | Weapons — 5 launch families, muzzle/impact/projectile FX, sound | `UAstrawildWeaponDefinition::MuzzleFlashVfx/ImpactVfx/ProjectileTrailVfx/FireSound/ImpactSound` **NEW** | 5 MESHES + 8 AUDIO DELIVERED + PATHS BOUND (Batch 4) |
| [CP-04](CP-04_ENVIRONMENT.md) | Environment — 12 biome asset lists, foliage, rocks, crystals, ruins, POIs | `UAstrawildBiomeDefinition::TreeMeshes/RockMeshes/GrassMeshes/LandscapeMaterial/AmbientAudio` | 19 MESHES DELIVERED + 12 BIOMES BOUND (Batch 4) |
| [CP-05](CP-05_NIAGARA.md) | Niagara — weapon, capture, scanner, shield, elemental systems | CP-03 fields + capture/scanner hooks (procedural fallbacks active) | FX TEXTURES + PATHS READY; 3 HERO SYSTEMS = RUNBOOK §3 (manual, in-editor) |
| [CP-06](CP-06_AUDIO.md) | Audio — weapons, creatures, ambience, UI, footsteps, environment | CP-03 sound fields + `UAstrawildBiomeDefinition::AmbientAudio` | 36 WAV DELIVERED + BOUND (Batch 4; SoundCue mixing next) |
| [CP-07](CP-07_MATERIALS.md) | Materials — landscape, metal, armor, crystal, water, hologram | Direct asset assignment (meshes/materials reference these) | 2 MASTERS + ~30 INSTANCES SCRIPTED (AwPipeline; water/hologram next) |
| [CP-08](CP-08_ANIMATION.md) | Animation — player locomotion/combat/dodge, Echo locomotion, capture | `ABP_` layer over existing C++ movement state | 25 CLIPS DELIVERED + CODE-DRIVEN STATE MACHINES (Batch 4; ABP upgrade path open) |
| [CP-09](CP-09_QUEST_NPC.md) | Quest/NPC content — dialogue trees, story flags, villages | `UAstrawildDialogueTreeDefinition` **NEW** (6 trees shipped) + 12 NPCs | SOURCE_IMPLEMENTED |
| [CP-10](CP-10_UX_HUD.md) | UX/HUD polish — glassmorphism, hit markers, radar | Pure-C++ UMG screens (HUD/shop/dialogue) restyle targets | SPEC_READY |

---

## Global rules (every pack)

1. **Asset naming:** `NS_` Niagara systems, `M_` materials, `MI_` material instances,
   `SM_` static meshes, `SK_` skeletal meshes, `T_` textures, `A_` sound waves,
   `SC_` sound cues, `AM_/ABP_` anim meshes/blueprints. Directory:
   `Content/ASTRAWILD/<Area>/<AssetName>.uasset`.
2. **Zero-asset fallback always:** every binding below has a live procedural fallback
   (Batch-2 VFX / PMC silhouettes / silence). Assets must REPLACE the fallback, never
   require code changes to appear.
3. **No synchronous loads:** all bindings are soft refs consumed via
   "loaded-or-fallback" dispatch. Pre-load via Asset Manager before level entry.
4. **Color = `FAstrawildVfxPalette`:** element/rarity/family/scanner tints are the single
   color language (CP-05 §palette). Derived materials parameterize on the same values.
5. **Budgets are hard gates:** tri counts, texture memory, particle counts and draw-call
   targets in each pack are acceptance criteria, not suggestions (60 FPS target, see
   `Docs/ASTRAWILD_PERFORMANCE.md`).
6. **No protected IP:** silhouettes, names and sounds are original ASTRAWILD designs.

## Suggested production order (Antigravity)

1. **CP-07 Materials + CP-04 Environment for Dawn Fields** (the starting biome) — biggest
   visual win per hour invested.
2. **CP-01 Player + CP-08 Animation** — the character is on screen 100% of the time.
3. **CP-03/CP-05/CP-06 Weapons** — the core loop is combat.
4. **CP-02 Echo bodies** — one body plan at a time, Beasts first (starter zone density).
5. **CP-09 dialogue + CP-10 HUD polish** — feel and story.

## Status vocabulary (quality bar — never inflate)

`SPEC_READY` → spec authored, awaiting assets.
`SOURCE_IMPLEMENTED` → C++/data shipped, compile pending UE5 verification.
`SOURCE_TESTED` → automation tests cover the logic.
`UE5_VERIFIED` → Antigravity confirmed in engine (48/48 baseline is verified; new packs start unverified).
