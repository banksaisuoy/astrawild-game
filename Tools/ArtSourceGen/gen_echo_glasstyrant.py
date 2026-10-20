"""
ASTRAWILD ArtSourceGen — SK_Echo_GlassTyrant (Ash Echo, desert world boss).

Original ASTRAWILD creature: The Glass Tyrant — MQ-14 world boss of the
Sunscar Desert, a shard-mass tyrant of fused dune glass. Canon: Ash element,
Elemental family (energy-glass surface), Crystalline body plan, Large size
class (runtime BodyScaleForSize 1.4 on top; authored at ~2x hero base scale
per CREATURE_VISUAL_STRATEGY §7).

Design: a walking shard-mass — faceted crystal cluster torso (lower mass +
upper mass + core spire) ringed by large flank shards and a hip shard crown;
asymmetric blade-limbs (long left reach, shorter right) built from faceted
blade slabs with ember edge-strips; a single tall crest blade sweeping off
the core spire; fracture-line emissive veins (dying-coal orange) cross the
torso facets and blade edges; three crystal stilts carry the mass; two shard
satellites hover on jointed armatures at the shoulders and orbit them.

Rig: Root + Core/UpperCore/CoreTop + Crest_01..02 + Cluster_L/R +
Blade_L_Upper/Lower + Blade_R_Upper/Lower + LegA_L/R + LegB +
Sat_L_01..02 + Sat_R_01..02. 19 bones.
Anims: AM_GlassTyrant_Idle (4.0s energy-glass pulse: core breathe, crest
sway, satellites full shoulder orbit, stilt micro-shift),
AM_GlassTyrant_Move (1.4s heavy blade-drag walk: blades X +/-16..20deg,
stilts step, core yaw, root lurch), AM_GlassTyrant_Hit (0.4s shudder:
blades flare Z, crest whips, core jolts back, root dips).
Material slots: Echo_Body / Echo_Armor / Echo_Emissive / Echo_Eye.

Run:  python3 gen_echo_glasstyrant.py
Output: ArtSource/Meshes/Echoes/SK_Echo_GlassTyrant.glb
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
OUT_PATH = os.path.join(OUT_DIR, "SK_Echo_GlassTyrant.glb")
SPECIES = "GlassTyrant"
DEG = math.pi / 180.0

# ----------------------------------------------------------------- materials
BODY = (0.44, 0.40, 0.38, 1.0)       # ember-grey dune glass
ARMOR = (0.27, 0.24, 0.23, 1.0)      # dark smoky crystal plate
EMBER = (0.95, 0.42, 0.14)           # Ash dying-coal ORANGE emissive

MATS = {
    "Echo_Body":     Material("Echo_Body", base_color=BODY, roughness=0.3, metallic=0.05),
    "Echo_Armor":    Material("Echo_Armor", base_color=ARMOR, roughness=0.35, metallic=0.12),
    "Echo_Emissive": Material("Echo_Emissive", base_color=(0.05, 0.03, 0.02, 1.0),
                              roughness=0.4, metallic=0.0, emissive=EMBER),
    "Echo_Eye":      Material("Echo_Eye", base_color=(0.08, 0.04, 0.02, 1.0),
                              roughness=0.25, metallic=0.0,
                              emissive=(1.0, 0.55, 0.25)),
}


# ------------------------------------------------------------- local helpers
def _align_y(m, ax: float, ay: float, az: float):
    """Rotates a Y-aligned shape so its +Y axis points along (ax, ay, az)."""
    n = math.sqrt(ax * ax + ay * ay + az * az)
    if n < 1e-9:
        return m
    ax, ay, az = ax / n, ay / n, az / n
    rx = math.asin(max(-1.0, min(1.0, az)))
    rz = math.atan2(-ax, ay)
    return rotate(m, rx, 0.0, rz)


def _shard(p, dx: float, dy: float, dz: float, h: float, r: float,
           sides: int = 6):
    """Crystal shard mesh at p, axis (dx,dy,dz), height h, radius r."""
    return translate(_align_y(crystal(h, r, sides), dx, dy, dz), tuple(p))


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
# Core masses (world positions); creature faces +Z.
CORE = np.array((0.0, 1.15, -0.05))
UPPER = np.array((0.0, 1.72, 0.03))
CORETOP = np.array((0.0, 2.22, 0.08))
EYE = np.array((0.0, 1.80, 0.34))
CREST_1 = np.array((0.10, 2.52, 0.20))
CREST_2 = np.array((0.34, 2.94, 0.34))

BLADE = {  # asymmetric blade-limb waypoints (shoulder -> elbow -> tip)
    "L": [np.array((-0.55, 1.40, 0.00)), np.array((-0.88, 1.52, 0.03)),
          np.array((-1.22, 0.84, 0.20)), np.array((-1.44, 0.14, 0.41))],
    "R": [np.array((0.50, 1.38, -0.02)), np.array((0.78, 1.48, 0.0)),
          np.array((1.02, 0.92, 0.15)), np.array((1.16, 0.38, 0.30))],
}
LEGS = {  # crystal stilts (front pair + rear single)
    "A_L": [np.array((-0.28, 0.80, 0.07)), np.array((-0.45, 0.03, 0.18))],
    "A_R": [np.array((0.28, 0.80, 0.07)), np.array((0.45, 0.03, 0.18))],
    "B":   [np.array((0.0, 0.77, -0.40)), np.array((0.0, 0.02, -0.62))],
}
SAT = {  # satellite armatures (shoulder pivot -> joint -> shard)
    "L": [np.array((-0.42, 1.88, 0.01)), np.array((-0.86, 1.97, -0.03)),
          np.array((-1.10, 2.02, 0.01))],
    "R": [np.array((0.42, 1.86, -0.01)), np.array((0.84, 1.95, -0.05)),
          np.array((1.06, 2.00, -0.01))],
}


def build_rig(builder: GlbBuilder) -> Rig:
    rig = Rig(builder, root_name="Root", root_position=(0, 0, 0))
    rig.add_bone("Core", "Root", tuple(CORE), direction=(0, 1, 0.15), length=0.56)
    rig.add_bone("UpperCore", "Core", tuple(UPPER - CORE), direction=(0, 1, 0.1), length=0.50)
    rig.add_bone("CoreTop", "UpperCore", tuple(CORETOP - UPPER), direction=(0, 1, 0.2), length=0.40)
    rig.add_bone("Crest_01", "CoreTop", tuple(CREST_1 - CORETOP),
                 direction=tuple(CREST_2 - CREST_1), length=float(np.linalg.norm(CREST_2 - CREST_1)))
    rig.add_bone("Crest_02", "Crest_01", tuple(CREST_2 - CREST_1),
                 direction=(0.4, 1, 0.3), length=0.35)
    for tag, s in (("L", -1), ("R", 1)):
        sh, el, kw, tip = BLADE[tag]
        rig.add_bone(f"Cluster_{tag}", "Core", tuple(sh - CORE),
                     direction=(s * 1, 0.35, 0.1), length=0.35)
        rig.add_bone(f"Blade_{tag}_Upper", f"Cluster_{tag}", tuple(el - sh),
                     direction=tuple(kw - el), length=float(np.linalg.norm(kw - el)))
        rig.add_bone(f"Blade_{tag}_Lower", f"Blade_{tag}_Upper", tuple(kw - el),
                     direction=tuple(tip - kw), length=float(np.linalg.norm(tip - kw)))
    for name, (hip, foot) in LEGS.items():
        parent = "Core"
        rig.add_bone(f"Leg{name}", parent, tuple(hip - CORE),
                     direction=tuple(foot - hip), length=float(np.linalg.norm(foot - hip)))
    for tag in ("L", "R"):
        piv, joint, shard = SAT[tag]
        rig.add_bone(f"Sat_{tag}_01", "UpperCore", tuple(piv - UPPER),
                     direction=tuple(joint - piv), length=float(np.linalg.norm(joint - piv)))
        rig.add_bone(f"Sat_{tag}_02", f"Sat_{tag}_01", tuple(joint - piv),
                     direction=tuple(shard - joint), length=float(np.linalg.norm(shard - joint)))
    return rig


def build_body(mb: MeshBuilder) -> None:
    T = translate

    # --- lower torso: faceted core mass + ring of large flank shards
    lower = lathe([(0.30, 0.0), (0.43, 0.16), (0.40, 0.36), (0.31, 0.54)],
                  12, smooth=True)
    mb.add(T(lower, tuple(CORE + (0.0, -0.28, 0.0))), "Echo_Body")
    # flank shard ring (10 large facets)
    for k in range(10):
        a = 2.0 * math.pi * k / 10
        ca, sa = math.cos(a), math.sin(a)
        p = CORE + np.array((0.40 * ca, -0.16 + 0.05 * sa, 0.36 * sa))
        h = 0.62 + 0.22 * ((k % 2) * 0.5 + 0.5)
        mb.add(_shard(p, 0.85 * ca, 0.5, 0.85 * sa, h, 0.095, 6), "Echo_Body")
    # hip shard crown (dense small shards at the waist)
    for k in range(10):
        a = 2.0 * math.pi * (k + 0.5) / 10
        ca, sa = math.cos(a), math.sin(a)
        p = CORE + np.array((0.30 * ca, -0.05, 0.26 * sa))
        mb.add(_shard(p, 0.7 * ca, 1.0, 0.7 * sa, 0.34 + 0.1 * (k % 3 == 0), 0.05, 5),
               "Echo_Armor")
    # fracture-line emissive veins across the lower mass
    for (vx, vy, vz, ry) in ((-0.22, 0.92, 0.18, 0.5), (0.20, 1.02, 0.20, -0.4),
                             (0.0, 0.86, 0.32, 1.2), (-0.30, 1.10, -0.12, 0.9),
                             (0.26, 0.90, -0.16, -0.8), (-0.34, 0.98, -0.30, 0.7),
                             (0.10, 1.08, 0.30, -1.0), (0.32, 1.12, -0.28, -0.5)):
        mb.add(T(rotate(box((0.024, 0.016, 0.34)), 0.0, ry, 0.35),
                 (vx, vy, vz)), "Echo_Emissive")
    # back shard pack (rear of the upper mass, fracture shoulders)
    for (bx, by, bz, h) in ((-0.16, 1.85, -0.22, 0.5), (0.0, 1.95, -0.24, 0.62),
                            (0.15, 1.83, -0.20, 0.46), (-0.05, 1.70, -0.26, 0.4)):
        mb.add(_shard(np.array((bx, by, bz)), 0.15, 1.0, -0.5, h, 0.06, 6), "Echo_Body")

    # --- upper torso: faceted mass + eye socket
    upper = lathe([(0.26, 0.0), (0.34, 0.14), (0.30, 0.32), (0.20, 0.48)], 12, smooth=True)
    mb.add(T(upper, tuple(UPPER + (0.0, -0.16, 0.0))), "Echo_Body")
    for k in range(6):
        a = 2.0 * math.pi * (k + 0.2) / 6
        ca, sa = math.cos(a), math.sin(a)
        if sa > 0.6:   # leave the face facet clear for the eye
            continue
        p = UPPER + np.array((0.26 * ca, 0.02 + 0.04 * sa, 0.22 * sa))
        mb.add(_shard(p, 0.8 * ca, 0.8, 0.8 * sa, 0.5 + 0.14 * (k % 2), 0.07, 6), "Echo_Body")
    # the eye: hot ember core in a dark socket ring
    mb.add(T(tube(0.16, 0.115, 0.07, radial=12, wall_segs=1), tuple(EYE + (0, 0, -0.015))),
           "Echo_Armor")
    mb.add(T(scale(sphere(0.115, 12, 8), (1.0, 1.0, 0.6)), tuple(EYE)), "Echo_Eye")
    for s in (-1, 1):
        mb.add(T(rotate(box((0.20, 0.045, 0.10)), -0.25 * s, 0, 0),
                 tuple(EYE + (s * 0.17, 0.10, -0.02))), "Echo_Armor")
    # upper fracture veins
    for (vx, vy, vz, ry) in ((-0.16, 1.62, 0.24, 0.3), (0.18, 1.72, 0.22, -0.5),
                             (0.0, 1.94, 0.18, 0.9)):
        mb.add(T(rotate(box((0.02, 0.014, 0.30)), 0.0, ry, 0.3), (vx, vy, vz)), "Echo_Emissive")

    # --- core spire + crest blade
    mb.add(_shard(CORETOP + (0.0, 0.05, 0.0), 0.0, 1.0, 0.25, 0.55, 0.06, 6), "Echo_Armor")
    mb.add(_shard(CREST_1 + (0.0, 0.1, 0.0), 0.45, 1.0, 0.25, 0.85, 0.07, 6), "Echo_Armor")
    mb.add(_shard(CREST_2 + (0.0, 0.08, 0.0), 0.4, 1.0, 0.3, 0.62, 0.055, 5), "Echo_Armor")
    mb.add(T(rotate(box((0.024, 0.62, 0.02)), 0.0, 0.0, -0.42),
             tuple(CREST_1 + (0.08, 0.42, 0.04))), "Echo_Emissive")
    mb.add(T(rotate(box((0.02, 0.48, 0.018)), 0.0, 0.0, -0.35),
             tuple(CREST_2 + (0.07, 0.32, 0.03))), "Echo_Emissive")

    # --- asymmetric blade-limbs (faceted slabs + ember edge strips)
    for tag, s in (("L", -1), ("R", 1)):
        sh, el, kw, tip = BLADE[tag]
        # shoulder shard cluster
        for (ox, oy, h) in ((0.0, 0.10, 0.44), (s * 0.14, 0.05, 0.34), (0.02, -0.06, 0.30)):
            mb.add(_shard(sh + np.array((ox, oy, 0.02)), s * 0.5, 1.0, 0.3, h, 0.075, 6),
                  "Echo_Body")
        # blade segments: slab + edge strip, spanning elbow->knee and knee->tip
        for (a0, a1) in ((el, kw), (kw, tip)):
            d = np.asarray(a1) - np.asarray(a0)
            L = float(np.linalg.norm(d))
            mid = (np.asarray(a0) + np.asarray(a1)) * 0.5
            blade = _align_y(box((0.11, L * 0.92, 0.035)), *d)
            mb.add(T(blade, tuple(mid)), "Echo_Body")
            edge = _align_y(box((0.02, L * 0.8, 0.014)), *d)
            mb.add(T(edge, tuple(mid + np.array((s * 0.048, 0, 0)))), "Echo_Emissive")
        # knee joint crystals on the blade limbs
        mb.add(_shard(kw, s * 0.4, -1.0, 0.2, 0.34, 0.05, 5), "Echo_Armor")
        mb.add(_shard(tip, s * 0.3, -1.0, 0.35, 0.22, 0.03, 4), "Echo_Emissive")

    # --- crystal stilts (front pair + rear)
    for name, (hip, foot) in LEGS.items():
        d = foot - hip
        L = float(np.linalg.norm(d))
        stilt = _align_y(crystal(L * 1.15, 0.09, 5), *d)
        mb.add(T(translate(stilt, (0, 0, 0)), tuple(hip)), "Echo_Armor")
        mb.add(T(scale(sphere(0.055, 8, 6), (1.5, 0.5, 1.5)), tuple(foot + (0, 0.01, 0))),
               "Echo_Armor")

    # --- shard satellites on jointed armatures
    for tag, s in (("L", -1), ("R", 1)):
        piv, joint, shard = SAT[tag]
        # arm segments (thin faceted struts)
        for (p0, p1) in ((piv, joint), (joint, shard)):
            d = np.asarray(p1) - np.asarray(p0)
            L = float(np.linalg.norm(d))
            strut = _align_y(box((0.045, L, 0.045)), *d)
            mb.add(T(strut, tuple((np.asarray(p0) + np.asarray(p1)) * 0.5)), "Echo_Armor")
        mb.add(T(sphere(0.05, 8, 6), tuple(joint)), "Echo_Armor")
        # the floating shard pair + ember cores
        mb.add(_shard(shard + (0, 0.05, 0), 0.2, 1.0, 0.1, 0.4, 0.05, 5), "Echo_Body")
        mb.add(_shard(shard + (s * 0.09, -0.04, 0.02), 0.5, 1.0, 0.1, 0.24, 0.035, 5),
               "Echo_Armor")
        mb.add(T(sphere(0.032, 6, 4), tuple(shard + (0, 0.0, 0.0))), "Echo_Emissive")


# ----------------------------------------------------------------- animation
def anim_idle(builder: GlbBuilder, rig: Rig) -> None:
    """4.0s — energy-glass pulse: core breathe, crest sway, satellites orbit
    their shoulders once per loop, stilts micro-shift."""
    anim = builder.add_animation("AM_GlassTyrant_Idle")
    dur, n = 4.0, 12
    _rot_sine(rig, anim, "Core", dur, n, 0.8, 0.0)
    _rot_sine(rig, anim, "UpperCore", dur, n, 1.4, 0.5)
    _rot_sine(rig, anim, "CoreTop", dur, n, 1.8, 1.0)
    _rot_sine(rig, anim, "Crest_01", dur, n, 2.5, 0.7, "x")
    _rot_sine(rig, anim, "Crest_01", dur, n, 2.0, 1.4, "z")
    _rot_sine(rig, anim, "Crest_02", dur, n, 3.5, 0.9, "x")
    for tag in ("L", "R"):
        _rot_sine(rig, anim, f"Cluster_{tag}", dur, n, 2.0, 0.3, "z")
    _spin(rig, anim, "Sat_L_01", dur, turns=1.0, ax="y")
    _spin(rig, anim, "Sat_R_01", dur, turns=-1.0, ax="y")
    _rot_sine(rig, anim, "Sat_L_02", dur, n, 6.0, 0.0, "y")
    _rot_sine(rig, anim, "Sat_R_02", dur, n, 6.0, math.pi, "y")
    for name in ("LegA_L", "LegA_R", "LegB"):
        _rot_sine(rig, anim, name, dur, n, 0.5, 0.4, "x")
    _tr_sine(rig, anim, "Root", dur, n, 0.012)


def anim_move(builder: GlbBuilder, rig: Rig) -> None:
    """1.4s — heavy blade-drag walk: blades swing X +/-16..20deg alternating,
    stilts step in tripod phase, core yaws, root lurches."""
    anim = builder.add_animation("AM_GlassTyrant_Move")
    dur, n = 1.4, 12
    for tag, ph in (("L", 0.0), ("R", math.pi)):
        _rot_sine(rig, anim, f"Blade_{tag}_Upper", dur, n, 16.0, ph, "x")
        _rot_sine(rig, anim, f"Blade_{tag}_Lower", dur, n, 20.0, ph + 0.9, "x")
        _rot_sine(rig, anim, f"Cluster_{tag}", dur, n, 3.0, ph + 0.4, "z")
    _rot_sine(rig, anim, "LegA_L", dur, n, 8.0, 0.0, "x")
    _rot_sine(rig, anim, "LegA_R", dur, n, 8.0, math.pi, "x")
    _rot_sine(rig, anim, "LegB", dur, n, 7.0, math.pi / 2, "x")
    _rot_sine(rig, anim, "Core", dur, n, 2.5, 0.3, "y")
    _rot_sine(rig, anim, "UpperCore", dur, n, 2.0, 0.8)
    _rot_sine(rig, anim, "Crest_01", dur, n, 4.0, 1.1, "z")
    _rot_sine(rig, anim, "Crest_02", dur, n, 6.0, 1.5, "z")
    _spin(rig, anim, "Sat_L_01", dur, turns=1.0, ax="y")
    _spin(rig, anim, "Sat_R_01", dur, turns=-1.0, ax="y")
    _tr_sine(rig, anim, "Root", dur, n, 0.03, freq=2.0, phase=0.4)


def anim_hit(builder: GlbBuilder, rig: Rig) -> None:
    """0.4s — shudder: blades flare out Z, crest whips, core jolts back,
    root dips."""
    anim = builder.add_animation("AM_GlassTyrant_Hit")
    deg = DEG
    ts = [0.0, 0.08, 0.16, 0.28, 0.40]
    for tag, s in (("L", -1), ("R", 1)):
        rig.add_rotation_channel(anim, f"Blade_{tag}_Upper", list(zip(ts, [
            (0, 0, 0), (0, 0, s * 10 * deg), (0, 0, s * 7 * deg),
            (0, 0, s * 2 * deg), (0, 0, 0)])))
        rig.add_rotation_channel(anim, f"Blade_{tag}_Lower", list(zip(ts, [
            (0, 0, 0), (-8 * deg, 0, s * 12 * deg), (-5 * deg, 0, s * 8 * deg),
            (-1 * deg, 0, s * 2 * deg), (0, 0, 0)])))
        rig.add_rotation_channel(anim, f"Cluster_{tag}", list(zip(ts, [
            (0, 0, 0), (0, 0, s * 6 * deg), (0, 0, s * 4 * deg),
            (0, 0, s * 1 * deg), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "UpperCore", list(zip(ts, [
        (0, 0, 0), (-6 * deg, 0, 0), (-4 * deg, 0, 0), (1 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Core", list(zip(ts, [
        (0, 0, 0), (-3 * deg, 0, 0), (-2 * deg, 0, 0), (0.5 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Crest_01", list(zip(ts, [
        (0, 0, 0), (12 * deg, 0, 0), (8 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Crest_02", list(zip(ts, [
        (0, 0, 0), (16 * deg, 0, 0), (11 * deg, 0, 0), (3 * deg, 0, 0), (0, 0, 0)])))
    for name in ("LegA_L", "LegA_R", "LegB"):
        rig.add_rotation_channel(anim, name, list(zip(ts, [
            (0, 0, 0), (-5 * deg, 0, 0), (-3 * deg, 0, 0), (-1 * deg, 0, 0), (0, 0, 0)])))
    rig.add_translation_channel(anim, "Root", [
        (0.0, (0, 0, 0)), (0.08, (0, -0.06, 0)), (0.16, (0, -0.04, 0)),
        (0.28, (0, -0.01, 0)), (0.40, (0, 0, 0))])


def main() -> None:
    builder = GlbBuilder()
    for m in MATS.values():
        builder.add_material(m)

    rig = build_rig(builder)
    mb = MeshBuilder()
    build_body(mb)
    print(f"[{SPECIES}] body parts: {len(mb.parts)}, tris: {mb.triangle_count()}")

    skin_idx, prims = rig.build_skin(mb, power=3.0)
    mesh_node = builder.add_node("SK_Echo_GlassTyrant", parent=0,
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
    stats["ue_path"] = "/Game/Characters/Echoes/SK_Echo_GlassTyrant"
    record("mesh", "SK_Echo_GlassTyrant", stats)
    print(f"[{SPECIES}] {OUT_PATH}")
    print(f"[{SPECIES}] bones={stats['bones']} tris={stats['triangles']} "
          f"anims={len(stats['animations'])} bytes={stats['bytes']} validate={stats['validate']}")
    if problems:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
