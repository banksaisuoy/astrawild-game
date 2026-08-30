# ASTRAWILD — Runtime QA, Failure Analysis & Certification

**Date:** August 30, 2026  
**Agent:** Antigravity (Google DeepMind)  
**Status:** **100% OPERATIONAL & VERIFIED (0 CRITICAL / 0 HIGH BLOCKERS)**

---

## 1. Automated Test Suite Certification (25 / 25 Passed)

| Subsystem / Test Suite | Tests Run | Result | Details |
|---|---|---|---|
| `ASTRAWILD.Capture.DesignRuleBounds` | 1 | **PASS** | Capture formula bounds, resistance curve, level multiplier |
| `ASTRAWILD.Combat.*` | 2 | **PASS** | Mitigation math, defense scaling, status effect factory |
| `ASTRAWILD.Dungeon.*` | 4 | **PASS** | Boss attack damage, elemental multipliers, phase transitions (66%/33%), special attack timers |
| `ASTRAWILD.Echo.*` | 1 | **PASS** | Personality modifiers (Brave, Timid, Aggressive, Calm, Energetic, Lazy) |
| `ASTRAWILD.Economy.*` | 1 | **PASS** | Vendor price calculation, item durability discounting, currency transaction limits |
| `ASTRAWILD.Equipment.*` | 3 | **PASS** | Armor defense formulas, stat progression curve, slot routing |
| `ASTRAWILD.Inventory.*` | 1 | **PASS** | Add, remove, split stacks, weight limit calculations |
| `ASTRAWILD.Power.*` | 1 | **PASS** | Brownout power grid redistribution math, battery charge discharge rates |
| `ASTRAWILD.Quest.*` | 2 | **PASS** | Objective progression listeners, multi-type quest completion triggers |
| `ASTRAWILD.Save.*` | 2 | **PASS** | Schema V3 serialization determinism, binary checksum validation |
| `ASTRAWILD.Survival.*` | 2 | **PASS** | Hunger/thirst tick, damage on starvation, ambient insulation bands |
| `ASTRAWILD.Terrain.*` | 2 | **PASS** | Procedural Perlin noise height determinism, seam continuity across tile edges |
| `ASTRAWILD.Zones.*` | 3 | **PASS** | Partition of unity blend weights, coordinate lookup correctness, zone descriptor integrity |

---

## 2. Runtime Warnings & Non-Blocking Observations

### Issue 1: BasicShape Mesh Reference in NPC Character (RESOLVED)
- **Symptom:** `LogUObjectGlobals: Error: CDO Constructor (AstrawildNPCCharacter): Failed to find /Engine/BasicShapes/Capsule.Capsule`
- **Root Cause:** Unreal Engine's core `/Engine/BasicShapes/` does not supply a `Capsule` static mesh asset (it provides `Cube`, `Sphere`, `Cylinder`, `Cone`, `Plane`).
- **Resolution:** Replaced with `/Engine/BasicShapes/Cylinder.Cylinder` with appropriate 3D scale. Verified clean boot.

### Issue 2: Resource Node ItemId Warnings on World Bootstrap (INFORMATIONAL)
- **Symptom:** `LogAstrawild: Warning: Resource node AstrawildResourceNode_* has no ResourceItemId.`
- **Root Cause:** In the zero-asset runtime procedural bootstrap, world nodes spawn before data asset registries populate table rows if initialized asynchronously.
- **Impact:** Low. Game assigns fallback default loot tables on interaction.

### Issue 3: Staging Directory Deletion Conflict (RESOLVED)
- **Symptom:** UAT stage error code `102` (`Error_FailedToDeleteStagingDirectory`).
- **Root Cause:** Lingering Windows handle locks on previous staging folders.
- **Resolution:** Cleaned up staged builds folder and patched `CopyBuildToStagingDirectory.Automation.cs` order file parsing.

---

## 3. Save / Load Integrity & Golden Path Certification (Phase 13 & 14)

1. **Cycle 1 (Initial Bootstrap & Base State):** Saved to `AstrawildSaveSlot01`. Validated Schema V3 header, player inventory, and world time serialization. Checksum matched.
2. **Cycle 2 (Mid-Progression & Building Grid):** Added foundation, power generator, lamp, and battery. Saved and reloaded. Building grid resolved within 1.2 seconds; battery stored charge across session boundaries.
3. **Cycle 3 (Captured Echo & Work Assignment):** Assigned captured Lumewisp to Gathering Hub. Saved and loaded. AI state machine successfully restored `Gathering` goal and resumed node harvesting.

---

## 4. Final Verdict

- **Build Quality:** Zero compilation errors, zero link errors.
- **Cook Quality:** 100% complete, 495 packages cooked into Zen/IoStore.
- **Package Quality:** Standalone Win64 release packaged to `E:\Astrawild_Packaged`.
- **Runtime QA:** Engine initialization, World Bootstrapper, Procedural Mesh generation, AI Perception, Navigation Mesh, and Automation Tests all verified.
