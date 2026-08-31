"""
ASTRAWILD ArtSourceGen — STATIC ruin meshes (3 pieces).

Deliverables -> ArtSource/Meshes/Environment/:
  SM_Ruin_Pillar  tapered broken 6-sided column, 3.0m, jagged top, rune strip
  SM_Ruin_Arch    two 2.4m pillars + 3.2m lintel span, chipped corners, rune
  SM_Ruin_Block   1.1m megalith slab tilted 8 deg, carved grooves, small rune

Conventions: meters, +Y up, origin at ground center (y=0). Slots:
  Ruin_Stone (weathered granite 0.5/0.48/0.45, roughness 0.85, metallic 0.05)
  Ruin_Glow  (near-black + faint teal emissive runes)
Budget <= 2000 tris each.

Run: python3 gen_ruins.py
"""
from __future__ import annotations

import math
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from aw_gltf import GlbBuilder, Material, validate_glb
from aw_manifest import record
from aw_shapes import MeshBuilder, box, lathe, rotate, translate

OUT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__),
                          "..", "..", "ArtSource", "Meshes", "Environment"))
MAX_TRIS = 2000

MATS = [
    Material("Ruin_Stone", base_color=(0.5, 0.48, 0.45, 1.0),
             metallic=0.05, roughness=0.85),
    Material("Ruin_Glow", base_color=(0.03, 0.05, 0.05, 1.0),
             metallic=0.0, roughness=0.6, emissive=(0.12, 0.42, 0.38)),
]


def hex_column(profile, y0=0.0, x=0.0, z=0.0):
    """6-sided lathe column with hard edges (broken-column look)."""
    m = lathe(profile, radial=6, smooth=False)
    return translate(m, (x, y0, z))


def save_asset(name, mb, budget=MAX_TRIS):
    builder = GlbBuilder()
    for m in MATS:
        builder.add_material(m)
    node = builder.add_node(name, parent=-1)
    builder.set_mesh(node, mb.build_primitives())
    os.makedirs(OUT_DIR, exist_ok=True)
    path = os.path.join(OUT_DIR, name + ".glb")
    stats = builder.save_glb(path)
    problems = validate_glb(path)
    stats["validate"] = "PASS" if not problems else "FAIL: " + "; ".join(problems)
    stats["asset_type"] = "static_mesh"
    stats["ue_path"] = "/Game/Environment/Ruins/" + name
    record("mesh", name, stats)
    pos = np.concatenate([m.positions for m, _ in mb.parts])
    lo, hi = pos.min(axis=0), pos.max(axis=0)
    print(f"[ruin] {name}: tris={stats['triangles']} bytes={stats['bytes']} "
          f"validate={stats['validate']} "
          f"bbox y[{lo[1]:+.3f}..{hi[1]:+.3f}] "
          f"x[{lo[0]:+.2f}..{hi[0]:+.2f}] z[{lo[2]:+.2f}..{hi[2]:+.2f}]")
    if problems:
        raise SystemExit(1)
    if stats["triangles"] > budget:
        raise SystemExit(f"{name}: {stats['triangles']} tris > budget {budget}")
    return stats


# ---------------------------------------------------------------- Pillar
def build_pillar():
    """Tapered broken column: plinth + 6-sided shaft + jagged top + runes."""
    mb = MeshBuilder()
    # stepped plinth
    mb.add(translate(box((0.80, 0.16, 0.80)), (0, 0.08, 0)), "Ruin_Stone")
    mb.add(translate(box((0.64, 0.10, 0.64)), (0, 0.20, 0)), "Ruin_Stone")
    # tapered 6-sided shaft (hard edges), broken collar
    shaft = hex_column([(0.24, 0.00), (0.24, 0.06), (0.205, 0.18),
                        (0.195, 2.30), (0.215, 2.45), (0.19, 2.57)], y0=0.25)
    mb.add(shaft, "Ruin_Stone")
    collar = lathe([(0.27, 0.0), (0.27, 0.09)], radial=6, smooth=False,
                   cap_top=True, cap_bottom=True)
    mb.add(translate(collar, (0, 2.72, 0)), "Ruin_Stone")
    # jagged broken top (offset blocks)
    mb.add(translate(box((0.28, 0.10, 0.26)), (0.03, 2.86, 0.02)), "Ruin_Stone")
    mb.add(translate(rotate(box((0.20, 0.08, 0.18)), 0, 0.4, 0),
                     (-0.10, 2.92, -0.06)), "Ruin_Stone")
    mb.add(translate(rotate(box((0.14, 0.07, 0.14)), 0, 1.0, 0),
                     (0.11, 2.95, 0.07)), "Ruin_Stone")
    # rune strips (faint teal) on two shaft faces
    mb.add(translate(rotate(box((0.05, 0.55, 0.015)), 0, 1.047, 0),
                     (0.150, 1.35, 0.0865)), "Ruin_Glow")
    mb.add(translate(rotate(box((0.05, 0.45, 0.015)), 0, -1.047, 0),
                     (-0.150, 1.90, 0.0865)), "Ruin_Glow")
    # fallen debris at the base
    mb.add(translate(rotate(box((0.25, 0.18, 0.20)), 0, 0.7, 0),
                     (0.48, 0.09, 0.28)), "Ruin_Stone")
    mb.add(translate(rotate(box((0.18, 0.12, 0.15)), 0, 1.9, 0),
                     (-0.42, 0.06, -0.32)), "Ruin_Stone")
    return mb


# ------------------------------------------------------------------ Arch
def build_arch():
    """Two 2.4m pillars + 3.2m lintel, chipped corners, lintel rune."""
    mb = MeshBuilder()
    pillar_profile = [(0.20, 0.00), (0.20, 0.05), (0.175, 0.15),
                      (0.17, 2.05), (0.19, 2.16), (0.16, 2.26)]
    for sx in (-1, 1):
        mb.add(translate(box((0.55, 0.14, 0.55)), (sx * 1.55, 0.07, 0)), "Ruin_Stone")
        mb.add(hex_column(pillar_profile, y0=0.14, x=sx * 1.55), "Ruin_Stone")
        mb.add(translate(box((0.44, 0.10, 0.44)), (sx * 1.55, 2.45, 0)), "Ruin_Stone")
        # pillar runes (front face)
        mb.add(translate(box((0.16, 0.40, 0.02)), (sx * 1.55, 1.50, 0.152)), "Ruin_Glow")
    # lintel spanning 3.2m + crown cap
    mb.add(translate(box((3.20, 0.36, 0.58)), (0, 2.68, 0)), "Ruin_Stone")
    mb.add(translate(box((3.30, 0.14, 0.50)), (0, 2.93, 0)), "Ruin_Stone")
    # chipped corners (offset broken blocks)
    mb.add(translate(rotate(box((0.18, 0.16, 0.20)), 0, 0.5, 0),
                     (1.68, 2.56, 0.15)), "Ruin_Stone")
    mb.add(translate(rotate(box((0.15, 0.14, 0.18)), 0, 1.2, 0),
                     (-1.70, 2.62, -0.12)), "Ruin_Stone")
    # lintel rune (facing +Z)
    mb.add(translate(box((0.90, 0.12, 0.02)), (0, 2.72, 0.30)), "Ruin_Glow")
    # fallen debris
    mb.add(translate(rotate(box((0.30, 0.20, 0.25)), 0, 0.7, 0),
                     (1.05, 0.10, 0.35)), "Ruin_Stone")
    mb.add(translate(rotate(box((0.22, 0.14, 0.20)), 0, 1.9, 0),
                     (-1.15, 0.07, -0.30)), "Ruin_Stone")
    mb.add(translate(rotate(box((0.26, 0.16, 0.22)), 0, 2.8, 0),
                     (0.40, 0.08, -0.55)), "Ruin_Stone")
    return mb


# ----------------------------------------------------------------- Block
def build_block():
    """1.1m megalith slab tilted 8 deg with carved grooves + small rune."""
    mb = MeshBuilder()
    tilt = math.radians(8.0)
    # slab + dressing built in local space, then tilted/raised as a group
    group = [
        (box((0.85, 1.10, 0.32)), (0.0, 0.0, 0.0), "Ruin_Stone"),
        (box((0.87, 0.09, 0.335)), (0.0, 0.53, 0.0), "Ruin_Stone"),   # top chamfer
        (box((0.55, 0.025, 0.02)), (0.0, -0.15, 0.165), "Ruin_Stone"),  # grooves
        (box((0.55, 0.025, 0.02)), (0.0, 0.05, 0.165), "Ruin_Stone"),
        (box((0.55, 0.025, 0.02)), (0.0, 0.25, 0.165), "Ruin_Stone"),
        (box((0.72, 0.05, 0.018)), (0.0, -0.34, 0.165), "Ruin_Stone"),  # carved band
        (box((0.16, 0.16, 0.02)), (0.10, 0.34, 0.165), "Ruin_Glow"),    # rune
        (box((0.02, 0.30, 0.10)), (0.425, 0.02, 0.0), "Ruin_Stone"),    # side notch
        (box((0.02, 0.30, 0.10)), (-0.425, 0.02, 0.0), "Ruin_Stone"),
    ]
    for shape, local, slot in group:
        m = translate(shape, local)
        m = rotate(m, 0, 0, tilt)
        mb.add(translate(m, (0.0, 0.604, 0.0)), slot)
    # rubble at the base (ground level)
    mb.add(translate(rotate(box((0.60, 0.10, 0.45)), 0, 0.3, 0),
                     (0.18, 0.05, 0.14)), "Ruin_Stone")
    mb.add(translate(rotate(box((0.30, 0.14, 0.25)), 0, 1.1, 0),
                     (-0.38, 0.07, -0.08)), "Ruin_Stone")
    mb.add(translate(rotate(box((0.16, 0.10, 0.18)), 0, 2.3, 0),
                     (0.52, 0.05, -0.24)), "Ruin_Stone")
    return mb


def main() -> None:
    print(f"[ruins] output dir: {OUT_DIR}")
    jobs = [
        ("SM_Ruin_Pillar", build_pillar),
        ("SM_Ruin_Arch", build_arch),
        ("SM_Ruin_Block", build_block),
    ]
    total = 0
    for name, fn in jobs:
        stats = save_asset(name, fn())
        total += stats["triangles"]
    print(f"[ruins] done: 3 static meshes, {total} tris total")


if __name__ == "__main__":
    main()
