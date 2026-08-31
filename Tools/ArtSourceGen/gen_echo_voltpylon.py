"""
ASTRAWILD ArtSourceGen — SK_Echo_Voltpylon (Pulse Echo, biped automaton).

Original ASTRAWILD creature: a hovering biped construct pylon — gunmetal
cylinder core torso ringed with two armor collars, a large cyan crystal shard
mounted on a small head box (the "pylon head"), clamp-pincer hands, piston
legs (outer cylinder + thinner shin cylinder) that end in emissive hover rings,
coil rings on the forearms and a chest core orb. Height 1.5m.

Rig: Root/Hips + Spine_01/02 + Neck/Head + 2x (Shoulder/UpperArm/Forearm/Hand)
+ 2x (Leg/Shin/Foot). 20 bones.
Anims: AM_Voltpylon_Idle (2.8s hover bob + head crystal yaw),
AM_Voltpylon_Move (1.0s robotic stride, legs +/-20deg stiff, arms counter
+/-12deg), AM_Voltpylon_Hit (0.35s torso pitch + crystal Y jerk).
Material slots: Echo_Body / Echo_Armor / Echo_Emissive / Echo_Eye.

Run:  python3 gen_echo_voltpylon.py
Output: ArtSource/Meshes/Echoes/SK_Echo_Voltpylon.glb
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
OUT_PATH = os.path.join(OUT_DIR, "SK_Echo_Voltpylon.glb")
SPECIES = "Voltpylon"
DEG = math.pi / 180.0

# ----------------------------------------------------------------- materials
BODY = (0.28, 0.30, 0.33, 1.0)       # gunmetal frame
ARMOR = (0.20, 0.22, 0.25, 1.0)      # dark gunmetal plate
PULSE = (0.2, 0.85, 0.9)             # Pulse ELECTRIC CYAN emissive

MATS = {
    "Echo_Body":     Material("Echo_Body", base_color=BODY, roughness=0.75, metallic=0.1),
    "Echo_Armor":    Material("Echo_Armor", base_color=ARMOR, roughness=0.45, metallic=0.55),
    "Echo_Emissive": Material("Echo_Emissive", base_color=(0.01, 0.03, 0.04, 1.0),
                              roughness=0.4, metallic=0.0, emissive=PULSE),
    "Echo_Eye":      Material("Echo_Eye", base_color=(0.02, 0.05, 0.06, 1.0),
                              roughness=0.25, metallic=0.0,
                              emissive=(0.55, 1.0, 1.0)),
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


def _seg_cylinder(p0, p1, r: float, radial: int = 8):
    """Cylinder spanning p0 -> p1 (base at p0)."""
    p0 = np.asarray(p0, dtype=float)
    p1 = np.asarray(p1, dtype=float)
    d = p1 - p0
    L = float(np.linalg.norm(d))
    m = _align_y(cylinder(r, L, radial), d[0], d[1], d[2])
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
def build_rig(builder: GlbBuilder) -> Rig:
    rig = Rig(builder, root_name="Root", root_position=(0, 0, 0))
    rig.add_bone("Hips", "Root", (0, 0.78, 0.0), direction=(0, -1, 0), length=0.10)
    rig.add_bone("Spine_01", "Hips", (0, 0.12, 0.03), direction=(0, 1, 0.25), length=0.13)
    rig.add_bone("Spine_02", "Spine_01", (0, 0.12, 0.02), direction=(0, 1, 0.2), length=0.13)
    rig.add_bone("Neck", "Spine_02", (0, 0.11, 0.01), direction=(0, 0.8, 0.5), length=0.08)
    rig.add_bone("Head", "Neck", (0, 0.09, 0.02), direction=(0, 0.4, 1), length=0.11)

    for s, tag in ((-1, "L"), (1, "R")):
        rig.add_bone(f"Shoulder_{tag}", "Spine_02", (s * 0.17, 0.05, 0.02),
                     direction=(s * 1, 0, 0), length=0.06)
        rig.add_bone(f"UpperArm_{tag}", f"Shoulder_{tag}", (s * 0.07, -0.02, 0.0),
                     direction=(s * 0.3, -1, 0), length=0.25)
        rig.add_bone(f"Forearm_{tag}", f"UpperArm_{tag}", (s * 0.05, -0.24, 0.005),
                     direction=(s * 0.2, -1, 0.05), length=0.23)
        rig.add_bone(f"Hand_{tag}", f"Forearm_{tag}", (s * 0.04, -0.22, 0.005),
                     direction=(0, -1, 0.2), length=0.08)
        # legs: thigh (Leg) + shin + foot — piston stack under the hips
        rig.add_bone(f"Leg_{tag}", "Hips", (s * 0.11, -0.03, 0.0),
                     direction=(0, -1, 0), length=0.33)
        rig.add_bone(f"Shin_{tag}", f"Leg_{tag}", (0.01, -0.33, 0.01),
                     direction=(0, -1, 0), length=0.33)
        rig.add_bone(f"Foot_{tag}", f"Shin_{tag}", (0, -0.33, -0.01),
                     direction=(0, -1, 0.35), length=0.07)
    return rig


def build_body(mb: MeshBuilder) -> None:
    T = translate

    # --- torso: cylinder core + 2 armor ring segments + pelvis block
    mb.add(T(cylinder(0.125, 0.40, 10), (0, 0.70, 0.03)), "Echo_Body")
    mb.add(T(torus(0.148, 0.022, 10, 5), (0, 0.84, 0.03)), "Echo_Armor")
    mb.add(T(torus(0.148, 0.022, 10, 5), (0, 1.00, 0.05)), "Echo_Armor")
    mb.add(T(box((0.24, 0.10, 0.20)), (0, 0.73, 0.0)), "Echo_Armor")
    mb.add(T(box((0.02, 0.18, 0.14)), (0, 1.03, -0.10)), "Echo_Armor")   # back fin
    # chest core orb (ELECTRIC CYAN)
    mb.add(T(sphere(0.055, 8, 6), (0, 0.985, 0.155)), "Echo_Emissive")

    # --- neck + pylon head
    mb.add(T(cylinder(0.05, 0.12, 8), (0, 1.11, 0.06)), "Echo_Body")
    mb.add(T(box((0.15, 0.12, 0.16)), (0, 1.27, 0.08)), "Echo_Body")     # head box
    for sx in (-1, 1):
        mb.add(T(box((0.025, 0.07, 0.10)), (sx * 0.083, 1.30, 0.03)), "Echo_Armor")
    # pylon head: large crystal shard, tilted slightly forward
    mb.add(T(rotate(crystal(0.17, 0.05, 6), 10 * DEG, 0, 0), (0, 1.32, 0.08)),
           "Echo_Emissive")
    for sx in (-1, 1):
        mb.add(T(sphere(0.013, 6, 4), (sx * 0.045, 1.29, 0.16)), "Echo_Eye")

    # --- arms: shoulder pads + capsules + coil rings + clamp hands
    for sx in (-1, 1):
        mb.add(T(box((0.09, 0.10, 0.12)), (sx * 0.185, 1.07, 0.06)), "Echo_Armor")
        mb.add(_seg_capsule((sx * 0.24, 1.05, 0.07), (sx * 0.29, 0.81, 0.075),
                            0.042, 2, 8), "Echo_Body")
        mb.add(_seg_capsule((sx * 0.29, 0.81, 0.075), (sx * 0.33, 0.59, 0.08),
                            0.034, 2, 8), "Echo_Body")
        mb.add(T(torus(0.046, 0.010, 8, 4), (sx * 0.302, 0.74, 0.077)), "Echo_Emissive")
        mb.add(T(torus(0.042, 0.009, 8, 4), (sx * 0.316, 0.655, 0.079)), "Echo_Emissive")
        # clamp hand: two opposing jaw boxes
        mb.add(T(box((0.07, 0.03, 0.11)), (sx * 0.335, 0.585, 0.115)), "Echo_Armor")
        mb.add(T(box((0.07, 0.03, 0.11)), (sx * 0.335, 0.545, 0.075)), "Echo_Armor")

    # --- piston legs: cylinder + thinner cylinder, hover-ring feet
    for sx in (-1, 1):
        mb.add(T(box((0.06, 0.10, 0.12)), (sx * 0.15, 0.75, 0.0)), "Echo_Armor")  # hip block
        mb.add(_seg_cylinder((sx * 0.11, 0.75, 0.0), (sx * 0.10, 0.42, 0.01),
                             0.052, 8), "Echo_Body")
        mb.add(T(box((0.08, 0.07, 0.08)), (sx * 0.10, 0.43, 0.01)), "Echo_Armor")  # knee hub
        mb.add(_seg_cylinder((sx * 0.10, 0.42, 0.01), (sx * 0.10, 0.09, 0.0),
                             0.038, 8), "Echo_Body")
        mb.add(_seg_cylinder((sx * 0.10, 0.16, 0.0), (sx * 0.10, 0.09, 0.0),
                             0.03, 8), "Echo_Body")
        mb.add(T(torus(0.07, 0.014, 10, 5), (sx * 0.10, 0.045, 0.005)), "Echo_Emissive")


# ----------------------------------------------------------------- animation
def anim_idle(builder: GlbBuilder, rig: Rig) -> None:
    """2.8s — hover bob (Hips y +/-0.02) + head crystal yaw +/-4deg."""
    anim = builder.add_animation("AM_Voltpylon_Idle")
    dur, n = 2.8, 8
    _tr_sine(rig, anim, "Hips", dur, n, 0.02)
    _rot_sine(rig, anim, "Head", dur, n, 4.0, 0.8, "y")     # crystal yaw
    _rot_sine(rig, anim, "Head", dur, n, 1.0, 0.3)
    _rot_sine(rig, anim, "Neck", dur, n, 1.5, 1.1, "y")
    _rot_sine(rig, anim, "Spine_01", dur, n, 1.0, 0.5, "y")
    for tag, s in (("L", -1), ("R", 1)):
        _rot_sine(rig, anim, f"UpperArm_{tag}", dur, n, 1.2, 0.6 if s < 0 else 2.2, "z")


def anim_move(builder: GlbBuilder, rig: Rig) -> None:
    """1.0s — robotic stride: legs +/-20deg stiff, arms counter +/-12deg."""
    anim = builder.add_animation("AM_Voltpylon_Move")
    dur, n = 1.0, 10
    ts = [dur * i / n for i in range(n + 1)]

    def sine(amp_deg: float, phase: float) -> list:
        return [math.radians(amp_deg) * math.sin(2.0 * math.pi * (i / n) + phase)
                for i in range(n + 1)]

    for tag, ph in (("L", 0.0), ("R", math.pi)):
        thigh = sine(20.0, ph)
        shin = [v * 0.8 for v in thigh]                       # stiff piston
        foot = [-(t_ + s_) * 0.4 for t_, s_ in zip(thigh, shin)]  # level feet
        rig.add_rotation_channel(anim, f"Leg_{tag}",
                                 list(zip(ts, [(v, 0, 0) for v in thigh])))
        rig.add_rotation_channel(anim, f"Shin_{tag}",
                                 list(zip(ts, [(v, 0, 0) for v in shin])))
        rig.add_rotation_channel(anim, f"Foot_{tag}",
                                 list(zip(ts, [(v, 0, 0) for v in foot])))
        # arms counter-swing (opposite phase to same-side leg)
        arm = sine(12.0, ph + math.pi)
        rig.add_rotation_channel(anim, f"UpperArm_{tag}",
                                 list(zip(ts, [(v, 0, 0) for v in arm])))
        rig.add_rotation_channel(anim, f"Forearm_{tag}",
                                 list(zip(ts, [(v * 0.4, 0, 0) for v in arm])))

    _tr_sine(rig, anim, "Hips", dur, n, 0.012, freq=2.0, phase=0.4)
    _rot_sine(rig, anim, "Spine_01", dur, n, 2.0, 0.2, "y")
    _rot_sine(rig, anim, "Head", dur, n, 1.5, 0.9, "y")
    _rot_sine(rig, anim, "Neck", dur, n, 1.0, 1.4)


def anim_hit(builder: GlbBuilder, rig: Rig) -> None:
    """0.35s — torso X -6deg + head crystal Y jerk."""
    anim = builder.add_animation("AM_Voltpylon_Hit")
    deg = DEG
    ts = [0.0, 0.06, 0.12, 0.24, 0.35]
    rig.add_rotation_channel(anim, "Spine_01", list(zip(ts, [
        (0, 0, 0), (-6 * deg, 0, 0), (-4 * deg, 0, 0), (1.5 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Spine_02", list(zip(ts, [
        (0, 0, 0), (-4 * deg, 0, 0), (-3 * deg, 0, 0), (1 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Head", list(zip(ts, [
        (0, 0, 0), (0, 9 * deg, 0), (0, 5 * deg, 0), (0, -2 * deg, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Head", list(zip(ts, [
        (0, 0, 0), (-3 * deg, 0, 0), (-2 * deg, 0, 0), (0.5 * deg, 0, 0), (0, 0, 0)])))
    rig.add_translation_channel(anim, "Hips", [
        (0.0, (0, 0, 0)), (0.06, (0, -0.012, 0)), (0.12, (0, -0.007, 0)),
        (0.24, (0, -0.002, 0)), (0.35, (0, 0, 0))])
    for tag, s in (("L", -1), ("R", 1)):
        rig.add_rotation_channel(anim, f"UpperArm_{tag}", list(zip(ts, [
            (0, 0, 0), (0, 0, s * 8 * deg), (0, 0, s * 5 * deg),
            (0, 0, s * 1 * deg), (0, 0, 0)])))


def main() -> None:
    builder = GlbBuilder()
    for m in MATS.values():
        builder.add_material(m)

    rig = build_rig(builder)
    mb = MeshBuilder()
    build_body(mb)
    print(f"[{SPECIES}] body parts: {len(mb.parts)}, tris: {mb.triangle_count()}")

    skin_idx, prims = rig.build_skin(mb, power=3.0)
    mesh_node = builder.add_node("SK_Echo_Voltpylon", parent=0,
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
    stats["element"] = "Pulse"
    stats["ue_path"] = "/Game/Characters/Echoes/SK_Echo_Voltpylon"
    record("mesh", "SK_Echo_Voltpylon", stats)
    print(f"[{SPECIES}] {OUT_PATH}")
    print(f"[{SPECIES}] bones={stats['bones']} tris={stats['triangles']} "
          f"anims={len(stats['animations'])} bytes={stats['bytes']} validate={stats['validate']}")
    if problems:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
