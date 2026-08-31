"""
ASTRAWILD ArtSourceGen — SK_Survivor_Exosuit reference rig.

Sci-Fi Frontier Survivor in a field exosuit (original ASTRAWILD design):
hard-surface plates over a softsuit, emissive teal visor, amber accents, boot
thrusters, scavenger backpack, left-forearm field scanner.

Deliverable per handoff §1:
  * Skeletal mesh + 7 animation clips (Idle/Walk/Run/Jump/Aim/Fire/Gather)
  * Sockets: Weapon_R (right hand), Scanner_L (left forearm), Backpack_Spine
    — created on import by Content/Python/AwPipeline/import_all.py
  * Material slots: Survivor_Suit / Survivor_Armor / Survivor_Accent /
    Survivor_Visor / Survivor_Scanner / Survivor_Thruster  (mapped to
    M_Survivor_* PBR materials on import)

Run:  python3 gen_survivor.py
Output: ArtSource/Meshes/Characters/Survivor/SK_Survivor_Exosuit.glb
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
from aw_shapes import (MeshBuilder, box, capsule, cone, cylinder, lathe,
                       mirror_x, recompute_smooth_normals, sphere, tube,
                       translate, rotate)

OUT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__),
                          "..", "..", "ArtSource", "Meshes", "Characters", "Survivor"))
OUT_PATH = os.path.join(OUT_DIR, "SK_Survivor_Exosuit.glb")

# ----------------------------------------------------------------- materials
TEAL = (0.29, 0.86, 0.78, 1.0)      # #4ADCC8 — project visor/scanner teal
AMBER = (0.91, 0.60, 0.19, 1.0)     # #E89830 — project accent amber
SUIT = (0.16, 0.18, 0.15, 1.0)      # dark grey-green fabric
ARMOR = (0.24, 0.25, 0.27, 1.0)     # graphite plate
ACCENT = (0.55, 0.42, 0.18, 1.0)    # amber trim, low emissive

MATS = {
    "Survivor_Suit":    Material("Survivor_Suit", base_color=SUIT, roughness=0.85, metallic=0.05),
    "Survivor_Armor":   Material("Survivor_Armor", base_color=ARMOR, roughness=0.45, metallic=0.85),
    "Survivor_Accent":  Material("Survivor_Accent", base_color=ACCENT, roughness=0.5, metallic=0.6),
    "Survivor_Visor":   Material("Survivor_Visor", base_color=(0.05, 0.08, 0.08, 1.0),
                                 roughness=0.2, metallic=0.0, emissive=(0.22, 0.86, 0.78)),
    "Survivor_Scanner": Material("Survivor_Scanner", base_color=(0.07, 0.07, 0.08, 1.0),
                                 roughness=0.3, metallic=0.4, emissive=(0.91, 0.55, 0.13)),
    "Survivor_Thruster": Material("Survivor_Thruster", base_color=(0.10, 0.11, 0.12, 1.0),
                                  roughness=0.35, metallic=0.7, emissive=(0.16, 0.55, 0.50)),
}


# --------------------------------------------------------------------- build
def build_body(mb: MeshBuilder) -> None:
    """Hard-surface exosuit parts. All positions in bind world space (meters)."""

    def T(m, t):
        return translate(m, t)

    # --- torso softsuit (hips -> chest)
    mb.add(T(capsule(0.155, 0.22, seg_v=4, radial=12), (0, 1.18, 0)), "Survivor_Suit")
    # pelvis
    mb.add(T(box((0.27, 0.16, 0.19)), (0, 0.94, 0)), "Survivor_Suit")
    # belt ring
    mb.add(T(tube(0.155, 0.135, 0.05, radial=14), (0, 1.00, 0)), "Survivor_Accent")
    # abdominal plate
    mb.add(T(box((0.28, 0.13, 0.10)), (0, 1.11, 0.085)), "Survivor_Armor")
    # chest plate
    mb.add(T(box((0.35, 0.24, 0.11)), (0, 1.28, 0.07)), "Survivor_Armor")
    # chest core vent (emissive trim)
    mb.add(T(box((0.08, 0.03, 0.02)), (0, 1.33, 0.13)), "Survivor_Thruster")
    # back frame + scavenger backpack
    mb.add(T(box((0.32, 0.34, 0.07)), (0, 1.24, -0.115)), "Survivor_Armor")
    mb.add(T(box((0.26, 0.34, 0.15)), (0, 1.31, -0.22)), "Survivor_Suit")
    # backpack thruster vents (teal glow)
    for sx in (-0.085, 0.085):
        mb.add(T(cylinder(0.032, 0.05, radial=8), (sx, 1.40, -0.24)), "Survivor_Thruster")
        mb.add(T(cone(0.032, 0.05, radial=8), (sx, 1.22, -0.24)), "Survivor_Thruster")
    # antenna
    mb.add(T(cylinder(0.008, 0.14, radial=6), (0.10, 1.55, -0.26)), "Survivor_Accent")
    mb.add(T(sphere(0.012, 8, 6), (0.10, 1.63, -0.26)), "Survivor_Scanner")

    # --- neck + helmet
    mb.add(T(cylinder(0.05, 0.09, radial=10), (0, 1.40, 0)), "Survivor_Suit")
    helm = recompute_smooth_normals(
        translate(sphere(0.118, 14, 10), (0, 1.555, 0.012)))
    mb.add(helm, "Survivor_Armor")
    # visor band (emissive teal)
    mb.add(T(box((0.155, 0.055, 0.055)), (0, 1.565, 0.092)), "Survivor_Visor")
    # helmet crest
    mb.add(T(box((0.03, 0.10, 0.14)), (0, 1.63, 0.01)), "Survivor_Armor")
    # jaw guard
    mb.add(T(box((0.14, 0.05, 0.08)), (0, 1.47, 0.055)), "Survivor_Accent")

    # --- shoulders / arms (bind: arms hang down, slight outward)
    for side in (-1, 1):
        sx = side
        # pauldron
        mb.add(T(rotate(box((0.14, 0.13, 0.16)), 0, 0, sx * 0.12),
                 (sx * 0.245, 1.37, 0)), "Survivor_Armor")
        # upper arm
        mb.add(T(capsule(0.052, 0.17, seg_v=3, radial=10),
                 (sx * 0.215, 1.235, 0)), "Survivor_Suit")
        # elbow pad
        mb.add(T(sphere(0.055, 8, 6), (sx * 0.222, 1.09, 0)), "Survivor_Armor")
        # forearm guard (tapered)
        guard = rotate(box((0.10, 0.20, 0.12)), sx * -0.06, 0, 0)
        mb.add(T(guard, (sx * 0.233, 0.98, 0)), "Survivor_Armor")
        # forearm fill
        mb.add(T(capsule(0.046, 0.15, seg_v=3, radial=10),
                 (sx * 0.235, 0.975, 0)), "Survivor_Suit")
        # glove
        mb.add(T(box((0.075, 0.10, 0.06)), (sx * 0.245, 0.845, 0.005)), "Survivor_Suit")

    # left forearm field scanner (emissive screen) — Scanner_L socket zone
    mb.add(T(rotate(box((0.055, 0.085, 0.035)), -0.12, 0, 0),
             (-0.287, 0.985, 0.02)), "Survivor_Armor")
    mb.add(T(rotate(box((0.045, 0.065, 0.012)), -0.12, 0, 0),
             (-0.287, 0.985, 0.045)), "Survivor_Scanner")

    # --- legs
    for side in (-1, 1):
        sx = side
        # thigh
        mb.add(T(capsule(0.068, 0.26, seg_v=3, radial=12),
                 (sx * 0.10, 0.615, 0)), "Survivor_Suit")
        # thigh plate
        mb.add(T(rotate(box((0.13, 0.24, 0.10)), 0, 0, sx * 0.05),
                 (sx * 0.115, 0.62, 0.045)), "Survivor_Armor")
        # knee cap
        mb.add(T(sphere(0.06, 8, 6), (sx * 0.10, 0.415, 0.01)), "Survivor_Armor")
        # shin
        mb.add(T(capsule(0.056, 0.26, seg_v=3, radial=10),
                 (sx * 0.10, 0.225, -0.005)), "Survivor_Suit")
        # shin guard
        mb.add(T(box((0.11, 0.28, 0.08)), (sx * 0.105, 0.22, 0.04)), "Survivor_Armor")
        # boot
        mb.add(T(box((0.115, 0.10, 0.235)), (sx * 0.10, 0.055, 0.055)), "Survivor_Armor")
        # toe cap
        mb.add(T(box((0.11, 0.06, 0.06)), (sx * 0.10, 0.04, 0.16)), "Survivor_Accent")
        # boot thruster nozzle (emissive)
        mb.add(T(rotate(cone(0.038, 0.07, radial=8), math.pi, 0, 0),
                 (sx * 0.10, 0.035, -0.055)), "Survivor_Thruster")


def build_rig(builder: GlbBuilder) -> Rig:
    rig = Rig(builder, root_name="Root", root_position=(0, 0, 0))
    rig.add_bone("Hips", "Root", (0, 0.95, 0), direction=(0, -1, 0), length=0.06)
    rig.add_bone("Spine_01", "Hips", (0, 0.10, 0), direction=(0, 1, 0), length=0.12)
    rig.add_bone("Spine_02", "Spine_01", (0, 0.12, 0), direction=(0, 1, 0), length=0.12)
    rig.add_bone("Spine_03", "Spine_02", (0, 0.12, 0), direction=(0, 1, 0), length=0.13)
    rig.add_bone("Neck", "Spine_03", (0, 0.13, 0), direction=(0, 1, 0), length=0.08)
    rig.add_bone("Head", "Neck", (0, 0.09, 0.005), direction=(0, 1, 0), length=0.16)
    rig.add_bone("Backpack", "Spine_03", (0, 0.06, -0.16), direction=(0, 1, 0), length=0.20)

    for side, tag in ((-1, "L"), (1, "R")):
        s = side
        rig.add_bone(f"Clavicle_{tag}", "Spine_03", (s * 0.055, 0.095, 0),
                     direction=(s, 0, 0), length=0.12)
        rig.add_bone(f"UpperArm_{tag}", f"Clavicle_{tag}", (s * 0.13, -0.005, 0),
                     direction=(s * 0.10, -1, 0), length=0.27)
        rig.add_bone(f"Forearm_{tag}", f"UpperArm_{tag}", (s * 0.02, -0.27, 0),
                     direction=(s * 0.06, -1, 0), length=0.25)
        rig.add_bone(f"Hand_{tag}", f"Forearm_{tag}", (s * 0.015, -0.25, 0),
                     direction=(s * 0.06, -1, 0), length=0.16)

        rig.add_bone(f"Thigh_{tag}", "Hips", (s * 0.10, -0.12, 0),
                     direction=(s * 0.02, -1, 0), length=0.41)
        rig.add_bone(f"Shin_{tag}", f"Thigh_{tag}", (s * 0.005, -0.41, 0),
                     direction=(0, -1, -0.02), length=0.40)
        rig.add_bone(f"Foot_{tag}", f"Shin_{tag}", (0, -0.40, 0.005),
                     direction=(0, -1, 1), length=0.21)
    return rig


# ----------------------------------------------------------------- animation
DEG = math.pi / 180.0


def anim_idle(builder: GlbBuilder, rig: Rig) -> None:
    anim = builder.add_animation("AM_Survivor_Idle")
    times = [0.0, 1.0, 2.0, 3.0, 4.0]
    rig.add_rotation_channel(anim, "Spine_02", list(zip(times, [(1.6 * DEG, 0, 0), (-1.2 * DEG, 0, 0.6 * DEG), (1.2 * DEG, 0, -0.6 * DEG), (-1.6 * DEG, 0, 0), (1.6 * DEG, 0, 0)])))
    rig.add_rotation_channel(anim, "Spine_03", list(zip(times, [(1.0 * DEG, 0, 0), (-0.8 * DEG, 0, 0), (0.8 * DEG, 0, 0), (-1.0 * DEG, 0, 0), (1.0 * DEG, 0, 0)])))
    rig.add_rotation_channel(anim, "Head", list(zip(times, [(0, 2.5 * DEG, 0), (0, 4.5 * DEG, 0.8 * DEG), (0, -2.0 * DEG, -0.8 * DEG), (0, 1.5 * DEG, 0), (0, 2.5 * DEG, 0)])))
    rig.add_rotation_channel(anim, "Neck", list(zip(times, [(0, -1.5 * DEG, 0), (0, -2.5 * DEG, -0.5 * DEG), (0, 1.0 * DEG, 0.5 * DEG), (0, -0.5 * DEG, 0), (0, -1.5 * DEG, 0)])))
    rig.add_rotation_channel(anim, "UpperArm_L", list(zip(times, [(0, 0, 1.2 * DEG), (0, 0, 2.0 * DEG), (0, 0, 0.5 * DEG), (0, 0, 1.4 * DEG), (0, 0, 1.2 * DEG)])))
    rig.add_rotation_channel(anim, "UpperArm_R", list(zip(times, [(0, 0, -1.2 * DEG), (0, 0, -2.0 * DEG), (0, 0, -0.5 * DEG), (0, 0, -1.4 * DEG), (0, 0, -1.2 * DEG)])))
    rig.add_translation_channel(anim, "Hips", [(0, (0, 0.004, 0)), (1, (0, -0.003, 0)), (2, (0, 0.004, 0)), (3, (0, -0.003, 0)), (4, (0, 0.004, 0))])


def _locomotion(builder: GlbBuilder, rig: Rig, name: str, duration: float, keys: int,
                thigh: float, shin: float, arm: float, elbow: float,
                spine_lean: float, hip_bob: float, hips_yaw: float) -> None:
    """Shared walk/run generator: sinusoidal limb swing, opposing arm swing,
    contact-phase knee flex, vertical hip bob, counter-rotating torso."""
    anim = builder.add_animation(name)
    ts = [duration * i / keys for i in range(keys)]
    deg = DEG

    def cyc(amplitude_deg: float, phase: float) -> list:
        return [amplitude_deg * deg * math.sin(2 * math.pi * (i / keys) + phase) for i in range(keys)]

    thigh_l = cyc(thigh, 0.0)
    thigh_r = cyc(thigh, math.pi)
    # knee flexes during the back-to-front transition (leg recovery)
    shin_l = [(shin * 0.35 + shin * 0.65 * max(0.0, math.sin(2 * math.pi * (i / keys) + 2.3))) * deg for i in range(keys)]
    shin_r = [(shin * 0.35 + shin * 0.65 * max(0.0, math.sin(2 * math.pi * (i / keys) + 2.3 + math.pi))) * deg for i in range(keys)]
    arm_l = cyc(arm, math.pi)
    arm_r = cyc(arm, 0.0)
    elbow_l = [(elbow * deg) + 6 * deg * math.sin(2 * math.pi * (i / keys)) for i in range(keys)]
    elbow_r = [(elbow * deg) + 6 * deg * math.sin(2 * math.pi * (i / keys) + math.pi) for i in range(keys)]
    bob = [hip_bob * math.sin(2 * math.pi * (i / keys) * 2.0 + 1.2) for i in range(keys)]

    rig.add_rotation_channel(anim, "Thigh_L", list(zip(ts, [(a, 0, 0) for a in thigh_l])))
    rig.add_rotation_channel(anim, "Thigh_R", list(zip(ts, [(a, 0, 0) for a in thigh_r])))
    rig.add_rotation_channel(anim, "Shin_L", list(zip(ts, [(a, 0, 0) for a in shin_l])))
    rig.add_rotation_channel(anim, "Shin_R", list(zip(ts, [(a, 0, 0) for a in shin_r])))
    rig.add_rotation_channel(anim, "Foot_L", list(zip(ts, [(-a * 0.35, 0, 0) for a in thigh_l])))
    rig.add_rotation_channel(anim, "Foot_R", list(zip(ts, [(-a * 0.35, 0, 0) for a in thigh_r])))
    rig.add_rotation_channel(anim, "UpperArm_L", list(zip(ts, [(a, 0, 2 * deg) for a in arm_l])))
    rig.add_rotation_channel(anim, "UpperArm_R", list(zip(ts, [(a, 0, -2 * deg) for a in arm_r])))
    rig.add_rotation_channel(anim, "Forearm_L", list(zip(ts, [(-a, 0, 0) for a in elbow_l])))
    rig.add_rotation_channel(anim, "Forearm_R", list(zip(ts, [(-a, 0, 0) for a in elbow_r])))
    lean = [(spine_lean * deg, 0, 0)] * keys
    rig.add_rotation_channel(anim, "Spine_01", list(zip(ts, lean)))
    yaw = [hips_yaw * deg * math.sin(2 * math.pi * (i / keys)) for i in range(keys)]
    rig.add_rotation_channel(anim, "Hips", list(zip(ts, [(0, a, 0) for a in yaw])))
    rig.add_rotation_channel(anim, "Spine_02", list(zip(ts, [(0, -a * 0.6, 0) for a in yaw])))
    rig.add_rotation_channel(anim, "Head", list(zip(ts, [(0, -a * 0.25, 0) for a in yaw])))
    rig.add_translation_channel(anim, "Hips", list(zip(ts, [(0, b, 0) for b in bob])))


def anim_jump(builder: GlbBuilder, rig: Rig) -> None:
    anim = builder.add_animation("AM_Survivor_Jump")
    ts = [0.0, 0.25, 0.35, 0.55, 0.75, 0.9]
    deg = DEG
    # phases: crouch -> launch (extend) -> apex -> falling -> land -> recover
    thigh = [(-68, 0, 0), (18, 0, 0), (8, 0, 0), (-6, 0, 0), (-58, 0, 0), (-20, 0, 0)]
    shin = [(96, 0, 0), (5, 0, 0), (18, 0, 0), (32, 0, 0), (78, 0, 0), (28, 0, 0)]
    arm = [(-24, 0, 0), (-65, 0, 0), (-85, 0, 0), (-60, 0, 0), (-18, 0, 0), (-8, 0, 0)]
    spine = [(14, 0, 0), (-4, 0, 0), (-2, 0, 0), (6, 0, 0), (16, 0, 0), (6, 0, 0)]
    rig.add_rotation_channel(anim, "Thigh_L", list(zip(ts, [tuple(v * deg for v in t) for t in thigh])))
    rig.add_rotation_channel(anim, "Thigh_R", list(zip(ts, [tuple(v * deg for v in t) for t in thigh])))
    rig.add_rotation_channel(anim, "Shin_L", list(zip(ts, [tuple(v * deg for v in t) for t in shin])))
    rig.add_rotation_channel(anim, "Shin_R", list(zip(ts, [tuple(v * deg for v in t) for t in shin])))
    rig.add_rotation_channel(anim, "UpperArm_L", list(zip(ts, [tuple(v * deg for v in t) for t in arm])))
    rig.add_rotation_channel(anim, "UpperArm_R", list(zip(ts, [tuple(v * deg for v in t) for t in arm])))
    rig.add_rotation_channel(anim, "Spine_01", list(zip(ts, [tuple(v * deg for v in t) for t in spine])))
    rig.add_translation_channel(anim, "Hips", [
        (0.0, (0, -0.22, 0)), (0.25, (0, 0.02, 0)), (0.35, (0, 0.04, 0)),
        (0.55, (0, 0.0, 0)), (0.75, (0, -0.19, 0)), (0.9, (0, -0.05, 0))])
    rig.add_rotation_channel(anim, "Foot_L", list(zip(ts, [(-a * 0.4 * deg, 0, 0) for a in [thigh[i][0] for i in range(6)]])))
    rig.add_rotation_channel(anim, "Foot_R", list(zip(ts, [(-a * 0.4 * deg, 0, 0) for a in [thigh[i][0] for i in range(6)]])))


def anim_aim(builder: GlbBuilder, rig: Rig) -> None:
    anim = builder.add_animation("AM_Survivor_Aim")
    ts = [0.0, 0.7, 1.4, 2.0]
    deg = DEG
    sway = [1.0, -1.0, 0.6, 1.0]
    # arms raised forward, weapon braced (bind arms point down: X ~ -80 raises forward)
    rig.add_rotation_channel(anim, "UpperArm_R", list(zip(ts, [(-79 + s * 0.4, 4, -3 * deg) for s in sway])))
    rig.add_rotation_channel(anim, "Forearm_R", list(zip(ts, [(-8 * deg, 2 * deg, 0)] * 4)))
    rig.add_rotation_channel(anim, "UpperArm_L", list(zip(ts, [(-62 + s * 0.3, 14 * deg, 12 * deg) for s in sway])))
    rig.add_rotation_channel(anim, "Forearm_L", list(zip(ts, [(-38 * deg, 8 * deg, 0)] * 4)))
    rig.add_rotation_channel(anim, "Spine_02", list(zip(ts, [(-2.5 * deg, -2 * deg, 0)] * 4)))
    rig.add_rotation_channel(anim, "Spine_01", list(zip(ts, [(-2 * deg, -3 * deg, 0)] * 4)))
    rig.add_rotation_channel(anim, "Head", list(zip(ts, [(-2 * deg, 0, 0)] * 4)))
    rig.add_rotation_channel(anim, "Hips", list(zip(ts, [(2 * deg, -2 * deg, 0)] * 4)))
    rig.add_rotation_channel(anim, "Thigh_L", list(zip(ts, [(-6 * deg, 0, 2 * deg)] * 4)))
    rig.add_rotation_channel(anim, "Thigh_R", list(zip(ts, [(6 * deg, 0, -2 * deg)] * 4)))


def anim_fire(builder: GlbBuilder, rig: Rig) -> None:
    anim = builder.add_animation("AM_Survivor_Fire")
    ts = [0.0, 0.06, 0.14, 0.35]
    deg = DEG
    kick = [0.0, 7.0, 2.5, 0.0]
    rig.add_rotation_channel(anim, "UpperArm_R", list(zip(ts, [(-79 + k * deg, 4 * deg, -3 * deg) for k in kick])))
    rig.add_rotation_channel(anim, "Forearm_R", list(zip(ts, [((-8 + k * 0.6) * deg, 2 * deg, 0) for k in kick])))
    rig.add_rotation_channel(anim, "Spine_03", list(zip(ts, [(k * 0.5 * deg, 0, 0) for k in kick])))
    rig.add_rotation_channel(anim, "UpperArm_L", list(zip(ts, [(-62 - k * 0.3, 14 * deg, 12 * deg) for k in kick])))
    rig.add_rotation_channel(anim, "Head", list(zip(ts, [(-1.5 * deg, 0, 0)] * 4)))
    rig.add_translation_channel(anim, "Hips", [(0.0, (0, 0, 0)), (0.06, (0, -0.004, 0)), (0.14, (0, -0.001, 0)), (0.35, (0, 0, 0))])


def anim_gather(builder: GlbBuilder, rig: Rig) -> None:
    anim = builder.add_animation("AM_Survivor_Gather")
    ts = [0.0, 0.45, 0.9, 1.35, 1.8]
    deg = DEG
    bend = [0.0, 1.0, 1.0, 0.35, 0.0]
    rig.add_rotation_channel(anim, "Spine_01", list(zip(ts, [(b * 22 * deg, 0, 0) for b in bend])))
    rig.add_rotation_channel(anim, "Spine_02", list(zip(ts, [(b * 18 * deg, 0, 0) for b in bend])))
    rig.add_rotation_channel(anim, "Spine_03", list(zip(ts, [(b * 12 * deg, 0, 0) for b in bend])))
    rig.add_rotation_channel(anim, "UpperArm_L", list(zip(ts, [(-b * 55 * deg, 0, 6 * deg) for b in bend])))
    rig.add_rotation_channel(anim, "UpperArm_R", list(zip(ts, [(-b * 55 * deg, 0, -6 * deg) for b in bend])))
    rig.add_rotation_channel(anim, "Forearm_L", list(zip(ts, [(-b * 20 * deg, 0, 0) for b in bend])))
    rig.add_rotation_channel(anim, "Forearm_R", list(zip(ts, [(-b * 20 * deg, 0, 0) for b in bend])))
    rig.add_rotation_channel(anim, "Thigh_L", list(zip(ts, [(-b * 18 * deg, 0, 4 * deg) for b in bend])))
    rig.add_rotation_channel(anim, "Thigh_R", list(zip(ts, [(-b * 18 * deg, 0, -4 * deg) for b in bend])))
    rig.add_rotation_channel(anim, "Shin_L", list(zip(ts, [(b * 30 * deg, 0, 0) for b in bend])))
    rig.add_rotation_channel(anim, "Shin_R", list(zip(ts, [(b * 30 * deg, 0, 0) for b in bend])))
    rig.add_rotation_channel(anim, "Head", list(zip(ts, [(b * -12 * deg, 0, 0) for b in bend])))
    rig.add_translation_channel(anim, "Hips", list(zip(ts, [(0, -b * 0.14, 0) for b in bend])))


def main() -> None:
    builder = GlbBuilder()
    for m in MATS.values():
        builder.add_material(m)

    rig = build_rig(builder)
    mb = MeshBuilder()
    build_body(mb)
    print(f"[survivor] body parts: {len(mb.parts)}, tris: {mb.triangle_count()}")

    skin_idx, prims = rig.build_skin(mb, power=3.2)
    mesh_node = builder.add_node("SK_Survivor_Exosuit", parent=0,
                                 translation=(0, 0, 0))
    builder.assign_skin(mesh_node, skin_idx, prims)

    anim_idle(builder, rig)
    _locomotion(builder, rig, "AM_Survivor_Walk", 1.15, 12,
                thigh=24, shin=42, arm=16, elbow=18, spine_lean=3,
                hip_bob=0.022, hips_yaw=6)
    _locomotion(builder, rig, "AM_Survivor_Run", 0.72, 10,
                thigh=38, shin=62, arm=34, elbow=48, spine_lean=10,
                hip_bob=0.045, hips_yaw=9)
    anim_jump(builder, rig)
    anim_aim(builder, rig)
    anim_fire(builder, rig)
    anim_gather(builder, rig)

    os.makedirs(OUT_DIR, exist_ok=True)
    stats = builder.save_glb(OUT_PATH)
    problems = validate_glb(OUT_PATH)
    stats["validate"] = "PASS" if not problems else problems
    stats["sockets"] = [
        {"name": "Weapon_R", "bone": "Hand_R", "world_pos": [0.26, 0.85, 0.06],
         "rotation_deg": [-90, 0, 0]},
        {"name": "Scanner_L", "bone": "Forearm_L", "world_pos": [-0.30, 0.99, 0.05],
         "rotation_deg": [0, 90, 0]},
        {"name": "Backpack_Spine", "bone": "Backpack", "world_pos": [0.0, 1.32, -0.30],
         "rotation_deg": [180, 0, 0]},
    ]
    stats["bones"] = len(rig.bones)
    stats["asset_type"] = "skeletal_mesh"
    stats["ue_path"] = "/Game/Characters/Survivor/SK_Survivor_Exosuit"
    record("mesh", "SK_Survivor_Exosuit", stats)
    print(f"[survivor] {OUT_PATH}")
    print(f"[survivor] stats: {stats}")
    if problems:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
