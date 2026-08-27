"""Create the safe ASTRAWILD Editor asset scaffold.

Run inside Unreal Editor after the C++ module has compiled:
    py "Scripts/setup_project_assets.py"

This script creates folders and conservative placeholder/scaffold assets only. It
never pretends to author final art, audio, Niagara graphs, maps, or Blueprint
behavior. Every optional factory or parameter API failure is reported so a UE
version/API difference is visible in Saved/Astrawild/AssetScaffoldReport.json.
"""
from __future__ import annotations

import json
from pathlib import Path

import unreal


ROOT = "/Game/Astrawild"
FOLDERS = [
    "Art",
    "Art/Materials",
    "Art/Materials/Landscape",
    "Art/Textures",
    "Art/Icons",
    "Animations",
    "Audio",
    "Audio/Ambience",
    "Audio/Boss",
    "Audio/SFX",
    "Blueprints",
    "Data",
    "Data/Imported",
    "Dungeons",
    "Echoes",
    "FX",
    "FX/Emissive",
    "Maps",
    "Physics",
    "UI",
    "Weapons",
    "World",
    "World/Foliage",
]

LANDSCAPE_MPC_PARAMETERS = {
    "AW_LandscapeWetness": 0.0,
    "AW_RainIntensity": 0.0,
    "AW_WindStrength": 0.35,
    "AW_GrassSlopeMaxDegrees": 18.0,
    "AW_RockSlopeStartDegrees": 48.0,
    "AW_MeadowHeightMeters": 100.0,
    "AW_MountainHeightMeters": 300.0,
}


AMBIENT_SCAFFOLDS = (
    ("Audio/Ambience", "SC_DawnMeadows_Day"),
    ("Audio/Ambience", "SC_DawnMeadows_Night"),
    ("Audio/Ambience", "SC_SylvanRainforest_Day"),
    ("Audio/Ambience", "SC_SylvanRainforest_Night"),
    ("Audio/Ambience", "SC_Caldera_Day"),
    ("Audio/Ambience", "SC_Caldera_Night"),
    ("Audio/Ambience", "SC_GlacialZenith_Day"),
    ("Audio/Ambience", "SC_GlacialZenith_Night"),
)

BOSS_AUDIO_SCAFFOLDS = tuple(
    ("Audio/Boss", f"{boss}_{stage}")
    for boss in ("Boss_SolarixAlpha", "Boss_Miremaw", "Boss_Terradon", "Boss_Stormshell", "Boss_FirstDawnDragon")
    for stage in ("PhaseOne", "PhaseTwo", "Ultimate")
)

MECHA_VFX_SCAFFOLDS = (
    ("FX/Exosuit", "NS_AstraBeamLine"),
    ("FX/Exosuit", "NS_OverboostThrusterTrail"),
    ("FX/Exosuit", "NS_PlasmaEdgeSparks"),
    ("FX/Exosuit", "NS_AstraMuzzleFlash"),
    ("FX/Exosuit", "NS_ExosuitShutdown"),
)

MECHA_ANIMATION_CONTRACT_PATHS = (
    "/Game/Astrawild/Animation/Mecha/ABP_AstraExosuit",
    "/Game/Astrawild/Animation/Mecha/AM_AstraOverboost",
    "/Game/Astrawild/Animation/Mecha/AM_AstraPlasmaEdge",
    "/Game/Astrawild/Animation/Mecha/AM_AstraHeavyCannon",
)


# The script is intentionally small and explicit: an asset is either created,
# already exists, skipped with a reason, or failed with the original exception.
def _log(message: str) -> None:
    unreal.log(f"[ASTRAWILD][AssetScaffold] {message}")


def _warn(message: str) -> None:
    unreal.log_warning(f"[ASTRAWILD][AssetScaffold] {message}")


def _asset_path(folder: str, name: str) -> str:
    return f"{ROOT}/{folder}/{name}"


def _create_asset(asset_tools, factory_class_name: str, folder: str, name: str, report: dict):
    path = _asset_path(folder, name)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        report["existing"].append(path)
        return unreal.load_asset(path)

    factory_class = getattr(unreal, factory_class_name, None)
    if factory_class is None:
        report["skipped"].append({"path": path, "reason": f"UE Python class {factory_class_name} unavailable"})
        _warn(f"Skipped {path}: {factory_class_name} is unavailable in this UE build")
        return None

    try:
        asset = asset_tools.create_asset(name, f"{ROOT}/{folder}", None, factory_class())
        if asset is None:
            raise RuntimeError("AssetTools.create_asset returned None")
        unreal.EditorAssetLibrary.save_loaded_asset(asset)
        report["created"].append(path)
        _log(f"Created {path}")
        return asset
    except Exception as exc:
        report["failed"].append({"path": path, "error": str(exc)})
        _warn(f"Failed {path}: {exc}")
        return None


def _configure_landscape_mpc(mpc, report: dict) -> None:
    if mpc is None:
        return
    try:
        collection_parameter_class = getattr(unreal, "CollectionParameter", None)
        if collection_parameter_class is None:
            report["skipped"].append({"path": _asset_path("Art/Materials/Landscape", "MPC_AstrawildLandscape"), "reason": "UE Python class CollectionParameter unavailable"})
            return
        parameters = []
        for name, default_value in LANDSCAPE_MPC_PARAMETERS.items():
            parameter = collection_parameter_class()
            try:
                parameter.set_editor_property("parameter_name", name)
            except Exception:
                parameter.set_editor_property("name", name)
            parameter.set_editor_property("default_value", default_value)
            parameters.append(parameter)
        mpc.set_editor_property("scalar_parameters", parameters)
        unreal.EditorAssetLibrary.save_loaded_asset(mpc)
        report["configured"].append({
            "path": _asset_path("Art/Materials/Landscape", "MPC_AstrawildLandscape"),
            "scalar_parameters": sorted(LANDSCAPE_MPC_PARAMETERS),
        })
        _log("Configured MPC_AstrawildLandscape scalar parameter contract")
    except Exception as exc:
        report["failed"].append({
            "path": _asset_path("Art/Materials/Landscape", "MPC_AstrawildLandscape"),
            "error": f"MPC scalar parameter configuration: {exc}",
        })
        _warn(f"Could not configure MPC_AstrawildLandscape: {exc}")


def _assign_default_material_parent(material, report: dict, path: str) -> None:
    if material is None:
        return
    default_material = unreal.load_asset("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")
    if default_material is None:
        report["skipped"].append({"path": path, "reason": "DefaultMaterial was unavailable"})
        return
    try:
        material.set_editor_property("parent", default_material)
        unreal.EditorAssetLibrary.save_loaded_asset(material)
    except Exception as exc:
        report["failed"].append({"path": path, "error": f"parent assignment: {exc}"})
        _warn(f"Could not assign parent for {path}: {exc}")


def setup_project_assets() -> dict:
    for relative_folder in FOLDERS:
        unreal.EditorAssetLibrary.make_directory(f"{ROOT}/{relative_folder}")

    report = {
        "root": ROOT,
        "folders": [f"{ROOT}/{folder}" for folder in FOLDERS],
        "created": [],
        "existing": [],
        "configured": [],
        "skipped": [],
        "failed": [],
        "required_editor_contract_paths": list(MECHA_ANIMATION_CONTRACT_PATHS),
        "notes": [
            "Scaffold assets are placeholders only; assign original production art/audio before PIE.",
            "Landscape MPC names and scalar parameters are an authored material-graph contract, not a finished shader.",
            "Physics asset creation may require a supplied skeletal mesh and can therefore be reported as skipped.",
            "A successful script run does not prove C++ compilation, Blueprint compilation, PIE, network PIE, or packaging.",
        ],
    }
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    placeholder_material = _create_asset(asset_tools, "MaterialInstanceConstantFactoryNew", "Art/Materials", "MI_AstrawildPlaceholder", report)
    _assign_default_material_parent(placeholder_material, report, _asset_path("Art/Materials", "MI_AstrawildPlaceholder"))

    landscape_mpc = _create_asset(asset_tools, "MaterialParameterCollectionFactoryNew", "Art/Materials/Landscape", "MPC_AstrawildLandscape", report)
    _configure_landscape_mpc(landscape_mpc, report)

    landscape_material = _create_asset(asset_tools, "MaterialInstanceConstantFactoryNew", "Art/Materials/Landscape", "MI_AstrawildLandscape", report)
    _assign_default_material_parent(landscape_material, report, _asset_path("Art/Materials/Landscape", "MI_AstrawildLandscape"))

    _create_asset(asset_tools, "PhysicsAssetFactory", "Physics", "PA_AstrawildPlaceholder", report)

    for name in ("NS_AstrawildPlaceholder", "NS_LandscapeEmissive", "NS_SolarSparks", "NS_GeoDust", "NS_TorrentSplash"):
        _create_asset(asset_tools, "NiagaraSystemFactoryNew", "FX/Emissive" if name != "NS_AstrawildPlaceholder" else "FX", name, report)
    for folder, name in MECHA_VFX_SCAFFOLDS:
        _create_asset(asset_tools, "NiagaraSystemFactoryNew", folder, name, report)

    _create_asset(asset_tools, "SoundCueFactoryNew", "Audio/SFX", "SC_AstrawildPlaceholder", report)
    for folder, name in AMBIENT_SCAFFOLDS + BOSS_AUDIO_SCAFFOLDS:
        _create_asset(asset_tools, "SoundCueFactoryNew", folder, name, report)

    project_dir = Path(unreal.Paths.project_dir())
    report_path = project_dir / "Saved" / "Astrawild" / "AssetScaffoldReport.json"
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, indent=2, ensure_ascii=False), encoding="utf-8")
    _log(f"Scaffold complete. Created={len(report['created'])}, existing={len(report['existing'])}, configured={len(report['configured'])}, skipped={len(report['skipped'])}, failed={len(report['failed'])}")
    _log(f"Report: {report_path}")
    return report


if __name__ == "__main__":
    setup_project_assets()
