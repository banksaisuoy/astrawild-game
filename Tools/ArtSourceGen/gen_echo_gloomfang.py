"""
ASTRAWILD ArtSourceGen — SK_Echo_Gloomfang (Ash Echo, night predator).

Original ASTRAWILD creature: the Gloomfang — MQ-05 hunt target (defeat 3),
the Underlight Warden (dungeon-1 boss of Hollow Underlight) and the night-raid
raider that tests the camp perimeter (NightRaid world event). Canon: Ash
element, Beast family (matte organic hide), Quadruped body plan, Medium size
class (runtime BodyScaleForSize 1.0 on top; authored at ~1.25x the hero Small
quadruped base per CREATURE_VISUAL_STRATEGY §7 — shoulder 0.52m, total ~2.1m).

Design: a low-slung ash predator — prowling quadruped with a spine that
slopes down from high hips (0.62) to low shoulders (0.52) and a head carried
low; ash-vent back plates: 8 shingled keratin plates along the spine with
dying-coal ORANGE emissive seams glowing in the gaps between them; blunt
heavy jaws (wide boxy muzzle + underslung jaw bone + stub fangs); a short
2-bone tail whip; ember eyes; shoulder/hip scutes and knuckle armor.

Rig: Root/Hips + Spine_01..03 + Neck/Head/Jaw + Tail_01/02 + 2x (FrontLeg +
FrontLower + FrontPaw) + 2x (BackLeg + BackLower + BackPaw). 22 bones.
Anims: AM_Gloomfang_Idle (3.5s predator breathing loom: deep slow spine
breathe + low head sweep + tail sway), AM_Gloomfang_Move (0.8s prowl trot,
diagonal pairs +/-16deg, steady predator gaze, counter tail), AM_Gloomfang_Hit
(0.4s flinch: spine arches so the vent plates flare, jaw gapes, root dips).
Material slots: Echo_Body / Echo_Armor / Echo_Emissive / Echo_Eye.

Run:  python3 gen_echo_gloomfang.py
Output: ArtSource/Meshes/Echoes/SK_Echo_Gloomfang.glb
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
OUT_PATH = os.path.join(OUT_DIR, "SK_Echo_Gloomfang.glb")
SPECIES = "Gloomfang"
DEG = math.pi / 180.0

# ----------------------------------------------------------------- materials
BODY = (0.35, 0.32, 0.38, 1.0)       # ash-grey matte hide
ARMOR = (0.20, 0.18, 0.24, 1.0)      # dark ash keratin plate
COAL = (0.85, 0.33, 0.10)            # Ash dying-coal ORANGE emissive

MATS = {
    "Echo_Body":     Material("Echo_Body", base_color=BODY, roughness=0.85, metallic=0.0),
    "Echo_Armor":    Material("Echo_Armor", base_color=ARMOR, roughness=0.5, metallic=0.15),
    "Echo_Emissive": Material("Echo_Emissive", base_color=(0.05, 0.02, 0.01, 1.0),
                              roughness=0.4, metallic=0.0, emissive=COAL),
    "Echo_Eye":      Material("Echo_Eye", base_color=(0.07, 0.03, 0.01, 1.0),
                              roughness=0.25, metallic=0.0,
                              emissive=(1.0, 0.45, 0.18)),
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
# World anchors (creature faces +Z, low-slung sloped spine, hips high).
HIPS = np.array((0.0, 0.62, -0.34))
SPINE1 = np.array((0.0, 0.63, -0.18))
SPINE2 = np.array((0.0, 0.59, -0.02))
SPINE3 = np.array((0.0, 0.555, 0.15))       # shoulders
NECK = np.array((0.0, 0.53, 0.30))
HEAD_BIND = np.array((0.0, 0.505, 0.44))
HEAD_CENTER = np.array((0.0, 0.475, 0.56))
SNOUT = HEAD_CENTER + np.array((0.0, -0.03, 0.24))
JAW_BASE = np.array((0.0, 0.425, 0.56))
JAW_END = np.array((0.0, 0.30, 0.71))
TAIL1 = np.array((0.0, 0.60, -0.44))
TAIL2 = np.array((0.0, 0.645, -0.64))
TAIL_TIP = np.array((0.0, 0.70, -0.80))

# Torso capsule chain waypoints (hips -> chest), radii taper toward the chest.
TORSO_WP = [HIPS, SPINE1, SPINE2, SPINE3,
            np.array((0.0, 0.52, 0.30)), np.array((0.0, 0.50, 0.40))]
TORSO_R = [0.165, 0.175, 0.168, 0.155, 0.135, 0.105]


def build_rig(builder: GlbBuilder) -> Rig:
    rig = Rig(builder, root_name="Root", root_position=(0, 0, 0))
    rig.add_bone("Hips", "Root", tuple(HIPS), direction=(0, -1, -0.12), length=0.15)
    rig.add_bone("Spine_01", "Hips", tuple(SPINE1 - HIPS),
                 direction=(0, -0.22, 1), length=0.17)
    rig.add_bone("Spine_02", "Spine_01", tuple(SPINE2 - SPINE1),
                 direction=(0, -0.22, 1), length=0.17)
    rig.add_bone("Spine_03", "Spine_02", tuple(SPINE3 - SPINE2),
                 direction=(0, -0.22, 1), length=0.17)
    rig.add_bone("Neck", "Spine_03", tuple(NECK - SPINE3),
                 direction=(0, -0.15, 1), length=0.14)
    # Head binds at the neck end; its segment extends out THROUGH the skull
    # (V25-C1 convention) so skull/muzzle/eyes bind to Head, jaw to Jaw.
    rig.add_bone("Head", "Neck", tuple(HEAD_BIND - NECK),
                 direction=tuple(SNOUT - HEAD_BIND),
                 length=float(np.linalg.norm(SNOUT - HEAD_BIND)))
    rig.add_bone("Jaw", "Head", tuple(JAW_BASE - HEAD_BIND),
                 direction=tuple(JAW_END - JAW_BASE),
                 length=float(np.linalg.norm(JAW_END - JAW_BASE)))
    # short tail whip (2 bones)
    rig.add_bone("Tail_01", "Hips", tuple(TAIL1 - HIPS),
                 direction=tuple(TAIL2 - TAIL1),
                 length=float(np.linalg.norm(TAIL2 - TAIL1)))
    rig.add_bone("Tail_02", "Tail_01", tuple(TAIL2 - TAIL1),
                 direction=tuple(TAIL_TIP - TAIL2),
                 length=float(np.linalg.norm(TAIL_TIP - TAIL2)))
    # front legs off the shoulders (Spine_03)
    for side, tag in ((-1, "L"), (1, "R")):
        s = side
        top = np.array((s * 0.14, 0.50, 0.27))
        elbow = np.array((s * 0.15, 0.26, 0.29))
        wrist = np.array((s * 0.15, 0.055, 0.30))
        rig.add_bone(f"FrontLeg_{tag}", "Spine_03", tuple(top - SPINE3),
                     direction=tuple(elbow - top),
                     length=float(np.linalg.norm(elbow - top)))
        rig.add_bone(f"FrontLower_{tag}", f"FrontLeg_{tag}",
                     tuple(elbow - top),
                     direction=tuple(wrist - elbow),
                     length=float(np.linalg.norm(wrist - elbow)))
        rig.add_bone(f"FrontPaw_{tag}", f"FrontLower_{tag}",
                     tuple(wrist - elbow),
                     direction=(0, -0.45, 1), length=0.09)
    # back legs off the hips
    for side, tag in ((-1, "L"), (1, "R")):
        s = side
        top = np.array((s * 0.15, 0.57, -0.36))
        knee = np.array((s * 0.16, 0.31, -0.40))
        ankle = np.array((s * 0.16, 0.075, -0.37))
        rig.add_bone(f"BackLeg_{tag}", "Hips", tuple(top - HIPS),
                     direction=tuple(knee - top),
                     length=float(np.linalg.norm(knee - top)))
        rig.add_bone(f"BackLower_{tag}", f"BackLeg_{tag}", tuple(knee - top),
                     direction=tuple(ankle - knee),
                     length=float(np.linalg.norm(ankle - knee)))
        rig.add_bone(f"BackPaw_{tag}", f"BackLower_{tag}", tuple(ankle - knee),
                     direction=(0, -0.45, 1), length=0.09)
    return rig


def build_body(mb: MeshBuilder) -> None:
    T = translate

    # --- torso: capsule chain along the sloped spine (hips high, chest low)
    for i in range(len(TORSO_WP) - 1):
        mb.add(_seg_capsule(TORSO_WP[i], TORSO_WP[i + 1], TORSO_R[i], 2, 11),
               "Echo_Body")
    # shoulder + croup muscle masses (heavy predator silhouette)
    mb.add(T(sphere(0.075, 9, 6), (0.0, 0.53, 0.24)), "Echo_Body")
    mb.add(T(sphere(0.085, 9, 6), (0.0, 0.63, -0.30)), "Echo_Body")
    # belly hide plate
    mb.add(T(rotate(box((0.19, 0.03, 0.52)), 0.10, 0, 0), (0, 0.445, -0.02)),
           "Echo_Armor")

    # --- ash-vent back plates: 8 shingled plates along the sloped spine with
    #     dying-coal emissive seams glowing in the gaps between them
    plate_rows = [
        ((0.0, 0.585, 0.20), 0.22, 0.16),   # withers pair region (front)
        ((0.0, 0.615, 0.06), 0.24, 0.17),
        ((0.0, 0.645, -0.08), 0.24, 0.17),
        ((0.0, 0.665, -0.22), 0.24, 0.16),
        ((0.0, 0.672, -0.35), 0.21, 0.14),  # hips
        ((0.0, 0.665, -0.46), 0.16, 0.11),  # tail root
    ]
    for (pos, w, ln) in plate_rows:
        mb.add(T(rotate(box((w, 0.05, ln)), -0.24, 0, 0), pos), "Echo_Armor")
    # neck plate (first shingle)
    mb.add(T(rotate(box((0.17, 0.04, 0.13)), -0.5, 0, 0), (0.0, 0.615, 0.32)),
           "Echo_Armor")
    # emissive seams between plates (recessed strips in the gaps)
    seam_rows = [
        ((0.0, 0.60, 0.13), 0.185),
        ((0.0, 0.628, -0.01), 0.20),
        ((0.0, 0.655, -0.15), 0.20),
        ((0.0, 0.667, -0.285), 0.185),
        ((0.0, 0.665, -0.405), 0.15),
    ]
    for (pos, w) in seam_rows:
        mb.add(T(rotate(box((w, 0.016, 0.035)), -0.24, 0, 0), pos),
               "Echo_Emissive")
    # hip crest vents (2 small emissive slits at the hips)
    for s in (-1, 1):
        mb.add(T(rotate(box((0.05, 0.014, 0.06)), 0, 0, 0.4 * s),
                 (s * 0.09, 0.70, -0.33)), "Echo_Emissive")
    # flank shingles (second smaller plate row on both sides)
    for s in (-1, 1):
        for k, z in enumerate((0.10, -0.04, -0.18)):
            mb.add(T(rotate(box((0.05, 0.035, 0.13)), -0.24, 0, 0.15 * s),
                     (s * 0.155, 0.575 - 0.01 * k, z)), "Echo_Armor")
        # flank rib vent slits (dying-coal glow low on the sides)
        for z in (0.02, -0.12):
            mb.add(T(rotate(box((0.03, 0.012, 0.05)), -0.24, 0, 0.15 * s),
                     (s * 0.165, 0.545, z)), "Echo_Emissive")

    # --- head: blunt heavy jaws — wide skull + boxy muzzle + underslung jaw
    mb.add(T(box((0.25, 0.17, 0.24)), tuple(HEAD_CENTER + (0.0, 0.02, 0.0))),
           "Echo_Body")
    mb.add(T(box((0.19, 0.13, 0.22)), tuple(HEAD_CENTER + (0.0, 0.005, 0.165))),
           "Echo_Body")
    mb.add(T(box((0.27, 0.045, 0.20)), tuple(HEAD_CENTER + (0.0, 0.10, 0.06))),
           "Echo_Armor")   # brow plate
    # jaw (binds to the Jaw bone segment; kept low + back so the skull and
    # muzzle keep their Head binding — steep segment hugs the jaw underside)
    mb.add(T(rotate(box((0.16, 0.075, 0.20)), 0.22, 0, 0),
             tuple(HEAD_CENTER + (0.0, -0.10, 0.10))), "Echo_Body")
    # stub fangs: 4 upper + 2 lower
    for s in (-1, 1):
        for k in (0.0, 0.09):
            mb.add(T(rotate(cone(0.018, 0.06, 4), math.pi, 0, 0),
                     tuple(HEAD_CENTER + (s * 0.075, -0.075, 0.20 + k))), "Echo_Armor")
        mb.add(T(rotate(cone(0.014, 0.045, 4), 0, 0, 0),
                 tuple(HEAD_CENTER + (s * 0.055, -0.115, 0.21))), "Echo_Armor")
    # ember eyes
    for s in (-1, 1):
        mb.add(T(sphere(0.02, 7, 5), tuple(HEAD_CENTER + (s * 0.098, 0.045, 0.13))),
               "Echo_Eye")
    # cheek vent plates
    for s in (-1, 1):
        for g in range(2):
            mb.add(T(rotate(box((0.02, 0.09, 0.05)), 0, 0, 0.25 * s),
                     tuple(HEAD_CENTER + (s * 0.13, -0.01, -0.02 - 0.08 * g))),
                   "Echo_Armor")
    # small ear fins + throat plates (under the skull, hugging the jaw line
    # so they bind Jaw/Head — kept clear of the front-leg segments)
    for s in (-1, 1):
        mb.add(T(rotate(cone(0.028, 0.06, 5), 0.1, 0, 0.3 * s),
                 tuple(HEAD_CENTER + (s * 0.10, 0.105, -0.03))), "Echo_Body")
    for k in range(3):
        mb.add(T(rotate(box((0.12, 0.025, 0.05)), 0.45, 0, 0),
                 (0.0, 0.415 - 0.042 * k, 0.47 + 0.048 * k)), "Echo_Armor")

    # --- legs: predator limbs + knuckle armor + claws
    for s in (-1, 1):
        # front (shoulder -> elbow -> wrist -> paw)
        top = np.array((s * 0.14, 0.50, 0.27))
        elbow = np.array((s * 0.15, 0.26, 0.29))
        wrist = np.array((s * 0.15, 0.055, 0.30))
        mb.add(_seg_capsule(top, elbow, 0.052, 2, 8), "Echo_Body")
        mb.add(_seg_capsule(elbow, wrist, 0.038, 2, 7), "Echo_Body")
        mb.add(T(box((0.085, 0.055, 0.14)), (s * 0.15, 0.045, 0.34)), "Echo_Armor")
        mb.add(T(box((0.11, 0.12, 0.16)), (s * 0.145, 0.44, 0.26)), "Echo_Armor")
        mb.add(T(sphere(0.042, 7, 5), tuple(elbow)), "Echo_Armor")
        for c in (-0.03, 0.03):
            mb.add(T(rotate(cone(0.012, 0.03, 4), 0.5, 0, 0),
                     (s * 0.15 + c, 0.02, 0.40)), "Echo_Armor")
        # back (hip -> knee -> ankle -> paw)
        btop = np.array((s * 0.15, 0.57, -0.36))
        bknee = np.array((s * 0.16, 0.31, -0.40))
        bankle = np.array((s * 0.16, 0.075, -0.37))
        mb.add(_seg_capsule(btop, bknee, 0.062, 2, 9), "Echo_Body")
        mb.add(_seg_capsule(bknee, bankle, 0.042, 2, 7), "Echo_Body")
        mb.add(T(box((0.095, 0.055, 0.14)), (s * 0.16, 0.05, -0.32)), "Echo_Armor")
        mb.add(T(box((0.13, 0.14, 0.17)), (s * 0.16, 0.52, -0.36)), "Echo_Armor")
        mb.add(T(sphere(0.046, 7, 5), tuple(bknee)), "Echo_Armor")
        # thigh armor band (Ash hide scute ring)
        mb.add(T(_align_y(tube(0.068, 0.052, 0.03, 8, 1),
                          *(np.asarray(bknee) - np.asarray(btop))),
                 tuple(np.asarray(btop) + (np.asarray(bknee) - np.asarray(btop)) * 0.5)),
               "Echo_Armor")
        for c in (-0.03, 0.03):
            mb.add(T(rotate(cone(0.013, 0.032, 4), 0.5, 0, 0),
                     (s * 0.16 + c, 0.02, -0.28)), "Echo_Armor")

    # --- tail whip: 2 tapering segments + dorsal spikes + blade tip
    mb.add(_seg_capsule(TAIL1, TAIL2, 0.052, 2, 8), "Echo_Body")
    mb.add(_seg_capsule(TAIL2, TAIL_TIP, 0.028, 2, 6), "Echo_Body")
    for k, t in enumerate((0.25, 0.55, 0.8)):
        p = TAIL1 + (TAIL_TIP - TAIL1) * t
        mb.add(T(rotate(cone(0.014, 0.045, 4), -0.5, 0, 0),
                 (0.0, p[1] + 0.02, p[2])), "Echo_Armor")
    mb.add(T(_align_y(box((0.02, 0.13, 0.06)), 0.0, 0.55, -0.85),
             tuple(TAIL_TIP + (0.0, 0.01, -0.02))), "Echo_Armor")


# ----------------------------------------------------------------- animation
def anim_idle(builder: GlbBuilder, rig: Rig) -> None:
    """3.5s — predator breathing loom: deep slow spine breathe, low head
    sweep, tail sway, jaw breathing."""
    anim = builder.add_animation("AM_Gloomfang_Idle")
    dur, n = 3.5, 10
    _rot_sine(rig, anim, "Spine_01", dur, n, 2.2, 0.0)
    _rot_sine(rig, anim, "Spine_02", dur, n, 1.8, 0.5)
    _rot_sine(rig, anim, "Spine_03", dur, n, 1.4, 1.0)
    _rot_sine(rig, anim, "Neck", dur, n, 2.5, 0.8)
    _rot_sine(rig, anim, "Neck", dur, n, 4.0, 1.3, "y")
    _rot_sine(rig, anim, "Head", dur, n, 6.0, 1.0, "y")
    _rot_sine(rig, anim, "Head", dur, n, 1.5, 1.6)
    _rot_sine(rig, anim, "Jaw", dur, n, 2.5, 0.4)
    _rot_sine(rig, anim, "Tail_01", dur, n, 6.0, 0.2, "y")
    _rot_sine(rig, anim, "Tail_02", dur, n, 9.0, 0.9, "y")
    _rot_sine(rig, anim, "Tail_02", dur, n, 3.0, 1.4)
    _tr_sine(rig, anim, "Hips", dur, n, 0.008, freq=1.0)


def anim_move(builder: GlbBuilder, rig: Rig) -> None:
    """0.8s — prowl trot: diagonal pairs +/-16deg, heavy low amplitude, spine
    counter-yaw, steady gaze, counter tail."""
    anim = builder.add_animation("AM_Gloomfang_Move")
    dur, n = 0.8, 8

    def swing(amp: float, phase: float) -> list:
        return [math.radians(amp) * math.sin(2.0 * math.pi * (i / n) + phase)
                for i in range(n + 1)]

    def flex(amp: float, phase: float) -> list:
        return [math.radians(amp) * (0.35 + 0.65 * max(0.0, math.sin(
            2.0 * math.pi * (i / n) + 2.3 + phase))) for i in range(n + 1)]

    ts = [dur * i / n for i in range(n + 1)]
    legs = [
        ("FrontLeg_L", "FrontLower_L", "FrontPaw_L", 16.0, 0.0),
        ("FrontLeg_R", "FrontLower_R", "FrontPaw_R", 16.0, math.pi),
        ("BackLeg_L", "BackLower_L", "BackPaw_L", 16.0, math.pi),
        ("BackLeg_R", "BackLower_R", "BackPaw_R", 16.0, 0.0),
    ]
    for bone, low, paw, amp, ph in legs:
        swing_vals = swing(amp, ph)
        rig.add_rotation_channel(anim, bone, list(zip(ts, [(v, 0, 0) for v in swing_vals])))
        flex_vals = flex(amp * 0.7, ph)
        rig.add_rotation_channel(anim, low, list(zip(ts, [(v, 0, 0) for v in flex_vals])))
        rig.add_rotation_channel(anim, paw, list(zip(ts, [(-v * 0.4, 0, 0) for v in swing_vals])))
    # prowl spine counter-yaw + hips bob, steady low head
    _rot_sine(rig, anim, "Spine_01", dur, n, 2.5, 0.4, "y")
    _rot_sine(rig, anim, "Spine_02", dur, n, 4.0, 0.9, "y")
    _rot_sine(rig, anim, "Spine_01", dur, n, 1.5, 0.2)
    _tr_sine(rig, anim, "Hips", dur, n, 0.016, freq=2.0, phase=0.8)
    _rot_sine(rig, anim, "Neck", dur, n, 2.0, 1.5, "y")
    _rot_sine(rig, anim, "Head", dur, n, 1.2, 1.0, "y")
    _rot_sine(rig, anim, "Head", dur, n, 1.5, 0.6)
    _rot_sine(rig, anim, "Tail_01", dur, n, 8.0, 0.3, "y")
    _rot_sine(rig, anim, "Tail_02", dur, n, 13.0, 0.9, "y")


def anim_hit(builder: GlbBuilder, rig: Rig) -> None:
    """0.4s — flinch + plate flare: spine arches (vent seams gap open), jaw
    gapes, head snaps up, root dips, tail lashes."""
    anim = builder.add_animation("AM_Gloomfang_Hit")
    deg = DEG
    ts = [0.0, 0.08, 0.16, 0.28, 0.40]
    # arch the spine UP so the shingled plates separate and seams flare
    rig.add_rotation_channel(anim, "Spine_01", list(zip(ts, [
        (0, 0, 0), (-9 * deg, 0, 0), (-6 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Spine_02", list(zip(ts, [
        (0, 0, 0), (-8 * deg, 0, 0), (-5.5 * deg, 0, 0), (1.5 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Spine_03", list(zip(ts, [
        (0, 0, 0), (-6 * deg, 0, 0), (-4 * deg, 0, 0), (1 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Neck", list(zip(ts, [
        (0, 0, 0), (-7 * deg, 0, 0), (-5 * deg, 0, 0), (1.5 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Head", list(zip(ts, [
        (0, 0, 0), (-12 * deg, 0, 0), (-8 * deg, 0, 0), (3 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Jaw", list(zip(ts, [
        (0, 0, 0), (10 * deg, 0, 0), (7 * deg, 0, 0), (1.5 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Tail_01", list(zip(ts, [
        (0, 0, 0), (10 * deg, 0, 0), (7 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Tail_02", list(zip(ts, [
        (0, 0, 0), (16 * deg, 0, 0), (11 * deg, 0, 0), (3 * deg, 0, 0), (0, 0, 0)])))
    # front legs brace backward, back legs plant
    for tag in ("L", "R"):
        rig.add_rotation_channel(anim, f"FrontLeg_{tag}", list(zip(ts, [
            (0, 0, 0), (8 * deg, 0, 0), (6 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
        rig.add_rotation_channel(anim, f"BackLeg_{tag}", list(zip(ts, [
            (0, 0, 0), (-6 * deg, 0, 0), (-4 * deg, 0, 0), (-1 * deg, 0, 0), (0, 0, 0)])))
    rig.add_translation_channel(anim, "Root", [
        (0.0, (0, 0, 0)), (0.08, (0, -0.05, 0)), (0.16, (0, -0.035, 0)),
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
    mesh_node = builder.add_node("SK_Echo_Gloomfang", parent=0,
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
    stats["ue_path"] = "/Game/Characters/Echoes/SK_Echo_Gloomfang"
    record("mesh", "SK_Echo_Gloomfang", stats)
    print(f"[{SPECIES}] {OUT_PATH}")
    print(f"[{SPECIES}] bones={stats['bones']} tris={stats['triangles']} "
          f"anims={len(stats['animations'])} bytes={stats['bytes']} validate={stats['validate']}")
    if problems:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
