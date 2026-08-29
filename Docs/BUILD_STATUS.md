# ASTRAWILD — Build Status (Updated 2026-08-29)

> Updated after user-reported `SharedPointer.h:1133` fix (`F`-prefix reflection in `Scripts/import_all_datatables.py`) and batch import re-run.

## Verified evidence (this session)

| Layer | Evidence | Path / Tool Output | Status |
|---|---|---|---|
| SharedPointer fix | `test_single_dt_import.py` executes; `import_all_datatables.py` runs to conclusion (38 queued, `LogPython: [ASTRAWILD][Import]`) | `Scripts/import_all_datatables.py` lines 104-197 (fixed `_load_row_struct` / `_to_script_struct_instance`) | ✅ PASS |
| DataTable import (disk reality) | 38 `.uasset` files (`DT_*.uasset`) present under `Content/Astrawild/Data/Imported/`; `DT_EchoDex_200.uasset` 946,689 B (largest); `DT_Recipes.uasset` 96,126 B | `Content/Astrawild/Data/Imported/` | ✅ 38/38 |
| DataTable import (report claim) | `DataTableImportReport.json`: `expected_count: 38`, `imported_count: 38`, `failed_count: 0` | `Saved/Astrawild/DataTableImportReport.json` | ✅ PASS |
| Generated assets | `GeneratedAssetImportReport.json`: `expected: 275`, `imported: 275`, `failed: 0`; meshes 233 (StaticMesh), audio 42 (SoundWave) | `Saved/Astrawild/` | ✅ PASS |
| Meshes (StaticMesh) | 9 Prop + 218 Echo + 2 Character + 4 MapKit = 233 `.uasset` under `Meshes/` | `Content/Astrawild/Meshes/` | ✅ PASS |
| Audio (SoundWave) | 31 SFX + 9 Ambience + 2 Music = 42 `.uasset` under `Audio/` | `Content/Astrawild/Audio/` | ✅ PASS |
| Map asset — Open World | `LV_DawnValley_OpenWorld.umap` present (8,516 B, timestamp 23:39); `LV_DawnValley_Main.umap` present (8,486 B) | `Content/Astrawild/Maps/` | ✅ PASS |
| Editor session log | `MAP LOAD FILE=...LV_DawnValley_OpenWorld.umap`, `MapCheck: 0 Error/0 Warning`, `LogAudio: registered with world 'LV_DawnValley_OpenWorld'` | `Saved/Logs/ASTRAWILD.log` (timestamp 05:31) | ✅ PASS |
| Module compile | `UnrealEditor-AstrawildCore.dll` present (compiled); `LogModuleManager` loads module; `LogPython: Python enabled` | `Binaries/Win64/` + log | ✅ PASS |
| Source contracts (38 CSV) | 13/13 validators PASS (`validate_content_contracts.py`, `validate_runtime_contracts.py`, `validate_generated_headers.py`, `validate_master_echodex.py`, `validate_character_map_assets.py`, `validate_audio_pack.py`, `validate_importer_coverage.py`, `validate_vertical_slice_guards.py`, `validate_mecha_contracts.py`, `validate_editor_automation.py`, `validate_handoff_contracts.py`, `validate_generated_assets.py`, `validate_vehicle_contracts.py`) | `Scripts/validate_*.py` | ✅ PASS |

---

## Corrected interpretation (previous false negative resolved)

Earlier `DataTableImportReport.json` showed `imported_count: 0`, `failed_count: 38` (cached from a pre-fix import session). After the user's `SharedPointer.h:1133` reflection fix and script re-run, the **disk reality** (verified independently via a separate Python verification script) shows all 38 `.uasset` DataTables present, with the report's latest values (`imported_count: 38`, `failed: 0`) matching disk state. The original `RuntimeError: DataTable import failed for 38 asset(s)` was resolved by the user's fix; the 0/38 reading was from a pre-fix report, not the current session.
