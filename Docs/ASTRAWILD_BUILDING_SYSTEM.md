# ASTRAWILD — Building System

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-29**
**Primary sources:** `AstrawildBuildingComponent.h/.cpp`, `AstrawildBuildingActor.h/.cpp`,
`AstrawildPowerSubsystem.h/.cpp`, `AstrawildContentLibrary.cpp::BuildBuildings()`

Modular base building with a client-side preview UX and **server-authoritative placement**. v1 scope is a
simplified grid-snap model (see Assumptions doc): one piece at a time, ghost preview, rotation in 15° steps.

---

## 1. Placement Flow (Controls)

| Key | Action | Code path |
|---|---|---|
| **B** | Toggle placement mode | `PlayerCharacter::ToggleBuildMode` → `BuildingComponent::TogglePlacementMode` |
| **N** | Rotate preview +15° | `PlayerCharacter::RotateBuilding` → `RotatePreview(15.0f)` |
| **LMB** | Confirm placement (while placing) | `Attack` input routes to `ConfirmPlacement()` instead of a light attack while `IsPlacing()` |
| **B** (again) | Cancel | `CancelPlacement()` destroys the ghost |

`CycleBuildingDefinition()` cycles through the unlocked building list (bound to B-toggle rebuild; the
cycle call exists on the component — a dedicated "cycle piece" key is NOT yet bound; entering/exiting
placement mode refreshes the unlocked list).

### Placement mode entry

1. Rebuild the unlocked list from the registry (`GetUnlockedBuildings` — tech gate `RequiredTechId`).
2. Refuse (and stay out of placement) when nothing is unlocked.
3. Spawn a **preview ghost**: an `AAstrawildBuildingActor` with collision disabled on actor + mesh,
   `SetRenderCustomDepth(true)` for a silhouette-style ghost. Not replicated — owner-only visual.
4. Broadcast `OnPlacementModeChanged(true)`.

---

## 2. Preview Update (component tick, while placing)

- **Target point**: `player location + forward × PlacementReach (600 cm)`.
- **Grid snap**: X/Y snapped with `FMath::GridSnap(_, SnapGridSize = 200 cm)`; Z ground-aligned via a
  ±500 cm line trace on `ECC_Visibility`.
- **Ghost transform**: location + rotation `(0, PreviewYaw, 0)`.
- **Silhouette scale** by category: Foundation `(2, 2, 0.2)`, Wall `(2, 0.2, 1.5)`, other `(1.2, 1.2, 1.0)`.
- **Validity** = has materials (`Inventory->HasItem(RequiredItemId, RequiredItemCount)`) **AND**
  `ValidatePlacementLocation` passes. Exposed as `IsPlacementValid()` for future ghost tinting (red/green
  visual feedback NOT IMPLEMENTED).

### Validation (overlap + geometry)

`ValidatePlacementLocation(Location, GridSize)`:
- Box overlap test, half-extents `(GridSize × 0.45, GridSize × 0.45, 50)` centered 50 cm above the location,
  channel `ECC_WorldStatic`.
- Valid when **nothing is hit** (no existing blocking geometry in the cell).

---

## 3. Confirm → Server Placement

```
ConfirmPlacement (client, only when valid)
  ├─ consume materials locally: Inventory->ConsumeItems({RequiredItem × count})
  └─ ServerPlaceBuilding(DefId, Location, Yaw)          [Server, Reliable]
        ├─ resolve definition from registry
        ├─ RE-VALIDATE overlap at Def->GridCellSize      (never trust the client)
        │     └─ invalid → REFUND materials (AddItem back) and abort
        ├─ spawn AAstrawildBuildingActor at Location/Yaw
        ├─ Building->InitializeFromDefinition(Def, Player->GetFName())
        ├─ OnBuildingPlaced broadcast
        └─ EventBus: Event.BuildingPlaced (quest progress — Homeground/Spark)
```

Materials are consumed client-side before the RPC and **refunded server-side** if validation fails —
the honest v1 trade-off for not double-paying round-trip inventory checks (documented in Assumptions).

---

## 4. BuildingActor Lifecycle

| Phase | Behavior |
|---|---|
| Spawn | `BuildingId = NewGuid()`; cube placeholder mesh; `InitializeFromDefinition` sets `DefinitionId`, `OwnerPlayerId`, `MaxHealth` from definition, category silhouette scale, power registration |
| Power registration | Generator / Battery / Consumer-with-draw registers with `UAstrawildPowerSubsystem` (structural pieces skip); unregisters in `EndPlay` |
| Runtime | `bIsSwitchedOn` (replicated, default true), `CurrentHealth` (replicated), `StoredCharge` (replicated); `ApplyBuildingDamage` (server) reduces health, `OnBuildingDamaged` broadcast, `Destroy()` at 0 |
| Save | `ToSaveData()` → `FAstrawildBuildingSaveData` (id, definition id, transform, health, charge, switch, owner); `FromSaveData()` restores and re-initializes (LoadWorld destroys all placed buildings then respawns from data) |

Ownership: `OwnerPlayerId` (FName of the placing player) is stored and replicated through save data; no
ownership-based permission enforcement yet (single-player-first; co-op rules PLANNED).

---

## 5. The 9 CODE_DEFAULT Buildings

From `AstrawildContentLibrary.cpp::BuildBuildings()`:

| Building | DefinitionId | Category | Cost (item × qty) | Tech gate | Health | Power role | Gen / Draw / Battery | Work type |
|---|---|---|---|---|---|---|---|---|
| Foundation | `Building_Foundation` | Foundation | Wood ×4 | — | 800 | Consumer | 0 / 0 / 0 | None |
| Wall | `Building_Wall` | Wall | Wood ×2 | — | 500 | Consumer | 0 / 0 / 0 | None |
| Workbench | `Building_Workbench` | Workstation | Wood ×8 | — | 400 | Consumer | 0 / 0 / 0 | Crafting |
| Campfire | `Building_Campfire` | Workstation | Wood ×5 | — | 300 | Consumer | 0 / 0 / 0 | Cooking |
| Echo Dynamo | `Building_Generator` | Power | Stone ×10 | `Tech_Electrical` | 600 | **Generator** | **8.0** / 0 / 0 | PowerGeneration |
| Charge Cell | `Building_Battery` | Power | Stone ×8 | `Tech_Electrical` | 400 | **Battery** | 0 / 0 / **600** | None |
| Dawn Lamp | `Building_LampPost` | Decoration | Wood ×3 | `Tech_Electrical` | 200 | Consumer | 0 / **2.0** / 0 | None |
| Farm Plot | `Building_FarmPlot` | Farm | Wood ×6 | — | 250 | Consumer | 0 / 0 / 0 | Farming |
| Research Desk | `Building_ResearchDesk` | Research | Wood ×6 | — | 350 | Consumer | 0 / **1.0** / 0 | ResearchAssist |

All buildings share `GridCellSize = 200` (definition default). All visuals are scaled engine cubes —
PLACEHOLDER / REPLACE_BEFORE_RELEASE.

Note: Building placement requires the *item* cost (Wood/Stone); the Workbench/Campfire used for crafting
gating are spawned by the WorldBootstrapper at camp — the placed variants are identical definitions.

---

## 6. Recipes & Building Integration

- Recipes do **not** consume buildings; stations are proximity gates (see Crafting doc).
- Tech gating: `GetUnlockedBuildings` filters by `RequiredTechId` through the Research subsystem —
  Generator/Battery/Lamp require `Tech_Electrical`.

---

## 7. Power Grid (summary)

`UAstrawildPowerSubsystem` (server, tickable):

- Buildings auto-connect into **one shared grid** by proximity (`ConnectivityRadius = 1200 cm`).
- **Re-solve every `ResolveIntervalSeconds = 2.0 s`** — never per frame.
- Energy storage: `StoredEnergy` accumulates `NetFlow` per second, clamped to total battery capacity
  (e.g. one Charge Cell = 600).
- **Brownout rule**: available = stored + generation; consumers sorted by priority
  **Research (0) > Workstation (1) > Farm (2) > Defense (3) > Decoration/others (4)**; when demand exceeds
  supply the lowest-priority consumers lose power first.
- `OnGridChanged(TotalGeneration, TotalDraw)` and `OnPowerStateChanged(bGridPowered)` broadcasts for UI.
- Work sites with `bRequiresPower` produce ×1.5 when powered, ×0 when required and unpowered;
  `IsLocationPowered` currently resolves to "within 1200 cm of any generator" (simplified shared grid v1).
- State is logged via `LogAstrawildBuilding`; `ASTRAWILD_TEST_PLAN.md` includes the brownout math test.

---

## 8. Not Implemented (honest)

| Feature | Status |
|---|---|
| Structural integrity / snapping to other pieces | NOT IMPLEMENTED (free grid placement only) |
| Deconstruct / move / refund of placed buildings | NOT IMPLEMENTED |
| Ghost validity color (red/green) | NOT IMPLEMENTED (`IsPlacementValid()` API exists) |
| Dedicated cycle-piece keybind | NOT IMPLEMENTED (cycle API exists, not bound) |
| Per-player building permissions in co-op | PLANNED |
| Real meshes / materials | PLACEHOLDER engine cubes |
