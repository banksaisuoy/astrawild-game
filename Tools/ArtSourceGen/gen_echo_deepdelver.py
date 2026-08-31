"""
ASTRAWILD ArtSourceGen — SK_Echo_Deepdelver (Frost Echo, drill-nosed mining ray).

Original ASTRAWILD creature: a levitating drill-nosed mining ray — dark
slate-blue manta disc body, an armored snout wedge carrying a vertical 6-sided
ridged drill crest (Echo_Armor) with an emissive tip cone and two orbiting
flute blades so the Y-axis spin reads on screen, two wing fins per side, four
floating pick shards (crystal, emissive) beside the body, a trailing tail
ribbon and an under-glow strip. Span 0.9m, floats with root at y=0.7.

Rig: Root(0,0.7,0)/Body + Snout_Drill + Wing_L/R_01/02 + Pick_L/R +
Tail_01/02. 11 bones.
Anims: AM_Deepdelver_Idle (3.6s continuous drill spin Y + body bob),
AM_Deepdelver_Move (1.2s wings +/-18deg + drill spins 2x faster + body pitch
X -6deg), AM_Deepdelver_Hit (0.35s picks scatter Z +/-10deg).
Material slots: Echo_Body / Echo_Armor / Echo_Emissive / Echo_Eye.

Note: drill spin keys use uniform 90deg steps (0/90/180/270/360 idle,
0..720 in 90deg steps on move) so linear quaternion interpolation always
advances forward — the literal 4-key sets would reverse mid-clip (quat
shortest-path) or degenerate (360->720 antipodal).

Run:  python3 gen_echo_deepdelver.py
Output: ArtSource/Meshes/Echoes/SK_Echo_Deepdelver.glb
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
OUT_PATH = os.path.join(OUT_DIR, "SK_Echo_Deepdelver.glb")
SPECIES = "Deepdelver"
DEG = math.pi / 180.0

# ----------------------------------------------------------------- materials
BODY = (0.24, 0.28, 0.33, 1.0)       # dark slate-blue
ARMOR = (0.17, 0.20, 0.24, 1.0)      # darker plate
TEAL = (0.2, 0.75, 0.7)              # deep teal emissive

MATS = {
    "Echo_Body":     Material("Echo_Body", base_color=BODY, roughness=0.75, metallic=0.1),
    "Echo_Armor":    Material("Echo_Armor", base_color=ARMOR, roughness=0.45, metallic=0.55),
    "Echo_Emissive": Material("Echo_Emissive", base_color=(0.01, 0.04, 0.04, 1.0),
                              roughness=0.4, metallic=0.0, emissive=TEAL),
    "Echo_Eye":      Material("Echo_Eye", base_color=(0.02, 0.07, 0.07, 1.0),
                              roughness=0.25, metallic=0.0,
                              emissive=(0.4, 0.95, 0.9)),
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


def _drill_spin(rig: Rig, anim, duration: float, total_deg: float, steps: int) -> None:
    """Continuous forward Y spin: uniform angle steps (<=90deg) keep every
    consecutive quaternion pair on the same 4D sheet (positive dot)."""
    keys = []
    for i in range(steps + 1):
        t = duration * i / steps
        theta = math.radians(total_deg * i / steps)
        keys.append((t, (0.0, theta, 0.0)))
    rig.add_rotation_channel(anim, "Snout_Drill", keys)


# --------------------------------------------------------------------- build
DRILL_PROFILE = [
    (0.026, 0.0), (0.068, 0.02), (0.048, 0.065), (0.064, 0.11),
    (0.042, 0.155), (0.056, 0.185), (0.0, 0.215),
]


def build_rig(builder: GlbBuilder) -> Rig:
    rig = Rig(builder, root_name="Root", root_position=(0, 0.7, 0))
    rig.add_bone("Body", "Root", (0, 0, 0), direction=(0, 0.1, 1), length=0.20)
    rig.add_bone("Snout_Drill", "Body", (0, 0.03, 0.18), direction=(0, 1, 0), length=0.22)
    for s, tag in ((-1, "L"), (1, "R")):
        rig.add_bone(f"Wing_{tag}_01", "Body", (s * 0.16, 0, 0),
                     direction=(s * 1, 0, 0.05), length=0.17)
        rig.add_bone(f"Wing_{tag}_02", f"Wing_{tag}_01", (s * 0.17, 0, 0),
                     direction=(s * 1, 0, -0.05), length=0.14)
        rig.add_bone(f"Pick_{tag}", "Body", (s * 0.20, -0.06, 0.08),
                     direction=(s * 0.7, -0.45, 0.55), length=0.13)
    rig.add_bone("Tail_01", "Body", (0, 0, -0.16), direction=(0, -0.15, -1), length=0.16)
    rig.add_bone("Tail_02", "Tail_01", (0, -0.025, -0.16), direction=(0, -0.3, -1), length=0.15)
    return rig


def build_body(mb: MeshBuilder) -> None:
    T = translate

    # --- manta disc body + snout wedge
    mb.add(T(scale(sphere(0.17, 12, 8), (1.0, 0.5, 1.35)), (0, 0.70, 0.0)), "Echo_Body")
    mb.add(T(box((0.15, 0.09, 0.16)), (0, 0.70, 0.19)), "Echo_Armor")

    # --- drill crest: 6-sided ridged cone lathe on the snout + collar + tip
    mb.add(T(tube(0.095, 0.075, 0.05, 8, 1), (0, 0.705, 0.18)), "Echo_Armor")
    mb.add(T(lathe(DRILL_PROFILE, 6, smooth=False), (0, 0.72, 0.18)), "Echo_Armor")
    for sx in (-1, 1):   # flute blades: make the spin visible
        mb.add(T(box((0.012, 0.185, 0.04)), (sx * 0.058, 0.81, 0.18)), "Echo_Armor")
    mb.add(T(cone(0.02, 0.05, 6), (0, 0.935, 0.18)), "Echo_Emissive")

    # --- 2 wing fins per side + armor leading spars
    for sx, t2 in ((-1, -8.0), (1, 8.0)):
        mb.add(T(box((0.16, 0.02, 0.13)), (sx * 0.245, 0.70, 0.0)), "Echo_Body")
        mb.add(T(rotate(box((0.13, 0.016, 0.10)), 0, 0, t2 * DEG),
                 (sx * 0.395, 0.695, -0.01)), "Echo_Body")
        mb.add(T(box((0.15, 0.022, 0.016)), (sx * 0.24, 0.70, 0.058)), "Echo_Armor")
        mb.add(T(rotate(box((0.12, 0.018, 0.014)), 0, 0, t2 * DEG),
                 (sx * 0.393, 0.696, 0.035)), "Echo_Armor")

    # --- 4 floating pick shards beside the body
    for sx in (-1, 1):
        mb.add(T(rotate(crystal(0.11, 0.02, 5), -0.6, 0, sx * -0.25),
                 (sx * 0.285, 0.585, 0.17)), "Echo_Emissive")
        mb.add(T(rotate(crystal(0.09, 0.017, 5), -0.2, 0, sx * -0.35),
                 (sx * 0.245, 0.62, 0.09)), "Echo_Emissive")

    # --- tail ribbon + tip glow
    mb.add(T(rotate(box((0.05, 0.012, 0.17)), 8 * DEG, 0, 0), (0, 0.688, -0.24)), "Echo_Body")
    mb.add(T(rotate(box((0.04, 0.010, 0.16)), 15 * DEG, 0, 0), (0, 0.652, -0.40)), "Echo_Body")
    mb.add(T(sphere(0.018, 6, 4), (0, 0.628, -0.485)), "Echo_Emissive")

    # --- under-glow strip
    mb.add(T(box((0.12, 0.014, 0.24)), (0, 0.612, -0.02)), "Echo_Emissive")

    # --- eyes
    for sx in (-1, 1):
        mb.add(T(sphere(0.015, 6, 4), (sx * 0.06, 0.73, 0.21)), "Echo_Eye")


# ----------------------------------------------------------------- animation
def anim_idle(builder: GlbBuilder, rig: Rig) -> None:
    """3.6s — drill Y 0/90/180/270/360 continuous spin + body bob."""
    anim = builder.add_animation("AM_Deepdelver_Idle")
    dur, n = 3.6, 10
    _drill_spin(rig, anim, dur, 360.0, 4)     # 0,90,180,270,360 (+270 for continuity)
    _tr_sine(rig, anim, "Root", dur, n, 0.03)
    for tag in ("L", "R"):
        _rot_sine(rig, anim, f"Wing_{tag}_01", dur, n, 4.0, 0.0, "y")
        _rot_sine(rig, anim, f"Wing_{tag}_02", dur, n, 4.0, 0.9, "y")
    _rot_sine(rig, anim, "Tail_01", dur, n, 3.0, 1.2, "y")
    _rot_sine(rig, anim, "Tail_02", dur, n, 4.5, 1.7, "y")
    for tag, s in (("L", -1), ("R", 1)):
        _rot_sine(rig, anim, f"Pick_{tag}", dur, n, s * 3.0, 0.8, "z")
    _rot_sine(rig, anim, "Body", dur, n, 1.0, 0.4)


def anim_move(builder: GlbBuilder, rig: Rig) -> None:
    """1.2s — wings +/-18deg, drill spins faster (0..720), body pitch X -6deg."""
    anim = builder.add_animation("AM_Deepdelver_Move")
    dur, n = 1.2, 10
    _drill_spin(rig, anim, dur, 720.0, 8)     # 90deg steps: 2 full turns
    _rot_sine(rig, anim, "Wing_L_01", dur, n, 18.0, 0.0, "y")
    _rot_sine(rig, anim, "Wing_L_02", dur, n, 18.0, 0.5, "y")
    _rot_sine(rig, anim, "Wing_R_01", dur, n, 18.0, math.pi, "y")
    _rot_sine(rig, anim, "Wing_R_02", dur, n, 18.0, math.pi + 0.5, "y")
    # body pitch: -6deg mean +/-2deg bob (endpoints equal -> seamless loop)
    keys = []
    for i in range(n + 1):
        t = dur * i / n
        v = math.radians(-6.0 + 2.0 * math.sin(2.0 * math.pi * (i / n)))
        keys.append((t, (v, 0, 0)))
    rig.add_rotation_channel(anim, "Body", keys)
    _tr_sine(rig, anim, "Root", dur, n, 0.03, freq=2.0, phase=0.4)
    _rot_sine(rig, anim, "Tail_01", dur, n, 5.0, 1.8, "y")
    _rot_sine(rig, anim, "Tail_02", dur, n, 6.0, 2.2, "y")
    for tag, s in (("L", -1), ("R", 1)):
        _rot_sine(rig, anim, f"Pick_{tag}", dur, n, s * 5.0, 1.0, "z")


def anim_hit(builder: GlbBuilder, rig: Rig) -> None:
    """0.35s — picks scatter Z +/-10deg."""
    anim = builder.add_animation("AM_Deepdelver_Hit")
    deg = DEG
    ts = [0.0, 0.07, 0.14, 0.25, 0.35]
    for tag, s in (("L", -1), ("R", 1)):
        rig.add_rotation_channel(anim, f"Pick_{tag}", list(zip(ts, [
            (0, 0, 0), (0, 0, s * 10 * deg), (0, 0, s * 7 * deg),
            (0, 0, s * 2 * deg), (0, 0, 0)])))
        rig.add_rotation_channel(anim, f"Pick_{tag}", list(zip(ts, [
            (0, 0, 0), (-6 * deg, 0, 0), (-4 * deg, 0, 0), (-1 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Body", list(zip(ts, [
        (0, 0, 0), (4 * deg, 0, 0), (2.5 * deg, 0, 0), (0.5 * deg, 0, 0), (0, 0, 0)])))
    rig.add_translation_channel(anim, "Root", [
        (0.0, (0, 0, 0)), (0.07, (0, -0.03, 0)), (0.14, (0, -0.02, 0)),
        (0.25, (0, -0.005, 0)), (0.35, (0, 0, 0))])
    for tag, s in (("L", -1), ("R", 1)):
        rig.add_rotation_channel(anim, f"Wing_{tag}_01", list(zip(ts, [
            (0, 0, 0), (0, 0, s * 8 * deg), (0, 0, s * 5 * deg),
            (0, 0, s * 1.5 * deg), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Tail_01", list(zip(ts, [
        (0, 0, 0), (6 * deg, 0, 0), (4 * deg, 0, 0), (1 * deg, 0, 0), (0, 0, 0)])))


def main() -> None:
    builder = GlbBuilder()
    for m in MATS.values():
        builder.add_material(m)

    rig = build_rig(builder)
    mb = MeshBuilder()
    build_body(mb)
    print(f"[{SPECIES}] body parts: {len(mb.parts)}, tris: {mb.triangle_count()}")

    skin_idx, prims = rig.build_skin(mb, power=3.0)
    mesh_node = builder.add_node("SK_Echo_Deepdelver", parent=0,
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
    stats["element"] = "Frost"
    stats["ue_path"] = "/Game/Characters/Echoes/SK_Echo_Deepdelver"
    record("mesh", "SK_Echo_Deepdelver", stats)
    print(f"[{SPECIES}] {OUT_PATH}")
    print(f"[{SPECIES}] bones={stats['bones']} tris={stats['triangles']} "
          f"anims={len(stats['animations'])} bytes={stats['bytes']} validate={stats['validate']}")
    if problems:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
