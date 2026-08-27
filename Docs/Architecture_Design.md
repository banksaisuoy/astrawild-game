# ASTRAWILD Architecture Design Specification

## 1. Core Principles & Technology Stack
- **Engine Target**: Unreal Engine 5.8
- **Primary C++ Module**: `AstrawildCore`
- **Memory & Performance Philosophy**:
  - Zero raw pointer ownership across frames; use `TObjectPtr<T>`, `TWeakObjectPtr<T>`, and UPROPERTY garbage collection handles.
  - Zero ticking where event-driven / timer-based updates suffice.
  - Cache traces, spatial queries, and avoid large per-frame heap allocations.
  - Target 60 FPS on mid-range hardware (Core i5 / Ryzen 5, RTX 3060 / RX 6600 class).

## 2. Separation of Concerns: C++ vs Blueprint / DataAsset
| Layer | Responsibilities | Technologies |
| :--- | :--- | :--- |
| **C++ Core (`AstrawildCore`)** | Gameplay rules, damage formulas, capture probability math, inventory transactions, serialization/deserialization, movement logic, AI controller base, component boundaries. | C++ classes, USTRUCTs, UENUMs, UFUNCTION(BlueprintCallable, BlueprintNativeEvent) |
| **Data Layer** | Specific species definitions, item stats, recipe ingredient tables, building costs, status effect timings. | `UDataAsset` (`UAstrawildEchoDataAsset`, `UAstrawildItemDataAsset`, `UAstrawildRecipeDataAsset`) |
| **Presentation / Blueprint Layer** | Animation Blueprints, particle/VFX spawns, sound cue triggers, HUD layout (UMG), material parameter updates. | Blueprints derived directly from C++ base classes. |

## 3. Stable Identifier (Stable ID) Strategy
Every gameplay entity has a persistent, unique Stable ID represented via `FGameplayTag` or `FName`:
- **Echoes**: `Echo.Pyrelite`, `Echo.Aquavine`, `Echo.Thornback`
- **Items**: `Item.Resource.Sunwood`, `Item.Resource.LumenStone`, `Item.Tool.AstraResonatorBasic`, etc.
- **Recipes**: `Recipe.Tool.ResonatorTier1`, `Recipe.Building.Campfire`, etc.
- **Abilities**: `Ability.Echo.FlameDart`, `Ability.Echo.BubbleCascade`, `Ability.Echo.QuillBarrage`
- **Buildings**: `Building.Campfire`, `Building.RestBed`, `Building.CraftingBench`, `Building.StorageChest`

## 4. Save / Load Persistence Contract
- Subsystem `UAstrawildSaveSubsystem` provides asynchronous slot reading/writing using `UAstrawildSaveGame`.
- State preserved across sessions:
  - Player: Transform, Current Health, Current Stamina, EXP, Level.
  - Inventory: Slot-based array containing Item Tag, Quantity, Durability, Metadata.
  - Echo Collection: Active party (up to 5 companions) + Reserve sanctuary storage.
  - Placed Structures: World transform, building piece ID, current durability, storage chest inventory contents.
  - World State: Harvested node respawn timestamps, day/night cycle game time.

## 5. Network & Co-op Scalability Roadmap
- Single-player vertical slice is authoritative on local client acting as standalone server/client (`ROLE_Authority`).
- Components decouple local presentation from state mutation:
  - State changes happen via authoritative functions (e.g. `Server_PerformAttack`, `CommitCraftingTransaction`).
  - Read-only queries expose clean const getters.
  - No single-player hacks that assume only 1 player controller exists.