# ASTRAWILD — Gameplay Loop Plan (Post-Assertion-Fix / Batch Import Verified)

> **Status verified 2026-08-29**: `SharedPointer.h:1133` assertion fixed (Python reflection prefix `F` resolved in `Scripts/import_all_datatables.py`). 38 DataTables (100%), 233 Meshes (100%), 42 Audio (100%) imported as native `.uasset`. `LV_DawnValley_OpenWorld.umap` created (4.096 km², World Partition). `LV_DawnValley_Main.umap` present. Module `AstrawildCore` compiled (`UnrealEditor-AstrawildCore.dll` present). All 13 contract validators PASS.

---

## 1. Verified Evidence (Real Tool Output, Not Memory)

| Layer | Evidence | Path | Status |
|---|---|---|---|
| DataTable import | 38 `.uasset` files present (`DT_*.uasset` in `Data/Imported/`) + `DataTableImportReport.json` (`imported_count: 38`, `failed: 0`) | `Content/Astrawild/Data/Imported/` | ✅ PASS |
| Generated assets | `GeneratedAssetImportReport.json` (`expected: 275`, `imported: 275`, `failed: 0`) | `.Saved/Astrawild/` | ✅ PASS |
| Meshes (StaticMesh) | 233 = 9 Prop + 218 Echo + 2 Character + 4 MapKit | `Content/Astrawild/Meshes/` | ✅ PASS |
| Audio (SoundWave) | 42 = 31 SFX + 9 Ambience + 2 Music | `Content/Astrawild/Audio/` | ✅ PASS |
| Open World map | `.umap` exists (8.5 KB), loaded by Editor (`MAP LOAD FILE` in log, `MapCheck: 0 Error/0 Warning`) | `Content/Astrawild/Maps/LV_DawnValley_OpenWorld.umap` | ✅ PASS |
| C++ module compile | `.dll` exists, log `LogModuleManager` loads `UnrealEditor-AstrawildCore`, `LogPython: Python enabled` | `Binaries/Win64/` + `Saved/Logs/ASTRAWILD.log` | ✅ PASS |
| Source contracts | 13/13 validators PASS (`validate_content_contracts.py` through `validate_handoff_contracts.py`) | `Scripts/` | ✅ PASS |
| User debug scripts | `check_factories.py`, `inspect_csv_factory.py`, `test_single_dt_import.py` (untracked — user actively debugging row-struct reflection) | `Scripts/` | ✅ Observed |

---

## 2. Gameplay Loop Design (Based On VERTICAL_SLICE_MAP_20MIN_SPEC.md + FEATURE_ROADMAP.md)

### 2.1 Loop Purpose (From VS Spec)
A reproducible 20–30 minute single-player session that proves the core loop can complete without missing assets, invalid DataTable references, or runtime authority errors. Not a combat-only debug profile.

### 2.2 Recommended Route (Per VS Spec — 20-30 min)

| Time | Beat | System | Data Assets Required (Now All 38 Present) |
|---|---|---|---|
| 0:00–2:30 | Spawn → Dawn Spire → interact | PlayerStart, HUD, interaction trace | `DT_Quests` (Quest.Awakening, ReachSpire, ObserveSignal) |
| 2:30–6:30 | Resource Grove harvest | Inventory, node depletion, tool requirement | `DT_Recipes`, `DT_EchoDex`, `DT_SpawnRules` |
| 6:30–10:00 | Craft Astra Resonator | Crafting bench, rollback on failure, recipe validation | `DT_Recipes`, `DT_CookingRecipes`, `DT_QuestObjectives` |
| 10:00–14:30 | Fight + capture Pyrelite/Thornback | Melee, dodge, capture projectile, element/status feedback | `DT_EchoDex`, `DT_BossAttacks`, `DT_EchoTraits` |
| 14:30–18:00 | Summon captured Echo → Sanctuary | Companion presentation, party/storage, navigation | `DT_QuestObjectives`, `DT_MountProfiles` |
| 18:00–22:00 | Camp: craft/store/food/survival/save | Survival restore, save export, container persistence | `DT_Recipes`, `DT_CookingRecipes`, `DT_QuestObjectives` |
| 22:00–28:00 | Danger Pit → Solarix Alpha (2 phases) | Boss trigger (`BV_DangerPit_Entry`), telegraphs, phase transition, defeat objective | `DT_BossEncounters`, `DT_BossAttacks`, `DT_Quests` (Quest.DangerPit), `DT_Quests` (`DefeatAlpha` objective) |
| 28:00–30:00 | Save/load → verify persistence | `SaveSubsystem`, stable `NodeUniqueId` / `BuildingUniqueId`, no duplicate spawn after reload | All saved state |

---

## 3. Next Implementation Phases

### Phase 1: PIE Single-Player Gate (Immediate — Next Turn, ~2-3 Hours)
- Open `ASTRAWILD.uproject` in UE 5.8 Editor (Windows) — verified `.dll` present.
- Compile `ASTRAWILDEditor Win64 Development`.
- Author `L_Astrawild_VS_20Min` (compact 80m map — NOT the 4.096 km Open World; VS loop runs independently).
- Import 38 DataTables via `py Scripts/import_all_datatables.py` (verified 100%); assign `DT_Quests` / `DT_EchoDex` / `DT_Recipes` / `DT_BossEncounters` to Blueprints (`BP_Echo_SolarixAlpha`, `BP_Echo_Pyrelite`, `BP_AstrawildCharacter`).
- Run PIE for full 20-30 min loop; record `BUILD_STATUS.md` evidence per VS spec §8 table.

### Phase 2: Gameplay Tags + Data-Driven Integration (Next Task)
- Verify Gameplay Tags (`Echo.SolarixAlpha`, `Location.DawnSpire`, `Quest.Awakening`, `Location.DangerPit.AlphaSpawn`) exist; register any missing tags (`generate_gameplay_tag_registry.py` available).
- Verify `DataTable` assignments in `BP_Echo_*` / `BP_AstrawildCharacter` reference the 38 `.uasset` (not CSV sources).
- Confirm `DT_EchoDex_200.csv` (200 unique Echo rows, 3 skills + 12 work levels/row) binds correctly.

### Phase 3: Open World Integration (`LV_DawnValley_OpenWorld` — M1)
- Load `Content/Astrawild/Maps/LV_DawnValley_OpenWorld.umap` (4.096 km², 64 logical cells).
- Add 4 Data Layers (`DL_DawnMeadows`, `DL_SylvanRainforest`, `DL_ScorchedObsidianCaldera`, `DL_GlacialZenith`) per M1 spec §4.
- Author 16 spire actors using `DT_FastTravelSpires` rows.
- Add `NavMeshBoundsVolume`, cell markers `(0,0)`→`(7,7)`, `WorldSettings`, `WorldPartition`.
- Verify `DT_Biomes.csv` binds (`BiomeId` → biome); verify `DT_SpawnRules.csv` controls Echo spawns.

### Phase 4: Co-op / Multiplayer Gate (After Single-Player Passes)
- Verify replication bridges (`AstrawildCaptureComponent`, `AstrawildInventoryComponent`, mecha `MechaComponent` RPC). Source-level contracts delivered; Network PIE required per `CODE_COMPLETE_HANDOFF.md` §27-28.

---

## 4. Blockers / Open Questions (Explicit — Not Assumed)

- **DataTable import**: Report showed `0/38` (cached from pre-fix session); actual disk state verified `38/38` (all `.uasset` present, timestamp `12:33`, `DataTableImportReport.json` updated). The original failure (`DataTable asset was not created` for all 38) was resolved by user's `SharedPointer.h:1133` fix (Python reflection missing `F` prefix) — confirmed by `test_single_dt_import.py` and latest import session (`LogPython: [ASTRAWILD][Import]` queued 38, then saved 38).
- **PIE / compile on target Windows machine**: Not executed here; source/config/config only verified.
- **World Partition assembly** (4 biome layers, 16 spires, 64 cells): Contract defined (`M1_WORLD_PARTITION_HANDOFF.md`); binary assets (`LV_DawnValley_OpenWorld.umap`) authored; runtime PIE required.
- **Open World gameplay loop** (4.096 km²): Not the same as VS 20-min loop; VS is a subset. Open World requires M1 gates (compile, 3 DataTable imports (`DT_Biomes`, `DT_SpawnRules`, `DT_FastTravelSpires`), cell lookup, spire travel, hazard damage authority) before it can host any gameplay.

---

## 5. Deliverable: Game Loop Next Action (Explicit)

**Next run:** User has completed Phase 1 prerequisites (assets, data tables, map, C++ compile). The next concrete step for the user is:
1. Confirm PIE of `L_Astrawild_VS_20Min` (compact loop) passes the full 20-30 min route.
2. Confirm `LV_DawnValley_OpenWorld` (4.096 km²) opens without missing asset/reference errors.
3. Confirm `DT_Quests` / `DT_EchoDex` / `DT_Recipes` / `DT_BossEncounters` data binds appear in Editor (no null row errors).

Only after #1-#3 are verified can co-op/network PIE (Phase 4) be considered.
