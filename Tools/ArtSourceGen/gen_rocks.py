"""
ASTRAWILD ArtSourceGen — environment ROCKS (static meshes).

Sci-fi frontier stone set (Glimmerwood-adjacent moss variants):
  * SM_Rock_Granite_L     (2.5m class) — strongly displaced boulder, sunk 0.25
  * SM_Rock_Granite_M     (1.1m)       — displaced pasture stone
  * SM_Rock_Granite_S     (0.5m)       — scattered pebble stone
  * SM_Rock_Boulder_Moss  (1.4m)       — granite boulder + draped moss cap
  * SM_Cliff_Shard        (3.5m)       — 5-sided hard-edged leaning slab, jagged top

Conventions: meters, +Y up, origin at GROUND CONTACT (y=0) with deliberate
sink so scatter placement never floats on uneven terrain.
Material slots: Rock_Granite / Rock_Moss — AwPipeline maps these onto the
M_Rock_* PBR masters on import.

Run:  python3 gen_rocks.py
Output: ArtSource/Meshes/Environment/SM_Rock_*.glb, SM_Cliff_Shard.glb
"""
from __future__ import annotations

import math
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from aw_gltf import GlbBuilder, Material, validate_glb
from aw_manifest import record
from aw_shapes import (MeshBuilder, lathe, sphere, displace,
                       recompute_smooth_normals, rotate, scale, translate)

OUT_DIR = os.path.abspath(os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "..", "..", "ArtSource", "Meshes", "Environment"))

ROCK_BUDGET = 1200  # hard triangle cap for rocks


# ------------------------------------------------------- deterministic noise
def rock_noise(amp: float, fscale: float = 1.0, seed: float = 0.0):
    """Multi-frequency deterministic rock lumpiness for displace() (|v| <= amp)."""
    def fn(pos: np.ndarray) -> np.ndarray:
        x, y, z = pos[:, 0] * fscale, pos[:, 1] * fscale, pos[:, 2] * fscale
        return amp * (0.55 * np.sin(2.1 * x + 1.2 + seed)
                      * np.cos(1.7 * y - 0.6 + seed)
                      * np.sin(2.4 * z + 0.9)
                      + 0.30 * np.sin(4.6 * x - 2.0 + seed)
                      * np.cos(3.3 * z + 1.7)
                      + 0.15 * np.sin(5.2 * y + 0.8 + seed))
    return fn


# ------------------------------------------------------------- shared plumbing
def rock_materials():
    return (
        Material("Rock_Granite", base_color=(0.42, 0.41, 0.39, 1.0),
                 roughness=0.95, metallic=0.02),
        Material("Rock_Moss", base_color=(0.20, 0.35, 0.28, 1.0), roughness=0.9),
    )


def snap_and_add(mb: MeshBuilder, parts, sink: float) -> None:
    """Translates a group of parts (shared local frame) so min-y sits at -sink."""
    min_y = min(float(m.positions[:, 1].min()) for m, _ in parts)
    dy = -min_y - sink
    for m, slot in parts:
        mb.add(translate(m, (0.0, dy, 0.0)), slot)


def part_bounds(mb: MeshBuilder):
    lo = np.full(3, +np.inf)
    hi = np.full(3, -np.inf)
    for m, _ in mb.parts:
        lo = np.minimum(lo, m.positions.min(axis=0))
        hi = np.maximum(hi, m.positions.max(axis=0))
    return lo, hi


def save_asset(name: str, builder: GlbBuilder, mb: MeshBuilder,
               budget: int = ROCK_BUDGET) -> dict:
    tris = mb.triangle_count()
    assert tris <= budget, f"{name}: {tris} tris exceeds hard budget {budget}"
    node = builder.add_node(name, parent=-1)
    builder.set_mesh(node, mb.build_primitives())
    os.makedirs(OUT_DIR, exist_ok=True)
    path = os.path.join(OUT_DIR, f"{name}.glb")
    stats = builder.save_glb(path)
    problems = validate_glb(path)
    if problems:
        raise SystemExit(f"{name}: VALIDATE FAIL: {problems}")
    lo, hi = part_bounds(mb)
    stats["validate"] = "PASS"
    stats["asset_type"] = "static_mesh"
    stats["ue_path"] = f"/Game/Environment/{name}"
    stats["bounds_m"] = [round(float(hi[i] - lo[i]), 3) for i in range(3)]
    record("mesh", name, stats)
    print(f"[{name}] tris={stats['triangles']} bytes={stats['bytes']} "
          f"bounds={stats['bounds_m']}m slots={stats['materials']} validate=PASS")
    return stats


# ================================================================= granite set
def gen_rock_granite(name: str, r: float, segs, noise_amp: float, fscale: float,
                     seed: float, scl, sink: float) -> dict:
    builder = GlbBuilder()
    builder.add_material(rock_materials()[0])
    mb = MeshBuilder()
    s = sphere(r, segs[0], segs[1])
    s = displace(s, rock_noise(noise_amp, fscale, seed))
    s = recompute_smooth_normals(s)
    s = scale(s, scl)
    s = recompute_smooth_normals(s)
    snap_and_add(mb, [(s, "Rock_Granite")], sink=sink)
    return save_asset(name, builder, mb)


def gen_rock_granite_l() -> dict:
    """SM_Rock_Granite_L — 2.5m-class field boulder, strong +-0.25 displacement."""
    return gen_rock_granite("SM_Rock_Granite_L", 0.9, (18, 14), 0.25, 1.0, 0.0,
                            (1.4, 0.75, 1.1), sink=0.25)


def gen_rock_granite_m() -> dict:
    """SM_Rock_Granite_M — 1.1m displaced pasture stone."""
    return gen_rock_granite("SM_Rock_Granite_M", 0.55, (16, 12), 0.12, 1.55, 2.1,
                            (1.15, 0.8, 1.0), sink=0.12)


def gen_rock_granite_s() -> dict:
    """SM_Rock_Granite_S — 0.5m scattered pebble stone."""
    return gen_rock_granite("SM_Rock_Granite_S", 0.26, (12, 9), 0.07, 3.2, 4.2,
                            (1.1, 0.9, 1.0), sink=0.05)


# ================================================================== moss boulder
def gen_rock_boulder_moss() -> dict:
    """SM_Rock_Boulder_Moss — 1.4m granite boulder + draped moss cap."""
    name = "SM_Rock_Boulder_Moss"
    builder = GlbBuilder()
    for m in rock_materials():
        builder.add_material(m)
    mb = MeshBuilder()

    # granite boulder (local frame: ellipsoid centred at origin)
    base = sphere(0.62, 16, 12)
    base = displace(base, rock_noise(0.16, 1.4, 1.0))
    base = recompute_smooth_normals(base)
    base = scale(base, (1.15, 0.85, 1.05))
    base = recompute_smooth_normals(base)

    # moss cap: displaced hemisphere, scaled y 0.5, shifted up 0.35
    moss = lathe([(0.66, 0.0), (0.63, 0.15), (0.48, 0.33), (0.26, 0.50), (0.0, 0.60)],
                 radial=12, cap_bottom=True)
    moss = displace(moss, rock_noise(0.05, 2.6, 3.0))
    moss = recompute_smooth_normals(moss)
    moss = scale(moss, (1.05, 0.5, 1.05))
    moss = recompute_smooth_normals(moss)
    moss = translate(moss, (0.0, 0.35, 0.0))

    snap_and_add(mb, [(base, "Rock_Granite"), (moss, "Rock_Moss")], sink=0.15)
    return save_asset(name, builder, mb)


# ==================================================================== cliff shard
def gen_cliff_shard() -> dict:
    """SM_Cliff_Shard — 3.5m angular 5-sided slab, lean 25 deg, jagged top."""
    name = "SM_Cliff_Shard"
    builder = GlbBuilder()
    builder.add_material(rock_materials()[0])
    mb = MeshBuilder()

    # main shard: jagged stepped profile, hard facets (smooth=False)
    main_prof = [(1.15, 0.0), (1.32, 0.75), (1.02, 1.45), (1.16, 2.15),
                 (0.74, 2.75), (0.88, 3.15), (0.42, 3.42), (0.0, 3.55)]
    main = lathe(main_prof, radial=5, smooth=False, cap_bottom=True)
    main = displace(main, rock_noise(0.07, 1.0, 0.5))
    main = rotate(main, 0.05, 0.35, math.radians(25.0))   # lean 25 deg + yaw

    # companion shard leaning the other way
    side_prof = [(0.55, 0.0), (0.66, 0.40), (0.50, 1.10), (0.60, 1.55),
                 (0.25, 1.85), (0.0, 1.95)]
    side = lathe(side_prof, radial=5, smooth=False, cap_bottom=True)
    side = displace(side, rock_noise(0.06, 1.8, 2.5))
    side = rotate(side, -0.04, -0.55, math.radians(-18.0))
    side = translate(side, (1.05, 0.0, 0.55))

    snap_and_add(mb, [(main, "Rock_Granite"), (side, "Rock_Granite")], sink=0.35)
    return save_asset(name, builder, mb)


# ------------------------------------------------------------------------ main
def main() -> None:
    print(f"[gen_rocks] output dir: {OUT_DIR}")
    stats = [gen_rock_granite_l(), gen_rock_granite_m(), gen_rock_granite_s(),
             gen_rock_boulder_moss(), gen_cliff_shard()]
    total_tris = sum(s["triangles"] for s in stats)
    total_bytes = sum(s["bytes"] for s in stats)
    print(f"[gen_rocks] done: {len(stats)} assets, "
          f"{total_tris} tris total, {total_bytes} bytes total")


if __name__ == "__main__":
    main()
