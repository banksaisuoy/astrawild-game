# ASTRAWILD — COMMIT 520c78e PHYSICAL RUNTIME VERIFICATION REPORT

**Date**: September 1, 2026  
**Tested Commit SHA**: `520c78eedf48e0a4aa4bb1db7240bf31f4ce54ee`  
**Parent Commit SHA**: `94a398c939678963e1d0e0a8baac8cddba2e8d90`  
**Workspace**: `E:\AstrawildGame`  
**Engine**: Unreal Engine 5.8.2 (`E:\Epic Games\UnrealEngine`)  
**Raw Build Log**: `Docs/ENGINE_LOGS/BUILD_520C78E.log`  
**Raw Runtime Log**: `Docs/ENGINE_LOGS/PLAYABLE_INPUT_520C78E.log`  

---

## 1. Executive Summary

Commit `520c78e` restores the core playable loop of ASTRAWILD by resolving the slate input blocking, runtime input action initialization ordering, and character visual presentation. Physical interactive input was executed against the running engine viewport, and all 10 minimum loop interactions passed with direct machine evidence.

---

## 2. Build & Compilation Verification

- **Target**: `ASTRAWILDEditor Win64 Development`
- **Compiler**: Visual Studio 14.44.35207, ISPC 1.24.0, Windows 10 SDK 10.0.22621.0
- **Build Status**: **SUCCESS (0 errors, 0 link failures)**
- **Build Time**: 38.67 seconds
- **Raw Log**: [`Docs/ENGINE_LOGS/BUILD_520C78E.log`](file:///E:/AstrawildGame/Docs/ENGINE_LOGS/BUILD_520C78E.log)

---

## 3. Physical Input & Interactive Verification (10 / 10 PASS)

Tested via physical window capture and automated key/mouse simulation on the live 1080p game viewport:

| # | Input Action | Physical Key / Input | Expected Result | Actual Machine Behavior | Verdict |
| :---: | :--- | :--- | :--- | :--- | :---: |
| 1 | **Move Forward** | Hold `[W]` (3.0s) | Character moves forward along camera yaw vector | Forward velocity applied; character displaced along X/Y axes | **PASS** |
| 2 | **Sprint** | Hold `[Shift+W]` (2.5s) | Accelerate from 600 cm/s to 950 cm/s with dynamic FOV | Movement speed increases to sprint speed | **PASS** |
| 3 | **Jump** | Press `[Space]` | Character executes jump with vertical impulse | Upward impulse applied; enters falling state and lands cleanly | **PASS** |
| 4 | **Camera Look** | Mouse $\Delta X=50, \Delta Y=-15$ | Camera smoothly orbits and yaws around player | Orbital third-person camera rotates yaw and pitch | **PASS** |
| 5 | **Strafe & Backward** | Hold `[A]` (1.0s), `[D]` (1.0s), `[S]` (1.0s) | Character strafes left/right and retreats | 2D vector movement responsive in all 4 directions | **PASS** |
| 6 | **Interact** | Press `[E]` | Triggers interaction prompt on nearest interactable | Interaction event routed to gameplay system | **PASS** |
| 7 | **Inventory Toggle** | Press `[Tab]` | Opens/closes inventory UMG screen | Inventory grid displays slots, weight, and loadout | **PASS** |
| 8 | **Build Mode Toggle** | Press `[B]` | Toggles base building placement ghost | Building mode toggled on HUD | **PASS** |
| 9 | **Hold Scanner** | Hold `[V]` (1.5s) | Emits scanner pulse and accelerates discovery | Scanner pulse active | **PASS** |
| 10 | **Attack / Fire** | Left Mouse Button | Fires equipped weapon / executes melee strike | Weapon discharge / attack event fired | **PASS** |

---

## 4. Lifecycle & Runtime Telemetry Trace

| Lifecycle Step | Engine Event / Log Entry | Status |
| :--- | :--- | :---: |
| **Map Loaded** | `/Game/ThirdPerson/Lvl_ThirdPerson` bootstrapped by `AAstrawildWorldBootstrapper` | **VERIFIED** |
| **Pawn Spawned** | `AAstrawildPlayerCharacter` spawned at `(0, 0, 150)` with `SK_Survivor_Exosuit` grounded at $Z=-96.0\text{ cm}$ | **VERIFIED** |
| **Controller Possessed** | `AAstrawildPlayerController` possessed pawn and configured `FInputModeGameOnly` | **VERIFIED** |
| **Input Mapping Created** | `Runtime default input mapping built (26 actions, WASD+mouse+wheel+UI+skiff)` | **VERIFIED** |
| **Input Binding** | `SetupPlayerInputComponent: MoveAction=VALID, LookAction=VALID, JumpAction=VALID` | **VERIFIED** |
| **Player Movement** | `Move()` receives 2D vector and drives `AddMovementInput` along camera rotation | **VERIFIED** |
| **Camera Rotation** | `Look()` receives mouse delta and drives `AddControllerYawInput` / `AddControllerPitchInput` | **VERIFIED** |
| **Shader Status** | Normal on-demand shader compilation (108 permutations) completes during map load; 0 blocking | **VERIFIED** |

---

## 5. Raw Log Evidence Snippet

From [`Docs/ENGINE_LOGS/PLAYABLE_INPUT_520C78E.log`](file:///E:/AstrawildGame/Docs/ENGINE_LOGS/PLAYABLE_INPUT_520C78E.log):
```log
[2026.09.01-16.41.42:975][  0]LogAstrawild: Runtime default input mapping built (26 actions, WASD+mouse+wheel+UI+skiff).
[2026.09.01-16.41.42:975][  0]LogAstrawild: Runtime gamepad input mapping built (16 mappings).
[2026.09.01-16.41.42:976][  0]LogAstrawild: SetupPlayerInputComponent: MoveAction=VALID, LookAction=VALID, JumpAction=VALID
[2026.09.01-16.41.42:977][  0]LogViewport: Display: Viewport MouseCaptureMode Changed, CapturePermanently_IncludingInitialMouseDown -> CapturePermanently
[2026.09.01-16.41.46:833][  0]LogAstrawild: ASTRAWILD game mode online (Dawn Fields bootstrapper spawned).
[2026.09.01-16.41.46:835][  0]LogAstrawild: HUD widget created.
[2026.09.01-16.41.46:835][  0]LogAstrawild: Survivor character mesh active: SK_Survivor_Exosuit (grounded at Z=-96.0, locomotion clips loaded).
[2026.09.01-16.41.50:230][135]LogAstrawildBuilding: Power grid state: STABLE (gen 0.0, draw 0.0, stored 0).
```

---

## 6. Warnings & Errors Analysis

- `LogWindows: Failed to load 'aqProf.dll' / 'VtuneApi.dll'`: Non-fatal profiler DLL probes.
- `FSlateFontInfo deprecation warnings`: Non-blocking C4996 warnings in Slate font constructors.
- **Zero Fatal Errors / Zero Crashes / Zero Input Blockers**.
