"""
ASTRAWILD ArtSourceGen — STATIC vehicle mesh: Dawn Skiff hover skiff.

Deliverable -> ArtSource/Meshes/Vehicles/SM_Vehicle_DawnSkiff.glb

Layout (meters, +Y up, +Z forward/bow):
  * 3.6m long (Z) x 1.7m wide (X) incl. prow + stern thruster.
  * Origin = geometric center of the DECK (deck top y = +0.12, flat/walkable).
  * Hull hangs below to y = -0.35. Hover height (ground -> origin) = 1.1m
    (recorded in stats["hover_height_m"]).
  * Slots: Vehicle_Hull (worn ivory) / Vehicle_Metal (dark) /
    Vehicle_Accent (amber) / Vehicle_Glow (teal emissive).
  * Parts: tapered hull, raised bow, 2 side pontoons (capsules 2.2m),
    4 down-facing thruster nozzles (Glow), deck plating lines, cockpit
    console + emissive screen, pilot seat, 6 railing posts + rails,
    cargo straps, 2 nav lights. Budget <= 3000 tris.

Run: python3 gen_vehicle_skiff.py
"""
from __future__ import annotations

import math
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from aw_gltf import GlbBuilder, Material, validate_glb
from aw_manifest import record
from aw_shapes import (MeshBuilder, box, capsule, cone, cylinder, rotate,
                       sphere, translate, tube)

OUT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__),
                          "..", "..", "ArtSource", "Meshes", "Vehicles"))
OUT_PATH = os.path.join(OUT_DIR, "SM_Vehicle_DawnSkiff.glb")
MAX_TRIS = 3000
HOVER_HEIGHT_M = 1.1
DECK_TOP_Y = 0.12

MATS = [
    Material("Vehicle_Hull", base_color=(0.78, 0.75, 0.68, 1.0),
             metallic=0.3, roughness=0.55),
    Material("Vehicle_Metal", base_color=(0.25, 0.26, 0.30, 1.0),
             metallic=0.85, roughness=0.4),
    Material("Vehicle_Accent", base_color=(0.9, 0.6, 0.2, 1.0),
             metallic=0.6, roughness=0.5),
    Material("Vehicle_Glow", base_color=(0.02, 0.02, 0.02, 1.0),
             metallic=0.0, roughness=0.35, emissive=(0.2, 0.8, 0.75)),
]


def zcyl(r, h, radial=10, capped=True, z0=0.0, y=0.0, x=0.0):
    return translate(rotate(cylinder(r, h, radial, capped), math.pi / 2, 0, 0),
                     (x, y, z0))


def build_skiff(mb: MeshBuilder) -> None:
    # ------------------------------------------------ deck (flat, walkable)
    mb.add(translate(box((1.44, 0.10, 3.40)), (0, 0.07, 0)), "Vehicle_Hull")
    for z in (-1.35, -0.85, -0.35, 0.15, 0.65, 1.15):        # plating lines
        mb.add(translate(box((1.44, 0.012, 0.05)), (0, 0.119, z)), "Vehicle_Metal")
    for sx in (-1, 1):                                        # long plate seams
        mb.add(translate(box((0.06, 0.012, 3.30)), (sx * 0.35, 0.119, 0)), "Vehicle_Metal")
    for sx in (-1, 1):                                        # deck edge trim
        mb.add(translate(box((0.05, 0.05, 3.40)), (sx * 0.715, 0.095, 0)), "Vehicle_Metal")

    # ------------------------------------------------ hull (tapers down)
    mb.add(translate(box((1.22, 0.26, 3.05)), (0, -0.11, -0.18)), "Vehicle_Hull")
    mb.add(translate(box((1.00, 0.16, 2.30)), (0, -0.27, -0.35)), "Vehicle_Hull")
    mb.add(translate(box((0.45, 0.05, 1.40)), (0, -0.325, -0.45)), "Vehicle_Metal")
    mb.add(translate(box((1.10, 0.24, 0.08)), (0, -0.12, -1.68)), "Vehicle_Metal")

    # ------------------------------------------------ raised bow (front +Z)
    mb.add(translate(rotate(box((1.30, 0.16, 0.85)), -0.30, 0, 0),
                     (0, 0.02, 1.28)), "Vehicle_Hull")
    mb.add(translate(rotate(box((0.90, 0.10, 0.30)), -0.30, 0, 0),
                     (0, 0.09, 1.60)), "Vehicle_Hull")
    # navigation lights (Glow spheres) at bow corners
    for sx in (-1, 1):
        mb.add(translate(cylinder(0.012, 0.10, 6), (sx * 0.60, 0.16, 1.52)),
               "Vehicle_Metal")
        mb.add(translate(sphere(0.03, 8, 6), (sx * 0.60, 0.28, 1.52)), "Vehicle_Glow")

    # ------------------------------------------------ side pontoons
    for sx in (-1, 1):
        pod = rotate(capsule(0.12, 2.20, seg_v=4, radial=12), math.pi / 2, 0, 0)
        mb.add(translate(pod, (sx * 0.72, -0.06, -0.10)), "Vehicle_Hull")
        for z in (-0.80, 0.55):                                # struts
            mb.add(translate(box((0.16, 0.06, 0.10)), (sx * 0.66, 0.0, z)),
                   "Vehicle_Metal")
        for z in (-0.80, 0.55):                                # thruster nozzles
            noz = rotate(cone(0.075, 0.10, 8), math.pi, 0, 0)  # pointing down
            mb.add(translate(noz, (sx * 0.72, -0.16, z)), "Vehicle_Glow")
            mb.add(translate(cylinder(0.06, 0.05, 8), (sx * 0.72, -0.15, z)),
                   "Vehicle_Metal")

    # ------------------------------------------------ stern thruster + intakes
    mb.add(zcyl(0.11, 0.14, 12, z0=-1.84, y=-0.12), "Vehicle_Metal")
    mb.add(zcyl(0.085, 0.025, 12, z0=-1.86, y=-0.12), "Vehicle_Glow")
    for sx in (-1, 1):
        mb.add(translate(box((0.30, 0.14, 0.06)), (sx * 0.50, -0.10, -1.66)),
               "Vehicle_Metal")
        mb.add(translate(box((0.26, 0.015, 0.02)), (sx * 0.50, -0.025, -1.67)),
               "Vehicle_Glow")

    # ------------------------------------------------ cockpit console (forward)
    mb.add(translate(box((0.44, 0.22, 0.16)), (0, 0.23, 0.98)), "Vehicle_Metal")
    mb.add(translate(rotate(box((0.56, 0.26, 0.10)), -0.45, 0, 0),
                     (0, 0.36, 1.06)), "Vehicle_Metal")
    mb.add(translate(rotate(box((0.48, 0.20, 0.012)), -0.45, 0, 0),
                     (0, 0.342, 1.006)), "Vehicle_Glow")
    for sx in (-1, 1):
        mb.add(translate(rotate(box((0.14, 0.03, 0.03)), -0.45, 0, 0),
                         (sx * 0.18, 0.31, 1.08)), "Vehicle_Accent")

    # ------------------------------------------------ pilot seat (center-rear)
    mb.add(translate(box((0.30, 0.06, 0.30)), (0, 0.10, -0.64)), "Vehicle_Metal")
    mb.add(translate(box((0.40, 0.10, 0.42)), (0, 0.16, -0.62)), "Vehicle_Metal")
    mb.add(translate(box((0.42, 0.06, 0.44)), (0, 0.235, -0.62)), "Vehicle_Hull")
    mb.add(translate(rotate(box((0.46, 0.55, 0.09)), 0.12, 0, 0),
                     (0, 0.50, -0.88)), "Vehicle_Hull")
    mb.add(translate(rotate(box((0.40, 0.55, 0.04)), 0.12, 0, 0),
                     (0, 0.50, -0.93)), "Vehicle_Metal")
    mb.add(translate(box((0.26, 0.14, 0.08)), (0, 0.82, -0.90)), "Vehicle_Hull")

    # ------------------------------------------------ railings (6 posts + rails)
    for sx in (-1, 1):
        for z in (-1.45, -0.75, -0.05):
            mb.add(translate(cylinder(0.016, 0.38, 6), (sx * 0.68, 0.13, z)),
                   "Vehicle_Metal")
        top = rotate(cylinder(0.016, 1.50, 6), math.pi / 2, 0, 0)
        mb.add(translate(top, (sx * 0.68, 0.50, -0.75)), "Vehicle_Metal")
        mid = rotate(cylinder(0.012, 1.50, 6), math.pi / 2, 0, 0)
        mb.add(translate(mid, (sx * 0.68, 0.32, -0.75)), "Vehicle_Metal")

    # ------------------------------------------------ cargo (rear deck)
    mb.add(translate(box((0.55, 0.34, 0.62)), (-0.28, 0.29, -1.10)), "Vehicle_Metal")
    mb.add(translate(box((0.57, 0.05, 0.64)), (-0.28, 0.475, -1.10)), "Vehicle_Accent")
    for z in (-1.00, -1.28):                                   # straps over the crate
        mb.add(translate(box((1.30, 0.018, 0.06)), (0, 0.485, z)), "Vehicle_Accent")
    for sx in (-1, 1):                                         # strap anchors
        mb.add(translate(box((0.06, 0.06, 0.09)), (sx * 0.62, 0.16, -1.00)),
               "Vehicle_Metal")

    # ------------------------------------------------ hull dressing
    for sx in (-1, 1):
        mb.add(translate(box((0.02, 0.07, 1.60)), (sx * 0.615, -0.16, -0.30)),
               "Vehicle_Accent")                                # amber side stripes
        for z in (0.75, 1.05):
            mb.add(translate(box((0.02, 0.08, 0.18)), (sx * 0.615, -0.14, z)),
                   "Vehicle_Metal")                             # bow vents
        for z in (-1.20, -0.60, 0.20):
            mb.add(translate(rotate(cylinder(0.014, 0.022, 6), 0, 0, math.pi / 2),
                             (sx * 0.615, -0.05, z)), "Vehicle_Metal")  # hull bolts
    # stern antenna + beacon
    mb.add(translate(cylinder(0.010, 0.55, 6), (0.45, 0.13, -1.55)), "Vehicle_Metal")
    mb.add(translate(sphere(0.015, 6, 4), (0.45, 0.71, -1.55)), "Vehicle_Glow")


def main() -> None:
    mb = MeshBuilder()
    build_skiff(mb)
    print(f"[skiff] parts: {len(mb.parts)}, tris: {mb.triangle_count()}")

    builder = GlbBuilder()
    for m in MATS:
        builder.add_material(m)
    node = builder.add_node("SM_Vehicle_DawnSkiff", parent=-1)
    builder.set_mesh(node, mb.build_primitives())

    os.makedirs(OUT_DIR, exist_ok=True)
    stats = builder.save_glb(OUT_PATH)
    problems = validate_glb(OUT_PATH)
    stats["validate"] = "PASS" if not problems else "FAIL: " + "; ".join(problems)
    stats["hover_height_m"] = HOVER_HEIGHT_M
    stats["deck_top_y"] = DECK_TOP_Y
    stats["asset_type"] = "static_mesh"
    stats["ue_path"] = "/Game/Vehicles/SM_Vehicle_DawnSkiff"
    record("mesh", "SM_Vehicle_DawnSkiff", stats)

    pos = np.concatenate([m.positions for m, _ in mb.parts])
    lo, hi = pos.min(axis=0), pos.max(axis=0)
    print(f"[skiff] SM_Vehicle_DawnSkiff: tris={stats['triangles']} "
          f"bytes={stats['bytes']} validate={stats['validate']} "
          f"hover={HOVER_HEIGHT_M}m")
    print(f"        bbox x[{lo[0]:+.3f}..{hi[0]:+.3f}] y[{lo[1]:+.3f}..{hi[1]:+.3f}] "
          f"z[{lo[2]:+.3f}..{hi[2]:+.3f}] len_z={hi[2]-lo[2]:.3f} "
          f"width_x={hi[0]-lo[0]:.3f}")
    if problems:
        raise SystemExit(1)
    if stats["triangles"] > MAX_TRIS:
        raise SystemExit(f"skiff {stats['triangles']} tris > budget {MAX_TRIS}")


if __name__ == "__main__":
    main()
