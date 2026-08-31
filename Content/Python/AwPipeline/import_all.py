"""
ASTRAWILD AwPipeline — UE 5.8 editor-side import orchestration.

Run inside the UE Editor (project open) via console:
    py "<ProjectRoot>/Content/Python/AwPipeline/import_all.py"

Pipeline (art pack batch 1 — mirrors ArtSource/manifest.json):
  1. Import textures (PNG -> /Game/Textures) with correct sRGB/compression.
  2. Import meshes (GLB -> manifest ue_path) via Interchange; normalize
     animation sequence paths to /<Folder>/AM_<ClipName>.
  3. Build master materials (M_Master_Surface, M_Landscape_SciFiFrontier).
  4. Create material instances + bind them to mesh material slots.
  5. Add sockets (survivor Weapon_R/Scanner_L/Backpack_Spine + weapon Muzzle).
  6. Save dirty packages; write verification report:
     <ProjectRoot>/Saved/AwPipelineReport/import_report.json

Every stage is defensive: failures are logged and collected into the report
instead of aborting the whole pipeline. Niagara systems (NS_AW_*) are NOT
automatable — see Docs/ASTRAWILD_ART_PACK_RUNBOOK.md for the 3 hero systems.
"""
import json
import os
import sys
import traceback

import unreal

_HERE = os.path.dirname(os.path.abspath(__file__))
if _HERE not in sys.path:
    sys.path.insert(0, _HERE)

import aw_materials  # noqa: E402  (same folder)

REPORT: dict = {"stages": {}, "missing": [], "warnings": [], "errors": []}

def log(msg: str) -> None:
    unreal.log("[AwPipeline] " + str(msg))

def warn(msg: str) -> None:
    REPORT["warnings"].append(str(msg))
    unreal.log_warning("[AwPipeline] " + str(msg))

def err(msg: str) -> None:
    REPORT["errors"].append(str(msg))
    unreal.log_error("[AwPipeline] " + str(msg))


def project_root() -> str:
    proj_file = unreal.Paths.get_project_file_path()
    root = os.path.dirname(os.path.abspath(proj_file))
    return root


def load_manifest(root: str) -> dict:
    manifest_path = os.path.join(root, "ArtSource", "manifest.json")
    with open(manifest_path, "r", encoding="utf-8") as f:
        return json.load(f)


# ---------------------------------------------------------------- textures
def import_textures(manifest: dict) -> None:
    stage = REPORT["stages"].setdefault("textures", {"imported": 0, "failed": 0})
    assets = manifest.get("assets", {})
    tex_entries = [(name, info) for name, info in assets.items()
                   if info.get("category") == "texture"]
    # map disk path -> destination
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    for name, info in tex_entries:
        src = info["path"]
        ue_path = info.get("ue_path", f"/Game/Textures/{name}")
        dest_path = os.path.dirname(ue_path)          # '/Game/Textures'
        dest_name = os.path.basename(ue_path)
        if not os.path.exists(src):
            err(f"texture source missing on disk: {src}")
            continue
        try:
            task = unreal.AssetImportTask()
            task.filename = src
            task.destination_path = dest_path
            task.destination_name = dest_name
            task.automated = True
            task.save = True
            task.replace_existing = True
            asset_tools.import_asset_tasks([task])
            tex = unreal.EditorAssetLibrary.load_asset(ue_path)
            if not tex:
                # glTF-imported duplicates might live at a suffixed name
                raise RuntimeError(f"import produced no asset at {ue_path}")
            low = dest_name.lower()
            srgb = bool(info.get("srgb", low.endswith("_d")))
            tex.set_editor_property("srgb", srgb)
            if low.endswith("_n"):
                tex.set_editor_property(
                    "compression_settings",
                    unreal.TextureCompressionSettings.TC_NORMALMAP)
                tex.set_editor_property("srgb", False)
            elif low.endswith("_orm") or low.endswith("_m") or "_fb" in low:
                tex.set_editor_property(
                    "compression_settings",
                    unreal.TextureCompressionSettings.TC_MASKS)
                tex.set_editor_property("srgb", False)
            elif low.endswith("_e"):
                tex.set_editor_property(
                    "compression_settings",
                    unreal.TextureCompressionSettings.TC_LINEAR_COLOR if "fb" in low
                    else unreal.TextureCompressionSettings.TC_MASKS)
                tex.set_editor_property("srgb", False)
            stage["imported"] += 1
        except Exception as e:  # noqa: BLE001
            stage["failed"] += 1
            err(f"texture {name}: {e}")
    log(f"textures: imported={stage['imported']} failed={stage['failed']}")


# ------------------------------------------------------------------- meshes
def import_meshes(manifest: dict) -> None:
    stage = REPORT["stages"].setdefault("meshes", {"imported": 0, "failed": 0, "anims": {}})
    assets = manifest.get("assets", {})
    mesh_entries = [(name, info) for name, info in assets.items()
                    if info.get("category") == "mesh"]
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    for name, info in mesh_entries:
        src = info["path"]
        ue_path = info.get("ue_path")
        if not os.path.exists(src):
            err(f"mesh source missing on disk: {src}")
            continue
        if not ue_path:
            warn(f"mesh {name} has no ue_path — skipped import")
            continue
        dest_path = os.path.dirname(ue_path)
        dest_name = os.path.basename(ue_path)
        try:
            task = unreal.AssetImportTask()
            task.filename = src
            task.destination_path = dest_path
            task.destination_name = dest_name
            task.automated = True
            task.save = True
            task.replace_existing = True
            asset_tools.import_asset_tasks([task])
            obj = unreal.EditorAssetLibrary.load_asset(ue_path)
            if not obj:
                raise RuntimeError(f"no asset produced at {ue_path}")
            stage["imported"] += 1
        except Exception as e:  # noqa: BLE001
            stage["failed"] += 1
            err(f"mesh {name}: {e}")
    log(f"meshes: imported={stage['imported']} failed={stage['failed']}")


def normalize_animation_paths(manifest: dict) -> None:
    """glTF import names animation sequences variously; enforce
    /<Folder>/AM_<ClipName> so C++ soft paths resolve deterministically."""
    stage = REPORT["stages"]["meshes"]
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = manifest.get("assets", {})
    for name, info in assets.items():
        if info.get("category") != "mesh":
            continue
        for clip in info.get("animations", []) or []:
            if not clip.startswith("AM_"):
                continue  # only our authored clips
            ue_path = info.get("ue_path", "")
            folder = os.path.dirname(ue_path)
            expected = f"{folder}/{clip}"
            if unreal.EditorAssetLibrary.does_asset_exist(expected):
                stage["anims"][clip] = expected
                continue
            # search any AnimSequence in that folder containing the clip name
            found = False
            for asset in ar.get_assets_by_path(folder, recursive=True):
                asset_name = str(asset.asset_name)
                cls = str(asset.asset_class_path.asset_name)
                if "AnimSequence" in cls and clip.replace("AM_", "") in asset_name:
                    src = unreal.Paths.combine([folder, str(asset.package_name)[len(folder):]
                                                if str(asset.package_name).startswith(folder)
                                                else str(asset.package_name)])
                    src = str(asset.package_name)
                    try:
                        unreal.EditorAssetLibrary.rename_asset(src, expected)
                        stage["anims"][clip] = expected
                        found = True
                        break
                    except Exception as e:  # noqa: BLE001
                        warn(f"could not rename {src} -> {expected}: {e}")
            if not found:
                # last resort: duplicate-named variants like <clip>_0
                suffixes = ["", "_0", "_1", "_2"]
                for suf in suffixes:
                    cand = f"{folder}/{clip}{suf}"
                    if suf and unreal.EditorAssetLibrary.does_asset_exist(cand):
                        try:
                            unreal.EditorAssetLibrary.rename_asset(cand, expected)
                            stage["anims"][clip] = expected
                            found = True
                        except Exception as e:  # noqa: BLE001
                            warn(f"rename {cand} failed: {e}")
                        break
                if not found:
                    REPORT["missing"].append(expected)
                    warn(f"animation clip not found after import: {expected}")


# ------------------------------------------------------------------ sockets
def add_sockets(manifest: dict) -> None:
    stage = REPORT["stages"].setdefault("sockets", {"added": 0})
    for name, info in manifest.get("assets", {}).items():
        if info.get("category") != "mesh" or not info.get("sockets"):
            continue
        ue_path = info.get("ue_path", "")
        mesh = unreal.EditorAssetLibrary.load_asset(ue_path)
        if not mesh:
            warn(f"sockets: mesh not found {ue_path}")
            continue
        try:
            if isinstance(mesh, unreal.SkeletalMesh):
                existing = mesh.get_editor_property("sockets") or []
                have = {str(s.get_editor_property("socket_name")) for s in existing}
                for sock in info["sockets"]:
                    sname = sock["name"]
                    if sname in have:
                        continue
                    sk_socket = unreal.SkeletalMeshSocket()
                    sk_socket.set_editor_property("socket_name", sname)
                    sk_socket.set_editor_property("bone_name", sock.get("bone", "Root"))
                    loc = unreal.Vector(*[v * 100.0 for v in sock["pos"]])  # m->cm
                    rot = sock.get("rot", [0, 0, 0])
                    # socket world pos minus bone bind world pos (identity rotation)
                    sk_socket.set_editor_property("relative_location", loc)
                    sk_socket.set_editor_property(
                        "relative_rotation",
                        unreal.Rotator(rot[0], rot[1], rot[2]))
                    existing.append(sk_socket)
                    stage["added"] += 1
                mesh.set_editor_property("sockets", existing)
            elif isinstance(mesh, unreal.StaticMesh):
                existing = mesh.get_editor_property("sockets") or []
                have = {str(s.get_editor_property("socket_name")) for s in existing}
                for sock in info["sockets"]:
                    sname = sock["name"]
                    if sname in have:
                        continue
                    sm_socket = unreal.StaticMeshSocket()
                    sm_socket.set_editor_property("socket_name", sname)
                    sm_socket.set_editor_property(
                        "relative_location",
                        unreal.Vector(*[v * 100.0 for v in sock["pos"]]))
                    existing.append(sm_socket)
                    stage["added"] += 1
                mesh.set_editor_property("sockets", existing)
        except Exception as e:  # noqa: BLE001
            warn(f"sockets on {ue_path}: {e}")
    log(f"sockets added: {stage['added']}")


# ------------------------------------------------------------------- report
def verify_coverage(manifest: dict) -> None:
    ok, missing = 0, []
    for name, info in manifest.get("assets", {}).items():
        ue_path = info.get("ue_path")
        if not ue_path:
            continue
        if unreal.EditorAssetLibrary.does_asset_exist(ue_path):
            ok += 1
        else:
            missing.append(ue_path)
    REPORT["coverage"] = {"resolved": ok, "missing": missing}
    for m in missing:
        REPORT["missing"].append(m)
    log(f"coverage: {ok} resolved, {len(missing)} missing")


def write_report(root: str) -> None:
    out_dir = os.path.join(root, "Saved", "AwPipelineReport")
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, "import_report.json")
    REPORT["total_missing"] = len(REPORT["missing"])
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(REPORT, f, indent=2)
    status = "PASS" if not REPORT["missing"] and not REPORT["errors"] else "NEEDS ATTENTION"
    log(f"report written: {out_path} -> {status}")
    unreal.log("=" * 60)
    unreal.log(f"[AwPipeline] FINAL: {status} "
               f"(errors={len(REPORT['errors'])}, warnings={len(REPORT['warnings'])}, "
               f"missing={len(REPORT['missing'])})")
    unreal.log("=" * 60)


def main() -> None:
    root = project_root()
    log(f"project root: {root}")
    manifest = load_manifest(root)
    total = len(manifest.get("assets", {}))
    log(f"manifest: {total} assets")
    try:
        import_textures(manifest)
    except Exception as e:  # noqa: BLE001
        err(f"texture stage crashed: {e}\n{traceback.format_exc()}")
    try:
        import_meshes(manifest)
        normalize_animation_paths(manifest)
    except Exception as e:  # noqa: BLE001
        err(f"mesh stage crashed: {e}\n{traceback.format_exc()}")
    try:
        aw_materials.build_materials(manifest)
    except Exception as e:  # noqa: BLE001
        err(f"material stage crashed: {e}\n{traceback.format_exc()}")
    try:
        add_sockets(manifest)
    except Exception as e:  # noqa: BLE001
        err(f"socket stage crashed: {e}\n{traceback.format_exc()}")
    try:
        verify_coverage(manifest)
    except Exception as e:  # noqa: BLE001
        err(f"verify stage crashed: {e}")
    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    except Exception as e:  # noqa: BLE001
        err(f"save failed: {e}")
    write_report(root)


if __name__ == "__main__":
    main()
    main = main  # keep linter quiet when executed via `py`
