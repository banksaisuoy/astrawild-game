"""Import ASTRAWILD source DataTables and generated OBJ/WAV source assets.

Run inside the Unreal Editor Python console:
    py "Scripts/import_all_datatables.py"

The script uses explicit native row structs for every CSV and explicit asset
mappings for generated OBJ props and WAV SFX. It replaces/saves imported assets,
then writes JSON reports under Saved/Astrawild/. The script never claims that a
DataAsset, material graph, Sound Cue, or Blueprint has been authored unless the
Editor reports the asset path as created.
"""
from __future__ import annotations

import json
from pathlib import Path

import unreal


DESTINATION_PATH = "/Game/Astrawild/Data/Imported"
MESH_DESTINATION_PATH = "/Game/Astrawild/Meshes/Props"
ECHO_MESH_DESTINATION_PATH = "/Game/Astrawild/Meshes/Echoes"
CHARACTER_MESH_DESTINATION_PATH = "/Game/Astrawild/Meshes/Characters"
MAP_KIT_DESTINATION_PATH = "/Game/Astrawild/Meshes/MapKit"
AUDIO_DESTINATION_PATH = "/Game/Astrawild/Audio/SFX"
AMBIENCE_DESTINATION_PATH = "/Game/Astrawild/Audio/Ambience"
MUSIC_DESTINATION_PATH = "/Game/Astrawild/Audio/Music"
REGISTRY_ASSET_PATH = f"{DESTINATION_PATH}/DA_GeneratedAssetRegistry"

# The names are actual reflected C++ UStruct names in AstrawildCore. Keep this
# mapping explicit so a newly added CSV cannot silently import with the wrong row type.
TABLE_MAPPING = {
    "DT_Biomes.csv": "FAstrawildBiomeDefinition",
    "DT_BossAttacks.csv": "FAstrawildBossAttackRow",
    "DT_CampaignChapters.csv": "FAstrawildCampaignChapterRow",
    "DT_BossEncounters.csv": "FAstrawildBossEncounterRow",
    "DT_CookingRecipes.csv": "FAstrawildCookingRecipeRow",
    "DT_BreedingGroups.csv": "FAstrawildBreedingGroupRow",
    "DT_BreedingFusions.csv": "FAstrawildBreedingFusionRow",
    "DT_CyberneticEvolutions.csv": "FAstrawildCyberneticEvolutionRow",
    "DT_Dungeons.csv": "FAstrawildDungeonRow",
    "DT_EchoDex.csv": "FAstrawildEchoDexRow",
    "DT_EchoDex_200.csv": "FAstrawildMasterEchoRow",
    "DT_FishDex.csv": "FAstrawildFishRow",
    "DT_EcosystemBehavior.csv": "FAstrawildEcosystemBehaviorRow",
    "DT_EchoTraits.csv": "FAstrawildEchoTraitRow",
    "DT_Evolutions.csv": "FAstrawildEvolutionRow",
    "DT_FoliageRules.csv": "FAstrawildFoliageRuleRow",
    "DT_FastTravelSpires.csv": "FAstrawildFastTravelSpire",
    "DT_Lore.csv": "FAstrawildLoreRow",
    "DT_MechaAnimationProfiles.csv": "FAstrawildMechaAnimationProfileRow",
    "DT_MechaFrames.csv": "FAstrawildMechaFrameRow",
    "DT_MechaVFX.csv": "FAstrawildMechaVFXBindingRow",
    "DT_MechaWeapons.csv": "FAstrawildMechaWeaponRow",
    "DT_MountProfiles.csv": "FAstrawildMountProfile",
    "DT_QuestObjectives.csv": "FAstrawildQuestObjectiveRow",
    "DT_Quests.csv": "FAstrawildQuestRow",
    "DT_PowerGrid.csv": "FAstrawildPowerGridNodeRow",
    "DT_PlayerPerks.csv": "FAstrawildPlayerPerkRow",
    "DT_RangedWeapons.csv": "FAstrawildRangedWeaponRow",
    "DT_Recipes.csv": "FAstrawildCraftingRecipeRow",
    "DT_SpawnRules.csv": "FAstrawildWorldSpawnRule",
    "DT_TechnologyNodes.csv": "FAstrawildTechnologyNodeRow",
    "DT_Weather.csv": "FAstrawildWeatherRow",
    "DT_UnderwaterZones.csv": "FAstrawildUnderwaterZoneRow",
    "DT_WorldEvents.csv": "FAstrawildWorldEventRow",
}

MESH_FILES = {
    "SM_SunwoodLog.obj": {"asset_id": "Mesh.SunwoodLog", "consumer": "Landscape/Foliage/World dressing"},
    "SM_LumenRock.obj": {"asset_id": "Mesh.LumenRock", "consumer": "Resource node / Foliage"},
    "SM_AstraCrystal.obj": {"asset_id": "Mesh.AstraCrystal", "consumer": "Astra resource / VFX anchor"},
    "SM_CampfireBase.obj": {"asset_id": "Mesh.CampfireBase", "consumer": "Building / Rest station"},
    "SM_PrimalAxe.obj": {"asset_id": "Mesh.PrimalAxe", "consumer": "Harvesting equipment"},
    "SM_PrimalPick.obj": {"asset_id": "Mesh.PrimalPick", "consumer": "Harvesting equipment"},
    "SM_AstraResonator.obj": {"asset_id": "Mesh.AstraResonator", "consumer": "Capture equipment"},
    "SM_Wall_Wood.obj": {"asset_id": "Mesh.WallWood", "consumer": "Building defense"},
    "SM_Door_Wood.obj": {"asset_id": "Mesh.DoorWood", "consumer": "Building defense"},
}

AUDIO_FILES = {
    "SFX_Melee_Swing.wav": {"cue_id": "SFX.Melee.Swing", "consumer": "UAstrawildAudioSubsystem::PlaySFX"},
    "SFX_Melee_Hit.wav": {"cue_id": "SFX.Melee.Hit", "consumer": "UAstrawildAudioSubsystem::PlaySFX"},
    "SFX_Capture_Throw.wav": {"cue_id": "SFX.Capture.Throw", "consumer": "UAstrawildAudioSubsystem::PlaySFX"},
    "SFX_Capture_Success.wav": {"cue_id": "SFX.Capture.Success", "consumer": "UAstrawildAudioSubsystem::PlaySFX"},
    "SFX_Dodge_Roll.wav": {"cue_id": "SFX.Dodge.Roll", "consumer": "UAstrawildAudioSubsystem::PlaySFX"},
    "SFX_Building_Place.wav": {"cue_id": "SFX.Building.Place", "consumer": "UAstrawildAudioSubsystem::PlaySFX"},
    "SFX_LevelUp.wav": {"cue_id": "SFX.LevelUp", "consumer": "UAstrawildAudioSubsystem::PlaySFX"},
}


def _log(message: str) -> None:
    unreal.log(f"[ASTRAWILD][Import] {message}")


def _log_warning(message: str) -> None:
    unreal.log_warning(f"[ASTRAWILD][Import] {message}")


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


def _make_datatable_import_task(source_file: Path, destination_name: str, row_struct):
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


def _make_generic_import_task(source_file: Path, destination_path: str, destination_name: str):
    task = unreal.AssetImportTask()
    _set_property(task, "filename", str(source_file))
    _set_property(task, "destination_path", destination_path)
    _set_property(task, "destination_name", destination_name)
    _set_property(task, "automated", True)
    _set_property(task, "replace_existing", True)
    _set_property(task, "save", True)
    return task


def _write_report(project_dir: Path, filename: str, report: dict) -> Path:
    report_path = project_dir / "Saved" / "Astrawild" / filename
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    return report_path


def _ensure_directory(path: str) -> None:
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)


def _create_generated_registry(asset_tools, imported: list[dict]) -> dict:
    registry = unreal.load_asset(REGISTRY_ASSET_PATH)
    if registry is None:
        registry_class = unreal.load_class(None, "/Script/AstrawildCore.AstrawildGeneratedAssetRegistry")
        if registry_class is None:
            raise RuntimeError("Cannot load UAstrawildGeneratedAssetRegistry; compile AstrawildCore before running importer")
        registry = asset_tools.create_asset("DA_GeneratedAssetRegistry", DESTINATION_PATH, registry_class, None)
    if registry is None:
        raise RuntimeError(f"Cannot create generated asset registry: {REGISTRY_ASSET_PATH}")

    mesh_bindings = []
    audio_bindings = []
    for item in imported:
        imported_asset = unreal.load_asset(item["asset"])
        if imported_asset is None:
            raise RuntimeError(f"Cannot load imported asset for registry: {item['asset']}")
        if item["kind"] == "StaticMesh":
            binding = unreal.AstrawildGeneratedMeshBinding()
            _set_property(binding, "asset_id", item["asset_id"])
            _set_property(binding, "mesh", imported_asset)
            _set_property(binding, "consumer", item["consumer"])
            mesh_bindings.append(binding)
        elif item["kind"] == "SoundWave":
            binding = unreal.AstrawildGeneratedAudioBinding()
            _set_property(binding, "cue_id", item["cue_id"])
            _set_property(binding, "sound", imported_asset)
            _set_property(binding, "consumer", item["consumer"])
            audio_bindings.append(binding)

    _set_property(registry, "meshes", mesh_bindings)
    _set_property(registry, "audio", audio_bindings)
    _set_property(registry, "registry_version", "GeneratedAssets.v1")
    unreal.EditorAssetLibrary.save_asset(REGISTRY_ASSET_PATH, only_if_is_dirty=False)
    return {
        "status": "created_or_updated",
        "asset": REGISTRY_ASSET_PATH,
        "mesh_bindings": len(mesh_bindings),
        "audio_bindings": len(audio_bindings),
    }


def _import_generated_assets(project_dir: Path, asset_tools) -> dict:
    mesh_source_dir = project_dir / "Content" / "Astrawild" / "Meshes" / "Props"
    audio_source_dir = project_dir / "Content" / "Astrawild" / "Audio" / "SFX"
    source_mesh_groups = (
        (mesh_source_dir, MESH_DESTINATION_PATH, MESH_FILES, "Prop"),
        (project_dir / "Content" / "Astrawild" / "Meshes" / "Echoes", ECHO_MESH_DESTINATION_PATH, None, "EchoSource"),
        (project_dir / "Content" / "Astrawild" / "Meshes" / "Characters", CHARACTER_MESH_DESTINATION_PATH, None, "CharacterSource"),
        (project_dir / "Content" / "Astrawild" / "Meshes" / "MapKit", MAP_KIT_DESTINATION_PATH, None, "MapKitSource"),
    )

    tasks = []
    metadata = []
    missing = []
    for source_dir, destination_path, explicit_files, source_kind in source_mesh_groups:
        _ensure_directory(destination_path)
        if explicit_files is not None:
            source_files = [(filename, contract) for filename, contract in sorted(explicit_files.items())]
        else:
            source_files = [
                (path.name, {"asset_id": f"Mesh.{path.stem}", "consumer": f"{source_kind} mesh source"})
                for path in sorted(source_dir.glob("*.obj"))
            ]
        for filename, contract in source_files:
            source_file = source_dir / filename
            asset_path = f"{destination_path}/{source_file.stem}"
            if not source_file.is_file():
                missing.append(str(source_file))
                continue
            tasks.append(_make_generic_import_task(source_file, destination_path, source_file.stem))
            metadata.append({
                "kind": "StaticMesh",
                "source_kind": source_kind,
                "source": str(source_file),
                "asset": asset_path,
                "asset_id": contract["asset_id"],
                "consumer": contract["consumer"],
            })

    _ensure_directory(AUDIO_DESTINATION_PATH)
    audio_source_groups = (
        (audio_source_dir, AUDIO_DESTINATION_PATH, AUDIO_FILES, "CoreSFX"),
        (project_dir / "Content" / "Astrawild" / "Audio" / "Ambience", AMBIENCE_DESTINATION_PATH, None, "Ambience"),
        (project_dir / "Content" / "Astrawild" / "Audio" / "Music", MUSIC_DESTINATION_PATH, None, "Music"),
    )
    for source_dir, destination_path, explicit_files, source_kind in audio_source_groups:
        _ensure_directory(destination_path)
        if explicit_files is not None:
            source_files = [
                (path.name, explicit_files.get(path.name, {"cue_id": f"{source_kind}.{path.stem}", "consumer": f"{source_kind} audio source"}))
                for path in sorted(source_dir.glob("*.wav")) + sorted(source_dir.glob("*.mp3"))
            ]
        else:
            source_files = [
                (path.name, {"cue_id": f"{source_kind}.{path.stem}", "consumer": f"{source_kind} audio source"})
                for path in sorted(source_dir.glob("*.wav")) + sorted(source_dir.glob("*.mp3"))
            ]
        for filename, contract in source_files:
            source_file = source_dir / filename
            asset_path = f"{destination_path}/{source_file.stem}"
            if not source_file.is_file():
                missing.append(str(source_file))
                continue
            tasks.append(_make_generic_import_task(source_file, destination_path, source_file.stem))
            metadata.append({
                "kind": "SoundWave",
                "source_kind": source_kind,
                "source": str(source_file),
                "asset": asset_path,
                "cue_id": contract["cue_id"],
                "consumer": contract["consumer"],
            })

    if missing:
        raise RuntimeError("Missing generated asset sources: " + "; ".join(sorted(missing)))
    if tasks:
        asset_tools.import_asset_tasks(tasks)

    imported = []
    failed = []
    for item in metadata:
        if unreal.EditorAssetLibrary.does_asset_exist(item["asset"]):
            imported.append(item)
            unreal.EditorAssetLibrary.save_asset(item["asset"], only_if_is_dirty=False)
        else:
            failed.append(item)
            _log_warning(f"Generated asset was not created: {item['asset']}")

    registry_report = _create_generated_registry(asset_tools, imported)
    report = {
        "mesh_destinations": [MESH_DESTINATION_PATH, ECHO_MESH_DESTINATION_PATH, CHARACTER_MESH_DESTINATION_PATH, MAP_KIT_DESTINATION_PATH],
        "audio_destinations": [AUDIO_DESTINATION_PATH, AMBIENCE_DESTINATION_PATH, MUSIC_DESTINATION_PATH],
        "expected_count": len(metadata),
        "imported_count": len(imported),
        "failed_count": len(failed),
        "imported": imported,
        "failed": failed,
        "registry": registry_report,
        "notes": [
            "OBJ imports are StaticMesh candidates; assign original materials/collision/Nanite settings in Editor.",
            "WAV imports are SoundWave assets; route them through Sound Cues or AudioSubsystem registry as needed.",
            "This report is the integration bridge; it does not mutate Echo DataAssets or create Blueprint references automatically.",
        ],
    }
    report_path = _write_report(project_dir, "GeneratedAssetImportReport.json", report)
    registry = {
        "registry_version": 1,
        "meshes": [item for item in imported if item["kind"] == "StaticMesh"],
        "audio": [item for item in imported if item["kind"] == "SoundWave"],
        "cpp_consumers": {
            "audio": "UAstrawildAudioSubsystem::PlaySFX and registered ambient/boss soft references",
            "world": "Editor-authored actors/components consume imported StaticMesh paths",
        },
    }
    registry_path = _write_report(project_dir, "GeneratedAssetRegistry.json", registry)
    _log(f"Generated assets: {len(imported)}/{len(metadata)} imported")
    _log(f"Generated asset report: {report_path}")
    _log(f"Generated asset registry: {registry_path}")
    if failed:
        raise RuntimeError(f"Generated asset import failed for {len(failed)} asset(s); see {report_path}")
    return report


def import_all_datatables() -> dict:
    project_dir = Path(unreal.Paths.project_dir())
    source_dir = project_dir / "Content" / "Astrawild" / "Data" / "Source"
    if not source_dir.is_dir():
        raise RuntimeError(f"CSV source directory not found: {source_dir}")

    _ensure_directory(DESTINATION_PATH)
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
        tasks.append(_make_datatable_import_task(source_file, asset_name, row_struct))
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

    generated_report = _import_generated_assets(project_dir, asset_tools)
    report = {
        "project_dir": str(project_dir),
        "source_dir": str(source_dir),
        "destination_path": DESTINATION_PATH,
        "expected_count": len(TABLE_MAPPING),
        "imported_count": len(imported),
        "failed_count": len(failed),
        "imported": imported,
        "failed": failed,
        "generated_assets": generated_report,
    }
    report_path = _write_report(project_dir, "DataTableImportReport.json", report)
    _log(f"DataTables: {len(imported)}/{len(TABLE_MAPPING)} imported")
    _log(f"Report: {report_path}")

    if failed:
        raise RuntimeError(f"DataTable import failed for {len(failed)} asset(s); see {report_path}")
    return report


if __name__ == "__main__":
    import_all_datatables()
