"""
ASTRAWILD ArtSourceGen — Tier-B archetype library (strategy §6, PCR-4/PCR-5).

One parameterized generator per body plan. Every Tier-B species bakes its OWN
unique GLB: same rig pattern per plan, but proportions/features/palette vary
per species (deterministic name-hash jitter + the bestiary row's colors) —
never a plain recolor.

Rigs follow the Tier-A conventions (bind translation only, world-axis-aligned
rotation deltas, proximity skinning; creatures face +Z in glTF space, engine
import rotates -90 so they face engine +X).

Spec dict (per species):
    name            display name (e.g. "Rimefang")
    plan            body plan key ("quadruped" | "biped" | "avian" | "serpent"
                    | "insectoid" | "amorphous" | "floating" | "crystalline")
    size            size-class scale (1.0 = Medium; Tiny 0.55 ... Huge 1.9)
    body_color      (r, g, b) 0..1 primary from the bestiary row
    armor_color     (r, g, b) 0..1 secondary from the bestiary row
    element_color   (r, g, b) 0..1 emissive — the species element palette
    seed            deterministic per-species variation seed (name hash)

Public surface:
    build_species(builder, spec) -> (rig, skin_idx, prims)
    add_clips(builder, rig, spec)  — Idle / Move / Hit
    materials_for(spec) -> {slot: Material}
"""
from __future__ import annotations

import math
from typing import Dict, Tuple

import numpy as np

from aw_gltf import GlbBuilder, Material
from aw_rig import Rig
from aw_shapes import (MeshBuilder, box, cone, crystal, lathe, mirror_x,
                       recompute_smooth_normals, sphere, translate, rotate,
                       scale, tube)

# Element emissive palette (mirrors FAstrawildVfxPalette::GetElementTint).
ELEMENT_TINTS = {
    "Flora":  (0.36, 0.88, 0.42),
    "Ember":  (1.00, 0.48, 0.20),
    "Frost":  (0.55, 0.88, 0.98),
    "Pulse":  (0.36, 0.92, 0.86),
    "Light":  (0.98, 0.95, 0.72),
    "Ash":    (0.62, 0.58, 0.55),
    "None":   (0.85, 0.85, 0.85),
}

SIZE_SCALES = {
    "Tiny": 0.55, "Small": 0.75, "Medium": 1.0, "Large": 1.35, "Huge": 1.9,
}


def _clamp01(c):
    return tuple(max(0.0, min(1.0, float(x))) for x in c)


def _spec_hash(name: str) -> int:
    h = 2166136261
    for ch in name:
        h = (h * 16777619) ^ ord(ch)
    return h & 0x7FFFFFFF


def _jitter(seed: int, index: int, spread: float) -> float:
    """Deterministic -1..1 jitter from the species seed."""
    v = ((seed >> (index * 5)) % 1024) / 1023.0
    return (v * 2.0 - 1.0) * spread


def materials_for(spec: dict) -> Dict[str, Material]:
    body = _clamp01(spec["body_color"])
    armor = _clamp01(spec["armor_color"])
    emissive = _clamp01(spec["element_color"])
    armor_d = tuple(max(0.02, c * 0.55) for c in armor)
    return {
        "Echo_Body":     Material("Echo_Body", base_color=body + (1.0,),
                                  roughness=0.75, metallic=0.10),
        "Echo_Armor":    Material("Echo_Armor", base_color=armor_d + (1.0,),
                                  roughness=0.45, metallic=0.55),
        "Echo_Emissive": Material("Echo_Emissive", base_color=(0.02, 0.03, 0.02, 1.0),
                                  roughness=0.40, metallic=0.0, emissive=emissive),
        "Echo_Eye":      Material("Echo_Eye", base_color=(0.03, 0.05, 0.03, 1.0),
                                  roughness=0.25, metallic=0.0,
                                  emissive=(emissive[0] * 0.6 + 0.3,
                                            emissive[1] * 0.6 + 0.3,
                                            emissive[2] * 0.6 + 0.3)),
    }


# --------------------------------------------------------------- shared parts

def _capsule_y(r, length, seg_v=3, radial=10):
    prof = []
    for i in range(seg_v + 1):
        a = math.pi * 0.5 * i / seg_v
        prof.append((r * math.sin(a), -length * 0.5 - r * math.cos(a)))
    for i in range(seg_v + 1):
        a = math.pi * 0.5 * (seg_v - i)
        prof.append((r * math.sin(a), length * 0.5 + r * math.cos(a)))
    return lathe(prof, radial, smooth=True)


def _capsule_z(r, length, seg_v=3, radial=10):
    return rotate(_capsule_y(r, length, seg_v, radial), math.pi / 2.0, 0.0, 0.0)


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


def _rot_keys(rig: Rig, anim, bone: str, keys_xyzt) -> None:
    rig.add_rotation_channel(anim, bone, keys_xyzt)


# ---------------------------------------------------------------- quadruped

def _build_quadruped(rig: Rig, mb: MeshBuilder, spec: dict) -> None:
    j = lambda i, s=0.18: _jitter(spec["seed"], i, s)
    leg_len = 0.16 + j(0, 0.05)
    neck_len = 0.10 + j(1, 0.04)
    tail_len = 0.16 + j(2, 0.05)
    bulk = 1.0 + j(3, 0.18)
    horns = (spec["seed"] >> 7) % 3  # 0 none / 1 single / 2 pair
    spikes = (spec["seed"] >> 11) % 3  # dorsal spike count 0..2
    s = spec["size"]

    # --- rig
    rig.add_bone("Hips", "Root", (0, 0.33 * s, -0.17 * s), direction=(0, -1, 0), length=0.10 * s)
    rig.add_bone("Spine_01", "Hips", (0, 0.005, 0.12 * s), direction=(0, 0.08, 1), length=0.14 * s)
    rig.add_bone("Spine_02", "Spine_01", (0, 0.005, 0.12 * s), direction=(0, 0.08, 1), length=0.14 * s)
    rig.add_bone("Neck", "Spine_02", (0, 0.03, 0.10 * s), direction=(0, 0.5, 1), length=(0.14 + neck_len) * s)
    rig.add_bone("Head", "Neck", (0, 0.05, 0.12 * s), direction=(0, 0.35, 1), length=0.13 * s)
    rig.add_bone("Tail_01", "Hips", (0, 0.03, -0.12 * s), direction=(0, 0.35, -1), length=tail_len * s)
    rig.add_bone("Tail_02", "Tail_01", (0, 0.035, -0.16 * s), direction=(0, 0.55, -1), length=tail_len * 0.9 * s)
    for side, tag in ((-1, "L"), (1, "R")):
        rig.add_bone(f"Leg_{tag}", "Spine_02", (side * 0.10 * s, -0.01, -0.02),
                     direction=(0, -1, 0), length=leg_len * s)
        rig.add_bone(f"Lower_{tag}", f"Leg_{tag}", (0, -leg_len * s, 0.005),
                     direction=(0, -1, 0), length=leg_len * 0.9 * s)
        rig.add_bone(f"BLeg_{tag}", "Hips", (side * 0.10 * s, -0.02, -0.02),
                     direction=(0, -1, -0.12), length=leg_len * 1.1 * s)
        rig.add_bone(f"BLower_{tag}", f"BLeg_{tag}", (0, -leg_len * 1.1 * s, -0.005),
                     direction=(0, -1, 0.1), length=leg_len * 0.75 * s)

    # --- body (parameterized silhouette)
    T = translate
    mb.add(T(_capsule_z(0.12 * s * bulk, 0.24 * s, 3, 10), (0, 0.33 * s, -0.03)), "Echo_Body")
    mb.add(T(sphere(0.125 * s * bulk, 10, 7), (0, 0.345 * s, 0.09 * s)), "Echo_Body")
    mb.add(T(sphere(0.11 * s * bulk, 10, 7), (0, 0.33 * s, -0.17 * s)), "Echo_Body")
    mb.add(T(box((0.15 * s, 0.03, 0.30 * s)), (0, 0.205 * s, -0.02)), "Echo_Armor")  # belly plate
    for k in range(spikes):
        mb.add(T(rotate(cone(0.03 * s, 0.09 * s, 6), math.pi, 0, 0),
                 (0, 0.44 * s, (-0.05 + 0.09 * k) * s)), "Echo_Emissive")  # dorsal spikes
    # neck + head
    mb.add(T(_capsule_z(0.055 * s, 0.07 + neck_len, 3, 8), (0, 0.40 * s, 0.34 * s)), "Echo_Body")
    mb.add(T(box((0.15 * s, 0.115 * s, 0.16 * s)), (0, 0.445 * s, 0.43 * s)), "Echo_Body")
    mb.add(T(box((0.085 * s, 0.06 * s, 0.10 * s)), (0, 0.44 * s, 0.515 * s)), "Echo_Body")  # snout
    if horns > 0:
        for hx in ((-0.05, 0.05) if horns == 2 else (0.0,)):
            mb.add(T(rotate(cone(0.025 * s, 0.11 * s, 6), 0.5, 0, 0),
                     (hx * s, 0.50 * s, 0.40 * s)), "Echo_Armor")
    for sx in (-1, 1):
        mb.add(T(rotate(cone(0.03 * s, 0.10 * s, 6), 0.15, 0, 0), (sx * 0.05 * s, 0.50 * s, 0.395 * s)), "Echo_Body")  # ears
        mb.add(T(sphere(0.015 * s, 6, 5), (sx * 0.046 * s, 0.468 * s, 0.477 * s)), "Echo_Eye")
    # legs
    for sx in (-1, 1):
        mb.add(T(_capsule_y(0.042 * s, 0.09 * s, 2, 8), (sx * 0.10 * s, 0.26 * s, 0.17 * s)), "Echo_Body")
        mb.add(T(_capsule_y(0.032 * s, 0.08 * s, 2, 8), (sx * 0.10 * s, 0.105 * s, 0.175 * s)), "Echo_Body")
        mb.add(T(box((0.065 * s, 0.05, 0.10 * s)), (sx * 0.10 * s, 0.025, 0.185 * s)), "Echo_Armor")
        mb.add(T(rotate(_capsule_y(0.052 * s, 0.10 * s, 2, 8), 0, 0, sx * 0.08),
                 (sx * 0.10 * s, 0.22 * s, -0.22 * s)), "Echo_Body")
        mb.add(T(_capsule_y(0.036 * s, 0.075 * s, 2, 8), (sx * 0.10 * s, 0.07 * s, -0.22 * s)), "Echo_Body")
        mb.add(T(box((0.085 * s, 0.12, 0.15 * s)), (sx * 0.108 * s, 0.31 * s, -0.19 * s)), "Echo_Armor")  # hip plate
    # tail + emissive tip
    mb.add(T(_capsule_z(0.048 * s, tail_len, 2, 8), (0, 0.388 * s, -0.34 * s)), "Echo_Body")
    mb.add(T(sphere(0.035 * s, 8, 6), (0, 0.41 * s, (-0.34 - tail_len) * s)), "Echo_Emissive")


# --------------------------------------------------------------------- biped

def _build_biped(rig: Rig, mb: MeshBuilder, spec: dict) -> None:
    j = lambda i, s=0.2: _jitter(spec["seed"], i, s)
    hunch = 0.2 + j(0, 0.15)
    arm_bulk = 1.0 + j(1, 0.3)
    crest = (spec["seed"] >> 8) % 2
    s = spec["size"]

    rig.add_bone("Hips", "Root", (0, 0.42 * s, 0.0), direction=(0, -1, 0), length=0.10 * s)
    rig.add_bone("Spine", "Hips", (0, 0.10 * s, 0.02 * s), direction=(0, 0.3, 1), length=0.18 * s)
    rig.add_bone("Chest", "Spine", (0, 0.12 * s, 0.02 * s), direction=(0, 0.3, 1), length=0.16 * s)
    rig.add_bone("Neck", "Chest", (0, 0.03 * s, 0.06 * s), direction=(0, 0.5, 1), length=0.10 * s)
    rig.add_bone("Head", "Neck", (0, 0.05 * s, 0.09 * s), direction=(0, 0.35, 1), length=0.10 * s)
    for side, tag in ((-1, "L"), (1, "R")):
        rig.add_bone(f"Arm_{tag}", "Chest", (side * 0.16 * s, 0.02 * s, 0.0),
                     direction=(0, -1, 0.15), length=0.22 * s)
        rig.add_bone(f"Forearm_{tag}", f"Arm_{tag}", (0, -0.22 * s, 0.03 * s),
                     direction=(0, -1, 0.1), length=0.20 * s)
        rig.add_bone(f"Leg_{tag}", "Hips", (side * 0.09 * s, -0.05 * s, 0.0),
                     direction=(0, -1, 0), length=0.22 * s)
        rig.add_bone(f"Shin_{tag}", f"Leg_{tag}", (0, -0.22 * s, 0.0),
                     direction=(0, -1, 0), length=0.20 * s)

    T = translate
    # hunched torso
    mb.add(T(rotate(_capsule_y(0.14 * s, 0.26 * s, 3, 10), -hunch, 0, 0), (0, 0.55 * s, 0.0)), "Echo_Body")
    mb.add(T(sphere(0.16 * s, 10, 7), (0, 0.72 * s, 0.05 * s)), "Echo_Body")
    mb.add(T(box((0.30 * s, 0.05, 0.24 * s)), (0, 0.52 * s, -0.02 * s)), "Echo_Armor")  # chest plate
    mb.add(T(box((0.12 * s, 0.10 * s, 0.13 * s)), (0, 0.84 * s, 0.10 * s)), "Echo_Body")  # head
    if crest:
        mb.add(T(rotate(cone(0.04 * s, 0.14 * s, 6), 0.7, 0, 0), (0, 0.93 * s, 0.02 * s)), "Echo_Emissive")
    for sx in (-1, 1):
        mb.add(T(sphere(0.017 * s, 6, 5), (sx * 0.045 * s, 0.85 * s, 0.17 * s)), "Echo_Eye")
        # arms
        mb.add(T(_capsule_y(0.05 * s * arm_bulk, 0.14 * s, 2, 8), (sx * 0.16 * s, 0.66 * s, 0.02 * s)), "Echo_Body")
        mb.add(T(_capsule_y(0.038 * s * arm_bulk, 0.13 * s, 2, 8), (sx * 0.16 * s, 0.42 * s, 0.05 * s)), "Echo_Body")
        mb.add(T(sphere(0.05 * s * arm_bulk, 8, 6), (sx * 0.16 * s, 0.28 * s, 0.08 * s)), "Echo_Body")
        # legs
        mb.add(T(_capsule_y(0.06 * s, 0.15 * s, 2, 8), (sx * 0.09 * s, 0.32 * s, 0.0)), "Echo_Body")
        mb.add(T(_capsule_y(0.045 * s, 0.14 * s, 2, 8), (sx * 0.09 * s, 0.12 * s, 0.01 * s)), "Echo_Body")
        mb.add(T(box((0.09 * s, 0.05, 0.16 * s)), (sx * 0.09 * s, 0.025, 0.03 * s)), "Echo_Armor")


# --------------------------------------------------------------------- avian

def _build_avian(rig: Rig, mb: MeshBuilder, spec: dict) -> None:
    j = lambda i, s=0.2: _jitter(spec["seed"], i, s)
    wingspan = 1.0 + j(0, 0.25)
    tail_streamer = 1.0 + j(1, 0.4)
    plume = (spec["seed"] >> 9) % 2
    s = spec["size"]

    rig.add_bone("Body", "Root", (0, 0.6 * s, 0.0), direction=(0, 0.2, 1), length=0.20 * s)
    rig.add_bone("Neck", "Body", (0, 0.03 * s, 0.12 * s), direction=(0, 0.4, 1), length=0.08 * s)
    rig.add_bone("Head", "Neck", (0, 0.03 * s, 0.08 * s), direction=(0, 0.2, 1), length=0.08 * s)
    rig.add_bone("Tail", "Body", (0, 0.02 * s, -0.10 * s), direction=(0, 0.1, -1), length=0.16 * s * tail_streamer)
    for side, tag in ((-1, "L"), (1, "R")):
        rig.add_bone(f"Wing_{tag}", "Body", (side * 0.06 * s, 0.02 * s, 0.02 * s),
                     direction=(side, 0.0, 0.0), length=0.16 * s * wingspan)
        rig.add_bone(f"WingTip_{tag}", f"Wing_{tag}", (side * 0.16 * s * wingspan, 0.0, 0.0),
                     direction=(side, -0.15, -0.1), length=0.14 * s * wingspan)

    T = translate
    mb.add(T(_capsule_z(0.075 * s, 0.16 * s, 3, 10), (0, 0.6 * s, 0.0)), "Echo_Body")
    mb.add(T(box((0.09 * s, 0.08 * s, 0.11 * s)), (0, 0.65 * s, 0.19 * s)), "Echo_Body")  # head
    mb.add(T(rotate(cone(0.02 * s, 0.06 * s, 5), 1.4, 0, 0), (0, 0.64 * s, 0.26 * s)), "Echo_Armor")  # beak
    if plume:
        mb.add(T(rotate(cone(0.028 * s, 0.16 * s, 6), -0.6, 0, 0), (0, 0.73 * s, 0.12 * s)), "Echo_Emissive")  # crest plume
    for sx in (-1, 1):
        mb.add(T(sphere(0.013 * s, 6, 5), (sx * 0.035 * s, 0.665 * s, 0.23 * s)), "Echo_Eye")
        # wing membranes (flat plates — cheap, readable)
        mb.add(T(rotate(box((0.30 * s * wingspan, 0.015, 0.22 * s)), 0, 0, 0),
                 (sx * 0.19 * s * wingspan, 0.62 * s, 0.01 * s)), "Echo_Body")
        mb.add(T(rotate(box((0.14 * s * wingspan, 0.012, 0.16 * s)), 0, 0, sx * 0.25),
                 (sx * 0.38 * s * wingspan, 0.60 * s, -0.01 * s)), "Echo_Body")
    # tail streamer + emissive tip
    mb.add(T(_capsule_z(0.03 * s, 0.16 * s * tail_streamer, 2, 6), (0, 0.61 * s, -0.22 * s)), "Echo_Body")
    mb.add(T(sphere(0.028 * s, 8, 6), (0, 0.61 * s, (-0.22 - 0.14 * tail_streamer) * s)), "Echo_Emissive")
    # perching legs
    for sx in (-1, 1):
        mb.add(T(_capsule_y(0.02 * s, 0.08 * s, 2, 6), (sx * 0.04 * s, 0.54 * s, 0.0)), "Echo_Armor")


# ------------------------------------------------------------------- serpent

def _build_serpent(rig: Rig, mb: MeshBuilder, spec: dict) -> None:
    j = lambda i, s=0.2: _jitter(spec["seed"], i, s)
    segs = 6 + (spec["seed"] >> 10) % 3  # 6..8 segments
    hood = (spec["seed"] >> 13) % 2
    s = spec["size"]
    seg_len = 0.14 * s

    rig.add_bone("Head", "Root", (0, 0.25 * s, 0.30 * s), direction=(0, 0.1, 1), length=0.10 * s)
    prev = "Head"
    z = 0.30 * s
    for i in range(segs):
        z -= seg_len
        name = f"Spine_{i:02d}"
        rig.add_bone(name, prev, (0, -0.01 * s, -seg_len), direction=(0, -0.2, -1), length=seg_len)
        prev = name

    T = translate
    # head
    mb.add(T(box((0.11 * s, 0.08 * s, 0.15 * s)), (0, 0.25 * s, 0.33 * s)), "Echo_Body")
    for sx in (-1, 1):
        mb.add(T(sphere(0.014 * s, 6, 5), (sx * 0.045 * s, 0.27 * s, 0.37 * s)), "Echo_Eye")
    if hood:
        mb.add(T(rotate(box((0.16 * s, 0.18 * s, 0.03 * s)), 0, 0, 0), (0, 0.32 * s, 0.22 * s)), "Echo_Emissive")
    # body segments — tapering capsule chain with a gentle ground curl
    r = 0.075 * s
    for i in range(segs):
        z = (0.30 - seg_len * (i + 1)) * s
        y = 0.25 * s - 0.09 * s * math.sin(i / max(1, segs - 1) * math.pi)  # rests on the ground, mid lifts slightly
        r_i = r * (1.0 - 0.45 * i / max(1, segs - 1))
        mb.add(T(_capsule_z(r_i, seg_len * 0.95, 2, 8), (0, y, z)), "Echo_Body")
        if i % 2 == 0:
            mb.add(T(box((r_i * 2.4, 0.02, 0.10 * s)), (0, y + r_i, z)), "Echo_Armor")  # scale ridge
    # tail tip emissive rattle
    mb.add(T(sphere(0.03 * s, 8, 6), (0, 0.22 * s, (0.30 - seg_len * (segs + 0.4)) * s)), "Echo_Emissive")


# ----------------------------------------------------------------- insectoid

def _build_insectoid(rig: Rig, mb: MeshBuilder, spec: dict) -> None:
    j = lambda i, s=0.2: _jitter(spec["seed"], i, s)
    abdomen_bulk = 1.0 + j(0, 0.3)
    mandibles = (spec["seed"] >> 12) % 2
    ridges = 2 + (spec["seed"] >> 14) % 2
    s = spec["size"]

    rig.add_bone("Thorax", "Root", (0, 0.30 * s, 0.10 * s), direction=(0, 0, 1), length=0.14 * s)
    rig.add_bone("Abdomen", "Thorax", (0, 0.0, -0.16 * s), direction=(0, -0.1, -1), length=0.18 * s)
    rig.add_bone("Head", "Thorax", (0, 0.02 * s, 0.10 * s), direction=(0, 0.2, 1), length=0.08 * s)
    for side, tag in ((-1, "L"), (1, "R")):
        rig.add_bone(f"Antenna_{tag}", "Head", (side * 0.02 * s, 0.03 * s, 0.05 * s),
                     direction=(side, 0.5, 1), length=0.10 * s)
        for k in range(3):
            rig.add_bone(f"Leg{k}_{tag}", "Thorax",
                         (side * 0.09 * s, -0.02 * s, (0.06 - 0.06 * k) * s),
                         direction=(side, -1, 0), length=0.10 * s)

    T = translate
    mb.add(T(sphere(0.09 * s, 10, 7), (0, 0.30 * s, 0.10 * s)), "Echo_Body")  # thorax
    mb.add(T(rotate(_capsule_z(0.11 * s * abdomen_bulk, 0.16 * s, 3, 10), -0.3, 0, 0),
             (0, 0.27 * s, -0.16 * s)), "Echo_Body")  # abdomen
    for k in range(ridges):
        mb.add(T(box((0.02, 0.02, 0.14 * s)), (0, 0.38 * s, (-0.08 - 0.05 * k) * s)), "Echo_Armor")  # carapace ridges
    mb.add(T(sphere(0.055 * s, 10, 7), (0, 0.33 * s, 0.19 * s)), "Echo_Body")  # head
    for sx in (-1, 1):
        mb.add(T(sphere(0.016 * s, 6, 5), (sx * 0.03 * s, 0.35 * s, 0.22 * s)), "Echo_Eye")
        mb.add(T(rotate(cone(0.008 * s, 0.10 * s, 4), -0.4, 0, sx * 0.3),
                 (sx * 0.02 * s, 0.38 * s, 0.24 * s)), "Echo_Body")  # antennae
        if mandibles:
            mb.add(T(rotate(cone(0.015 * s, 0.07 * s, 4), 1.3, 0, sx * 0.4),
                     (sx * 0.03 * s, 0.28 * s, 0.24 * s)), "Echo_Armor")
        for k in range(3):  # legs
            z = (0.06 - 0.06 * k) * s
            mb.add(T(rotate(_capsule_y(0.014 * s, 0.14 * s, 1, 5), 0.4, 0, sx * 0.9),
                     (sx * 0.09 * s, 0.24 * s, z)), "Echo_Armor")
    # abdomen emissive glow spots
    for k in range(2):
        mb.add(T(sphere(0.02 * s, 6, 5), (0.05 * s, 0.27 * s, (-0.18 - 0.05 * k) * s)), "Echo_Emissive")


# ------------------------------------------------------------------ amorphous

def _build_amorphous(rig: Rig, mb: MeshBuilder, spec: dict) -> None:
    j = lambda i, s=0.25: _jitter(spec["seed"], i, s)
    blob_count = 6 + (spec["seed"] >> 9) % 3  # 6..8
    spread = 0.22 + j(0, 0.06)
    spikes = (spec["seed"] >> 13) % 3
    s = spec["size"]

    rig.add_bone("Core", "Root", (0, 0.35 * s, 0.0), direction=(0, -1, 0), length=0.15 * s)
    for i in range(blob_count):
        ang = 2.0 * math.pi * i / blob_count
        rig.add_bone(f"Blob_{i:02d}", "Core",
                     (math.cos(ang) * spread * s, j(i + 1, 0.05) * s, math.sin(ang) * spread * s),
                     direction=(math.cos(ang), 0.3, math.sin(ang)), length=0.10 * s)

    T = translate
    mb.add(T(sphere(0.17 * s, 12, 8), (0, 0.35 * s, 0.0)), "Echo_Body")  # core
    mb.add(T(sphere(0.075 * s, 8, 6), (0, 0.50 * s, 0.0)), "Echo_Emissive")  # eye core
    for i in range(blob_count):
        ang = 2.0 * math.pi * i / blob_count
        r_blob = (0.06 + abs(j(i + 1, 0.02))) * s
        px = math.cos(ang) * spread * s
        pz = math.sin(ang) * spread * s
        py = 0.33 * s + j(i + blob_count + 2, 0.06) * s
        mb.add(T(sphere(r_blob, 8, 6), (px, py, pz)), "Echo_Body")
    for k in range(spikes):
        ang = 2.0 * math.pi * (k + 0.5) / max(1, spikes)
        mb.add(T(rotate(cone(0.03 * s, 0.12 * s, 5), 1.2, 0, 0),
                 (math.cos(ang) * spread * 0.9 * s, 0.42 * s, math.sin(ang) * spread * 0.9 * s)), "Echo_Armor")


# ------------------------------------------------------------------- floating

def _build_floating(rig: Rig, mb: MeshBuilder, spec: dict) -> None:
    j = lambda i, s=0.25: _jitter(spec["seed"], i, s)
    orbital_count = 3
    orbital_radius = 0.20 + j(0, 0.06)
    lantern = (spec["seed"] >> 11) % 2
    s = spec["size"]

    rig.add_bone("Core", "Root", (0, 0.65 * s, 0.0), direction=(0, 0, 1), length=0.12 * s)
    for i in range(orbital_count):
        ang = 2.0 * math.pi * i / orbital_count + 0.5
        rig.add_bone(f"Orbital_{i:02d}", "Core",
                     (math.cos(ang) * orbital_radius * s, 0.0, math.sin(ang) * orbital_radius * s),
                     direction=(math.cos(ang), 0.4, math.sin(ang)), length=0.08 * s)

    T = translate
    mb.add(T(sphere(0.11 * s, 12, 8), (0, 0.65 * s, 0.0)), "Echo_Body")  # core
    mb.add(T(sphere(0.055 * s, 8, 6), (0, 0.65 * s, 0.06 * s)), "Echo_Emissive")  # heart
    if lantern:
        mb.add(T(rotate(cone(0.05 * s, 0.12 * s, 6), math.pi, 0, 0), (0, 0.80 * s, 0.0)), "Echo_Emissive")  # lantern crown
    for i in range(orbital_count):
        ang = 2.0 * math.pi * i / orbital_count + 0.5
        r_orb = (0.045 + abs(j(i + 3, 0.015))) * s
        mb.add(T(sphere(r_orb, 8, 6),
                 (math.cos(ang) * orbital_radius * s, 0.65 * s + j(i + 6, 0.04) * s,
                  math.sin(ang) * orbital_radius * s)), "Echo_Armor")
    # veil plane (thin disc — reads as an energy skirt)
    mb.add(T(rotate(box((0.30 * s, 0.30 * s, 0.008)), 0, 0, 0), (0, 0.52 * s, 0.0)), "Echo_Emissive")


# ---------------------------------------------------------------- crystalline

def _build_crystalline(rig: Rig, mb: MeshBuilder, spec: dict) -> None:
    j = lambda i, s=0.25: _jitter(spec["seed"], i, s)
    shard_count = 5 + (spec["seed"] >> 8) % 4  # 5..8
    s = spec["size"]

    rig.add_bone("Core", "Root", (0, 0.35 * s, 0.0), direction=(0, -1, 0), length=0.15 * s)
    for i in range(shard_count):
        ang = 2.0 * math.pi * i / shard_count
        rig.add_bone(f"Shard_{i:02d}", "Core",
                     (math.cos(ang) * 0.10 * s, 0.12 * s + j(i, 0.04) * s, math.sin(ang) * 0.10 * s),
                     direction=(math.cos(ang) * 0.4, 1.0, math.sin(ang) * 0.4), length=0.14 * s)

    T = translate
    mb.add(T(sphere(0.13 * s, 12, 8), (0, 0.35 * s, 0.0)), "Echo_Body")  # core mass
    mb.add(T(sphere(0.06 * s, 8, 6), (0, 0.44 * s, 0.03 * s)), "Echo_Eye")
    for i in range(shard_count):
        ang = 2.0 * math.pi * i / shard_count
        h = (0.18 + abs(j(i + 1, 0.08))) * s
        r_shard = (0.035 + abs(j(i + 4, 0.012))) * s
        tilt = 0.35 + abs(j(i + 7, 0.2))
        mb.add(T(rotate(crystal(h, r_shard, 6, tip_ratio=0.4), tilt * math.cos(ang), 0, tilt * math.sin(ang)),
                 (math.cos(ang) * 0.09 * s, 0.38 * s, math.sin(ang) * 0.09 * s)), "Echo_Emissive")
    # base rock
    mb.add(T(box((0.20 * s, 0.06, 0.20 * s)), (0, 0.24 * s, 0.0)), "Echo_Armor")


# ---------------------------------------------------------------- dispatcher

_BUILDERS = {
    "quadruped": _build_quadruped,
    "biped": _build_biped,
    "avian": _build_avian,
    "serpent": _build_serpent,
    "insectoid": _build_insectoid,
    "amorphous": _build_amorphous,
    "floating": _build_floating,
    "crystalline": _build_crystalline,
}

# Sine phases for mirrored limb pairs (diagonal gait).
_DIAGONAL = (0.0, math.pi)


def build_species(builder: GlbBuilder, spec: dict):
    """Build rig + skinned primitives for one species. Returns (rig, skin_idx, prims)."""
    plan = spec["plan"]
    build = _BUILDERS[plan]
    rig = Rig(builder, root_name="Root", root_position=(0, 0, 0))
    mb = MeshBuilder()
    build(rig, mb, spec)
    skin_idx, prims = rig.build_skin(mb, power=3.3)
    return rig, skin_idx, prims


def add_clips(builder: GlbBuilder, rig: Rig, spec: dict) -> None:
    """Idle / Move / Hit — one gait per body plan, parameterized by clip timing."""
    plan = spec["plan"]
    s = spec["size"]
    name = spec["name"]

    # ---------------- Idle: breathe + sway
    idle = builder.add_animation(f"AM_{name}_Idle")
    spine_bone = {"quadruped": "Spine_01", "biped": "Spine", "serpent": "Spine_00",
                  "insectoid": "Thorax", "amorphous": "Core", "floating": "Core",
                  "crystalline": "Core", "avian": "Body"}.get(plan, "Spine_01")
    if spine_bone in rig.bones:
        _rot_sine(rig, idle, spine_bone, 2.8, 14, 2.2, 0.0, "x")
    if plan in ("quadruped", "biped"):
        _rot_sine(rig, idle, "Head", 2.8, 14, 3.0, 0.4, "y")
    if plan == "quadruped":
        _rot_sine(rig, idle, "Tail_01", 2.8, 14, 6.0, 0.0, "y")
    if plan == "avian":
        _rot_sine(rig, idle, "Wing_L", 2.8, 14, 3.0, 0.0, "x")
        _rot_sine(rig, idle, "Wing_R", 2.8, 14, 3.0, math.pi, "x")
    if plan == "serpent":
        for i in range(0, len(rig.order), 2):
            _rot_sine(rig, idle, rig.order[i], 2.8, 14, 2.5, i * 0.35, "y")
    if plan in ("amorphous", "floating", "crystalline"):
        for i, bone in enumerate(rig.order):
            if bone == "Root":
                continue
            _rot_sine(rig, idle, bone, 2.8, 14, 4.0, i * 0.7, "y")
    if plan == "insectoid":
        _rot_sine(rig, idle, "Antenna_L", 2.8, 14, 8.0, 0.0, "x")
        _rot_sine(rig, idle, "Antenna_R", 2.8, 14, 8.0, 1.2, "x")

    # ---------------- Move: plan-specific gait
    move = builder.add_animation(f"AM_{name}_Move")
    if plan == "quadruped":
        for side, tag in ((-1, "L"), (1, "R")):
            _rot_sine(rig, move, f"Leg_{tag}", 0.9, 10, 22.0, _DIAGONAL[0 if side < 0 else 1], "x")
            _rot_sine(rig, move, f"Lower_{tag}", 0.9, 10, 16.0, _DIAGONAL[0 if side < 0 else 1] + 0.6, "x")
            _rot_sine(rig, move, f"BLeg_{tag}", 0.9, 10, 22.0, _DIAGONAL[1 if side < 0 else 0], "x")
            _rot_sine(rig, move, f"BLower_{tag}", 0.9, 10, 16.0, _DIAGONAL[1 if side < 0 else 0] + 0.6, "x")
        _rot_sine(rig, move, "Spine_01", 0.9, 10, 4.0, 0.0, "y")
        _rot_sine(rig, move, "Tail_01", 0.9, 10, 10.0, 0.0, "y")
    elif plan == "biped":
        for side, tag in ((-1, "L"), (1, "R")):
            _rot_sine(rig, move, f"Leg_{tag}", 0.9, 10, 24.0, _DIAGONAL[0 if side < 0 else 1], "x")
            _rot_sine(rig, move, f"Shin_{tag}", 0.9, 10, 18.0, _DIAGONAL[0 if side < 0 else 1] + 0.5, "x")
            _rot_sine(rig, move, f"Arm_{tag}", 0.9, 10, 16.0, _DIAGONAL[1 if side < 0 else 0], "x")
        _rot_sine(rig, move, "Spine", 0.9, 10, 5.0, 0.0, "y")
    elif plan == "avian":
        for side, tag in ((-1, "L"), (1, "R")):
            _rot_sine(rig, move, f"Wing_{tag}", 0.9, 12, 34.0, 0.0 if side < 0 else math.pi, "z")
            _rot_sine(rig, move, f"WingTip_{tag}", 0.9, 12, 20.0, 0.3 if side < 0 else math.pi + 0.3, "z")
    elif plan == "serpent":
        for i, bone in enumerate(rig.order):
            if bone == "Root":
                continue
            _rot_sine(rig, move, bone, 0.9, 12, 9.0, i * 0.5, "y")
    elif plan == "insectoid":
        for side, tag in ((-1, "L"), (1, "R")):
            for k in range(3):
                _rot_sine(rig, move, f"Leg{k}_{tag}", 0.9, 12, 20.0,
                          (k * 0.8) + (0.0 if side < 0 else math.pi), "x")
    else:
        # amorphous / floating / crystalline: bob + orbital spin
        for i, bone in enumerate(rig.order):
            if bone == "Root":
                continue
            _rot_sine(rig, move, bone, 0.9, 10, 12.0, i * 0.9, "y")
        _rot_sine(rig, move, rig.order[1], 0.9, 10, 3.0, 0.0, "x")

    # ---------------- Hit: flinch back + recover
    hit = builder.add_animation(f"AM_{name}_Hit")
    spine_bone = {"quadruped": "Spine_01", "biped": "Spine", "serpent": "Spine_00",
                  "insectoid": "Thorax", "amorphous": "Core", "floating": "Core",
                  "crystalline": "Core", "avian": "Body"}.get(plan, "Spine_01")
    if spine_bone in rig.bones:
        _rot_keys(rig, hit, spine_bone, [
            (0.0, (-0.18, 0.0, 0.0)),
            (0.12, (0.26, 0.0, 0.0)),
            (0.4, (0.0, 0.0, 0.0)),
        ])
    head_bone = "Head" if "Head" in rig.bones else None
    if head_bone:
        _rot_keys(rig, hit, head_bone, [
            (0.0, (0.0, 0.0, 0.0)),
            (0.12, (-0.3, 0.0, 0.0)),
            (0.4, (0.0, 0.0, 0.0)),
        ])
