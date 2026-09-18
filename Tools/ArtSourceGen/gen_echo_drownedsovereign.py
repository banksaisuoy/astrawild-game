"""
ASTRAWILD ArtSourceGen — SK_Echo_DrownedSovereign (Pulse Echo, final-boss serpent).

Original ASTRAWILD creature: The Drowned Sovereign — final boss of MQ-16
("Eye of the Maelstrom"), a drowned Ancient sovereign that rises from the
maelstrom across three phases. Canon: Pulse element, Ancient family, Serpent
body plan, Huge size class (runtime BodyScaleForSize 1.9 applies on top of
this mesh; authored at ~2.8x hero base scale per CREATURE_VISUAL_STRATEGY §7).

Design: a long relic-serpent — 15 chain segments (3 tail + 10 spine + neck +
head) rising in a lateral S-curve from a ground coil to a hooded head at
3.3m. Every vertebra carries a relic-metal hex ring; twin deep-drowned
Pulse-violet emissive channels run down both flanks; broken monolith shards
(relic architecture) are embedded along the upper spine as dorsal fins; a
hooded crown frill fans out behind the head with emissive spires; two whisker
cables trail from the jaws; two small flipper fins sit at the chest. Ancient
surface = weathered relic-metal on the armor slot, drowned scaled hide on the
body slot.

Rig: Root + Tail_01..03 + Spine_01..10 + Neck/Head/Jaw/Crown +
Whisker_L/R_01..02 + Fin_L/R. 24 bones.
Anims: AM_DrownedSovereign_Idle (4.2s breathing ripple: traveling spine sine
phase-stepped, tail sway, whisker drift, crown breathe, root bob),
AM_DrownedSovereign_Move (1.4s serpentine undulation: spine Y +/-6.5deg
traveling wave, tail +/-10..17deg, fins rowing, root lurch),
AM_DrownedSovereign_Hit (0.42s head recoil + jaw gape + crown flare + root dip).
Material slots: Echo_Body / Echo_Armor / Echo_Emissive / Echo_Eye.

Run:  python3 gen_echo_drownedsovereign.py
Output: ArtSource/Meshes/Echoes/SK_Echo_DrownedSovereign.glb
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
OUT_PATH = os.path.join(OUT_DIR, "SK_Echo_DrownedSovereign.glb")
SPECIES = "DrownedSovereign"
DEG = math.pi / 180.0

# ----------------------------------------------------------------- materials
BODY = (0.15, 0.21, 0.25, 1.0)       # drowned teal-slate hide
ARMOR = (0.29, 0.32, 0.28, 1.0)      # weathered relic bronze (verdigris)
PULSE = (0.55, 0.35, 0.95)           # deep-drowned Pulse VIOLET emissive

MATS = {
    "Echo_Body":     Material("Echo_Body", base_color=BODY, roughness=0.6, metallic=0.2),
    "Echo_Armor":    Material("Echo_Armor", base_color=ARMOR, roughness=0.5, metallic=0.55),
    "Echo_Emissive": Material("Echo_Emissive", base_color=(0.03, 0.02, 0.07, 1.0),
                              roughness=0.4, metallic=0.0, emissive=PULSE),
    "Echo_Eye":      Material("Echo_Eye", base_color=(0.05, 0.03, 0.09, 1.0),
                              roughness=0.25, metallic=0.0,
                              emissive=(0.85, 0.72, 1.0)),
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
# Serpent spine: 16 waypoints (world). Creature faces +Z; tail coils on the
# ground behind, body rises in a lateral S-curve to the head at ~3.3m.
WP = [
    # tail (ground sweep)
    (0.02, 0.16, -2.55), (0.10, 0.20, -2.18), (0.19, 0.26, -1.86), (0.26, 0.34, -1.58),
    # spine (rising S-curve)
    (0.32, 0.55, -1.38), (0.46, 0.95, -1.10), (0.52, 1.38, -0.78), (0.42, 1.78, -0.46),
    (0.18, 2.12, -0.16), (-0.12, 2.42, 0.10), (-0.34, 2.68, 0.34), (-0.36, 2.88, 0.56),
    (-0.22, 3.02, 0.72), (0.00, 3.10, 0.82),
    # neck / head base
    (0.10, 3.16, 0.92),
    # head
    (0.12, 3.30, 1.10),
]
RADIUS = [0.055, 0.09, 0.13, 0.17,          # tail
          0.21, 0.26, 0.29, 0.30, 0.31, 0.31, 0.30, 0.28, 0.26, 0.24,  # spine
          0.21]                              # neck->head

HEAD = np.array(WP[15])
NECK = np.array(WP[14])
CROWN_POS = HEAD + np.array((0.0, 0.06, -0.30))
WHISKER_ROOTS = {  # attach at the jaw back-corners, clear of the jaw mesh
    "L": HEAD + np.array((-0.22, -0.12, 0.10)),
    "R": HEAD + np.array((0.22, -0.12, 0.10)),
}
WHISKER_WAY = {  # cables sweep out-and-down past the body silhouette
    "L": [np.array((-0.34, -0.28, -0.34)), np.array((-0.20, -0.22, -0.40))],
    "R": [np.array((0.34, -0.28, -0.34)), np.array((0.20, -0.22, -0.40))],
}
FIN_ANCHOR = np.array(WP[7])                # chest (S3)


def build_rig(builder: GlbBuilder) -> Rig:
    rig = Rig(builder, root_name="Root", root_position=(0, 0, 0))
    # chain bones: bind at WP[i], segment toward WP[i+1]
    names = ["Tail_01", "Tail_02", "Tail_03"] + [f"Spine_{i:02d}" for i in range(1, 11)] \
        + ["Neck", "Head"]
    prev_world = np.array((0.0, 0.0, 0.0))
    # NOTE: the "Head" chain bone binds at NECK (WP[14]); its segment extends
    # forward THROUGH the skull so skull verts dominate on Head, and children
    # (Jaw/Crown/Whiskers) take locals relative to NECK.
    for i, name in enumerate(names):
        pos = np.array(WP[i])
        nxt = np.array(WP[i + 1])
        d = nxt - pos
        L = float(np.linalg.norm(d))
        if name == "Head":
            # extend the head segment out past the snout (through the skull)
            snout = HEAD + np.array((0.0, 0.0, 0.52))
            d = snout - pos
            L = float(np.linalg.norm(d))
        rig.add_bone(name, "Root" if i == 0 else names[i - 1], tuple(pos - prev_world),
                     direction=tuple(d), length=L)
        prev_world = pos
    # head assembly (note: the Head bone BINDS at NECK = WP[14], so child
    # bone locals are offsets from NECK, not from the skull center at HEAD)
    head_local = HEAD - NECK
    rig.add_bone("Jaw", "Head", tuple(head_local + (0.0, -0.10, 0.20)),
                 direction=(0, -0.4, 1), length=0.44)
    rig.add_bone("Crown", "Head", tuple(CROWN_POS - NECK), direction=(0, 0.75, -1), length=0.30)
    for tag, s in (("L", -1), ("R", 1)):
        root = WHISKER_ROOTS[tag]
        w1 = root + WHISKER_WAY[tag][0]
        w2 = w1 + WHISKER_WAY[tag][1]
        rig.add_bone(f"Whisker_{tag}_01", "Head", tuple(root - NECK),
                     direction=tuple(w1 - root), length=float(np.linalg.norm(w1 - root)))
        rig.add_bone(f"Whisker_{tag}_02", f"Whisker_{tag}_01", tuple(w1 - root),
                     direction=tuple(w2 - w1), length=float(np.linalg.norm(w2 - w1)))
    # chest flipper fins (children of Spine_05, which binds at WP[7])
    for tag, s in (("L", -1), ("R", 1)):
        anchor = FIN_ANCHOR + np.array((s * 0.27, -0.06, 0.03))
        rig.add_bone(f"Fin_{tag}", "Spine_05", tuple(anchor - FIN_ANCHOR),
                     direction=(s * 1, -0.22, 0.12), length=0.36)
    return rig


def build_body(mb: MeshBuilder) -> None:
    T = translate

    # --- 15 vertebra segments: body mass + relic hex ring + flank channels
    for i in range(15):
        p0 = np.array(WP[i])
        p1 = np.array(WP[i + 1])
        d = p1 - p0
        L = float(np.linalg.norm(d))
        dn = d / L
        mid = (p0 + p1) * 0.5
        r = RADIUS[i]
        # body mass (slightly squashed, a touch wider than deep)
        mb.add(T(scale(sphere(r, 12, 7), (1.06, 0.92, 1.0)), tuple(mid)), "Echo_Body")
        # relic-metal hex ring at the joint
        ring = tube(r * 1.13, r * 0.86, 0.11, radial=6, wall_segs=1)
        mb.add(T(_align_y(ring, *dn), tuple(p0 + dn * (L * 0.5))), "Echo_Armor")
        # twin Pulse-violet flank channels (deep-drowned emissive)
        perp = np.cross(dn, (0.0, 1.0, 0.0))
        if np.linalg.norm(perp) < 1e-6:
            perp = np.array((1.0, 0.0, 0.0))
        perp = perp / np.linalg.norm(perp)
        for s in (-1, 1):
            strip = box((0.024, 0.045, L * 0.6))
            mb.add(T(_align_y(strip, *dn), tuple(mid + perp * (s * r * 0.93))),
                   "Echo_Emissive")

    # --- broken monolith shards embedded along the upper spine (relic fins)
    for wi, h in ((5, 0.52), (7, 0.66), (9, 0.74), (11, 0.62), (13, 0.48)):
        p = np.array(WP[wi])
        r = RADIUS[wi]
        shard = rotate(crystal(h, 0.05 + 0.02 * (h / 0.7), 5), -0.38, 0.0, 0.0)
        mb.add(T(shard, tuple(p + np.array((0.0, r * 0.55, 0.0)))), "Echo_Armor")

    # --- head: skull, snout, brows, fangs, eyes
    mb.add(T(box((0.34, 0.27, 0.46)), tuple(HEAD + (0.0, 0.02, 0.10))), "Echo_Body")
    mb.add(T(box((0.23, 0.16, 0.34)), tuple(HEAD + (0.0, 0.0, 0.40))), "Echo_Body")
    mb.add(T(box((0.37, 0.05, 0.30)), tuple(HEAD + (0.0, 0.15, 0.16))), "Echo_Armor")
    for s in (-1, 1):
        mb.add(T(box((0.10, 0.05, 0.28)), tuple(HEAD + (s * 0.14, 0.115, 0.30))), "Echo_Armor")
        mb.add(T(sphere(0.042, 8, 6), tuple(HEAD + (s * 0.145, 0.06, 0.30))), "Echo_Eye")
        mb.add(T(rotate(cone(0.022, 0.09, 4), math.pi, 0, 0),
                 tuple(HEAD + (s * 0.085, -0.075, 0.42))), "Echo_Armor")
    # jaw (binds to Jaw bone near its segment)
    mb.add(T(rotate(box((0.25, 0.07, 0.42)), 0.22, 0, 0),
             tuple(HEAD + (0.0, -0.115, 0.26))), "Echo_Body")
    mb.add(T(box((0.27, 0.035, 0.34)), tuple(HEAD + (0.0, -0.135, 0.24))), "Echo_Armor")

    # --- hooded crown frill: fan of relic plates + emissive spires behind head
    fan = [(-0.62, 0.30, -0.34), (-0.34, 0.52, -0.42), (0.0, 0.62, -0.44),
           (0.34, 0.52, -0.42), (0.62, 0.30, -0.34)]
    for (fx, fy, fz) in fan:
        plate = box((0.14, 0.34, 0.026))
        m = _align_y(plate, fx, fy + 0.25, fz)
        pos = CROWN_POS + np.array((fx * 0.42, fy * 0.5, fz * 0.5))
        mb.add(T(m, tuple(pos)), "Echo_Armor")
        mb.add(T(_align_y(box((0.02, 0.30, 0.014)), fx, fy + 0.25, fz),
                 tuple(pos + np.array((fx * 0.08, 0.0, 0.012)))), "Echo_Emissive")
    for (sx, h) in ((-0.16, 0.46), (0.0, 0.62), (0.16, 0.46)):
        spire = rotate(crystal(h, 0.045, 5), -0.30, 0.0, 0.0)
        mb.add(T(spire, tuple(CROWN_POS + (sx, 0.34, -0.16))), "Echo_Armor")
        mb.add(T(rotate(crystal(h * 0.5, 0.02, 5), -0.30, 0.0, 0.0),
                 tuple(CROWN_POS + (sx, 0.34 + h * 0.78, -0.16 - h * 0.24))),
               "Echo_Emissive")

    # --- whisker cables trailing from the jaw corners (outside the silhouette)
    for tag, s in (("L", -1), ("R", 1)):
        root = WHISKER_ROOTS[tag]
        w1 = root + WHISKER_WAY[tag][0]
        w2 = w1 + WHISKER_WAY[tag][1]
        mb.add(_seg_capsule(root, w1, 0.012, 2, 7), "Echo_Body")
        mb.add(_seg_capsule(w1, w2, 0.009, 2, 7), "Echo_Body")
        mb.add(T(sphere(0.024, 6, 4), tuple(w2)), "Echo_Emissive")

    # --- chest flipper fins (relic paddle blades)
    for tag, s in (("L", -1), ("R", 1)):
        anchor = FIN_ANCHOR + np.array((s * 0.27, -0.06, 0.03))
        tip = anchor + np.array((s * 0.36, -0.115, 0.05))
        blade = _align_y(box((0.15, 0.40, 0.022)), s * 1, -0.22, 0.12)
        mb.add(T(blade, tuple(anchor + np.array((s * 0.16, -0.04, 0.0)))), "Echo_Armor")
        for k in (0.35, 0.68):
            spine_cone = _align_y(cone(0.016, 0.14, 4), s * 1, 0.1, 0.05)
            mb.add(T(spine_cone, tuple(anchor + (tip - anchor) * k + (0, 0.03, 0.03))),
                   "Echo_Armor")
        mb.add(T(sphere(0.02, 6, 4), tuple(tip)), "Echo_Emissive")


# ----------------------------------------------------------------- animation
def anim_idle(builder: GlbBuilder, rig: Rig) -> None:
    """4.2s — breathing ripple: traveling sine along the spine, tail sway,
    whisker drift, crown breathe, slow root rise/fall."""
    anim = builder.add_animation("AM_DrownedSovereign_Idle")
    dur, n = 4.2, 12
    _tr_sine(rig, anim, "Root", dur, n, 0.03)
    for i, name in enumerate(["Tail_01", "Tail_02", "Tail_03"]
                             + [f"Spine_{k:02d}" for k in range(1, 11)]):
        amp = 4.5 + 0.5 * i if i < 3 else 2.0 + 0.08 * (i - 3)
        _rot_sine(rig, anim, name, dur, n, amp, 0.52 * i, "y")
        _rot_sine(rig, anim, name, dur, n, 0.9, 0.35 * i + 0.8, "x")
    _rot_sine(rig, anim, "Neck", dur, n, 2.5, 0.2, "y")
    _rot_sine(rig, anim, "Neck", dur, n, 1.2, 0.9, "x")
    _rot_sine(rig, anim, "Head", dur, n, 3.5, 0.9, "y")
    _rot_sine(rig, anim, "Head", dur, n, 1.5, 0.4, "x")
    _rot_sine(rig, anim, "Jaw", dur, n, 1.8, 0.0, "x")
    _rot_sine(rig, anim, "Crown", dur, n, 2.5, 0.6, "x")
    _rot_sine(rig, anim, "Crown", dur, n, 2.0, 1.1, "y")
    for tag, ph in (("L", 1.2), ("R", 2.4)):
        _rot_sine(rig, anim, f"Whisker_{tag}_01", dur, n, 5.0, ph, "y")
        _rot_sine(rig, anim, f"Whisker_{tag}_02", dur, n, 7.0, ph + 1.0, "y")
        _rot_sine(rig, anim, f"Whisker_{tag}_02", dur, n, 4.0, ph + 0.5, "z")
    _rot_sine(rig, anim, "Fin_L", dur, n, 3.5, 0.3, "x")
    _rot_sine(rig, anim, "Fin_R", dur, n, 3.5, 0.3 + math.pi, "x")


def anim_move(builder: GlbBuilder, rig: Rig) -> None:
    """1.4s — serpentine undulation: spine Y +/-6.5deg traveling wave,
    tail +/-10..17deg, fins rowing, whiskers trailing, root lurch."""
    anim = builder.add_animation("AM_DrownedSovereign_Move")
    dur, n = 1.4, 12
    _tr_sine(rig, anim, "Root", dur, n, 0.05, freq=1.0)
    for i, name in enumerate(["Tail_01", "Tail_02", "Tail_03"]
                             + [f"Spine_{k:02d}" for k in range(1, 11)]):
        amp = 10.0 + 2.0 * i if i < 3 else 6.5
        _rot_sine(rig, anim, name, dur, n, amp, 0.55 * i, "y")
        _rot_sine(rig, anim, name, dur, n, 1.5, 0.30 * i, "x")
    _rot_sine(rig, anim, "Neck", dur, n, 2.0, 3.0, "y")
    _rot_sine(rig, anim, "Head", dur, n, 3.0, 1.1, "y")
    _rot_sine(rig, anim, "Head", dur, n, 2.0, 1.6, "x")
    _rot_sine(rig, anim, "Jaw", dur, n, 2.5, 0.7, "x")
    _rot_sine(rig, anim, "Crown", dur, n, 3.0, 1.0, "y")
    for tag, ph in (("L", 2.0), ("R", 3.1)):
        _rot_sine(rig, anim, f"Whisker_{tag}_01", dur, n, 9.0, ph, "y")
        _rot_sine(rig, anim, f"Whisker_{tag}_02", dur, n, 11.0, ph + 0.8, "y")
    _rot_sine(rig, anim, "Fin_L", dur, n, 12.0, 0.0, "x")
    _rot_sine(rig, anim, "Fin_R", dur, n, 12.0, math.pi, "x")


def anim_hit(builder: GlbBuilder, rig: Rig) -> None:
    """0.42s — head recoil, jaw gapes, crown flares back, spine counter-curls,
    root dips."""
    anim = builder.add_animation("AM_DrownedSovereign_Hit")
    deg = DEG
    ts = [0.0, 0.08, 0.16, 0.28, 0.42]
    rig.add_rotation_channel(anim, "Head", list(zip(ts, [
        (0, 0, 0), (-14 * deg, 0, 0), (-9 * deg, 0, 0), (3 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Neck", list(zip(ts, [
        (0, 0, 0), (-9 * deg, 0, 0), (-6 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Jaw", list(zip(ts, [
        (0, 0, 0), (12 * deg, 0, 0), (8 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Crown", list(zip(ts, [
        (0, 0, 0), (10 * deg, 0, 0), (7 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
    for k, amp in ((1, -4.0), (2, -3.5), (3, -3.0), (4, -2.5), (5, -2.0)):
        rig.add_rotation_channel(anim, f"Spine_{k:02d}", list(zip(ts, [
            (0, 0, 0), (amp * deg, 0, 0), (amp * 0.7 * deg, 0, 0),
            (-amp * 0.25 * deg, 0, 0), (0, 0, 0)])))
    for tag, s in (("L", -1), ("R", 1)):
        rig.add_rotation_channel(anim, f"Whisker_{tag}_01", list(zip(ts, [
            (0, 0, 0), (0, 0, s * 14 * deg), (0, 0, s * 10 * deg),
            (0, 0, s * 3 * deg), (0, 0, 0)])))
        rig.add_rotation_channel(anim, f"Fin_{tag}", list(zip(ts, [
            (0, 0, 0), (10 * deg, 0, 0), (7 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
    rig.add_translation_channel(anim, "Root", [
        (0.0, (0, 0, 0)), (0.08, (0, -0.09, 0)), (0.16, (0, -0.06, 0)),
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
    mesh_node = builder.add_node("SK_Echo_DrownedSovereign", parent=0,
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
    stats["ue_path"] = "/Game/Characters/Echoes/SK_Echo_DrownedSovereign"
    record("mesh", "SK_Echo_DrownedSovereign", stats)
    print(f"[{SPECIES}] {OUT_PATH}")
    print(f"[{SPECIES}] bones={stats['bones']} tris={stats['triangles']} "
          f"anims={len(stats['animations'])} bytes={stats['bytes']} validate={stats['validate']}")
    if problems:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
