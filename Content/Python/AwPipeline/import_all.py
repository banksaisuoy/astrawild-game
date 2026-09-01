"""
ASTRAWILD AwPipeline — UE 5.8 editor-side import orchestration.

Run inside the UE Editor (project open) via console:
    py "<ProjectRoot>/Content/Python/AwPipeline/import_all.py"

Pipeline (art pack batch 1 — mirrors ArtSource/manifest.json):
  1. Import textures (PNG -> /Game/Textures) with correct sRGB/compression.
  2. Import audio (WAV -> /Game/Audio).
  3. Import meshes (GLB -> manifest ue_path) via Interchange, flattening nested folders.
  4. Normalize animation sequence paths to /<Folder>/AM_<ClipName>.
  5. Build master materials (M_Master_Surface, M_Landscape_SciFiFrontier) & Material Instances.
  6. Add sockets (survivor Weapon_R/Scanner_L/Backpack_Spine + weapon Muzzle).
  7. Create Hero Niagara Systems (/Game/VFX/NS_AW_*).
  8. Save dirty packages; write verification report:
     <ProjectRoot>/Saved/AwPipelineReport/import_report.json
"""
import glob
import json
import os
import sys
import traceback

import unreal

_HERE = os.path.dirname(os.path.abspath(__file__))
if _HERE not in sys.path:
    sys.path.insert(0, _HERE)

import aw_materials  # noqa: E402  (same folder)

REPORT = {"stages": {}, "missing": [], "warnings": [], "errors": []}

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


def resolve_src(root: str, path_str: str) -> str:
    if not path_str:
        return ""
    if os.path.exists(path_str):
        return path_str
    if "ArtSource" in path_str:
        sub = path_str.split("ArtSource")[-1].lstrip("/\\").replace("/", os.sep).replace("\\", os.sep)
        cand = os.path.join(root, "ArtSource", sub)
        if os.path.exists(cand):
            return cand
    return path_str


# ---------------------------------------------------------------- textures
def import_textures(manifest: dict, root: str) -> None:
    stage = REPORT["stages"].setdefault("textures", {"imported": 0, "failed": 0})
    assets = manifest.get("assets", {})
    
    tex_entries = []
    for name, info in assets.items():
        cat = info.get("category", "")
        ue_path = info.get("ue_path", "")
        path = info.get("path", "")
        if name.startswith("T_") or path.endswith(".png") or ue_path.startswith("/Game/Textures") or cat == "texture":
            tex_entries.append((name, info))
            
    tex_dir = os.path.join(root, "ArtSource", "Textures")
    if os.path.exists(tex_dir):
        manifest_paths = {resolve_src(root, info.get("path", "")) for _, info in tex_entries}
        for png_file in glob.glob(os.path.join(tex_dir, "*.png")):
            if png_file not in manifest_paths:
                tname = os.path.splitext(os.path.basename(png_file))[0]
                tex_entries.append((tname, {
                    "path": png_file,
                    "ue_path": f"/Game/Textures/{tname}"
                }))

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    for name, info in tex_entries:
        src = resolve_src(root, info.get("path", ""))
        ue_path = info.get("ue_path", f"/Game/Textures/{name}")
        dest_path = os.path.dirname(ue_path)
        dest_name = os.path.basename(ue_path)
        if not os.path.exists(src):
            err(f"texture source missing on disk: {src}")
            continue
        try:
            if not unreal.EditorAssetLibrary.does_asset_exist(ue_path):
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
        except Exception as e:
            stage["failed"] += 1
            err(f"texture {name}: {e}")
    log(f"textures: imported={stage['imported']} failed={stage['failed']}")


# ------------------------------------------------------------------- audio
def import_audio(manifest: dict, root: str) -> None:
    stage = REPORT["stages"].setdefault("audio", {"imported": 0, "failed": 0})
    assets = manifest.get("assets", {})
    
    audio_entries = []
    for name, info in assets.items():
        cat = info.get("category", "")
        ue_path = info.get("ue_path", "")
        path = info.get("path", "")
        if name.startswith("A_") or path.endswith(".wav") or ue_path.startswith("/Game/Audio") or cat in ("ambience", "creature", "footstep", "player", "ui", "weapon", "audio"):
            audio_entries.append((name, info))
            
    audio_dir = os.path.join(root, "ArtSource", "Audio")
    if os.path.exists(audio_dir):
        manifest_paths = {resolve_src(root, info.get("path", "")) for _, info in audio_entries}
        for wav_file in glob.glob(os.path.join(audio_dir, "*.wav")):
            if wav_file not in manifest_paths:
                aname = os.path.splitext(os.path.basename(wav_file))[0]
                audio_entries.append((aname, {
                    "path": wav_file,
                    "ue_path": f"/Game/Audio/{aname}"
                }))

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    for name, info in audio_entries:
        src = resolve_src(root, info.get("path", ""))
        ue_path = info.get("ue_path", f"/Game/Audio/{name}")
        dest_path = os.path.dirname(ue_path)
        dest_name = os.path.basename(ue_path)
        if not os.path.exists(src):
            alt = os.path.join(audio_dir, f"{name}.wav")
            if os.path.exists(alt):
                src = alt
            else:
                err(f"audio source missing on disk: {src}")
                continue
        try:
            if unreal.EditorAssetLibrary.does_asset_exist(ue_path):
                stage["imported"] += 1
                continue
            task = unreal.AssetImportTask()
            task.filename = src
            task.destination_path = dest_path
            task.destination_name = dest_name
            task.automated = True
            task.save = True
            task.replace_existing = True
            asset_tools.import_asset_tasks([task])
            snd = unreal.EditorAssetLibrary.load_asset(ue_path)
            if not snd:
                raise RuntimeError(f"import produced no audio asset at {ue_path}")
            stage["imported"] += 1
        except Exception as e:
            stage["failed"] += 1
            err(f"audio {name}: {e}")
    log(f"audio: imported={stage['imported']} failed={stage['failed']}")


# ------------------------------------------------------------------- meshes
def find_imported_mesh(folder: str, name: str):
    exact = f"{folder}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(exact):
        return exact
        
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = ar.get_assets_by_path(folder, recursive=True)
    for asset in assets:
        aname = str(asset.asset_name)
        cls = str(asset.asset_class_path.asset_name)
        if aname == name and cls in ("SkeletalMesh", "StaticMesh"):
            return str(asset.package_name)
    return None


def import_meshes(manifest: dict, root: str) -> None:
    stage = REPORT["stages"].setdefault("meshes", {"imported": 0, "failed": 0, "anims": {}})
    assets = manifest.get("assets", {})
    mesh_entries = [(name, info) for name, info in assets.items()
                    if info.get("category") == "mesh" or info.get("ue_path", "").startswith(("/Game/Characters", "/Game/Echoes", "/Game/Weapons", "/Game/Vehicles", "/Game/Environment"))]
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    
    for name, info in mesh_entries:
        src = resolve_src(root, info.get("path", ""))
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
            existing = find_imported_mesh(dest_path, dest_name)
            if existing:
                if existing != ue_path:
                    unreal.EditorAssetLibrary.rename_asset(existing, ue_path)
                stage["imported"] += 1
                continue
                
            task = unreal.AssetImportTask()
            task.filename = src
            task.destination_path = dest_path
            task.destination_name = dest_name
            task.automated = True
            task.save = True
            task.replace_existing = True
            asset_tools.import_asset_tasks([task])
            
            found = find_imported_mesh(dest_path, dest_name)
            if not found:
                raise RuntimeError(f"no asset produced for {name} under {dest_path}")
            if found != ue_path:
                unreal.EditorAssetLibrary.rename_asset(found, ue_path)
                
            stage["imported"] += 1
        except Exception as e:
            stage["failed"] += 1
            err(f"mesh {name}: {e}")
    log(f"meshes: imported={stage['imported']} failed={stage['failed']}")


def normalize_animation_paths(manifest: dict) -> None:
    stage = REPORT["stages"].setdefault("meshes", {"imported": 0, "failed": 0, "anims": {}})
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = manifest.get("assets", {})
    for name, info in assets.items():
        for clip in info.get("animations", []) or []:
            if not clip.startswith("AM_"):
                continue
            ue_path = info.get("ue_path", "")
            folder = os.path.dirname(ue_path)
            expected = f"{folder}/{clip}"
            if unreal.EditorAssetLibrary.does_asset_exist(expected):
                stage["anims"][clip] = expected
                continue
            found = False
            for asset in ar.get_assets_by_path(folder, recursive=True):
                asset_name = str(asset.asset_name)
                cls = str(asset.asset_class_path.asset_name)
                if "AnimSequence" in cls and clip.replace("AM_", "") in asset_name:
                    src = str(asset.package_name)
                    try:
                        unreal.EditorAssetLibrary.rename_asset(src, expected)
                        stage["anims"][clip] = expected
                        found = True
                        break
                    except Exception as e:
                        warn(f"could not rename {src} -> {expected}: {e}")
            if not found:
                suffixes = ["", "_0", "_1", "_2"]
                for suf in suffixes:
                    cand = f"{folder}/{clip}{suf}"
                    if suf and unreal.EditorAssetLibrary.does_asset_exist(cand):
                        try:
                            unreal.EditorAssetLibrary.rename_asset(cand, expected)
                            stage["anims"][clip] = expected
                            found = True
                        except Exception as e:
                            warn(f"rename {cand} failed: {e}")
                        break
                if not found:
                    REPORT["missing"].append(expected)
                    warn(f"animation clip not found after import: {expected}")


# ------------------------------------------------------------------ sockets
def add_sockets(manifest: dict) -> None:
    stage = REPORT["stages"].setdefault("sockets", {"added": 0})
    for name, info in manifest.get("assets", {}).items():
        if not info.get("sockets"):
            continue
        ue_path = info.get("ue_path", "")
        mesh = unreal.EditorAssetLibrary.load_asset(ue_path)
        if not mesh:
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
                    loc = unreal.Vector(*[v * 100.0 for v in sock["pos"]])
                    rot = sock.get("rot", [0, 0, 0])
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
        except Exception as e:
            warn(f"sockets on {ue_path}: {e}")
    log(f"sockets added: {stage['added']}")


# ----------------------------------------------------------- niagara systems
def create_hero_niagara_systems() -> None:
    stage = REPORT["stages"].setdefault("niagara", {"created": 0})
    hero_systems = [
        "/Game/VFX/NS_AW_MuzzleFlash",
        "/Game/VFX/NS_AW_Weap_Impact",
        "/Game/VFX/NS_AW_Weap_Trail"
    ]
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    for sys_path in hero_systems:
        if unreal.EditorAssetLibrary.does_asset_exist(sys_path):
            continue
        dest_path = os.path.dirname(sys_path)
        dest_name = os.path.basename(sys_path)
        try:
            factory = unreal.NiagaraSystemFactoryNew() if hasattr(unreal, "NiagaraSystemFactoryNew") else None
            if factory:
                asset_tools.create_asset(dest_name, dest_path, unreal.NiagaraSystem, factory)
            else:
                asset_tools.create_asset(dest_name, dest_path, None, None)
            stage["created"] += 1
            log(f"Created Niagara System: {sys_path}")
        except Exception as e:
            warn(f"Could not create Niagara system {sys_path}: {e}")


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
    
    for hero in ["/Game/VFX/NS_AW_MuzzleFlash", "/Game/VFX/NS_AW_Weap_Impact", "/Game/VFX/NS_AW_Weap_Trail"]:
        if unreal.EditorAssetLibrary.does_asset_exist(hero):
            ok += 1
        else:
            missing.append(hero)
            
    REPORT["coverage"] = {"resolved": ok, "missing": missing}
    REPORT["missing"] = missing
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
        import_textures(manifest, root)
    except Exception as e:
        err(f"texture stage crashed: {e}\n{traceback.format_exc()}")
        
    try:
        import_audio(manifest, root)
    except Exception as e:
        err(f"audio stage crashed: {e}\n{traceback.format_exc()}")
        
    try:
        import_meshes(manifest, root)
        normalize_animation_paths(manifest)
    except Exception as e:
        err(f"mesh stage crashed: {e}\n{traceback.format_exc()}")
        
    try:
        create_hero_niagara_systems()
    except Exception as e:
        err(f"niagara stage crashed: {e}\n{traceback.format_exc()}")
        
    try:
        aw_materials.build_materials(manifest)
    except Exception as e:
        err(f"material stage crashed: {e}\n{traceback.format_exc()}")
        
    try:
        add_sockets(manifest)
    except Exception as e:
        err(f"socket stage crashed: {e}\n{traceback.format_exc()}")
        
    try:
        verify_coverage(manifest)
    except Exception as e:
        err(f"verify stage crashed: {e}")
        
    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    except Exception as e:
        err(f"save failed: {e}")
        
    write_report(root)


if __name__ == "__main__":
    main()
