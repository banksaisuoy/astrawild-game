"""
ASTRAWILD ArtSourceGen — SK_Echo_EyeSentinel (Pulse Echo, Maelstrom construct).

Original ASTRAWILD creature: the Eye Sentinel — the floating guards of the
Eye of the Maelstrom dungeon and the Drowned Sovereign's summons (MQ-15/16).
Canon: Pulse element, Construct family (machined metal), Floating body plan,
Medium size class (runtime BodyScaleForSize 1.0; authored at ~1.4x hero base
scale per CREATURE_VISUAL_STRATEGY §7 — constructs read tall, not wide).

Design: a hovering obelisk-construct — tapered 6-sided machined prism shaft
rising 2.45m above a downward emitter spike, crowned by a forward-angled
spire; a single large emissive iris (deep Pulse violet lens + darker pupil in
a machined socket rim) set into the front face; a gyroscopic ring (faceted
torus + glowing inner track + bolt studs) tumbles around the iris on two
crossing driver bones that always animate identically (rigid ring motion);
two orbital shards sweep cones around the crown on jointed armatures with
emissive cores; side heat fins and greeble panels complete the machined
surface. Root hovers at y=1.55.

Rig: Root + Base + Obelisk_Lower/Upper + Crown + Iris + Ring_01/02 +
OrbitA_01/02 + OrbitB_01/02 + Vent_L/R. 14 bones.
Anims: AM_EyeSentinel_Idle (4.2s hover-bob + gyroscopic ring wobble + crown
shard orbit + gaze scan), AM_EyeSentinel_Move (1.3s hover-glide: root pitch/
bank, faster ring tumble and shard orbits), AM_EyeSentinel_Hit (0.4s ring
jolt + root dip + armature flare).
Material slots: Echo_Body / Echo_Armor / Echo_Emissive / Echo_Eye.

Run:  python3 gen_echo_eyesentinel.py
Output: ArtSource/Meshes/Echoes/SK_Echo_EyeSentinel.glb
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
OUT_PATH = os.path.join(OUT_DIR, "SK_Echo_EyeSentinel.glb")
SPECIES = "EyeSentinel"
DEG = math.pi / 180.0

# ----------------------------------------------------------------- materials
BODY = (0.33, 0.35, 0.40, 1.0)       # machined gunmetal
ARMOR = (0.19, 0.20, 0.24, 1.0)      # dark edge trim
PULSE = (0.55, 0.35, 0.95)           # Pulse VIOLET emissive

MATS = {
    "Echo_Body":     Material("Echo_Body", base_color=BODY, roughness=0.45, metallic=0.5),
    "Echo_Armor":    Material("Echo_Armor", base_color=ARMOR, roughness=0.3, metallic=0.65),
    "Echo_Emissive": Material("Echo_Emissive", base_color=(0.03, 0.02, 0.06, 1.0),
                              roughness=0.4, metallic=0.0, emissive=PULSE),
    "Echo_Eye":      Material("Echo_Eye", base_color=(0.05, 0.03, 0.09, 1.0),
                              roughness=0.25, metallic=0.0,
                              emissive=(0.88, 0.78, 1.0)),
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


def _spin(rig: Rig, anim, bone: str, dur: float, turns: float = 1.0,
          ax: str = "y") -> None:
    """Full-axis spin channel (loop-safe: end orientation == start)."""
    idx = "xyz".index(ax)
    n = max(8, int(16 * abs(turns)))
    keys = []
    for i in range(n + 1):
        t = dur * i / n
        v = 2.0 * math.pi * turns * i / n
        e = [0.0, 0.0, 0.0]
        e[idx] = v
        keys.append((t, tuple(e)))
    rig.add_rotation_channel(anim, bone, keys)


# --------------------------------------------------------------------- build
ROOT = np.array((0.0, 1.55, 0.0))
IRIS_C = np.array((0.0, 1.62, 0.28))     # iris + gyro ring center (front)
ORBIT = {
    "A": [np.array((0.0, 2.40, 0.02)), np.array((0.39, 2.49, 0.14)),
          np.array((0.60, 2.52, 0.19))],
    "B": [np.array((0.0, 2.35, -0.02)), np.array((-0.36, 2.28, 0.10)),
          np.array((-0.55, 2.24, 0.17))],
}


def build_rig(builder: GlbBuilder) -> Rig:
    rig = Rig(builder, root_name="Root", root_position=tuple(ROOT))
    # shaft bones run diagonally through the prism so every shaft vertex stays
    # closer to an obelisk segment than to the forward gyro-ring chords
    rig.add_bone("Base", "Root", (0.0, -0.77, 0.0), direction=(0, -1, 0), length=0.40)
    rig.add_bone("Obelisk_Lower", "Root", (0.0, -0.80, 0.02),
                 direction=(0.15, 1, 0.12), length=0.78)
    rig.add_bone("Obelisk_Upper", "Root", (0.0, 0.07, 0.02),
                 direction=(-0.15, 1, -0.12), length=0.85)
    rig.add_bone("Crown", "Root", (0.0, 0.87, 0.02), direction=(0, 1, 0.35), length=0.33)
    # iris + the two gyro-ring driver bones (they always animate identically,
    # so the ring they both touch moves rigidly)
    rig.add_bone("Iris", "Root", tuple(IRIS_C - ROOT - (0, 0, 0.07)),
                 direction=(0, 0, 1), length=0.22)
    rig.add_bone("Ring_01", "Root", tuple(IRIS_C - ROOT), direction=(0, 1, 0), length=0.64)
    rig.add_bone("Ring_02", "Root", tuple(IRIS_C - ROOT), direction=(1, 0, 0), length=0.64)
    # orbital shard armatures around the crown
    for name, (piv, joint, shard) in ORBIT.items():
        d1 = joint - piv
        d2 = shard - joint
        rig.add_bone(f"Orbit{name}_01", "Root", tuple(piv - ROOT),
                     direction=tuple(d1), length=float(np.linalg.norm(d1)))
        rig.add_bone(f"Orbit{name}_02", f"Orbit{name}_01", tuple(joint - piv),
                     direction=tuple(d2), length=float(np.linalg.norm(d2)))
    # side heat fins
    for tag, s in (("L", -1), ("R", 1)):
        anchor = ROOT + np.array((s * 0.16, -0.52, 0.04))
        rig.add_bone(f"Vent_{tag}", "Root", tuple(anchor - ROOT),
                     direction=(s * 1, -0.1, 0.1), length=0.18)
    return rig


def build_body(mb: MeshBuilder) -> None:
    T = translate

    # --- obelisk shaft: tapered 6-sided prism (hard machined facets)
    shaft = lathe([(0.0, 0.30), (0.13, 0.45), (0.145, 0.75), (0.12, 1.15),
                   (0.115, 1.45), (0.10, 1.85), (0.085, 2.15), (0.0, 2.38)],
                  6, smooth=False)
    mb.add(shaft, "Echo_Body")
    # collar rings (machined bands)
    mb.add(T(tube(0.165, 0.13, 0.07, 9, 1), (0.0, 0.62, 0.0)), "Echo_Armor")
    mb.add(T(tube(0.135, 0.105, 0.06, 9, 1), (0.0, 1.95, 0.0)), "Echo_Armor")
    # base emitter spike (points down)
    spike = _align_y(cone(0.085, 0.35, 6), 0, -1, 0)
    mb.add(T(spike, (0.0, 0.40, 0.0)), "Echo_Armor")
    mb.add(T(sphere(0.035, 6, 4), (0.0, 0.06, 0.0)), "Echo_Emissive")
    # crown spire + emissive tip
    mb.add(T(_align_y(crystal(0.38, 0.055, 5), 0, 1, 0.35), (0.0, 2.40, 0.03)),
           "Echo_Armor")
    mb.add(T(_align_y(crystal(0.14, 0.016, 4), 0, 1, 0.35), (0.0, 2.70, 0.135)),
           "Echo_Emissive")
    # greeble panels + rivet studs + emissive seam strips on the shaft
    for (gx, gy, gz, w, h) in ((-0.10, 1.05, 0.08, 0.05, 0.16), (0.11, 1.35, 0.04, 0.04, 0.2),
                               (-0.09, 1.75, -0.05, 0.05, 0.12), (0.08, 0.85, -0.07, 0.045, 0.14),
                               (0.0, 0.95, -0.11, 0.06, 0.18), (-0.06, 1.55, 0.10, 0.04, 0.1),
                               (0.07, 2.05, 0.07, 0.035, 0.09), (-0.05, 2.15, -0.07, 0.04, 0.08)):
        mb.add(T(rotate(box((w, h, 0.02)), 0, 0, 0.35), (gx, gy, gz)), "Echo_Armor")
    for (rx, ry, rz) in ((-0.12, 0.95, 0.05), (0.12, 1.15, -0.02), (-0.11, 1.65, 0.06),
                         (0.10, 1.85, -0.06), (0.0, 1.25, 0.11), (0.0, 1.75, -0.11)):
        mb.add(T(box((0.028, 0.028, 0.02)), (rx, ry, rz)), "Echo_Armor")
    for (sx, sy, sz) in ((-0.115, 1.1, 0.03), (0.115, 1.5, -0.03),
                         (0.0, 0.7, -0.125), (0.0, 2.0, 0.095)):
        mb.add(T(box((0.016, 0.34, 0.012)), (sx, sy, sz)), "Echo_Emissive")

    # --- iris: violet lens + pupil + machined socket rim
    mb.add(T(tube(0.145, 0.11, 0.06, 9, 1), (0.0, 1.62, 0.12)), "Echo_Armor")
    mb.add(T(scale(sphere(0.115, 12, 8), (1.1, 1.1, 0.5)), (0.0, 1.62, 0.17)), "Echo_Eye")
    mb.add(T(sphere(0.045, 6, 4), (0.0, 1.62, 0.22)), "Echo_Emissive")

    # --- gyroscopic ring around the iris (both driver bones animate as one)
    mb.add(T(torus(0.30, 0.028, 22, 9), tuple(IRIS_C)), "Echo_Armor")
    mb.add(T(torus(0.255, 0.011, 18, 7), tuple(IRIS_C)), "Echo_Emissive")
    for (ax_, ay_) in ((0.21, 0.21), (-0.21, 0.21), (0.21, -0.21), (-0.21, -0.21)):
        mb.add(T(box((0.05, 0.05, 0.04)), (ax_, 1.62 + ay_, 0.28)), "Echo_Armor")
        mb.add(T(sphere(0.016, 6, 4), (ax_ * 1.18, 1.62 + ay_ * 1.18, 0.28)),
               "Echo_Emissive")

    # --- orbital shard armatures around the crown
    for name, (piv, joint, shard) in ORBIT.items():
        mb.add(_seg_capsule(piv, joint, 0.022, 2, 8), "Echo_Armor")
        mb.add(_seg_capsule(joint, shard, 0.018, 2, 8), "Echo_Armor")
        mb.add(T(sphere(0.042, 6, 4), tuple(joint)), "Echo_Armor")
        mb.add(T(_align_y(crystal(0.30, 0.045, 5), 0.15, 1, 0.2), tuple(shard)), "Echo_Body")
        mb.add(T(sphere(0.028, 6, 4), tuple(shard - (0.0, 0.06, 0.0))), "Echo_Emissive")

    # --- side heat fins
    for tag, s in (("L", -1), ("R", 1)):
        anchor = ROOT + np.array((s * 0.16, -0.52, 0.04))
        fin = _align_y(box((0.02, 0.14, 0.09)), s * 1, -0.1, 0.1)
        mb.add(T(fin, tuple(anchor + np.array((s * 0.07, -0.02, 0.0)))), "Echo_Armor")
        mb.add(T(rotate(box((0.014, 0.05, 0.07)), 0.3 * s, 0, 0),
                 tuple(anchor + np.array((s * 0.14, -0.05, 0.02)))), "Echo_Armor")
        mb.add(T(sphere(0.014, 6, 4), tuple(anchor + np.array((s * 0.03, -0.06, 0.02)))),
               "Echo_Emissive")


# ----------------------------------------------------------------- animation
def anim_idle(builder: GlbBuilder, rig: Rig) -> None:
    """4.2s — hover-bob, gyroscopic ring wobble (both ring bones identical
    = rigid ring), crown shard orbits, slow gaze scan."""
    anim = builder.add_animation("AM_EyeSentinel_Idle")
    dur, n = 4.2, 12
    _tr_sine(rig, anim, "Root", dur, n, 0.06)
    for ring in ("Ring_01", "Ring_02"):
        _rot_sine(rig, anim, ring, dur, n, 7.0, 0.3, "x")
        _rot_sine(rig, anim, ring, dur, n, 5.0, 1.5, "y")
    _rot_sine(rig, anim, "Iris", dur, n, 4.0, 0.8, "y")
    _spin(rig, anim, "OrbitA_01", dur, turns=1.0, ax="y")
    _spin(rig, anim, "OrbitB_01", dur, turns=-1.0, ax="y")
    _rot_sine(rig, anim, "OrbitA_02", dur, n, 10.0, 0.0, "y")
    _rot_sine(rig, anim, "OrbitB_02", dur, n, 10.0, math.pi, "y")
    _rot_sine(rig, anim, "Crown", dur, n, 2.0, 0.6, "x")
    _rot_sine(rig, anim, "Base", dur, n, 1.5, 1.2, "x")
    _rot_sine(rig, anim, "Vent_L", dur, n, 4.0, 0.2, "x")
    _rot_sine(rig, anim, "Vent_R", dur, n, 4.0, 0.2 + math.pi, "x")


def anim_move(builder: GlbBuilder, rig: Rig) -> None:
    """1.3s — hover-glide: root pitch/bank, faster ring tumble, faster shard
    orbits, vents flutter."""
    anim = builder.add_animation("AM_EyeSentinel_Move")
    dur, n = 1.3, 12
    _tr_sine(rig, anim, "Root", dur, n, 0.05, freq=2.0, phase=0.3)
    _rot_sine(rig, anim, "Root", dur, n, 3.0, 0.4, "x")
    _rot_sine(rig, anim, "Root", dur, n, 2.0, 1.1, "z")
    for ring in ("Ring_01", "Ring_02"):
        _rot_sine(rig, anim, ring, dur, n, 10.0, 0.2, "x")
        _rot_sine(rig, anim, ring, dur, n, 8.0, 1.0, "y")
    _rot_sine(rig, anim, "Iris", dur, n, 6.0, 0.5, "y")
    _spin(rig, anim, "OrbitA_01", dur, turns=1.0, ax="y")
    _spin(rig, anim, "OrbitB_01", dur, turns=-1.0, ax="y")
    _rot_sine(rig, anim, "OrbitA_02", dur, n, 14.0, 0.0, "y")
    _rot_sine(rig, anim, "OrbitB_02", dur, n, 14.0, math.pi, "y")
    _rot_sine(rig, anim, "Crown", dur, n, 3.0, 0.9, "x")
    _rot_sine(rig, anim, "Vent_L", dur, n, 8.0, 0.0, "x")
    _rot_sine(rig, anim, "Vent_R", dur, n, 8.0, math.pi, "x")


def anim_hit(builder: GlbBuilder, rig: Rig) -> None:
    """0.4s — ring jolt, root dip, armatures flare, iris flicks down."""
    anim = builder.add_animation("AM_EyeSentinel_Hit")
    deg = DEG
    ts = [0.0, 0.08, 0.16, 0.28, 0.40]
    for ring in ("Ring_01", "Ring_02"):
        rig.add_rotation_channel(anim, ring, list(zip(ts, [
            (0, 0, 0), (-10 * deg, 0, 0), (-7 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Iris", list(zip(ts, [
        (0, 0, 0), (8 * deg, 0, 0), (5 * deg, 0, 0), (1 * deg, 0, 0), (0, 0, 0)])))
    for name, s in (("A", 1), ("B", -1)):
        rig.add_rotation_channel(anim, f"Orbit{name}_01", list(zip(ts, [
            (0, 0, 0), (0, 0, s * 12 * deg), (0, 0, s * 8 * deg),
            (0, 0, s * 2 * deg), (0, 0, 0)])))
        rig.add_rotation_channel(anim, f"Orbit{name}_02", list(zip(ts, [
            (0, 0, 0), (-8 * deg, 0, 0), (-5 * deg, 0, 0), (-1 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Crown", list(zip(ts, [
        (0, 0, 0), (6 * deg, 0, 0), (4 * deg, 0, 0), (1 * deg, 0, 0), (0, 0, 0)])))
    for tag in ("L", "R"):
        rig.add_rotation_channel(anim, f"Vent_{tag}", list(zip(ts, [
            (0, 0, 0), (10 * deg, 0, 0), (6 * deg, 0, 0), (1.5 * deg, 0, 0), (0, 0, 0)])))
    rig.add_translation_channel(anim, "Root", [
        (0.0, (0, 0, 0)), (0.08, (0, -0.07, 0)), (0.16, (0, -0.05, 0)),
        (0.28, (0, -0.012, 0)), (0.40, (0, 0, 0))])


def main() -> None:
    builder = GlbBuilder()
    for m in MATS.values():
        builder.add_material(m)

    rig = build_rig(builder)
    mb = MeshBuilder()
    build_body(mb)
    print(f"[{SPECIES}] body parts: {len(mb.parts)}, tris: {mb.triangle_count()}")

    skin_idx, prims = rig.build_skin(mb, power=3.0)
    mesh_node = builder.add_node("SK_Echo_EyeSentinel", parent=0,
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
    stats["ue_path"] = "/Game/Characters/Echoes/SK_Echo_EyeSentinel"
    record("mesh", "SK_Echo_EyeSentinel", stats)
    print(f"[{SPECIES}] {OUT_PATH}")
    print(f"[{SPECIES}] bones={stats['bones']} tris={stats['triangles']} "
          f"anims={len(stats['animations'])} bytes={stats['bytes']} validate={stats['validate']}")
    if problems:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
