"""
ASTRAWILD ArtSourceGen — SK_Echo_Sprigling (Flora Echo, herd flora-foal).

Original ASTRAWILD creature: the Sprigling — MQ-06 capture-lesson species
(Echo husbandry) and the Great Migration world-event herd of the Verdant
Reach / Dawn Fields. Canon: Flora element, Flora family (subsurface leaf
surface), Biped body plan, Small size class (runtime BodyScaleForSize 0.7 on
top; authored at the hero Small biped scale per CREATURE_VISUAL_STRATEGY §7,
stubbiy-foal proportions at ~1.0m tall).

Design: a flora foal — biped sprout body (bulb torso lathe with chlorophyll-
green emissive veins spiraling the front), leaf-blade ears (two big leaves),
a closed bud crest on the head with sepals and a glowing bud tip, small twig
arms with leaf hands, vine-wrapped foal legs (thigh + shin/hoof with emissive
vine bands), bark armor collar and knee caps, tiny muzzle and bright eyes.

Rig: Root/Hips + Spine_01/02 + Neck/Head + Ear_L/R + Bud + Arm_L/R +
Leg_L/R + Foot_L/R. 15 bones.
Anims: AM_Sprigling_Idle (3.2s sprout sway + ear flick + bud bob + breathe),
AM_Sprigling_Move (0.7s hopping biped cycle: legs spring, hips bounce, ears
+ bud flap), AM_Sprigling_Hit (0.4s flinch back, ears flatten, bud dips).
Material slots: Echo_Body / Echo_Armor / Echo_Emissive / Echo_Eye.

Run:  python3 gen_echo_sprigling.py
Output: ArtSource/Meshes/Echoes/SK_Echo_Sprigling.glb
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
OUT_PATH = os.path.join(OUT_DIR, "SK_Echo_Sprigling.glb")
SPECIES = "Sprigling"
DEG = math.pi / 180.0

# ----------------------------------------------------------------- materials
BODY = (0.52, 0.68, 0.42, 1.0)       # moss-sprout subsurface leaf
ARMOR = (0.30, 0.38, 0.22, 1.0)      # bark plate / vine wrap
CHLORO = (0.32, 0.95, 0.45)          # Flora chlorophyll-GREEN emissive

MATS = {
    "Echo_Body":     Material("Echo_Body", base_color=BODY, roughness=0.65, metallic=0.0),
    "Echo_Armor":    Material("Echo_Armor", base_color=ARMOR, roughness=0.55, metallic=0.1),
    "Echo_Emissive": Material("Echo_Emissive", base_color=(0.02, 0.06, 0.02, 1.0),
                              roughness=0.4, metallic=0.0, emissive=CHLORO),
    "Echo_Eye":      Material("Echo_Eye", base_color=(0.03, 0.06, 0.03, 1.0),
                              roughness=0.25, metallic=0.0,
                              emissive=(0.55, 1.0, 0.60)),
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


def _blade_between(p0, p1, width: float, thickness: float, curve: float, segs: int):
    """Leaf blade spanning p0 -> p1 (base at p0, tip at p1)."""
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
# World anchors (creature faces +Z, stands on the ground, ~1.0m tall).
HIPS = np.array((0.0, 0.52, 0.0))
SPINE1 = np.array((0.0, 0.60, 0.012))
SPINE2 = np.array((0.0, 0.72, 0.024))
NECK = np.array((0.0, 0.84, 0.034))
HEAD = np.array((0.0, 0.90, 0.06))

EAR_L_BASE = np.array((-0.085, 1.005, 0.02))
EAR_L_END = EAR_L_BASE + np.array((-0.11, 0.155, -0.07))
BUD_BASE = np.array((0.0, 1.015, 0.05))
BUD_END = BUD_BASE + np.array((0.0, 0.095, -0.008))
ARM_L_BASE = np.array((-0.145, 0.775, 0.035))
ARM_L_END = ARM_L_BASE + np.array((-0.045, -0.085, 0.022))
LEG_L_TOP = np.array((-0.075, 0.52, -0.02))
LEG_L_KNEE = np.array((-0.075, 0.33, -0.006))
LEG_L_GROUND = np.array((-0.075, 0.045, 0.008))


def build_rig(builder: GlbBuilder) -> Rig:
    rig = Rig(builder, root_name="Root", root_position=(0, 0, 0))
    rig.add_bone("Hips", "Root", tuple(HIPS), direction=(0, -1, 0), length=0.09)
    rig.add_bone("Spine_01", "Hips", tuple(SPINE1 - HIPS),
                 direction=(0, 1, 0.1), length=0.12)
    rig.add_bone("Spine_02", "Spine_01", tuple(SPINE2 - SPINE1),
                 direction=(0, 1, 0.08), length=0.12)
    rig.add_bone("Neck", "Spine_02", tuple(NECK - SPINE2),
                 direction=(0, 0.85, 0.4), length=0.07)
    rig.add_bone("Head", "Neck", tuple(HEAD - NECK),
                 direction=(0, 0.35, 1), length=0.09)
    # leaf ears (single bones — they flick from the base)
    for base, end, tag in ((EAR_L_BASE, EAR_L_END, "L"),
                           ((0.085, 1.005, 0.02), (0.195, 1.16, -0.05), "R")):
        d = np.asarray(end) - np.asarray(base)
        rig.add_bone(f"Ear_{tag}", "Head", tuple(np.asarray(base) - HEAD),
                     direction=tuple(d), length=float(np.linalg.norm(d)))
    # bud crest
    rig.add_bone("Bud", "Head", tuple(BUD_BASE - HEAD),
                 direction=tuple(BUD_END - BUD_BASE),
                 length=float(np.linalg.norm(BUD_END - BUD_BASE)))
    # twig arms (Voltpylon shoulder convention: anchored OUT at the narrow
    # collar, segments point away from the bulb axis so the torso keeps its
    # spine binding)
    rig.add_bone("Arm_L", "Spine_02", tuple(ARM_L_BASE - SPINE2),
                 direction=tuple(ARM_L_END - ARM_L_BASE),
                 length=float(np.linalg.norm(ARM_L_END - ARM_L_BASE)))
    rig.add_bone("Arm_R", "Spine_02", (0.145, 0.055, 0.035),
                 direction=(0.45, -0.88, 0.22), length=0.097)
    # foal legs: thigh (hip->knee) + foot chain (knee->ground, hoof pivot)
    rig.add_bone("Leg_L", "Hips", tuple(LEG_L_TOP - HIPS),
                 direction=tuple(LEG_L_KNEE - LEG_L_TOP),
                 length=float(np.linalg.norm(LEG_L_KNEE - LEG_L_TOP)))
    rig.add_bone("Foot_L", "Leg_L", tuple(LEG_L_KNEE - LEG_L_TOP),
                 direction=tuple(LEG_L_GROUND - LEG_L_KNEE),
                 length=float(np.linalg.norm(LEG_L_GROUND - LEG_L_KNEE)))
    rig.add_bone("Leg_R", "Hips", (0.075, 0.0, -0.02),
                 direction=(0, -1, 0.075), length=0.19)
    rig.add_bone("Foot_R", "Leg_R", (0, -0.19, 0.014),
                 direction=(0, -1, 0.075), length=0.285)
    return rig


def build_body(mb: MeshBuilder) -> None:
    T = translate

    # --- bulb torso (subsurface sprout body), open top toward the neck
    prof = [(0.05, 0.0), (0.125, 0.05), (0.15, 0.13), (0.135, 0.20),
            (0.095, 0.26), (0.05, 0.30), (0.0, 0.325)]
    mb.add(T(lathe(prof, 11, smooth=True), tuple(HIPS)), "Echo_Body")
    # shoulder leaf sprigs (small blades at the collar)
    for s in (-1, 1):
        mb.add(_blade_between((s * 0.10, 0.75, 0.05),
                              (s * 0.16, 0.85, 0.02), 0.035, 0.006, 0.02, 4),
               "Echo_Body")
    # bark collar at the neck base
    mb.add(T(tube(0.062, 0.05, 0.03, 9, 1), (0, 0.83, 0.03)), "Echo_Armor")

    # --- chlorophyll veins: 3 vertical emissive strips + a spiral waist ring
    for (x, z) in ((0.0, 0.148), (-0.082, 0.118), (0.082, 0.118)):
        mb.add(T(rotate(box((0.013, 0.21, 0.008)), 0.06, 0, 0),
                 (x, 0.665, z)), "Echo_Emissive")
    mb.add(T(rotate(tube(0.132, 0.118, 0.014, 10, 1), 8 * DEG, 0, 12 * DEG),
             (0, 0.60, 0.0)), "Echo_Emissive")

    # --- head: rounded sprout skull + muzzle + eyes
    mb.add(T(sphere(0.09, 11, 8), (0, 0.94, 0.10)), "Echo_Body")
    mb.add(T(box((0.062, 0.05, 0.055)), (0, 0.912, 0.185)), "Echo_Body")
    mb.add(T(box((0.13, 0.018, 0.06)), (0, 0.985, 0.135)), "Echo_Armor")  # brow
    for sx in (-1, 1):
        mb.add(T(sphere(0.016, 7, 5), (sx * 0.046, 0.958, 0.168)), "Echo_Eye")

    # --- leaf-blade ears + armor midribs
    for base, end, mirror_end in ((EAR_L_BASE, EAR_L_END, None),
                                  ((0.085, 1.005, 0.02), (0.195, 1.16, -0.05), None)):
        mb.add(_blade_between(base, end, 0.068, 0.01, 0.05, 6), "Echo_Body")
        d = np.asarray(end) - np.asarray(base)
        mid = np.asarray(base) + d * 0.45
        mb.add(_blade_between(base, tuple(mid), 0.014, 0.012, 0.0, 3), "Echo_Armor")

    # --- bud crest: closed bud + sepals + glowing tip
    bud_prof = [(0.005, 0.0), (0.028, 0.014), (0.036, 0.05),
                (0.02, 0.088), (0.0, 0.108)]
    mb.add(T(lathe(bud_prof, 8, smooth=True), tuple(BUD_BASE)), "Echo_Body")
    for k, ang in enumerate((-40.0, 40.0, 180.0)):
        a = math.radians(ang)
        mb.add(_blade_between(BUD_BASE + np.array((0.02 * math.sin(a), 0.004, 0.012 * math.cos(a))),
                              BUD_BASE + np.array((0.052 * math.sin(a), 0.03, 0.036 * math.cos(a))),
                              0.018, 0.005, 0.01, 3), "Echo_Body")
    mb.add(T(sphere(0.013, 6, 4), tuple(BUD_END)), "Echo_Emissive")

    # --- twig arms with leaf hands + shoulder sprig mounts
    for base, end in ((ARM_L_BASE, ARM_L_END),
                      ((0.145, 0.775, 0.035), (0.19, 0.69, 0.057))):
        mb.add(_seg_capsule(base, end, 0.017, 2, 6), "Echo_Body")
        hand_dir = np.asarray(end) - np.asarray(base)
        hand = _blade_between(end, tuple(np.asarray(end) + hand_dir * 0.55 + np.array((0, 0.01, 0.012))),
                              0.032, 0.006, 0.015, 3)
        mb.add(hand, "Echo_Body")
        # shoulder sprig (bridges the collar to the arm root)
        mb.add(T(sphere(0.028, 6, 4),
                 tuple(np.asarray(base) + np.array((np.sign(base[0]) * -0.028, 0.012, -0.004)))),
               "Echo_Body")

    # --- vine-wrapped foal legs (thigh + shin/hoof + emissive vine bands)
    for top, knee, ground in ((LEG_L_TOP, LEG_L_KNEE, LEG_L_GROUND),
                              ((0.075, 0.52, -0.02), (0.075, 0.33, -0.006), (0.075, 0.045, 0.008))):
        x = top[0]
        mb.add(_seg_capsule(top, knee, 0.048, 2, 8), "Echo_Body")          # thigh
        mb.add(_seg_capsule(knee, ground, 0.03, 2, 7), "Echo_Body")        # shin
        mb.add(T(box((0.07, 0.048, 0.115)), (x, 0.032, ground[2] + 0.028)), "Echo_Armor")  # hoof
        # knee bark cap
        mb.add(T(sphere(0.038, 7, 5), tuple(knee)), "Echo_Armor")
        # emissive vine bands (thigh x2, shin x1)
        for t in (0.35, 0.7):
            p = np.asarray(top) + (np.asarray(knee) - np.asarray(top)) * t
            mb.add(T(cylinder(0.05, 0.018, 7), tuple(p)), "Echo_Emissive")
        p = np.asarray(knee) + (np.asarray(ground) - np.asarray(knee)) * 0.4
        mb.add(T(cylinder(0.033, 0.016, 7), tuple(p)), "Echo_Emissive")

    # --- rump leaf tail (tiny sprig)
    mb.add(_blade_between((0, 0.56, -0.135), (0, 0.62, -0.20), 0.04, 0.006, 0.02, 4),
           "Echo_Body")


# ----------------------------------------------------------------- animation
def anim_idle(builder: GlbBuilder, rig: Rig) -> None:
    """3.2s — sprout sway, ear flick, bud bob, gentle breathe."""
    anim = builder.add_animation("AM_Sprigling_Idle")
    dur, n = 3.2, 9
    _rot_sine(rig, anim, "Spine_01", dur, n, 2.2, 0.0, "z")
    _rot_sine(rig, anim, "Spine_02", dur, n, 1.6, 0.6, "z")
    _rot_sine(rig, anim, "Spine_01", dur, n, 1.2)
    _rot_sine(rig, anim, "Spine_02", dur, n, 0.9, 0.7)
    _rot_sine(rig, anim, "Neck", dur, n, 2.0, 1.1, "z")
    _rot_sine(rig, anim, "Neck", dur, n, 1.5, 0.4)
    _rot_sine(rig, anim, "Head", dur, n, 2.5, 1.5, "y")
    _rot_sine(rig, anim, "Head", dur, n, 1.2, 0.9)
    for tag, ph in (("L", 0.3), ("R", 2.1)):
        _rot_sine(rig, anim, f"Ear_{tag}", dur, n, 4.0, ph, "y")
        _rot_sine(rig, anim, f"Ear_{tag}", dur, n, 2.5, ph + 0.5)
    _rot_sine(rig, anim, "Bud", dur, n, 3.0, 0.8)
    _rot_sine(rig, anim, "Bud", dur, n, 3.5, 1.4, "y")
    for tag, ph in (("L", 0.0), ("R", 1.6)):
        _rot_sine(rig, anim, f"Arm_{tag}", dur, n, 3.0, ph, "x")
    _tr_sine(rig, anim, "Hips", dur, n, 0.006)


def anim_move(builder: GlbBuilder, rig: Rig) -> None:
    """0.7s — hopping biped cycle: legs spring together, hips bounce 2 beats,
    ears + bud flap, arms counter."""
    anim = builder.add_animation("AM_Sprigling_Move")
    dur, n = 0.7, 8

    def hop_swing(amp: float, phase: float) -> list:
        return [math.radians(amp) * math.sin(2.0 * math.pi * (i / n) + phase)
                for i in range(n + 1)]

    ts = [dur * i / n for i in range(n + 1)]
    for tag in ("L", "R"):
        v = hop_swing(16.0, 0.0)
        rig.add_rotation_channel(anim, f"Leg_{tag}",
                                 list(zip(ts, [(x, 0, 0) for x in v])))
        v = hop_swing(11.0, 2.4)
        rig.add_rotation_channel(anim, f"Foot_{tag}",
                                 list(zip(ts, [(x, 0, 0) for x in v])))
        v = hop_swing(6.0, 0.0)
        rig.add_rotation_channel(anim, f"Arm_{tag}",
                                 list(zip(ts, [(x, 0, 0) for x in v])))
    # hips bounce: down on landing, up through the air (2 beats per cycle)
    _tr_sine(rig, anim, "Hips", dur, n, 0.035, freq=2.0, phase=0.3)
    _rot_sine(rig, anim, "Spine_01", dur, n, 5.0, 1.2)
    _rot_sine(rig, anim, "Spine_02", dur, n, 3.5, 1.4)
    _rot_sine(rig, anim, "Neck", dur, n, 3.0, 0.9)
    _rot_sine(rig, anim, "Head", dur, n, 2.0, 1.6)
    for tag, ph in (("L", 0.0), ("R", 0.4)):
        _rot_sine(rig, anim, f"Ear_{tag}", dur, n, 14.0, ph, "x")
    _rot_sine(rig, anim, "Bud", dur, n, 10.0, 1.8)
    _rot_sine(rig, anim, "Bud", dur, n, 8.0, 1.2, "y")


def anim_hit(builder: GlbBuilder, rig: Rig) -> None:
    """0.4s — flinch back: spine/arch recoil, ears flatten, bud dips, hips drop."""
    anim = builder.add_animation("AM_Sprigling_Hit")
    deg = DEG
    ts = [0.0, 0.08, 0.16, 0.28, 0.40]
    rig.add_rotation_channel(anim, "Spine_01", list(zip(ts, [
        (0, 0, 0), (-7 * deg, 0, 0), (-5 * deg, 0, 0), (1.5 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Spine_02", list(zip(ts, [
        (0, 0, 0), (-5 * deg, 0, 0), (-3 * deg, 0, 0), (1 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Neck", list(zip(ts, [
        (0, 0, 0), (-9 * deg, 0, 0), (-6 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Head", list(zip(ts, [
        (0, 0, 0), (-12 * deg, 0, 0), (-8 * deg, 0, 0), (3 * deg, 0, 0), (0, 0, 0)])))
    for tag in ("L", "R"):
        rig.add_rotation_channel(anim, f"Ear_{tag}", list(zip(ts, [
            (0, 0, 0), (-16 * deg, 0, 0), (-11 * deg, 0, 0), (-3 * deg, 0, 0), (0, 0, 0)])))
        rig.add_rotation_channel(anim, f"Leg_{tag}", list(zip(ts, [
            (0, 0, 0), (10 * deg, 0, 0), (7 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Bud", list(zip(ts, [
        (0, 0, 0), (8 * deg, 0, 0), (5 * deg, 0, 0), (1 * deg, 0, 0), (0, 0, 0)])))
    rig.add_translation_channel(anim, "Hips", [
        (0.0, (0, 0, 0)), (0.08, (0, -0.03, -0.012)), (0.16, (0, -0.02, -0.008)),
        (0.28, (0, -0.005, -0.002)), (0.40, (0, 0, 0))])


def main() -> None:
    builder = GlbBuilder()
    for m in MATS.values():
        builder.add_material(m)

    rig = build_rig(builder)
    mb = MeshBuilder()
    build_body(mb)
    print(f"[{SPECIES}] body parts: {len(mb.parts)}, tris: {mb.triangle_count()}")

    skin_idx, prims = rig.build_skin(mb, power=3.3)
    mesh_node = builder.add_node("SK_Echo_Sprigling", parent=0,
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
    stats["ue_path"] = "/Game/Characters/Echoes/SK_Echo_Sprigling"
    record("mesh", "SK_Echo_Sprigling", stats)
    print(f"[{SPECIES}] {OUT_PATH}")
    print(f"[{SPECIES}] bones={stats['bones']} tris={stats['triangles']} "
          f"anims={len(stats['animations'])} bytes={stats['bytes']} validate={stats['validate']}")
    if problems:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
