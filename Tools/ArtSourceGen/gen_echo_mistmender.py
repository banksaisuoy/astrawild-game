"""
ASTRAWILD ArtSourceGen — SK_Echo_Mistmender (Light Echo, manta-wisp healer).

Original ASTRAWILD creature: a levitating manta-wisp healer — flattened pale
mist body disc, two wing fins per side (inner + outer, two bones each),
two trailing veil ribbons, a Light-gold core sphere tucked under the body,
four small halo crystal shards floating above, two bright eyes. NO legs.
Disc span 0.85m, floats with root at y=0.9.

Rig: Root(0,0.9,0)/Body + Wing_L_01/02 + Wing_R_01/02 + Veil_01/02 + Core.
10 bones.
Anims: AM_Mistmender_Idle (4.0s root bob y +/-0.05 + wing ripple),
AM_Mistmender_Move (1.3s wing flap Y +/-22deg alternating + body pitch
+/-4deg), AM_Mistmender_Hit (0.4s wings tuck Z + body dips -0.08).
Material slots: Echo_Body / Echo_Armor / Echo_Emissive / Echo_Eye.

Run:  python3 gen_echo_mistmender.py
Output: ArtSource/Meshes/Echoes/SK_Echo_Mistmender.glb
"""
from __future__ import annotations

import math
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from aw_gltf import GlbBuilder, Material, validate_glb
from aw_manifest import record
from aw_rig import Rig
from aw_shapes import (MeshBuilder, box, cone, crystal, cylinder, lathe,
                       mirror_x, recompute_smooth_normals, sphere, torus,
                       translate, rotate, scale, tube)

OUT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__),
                          "..", "..", "ArtSource", "Meshes", "Echoes"))
OUT_PATH = os.path.join(OUT_DIR, "SK_Echo_Mistmender.glb")
SPECIES = "Mistmender"
DEG = math.pi / 180.0

# ----------------------------------------------------------------- materials
BODY = (0.75, 0.78, 0.82, 1.0)       # pale mist
ARMOR = (0.62, 0.64, 0.68, 1.0)      # pale grey fin spar
LIGHT = (0.95, 0.85, 0.5)            # Light GOLD emissive

MATS = {
    "Echo_Body":     Material("Echo_Body", base_color=BODY, roughness=0.75, metallic=0.1),
    "Echo_Armor":    Material("Echo_Armor", base_color=ARMOR, roughness=0.45, metallic=0.55),
    "Echo_Emissive": Material("Echo_Emissive", base_color=(0.06, 0.05, 0.02, 1.0),
                              roughness=0.4, metallic=0.0, emissive=LIGHT),
    "Echo_Eye":      Material("Echo_Eye", base_color=(0.08, 0.07, 0.03, 1.0),
                              roughness=0.25, metallic=0.0,
                              emissive=(1.0, 0.95, 0.7)),
}


# ------------------------------------------------------------- local helpers
def _rot_sine(rig: Rig, anim, bone: str, dur: float, n: int, amp_deg: float,
              phase: float = 0.0, ax: str = "x") -> None:
    idx = "xyz".index(ax)
    keys = []
    for i in range(n + 1):
        t = dur * i / n
        v = math.radians(amp_deg) * math.sin(2.0 * math.pi * (i / n) + phase)
        e = [0.0, 0.0, 0.0]
        e[idx] = v
        keys.append((t, tuple(e)))
    rig.add_rotation_channel(anim, bone, keys)


def _tr_sine(rig: Rig, anim, bone: str, dur: float, n: int, dy: float,
             freq: float = 1.0, phase: float = 0.0) -> None:
    keys = []
    for i in range(n + 1):
        t = dur * i / n
        v = dy * math.sin(2.0 * math.pi * freq * (i / n) + phase)
        keys.append((t, (0.0, v, 0.0)))
    rig.add_translation_channel(anim, bone, keys)


# --------------------------------------------------------------------- build
def build_rig(builder: GlbBuilder) -> Rig:
    rig = Rig(builder, root_name="Root", root_position=(0, 0.9, 0))
    rig.add_bone("Body", "Root", (0, 0, 0), direction=(0, 0.35, 1), length=0.17)
    for s, tag in ((-1, "L"), (1, "R")):
        rig.add_bone(f"Wing_{tag}_01", "Body", (s * 0.15, 0.005, 0.01),
                     direction=(s * 1, 0.05, 0.1), length=0.17)
        rig.add_bone(f"Wing_{tag}_02", f"Wing_{tag}_01", (s * 0.17, 0, 0.005),
                     direction=(s * 1, 0, -0.08), length=0.11)
    rig.add_bone("Veil_01", "Body", (0, -0.04, -0.13), direction=(0, -0.35, -1), length=0.17)
    rig.add_bone("Veil_02", "Veil_01", (0, -0.03, -0.165), direction=(0, -0.4, -1), length=0.16)
    rig.add_bone("Core", "Body", (0, -0.07, 0.0), direction=(0, -1, 0), length=0.06)
    return rig


def build_body(mb: MeshBuilder) -> None:
    T = translate

    # --- flattened sphere body
    mb.add(T(scale(sphere(0.16, 12, 8), (1.05, 0.45, 1.25)), (0, 0.90, 0.02)), "Echo_Body")

    # --- 2 wing fins per side (scaled boxes angled out) + armor leading spars
    for sx, t1, t2 in ((-1, -4.0, -12.0), (1, 4.0, 12.0)):
        mb.add(T(rotate(box((0.17, 0.018, 0.13)), 0, 0, t1 * DEG),
                 (sx * 0.245, 0.905, 0.015)), "Echo_Body")
        mb.add(T(rotate(box((0.11, 0.014, 0.10)), 0, 0, t2 * DEG),
                 (sx * 0.375, 0.893, 0.0)), "Echo_Body")
        mb.add(T(rotate(box((0.16, 0.02, 0.018)), 0, 0, t1 * DEG),
                 (sx * 0.245, 0.906, 0.075)), "Echo_Armor")
        mb.add(T(rotate(box((0.10, 0.016, 0.015)), 0, 0, t2 * DEG),
                 (sx * 0.372, 0.894, 0.045)), "Echo_Armor")

    # --- 2 trailing veil ribbons (thin long boxes, pale tint)
    mb.add(T(rotate(box((0.07, 0.012, 0.18)), 10 * DEG, 0, 0), (0, 0.862, -0.22)), "Echo_Body")
    mb.add(T(rotate(box((0.05, 0.010, 0.17)), 18 * DEG, 0, 0), (0, 0.805, -0.385)), "Echo_Body")

    # --- core sphere under the body (Light GOLD)
    mb.add(T(sphere(0.07, 8, 6), (0, 0.845, 0.0)), "Echo_Emissive")

    # --- 4 small halo shards above the body
    for sx, t in ((-1, -18.0), (1, 18.0)):
        mb.add(T(rotate(crystal(0.085, 0.015, 5), 0, 0, t * DEG),
                 (sx * 0.10, 0.99, -0.03)), "Echo_Emissive")
    for sx, t in ((-1, -30.0), (1, 30.0)):
        mb.add(T(rotate(crystal(0.07, 0.013, 5), 0, 0, t * DEG),
                 (sx * 0.05, 1.005, 0.10)), "Echo_Emissive")

    # --- eyes
    for sx in (-1, 1):
        mb.add(T(sphere(0.014, 6, 4), (sx * 0.05, 0.925, 0.205)), "Echo_Eye")


# ----------------------------------------------------------------- animation
def anim_idle(builder: GlbBuilder, rig: Rig) -> None:
    """4.0s — root bob y +/-0.05 sine, wings Y +/-6deg ripple offset phases."""
    anim = builder.add_animation("AM_Mistmender_Idle")
    dur, n = 4.0, 10
    _tr_sine(rig, anim, "Root", dur, n, 0.05)
    for tag in ("L", "R"):
        _rot_sine(rig, anim, f"Wing_{tag}_01", dur, n, 6.0, 0.0, "y")
        _rot_sine(rig, anim, f"Wing_{tag}_02", dur, n, 6.0, 1.0, "y")
    for tag, s in (("L", -1), ("R", 1)):
        _rot_sine(rig, anim, f"Wing_{tag}_01", dur, n, s * 1.5, 0.5, "z")
    _rot_sine(rig, anim, "Body", dur, n, 1.5, 0.5)
    _rot_sine(rig, anim, "Veil_01", dur, n, 2.5, 1.3, "y")
    _rot_sine(rig, anim, "Veil_02", dur, n, 3.5, 1.8, "y")


def anim_move(builder: GlbBuilder, rig: Rig) -> None:
    """1.3s — wings flap Y +/-22deg alternating, body pitch X +/-4deg."""
    anim = builder.add_animation("AM_Mistmender_Move")
    dur, n = 1.3, 10
    _rot_sine(rig, anim, "Wing_L_01", dur, n, 22.0, 0.0, "y")
    _rot_sine(rig, anim, "Wing_L_02", dur, n, 22.0, 0.6, "y")
    _rot_sine(rig, anim, "Wing_R_01", dur, n, 22.0, math.pi, "y")
    _rot_sine(rig, anim, "Wing_R_02", dur, n, 22.0, math.pi + 0.6, "y")
    _rot_sine(rig, anim, "Body", dur, n, 4.0, 0.3)
    _tr_sine(rig, anim, "Root", dur, n, 0.03, freq=2.0, phase=0.5)
    _rot_sine(rig, anim, "Veil_01", dur, n, 5.0, 2.0, "y")
    _rot_sine(rig, anim, "Veil_02", dur, n, 6.0, 2.4, "y")


def anim_hit(builder: GlbBuilder, rig: Rig) -> None:
    """0.4s — wings tuck in (Z rotation), body dips -0.08 translation."""
    anim = builder.add_animation("AM_Mistmender_Hit")
    deg = DEG
    ts = [0.0, 0.08, 0.16, 0.28, 0.40]
    for tag, s in (("L", -1), ("R", 1)):
        rig.add_rotation_channel(anim, f"Wing_{tag}_01", list(zip(ts, [
            (0, 0, 0), (0, 0, s * 20 * deg), (0, 0, s * 14 * deg),
            (0, 0, s * 4 * deg), (0, 0, 0)])))
        rig.add_rotation_channel(anim, f"Wing_{tag}_02", list(zip(ts, [
            (0, 0, 0), (0, 0, s * 26 * deg), (0, 0, s * 18 * deg),
            (0, 0, s * 5 * deg), (0, 0, 0)])))
    rig.add_translation_channel(anim, "Root", [
        (0.0, (0, 0, 0)), (0.08, (0, -0.08, 0)), (0.16, (0, -0.055, 0)),
        (0.28, (0, -0.015, 0)), (0.40, (0, 0, 0))])
    rig.add_rotation_channel(anim, "Body", list(zip(ts, [
        (0, 0, 0), (-5 * deg, 0, 0), (-3 * deg, 0, 0), (0.5 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Veil_01", list(zip(ts, [
        (0, 0, 0), (8 * deg, 0, 0), (5 * deg, 0, 0), (1 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Veil_02", list(zip(ts, [
        (0, 0, 0), (10 * deg, 0, 0), (6 * deg, 0, 0), (1.5 * deg, 0, 0), (0, 0, 0)])))


def main() -> None:
    builder = GlbBuilder()
    for m in MATS.values():
        builder.add_material(m)

    rig = build_rig(builder)
    mb = MeshBuilder()
    build_body(mb)
    print(f"[{SPECIES}] body parts: {len(mb.parts)}, tris: {mb.triangle_count()}")

    skin_idx, prims = rig.build_skin(mb, power=3.0)
    mesh_node = builder.add_node("SK_Echo_Mistmender", parent=0,
                                 translation=(0, 0, 0))
    builder.assign_skin(mesh_node, skin_idx, prims)

    anim_idle(builder, rig)
    anim_move(builder, rig)
    anim_hit(builder, rig)

    os.makedirs(OUT_DIR, exist_ok=True)
    stats = builder.save_glb(OUT_PATH)
    problems = validate_glb(OUT_PATH)
    stats["validate"] = "PASS" if not problems else problems
    stats["bones"] = len(rig.bones)
    stats["asset_type"] = "skeletal_mesh"
    stats["species"] = SPECIES
    stats["element"] = "Light"
    stats["ue_path"] = "/Game/Characters/Echoes/SK_Echo_Mistmender"
    record("mesh", "SK_Echo_Mistmender", stats)
    print(f"[{SPECIES}] {OUT_PATH}")
    print(f"[{SPECIES}] bones={stats['bones']} tris={stats['triangles']} "
          f"anims={len(stats['animations'])} bytes={stats['bytes']} validate={stats['validate']}")
    if problems:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
