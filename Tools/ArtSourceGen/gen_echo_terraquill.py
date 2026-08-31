"""
ASTRAWILD ArtSourceGen — SK_Echo_Terraquill (Flora Echo, gatherer fox).

Original ASTRAWILD creature: a moss-russet quadruped gatherer fox whose spine
carries a ridge of flora-green crystal shards. Quick, light silhouette: low
shoulder (0.42m), long curved tail, upright ears, moss-russet body plates.

Rig: Root/Hips + Spine_01..03 + Neck/Head/Jaw + Tail_01..03 + 4x (upper+lower+paw).
Anims: AM_Terraquill_Idle (3.0s breathe + tail sway), AM_Terraquill_Move (0.9s
trot, diagonal pairs +/-25deg), AM_Terraquill_Hit (0.4s flinch rear-up + recover).
Material slots: Echo_Body / Echo_Armor / Echo_Emissive / Echo_Eye.

Run:  python3 gen_echo_terraquill.py
Output: ArtSource/Meshes/Echoes/SK_Echo_Terraquill.glb
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
from aw_shapes import (MeshBuilder, box, cone, crystal, lathe, mirror_x,
                       recompute_smooth_normals, sphere, torus, translate,
                       rotate, scale, tube)

OUT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__),
                          "..", "..", "ArtSource", "Meshes", "Echoes"))
OUT_PATH = os.path.join(OUT_DIR, "SK_Echo_Terraquill.glb")
SPECIES = "Terraquill"
DEG = math.pi / 180.0

# ----------------------------------------------------------------- materials
BODY = (0.42, 0.34, 0.26, 1.0)       # moss-russet
ARMOR = (0.24, 0.19, 0.14, 1.0)      # dark bark plate
FLORA = (0.25, 0.85, 0.35)           # Flora GREEN emissive

MATS = {
    "Echo_Body":     Material("Echo_Body", base_color=BODY, roughness=0.75, metallic=0.1),
    "Echo_Armor":    Material("Echo_Armor", base_color=ARMOR, roughness=0.45, metallic=0.55),
    "Echo_Emissive": Material("Echo_Emissive", base_color=(0.02, 0.03, 0.02, 1.0),
                              roughness=0.4, metallic=0.0, emissive=FLORA),
    "Echo_Eye":      Material("Echo_Eye", base_color=(0.03, 0.05, 0.03, 1.0),
                              roughness=0.25, metallic=0.0,
                              emissive=(0.35, 1.0, 0.45)),
}


# ------------------------------------------------------------- local helpers
def _capsule(r: float, length: float, seg_v: int = 3, radial: int = 10):
    """Correct capsule along Y (closed poles both ends, straight cylinder body).
    Local fix for the aw_shapes.capsule profile bug (tapered spindle)."""
    prof = []
    for i in range(seg_v + 1):
        a = math.pi * 0.5 * i / seg_v
        prof.append((r * math.sin(a), -length * 0.5 - r * math.cos(a)))
    for i in range(seg_v + 1):
        a = math.pi * 0.5 * (seg_v - i) / seg_v
        prof.append((r * math.sin(a), length * 0.5 + r * math.cos(a)))
    return lathe(prof, radial, smooth=True)


def _capsule_z(r: float, length: float, seg_v: int = 3, radial: int = 10):
    """Capsule along +Z (creature faces +Z)."""
    return rotate(_capsule(r, length, seg_v, radial), math.pi / 2.0, 0.0, 0.0)


def _tilt_to(m, ty: float, tz: float):
    """Tilt a Y-aligned shape so its axis points (0, ty, tz)."""
    return rotate(m, math.atan2(tz, ty), 0.0, 0.0)


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
    rig = Rig(builder, root_name="Root", root_position=(0, 0, 0))
    rig.add_bone("Hips", "Root", (0, 0.33, -0.17), direction=(0, -1, 0), length=0.10)
    rig.add_bone("Spine_01", "Hips", (0, 0.005, 0.12), direction=(0, 0.08, 1), length=0.14)
    rig.add_bone("Spine_02", "Spine_01", (0, 0.005, 0.12), direction=(0, 0.08, 1), length=0.14)
    rig.add_bone("Spine_03", "Spine_02", (0, 0.01, 0.12), direction=(0, 0.08, 1), length=0.14)
    rig.add_bone("Neck", "Spine_03", (0, 0.03, 0.10), direction=(0, 0.5, 1), length=0.14)
    rig.add_bone("Head", "Neck", (0, 0.05, 0.12), direction=(0, 0.35, 1), length=0.13)
    rig.add_bone("Jaw", "Head", (0, -0.03, 0.085), direction=(0, -0.45, 1), length=0.08)
    rig.add_bone("Tail_01", "Hips", (0, 0.03, -0.12), direction=(0, 0.35, -1), length=0.16)
    rig.add_bone("Tail_02", "Tail_01", (0, 0.035, -0.16), direction=(0, 0.55, -1), length=0.15)
    rig.add_bone("Tail_03", "Tail_02", (0, 0.045, -0.15), direction=(0, 1.0, -0.9), length=0.14)

    for side, tag in ((-1, "L"), (1, "R")):
        s = side
        # front legs hang off the chest (Spine_03)
        rig.add_bone(f"FrontLeg_{tag}", "Spine_03", (s * 0.10, -0.01, -0.02),
                     direction=(0, -1, 0), length=0.16)
        rig.add_bone(f"FrontLower_{tag}", f"FrontLeg_{tag}", (0, -0.16, 0.005),
                     direction=(0, -1, 0), length=0.15)
        rig.add_bone(f"FrontPaw_{tag}", f"FrontLower_{tag}", (0, -0.15, 0.0),
                     direction=(0, -1, 0.4), length=0.07)
        # back legs off the hips, thigh kicks slightly back
        rig.add_bone(f"BackLeg_{tag}", "Hips", (s * 0.10, -0.02, -0.02),
                     direction=(0, -1, -0.12), length=0.18)
        rig.add_bone(f"BackLower_{tag}", f"BackLeg_{tag}", (0, -0.18, -0.005),
                     direction=(0, -1, 0.1), length=0.12)
        rig.add_bone(f"BackPaw_{tag}", f"BackLower_{tag}", (0, -0.12, 0.005),
                     direction=(0, -1, 0.4), length=0.06)
    return rig


def build_body(mb: MeshBuilder) -> None:
    T = translate

    # --- torso: capsule + chest/hip masses
    mb.add(T(_capsule_z(0.12, 0.24, 3, 10), (0, 0.33, -0.03)), "Echo_Body")
    mb.add(T(sphere(0.125, 10, 7), (0, 0.345, 0.09)), "Echo_Body")
    mb.add(T(sphere(0.11, 10, 7), (0, 0.33, -0.17)), "Echo_Body")
    # belly plate
    mb.add(T(box((0.15, 0.03, 0.30)), (0, 0.205, -0.02)), "Echo_Armor")
    # collar ring greeble
    mb.add(T(tube(0.105, 0.09, 0.03, radial=10, wall_segs=1), (0, 0.36, 0.215)), "Echo_Armor")
    # belly studs x2
    for sx in (-0.05, 0.05):
        mb.add(T(sphere(0.014, 6, 4), (sx, 0.222, 0.0)), "Echo_Armor")

    # --- neck + head
    neck = _tilt_to(_capsule(0.055, 0.07, 3, 8), 0.5, 1.0)
    mb.add(T(neck, (0, 0.405, 0.34)), "Echo_Body")
    mb.add(T(box((0.15, 0.115, 0.16)), (0, 0.445, 0.43)), "Echo_Body")
    mb.add(T(sphere(0.078, 10, 7), (0, 0.455, 0.40)), "Echo_Body")
    mb.add(T(box((0.085, 0.06, 0.10)), (0, 0.44, 0.515)), "Echo_Body")     # snout
    mb.add(T(box((0.062, 0.028, 0.09)), (0, 0.375, 0.51)), "Echo_Body")    # jaw
    mb.add(T(box((0.13, 0.02, 0.05)), (0, 0.492, 0.435)), "Echo_Armor")    # brow ridge
    # ears: cones tilted back
    for sx in (-1, 1):
        mb.add(T(rotate(cone(0.03, 0.10, 7), 0.15, 0, 0), (sx * 0.05, 0.50, 0.395)), "Echo_Body")
    # eyes
    for sx in (-1, 1):
        mb.add(T(sphere(0.015, 6, 5), (sx * 0.046, 0.468, 0.477)), "Echo_Eye")

    # --- legs
    for sx in (-1, 1):
        # front
        mb.add(T(_capsule(0.042, 0.09, 2, 8), (sx * 0.10, 0.26, 0.17)), "Echo_Body")
        mb.add(T(_capsule(0.032, 0.08, 2, 8), (sx * 0.10, 0.105, 0.175)), "Echo_Body")
        mb.add(T(box((0.065, 0.05, 0.10)), (sx * 0.10, 0.025, 0.185)), "Echo_Armor")
        mb.add(T(box((0.075, 0.10, 0.13)), (sx * 0.105, 0.335, 0.16)), "Echo_Armor")  # shoulder plate
        # back
        mb.add(T(rotate(_capsule(0.052, 0.10, 2, 8), 0, 0, sx * 0.08),
                 (sx * 0.10, 0.22, -0.22)), "Echo_Body")
        mb.add(T(_capsule(0.036, 0.075, 2, 8), (sx * 0.10, 0.07, -0.22)), "Echo_Body")
        mb.add(T(box((0.07, 0.05, 0.11)), (sx * 0.10, 0.02, -0.21)), "Echo_Armor")
        mb.add(T(box((0.085, 0.12, 0.15)), (sx * 0.108, 0.31, -0.19)), "Echo_Armor")  # hip plate

    # --- tail (curled up) + emissive tip
    mb.add(T(_tilt_to(_capsule(0.048, 0.10, 2, 8), 0.35, -1.0), (0, 0.388, -0.365)), "Echo_Body")
    mb.add(T(_tilt_to(_capsule(0.036, 0.09, 2, 8), 0.55, -1.0), (0, 0.42, -0.525)), "Echo_Body")
    mb.add(T(_tilt_to(_capsule(0.026, 0.08, 2, 8), 1.0, -0.9), (0, 0.455, -0.665)), "Echo_Body")
    mb.add(T(rotate(crystal(0.09, 0.026, 5), math.radians(-42), 0, 0),
             (0, 0.495, -0.70)), "Echo_Emissive")

    # --- crystal spine ridge (Flora shards)
    shards = [
        ((0, 0.43, -0.26), 0.13, 0.024, 0.0),
        ((0, 0.44, -0.18), 0.16, 0.027, 0.0),
        ((0, 0.44, -0.10), 0.19, 0.030, 0.0),
        ((0, 0.445, -0.02), 0.21, 0.032, 0.0),
        ((0, 0.45, 0.06), 0.19, 0.030, 0.0),
        ((0, 0.455, 0.14), 0.16, 0.027, 0.0),
    ]
    for (pos, h, r, lean) in shards:
        mb.add(T(rotate(crystal(h, r, 5), lean, 0, 0), pos), "Echo_Emissive")
    # rump shard leaning back
    mb.add(T(rotate(crystal(0.12, 0.024, 5), math.radians(-30), 0, 0),
             (0, 0.42, -0.27)), "Echo_Emissive")


# ----------------------------------------------------------------- animation
def anim_idle(builder: GlbBuilder, rig: Rig) -> None:
    anim = builder.add_animation("AM_Terraquill_Idle")
    dur, n = 3.0, 8
    _rot_sine(rig, anim, "Spine_01", dur, n, 1.5)
    _rot_sine(rig, anim, "Spine_02", dur, n, 1.0, 0.5)
    _rot_sine(rig, anim, "Spine_03", dur, n, 0.8, 1.0)
    _rot_sine(rig, anim, "Neck", dur, n, 2.0, 1.2)
    _rot_sine(rig, anim, "Neck", dur, n, 1.5, 0.3, "z")
    _rot_sine(rig, anim, "Head", dur, n, 3.0, 0.6, "y")
    _rot_sine(rig, anim, "Head", dur, n, 1.2, 1.5)
    _rot_sine(rig, anim, "Jaw", dur, n, 1.5, 0.0)
    _rot_sine(rig, anim, "Tail_01", dur, n, 9.0, 0.0, "y")
    _rot_sine(rig, anim, "Tail_02", dur, n, 14.0, 0.7, "y")
    _rot_sine(rig, anim, "Tail_03", dur, n, 12.0, 1.4, "y")
    _rot_sine(rig, anim, "Tail_02", dur, n, 3.0, 1.0)
    _tr_sine(rig, anim, "Hips", dur, n, 0.006)


def anim_move(builder: GlbBuilder, rig: Rig) -> None:
    """0.9s trot — diagonal pairs (FrontL+BackR vs FrontR+BackL), +/-25 deg."""
    anim = builder.add_animation("AM_Terraquill_Move")
    dur, n = 0.9, 9

    def swing(amp: float, phase: float) -> list:
        return [math.radians(amp) * math.sin(2.0 * math.pi * (i / n) + phase)
                for i in range(n + 1)]

    def flex(amp: float, phase: float) -> list:
        return [math.radians(amp) * (0.35 + 0.65 * max(0.0, math.sin(
            2.0 * math.pi * (i / n) + 2.3 + phase))) for i in range(n + 1)]

    ts = [dur * i / n for i in range(n + 1)]
    legs = [
        ("FrontLeg_L", "FrontLower_L", "FrontPaw_L", 25.0, 0.0),
        ("FrontLeg_R", "FrontLower_R", "FrontPaw_R", 25.0, math.pi),
        ("BackLeg_L", "BackLower_L", "BackPaw_L", 25.0, math.pi),
        ("BackLeg_R", "BackLower_R", "BackPaw_R", 25.0, 0.0),
    ]
    for bone, low, paw, amp, ph in legs:
        swing_vals = swing(amp, ph)
        rig.add_rotation_channel(anim, bone, list(zip(ts, [(v, 0, 0) for v in swing_vals])))
        # lower leg flexes during recovery, paw counters
        flex_vals = flex(amp * 0.72, ph)
        rig.add_rotation_channel(anim, low, list(zip(ts, [(v, 0, 0) for v in flex_vals])))
        rig.add_rotation_channel(anim, paw, list(zip(ts, [(-v * 0.35, 0, 0) for v in swing_vals])))

    # spine counter-yaw + hips bob (2 beats per stride)
    _rot_sine(rig, anim, "Spine_01", dur, n, 2.0, 0.4, "y")
    _rot_sine(rig, anim, "Spine_02", dur, n, 3.5, 0.9, "y")
    _rot_sine(rig, anim, "Hips", dur, n, 1.5, 0.0, "y")
    _tr_sine(rig, anim, "Hips", dur, n, 0.018, freq=2.0, phase=0.8)
    _rot_sine(rig, anim, "Neck", dur, n, 1.5, 1.6)
    _rot_sine(rig, anim, "Head", dur, n, 2.0, 0.5)
    _rot_sine(rig, anim, "Tail_01", dur, n, 10.0, 0.3, "y")
    _rot_sine(rig, anim, "Tail_02", dur, n, 14.0, 0.9, "y")
    _rot_sine(rig, anim, "Tail_03", dur, n, 16.0, 1.5, "y")


def anim_hit(builder: GlbBuilder, rig: Rig) -> None:
    anim = builder.add_animation("AM_Terraquill_Hit")
    deg = DEG
    ts = [0.0, 0.08, 0.16, 0.28, 0.40]
    rig.add_rotation_channel(anim, "Spine_01", list(zip(ts, [
        (0, 0, 0), (-8 * deg, 0, 0), (-5 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Spine_02", list(zip(ts, [
        (0, 0, 0), (-6 * deg, 0, 0), (-3 * deg, 0, 0), (1.5 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Neck", list(zip(ts, [
        (0, 0, 0), (-9 * deg, 0, 0), (-6 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Head", list(zip(ts, [
        (0, 0, 0), (-12 * deg, 0, 0), (-8 * deg, 0, 0), (3 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Jaw", list(zip(ts, [
        (0, 0, 0), (6 * deg, 0, 0), (4 * deg, 0, 0), (1 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Tail_01", list(zip(ts, [
        (0, 0, 0), (12 * deg, 0, 0), (9 * deg, 0, 0), (3 * deg, 0, 0), (0, 0, 0)])))
    # front legs brace backward
    for tag in ("L", "R"):
        rig.add_rotation_channel(anim, f"FrontLeg_{tag}", list(zip(ts, [
            (0, 0, 0), (8 * deg, 0, 0), (6 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
        rig.add_rotation_channel(anim, f"BackLeg_{tag}", list(zip(ts, [
            (0, 0, 0), (-6 * deg, 0, 0), (-4 * deg, 0, 0), (-1 * deg, 0, 0), (0, 0, 0)])))
    rig.add_translation_channel(anim, "Hips", [
        (0.0, (0, 0, 0)), (0.08, (0, -0.012, 0)), (0.16, (0, -0.008, 0)),
        (0.28, (0, -0.002, 0)), (0.40, (0, 0, 0))])


def main() -> None:
    builder = GlbBuilder()
    for m in MATS.values():
        builder.add_material(m)

    rig = build_rig(builder)
    mb = MeshBuilder()
    build_body(mb)
    print(f"[{SPECIES}] body parts: {len(mb.parts)}, tris: {mb.triangle_count()}")

    skin_idx, prims = rig.build_skin(mb, power=3.3)
    mesh_node = builder.add_node("SK_Echo_Terraquill", parent=0,
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
    stats["element"] = "Flora"
    stats["ue_path"] = "/Game/Characters/Echoes/SK_Echo_Terraquill"
    record("mesh", "SK_Echo_Terraquill", stats)
    print(f"[{SPECIES}] {OUT_PATH}")
    print(f"[{SPECIES}] bones={stats['bones']} tris={stats['triangles']} "
          f"anims={len(stats['animations'])} bytes={stats['bytes']} validate={stats['validate']}")
    if problems:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
