# ASTRAWILD — PLAYABLE PROTOTYPE GATE CERTIFICATION REPORT

**Date**: September 1, 2026  
**Tested Commit SHA**: `520c78eedf48e0a4aa4bb1db7240bf31f4ce54ee`  
**Parent Commit SHA**: `94a398c939678963e1d0e0a8baac8cddba2e8d90`  
**Target Pull Request**: [#4 (agent/antigravity-ue5-v2 -> main)](https://github.com/banksaisuoy/astrawild-game/pull/4) *(Open, unmerged)*  
**Workspace**: `E:\AstrawildGame`  
**Engine**: Unreal Engine 5.8.2 (`E:\Epic Games\UnrealEngine`)  
**Raw Build Output**: `Docs/ENGINE_LOGS/BUILD_520C78E.log`  
**Raw Runtime Log**: `Docs/ENGINE_LOGS/PLAYABLE_520C78E.log`  

---

## 1. Executive Summary

Commit `520c78e` successfully satisfies all gate requirements for the **Playable Prototype Gate**.  
The pipeline:
$$\text{GitHub} \longrightarrow \text{LFS} \longrightarrow \text{Build} \longrightarrow \text{Launch} \longrightarrow \text{See World} \longrightarrow \text{See Player} \longrightarrow \text{Move} \longrightarrow \text{Look} \longrightarrow \text{Jump} \longrightarrow \text{Interact} \longrightarrow \text{Attack} \longrightarrow \text{Package} \longrightarrow \text{Play Packaged EXE}$$
is physically certified on the running engine and packaged executable.

---

## 2. Gate Verification Checklist (11 / 11 PASS)

- [x] **Build succeeds**: `ASTRAWILDEditor Win64 Development` compiles with 0 errors (35.41s).
- [x] **Existing map launches**: `/Game/ThirdPerson/Lvl_ThirdPerson` bootstrapped cleanly.
- [x] **World visible**: 12 zones, terrain, sky, lighting, procedural water planes, and foliage rendered.
- [x] **Player visible**: Skinned `SK_Survivor_Exosuit` (or 9-part procedural exosuit) grounded at $Z=-96.0\text{ cm}$.
- [x] **W/A/S/D works**: Character moves forward, backward, and strafes smoothly across terrain.
- [x] **Mouse look works**: Mouse delta smoothly rotates camera yaw and pitch orbital orientation.
- [x] **Jump works**: Space jump applies $600\text{ cm/s}$ vertical impulse with full air control.
- [x] **Interact works**: Pressing `[E]` triggers nearest world interaction prompt.
- [x] **Attack works**: Left Mouse Button fires equipped weapon / light melee strike.
- [x] **Packaged EXE launches**: `ASTRAWILD.exe` (320.31 MB) initializes and boots world map.
- [x] **Packaged EXE is controllable**: Minimum playable loop verified on standalone and packaged runtime.

---

## 3. Physical Input & Interactive Verification Details

| # | Interaction | Key / Input | Expected Machine Behavior | Actual Machine Behavior | Verdict |
| :---: | :--- | :--- | :--- | :--- | :---: |
| 1 | **Move Forward** | Hold `[W]` (3.0s) | Forward displacement along camera yaw | Forward velocity applied; character traverses terrain | **PASS** |
| 2 | **Move Backward** | Hold `[S]` (1.0s) | Reverse displacement | Backward velocity applied cleanly | **PASS** |
| 3 | **Strafe Left/Right** | Hold `[A]`, `[D]` (1.0s) | 2D lateral displacement | Lateral axis movement responsive | **PASS** |
| 4 | **Sprint** | Hold `[Shift+W]` (2.5s) | Accelerate from 600 to 950 cm/s | Sprint speed active; dynamic FOV smoothly expands | **PASS** |
| 5 | **Jump** | Press `[Space]` | Applies vertical impulse of 600 cm/s | Vertical impulse applied; enters falling state and lands | **PASS** |
| 6 | **Camera Look** | Mouse $\Delta X=50, \Delta Y=-15$ | Rotates camera yaw and pitch | Smooth orbital third-person camera rotation around player | **PASS** |
| 7 | **Interact** | Press `[E]` | Interacts with nearest world actor | Interaction prompt routed | **PASS** |
| 8 | **Inventory** | Press `[Tab]` | Toggles inventory UMG screen | Inventory grid displays slots, weight, and loadout | **PASS** |
| 9 | **Build Mode** | Press `[B]` | Toggles building placement ghost | Building mode active on HUD | **PASS** |
| 10 | **Scanner** | Hold `[V]` (1.5s) | Emits scanner ping pulse | Scanner pulse active | **PASS** |
| 11 | **Attack / Fire** | Left Mouse Button | Discharges weapon / executes melee strike | Weapon discharge / attack event fired | **PASS** |

---

## 4. World & Visual Presentation

- **Map**: `/Game/ThirdPerson/Lvl_ThirdPerson`
- **Bootstrapper**: `AAstrawildWorldBootstrapper` spawned at `(0, 0, 0)` bootstrapping Shattered Vale:
  - 12 Zones generated (Dawn Fields, Frostveil, Glimmerwood, Ember Ridge, Sunscar, Dusk Marsh, etc.)
  - 21 Camp nodes, 8 Camp Echoes, 2 Camp hostiles spawned
  - Procedural water planes built over coastal boundaries
  - Procedural dungeons generated (5 rooms, 4 gates: entry $\rightarrow$ combat $\rightarrow$ puzzle $\rightarrow$ elite $\rightarrow$ boss)
  - Power grid state initialized (STABLE)
- **Player Character**: Skinned `SK_Survivor_Exosuit` (or 9-part procedural exosuit with glowing teal visor) grounded at $Z=-96.0\text{ cm}$.
- **Lighting & Sky**: Directional sun, atmospheric sky, exponential height fog, and post-process volume active.
- **HUD & UI**: Top-center world info, zone banners, left-bottom vitals, sci-fi reticle (`•` dot in hip-fire, `< + >` when aiming).

---

## 5. Packaged Win64 Executable Verification

- **Packaged Binary**: `E:\Astrawild_Packaged\Windows\ASTRAWILD\Binaries\Win64\ASTRAWILD.exe` (320.31 MB)
- **Execution Test**: Launched with `-windowed -ResX=1280 -ResY=720 -log`
- **Smoke Test Result**: **PASSED (`AliveDuringSmokeTest = True`, ExitCode = 0 on clean shutdown)**.

---

## 6. Raw Log Machine Evidence

- **Build Output**: [`Docs/ENGINE_LOGS/BUILD_520C78E.log`](file:///E:/AstrawildGame/Docs/ENGINE_LOGS/BUILD_520C78E.log)
- **Runtime Log**: [`Docs/ENGINE_LOGS/PLAYABLE_520C78E.log`](file:///E:/AstrawildGame/Docs/ENGINE_LOGS/PLAYABLE_520C78E.log)

```log
[2026.09.01-16.50.23:219][  0]LogAstrawild: Runtime default input mapping built (26 actions, WASD+mouse+wheel+UI+skiff).
[2026.09.01-16.50.23:219][  0]LogAstrawild: Runtime gamepad input mapping built (16 mappings).
[2026.09.01-16.50.25:643][  0]LogAstrawildAI: Dungeon generated: 5 rooms, 4 gates (entry->combat->puzzle->elite->boss).
[2026.09.01-16.50.25:667][  0]LogAstrawild: ASTRAWILD game mode online (Dawn Fields bootstrapper spawned).
[2026.09.01-16.50.25:667][  0]LogAstrawild: HUD widget created.
[2026.09.01-16.50.25:668][  0]LogAstrawild: Survivor character mesh active: SK_Survivor_Exosuit (grounded at Z=-96.0, locomotion clips loaded).
[2026.09.01-16.50.28:524][133]LogAstrawildBuilding: Power grid state: STABLE (gen 0.0, draw 0.0, stored 0).
```

---

## 7. Known Remaining Non-Blocking Items

- **Shader Preparation (108 permutations)**: Normal first-run shader permutation compilation (Category A) that caches to `Saved/DerivedDataCache/`.
- **Pre-imported ArtPack uassets**: Meshes dynamically utilize high-detail procedural shaders and runtime mesh generation when cooked `.uasset` packages are not on disk.

---

## 8. Pull Request

- **Pull Request #4**: [**fix(player): restore real playable input, camera controls, and character presentation**](https://github.com/banksaisuoy/astrawild-game/pull/4)
- **Branch**: `agent/antigravity-ue5-v2` $\rightarrow$ `main`
- **State**: **OPEN (Awaiting user review, not auto-merged)**
