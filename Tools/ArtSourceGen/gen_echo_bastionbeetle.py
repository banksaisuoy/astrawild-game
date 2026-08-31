"""
ASTRAWILD ArtSourceGen — SK_Echo_Bastionbeetle (Ash Echo, armored beetle).

Original ASTRAWILD creature: a heavy chitin bulwark beetle — three body
segments (abdomen/thorax spheres + head box), a forward horn cone flanked by
two mandible cones, two wing casings slightly open in bind with emissive Ash
violet vein strips, six capsule legs (angled femur out + tibia down + tiny
foot). Length 1.55m, height 0.75m.

Rig: Root/Abdomen/Thorax/Head/Horn + WingCase_L/R + 6 legs x 2 segments
(LegA/B/C_Femur + LegA/B/C_Tibia per side). 19 bones.
Anims: AM_Bastionbeetle_Idle (3.5s head yaw + casing shimmer),
AM_Bastionbeetle_Move (0.85s metachronal scuttle, leg pairs +0.5rad phase,
femur +/-20deg tibia +/-30deg), AM_Bastionbeetle_Hit (0.4s casing flare
Z +/-12deg + body duck).
Material slots: Echo_Body / Echo_Armor / Echo_Emissive / Echo_Eye.

Run:  python3 gen_echo_bastionbeetle.py
Output: ArtSource/Meshes/Echoes/SK_Echo_Bastionbeetle.glb
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
OUT_PATH = os.path.join(OUT_DIR, "SK_Echo_Bastionbeetle.glb")
SPECIES = "Bastionbeetle"
DEG = math.pi / 180.0

# ----------------------------------------------------------------- materials
BODY = (0.22, 0.23, 0.27, 1.0)       # dark slate chitin
ARMOR = (0.16, 0.17, 0.20, 1.0)      # darker plate
ASH = (0.55, 0.45, 0.9)              # Ash VIOLET emissive

MATS = {
    "Echo_Body":     Material("Echo_Body", base_color=BODY, roughness=0.75, metallic=0.1),
    "Echo_Armor":    Material("Echo_Armor", base_color=ARMOR, roughness=0.45, metallic=0.55),
    "Echo_Emissive": Material("Echo_Emissive", base_color=(0.03, 0.02, 0.05, 1.0),
                              roughness=0.4, metallic=0.0, emissive=ASH),
    "Echo_Eye":      Material("Echo_Eye", base_color=(0.05, 0.04, 0.08, 1.0),
                              roughness=0.25, metallic=0.0,
                              emissive=(0.75, 0.6, 1.0)),
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


def _align_y(m, ax: float, ay: float, az: float):
    """Rotates a Y-aligned shape so its +Y axis points along (ax, ay, az)."""
    n = math.sqrt(ax * ax + ay * ay + az * az)
    if n < 1e-9:
        return m
    ax, ay, az = ax / n, ay / n, az / n
    rx = math.asin(max(-1.0, min(1.0, az)))
    rz = math.atan2(-ax, ay)
    return rotate(m, rx, 0.0, rz)


def _seg_capsule(p0, p1, r: float, seg_v: int = 2, radial: int = 8):
    """Capsule spanning p0 -> p1 (meters), centered on the segment."""
    p0 = np.asarray(p0, dtype=float)
    p1 = np.asarray(p1, dtype=float)
    d = p1 - p0
    L = float(np.linalg.norm(d))
    body = max(L - 2.0 * r, 0.02)
    m = _align_y(_capsule(r, body, seg_v, radial), d[0], d[1], d[2])
    return translate(m, tuple((p0 + p1) * 0.5))


def _seg_cone(p0, p1, r: float, radial: int = 7):
    """Cone spanning p0 -> p1 (base at p0, tip at p1)."""
    p0 = np.asarray(p0, dtype=float)
    p1 = np.asarray(p1, dtype=float)
    d = p1 - p0
    L = float(np.linalg.norm(d))
    m = _align_y(cone(r, L, radial), d[0], d[1], d[2])
    return translate(m, tuple(p0))


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
# leg anchor -> knee -> foot, per leg (L side; R mirrored via _mx)
_LEGS = {
    "A": ((-0.17, 0.415, -0.02), (-0.42, 0.325, 0.04), (-0.465, 0.02, 0.06)),
    "B": ((-0.17, 0.405, -0.24), (-0.44, 0.325, -0.24), (-0.47, 0.02, -0.24)),
    "C": ((-0.15, 0.40, -0.80), (-0.43, 0.33, -0.84), (-0.46, 0.02, -0.84)),
}


def _mx(v, mirror: bool):
    """Optionally mirrors a point across the YZ plane (x -> -x)."""
    v = np.asarray(v, dtype=float).copy()
    if mirror:
        v[0] = -v[0]
    return v


def build_rig(builder: GlbBuilder) -> Rig:
    rig = Rig(builder, root_name="Root", root_position=(0, 0, 0))
    rig.add_bone("Abdomen", "Root", (0, 0.45, -0.62), direction=(0, 0.05, -1), length=0.48)
    rig.add_bone("Thorax", "Abdomen", (0, 0.015, 0.44), direction=(0, 0.15, 1), length=0.36)
    rig.add_bone("Head", "Thorax", (0, 0.0, 0.30), direction=(0, 0.25, 1), length=0.15)
    rig.add_bone("Horn", "Head", (0, 0.05, 0.08), direction=(0, 0.55, 1), length=0.36)
    for s, tag in ((-1, "L"), (1, "R")):
        rig.add_bone(f"WingCase_{tag}", "Thorax", (s * 0.09, 0.135, 0.20),
                     direction=(s * 0.25, 0.12, -1), length=0.80)
        for k, (hip, knee, foot) in _LEGS.items():
            parent = "Thorax" if k in ("A", "B") else "Abdomen"
            hip = _mx(hip, s > 0)
            knee = _mx(knee, s > 0)
            foot = _mx(foot, s > 0)
            bind = {
                "Abdomen": np.array([0.0, 0.45, -0.62]),
                "Thorax": np.array([0.0, 0.465, -0.18]),
            }[parent]
            rig.add_bone(f"Leg{k}_Femur_{tag}", parent, tuple(hip - bind),
                         direction=tuple(knee - hip), length=float(np.linalg.norm(knee - hip)))
            rig.add_bone(f"Leg{k}_Tibia_{tag}", f"Leg{k}_Femur_{tag}", tuple(knee - hip),
                         direction=tuple(foot - knee), length=float(np.linalg.norm(foot - knee)))
    return rig


def build_body(mb: MeshBuilder) -> None:
    T = translate

    # --- 3 body segments
    mb.add(T(scale(sphere(0.27, 12, 8), (1.0, 0.8, 1.12)), (0, 0.45, -0.82)), "Echo_Body")
    mb.add(T(box((0.16, 0.10, 0.06)), (0, 0.45, -1.09)), "Echo_Armor")     # abdomen cap
    mb.add(T(scale(sphere(0.24, 12, 8), (1.05, 0.85, 1.18)), (0, 0.47, -0.02)), "Echo_Body")
    mb.add(T(box((0.30, 0.035, 0.36)), (0, 0.655, -0.02)), "Echo_Armor")   # thorax ridge
    mb.add(T(box((0.22, 0.15, 0.24)), (0, 0.465, 0.19)), "Echo_Body")      # head
    mb.add(T(box((0.24, 0.05, 0.20)), (0, 0.535, 0.16)), "Echo_Armor")     # head crest

    # --- horn + mandibles
    mb.add(_seg_cone((0, 0.50, 0.18), (0, 0.683, 0.51), 0.05, 7), "Echo_Armor")
    for sx in (-1, 1):
        mb.add(_seg_cone((sx * 0.07, 0.44, 0.28), (sx * 0.095, 0.385, 0.43), 0.028, 6),
               "Echo_Armor")

    # --- eyes
    for sx in (-1, 1):
        mb.add(T(sphere(0.018, 6, 4), (sx * 0.085, 0.487, 0.295)), "Echo_Eye")

    # --- wing casings (slightly open in bind) + emissive vein strips
    for sx, tilt in ((-1, -10.0), (1, 10.0)):
        mb.add(T(rotate(box((0.185, 0.05, 0.66)), 0, 0, tilt * DEG),
                 (sx * 0.15, 0.635, -0.32)), "Echo_Armor")
        mb.add(T(rotate(box((0.02, 0.014, 0.52)), 0, 0, tilt * DEG),
                 (sx * 0.13, 0.662, -0.32)), "Echo_Emissive")
        mb.add(T(rotate(box((0.02, 0.012, 0.30)), 0, 0, tilt * DEG),
                 (sx * 0.155, 0.660, -0.62)), "Echo_Emissive")

    # --- 6 legs: capsule femur (angled out) + capsule tibia (down) + tiny foot
    for k, (hip, knee, foot) in _LEGS.items():
        for sx in (-1, 1):
            hip_w = _mx(hip, sx > 0)
            knee_w = _mx(knee, sx > 0)
            foot_w = _mx(foot, sx > 0)
            mb.add(_seg_capsule(hip_w, knee_w, 0.036, 2, 8), "Echo_Body")
            mb.add(T(box((0.06, 0.05, 0.06)), tuple(hip_w + (knee_w - hip_w) * 0.85)),
                   "Echo_Armor")                                             # knee hub
            mb.add(_seg_capsule(knee_w, foot_w, 0.027, 2, 8), "Echo_Body")
            mb.add(T(box((0.06, 0.035, 0.09)), (foot_w[0], 0.018, foot_w[2] + 0.01)),
                   "Echo_Armor")                                             # tiny foot


# ----------------------------------------------------------------- animation
def anim_idle(builder: GlbBuilder, rig: Rig) -> None:
    """3.5s — head Y +/-3deg, casings Z +/-1.5deg."""
    anim = builder.add_animation("AM_Bastionbeetle_Idle")
    dur, n = 3.5, 10
    _rot_sine(rig, anim, "Head", dur, n, 3.0, 0.5, "y")
    _rot_sine(rig, anim, "Horn", dur, n, 1.5, 0.9, "y")
    _rot_sine(rig, anim, "WingCase_L", dur, n, 1.5, 0.0, "z")
    _rot_sine(rig, anim, "WingCase_R", dur, n, -1.5, 0.0, "z")
    _rot_sine(rig, anim, "Abdomen", dur, n, 0.8, 0.3)


def anim_move(builder: GlbBuilder, rig: Rig) -> None:
    """0.85s — metachronal scuttle: leg pairs +0.5rad phase each,
    femur +/-20deg, tibia +/-30deg."""
    anim = builder.add_animation("AM_Bastionbeetle_Move")
    dur, n = 0.85, 9
    ts = [dur * i / n for i in range(n + 1)]

    def sine(amp_deg: float, phase: float) -> list:
        return [math.radians(amp_deg) * math.sin(2.0 * math.pi * (i / n) + phase)
                for i in range(n + 1)]

    for k, pair_phase in (("A", 0.0), ("B", 0.5), ("C", 1.0)):
        for tag, side_phase in (("L", 0.0), ("R", math.pi)):
            ph = pair_phase + side_phase
            femur = sine(20.0, ph)
            tibia = sine(30.0, ph + 0.9)
            rig.add_rotation_channel(anim, f"Leg{k}_Femur_{tag}",
                                     list(zip(ts, [(v, 0, 0) for v in femur])))
            rig.add_rotation_channel(anim, f"Leg{k}_Tibia_{tag}",
                                     list(zip(ts, [(v, 0, 0) for v in tibia])))

    _rot_sine(rig, anim, "Abdomen", dur, n, 1.5, 0.25, "y")
    _tr_sine(rig, anim, "Abdomen", dur, n, 0.008, freq=2.0, phase=0.4)
    _rot_sine(rig, anim, "Head", dur, n, 2.0, 1.2, "y")
    _rot_sine(rig, anim, "WingCase_L", dur, n, 1.0, 0.5, "z")
    _rot_sine(rig, anim, "WingCase_R", dur, n, -1.0, 0.5, "z")


def anim_hit(builder: GlbBuilder, rig: Rig) -> None:
    """0.4s — casings flare Z +/-12deg + body duck."""
    anim = builder.add_animation("AM_Bastionbeetle_Hit")
    deg = DEG
    ts = [0.0, 0.08, 0.15, 0.28, 0.40]
    rig.add_rotation_channel(anim, "WingCase_L", list(zip(ts, [
        (0, 0, 0), (0, 0, -12 * deg), (0, 0, -9 * deg), (0, 0, -3 * deg), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "WingCase_R", list(zip(ts, [
        (0, 0, 0), (0, 0, 12 * deg), (0, 0, 9 * deg), (0, 0, 3 * deg), (0, 0, 0)])))
    rig.add_translation_channel(anim, "Abdomen", [
        (0.0, (0, 0, 0)), (0.08, (0, -0.045, 0)), (0.15, (0, -0.03, 0)),
        (0.28, (0, -0.008, 0)), (0.40, (0, 0, 0))])
    rig.add_rotation_channel(anim, "Head", list(zip(ts, [
        (0, 0, 0), (6 * deg, 0, 0), (4 * deg, 0, 0), (1 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Horn", list(zip(ts, [
        (0, 0, 0), (4 * deg, 0, 0), (3 * deg, 0, 0), (1 * deg, 0, 0), (0, 0, 0)])))
    for k in ("A", "B", "C"):
        for tag in ("L", "R"):
            rig.add_rotation_channel(anim, f"Leg{k}_Femur_{tag}", list(zip(ts, [
                (0, 0, 0), (-6 * deg, 0, 0), (-4 * deg, 0, 0), (-1 * deg, 0, 0), (0, 0, 0)])))
            rig.add_rotation_channel(anim, f"Leg{k}_Tibia_{tag}", list(zip(ts, [
                (0, 0, 0), (8 * deg, 0, 0), (5 * deg, 0, 0), (1.5 * deg, 0, 0), (0, 0, 0)])))


def main() -> None:
    builder = GlbBuilder()
    for m in MATS.values():
        builder.add_material(m)

    rig = build_rig(builder)
    mb = MeshBuilder()
    build_body(mb)
    print(f"[{SPECIES}] body parts: {len(mb.parts)}, tris: {mb.triangle_count()}")

    skin_idx, prims = rig.build_skin(mb, power=3.0)
    mesh_node = builder.add_node("SK_Echo_Bastionbeetle", parent=0,
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
    stats["element"] = "Ash"
    stats["ue_path"] = "/Game/Characters/Echoes/SK_Echo_Bastionbeetle"
    record("mesh", "SK_Echo_Bastionbeetle", stats)
    print(f"[{SPECIES}] {OUT_PATH}")
    print(f"[{SPECIES}] bones={stats['bones']} tris={stats['triangles']} "
          f"anims={len(stats['animations'])} bytes={stats['bytes']} validate={stats['validate']}")
    if problems:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
