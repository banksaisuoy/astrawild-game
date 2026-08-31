"""
ASTRAWILD ArtSourceGen — SK_Echo_Cindermule (Ember Echo, pack construct-beast).

Original ASTRAWILD creature: a heavy dark-umber construct mule that hauls an
ember furnace — cargo frame with side crates, chimney stack, glowing furnace
grate at the rear, armored greaves and hooves. Broad, slow, industrial silhouette
(shoulder 0.72m, length ~1.15m).

Rig: Root/Hips + Spine_01..04 + Neck/Head/Jaw + Tail_01/02 + 4x (upper+lower+hoof).
Anims: AM_Cindermule_Idle (3.2s furnace flicker + head low bob),
AM_Cindermule_Move (1.1s heavy walk, legs +/-18deg, hips bob 0.03),
AM_Cindermule_Hit (0.45s flinch + crate shake).
Material slots: Echo_Body / Echo_Armor / Echo_Emissive / Echo_Eye.

Run:  python3 gen_echo_cindermule.py
Output: ArtSource/Meshes/Echoes/SK_Echo_Cindermule.glb
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
OUT_PATH = os.path.join(OUT_DIR, "SK_Echo_Cindermule.glb")
SPECIES = "Cindermule"
DEG = math.pi / 180.0

# ----------------------------------------------------------------- materials
BODY = (0.30, 0.24, 0.20, 1.0)       # dark umber hide
ARMOR = (0.17, 0.13, 0.11, 1.0)      # scorched plate
EMBER = (0.95, 0.45, 0.15)           # Ember ORANGE emissive

MATS = {
    "Echo_Body":     Material("Echo_Body", base_color=BODY, roughness=0.75, metallic=0.1),
    "Echo_Armor":    Material("Echo_Armor", base_color=ARMOR, roughness=0.45, metallic=0.55),
    "Echo_Emissive": Material("Echo_Emissive", base_color=(0.04, 0.02, 0.01, 1.0),
                              roughness=0.4, metallic=0.0, emissive=EMBER),
    "Echo_Eye":      Material("Echo_Eye", base_color=(0.05, 0.03, 0.02, 1.0),
                              roughness=0.25, metallic=0.0,
                              emissive=(1.0, 0.55, 0.22)),
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


def _hoof(r: float, h: float, radial: int = 6):
    """Hoof = cone flipped tip-down."""
    return rotate(cone(r, h, radial), math.pi, 0.0, 0.0)


def _rot_sine(rig: Rig, anim, bone: str, dur: float, n: int, amp_deg: float,
              phase: float = 0.0, ax: str = "x", freq: float = 1.0) -> None:
    idx = "xyz".index(ax)
    keys = []
    for i in range(n + 1):
        t = dur * i / n
        v = math.radians(amp_deg) * math.sin(2.0 * math.pi * freq * (i / n) + phase)
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
    rig.add_bone("Hips", "Root", (0, 0.66, -0.33), direction=(0, -1, 0), length=0.12)
    rig.add_bone("Spine_01", "Hips", (0, 0.015, 0.13), direction=(0, 0.06, 1), length=0.14)
    rig.add_bone("Spine_02", "Spine_01", (0, 0.015, 0.13), direction=(0, 0.06, 1), length=0.14)
    rig.add_bone("Spine_03", "Spine_02", (0, 0.015, 0.13), direction=(0, 0.22, 1), length=0.15)
    rig.add_bone("Spine_04", "Spine_03", (0, 0.015, 0.13), direction=(0, 0.22, 1), length=0.15)
    rig.add_bone("Neck", "Spine_04", (0, 0.03, 0.12), direction=(0, 0.45, 1), length=0.14)
    rig.add_bone("Head", "Neck", (0, 0.05, 0.12), direction=(0, 0.3, 1), length=0.13)
    rig.add_bone("Jaw", "Head", (0, -0.035, 0.085), direction=(0, -0.4, 1), length=0.08)
    rig.add_bone("Tail_01", "Hips", (0, 0.02, -0.14), direction=(0, 0.2, -1), length=0.13)
    rig.add_bone("Tail_02", "Tail_01", (0, 0.02, -0.13), direction=(0, 0.3, -1), length=0.12)

    for side, tag in ((-1, "L"), (1, "R")):
        s = side
        # front legs off the chest (Spine_04) — attach low so cargo binds to spine
        rig.add_bone(f"FrontLeg_{tag}", "Spine_04", (s * 0.14, -0.09, 0.05),
                     direction=(0, -1, 0.05), length=0.26)
        rig.add_bone(f"FrontLower_{tag}", f"FrontLeg_{tag}", (0, -0.26, 0.005),
                     direction=(0, -1, 0), length=0.24)
        rig.add_bone(f"FrontHoof_{tag}", f"FrontLower_{tag}", (0, -0.24, 0.0),
                     direction=(0, -1, 0.1), length=0.10)
        # back legs off the hips
        rig.add_bone(f"BackLeg_{tag}", "Hips", (s * 0.15, -0.02, -0.02),
                     direction=(0, -1, -0.1), length=0.28)
        rig.add_bone(f"BackLower_{tag}", f"BackLeg_{tag}", (0, -0.28, -0.005),
                     direction=(0, -1, 0.08), length=0.22)
        rig.add_bone(f"BackHoof_{tag}", f"BackLower_{tag}", (0, -0.22, 0.0),
                     direction=(0, -1, 0.1), length=0.09)
    return rig


def build_body(mb: MeshBuilder) -> None:
    T = translate

    # --- heavy torso
    mb.add(T(_capsule_z(0.19, 0.36, 3, 10), (0, 0.68, -0.04)), "Echo_Body")
    mb.add(T(sphere(0.20, 10, 7), (0, 0.70, 0.20)), "Echo_Body")
    mb.add(T(sphere(0.185, 10, 7), (0, 0.66, -0.33)), "Echo_Body")
    mb.add(T(box((0.28, 0.04, 0.46)), (0, 0.475, -0.05)), "Echo_Armor")  # belly plate

    # --- neck + stubby plated head
    neck = _tilt_to(_capsule(0.10, 0.10, 3, 8), 0.45, 1.0)
    mb.add(T(neck, (0, 0.775, 0.37)), "Echo_Body")
    mb.add(T(box((0.22, 0.19, 0.24)), (0, 0.83, 0.53)), "Echo_Body")
    mb.add(T(box((0.13, 0.11, 0.13)), (0, 0.77, 0.66)), "Echo_Body")      # snout
    mb.add(T(box((0.26, 0.05, 0.18)), (0, 0.935, 0.55)), "Echo_Armor")    # brow plate
    mb.add(T(box((0.10, 0.04, 0.12)), (0, 0.755, 0.645)), "Echo_Body")    # jaw
    for sx in (-1, 1):  # stubby ears
        mb.add(T(rotate(cone(0.04, 0.08, 6), 0.2, 0, 0), (sx * 0.08, 0.95, 0.47)), "Echo_Body")
    for sx in (-1, 1):  # eyes
        mb.add(T(sphere(0.017, 6, 5), (sx * 0.085, 0.855, 0.625)), "Echo_Eye")

    # --- cargo frame + crates + furnace + chimney (the silhouette maker)
    mb.add(T(box((0.34, 0.06, 0.40)), (0, 0.865, -0.10)), "Echo_Armor")    # saddle frame
    for sx in (-1, 1):
        mb.add(T(box((0.16, 0.26, 0.32)), (sx * 0.25, 0.72, -0.10)), "Echo_Armor")   # side crate
        mb.add(T(box((0.17, 0.03, 0.33)), (sx * 0.25, 0.83, -0.10)), "Echo_Body")    # crate strap
        mb.add(T(box((0.05, 0.15, 0.05)), (sx * 0.11, 0.93, -0.10)), "Echo_Armor")   # rail
    mb.add(T(cylinder(0.045, 0.24, 8), (0.10, 0.90, -0.26)), "Echo_Armor")           # chimney
    mb.add(T(tube(0.06, 0.048, 0.03, radial=8, wall_segs=1), (0.10, 1.145, -0.26)), "Echo_Armor")  # cap ring
    mb.add(T(box((0.22, 0.18, 0.05)), (0, 0.70, -0.545)), "Echo_Emissive")           # furnace glow
    for y in (0.64, 0.70, 0.76):   # furnace vent slats
        mb.add(T(box((0.18, 0.02, 0.012)), (0, y, -0.575)), "Echo_Armor")
    for sx in (-1, 1):   # saddle rivets
        for z in (-0.26, 0.06):
            mb.add(T(sphere(0.014, 6, 4), (sx * 0.14, 0.9, z)), "Echo_Armor")

    # --- chest vents (greeble)
    mb.add(T(box((0.10, 0.025, 0.02)), (0, 0.63, 0.385)), "Echo_Armor")
    mb.add(T(box((0.10, 0.025, 0.02)), (0, 0.56, 0.33)), "Echo_Armor")

    # --- legs: thick, armored greaves, hooves
    for sx in (-1, 1):
        # front
        mb.add(T(_capsule(0.07, 0.13, 3, 8), (sx * 0.14, 0.56, 0.245)), "Echo_Body")
        mb.add(T(sphere(0.06, 8, 5), (sx * 0.14, 0.43, 0.255)), "Echo_Armor")        # knee
        mb.add(T(_capsule(0.055, 0.11, 3, 8), (sx * 0.14, 0.31, 0.257)), "Echo_Body")
        mb.add(T(box((0.13, 0.24, 0.16)), (sx * 0.14, 0.30, 0.265)), "Echo_Armor")   # greave
        mb.add(T(_hoof(0.06, 0.10), (sx * 0.14, 0.10, 0.262)), "Echo_Armor")
        mb.add(T(box((0.12, 0.16, 0.16)), (sx * 0.155, 0.72, 0.22)), "Echo_Armor")   # shoulder plate
        # back
        mb.add(T(_capsule(0.08, 0.14, 3, 8), (sx * 0.15, 0.50, -0.36)), "Echo_Body")
        mb.add(T(sphere(0.065, 8, 5), (sx * 0.15, 0.36, -0.378)), "Echo_Armor")      # knee
        mb.add(T(_capsule(0.06, 0.11, 3, 8), (sx * 0.15, 0.25, -0.373)), "Echo_Body")
        mb.add(T(box((0.14, 0.24, 0.17)), (sx * 0.15, 0.24, -0.365)), "Echo_Armor")  # greave
        mb.add(T(_hoof(0.065, 0.10), (sx * 0.15, 0.10, -0.36)), "Echo_Armor")
        mb.add(T(box((0.13, 0.18, 0.18)), (sx * 0.16, 0.64, -0.33)), "Echo_Armor")   # hip plate

    # --- short tail with ember tuft
    mb.add(T(_tilt_to(_capsule(0.045, 0.09, 2, 8), 0.2, -1.0), (0, 0.68, -0.50)), "Echo_Body")
    mb.add(T(sphere(0.035, 6, 4), (0, 0.715, -0.63)), "Echo_Emissive")


# ----------------------------------------------------------------- animation
def anim_idle(builder: GlbBuilder, rig: Rig) -> None:
    """3.2s: furnace flicker (Spine_02 pulse), head low bob, slow tail sway."""
    anim = builder.add_animation("AM_Cindermule_Idle")
    dur, n = 3.2, 8
    # furnace flicker: two pulses per loop on the spine carrying the frame
    _rot_sine(rig, anim, "Spine_02", dur, n, 1.2, 0.0, freq=2.0)
    _rot_sine(rig, anim, "Spine_02", dur, n, 0.8, 0.9, "z", freq=2.0)
    _rot_sine(rig, anim, "Spine_03", dur, n, 0.8, 0.4, freq=2.0)
    # heavy breathing + head low bob
    _rot_sine(rig, anim, "Spine_01", dur, n, 1.0)
    _rot_sine(rig, anim, "Neck", dur, n, 3.0, 0.6)
    _rot_sine(rig, anim, "Head", dur, n, 4.0, 0.2)     # low head bob
    _rot_sine(rig, anim, "Jaw", dur, n, 2.0, 1.0)      # chewing
    _rot_sine(rig, anim, "Head", dur, n, 2.0, 0.9, "y")
    _rot_sine(rig, anim, "Tail_01", dur, n, 7.0, 0.0, "y")
    _rot_sine(rig, anim, "Tail_02", dur, n, 9.0, 0.8, "y")
    _tr_sine(rig, anim, "Hips", dur, n, 0.008)


def anim_move(builder: GlbBuilder, rig: Rig) -> None:
    """1.1s heavy walk: legs +/-18 deg, hips bob 0.03, counter spine yaw."""
    anim = builder.add_animation("AM_Cindermule_Move")
    dur, n = 1.1, 8

    def swing(amp: float, phase: float) -> list:
        return [math.radians(amp) * math.sin(2.0 * math.pi * (i / n) + phase)
                for i in range(n + 1)]

    def flex(amp: float, phase: float) -> list:
        return [math.radians(amp) * (0.35 + 0.65 * max(0.0, math.sin(
            2.0 * math.pi * (i / n) + 2.3 + phase))) for i in range(n + 1)]

    ts = [dur * i / n for i in range(n + 1)]
    legs = [
        ("FrontLeg_L", "FrontLower_L", "FrontHoof_L", 18.0, 0.0),
        ("FrontLeg_R", "FrontLower_R", "FrontHoof_R", 18.0, math.pi),
        ("BackLeg_L", "BackLower_L", "BackHoof_L", 18.0, math.pi),
        ("BackLeg_R", "BackLower_R", "BackHoof_R", 18.0, 0.0),
    ]
    for bone, low, hoof, amp, ph in legs:
        swing_vals = swing(amp, ph)
        rig.add_rotation_channel(anim, bone, list(zip(ts, [(v, 0, 0) for v in swing_vals])))
        flex_vals = flex(amp * 0.8, ph)
        rig.add_rotation_channel(anim, low, list(zip(ts, [(v, 0, 0) for v in flex_vals])))
        rig.add_rotation_channel(anim, hoof, list(zip(ts, [(-v * 0.3, 0, 0) for v in swing_vals])))

    # heavy counter sway + big hip bob
    _rot_sine(rig, anim, "Spine_01", dur, n, 1.6, 0.4, "y")
    _rot_sine(rig, anim, "Spine_02", dur, n, 2.2, 0.9, "y")
    _rot_sine(rig, anim, "Hips", dur, n, 1.8, 0.0, "y")
    _tr_sine(rig, anim, "Hips", dur, n, 0.03, freq=2.0, phase=0.8)
    _rot_sine(rig, anim, "Neck", dur, n, 2.5, 1.6)
    _rot_sine(rig, anim, "Head", dur, n, 2.0, 0.5)
    # cargo inertia: frame lags the body sway
    _rot_sine(rig, anim, "Spine_03", dur, n, 1.6, 1.3, "y")
    _rot_sine(rig, anim, "Spine_04", dur, n, 1.2, 1.7, "y")
    _rot_sine(rig, anim, "Tail_01", dur, n, 8.0, 0.3, "y")
    _rot_sine(rig, anim, "Tail_02", dur, n, 10.0, 0.9, "y")


def anim_hit(builder: GlbBuilder, rig: Rig) -> None:
    """0.45s flinch: spine dips, crates shake (Z jiggle), head snaps up."""
    anim = builder.add_animation("AM_Cindermule_Hit")
    deg = DEG
    ts = [0.0, 0.09, 0.18, 0.30, 0.45]
    rig.add_rotation_channel(anim, "Spine_03", list(zip(ts, [
        (0, 0, 0), (-7 * deg, 0, 2 * deg), (-4 * deg, 0, -2 * deg),
        (1 * deg, 0, 0.5 * deg), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Spine_04", list(zip(ts, [
        (0, 0, 0), (-6 * deg, 0, -2 * deg), (-3 * deg, 0, 1.5 * deg),
        (1 * deg, 0, 0), (0, 0, 0)])))
    # crates shake: opposing Z jiggle through the frame-carrying spine
    rig.add_rotation_channel(anim, "Spine_02", list(zip(ts, [
        (0, 0, 0), (0, 0, -2.5 * deg), (0, 0, 2 * deg),
        (0, 0, -0.5 * deg), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Spine_01", list(zip(ts, [
        (0, 0, 0), (-4 * deg, 0, 1 * deg), (-2 * deg, 0, -0.5 * deg),
        (0.5 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Neck", list(zip(ts, [
        (0, 0, 0), (-8 * deg, 0, 0), (-5 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Head", list(zip(ts, [
        (0, 0, 0), (-10 * deg, 0, 0), (-7 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Jaw", list(zip(ts, [
        (0, 0, 0), (5 * deg, 0, 0), (3 * deg, 0, 0), (1 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Tail_01", list(zip(ts, [
        (0, 0, 0), (10 * deg, 0, 0), (7 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
    for tag in ("L", "R"):
        rig.add_rotation_channel(anim, f"FrontLeg_{tag}", list(zip(ts, [
            (0, 0, 0), (7 * deg, 0, 0), (5 * deg, 0, 0), (1.5 * deg, 0, 0), (0, 0, 0)])))
        rig.add_rotation_channel(anim, f"BackLeg_{tag}", list(zip(ts, [
            (0, 0, 0), (-5 * deg, 0, 0), (-3 * deg, 0, 0), (-1 * deg, 0, 0), (0, 0, 0)])))
    rig.add_translation_channel(anim, "Hips", [
        (0.0, (0, 0, 0)), (0.09, (0, -0.02, 0)), (0.18, (0, -0.014, 0)),
        (0.30, (0, -0.004, 0)), (0.45, (0, 0, 0))])


def main() -> None:
    builder = GlbBuilder()
    for m in MATS.values():
        builder.add_material(m)

    rig = build_rig(builder)
    mb = MeshBuilder()
    build_body(mb)
    print(f"[{SPECIES}] body parts: {len(mb.parts)}, tris: {mb.triangle_count()}")

    skin_idx, prims = rig.build_skin(mb, power=3.4)
    mesh_node = builder.add_node("SK_Echo_Cindermule", parent=0,
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
    stats["element"] = "Ember"
    stats["ue_path"] = "/Game/Characters/Echoes/SK_Echo_Cindermule"
    record("mesh", "SK_Echo_Cindermule", stats)
    print(f"[{SPECIES}] {OUT_PATH}")
    print(f"[{SPECIES}] bones={stats['bones']} tris={stats['triangles']} "
          f"anims={len(stats['animations'])} bytes={stats['bytes']} validate={stats['validate']}")
    if problems:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
