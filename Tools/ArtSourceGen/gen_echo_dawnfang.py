"""
ASTRAWILD ArtSourceGen — SK_Echo_Dawnfang (Light Echo, Sunken Vault boss).

Original ASTRAWILD creature: the Dawnfang — MQ-10 dungeon boss of the Sunken
Vault (Tidebreaker Isles), the sea-dragon that coils at the vault's heart and
wards its long watch. Canon: Light element, Dragon family (scaled hide +
wet sheen), Serpent body plan, Large size class (runtime BodyScaleForSize
1.4 on top; authored at ~2x hero base scale per CREATURE_VISUAL_STRATEGY §7).

Design: an aquatic dragon-serpent hybrid — barrel body chain (10 segments)
in a gentle plan-view S-curve with the head raised; a dorsal fin ribbon of
overlapping plates runs head-crest to tail; two large paddle fins at the
chest and two small hind paddles at the hips; a bioluminescent dawn-gold
lateral line runs head-to-tail on both flanks (strip + node cluster pattern);
a wide tail fluke; jaws with fangs and a light-lure antenna (emissive bulb)
bobbing in front of the snout.

Rig: Root + Tail_01..03 + Hips + Spine_01..03 + Chest/Neck/Head + Jaw/Lure +
Paddle_L/R + HindPaddle_L/R + Dorsal_01..02 + TailFin. 20 bones.
Anims: AM_Dawnfang_Idle (4.2s water hover: spine ripple, paddle wave, lure
bob, tail sway, root float), AM_Dawnfang_Move (1.4s serpentine swim: spine
Y +/-7deg traveling wave, tail +/-12..20deg, paddles rowing alternating,
root bob), AM_Dawnfang_Hit (0.4s contract: spine curls, jaw gapes, lure
whips, paddles tuck, root dips).
Material slots: Echo_Body / Echo_Armor / Echo_Emissive / Echo_Eye.

Run:  python3 gen_echo_dawnfang.py
Output: ArtSource/Meshes/Echoes/SK_Echo_Dawnfang.glb
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
OUT_PATH = os.path.join(OUT_DIR, "SK_Echo_Dawnfang.glb")
SPECIES = "Dawnfang"
DEG = math.pi / 180.0

# ----------------------------------------------------------------- materials
BODY = (0.18, 0.30, 0.34, 1.0)       # deep sea-teal scaled hide
ARMOR = (0.10, 0.19, 0.23, 1.0)      # dark scale plate, wet sheen
DAWN = (0.95, 0.82, 0.45)            # Light dawn-GOLD emissive

MATS = {
    "Echo_Body":     Material("Echo_Body", base_color=BODY, roughness=0.5, metallic=0.05),
    "Echo_Armor":    Material("Echo_Armor", base_color=ARMOR, roughness=0.45, metallic=0.3),
    "Echo_Emissive": Material("Echo_Emissive", base_color=(0.06, 0.05, 0.02, 1.0),
                              roughness=0.4, metallic=0.0, emissive=DAWN),
    "Echo_Eye":      Material("Echo_Eye", base_color=(0.09, 0.08, 0.04, 1.0),
                              roughness=0.25, metallic=0.0,
                              emissive=(1.0, 0.95, 0.72)),
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


# --------------------------------------------------------------------- build
# Body chain: 11 waypoints (world), creature faces +Z, plan-view S-curve,
# head raised. Bones bind at WP[i] with segments toward WP[i+1].
WP = [
    (0.16, 0.50, -1.80),   # 0  tail tip
    (0.06, 0.56, -1.45),   # 1
    (-0.12, 0.62, -1.12),  # 2
    (-0.26, 0.70, -0.80),  # 3
    (-0.30, 0.78, -0.50),  # 4  hips
    (-0.26, 0.86, -0.18),  # 5
    (-0.10, 0.92, 0.12),   # 6
    (0.08, 0.95, 0.40),    # 7
    (0.20, 0.98, 0.66),    # 8  chest
    (0.22, 1.06, 0.90),    # 9  neck base (Head bone binds here)
    (0.18, 1.16, 1.12),    # 10 head center
]
RADIUS = [0.05, 0.08, 0.11, 0.14, 0.17, 0.20, 0.23, 0.26, 0.28, 0.22]

HEAD = np.array(WP[10])
NECK = np.array(WP[9])
SNOUT = HEAD + np.array((0.0, 0.02, 0.42))
LURE_ROOT = HEAD + np.array((0.0, 0.13, -0.02))   # forehead (between crest)
LURE_TIP = HEAD + np.array((0.0, 0.28, 0.50))    # bulb ahead of the snout
LURE_BIND = HEAD + np.array((0.0, 0.21, 0.28))   # bone covers the outer stalk


def build_rig(builder: GlbBuilder) -> Rig:
    rig = Rig(builder, root_name="Root", root_position=(0, 0, 0))
    names = ["Tail_01", "Tail_02", "Tail_03", "Hips", "Spine_01", "Spine_02",
             "Spine_03", "Chest", "Neck", "Head"]
    prev_world = np.array((0.0, 0.0, 0.0))
    for i, name in enumerate(names):
        pos = np.array(WP[i])
        nxt = np.array(WP[i + 1])
        d = nxt - pos
        L = float(np.linalg.norm(d))
        if name == "Head":
            # extend the head segment out past the snout (through the skull)
            d = SNOUT - pos
            L = float(np.linalg.norm(d))
        rig.add_bone(name, "Root" if i == 0 else names[i - 1], tuple(pos - prev_world),
                     direction=tuple(d), length=L)
        prev_world = pos
    # head assembly (children take locals relative to the Head bind at NECK)
    head_local = HEAD - NECK
    rig.add_bone("Jaw", "Head", tuple(head_local + (0.0, -0.07, 0.12)),
                 direction=(0, -0.45, 1), length=0.34)
    # light-lure bone: segment covers the outer stalk + bulb only, so the
    # inner stalk stays bound to the skull (flexes at mid when the lure bobs)
    rig.add_bone("Lure", "Head", tuple(LURE_BIND - NECK),
                 direction=(0, 0.4, 1), length=0.26)
    # paddle fins at the chest (children of Chest, which binds at WP[7])
    chest = np.array(WP[7])
    for tag, s in (("L", -1), ("R", 1)):
        anchor = chest + np.array((s * 0.30, -0.03, 0.06))
        rig.add_bone(f"Paddle_{tag}", "Chest", tuple(anchor - chest),
                     direction=(s * 1, -0.18, 0.35), length=0.34)
    # hind paddles at the hips (children of Hips, which binds at WP[3])
    hips = np.array(WP[3])
    for tag, s in (("L", -1), ("R", 1)):
        anchor = hips + np.array((s * 0.20, -0.04, -0.02))
        rig.add_bone(f"HindPaddle_{tag}", "Hips", tuple(anchor - hips),
                     direction=(s * 1, -0.3, 0.1), length=0.22)
    # dorsal fin control bones at the fin peak (children of Spine_02/Spine_03)
    for name, parent, wp, up in (("Dorsal_01", "Spine_02", 5, 0.24),
                                 ("Dorsal_02", "Spine_03", 6, 0.30)):
        base = np.array(WP[wp])
        rig.add_bone(name, parent, tuple(base + (0.0, up * 0.2, 0.0) - base),
                     direction=(0, 1, 0.2), length=up)
    # tail fluke (child of Tail_01, which binds at WP[0])
    tail_tip = np.array(WP[0])
    rig.add_bone("TailFin", "Tail_01", (0.0, 0.0, 0.0),
                 direction=(0, 0.6, -1), length=0.30)
    return rig


def build_body(mb: MeshBuilder) -> None:
    T = translate

    # --- barrel body: 9 segment masses (tail through neck)
    for i in range(9):
        p0 = np.array(WP[i])
        p1 = np.array(WP[i + 1])
        d = p1 - p0
        L = float(np.linalg.norm(d))
        dn = d / L
        mid = (p0 + p1) * 0.5
        r = RADIUS[i]
        mb.add(T(scale(sphere(r, 12, 7), (1.05, 0.92, 1.0)), tuple(mid)), "Echo_Body")
        # armor scale ring at each joint (Dragon scaled hide)
        ring = tube(r * 1.16, r * 0.9, 0.08, radial=7, wall_segs=1)
        mb.add(T(_align_y(ring, *dn), tuple(p0 + dn * (L * 0.5))), "Echo_Armor")
        # dawn-gold lateral line: strip + node diamond on both flanks
        perp = np.cross(dn, (0.0, 1.0, 0.0))
        if np.linalg.norm(perp) < 1e-6:
            perp = np.array((1.0, 0.0, 0.0))
        perp = perp / np.linalg.norm(perp)
        for s in (-1, 1):
            strip = box((0.02, 0.028, L * 0.7))
            mb.add(T(_align_y(strip, *dn), tuple(mid + perp * (s * r * 0.95))),
                   "Echo_Emissive")
            if i % 2 == 0:
                node = rotate(_align_y(box((0.05, 0.032, 0.05)), *dn), 0.0, 0.0, 0.78)
                mb.add(T(node, tuple(mid + perp * (s * r * 1.0))), "Echo_Emissive")

    # --- dorsal fin ribbon: overlapping plates from chest to tail, peaked mid
    fin_h = [0.30, 0.34, 0.32, 0.27, 0.22, 0.17, 0.12]
    for k, wp in enumerate(range(2, 9)):
        base = np.array(WP[wp])
        h = fin_h[k]
        lean = -0.55 + 0.05 * k
        plate = _align_y(box((0.02, h * 1.15, 0.16)), 0.0, 1.0, 0.55)
        plate = rotate(plate, lean, 0.0, 0.0)
        mb.add(T(plate, tuple(base + np.array((0.0, RADIUS[wp] * 0.55 + h * 0.4, 0.02)))),
               "Echo_Armor")
        if k < 6:
            mb.add(T(rotate(cone(0.012, h * 0.5, 4), lean, 0.0, 0.0),
                     tuple(base + np.array((0.0, RADIUS[wp] * 0.5 + h * 0.85, 0.0)))),
                   "Echo_Armor")

    # --- tail fluke (vertical crescent at the tail tip)
    tail_tip = np.array(WP[0])
    for s in (-1, 1):
        fluke = _align_y(box((0.022, 0.30, 0.17)), 0.0, s * 0.9, -1.0)
        mb.add(T(fluke, tuple(tail_tip + np.array((0.0, s * 0.14, -0.09)))), "Echo_Armor")
    mb.add(T(sphere(0.03, 6, 4), tuple(tail_tip + (0.0, 0.0, -0.02))), "Echo_Emissive")

    # --- chest paddle fins (large, webbed)
    chest = np.array(WP[7])
    for tag, s in (("L", -1), ("R", 1)):
        anchor = chest + np.array((s * 0.30, -0.03, 0.06))
        tip = anchor + np.array((s * 0.36, -0.11, 0.14))
        mb.add(_seg_capsule(anchor, tip, 0.035, 2, 8), "Echo_Body")
        blade = _align_y(box((0.10, 0.30, 0.02)), s * 1, -0.18, 0.35)
        mb.add(T(blade, tuple(anchor + np.array((s * 0.17, -0.045, 0.05)))), "Echo_Armor")
        for k in (0.3, 0.62):
            mb.add(T(_align_y(rotate(box((0.018, 0.22, 0.055)), 0.2 * s, 0, 0),
                              s * 1, -0.18, 0.35),
                     tuple(anchor + (tip - anchor) * k)), "Echo_Armor")
        mb.add(T(sphere(0.02, 6, 4), tuple(tip)), "Echo_Emissive")

    # --- hind paddle fins (small)
    hips = np.array(WP[3])
    for tag, s in (("L", -1), ("R", 1)):
        anchor = hips + np.array((s * 0.20, -0.04, -0.02))
        tip = anchor + np.array((s * 0.24, -0.09, 0.03))
        mb.add(_seg_capsule(anchor, tip, 0.025, 2, 8), "Echo_Body")
        blade = _align_y(box((0.07, 0.20, 0.018)), s * 1, -0.3, 0.1)
        mb.add(T(blade, tuple(anchor + np.array((s * 0.11, -0.03, 0.0)))), "Echo_Armor")

    # --- head: skull, snout, jaw, fangs, eyes, crest, gill plates
    mb.add(T(box((0.30, 0.22, 0.40)), tuple(HEAD + (0.0, 0.0, 0.06))), "Echo_Body")
    mb.add(T(box((0.19, 0.13, 0.30)), tuple(HEAD + (0.0, 0.01, 0.34))), "Echo_Body")
    mb.add(T(box((0.32, 0.035, 0.30)), tuple(HEAD + (0.0, 0.115, 0.16))), "Echo_Armor")
    # head crest fin (first plate of the dorsal ribbon)
    crest = _align_y(box((0.02, 0.24, 0.17)), 0.0, 0.9, -0.4)
    mb.add(T(crest, tuple(HEAD + (0.0, 0.18, -0.06))), "Echo_Armor")
    for s in (-1, 1):
        mb.add(T(sphere(0.035, 8, 6), tuple(HEAD + (s * 0.115, 0.055, 0.26))), "Echo_Eye")
        # gill plates at the chest sides
        for g in range(3):
            mb.add(T(rotate(box((0.02, 0.16, 0.05)), 0.0, 0.0, -0.2 * g),
                     tuple(HEAD + (s * 0.15, 0.0, -0.02 - 0.07 * g))), "Echo_Armor")
        # fangs (upper)
        mb.add(T(rotate(cone(0.016, 0.08, 4), math.pi, 0, 0),
                 tuple(HEAD + (s * 0.07, -0.07, 0.42))), "Echo_Armor")
    # jaw (binds to the Jaw bone segment)
    mb.add(T(rotate(box((0.20, 0.055, 0.34)), 0.18, 0, 0),
             tuple(HEAD + (0.0, -0.10, 0.28))), "Echo_Body")
    # light-lure: arced stalk from the forehead + emissive bulb ahead of the
    # snout (inner stalk binds to the skull, outer stalk + bulb to the Lure)
    lure_mid = (LURE_ROOT + LURE_TIP) * 0.5 + np.array((0.0, 0.03, 0.0))
    mb.add(_seg_capsule(LURE_ROOT, lure_mid, 0.014, 2, 6), "Echo_Body")
    mb.add(_seg_capsule(lure_mid, LURE_TIP, 0.010, 2, 6), "Echo_Body")
    mb.add(T(scale(sphere(0.05, 8, 6), (1.0, 1.15, 1.0)), tuple(LURE_TIP)), "Echo_Emissive")
    mb.add(T(sphere(0.02, 6, 4), tuple(LURE_TIP + (0.0, 0.07, 0.0))), "Echo_Emissive")

    # --- belly plates (armor underside)
    for (bx, by, bz) in ((-0.02, 0.72, 0.02), (0.02, 0.76, 0.24),
                         (0.04, 0.82, 0.46), (-0.06, 0.66, -0.22)):
        mb.add(T(rotate(box((0.18, 0.03, 0.22)), 0.12, 0, 0), (bx, by, bz)), "Echo_Armor")


# ----------------------------------------------------------------- animation
def anim_idle(builder: GlbBuilder, rig: Rig) -> None:
    """4.2s — water hover: spine ripple, paddle wave, lure bob, tail sway,
    root float."""
    anim = builder.add_animation("AM_Dawnfang_Idle")
    dur, n = 4.2, 12
    _tr_sine(rig, anim, "Root", dur, n, 0.05)
    for i, name in enumerate(["Tail_01", "Tail_02", "Tail_03", "Hips", "Spine_01",
                              "Spine_02", "Spine_03", "Chest", "Neck"]):
        amp = 4.5 + 0.4 * i if i < 3 else 1.8 + 0.15 * (i - 3)
        _rot_sine(rig, anim, name, dur, n, amp, 0.45 * i, "y")
        _rot_sine(rig, anim, name, dur, n, 0.8, 0.3 * i + 0.6, "x")
    _rot_sine(rig, anim, "Head", dur, n, 3.0, 0.8, "y")
    _rot_sine(rig, anim, "Head", dur, n, 1.5, 0.4, "x")
    _rot_sine(rig, anim, "Jaw", dur, n, 1.5, 0.0, "x")
    _rot_sine(rig, anim, "Lure", dur, n, 7.0, 0.5, "y")
    _rot_sine(rig, anim, "Lure", dur, n, 5.0, 0.0, "x")
    _rot_sine(rig, anim, "Paddle_L", dur, n, 8.0, 0.3, "x")
    _rot_sine(rig, anim, "Paddle_R", dur, n, 8.0, 0.3 + math.pi, "x")
    _rot_sine(rig, anim, "HindPaddle_L", dur, n, 6.0, 1.1, "x")
    _rot_sine(rig, anim, "HindPaddle_R", dur, n, 6.0, 1.1 + math.pi, "x")
    _rot_sine(rig, anim, "Dorsal_01", dur, n, 2.5, 0.9, "x")
    _rot_sine(rig, anim, "Dorsal_02", dur, n, 2.5, 1.3, "x")
    _rot_sine(rig, anim, "TailFin", dur, n, 8.0, 0.4, "y")


def anim_move(builder: GlbBuilder, rig: Rig) -> None:
    """1.4s — serpentine swim: spine Y +/-7deg traveling wave, tail
    +/-12..20deg, paddles rowing alternating, root bob."""
    anim = builder.add_animation("AM_Dawnfang_Move")
    dur, n = 1.4, 12
    _tr_sine(rig, anim, "Root", dur, n, 0.04, freq=2.0, phase=0.5)
    for i, name in enumerate(["Tail_01", "Tail_02", "Tail_03", "Hips", "Spine_01",
                              "Spine_02", "Spine_03", "Chest", "Neck"]):
        amp = 12.0 + 2.5 * i if i < 3 else 7.0
        _rot_sine(rig, anim, name, dur, n, amp, 0.6 * i, "y")
        _rot_sine(rig, anim, name, dur, n, 1.5, 0.3 * i, "x")
    _rot_sine(rig, anim, "Head", dur, n, 2.5, 0.9, "y")
    _rot_sine(rig, anim, "Head", dur, n, 2.0, 1.4, "x")
    _rot_sine(rig, anim, "Jaw", dur, n, 2.0, 0.6, "x")
    _rot_sine(rig, anim, "Lure", dur, n, 9.0, 2.0, "y")
    _rot_sine(rig, anim, "Paddle_L", dur, n, 20.0, 0.0, "x")
    _rot_sine(rig, anim, "Paddle_R", dur, n, 20.0, math.pi, "x")
    _rot_sine(rig, anim, "HindPaddle_L", dur, n, 12.0, 0.8, "x")
    _rot_sine(rig, anim, "HindPaddle_R", dur, n, 12.0, 0.8 + math.pi, "x")
    _rot_sine(rig, anim, "Dorsal_01", dur, n, 4.0, 0.7, "x")
    _rot_sine(rig, anim, "Dorsal_02", dur, n, 4.0, 1.1, "x")
    _rot_sine(rig, anim, "TailFin", dur, n, 14.0, 0.6, "y")


def anim_hit(builder: GlbBuilder, rig: Rig) -> None:
    """0.42s — contract: spine curls, jaw gapes, lure whips, paddles tuck,
    root dips."""
    anim = builder.add_animation("AM_Dawnfang_Hit")
    deg = DEG
    ts = [0.0, 0.08, 0.16, 0.28, 0.42]
    rig.add_rotation_channel(anim, "Head", list(zip(ts, [
        (0, 0, 0), (-10 * deg, 0, 0), (-7 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Neck", list(zip(ts, [
        (0, 0, 0), (-7 * deg, 0, 0), (-5 * deg, 0, 0), (1.5 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Jaw", list(zip(ts, [
        (0, 0, 0), (9 * deg, 0, 0), (6 * deg, 0, 0), (1.5 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Lure", list(zip(ts, [
        (0, 0, 0), (14 * deg, 0, 0), (9 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
    for k, amp in ((4, 4.0), (5, 3.5), (6, 3.0), (7, 2.5), (8, 2.0)):
        rig.add_rotation_channel(anim, ["Hips", "Spine_01", "Spine_02", "Spine_03",
                                        "Chest"][k - 4], list(zip(ts, [
            (0, 0, 0), (amp * deg, 0, 0), (amp * 0.7 * deg, 0, 0),
            (-amp * 0.25 * deg, 0, 0), (0, 0, 0)])))
    for tag in ("L", "R"):
        rig.add_rotation_channel(anim, f"Paddle_{tag}", list(zip(ts, [
            (0, 0, 0), (-16 * deg, 0, 0), (-11 * deg, 0, 0), (-3 * deg, 0, 0), (0, 0, 0)])))
        rig.add_rotation_channel(anim, f"HindPaddle_{tag}", list(zip(ts, [
            (0, 0, 0), (-12 * deg, 0, 0), (-8 * deg, 0, 0), (-2 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "TailFin", list(zip(ts, [
        (0, 0, 0), (12 * deg, 0, 0), (8 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
    rig.add_translation_channel(anim, "Root", [
        (0.0, (0, 0, 0)), (0.08, (0, -0.08, 0)), (0.16, (0, -0.055, 0)),
        (0.28, (0, -0.015, 0)), (0.42, (0, 0, 0))])


def main() -> None:
    builder = GlbBuilder()
    for m in MATS.values():
        builder.add_material(m)

    rig = build_rig(builder)
    mb = MeshBuilder()
    build_body(mb)
    print(f"[{SPECIES}] body parts: {len(mb.parts)}, tris: {mb.triangle_count()}")

    skin_idx, prims = rig.build_skin(mb, power=3.0)
    mesh_node = builder.add_node("SK_Echo_Dawnfang", parent=0,
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
    stats["ue_path"] = "/Game/Characters/Echoes/SK_Echo_Dawnfang"
    record("mesh", "SK_Echo_Dawnfang", stats)
    print(f"[{SPECIES}] {OUT_PATH}")
    print(f"[{SPECIES}] bones={stats['bones']} tris={stats['triangles']} "
          f"anims={len(stats['animations'])} bytes={stats['bytes']} validate={stats['validate']}")
    if problems:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
