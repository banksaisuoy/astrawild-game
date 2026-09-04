"""
ASTRAWILD ArtSourceGen — SK_Echo_Auroraling (Light Echo, ancient ribbon rare).

Original ASTRAWILD creature: the Auroraling — the ONE-per-world Ancient rare
(hardest capture, 0.95) revealed by the Rare Echo Bloom world event in the
deep Glimmerwood. Canon: Light element, Ancient family (weathered relic-metal
surface), Floating body plan, Small size class (runtime BodyScaleForSize 0.7
on top; authored at the hero Small floater scale per CREATURE_VISUAL_STRATEGY
§7 — floats at root y 0.85).

Design: an aurora ribbon spirit — a slender pearl-white floating body
(weathered relic-metal sheen with aged-gold band rings and a chest core
light), a small crested head with two floating antenna motes on relic spars,
and TWO long translucent aurora ribbon wings, each a 4-bone chain for the
wave ripple. The ribbons carry a soft multi-band Light emissive gradient that
shifts warm ivory (inner segments, Echo_Emissive) -> pale teal (outer
segments + eyes, Echo_Eye doubles as the outer band so the gradient lives
inside the fixed 4-slot material contract); a 2-bone veil streamer trails
below the body.

Rig: Root + Body + Head + Ribbon_L_01..04 + Ribbon_R_01..04 + Antenna_L/R +
Veil_01/02. 15 bones (ribbon chains dominate).
Anims: AM_Auroraling_Idle (4.5s slow ribbon undulation: traveling sine down
each chain, body bob, antenna sway), AM_Auroraling_Move (1.6s sailing glide:
faster traveling wave, body pitch, antennae trail), AM_Auroraling_Hit
(0.35s ribbons snap back, body dips).
Material slots: Echo_Body / Echo_Armor / Echo_Emissive / Echo_Eye.

Run:  python3 gen_echo_auroraling.py
Output: ArtSource/Meshes/Echoes/SK_Echo_Auroraling.glb
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
                       translate, rotate, scale, tube, blade)

OUT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__),
                          "..", "..", "ArtSource", "Meshes", "Echoes"))
OUT_PATH = os.path.join(OUT_DIR, "SK_Echo_Auroraling.glb")
SPECIES = "Auroraling"
DEG = math.pi / 180.0

# ----------------------------------------------------------------- materials
BODY = (0.86, 0.85, 0.82, 1.0)       # pearl-white weathered relic-metal
ARMOR = (0.62, 0.52, 0.34, 1.0)      # aged gold band/spar
IVORY = (1.00, 0.90, 0.60)           # inner-band warm-IVORY emissive
TEAL = (0.55, 0.95, 0.85)            # outer-band pale-TEAL emissive (+ eyes)

MATS = {
    "Echo_Body":     Material("Echo_Body", base_color=BODY, roughness=0.4, metallic=0.35),
    "Echo_Armor":    Material("Echo_Armor", base_color=ARMOR, roughness=0.35, metallic=0.6),
    "Echo_Emissive": Material("Echo_Emissive", base_color=(0.06, 0.05, 0.03, 1.0),
                              roughness=0.4, metallic=0.0, emissive=IVORY),
    "Echo_Eye":      Material("Echo_Eye", base_color=(0.03, 0.07, 0.06, 1.0),
                              roughness=0.25, metallic=0.0, emissive=TEAL),
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


def _seg_capsule(p0, p1, r: float, seg_v: int = 2, radial: int = 8):
    """Capsule spanning p0 -> p1 (meters), centered on the segment."""
    p0 = np.asarray(p0, dtype=float)
    p1 = np.asarray(p1, dtype=float)
    d = p1 - p0
    L = float(np.linalg.norm(d))
    body = max(L - 2.0 * r, 0.02)
    m = _align_y(_capsule(r, body, seg_v, radial), d[0], d[1], d[2])
    return translate(m, tuple((p0 + p1) * 0.5))


def _blade_between(p0, p1, width: float, thickness: float, curve: float, segs: int):
    """Ribbon/leaf blade spanning p0 -> p1 (base at p0, tip at p1)."""
    d = np.asarray(p1, dtype=float) - np.asarray(p0, dtype=float)
    L = float(np.linalg.norm(d))
    return translate(_align_y(blade(L, width, thickness, curve=curve, segs=segs),
                               d[0], d[1], d[2]), tuple(p0))


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
# World anchors (creature faces +Z, floats with root at y=0.85).
ROOT_POS = np.array((0.0, 0.85, 0.0))
BODY_BIND = np.array((0.0, 0.99, 0.0))       # Body bone bind (crown end)
HEAD_BIND = np.array((0.0, 0.965, 0.05))
HEAD_CENTER = np.array((0.0, 0.975, 0.095))
ANT_L_BASE = np.array((-0.055, 0.995, -0.005))
ANT_L_TIP = ANT_L_BASE + np.array((-0.07, 0.08, -0.09))
VEIL_BASE = np.array((0.0, 0.755, -0.045))
VEIL_END = VEIL_BASE + np.array((0.0, -0.09, -0.26))

# Ribbon chains (per side): 4 segments sweeping out-and-back like an aurora
# curtain. Joint positions in world space.
RIB_JOINTS_L = [
    np.array((-0.065, 0.855, -0.02)),    # shoulder anchor
    np.array((-0.28, 0.895, -0.125)),
    np.array((-0.425, 0.905, -0.33)),
    np.array((-0.475, 0.885, -0.525)),
    np.array((-0.47, 0.825, -0.72)),     # ribbon tip
]
RIB_WIDTHS = [0.088, 0.078, 0.064, 0.05]


def _mirror_joints():
    j = []
    for p in RIB_JOINTS_L:
        q = p.copy()
        q[0] = -q[0]
        j.append(q)
    return j


RIB_JOINTS_R = _mirror_joints()


def build_rig(builder: GlbBuilder) -> Rig:
    rig = Rig(builder, root_name="Root", root_position=tuple(ROOT_POS))
    rig.add_bone("Body", "Root", tuple(BODY_BIND - ROOT_POS),
                 direction=(0, -1, 0.08), length=0.28)
    rig.add_bone("Head", "Body", tuple(HEAD_BIND - BODY_BIND),
                 direction=(0, 0.45, 1), length=0.08)
    # ribbon chains: 4 bones per side, segments follow the joint chain
    for joints, tag in ((RIB_JOINTS_L, "L"), (RIB_JOINTS_R, "R")):
        prev = "Body"
        for k in range(4):
            p0 = joints[k]
            p1 = joints[k + 1]
            d = p1 - p0
            # parent bone world bind: Body binds at BODY_BIND; Ribbon_0m binds
            # at joints[m-1] (each child binds where the parent segment ends)
            parent_world = BODY_BIND if prev == "Body" else joints[int(prev[-1]) - 1]
            rig.add_bone(f"Ribbon_{tag}_0{k + 1}", prev,
                         tuple(p0 - parent_world),
                         direction=tuple(d),
                         length=float(np.linalg.norm(d)))
            prev = f"Ribbon_{tag}_0{k + 1}"
    # floating antenna motes (relic spars off the head, bases swept back so
    # the eyes keep their Head binding)
    for base, tip, tag in ((ANT_L_BASE, ANT_L_TIP, "L"),
                           ((0.055, 0.995, -0.005), (0.125, 1.075, -0.095), "R")):
        d = np.asarray(tip) - np.asarray(base)
        rig.add_bone(f"Antenna_{tag}", "Head", tuple(np.asarray(base) - HEAD_BIND),
                     direction=tuple(d), length=float(np.linalg.norm(d)))
    # veil streamer below the body (2 bones)
    mid = VEIL_BASE + (VEIL_END - VEIL_BASE) * 0.5
    rig.add_bone("Veil_01", "Body", tuple(VEIL_BASE - BODY_BIND),
                 direction=tuple(mid - VEIL_BASE),
                 length=float(np.linalg.norm(mid - VEIL_BASE)))
    rig.add_bone("Veil_02", "Veil_01", tuple(mid - VEIL_BASE),
                 direction=tuple(VEIL_END - mid),
                 length=float(np.linalg.norm(VEIL_END - mid)))
    return rig


def build_body(mb: MeshBuilder) -> None:
    T = translate

    # --- slender pearl-white body (weathered relic-metal ovoid)
    prof = [(0.0, 0.145), (0.018, 0.125), (0.042, 0.075), (0.062, 0.03),
            (0.066, -0.04), (0.058, -0.07), (0.048, -0.10), (0.02, -0.145),
            (0.0, -0.17)]
    mb.add(T(lathe(prof, 13, smooth=True), tuple(ROOT_POS)), "Echo_Body")
    # aged-gold band rings (waist + collar)
    mb.add(T(tube(0.075, 0.058, 0.022, 10, 1), (0, 0.815, 0.0)), "Echo_Armor")
    mb.add(T(tube(0.052, 0.038, 0.02, 9, 1), (0, 0.945, 0.01)), "Echo_Armor")
    # flank etch plates + small stabilizer fins (relic engraving)
    for s in (-1, 1):
        mb.add(T(rotate(box((0.012, 0.09, 0.03)), 0, 0, 0.35 * s),
                 (s * 0.062, 0.87, -0.02)), "Echo_Armor")
        mb.add(T(rotate(box((0.01, 0.07, 0.024)), 0, 0, 0.5 * s),
                 (s * 0.058, 0.80, -0.03)), "Echo_Armor")
        mb.add(_blade_between((s * 0.06, 0.755, -0.02),
                              (s * 0.105, 0.70, -0.075), 0.022, 0.005, 0.01, 4),
               "Echo_Body")
    # chest core light (the relic's heart)
    mb.add(T(sphere(0.02, 7, 5), (0, 0.885, 0.058)), "Echo_Emissive")

    # --- head: small crested crown + eyes (pale teal)
    mb.add(T(sphere(0.052, 10, 8), tuple(HEAD_CENTER)), "Echo_Body")
    mb.add(T(rotate(crystal(0.075, 0.014, 5), -0.2, 0, 0),
             (0, 1.045, 0.03)), "Echo_Armor")
    for s in (-1, 1):
        mb.add(T(sphere(0.011, 6, 4), (s * 0.024, 0.965, 0.038)), "Echo_Eye")

    # --- antenna spars with floating motes
    for base, tip in ((ANT_L_BASE, ANT_L_TIP),
                      ((0.055, 0.995, -0.005), (0.125, 1.075, -0.095))):
        mb.add(_seg_capsule(base, tip, 0.007, 1, 6), "Echo_Armor")
        mb.add(T(sphere(0.016, 7, 5), tuple(tip)), "Echo_Emissive")
        mb.add(T(torus(0.026, 0.005, 8, 4), tuple(tip)), "Echo_Emissive")
        # mid-shaft relic ring
        mid = np.asarray(base) + (np.asarray(tip) - np.asarray(base)) * 0.45
        mb.add(T(torus(0.014, 0.004, 7, 4), tuple(mid)), "Echo_Armor")

    # --- aurora ribbon wings: 4 blades per side; inner 2 bands warm ivory
    #     (Echo_Emissive), outer 2 bands pale teal (Echo_Eye) — the gradient
    for joints, s in ((RIB_JOINTS_L, -1), (RIB_JOINTS_R, 1)):
        for k in range(4):
            p0 = joints[k]
            p1 = joints[k + 1]
            d = p1 - p0
            L = float(np.linalg.norm(d))
            start = p0 - d * (0.12 if k > 0 else 0.0)          # shingle overlap
            end = p1 + d * (0.10 if k < 3 else 0.0)
            mat = "Echo_Emissive" if k < 2 else "Echo_Eye"
            mb.add(_blade_between(start, end, RIB_WIDTHS[k], 0.007,
                                  0.02 * s, 7), mat)
            # wisp overlay strip (trailing edge accent, same band)
            wisp = _blade_between(start + np.array((0.0, -0.012, 0.0)),
                                  end - d * 0.22, RIB_WIDTHS[k] * 0.45,
                                  0.005, 0.04 * s, 5)
            mb.add(wisp, mat)
            # relic-metal diamond node at each ribbon joint + edge motes
            if k < 3:
                node = rotate(box((0.02, 0.02, 0.032)), 0.0, 0.0, 0.78)
                mb.add(T(node, tuple(p1 - d * 0.12)), "Echo_Armor")
                perp = np.cross(d / L, (0.0, 1.0, 0.0))
                if np.linalg.norm(perp) < 1e-6:
                    perp = np.array((1.0, 0.0, 0.0))
                perp = perp / np.linalg.norm(perp) * s
                mote_p = p1 - d * 0.12 + np.array((0.0, -RIB_WIDTHS[k] * 0.5, 0.0)) \
                    - perp * 0.01
                mb.add(T(sphere(0.009, 5, 3), tuple(mote_p)), "Echo_Emissive")
        # shoulder pauldron (ribbon root mount)
        mb.add(T(rotate(box((0.035, 0.05, 0.05)), 0.0, 0.0, 0.3 * s),
                 tuple(joints[0])), "Echo_Armor")

    # --- veil streamer below the body
    mid = VEIL_BASE + (VEIL_END - VEIL_BASE) * 0.5
    mb.add(_blade_between(VEIL_BASE, mid, 0.05, 0.006, 0.02, 5), "Echo_Emissive")
    mb.add(_blade_between(mid, VEIL_END, 0.036, 0.005, 0.03, 5), "Echo_Emissive")
    mb.add(T(sphere(0.012, 6, 4), tuple(VEIL_END)), "Echo_Emissive")
    mb.add(T(sphere(0.008, 5, 3), tuple(VEIL_BASE + (VEIL_END - VEIL_BASE) * 0.75)),
           "Echo_Emissive")


# ----------------------------------------------------------------- animation
def _ribbon_wave(rig: Rig, anim, tag: str, dur: float, n: int, base_amp: float,
                 phase0: float, speed: float = 1.0) -> None:
    """Traveling sine wave down a 4-bone ribbon chain (phase delay per joint)."""
    for k in range(1, 5):
        amp = base_amp * (0.7 + 0.22 * k)
        ph = phase0 + 0.55 * (k - 1) * speed
        _rot_sine(rig, anim, f"Ribbon_{tag}_0{k}", dur, n, amp, ph, "y")
        _rot_sine(rig, anim, f"Ribbon_{tag}_0{k}", dur, n, amp * 0.55,
                  ph + 0.3, "z")


def anim_idle(builder: GlbBuilder, rig: Rig) -> None:
    """4.5s — slow ribbon undulation, body bob, antenna sway, veil drift."""
    anim = builder.add_animation("AM_Auroraling_Idle")
    dur, n = 4.5, 12
    _tr_sine(rig, anim, "Root", dur, n, 0.045)
    _ribbon_wave(rig, anim, "L", dur, n, 5.0, 0.0)
    _ribbon_wave(rig, anim, "R", dur, n, 5.0, math.pi)
    _rot_sine(rig, anim, "Body", dur, n, 1.5, 0.5)
    _rot_sine(rig, anim, "Head", dur, n, 2.5, 1.0, "y")
    _rot_sine(rig, anim, "Head", dur, n, 1.2, 0.4)
    for tag, ph in (("L", 0.0), ("R", math.pi)):
        _rot_sine(rig, anim, f"Antenna_{tag}", dur, n, 6.0, ph, "y")
        _rot_sine(rig, anim, f"Antenna_{tag}", dur, n, 4.0, ph + 0.5)
    _rot_sine(rig, anim, "Veil_01", dur, n, 5.0, 1.2, "y")
    _rot_sine(rig, anim, "Veil_02", dur, n, 7.0, 1.8, "y")
    _rot_sine(rig, anim, "Veil_01", dur, n, 3.0, 0.8)


def anim_move(builder: GlbBuilder, rig: Rig) -> None:
    """1.6s — ribbon sailing glide: faster traveling wave, body pitch, veils
    and antennae trail."""
    anim = builder.add_animation("AM_Auroraling_Move")
    dur, n = 1.6, 12
    _tr_sine(rig, anim, "Root", dur, n, 0.035, freq=2.0, phase=0.5)
    _ribbon_wave(rig, anim, "L", dur, n, 12.0, 0.0, speed=1.6)
    _ribbon_wave(rig, anim, "R", dur, n, 12.0, math.pi, speed=1.6)
    # base joints stream back under the body's forward lean
    for tag, s in (("L", -1), ("R", 1)):
        _rot_sine(rig, anim, f"Ribbon_{tag}_01", dur, n, 9.0, 0.3, "x")
    _rot_sine(rig, anim, "Body", dur, n, 4.0, 0.3)
    _rot_sine(rig, anim, "Head", dur, n, 3.0, 1.2, "x")
    _rot_sine(rig, anim, "Head", dur, n, 2.5, 0.6, "y")
    for tag, ph in (("L", 0.9), ("R", math.pi + 0.9)):
        _rot_sine(rig, anim, f"Antenna_{tag}", dur, n, 12.0, ph, "y")
        _rot_sine(rig, anim, f"Antenna_{tag}", dur, n, 8.0, ph + 0.4)
    _rot_sine(rig, anim, "Veil_01", dur, n, 9.0, 1.1, "y")
    _rot_sine(rig, anim, "Veil_02", dur, n, 13.0, 1.7, "y")
    _rot_sine(rig, anim, "Veil_01", dur, n, 6.0, 0.7)


def anim_hit(builder: GlbBuilder, rig: Rig) -> None:
    """0.35s — ribbons snap back and up, body dips, antennae whip, veil kicks."""
    anim = builder.add_animation("AM_Auroraling_Hit")
    deg = DEG
    ts = [0.0, 0.07, 0.14, 0.24, 0.35]
    for tag, s in (("L", -1), ("R", 1)):
        for k in range(1, 5):
            amp = 18.0 - 2.5 * k
            rig.add_rotation_channel(anim, f"Ribbon_{tag}_0{k}", list(zip(ts, [
                (0, 0, 0), (s * amp * deg, 0, 0), (s * amp * 0.7 * deg, 0, 0),
                (s * amp * 0.2 * deg, 0, 0), (0, 0, 0)])))
        rig.add_rotation_channel(anim, f"Antenna_{tag}", list(zip(ts, [
            (0, 0, 0), (-14 * deg, 0, 0), (-9 * deg, 0, 0),
            (-2 * deg, 0, 0), (0, 0, 0)])))
    rig.add_translation_channel(anim, "Root", [
        (0.0, (0, 0, 0)), (0.07, (0, -0.07, 0)), (0.14, (0, -0.05, 0)),
        (0.24, (0, -0.015, 0)), (0.35, (0, 0, 0))])
    rig.add_rotation_channel(anim, "Body", list(zip(ts, [
        (0, 0, 0), (-6 * deg, 0, 0), (-4 * deg, 0, 0), (1 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Head", list(zip(ts, [
        (0, 0, 0), (-8 * deg, 0, 0), (-5 * deg, 0, 0), (1.5 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Veil_01", list(zip(ts, [
        (0, 0, 0), (12 * deg, 0, 0), (8 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Veil_02", list(zip(ts, [
        (0, 0, 0), (16 * deg, 0, 0), (11 * deg, 0, 0), (3 * deg, 0, 0), (0, 0, 0)])))


def main() -> None:
    builder = GlbBuilder()
    for m in MATS.values():
        builder.add_material(m)

    rig = build_rig(builder)
    mb = MeshBuilder()
    build_body(mb)
    print(f"[{SPECIES}] body parts: {len(mb.parts)}, tris: {mb.triangle_count()}")

    skin_idx, prims = rig.build_skin(mb, power=3.0)
    mesh_node = builder.add_node("SK_Echo_Auroraling", parent=0,
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
    stats["ue_path"] = "/Game/Characters/Echoes/SK_Echo_Auroraling"
    record("mesh", "SK_Echo_Auroraling", stats)
    print(f"[{SPECIES}] {OUT_PATH}")
    print(f"[{SPECIES}] bones={stats['bones']} tris={stats['triangles']} "
          f"anims={len(stats['animations'])} bytes={stats['bytes']} validate={stats['validate']}")
    if problems:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
