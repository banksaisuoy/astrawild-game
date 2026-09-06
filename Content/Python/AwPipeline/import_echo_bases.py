"""
ASTRAWILD AwPipeline — Sci-Fantasy base import (directive SCI Phase 4).

Imports the Sci-Fantasy mutation-system additions into the engine:

  1. The 16 baked base archetypes (ArtSource/Meshes/Echoes/BaseMeshes/
     SK_Base_*.glb -> /Game/Characters/Echoes/BaseMeshes/) as SKELETAL
     meshes (Interchange), then normalizes the Idle/Move/Hit clip names to
     the AM_<id>_<Clip> convention FAstrawildEchoMutator derives.
  2. The 8 theme sound sets (ArtSource/Audio/Echoes/SFXSet_*.wav ->
     /Game/Audio/Echoes/SFXSet_<Theme>_<n> SoundWaves).
  3. The 7 persistent element Niagara templates
     (/Game/VFX/NS_AW_Elem_{Fire,Frost,Electric,Void,Poison,Spore,Radiant})
     — template systems (same pattern as the hero weapon systems); full
     visual authoring is the engine-verification pass, never faked here.
  4. The 6 Sci-Fantasy master materials with exposed parameters
     (M_SciFi_MetallicRobot, M_SciFi_EnergyBody, M_SciFi_StonyGolem,
     M_SciFi_Slime, M_SciFi_Chitin, M_SciFi_VoidFlesh) — parameterized
     (Tint / PatternTint / GlowIntensity / Roughness / Metallic) so the
     runtime material-swap language has an authoring surface.

Writes Saved/AwPipelineReport/echo_base_report.json.
Acceptance: total_missing == 0 and errors == [].

REQUIRES LOCAL EXECUTION (UE 5.8 editor):
  & "E:\\Epic Games\\UnrealEngine\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" `
    "E:\\AstrawildGame\\ASTRAWILD.uproject" `
    -ExecutePythonScript="E:\\AstrawildGame\\Content\\Python\\AwPipeline\\import_echo_bases.py" `
    -NoUI -Log
"""

import json
import os
import traceback

import unreal

REPORT = {
    "pipeline": "AwPipeline Sci-Fantasy base import (directive SCI Phase 4)",
    "stages": {},
    "missing": [],
    "warnings": [],
    "errors": [],
    "coverage": {},
    "total_missing": 0,
}

BASE_IDS = [
    "SK_Base_GolemQuadruped", "SK_Base_MonolithColossus",
    "SK_Base_ElemDrake", "SK_Base_ElemWisp",
    "SK_Base_MutantBeast", "SK_Base_MutantAvian",
    "SK_Base_ArmoredBeetle", "SK_Base_ArmoredCrab",
    "SK_Base_SpiritWisp", "SK_Base_SpiritOrb",
    "SK_Base_CyborgBeast", "SK_Base_CyborgSerpent",
    "SK_Base_PlantMaw", "SK_Base_Mushroomling",
    "SK_Base_VoidBlob", "SK_Base_VoidTentacle",
]

ELEMENT_VFX = ["Fire", "Frost", "Electric", "Void", "Poison", "Spore", "Radiant"]

SOUND_SETS = ["AncientConstruct", "ElementalBeast", "MutatedFauna", "ArmoredOrganic",
              "EtherealSpirit", "MechanicalHybrid", "PlantMonster", "VoidAbomination"]

# (material asset name, theme, metallic, roughness, emissive intensity)
THEME_MASTERS = [
    ("M_SciFi_MetallicRobot", "MechanicalHybrid", 0.92, 0.28, 0.35),
    ("M_SciFi_EnergyBody",    "ElementalBeast",   0.10, 0.22, 2.20),
    ("M_SciFi_StonyGolem",    "AncientConstruct", 0.05, 0.85, 0.12),
    ("M_SciFi_Slime",         "VoidAbomination",  0.02, 0.12, 0.45),
    ("M_SciFi_Chitin",        "ArmoredOrganic",   0.55, 0.45, 0.30),
    ("M_SciFi_VoidFlesh",     "VoidAbomination",  0.00, 0.35, 0.80),
]


def log(msg: str) -> None:
    unreal.log("[AwPipeline/EchoBases] " + str(msg))


def warn(msg: str) -> None:
    REPORT["warnings"].append(str(msg))
    unreal.log_warning("[AwPipeline/EchoBases] " + str(msg))


def err(msg: str) -> None:
    REPORT["errors"].append(str(msg))
    unreal.log_error("[AwPipeline/EchoBases] " + str(msg))


def project_root() -> str:
    proj_file = unreal.Paths.get_project_file_path()
    return os.path.dirname(os.path.abspath(proj_file))


# ---------------------------------------------------------------- bases
def import_base_meshes(root: str) -> None:
    stage = REPORT["stages"].setdefault("base_meshes", {"imported": 0, "failed": 0, "anims": {}})
    src_dir = os.path.join(root, "ArtSource", "Meshes", "Echoes", "BaseMeshes")
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    dest_folder = "/Game/Characters/Echoes/BaseMeshes"

    for base_id in BASE_IDS:
        src = os.path.join(src_dir, base_id + ".glb")
        ue_path = f"{dest_folder}/{base_id}"
        if not os.path.exists(src):
            err(f"base GLB missing on disk: {src}")
            continue
        try:
            if not unreal.EditorAssetLibrary.does_asset_exist(ue_path):
                task = unreal.AssetImportTask()
                task.filename = src
                task.destination_path = dest_folder
                task.destination_name = base_id
                task.automated = True
                task.save = True
                task.replace_existing = True
                asset_tools.import_asset_tasks([task])

            mesh = unreal.EditorAssetLibrary.load_asset(ue_path)
            if not mesh:
                raise RuntimeError(f"import produced no asset at {ue_path}")
            stage["imported"] += 1
            log(f"base imported: {ue_path}")
        except Exception as e:
            stage["failed"] += 1
            err(f"base {base_id}: {e}")

    # Normalize the animation clips the GLB bakes carry (Interchange may land
    # them with suffixed names) to the AM_<id>_<Clip> convention the runtime
    # derives (FAstrawildEchoMutator::BuildSciFantasyAnimPath).
    ar = unreal.AssetRegistryHelpers.get_asset_registry()
    for base_id in BASE_IDS:
        for clip in ("Idle", "Move", "Hit"):
            expected = f"{dest_folder}/AM_{base_id}_{clip}"
            if unreal.EditorAssetLibrary.does_asset_exist(expected):
                stage["anims"][f"AM_{base_id}_{clip}"] = expected
                continue
            found = False
            for asset in ar.get_assets_by_path(dest_folder, recursive=True):
                asset_name = str(asset.asset_name)
                cls = str(asset.asset_class_path.asset_name)
                if "AnimSequence" in cls and base_id in asset_name and clip.lower() in asset_name.lower():
                    try:
                        unreal.EditorAssetLibrary.rename_asset(str(asset.package_name), expected)
                        stage["anims"][f"AM_{base_id}_{clip}"] = expected
                        found = True
                    except Exception as e:
                        warn(f"clip rename for {base_id}_{clip} failed: {e}")
                    break
            if not found and clip != "Hit":  # Hit clips are optional at runtime
                REPORT["missing"].append(expected)
                warn(f"animation clip not found after import: {expected}")


# ---------------------------------------------------------------- sound
def import_echo_audio(root: str) -> None:
    stage = REPORT["stages"].setdefault("echo_audio", {"imported": 0, "failed": 0})
    src_dir = os.path.join(root, "ArtSource", "Audio", "Echoes")
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    dest_folder = "/Game/Audio/Echoes"

    if not os.path.isdir(src_dir):
        err(f"echo audio staging dir missing: {src_dir}")
        return

    for set_theme in SOUND_SETS:
        for index in (0, 1):
            cue_name = f"SFXSet_{set_theme}_{index}"
            src = os.path.join(src_dir, cue_name + ".wav")
            ue_path = f"{dest_folder}/{cue_name}"
            if not os.path.exists(src):
                REPORT["missing"].append(ue_path)
                warn(f"cue source missing on disk: {src}")
                continue
            try:
                if not unreal.EditorAssetLibrary.does_asset_exist(ue_path):
                    task = unreal.AssetImportTask()
                    task.filename = src
                    task.destination_path = dest_folder
                    task.destination_name = cue_name
                    task.automated = True
                    task.save = True
                    task.replace_existing = True
                    asset_tools.import_asset_tasks([task])
                cue = unreal.EditorAssetLibrary.load_asset(ue_path)
                if not cue:
                    raise RuntimeError(f"import produced no SoundWave at {ue_path}")
                stage["imported"] += 1
            except Exception as e:
                stage["failed"] += 1
                err(f"cue {cue_name}: {e}")
    log(f"echo audio: imported={stage['imported']} failed={stage['failed']}")


# ---------------------------------------------------------------- vfx
def create_element_niagara_systems() -> None:
    stage = REPORT["stages"].setdefault("element_vfx", {"created": 0})
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    for elem in ELEMENT_VFX:
        sys_path = f"/Game/VFX/NS_AW_Elem_{elem}"
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
            log(f"Created element Niagara template: {sys_path}")
        except Exception as e:
            warn(f"Could not create element Niagara system {sys_path}: {e}")


# ---------------------------------------------------------------- materials
def create_theme_master_materials() -> None:
    """Parameterized Sci-Fantasy master materials (directive Phase 4):

        Tint        (VectorParam)  -> BaseColor
        PatternTint (VectorParam) x GlowIntensity (ScalarParam) -> Emissive
        Metallic    / Roughness    (ScalarParam)  -> Metallic / Roughness

    One master per theme family, built with the SAME expression-graph API
    aw_materials.py uses (MaterialEditingLibrary). The runtime material-swap
    language (EAstrawildMutationMaterialTheme) maps onto these package paths.
    """
    stage = REPORT["stages"].setdefault("theme_materials", {"created": 0})
    mel = unreal.MaterialEditingLibrary
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat_folder = "/Game/Materials"

    for mat_name, theme, metallic_default, roughness_default, glow_default in THEME_MASTERS:
        mat_path = f"{mat_folder}/{mat_name}"
        if unreal.EditorAssetLibrary.does_asset_exist(mat_path):
            stage["created"] += 1
            continue
        try:
            mat = asset_tools.create_asset(mat_name, mat_folder, unreal.Material,
                                           unreal.MaterialFactoryNew())
            if not mat:
                raise RuntimeError("create_asset returned None")
            try:
                mel.delete_all_material_expressions(mat)
            except Exception:  # noqa: BLE001
                pass

            tint = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -900, -200)
            tint.set_editor_property("parameter_name", "Tint")
            tint.set_editor_property("default_value", unreal.LinearColor(0.5, 0.5, 0.5, 1.0))
            mel.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)

            pattern = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -900, 300)
            pattern.set_editor_property("parameter_name", "PatternTint")
            pattern.set_editor_property("default_value", unreal.LinearColor(0.05, 0.05, 0.05, 1.0))
            glow = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -900, 460)
            glow.set_editor_property("parameter_name", "GlowIntensity")
            glow.set_editor_property("default_value", glow_default)
            mul_e = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, -600, 360)
            mel.connect_material_expressions(pattern, "", mul_e, "A")
            mel.connect_material_expressions(glow, "", mul_e, "B")
            mel.connect_material_property(mul_e, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

            metal_p = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -900, 600)
            metal_p.set_editor_property("parameter_name", "Metallic")
            metal_p.set_editor_property("default_value", metallic_default)
            mel.connect_material_property(metal_p, "", unreal.MaterialProperty.MP_METALLIC)

            rough_p = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -900, 760)
            rough_p.set_editor_property("parameter_name", "Roughness")
            rough_p.set_editor_property("default_value", roughness_default)
            sat_r = mel.create_material_expression(mat, unreal.MaterialExpressionSaturate, -600, 780)
            mel.connect_material_expressions(rough_p, "", sat_r, "")
            mel.connect_material_property(sat_r, "", unreal.MaterialProperty.MP_ROUGHNESS)

            mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
            mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
            try:
                mel.update_material_after_render_data_change(mat)
                mel.recompile_material(mat)
            except Exception as e:  # noqa: BLE001
                warn(f"recompile {mat_name}: {e}")
            unreal.EditorAssetLibrary.save_asset(mat_path)
            stage["created"] += 1
            log(f"Created theme master: {mat_path}")
        except Exception as e:
            warn(f"theme master {mat_name}: {e} — author manually per runbook")


# ---------------------------------------------------------------- report
def verify_coverage() -> None:
    ok, missing = 0, []
    for base_id in BASE_IDS:
        path = f"/Game/Characters/Echoes/BaseMeshes/{base_id}"
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            ok += 1
        else:
            missing.append(path)
        for clip in ("Idle", "Move"):
            anim = f"/Game/Characters/Echoes/BaseMeshes/AM_{base_id}_{clip}"
            if unreal.EditorAssetLibrary.does_asset_exist(anim):
                ok += 1
            else:
                missing.append(anim)
    for set_theme in SOUND_SETS:
        for index in (0, 1):
            cue = f"/Game/Audio/Echoes/SFXSet_{set_theme}_{index}"
            if unreal.EditorAssetLibrary.does_asset_exist(cue):
                ok += 1
            else:
                missing.append(cue)
    for elem in ELEMENT_VFX:
        vfx = f"/Game/VFX/NS_AW_Elem_{elem}"
        if unreal.EditorAssetLibrary.does_asset_exist(vfx):
            ok += 1
        else:
            missing.append(vfx)

    REPORT["coverage"] = {"resolved": ok, "missing": missing}
    REPORT["missing"] = missing
    log(f"coverage: {ok} resolved, {len(missing)} missing")


def write_report(root: str) -> None:
    out_dir = os.path.join(root, "Saved", "AwPipelineReport")
    os.makedirs(out_dir, exist_ok=True)
    out_path = os.path.join(out_dir, "echo_base_report.json")
    REPORT["total_missing"] = len(REPORT["missing"])
    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(REPORT, f, indent=2)
    status = "PASS" if not REPORT["missing"] and not REPORT["errors"] else "NEEDS ATTENTION"
    log(f"report written: {out_path} -> {status}")
    unreal.log("=" * 60)
    unreal.log(f"[AwPipeline/EchoBases] FINAL: {status} "
               f"(errors={len(REPORT['errors'])}, warnings={len(REPORT['warnings'])}, "
               f"missing={len(REPORT['missing'])})")
    unreal.log("=" * 60)


def main() -> None:
    root = project_root()
    log(f"project root: {root}")

    try:
        import_base_meshes(root)
    except Exception as e:
        err(f"base mesh stage crashed: {e}\n{traceback.format_exc()}")

    try:
        import_echo_audio(root)
    except Exception as e:
        err(f"echo audio stage crashed: {e}\n{traceback.format_exc()}")

    try:
        create_element_niagara_systems()
    except Exception as e:
        err(f"element vfx stage crashed: {e}\n{traceback.format_exc()}")

    try:
        create_theme_master_materials()
    except Exception as e:
        err(f"theme material stage crashed: {e}\n{traceback.format_exc()}")

    try:
        verify_coverage()
    except Exception as e:
        err(f"verify stage crashed: {e}")

    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    except Exception as e:
        err(f"save failed: {e}")

    write_report(root)


if __name__ == "__main__":
    main()
