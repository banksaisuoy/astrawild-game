"""
ASTRAWILD AwPipeline — master materials, material instances and slot binding.

Masters:
  * M_Master_Surface — param-driven PBR surface (T_D/T_N/T_ORM/T_E +
    TintColor/RoughnessMul/RoughnessAdd/MetallicMul/EmissiveColor/EmissiveBoost).
    Everything in the pack is an instance of this master.
  * M_Landscape_SciFiFrontier — 4-layer world-aligned slope+height blend
    (Bioluminescent Grass / Cliff Granite / Fertile Soil / Beach Sand) for the
    procedural terrain (works on any mesh: PixelNormalWS + WorldPosition).

Instances live in /Game/Materials/Instances (MI_*) and are bound onto imported
mesh material slots by name (ArtSource manifest 'materials' slot contracts).
All operations are defensive — failures downgrade to warnings; C++ runtime
material binding covers anything this stage cannot set.
"""
import os

import unreal

M_MAT_PATH = "/Game/Materials"
M_INST_PATH = "/Game/Materials/Instances"
TEX_BASE = "/Game/Textures"


def log(msg: str) -> None:
    unreal.log("[AwMaterials] " + str(msg))


def warn(msg: str) -> None:
    unreal.log_warning("[AwMaterials] " + str(msg))


# ------------------------------------------------------------------ helpers
def _expr(mat, cls, x, y, **props):
    e = unreal.MaterialEditingLibrary.create_material_expression(mat, cls, x, y)
    for k, v in props.items():
        try:
            e.set_editor_property(k, v)
        except Exception as ex:  # noqa: BLE001
            warn(f"property {k}={v} on {cls.__name__}: {ex}")
    return e


def _connect(src, src_out, dst, dst_in):
    unreal.MaterialEditingLibrary.connect_material_expressions(src, src_out, dst, dst_in)


def _prop(src, src_out, prop):
    unreal.MaterialEditingLibrary.connect_material_property(src, src_out, prop)


def _tex_param(mat, name, texture, x, y):
    e = _expr(mat, unreal.MaterialExpressionTextureSampleParameter2D, x, y,
              parameter_name=name, texture=texture)
    return e


def _load_tex(name):
    tex = unreal.EditorAssetLibrary.load_asset(f"{TEX_BASE}/{name}")
    if not tex:
        warn(f"texture not loaded: {TEX_BASE}/{name}")
    return tex


def _create_asset(name, path, cls, factory):
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    ue_path = f"{path}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(ue_path):
        return unreal.EditorAssetLibrary.load_asset(ue_path)
    return asset_tools.create_asset(name, path, cls, factory)


def _world_normal_expr(mat, x, y):
    for cls_name in ("MaterialExpressionPixelNormalWS", "MaterialExpressionVertexNormalWS",
                     "MaterialExpressionWorldNormal"):
        cls = getattr(unreal, cls_name, None)
        if cls is not None:
            try:
                return _expr(mat, cls, x, y)
            except Exception:  # noqa: BLE001
                continue
    warn("no world-normal expression available")
    return None


# ------------------------------------------------------------------ masters
def build_master_surface() -> unreal.Material:
    mat = _create_asset("M_Master_Surface", M_MAT_PATH, unreal.Material,
                        unreal.MaterialFactoryNew())
    if mat is None:
        raise RuntimeError("could not create M_Master_Surface")
    mel = unreal.MaterialEditingLibrary
    try:
        mel.delete_all_material_expressions(mat)
    except Exception:  # noqa: BLE001
        pass
    white = _load_tex("T_DefaultORM")
    glow = _load_tex("T_Weapon_Glow_E")

    t_d = _tex_param(mat, "T_D", _load_tex("T_DefaultORM") or white, -1400, -200)
    t_n = _tex_param(mat, "T_N", _load_tex("T_Landscape_Granite_N") or white, -1400, 100)
    t_orm = _tex_param(mat, "T_ORM", _load_tex("T_DefaultORM"), -1400, 400)
    t_e = _tex_param(mat, "T_E", glow, -1400, 700)
    tint = _expr(mat, unreal.MaterialExpressionVectorParameter, -900, -220,
                 parameter_name="TintColor",
                 default_value=unreal.LinearColor(1, 1, 1, 1))
    rough_mul = _expr(mat, unreal.MaterialExpressionScalarParameter, -900, 330,
                      parameter_name="RoughnessMul", default_value=1.0)
    rough_add = _expr(mat, unreal.MaterialExpressionScalarParameter, -900, 430,
                      parameter_name="RoughnessAdd", default_value=0.0)
    metal_mul = _expr(mat, unreal.MaterialExpressionScalarParameter, -900, 530,
                      parameter_name="MetallicMul", default_value=1.0)
    emis_col = _expr(mat, unreal.MaterialExpressionVectorParameter, -900, 720,
                     parameter_name="EmissiveColor",
                     default_value=unreal.LinearColor(0, 0, 0, 1))
    emis_boost = _expr(mat, unreal.MaterialExpressionScalarParameter, -900, 830,
                       parameter_name="EmissiveBoost", default_value=0.0)

    mul_bc = _expr(mat, unreal.MaterialExpressionMultiply, -600, -180)
    _connect(t_d, "", mul_bc, "A")
    _connect(tint, "", mul_bc, "B")
    _prop(mul_bc, "", unreal.MaterialProperty.MP_BASE_COLOR)

    _prop(t_n, "", unreal.MaterialProperty.MP_NORMAL)

    mul_r1 = _expr(mat, unreal.MaterialExpressionMultiply, -600, 380)
    _connect(t_orm, "G", mul_r1, "A")
    _connect(rough_mul, "", mul_r1, "B")
    add_r = _expr(mat, unreal.MaterialExpressionAdd, -400, 400)
    _connect(mul_r1, "", add_r, "A")
    _connect(rough_add, "", add_r, "B")
    sat_r = _expr(mat, unreal.MaterialExpressionSaturate, -260, 410)
    _connect(add_r, "", sat_r, "")
    _prop(sat_r, "", unreal.MaterialProperty.MP_ROUGHNESS)

    mul_m = _expr(mat, unreal.MaterialExpressionMultiply, -600, 540)
    _connect(t_orm, "B", mul_m, "A")
    _connect(metal_mul, "", mul_m, "B")
    _prop(mul_m, "", unreal.MaterialProperty.MP_METALLIC)

    mul_e1 = _expr(mat, unreal.MaterialExpressionMultiply, -600, 740)
    _connect(t_e, "", mul_e1, "A")
    _connect(emis_col, "", mul_e1, "B")
    mul_e2 = _expr(mat, unreal.MaterialExpressionMultiply, -400, 760)
    _connect(mul_e1, "", mul_e2, "A")
    _connect(emis_boost, "", mul_e2, "B")
    _prop(mul_e2, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    try:
        mel.update_material_after_render_data_change(mat)
        mel.recompile_material(mat)
    except Exception as e:  # noqa: BLE001
        warn(f"recompile M_Master_Surface: {e}")
    unreal.EditorAssetLibrary.save_asset(f"{M_MAT_PATH}/M_Master_Surface")
    log("M_Master_Surface built")
    return mat


def build_landscape_master() -> unreal.Material:
    mat = _create_asset("M_Landscape_SciFiFrontier", M_MAT_PATH, unreal.Material,
                        unreal.MaterialFactoryNew())
    if mat is None:
        raise RuntimeError("could not create M_Landscape_SciFiFrontier")
    mel = unreal.MaterialEditingLibrary
    try:
        mel.delete_all_material_expressions(mat)
    except Exception:  # noqa: BLE001
        pass

    tex = {
        "grass_d": _load_tex("T_Landscape_Grass_D"),
        "grass_n": _load_tex("T_Landscape_Grass_N"),
        "rock_d": _load_tex("T_Landscape_Granite_D"),
        "rock_n": _load_tex("T_Landscape_Granite_N"),
        "soil_d": _load_tex("T_Landscape_Soil_D"),
        "soil_n": _load_tex("T_Landscape_Soil_N"),
        "sand_d": _load_tex("T_Landscape_Sand_D"),
        "sand_n": _load_tex("T_Landscape_Sand_N"),
    }

    wpos = _expr(mat, unreal.MaterialExpressionWorldPosition, -2200, 100)
    wpos_mask = _expr(mat, unreal.MaterialExpressionComponentMask, -2000, 100,
                      r=True, g=True, b=False, a=False)
    _connect(wpos, "", wpos_mask, "")
    tile = _expr(mat, unreal.MaterialExpressionScalarParameter, -2000, 260,
                 parameter_name="TilingScale", default_value=0.013)
    uv = _expr(mat, unreal.MaterialExpressionMultiply, -1800, 160)
    _connect(wpos_mask, "", uv, "A")
    _connect(tile, "", uv, "B")

    def scaled_uv(x, y, mult):
        c = _expr(mat, unreal.MaterialExpressionConstant, x - 90, y + 24, r=mult)
        m = _expr(mat, unreal.MaterialExpressionMultiply, x, y)
        _connect(uv, "", m, "A")
        _connect(c, "", m, "B")
        return m

    def sample(texture, uv_in, x, y):
        s = _expr(mat, unreal.MaterialExpressionTextureSample, x, y, texture=texture)
        _connect(uv_in, "", s, "UVs")
        return s

    # ---- layer weights
    normal_ws = _world_normal_expr(mat, -2200, 700)
    up = _expr(mat, unreal.MaterialExpressionConstant3Vector, -2200, 840,
               constant=unreal.LinearColor(0, 0, 1, 0))
    slope = _expr(mat, unreal.MaterialExpressionDotProduct, -2000, 780)
    _connect(normal_ws, "", slope, "A")
    _connect(up, "", slope, "B")
    slope_pow = _expr(mat, unreal.MaterialExpressionPower, -1800, 800)
    _connect(slope, "", slope_pow, "Base")
    slope_exp = _expr(mat, unreal.MaterialExpressionScalarParameter, -1800, 960,
                      parameter_name="GrassSlopePower", default_value=3.0)
    _connect(slope_exp, "", slope_pow, "Exponent")

    # soil: mid-slope band
    soil_band = _expr(mat, unreal.MaterialExpressionSaturate, -1600, 1040)
    slope_mid = _expr(mat, unreal.MaterialExpressionAdd, -1800, 1040)
    _connect(slope, "", slope_mid, "A")
    mid_c = _expr(mat, unreal.MaterialExpressionConstant, -1800, 1160, r=-0.75)
    _connect(mid_c, "", slope_mid, "B")
    soil_sharp = _expr(mat, unreal.MaterialExpressionMultiply, -1600, 1080)
    _connect(slope_mid, "", soil_sharp, "A")
    soil_k = _expr(mat, unreal.MaterialExpressionScalarParameter, -1600, 1200,
                   parameter_name="SoilBandSharpness", default_value=4.0)
    _connect(soil_k, "", soil_sharp, "B")
    _connect(soil_sharp, "", soil_band, "")
    one_minus_band = _expr(mat, unreal.MaterialExpressionOneMinus, -1440, 1040)
    _connect(soil_band, "", one_minus_band, "")
    soil_strength = _expr(mat, unreal.MaterialExpressionScalarParameter, -1440, 1180,
                          parameter_name="SoilStrength", default_value=0.55)
    soil_w = _expr(mat, unreal.MaterialExpressionMultiply, -1280, 1060)
    _connect(one_minus_band, "", soil_w, "A")
    _connect(soil_strength, "", soil_w, "B")

    # sand: below SandZ (cm)
    z_mask = _expr(mat, unreal.MaterialExpressionComponentMask, -2200, 460,
                   r=False, g=False, b=True, a=False)
    _connect(wpos, "", z_mask, "")
    sand_z = _expr(mat, unreal.MaterialExpressionScalarParameter, -2000, 500,
                   parameter_name="SandZ", default_value=0.0)
    sand_blend = _expr(mat, unreal.MaterialExpressionScalarParameter, -2000, 620,
                       parameter_name="SandBlendHeight", default_value=300.0)
    sand_delta = _expr(mat, unreal.MaterialExpressionAdd, -1800, 480)
    _connect(sand_z, "", sand_delta, "A")
    half = _expr(mat, unreal.MaterialExpressionConstant, -2000, 720, r=0.5)
    sand_half = _expr(mat, unreal.MaterialExpressionMultiply, -1800, 620)
    _connect(sand_blend, "", sand_half, "A")
    _connect(half, "", sand_half, "B")
    sand_rel = _expr(mat, unreal.MaterialExpressionAdd, -1600, 500)
    _connect(sand_delta, "", sand_rel, "A")
    _connect(sand_half, "", sand_rel, "B")
    sand_frac = _expr(mat, unreal.MaterialExpressionDivide, -1600, 620)
    _connect(sand_rel, "", sand_frac, "A")
    _connect(sand_blend, "", sand_frac, "B")
    sand_w = _expr(mat, unreal.MaterialExpressionSaturate, -1400, 560)
    _connect(sand_frac, "", sand_w, "")

    # ---- textures per layer (D + N)
    g_uv = scaled_uv(-1600, 180, 1.0)
    r_uv = scaled_uv(-1600, 280, 0.35)
    s_uv = scaled_uv(-1600, 380, 1.35)
    a_uv = scaled_uv(-1600, 480, 1.8)
    g_d = sample(tex["grass_d"], g_uv, -1400, 160)
    g_n = sample(tex["grass_n"], g_uv, -1400, 260)
    r_d = sample(tex["rock_d"], r_uv, -1400, 300)
    r_n = sample(tex["rock_n"], r_uv, -1400, 400)
    s_d = sample(tex["soil_d"], s_uv, -1400, 440)
    s_n = sample(tex["soil_n"], s_uv, -1400, 540)
    a_d = sample(tex["sand_d"], a_uv, -1400, 580)
    a_n = sample(tex["sand_n"], a_uv, -1400, 680)

    # ---- blend: base = lerp(granite, grass, slopePow); mid = lerp(base, soil); final = lerp(mid, sand)
    for prop, d_layers, n_layers, roughs in (
        (unreal.MaterialProperty.MP_BASE_COLOR, (r_d, g_d, s_d, a_d), None, (0.92, 0.78, 0.85, 0.72)),
        (unreal.MaterialProperty.MP_NORMAL, (r_n, g_n, s_n, a_n), None, None),
    ):
        l1 = _expr(mat, unreal.MaterialExpressionLinearInterpolate, -900, 120)
        _connect(d_layers[0], "", l1, "A")
        _connect(d_layers[1], "", l1, "B")
        _connect(slope_pow, "", l1, "Alpha")
        l2 = _expr(mat, unreal.MaterialExpressionLinearInterpolate, -600, 140)
        _connect(l1, "", l2, "A")
        _connect(d_layers[2], "", l2, "B")
        _connect(soil_w, "", l2, "Alpha")
        l3 = _expr(mat, unreal.MaterialExpressionLinearInterpolate, -320, 160)
        _connect(l2, "", l3, "A")
        _connect(d_layers[3], "", l3, "B")
        _connect(sand_w, "", l3, "Alpha")
        _prop(l3, "", prop)

    # roughness: nested lerp of constants
    c_r = [_expr(mat, unreal.MaterialExpressionConstant, -900 + i * 120, 480, r=v)
           for i, v in enumerate((0.92, 0.78, 0.85, 0.72))]
    lr1 = _expr(mat, unreal.MaterialExpressionLinearInterpolate, -500, 440)
    _connect(c_r[0], "", lr1, "A")
    _connect(c_r[1], "", lr1, "B")
    _connect(slope_pow, "", lr1, "Alpha")
    lr2 = _expr(mat, unreal.MaterialExpressionLinearInterpolate, -260, 460)
    _connect(lr1, "", lr2, "A")
    _connect(c_r[2], "", lr2, "B")
    _connect(soil_w, "", lr2, "Alpha")
    lr3 = _expr(mat, unreal.MaterialExpressionLinearInterpolate, -60, 480)
    _connect(lr2, "", lr3, "A")
    _connect(c_r[3], "", lr3, "B")
    _connect(sand_w, "", lr3, "Alpha")
    _prop(lr3, "", unreal.MaterialProperty.MP_ROUGHNESS)

    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_DEFAULT_LIT)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property("two_sided", True)
    try:
        mel.update_material_after_render_data_change(mat)
        mel.recompile_material(mat)
    except Exception as e:  # noqa: BLE001
        warn(f"recompile landscape: {e}")
    unreal.EditorAssetLibrary.save_asset(f"{M_MAT_PATH}/M_Landscape_SciFiFrontier")
    log("M_Landscape_SciFiFrontier built")
    return mat


# ---------------------------------------------------------------- instances
ELEMENT_COLORS = {
    "Terraquill": (0.25, 0.85, 0.35), "Cindermule": (0.95, 0.45, 0.15),
    "Voltpylon": (0.20, 0.85, 0.90), "Bastionbeetle": (0.55, 0.45, 0.90),
    "Mistmender": (0.95, 0.85, 0.50), "Deepdelver": (0.20, 0.75, 0.70),
}
FAMILY_GLOW = {
    "ScrapRifle": (0.95, 0.60, 0.20), "PlasmaCarbine": (0.80, 0.30, 0.90),
    "ArcCannon": (0.20, 0.85, 0.90), "Railgun": (0.60, 0.40, 1.00),
    "SingularityCannon": (0.29, 0.86, 0.78),
}
NODE_GLOW = {
    "Astraite": (0.25, 0.90, 0.80), "Pyronite": (1.00, 0.45, 0.10),
    "Voidstone": (0.60, 0.40, 1.00), "AncientVein": (0.95, 0.80, 0.45),
}


def _lc(rgb, a=1.0):
    return unreal.LinearColor(rgb[0], rgb[1], rgb[2], a)


def _make_instance(name, master, tex_d=None, tex_n=None, tex_orm=None, tex_e=None,
                   tint=None, rough_mul=None, rough_add=None, metal_mul=None,
                   emis_color=None, emis_boost=None):
    mi = _create_asset(name, M_INST_PATH, unreal.MaterialInstanceConstant,
                       unreal.MaterialInstanceConstantFactoryNew())
    if mi is None:
        warn(f"could not create instance {name}")
        return None
    try:
        mi.set_editor_property("parent", master)
    except Exception as e:  # noqa: BLE001
        warn(f"parent on {name}: {e}")
    mel = unreal.MaterialEditingLibrary

    def set_tex(param, value):
        if value is None:
            return
        try:
            mel.set_material_instance_texture_parameter_value(mi, param, value)
        except Exception as e:  # noqa: BLE001
            warn(f"{name}: texture param {param}: {e}")

    def set_scalar(param, value):
        try:
            mel.set_material_instance_scalar_parameter_value(mi, param, value)
        except Exception as e:  # noqa: BLE001
            warn(f"{name}: scalar param {param}: {e}")

    def set_vector(param, value):
        try:
            mel.set_material_instance_vector_parameter_value(mi, param, value)
        except Exception as e:  # noqa: BLE001
            warn(f"{name}: vector param {param}: {e}")

    set_tex("T_D", tex_d)
    set_tex("T_N", tex_n)
    set_tex("T_ORM", tex_orm)
    set_tex("T_E", tex_e)
    if tint:
        set_vector("TintColor", _lc(tint))
    if rough_mul is not None:
        set_scalar("RoughnessMul", rough_mul)
    if rough_add is not None:
        set_scalar("RoughnessAdd", rough_add)
    if metal_mul is not None:
        set_scalar("MetallicMul", metal_mul)
    if emis_color:
        set_vector("EmissiveColor", _lc(emis_color))
    if emis_boost is not None:
        set_scalar("EmissiveBoost", emis_boost)
    unreal.EditorAssetLibrary.save_asset(f"{M_INST_PATH}/{name}")
    return mi


def build_instances(master) -> dict:
    """Returns slot-name -> instance asset map (global slots) plus per-asset maps."""
    T = {n: _load_tex(n) for n in (
        "T_Survivor_Exosuit_D", "T_Survivor_Exosuit_N", "T_Survivor_ORM",
        "T_Survivor_Visor_E", "T_Weapon_Glow_E", "T_Echo_Body_D", "T_Echo_Body_N",
        "T_Echo_Body_ORM", "T_Echo_Emissive_M", "T_Weapon_Scraps_D", "T_Weapon_Scraps_N",
        "T_Weapon_Scraps_ORM", "T_Weapon_Tech_D", "T_Weapon_Tech_N", "T_Weapon_Tech_ORM",
        "T_Foliage_Canopy_D", "T_Foliage_Canopy_N", "T_Bark_D", "T_Bark_N",
        "T_Rock_D", "T_Rock_N", "T_Ruin_D", "T_Ruin_N", "T_Crystal_D", "T_Crystal_N",
        "T_Crystal_E", "T_Foliage_Spore_E", "T_Vehicle_Hull_D", "T_Vehicle_Hull_N",
        "T_Vehicle_Hull_ORM", "T_Vehicle_Glow_E", "T_DefaultORM",
        "T_Landscape_Granite_N")}
    inst = {}

    def add(slot, name, **kw):
        mi = _make_instance(name, master, **kw)
        if mi:
            inst[slot] = mi
        return mi

    # survivor
    add("Survivor_Suit", "MI_Survivor_Suit", tex_d=T["T_Survivor_Exosuit_D"],
        tex_n=T["T_Survivor_Exosuit_N"], tex_orm=T["T_Survivor_ORM"], rough_mul=1.0, metal_mul=0.15)
    add("Survivor_Armor", "MI_Survivor_Armor", tex_d=T["T_Survivor_Exosuit_D"],
        tex_n=T["T_Survivor_Exosuit_N"], tex_orm=T["T_Survivor_ORM"], rough_mul=0.6, metal_mul=1.5)
    add("Survivor_Accent", "MI_Survivor_Accent", tex_d=T["T_Survivor_Exosuit_D"],
        tex_n=T["T_Survivor_Exosuit_N"], tex_orm=T["T_Survivor_ORM"], rough_mul=0.5, metal_mul=1.2,
        tint=(1.1, 0.85, 0.5))
    add("Survivor_Visor", "MI_Survivor_Visor", tex_d=T["T_Survivor_Exosuit_D"],
        tex_n=T["T_Survivor_Exosuit_N"], tex_orm=T["T_DefaultORM"], tex_e=T["T_Survivor_Visor_E"],
        emis_color=(0.29, 0.86, 0.78), emis_boost=4.0, rough_mul=0.3, metal_mul=0.2)
    add("Survivor_Scanner", "MI_Survivor_Scanner", tex_d=T["T_Weapon_Tech_D"],
        tex_n=T["T_Weapon_Tech_N"], tex_orm=T["T_Weapon_Tech_ORM"], tex_e=T["T_Weapon_Glow_E"],
        emis_color=(0.91, 0.60, 0.19), emis_boost=3.5, rough_mul=0.5, metal_mul=0.8)
    add("Survivor_Thruster", "MI_Survivor_Thruster", tex_d=T["T_Weapon_Tech_D"],
        tex_n=T["T_Weapon_Tech_N"], tex_orm=T["T_Weapon_Tech_ORM"], tex_e=T["T_Weapon_Glow_E"],
        emis_color=(0.29, 0.86, 0.78), emis_boost=3.0, rough_mul=0.4, metal_mul=0.9)
    # echo body
    add("Echo_Body", "MI_Echo_Body", tex_d=T["T_Echo_Body_D"], tex_n=T["T_Echo_Body_N"],
        tex_orm=T["T_Echo_Body_ORM"], rough_mul=1.0, metal_mul=0.1)
    add("Echo_Armor", "MI_Echo_Armor", tex_d=T["T_Echo_Body_D"], tex_n=T["T_Echo_Body_N"],
        tex_orm=T["T_Echo_Body_ORM"], rough_mul=0.6, metal_mul=1.3)
    # per-species glow
    species_map = {}
    for species, color in ELEMENT_COLORS.items():
        mi = _make_instance(f"MI_Echo_Glow_{species}", master,
                            tex_d=T["T_DefaultORM"], tex_n=T["T_Landscape_Granite_N"],
                            tex_orm=T["T_DefaultORM"], tex_e=T["T_Echo_Emissive_M"],
                            emis_color=color, emis_boost=3.2, rough_mul=0.35, metal_mul=0.3)
        if mi:
            species_map[species] = mi
    # weapons
    add("Weapon_Body_Scraps", "MI_Weapon_Scraps", tex_d=T["T_Weapon_Scraps_D"],
        tex_n=T["T_Weapon_Scraps_N"], tex_orm=T["T_Weapon_Scraps_ORM"], rough_mul=1.0, metal_mul=0.6)
    add("Weapon_Body_Tech", "MI_Weapon_Tech", tex_d=T["T_Weapon_Tech_D"],
        tex_n=T["T_Weapon_Tech_N"], tex_orm=T["T_Weapon_Tech_ORM"], rough_mul=0.8, metal_mul=0.7)
    add("Weapon_Metal_Scraps", "MI_Weapon_Metal_Scraps", tex_d=T["T_Weapon_Scraps_D"],
        tex_n=T["T_Weapon_Scraps_N"], tex_orm=T["T_Weapon_Scraps_ORM"], rough_mul=0.5, metal_mul=1.3)
    add("Weapon_Metal_Tech", "MI_Weapon_Metal_Tech", tex_d=T["T_Weapon_Tech_D"],
        tex_n=T["T_Weapon_Tech_N"], tex_orm=T["T_Weapon_Tech_ORM"], rough_mul=0.45, metal_mul=1.4)
    add("Weapon_Grip", "MI_Weapon_Grip", tex_d=T["T_Weapon_Scraps_D"],
        tex_n=T["T_Weapon_Scraps_N"], tex_orm=T["T_Weapon_Scraps_ORM"], rough_mul=1.5, metal_mul=0.05)
    add("Weapon_Coil", "MI_Weapon_Coil", tex_d=T["T_Weapon_Scraps_D"],
        tex_n=T["T_Weapon_Scraps_N"], tex_orm=T["T_Weapon_Scraps_ORM"], tint=(1.2, 0.8, 0.55),
        rough_mul=0.5, metal_mul=1.2)
    # foliage / rock / ruins
    add("Foliage_Bark", "MI_Foliage_Bark", tex_d=T["T_Bark_D"], tex_n=T["T_Bark_N"],
        tex_orm=T["T_DefaultORM"], rough_mul=1.1, metal_mul=0.0)
    add("Foliage_Canopy", "MI_Foliage_Canopy", tex_d=T["T_Foliage_Canopy_D"],
        tex_n=T["T_Foliage_Canopy_N"], tex_orm=T["T_DefaultORM"], rough_mul=0.9, metal_mul=0.0)
    add("Foliage_Glow", "MI_Foliage_Glow", tex_d=T["T_DefaultORM"],
        tex_n=T["T_Landscape_Granite_N"], tex_orm=T["T_DefaultORM"],
        tex_e=T["T_Foliage_Spore_E"], emis_color=(0.29, 0.86, 0.78), emis_boost=2.6)
    add("Rock_Granite", "MI_Rock", tex_d=T["T_Rock_D"], tex_n=T["T_Rock_N"],
        tex_orm=T["T_DefaultORM"], rough_mul=1.0, metal_mul=0.05)
    add("Rock_Moss", "MI_Rock_Moss", tex_d=T["T_Rock_D"], tex_n=T["T_Rock_N"],
        tex_orm=T["T_DefaultORM"], tint=(0.45, 0.75, 0.55), rough_mul=1.0, metal_mul=0.0)
    add("Ruin_Stone", "MI_Ruin", tex_d=T["T_Ruin_D"], tex_n=T["T_Ruin_N"],
        tex_orm=T["T_DefaultORM"], rough_mul=1.0, metal_mul=0.05)
    add("Ruin_Glow", "MI_Ruin_Glow", tex_d=T["T_Ruin_D"], tex_n=T["T_Ruin_N"],
        tex_orm=T["T_DefaultORM"], tex_e=T["T_Crystal_E"], emis_color=(0.29, 0.86, 0.78),
        emis_boost=1.6)
    add("Node_Rock", "MI_Node_Rock", tex_d=T["T_Rock_D"], tex_n=T["T_Rock_N"],
        tex_orm=T["T_DefaultORM"], rough_mul=1.0, metal_mul=0.05)
    # vehicle
    add("Vehicle_Hull", "MI_Vehicle_Hull", tex_d=T["T_Vehicle_Hull_D"],
        tex_n=T["T_Vehicle_Hull_N"], tex_orm=T["T_Vehicle_Hull_ORM"], rough_mul=1.0, metal_mul=0.3)
    add("Vehicle_Metal", "MI_Vehicle_Metal", tex_d=T["T_Vehicle_Hull_D"],
        tex_n=T["T_Vehicle_Hull_N"], tex_orm=T["T_Vehicle_Hull_ORM"], rough_mul=0.55, metal_mul=1.4)
    add("Vehicle_Accent", "MI_Vehicle_Accent", tex_d=T["T_Vehicle_Hull_D"],
        tex_n=T["T_Vehicle_Hull_N"], tex_orm=T["T_Vehicle_Hull_ORM"], tint=(1.3, 0.95, 0.55),
        rough_mul=0.6, metal_mul=0.9)
    add("Vehicle_Glow", "MI_Vehicle_Glow", tex_d=T["T_Weapon_Tech_D"],
        tex_n=T["T_Weapon_Tech_N"], tex_orm=T["T_Weapon_Tech_ORM"], tex_e=T["T_Vehicle_Glow_E"],
        emis_color=(0.29, 0.86, 0.78), emis_boost=3.0)
    # crystal nodes + FX crystals
    node_map = {}
    for node_name, color in NODE_GLOW.items():
        mi = _make_instance(f"MI_Crystal_{node_name}", master,
                            tex_d=T["T_Crystal_D"], tex_n=T["T_Crystal_N"],
                            tex_orm=T["T_DefaultORM"], tex_e=T["T_Crystal_E"],
                            emis_color=color, emis_boost=2.8, rough_mul=0.25, metal_mul=0.1)
        if mi:
            node_map[node_name] = mi
    # per weapon family glow
    family_map = {}
    for fam, color in FAMILY_GLOW.items():
        mi = _make_instance(f"MI_Weapon_Glow_{fam}", master,
                            tex_d=T["T_Weapon_Tech_D"], tex_n=T["T_Weapon_Tech_N"],
                            tex_orm=T["T_Weapon_Tech_ORM"], tex_e=T["T_Weapon_Glow_E"],
                            emis_color=color, emis_boost=3.4, rough_mul=0.3, metal_mul=0.4)
        if mi:
            family_map[fam] = mi
    return {"slots": inst, "species": species_map, "nodes": node_map, "families": family_map,
            "textures": T}


# ------------------------------------------------------------------- binding
def _slot_map_for_asset(asset_name, slot, maps):
    """Resolves a glTF slot name to a material instance for a given asset."""
    inst = maps["slots"]
    families = maps["families"]
    species = maps["species"]
    nodes = maps["nodes"]
    # species glow slots
    for sp in species:
        if sp in asset_name and slot in ("Echo_Emissive", "Echo_Eye"):
            return species[sp]
    # node crystal slots
    for n in nodes:
        if n in asset_name and slot in ("Node_Crystal", "Crystal"):
            return nodes[n]
    # weapon family glow + body style
    if slot == "Weapon_Glow":
        for fam in families:
            if fam in asset_name:
                return families[fam]
        return families.get("ScrapRifle")
    if slot == "Weapon_Body":
        return inst.get("Weapon_Body_Scraps" if "ScrapRifle" in asset_name else "Weapon_Body_Tech")
    if slot == "Weapon_Metal":
        return inst.get("Weapon_Metal_Scraps" if "ScrapRifle" in asset_name else "Weapon_Metal_Tech")
    return inst.get(slot)


def bind_mesh_materials(manifest, maps) -> None:
    bound, missed = 0, []
    for name, info in manifest.get("assets", {}).items():
        if info.get("category") != "mesh":
            continue
        ue_path = info.get("ue_path", "")
        mesh = unreal.EditorAssetLibrary.load_asset(ue_path)
        if not mesh:
            continue
        try:
            if isinstance(mesh, unreal.StaticMesh):
                slots = mesh.get_editor_property("static_materials") or []
                new_slots = []
                changed = False
                for sm_mat in slots:
                    slot_name = str(sm_mat.get_editor_property("material_slot_name"))
                    mi = _slot_map_for_asset(name, slot_name, maps)
                    if mi:
                        sm_mat.set_editor_property("material_interface", mi)
                        changed = True
                        bound += 1
                    new_slots.append(sm_mat)
                if changed:
                    mesh.set_editor_property("static_materials", new_slots)
                    unreal.EditorAssetLibrary.save_asset(ue_path)
            elif isinstance(mesh, unreal.SkeletalMesh):
                slots = mesh.get_editor_property("materials") or []
                new_slots = []
                changed = False
                for sk_mat in slots:
                    slot_name = str(sk_mat.get_editor_property("material_slot_name"))
                    mi = _slot_map_for_asset(name, slot_name, maps)
                    if mi:
                        try:
                            sk_mat.set_editor_property("material_interface", mi)
                        except Exception:  # noqa: BLE001
                            sk_mat.set_editor_property("material", mi)
                        changed = True
                        bound += 1
                    new_slots.append(sk_mat)
                if changed:
                    mesh.set_editor_property("materials", new_slots)
                    unreal.EditorAssetLibrary.save_asset(ue_path)
        except Exception as e:  # noqa: BLE001
            missed.append(f"{ue_path}: {e}")
    for m in missed:
        warn(f"bind failed: {m}")
    log(f"slot bindings applied: {bound} (failures: {len(missed)})")


def build_materials(manifest) -> None:
    master = build_master_surface()
    try:
        build_landscape_master()
    except Exception as e:  # noqa: BLE001
        warn(f"landscape master failed: {e}")
    maps = build_instances(master)
    bind_mesh_materials(manifest, maps)
