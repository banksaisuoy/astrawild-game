"""Import all ASTRAWILD CSV source tables as Unreal DataTable assets.

Run inside the Unreal Editor Python console or with:
    py "Scripts/import_all_datatables.py"

The script is intentionally conservative: it imports only the explicitly mapped
CSV files, uses native reflected row structs, replaces existing DataTables, saves
them, and writes an import report under Saved/Astrawild/.
"""
from __future__ import annotations

import json
import os
from pathlib import Path

import unreal


DESTINATION_PATH = "/Game/Astrawild/Data/Imported"

# The names are the actual reflected C++ UStruct names in AstrawildCore. Keep this
# mapping explicit so a newly added CSV cannot silently import with the wrong row type.
TABLE_MAPPING = {
    "DT_Biomes.csv": "FAstrawildBiomeDefinition",
    "DT_BossAttacks.csv": "FAstrawildBossAttackRow",
    "DT_BossEncounters.csv": "FAstrawildBossEncounterRow",
    "DT_BreedingGroups.csv": "FAstrawildBreedingGroupRow",
    "DT_CyberneticEvolutions.csv": "FAstrawildCyberneticEvolutionRow",
    "DT_Dungeons.csv": "FAstrawildDungeonRow",
    "DT_EchoDex.csv": "FAstrawildEchoDexRow",
    "DT_EchoTraits.csv": "FAstrawildEchoTraitRow",
    "DT_Evolutions.csv": "FAstrawildEvolutionRow",
    "DT_FoliageRules.csv": "FAstrawildFoliageRuleRow",
    "DT_FastTravelSpires.csv": "FAstrawildFastTravelSpire",
    "DT_Lore.csv": "FAstrawildLoreRow",
    "DT_MechaFrames.csv": "FAstrawildMechaFrameRow",
    "DT_MechaWeapons.csv": "FAstrawildMechaWeaponRow",
    "DT_MountProfiles.csv": "FAstrawildMountProfile",
    "DT_QuestObjectives.csv": "FAstrawildQuestObjectiveRow",
    "DT_Quests.csv": "FAstrawildQuestRow",
    "DT_RangedWeapons.csv": "FAstrawildRangedWeaponRow",
    "DT_Recipes.csv": "FAstrawildCraftingRecipeRow",
    "DT_SpawnRules.csv": "FAstrawildWorldSpawnRule",
    "DT_TechnologyNodes.csv": "FAstrawildTechnologyNodeRow",
    "DT_Weather.csv": "FAstrawildWeatherRow",
}


def _log(message: str) -> None:
    unreal.log(f"[ASTRAWILD][DataTables] {message}")


def _log_warning(message: str) -> None:
    unreal.log_warning(f"[ASTRAWILD][DataTables] {message}")


def _load_row_struct(struct_name: str):
    object_path = f"/Script/AstrawildCore.{struct_name}"
    row_struct = unreal.load_object(None, object_path)
    if row_struct is None:
        row_struct = unreal.find_object(None, struct_name)
    if row_struct is None:
        raise RuntimeError(f"Cannot find reflected row struct: {object_path}")
    return row_struct


def _set_property(object_or_class, property_name: str, value) -> None:
    try:
        object_or_class.set_editor_property(property_name, value)
    except Exception:
        setattr(object_or_class, property_name, value)


def _make_import_task(source_file: Path, destination_name: str, row_struct):
    factory = unreal.DataTableFactory()
    _set_property(factory, "struct", row_struct)

    task = unreal.AssetImportTask()
    _set_property(task, "filename", str(source_file))
    _set_property(task, "destination_path", DESTINATION_PATH)
    _set_property(task, "destination_name", destination_name)
    _set_property(task, "factory", factory)
    _set_property(task, "automated", True)
    _set_property(task, "replace_existing", True)
    _set_property(task, "save", True)
    return task


def _write_report(project_dir: Path, report: dict) -> Path:
    report_path = project_dir / "Saved" / "Astrawild" / "DataTableImportReport.json"
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    return report_path


def import_all_datatables() -> dict:
    project_dir = Path(unreal.Paths.project_dir())
    source_dir = project_dir / "Content" / "Astrawild" / "Data" / "Source"
    if not source_dir.is_dir():
        raise RuntimeError(f"CSV source directory not found: {source_dir}")

    if not unreal.EditorAssetLibrary.does_directory_exist(DESTINATION_PATH):
        unreal.EditorAssetLibrary.make_directory(DESTINATION_PATH)

    missing_csv = sorted(name for name in TABLE_MAPPING if not (source_dir / name).is_file())
    if missing_csv:
        raise RuntimeError("Missing mapped CSV files: " + ", ".join(missing_csv))

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    tasks = []
    task_metadata = []
    for csv_name, struct_name in sorted(TABLE_MAPPING.items()):
        source_file = source_dir / csv_name
        row_struct = _load_row_struct(struct_name)
        asset_name = Path(csv_name).stem
        tasks.append(_make_import_task(source_file, asset_name, row_struct))
        task_metadata.append({
            "csv": csv_name,
            "row_struct": struct_name,
            "asset": f"{DESTINATION_PATH}/{asset_name}",
        })
        _log(f"Queued {csv_name} -> {struct_name}")

    asset_tools.import_asset_tasks(tasks)

    imported = []
    failed = []
    for item in task_metadata:
        asset_path = item["asset"]
        if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
            imported.append(item)
            unreal.EditorAssetLibrary.save_asset(asset_path, only_if_is_dirty=False)
        else:
            failed.append(item)
            _log_warning(f"DataTable asset was not created: {asset_path}")

    report = {
        "project_dir": str(project_dir),
        "source_dir": str(source_dir),
        "destination_path": DESTINATION_PATH,
        "expected_count": len(TABLE_MAPPING),
        "imported_count": len(imported),
        "failed_count": len(failed),
        "imported": imported,
        "failed": failed,
    }
    report_path = _write_report(project_dir, report)
    _log(f"Completed: {len(imported)}/{len(TABLE_MAPPING)} DataTables imported")
    _log(f"Report: {report_path}")

    if failed:
        raise RuntimeError(f"DataTable import failed for {len(failed)} asset(s); see {report_path}")
    return report


if __name__ == "__main__":
    import_all_datatables()
