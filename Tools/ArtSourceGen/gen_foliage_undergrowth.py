"""
ASTRAWILD ArtSourceGen — environment foliage: UNDERGROWTH (static meshes).

Bioluminescent sci-fi frontier ground flora:
  * SM_Fern        (0.65m) — 8 arcing blades + young centre frond, canopy green
  * SM_SporeBush   (0.85m) — 4 displaced blob cluster + 6 glow spore dots
  * SM_Grass_Tuft  (0.35m) — 9 thin fanned blades (budget <= 350 tris)
  * SM_GlowReed    (1.3m)  — 5 tall marsh stalks with emissive tip bulbs

Conventions: meters, +Y up, origin at GROUND CONTACT (y=0), natural baked tilt.
Material slots: Foliage_Canopy / Foliage_Glow — AwPipeline maps these onto the
M_Foliage_* PBR masters on import (leaf master has the wind/tint params).

Run:  python3 gen_foliage_undergrowth.py
Output: ArtSource/Meshes/Environment/SM_{Fern,SporeBush,Grass_Tuft,GlowReed}.glb
"""
from __future__ import annotations

import math
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from aw_gltf import (GlbBuilder, Material, quat_from_euler, quat_to_matrix,
                     validate_glb)
from aw_manifest import record
from aw_shapes import (MeshBuilder, blade, cylinder, sphere, displace,
                       recompute_smooth_normals, rotate, scale, translate)

OUT_DIR = os.path.abspath(os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "..", "..", "ArtSource", "Meshes", "Environment"))

BUSH_BUDGET = 900   # shrubs / ferns / reeds
GRASS_BUDGET = 350  # grass tufts


# ------------------------------------------------------- deterministic noise
def wobble(amp: float, freq: float, seed: float):
    """Deterministic organic noise (sin/cos product) for displace()."""
    def fn(pos: np.ndarray) -> np.ndarray:
        return (amp
                * np.sin(freq * pos[:, 0] + seed)
                * np.sin(freq * 0.83 * pos[:, 1] + seed * 1.7 + 0.6)
                * np.cos(freq * 1.19 * pos[:, 2] + seed * 0.31 + 1.1))
    return fn


def sway(amp: float, freq: float, seed: float):
    """Azimuth-modulated radial displacement -> bends a stalk, not just bulges."""
    def fn(pos: np.ndarray) -> np.ndarray:
        az = np.arctan2(pos[:, 2], pos[:, 0])
        return amp * np.sin(freq * pos[:, 1] + seed) * np.cos(az)
    return fn


def rot_vec(v, rx: float = 0.0, ry: float = 0.0, rz: float = 0.0) -> np.ndarray:
    m = quat_to_matrix(quat_from_euler(rx, ry, rz))
    return m @ np.asarray(v, dtype=float)


# ------------------------------------------------------------- shared plumbing
def foliage_materials():
    return (
        Material("Foliage_Canopy", base_color=(0.16, 0.42, 0.32, 1.0), roughness=0.8),
        Material("Foliage_Glow", base_color=(0.02, 0.03, 0.035, 1.0),
                 roughness=0.6, emissive=(0.2, 0.8, 0.7)),
    )


def part_bounds(mb: MeshBuilder):
    lo = np.full(3, +np.inf)
    hi = np.full(3, -np.inf)
    for m, _ in mb.parts:
        lo = np.minimum(lo, m.positions.min(axis=0))
        hi = np.maximum(hi, m.positions.max(axis=0))
    return lo, hi


def save_asset(name: str, builder: GlbBuilder, mb: MeshBuilder,
               budget: int) -> dict:
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


def snap_and_add(mb: MeshBuilder, parts, sink: float = 0.0) -> None:
    """Translates a group of parts (shared local frame) so min-y sits at -sink."""
    min_y = min(float(m.positions[:, 1].min()) for m, _ in parts)
    dy = -min_y - sink
    for m, slot in parts:
        mb.add(translate(m, (0.0, dy, 0.0)), slot)


# ========================================================================= fern
def gen_fern() -> dict:
    """SM_Fern — 0.65m, 8 blades at 45 deg steps tilted out 25-40 deg."""
    name = "SM_Fern"
    builder = GlbBuilder()
    builder.add_material(foliage_materials()[0])   # canopy green only
    mb = MeshBuilder()

    n = 8
    for i in range(n):
        yaw = i * (2.0 * math.pi / n) + 0.16 * math.sin(i * 2.1)
        tilt = math.radians(25.0 + 15.0 * (0.5 + 0.5 * math.sin(i * 1.9 + 0.7)))
        ln = 0.60 * (0.92 + 0.10 * math.sin(i * 1.3))
        b = blade(length=ln, width=0.09, thickness=0.006, curve=0.18, segs=5)
        b = displace(b, wobble(0.012, 7.0, i * 0.83))
        b = recompute_smooth_normals(b)
        b = rotate(rotate(b, tilt, 0, 0), 0, yaw, 0)
        b = translate(b, (0.012 * math.sin(i * 3.3), 0.0, 0.012 * math.cos(i * 3.3)))
        mb.add(b, "Foliage_Canopy")

    # young centre frond — carries the full 0.65m silhouette height
    b = blade(length=0.64, width=0.07, thickness=0.005, curve=0.10, segs=5)
    b = displace(b, wobble(0.010, 7.5, 4.4))
    b = recompute_smooth_normals(b)
    b = rotate(b, 0.10, 0.80, 0.03)
    mb.add(b, "Foliage_Canopy")

    return save_asset(name, builder, mb, budget=BUSH_BUDGET)


# =================================================================== spore bush
def gen_spore_bush() -> dict:
    """SM_SporeBush — 0.85m blob cluster + 6 emissive spore dots."""
    name = "SM_SporeBush"
    builder = GlbBuilder()
    for m in foliage_materials():
        builder.add_material(m)
    mb = MeshBuilder()

    parts = []
    blobs = [  # (center, radius)
        ((0.00, 0.46, 0.00), 0.42),
        ((0.30, 0.32, 0.20), 0.31),
        ((-0.27, 0.36, -0.12), 0.28),
        ((0.06, 0.28, -0.29), 0.25),
    ]
    for i, (c, r) in enumerate(blobs):
        s = sphere(r, 10, 8)
        s = displace(s, wobble(0.05, 4.4, 1.0 + i * 2.3))
        s = recompute_smooth_normals(s)
        s = scale(s, (1.0 + 0.10 * math.sin(1.0 + i), 0.86 + 0.06 * math.cos(i * 1.3),
                      1.0 + 0.08 * math.sin(i * 0.7)))
        s = recompute_smooth_normals(s)
        s = translate(s, (c[0], c[1], c[2]))
        parts.append((s, "Foliage_Canopy"))

    # 6 glow spore dots on the upper blob surfaces
    for i in range(6):
        c, r = blobs[i % 3]
        a = 0.9 * i + 0.3
        el = 0.55 + 0.30 * math.sin(i * 1.4)             # elevation, biased up
        d = np.array([math.cos(a) * math.cos(el), math.sin(el),
                      math.sin(a) * math.cos(el)])
        p = np.asarray(c, dtype=float) + (r + 0.01) * d
        dot = sphere(0.028, 5, 3)
        parts.append((translate(dot, (float(p[0]), float(p[1]), float(p[2]))),
                      "Foliage_Glow"))

    snap_and_add(mb, parts, sink=0.02)   # ground contact at y=0
    return save_asset(name, builder, mb, budget=BUSH_BUDGET)


# ==================================================================== grass tuft
def gen_grass_tuft() -> dict:
    """SM_Grass_Tuft — 0.35m, 9 thin blades fanned outward (<= 350 tris)."""
    name = "SM_Grass_Tuft"
    builder = GlbBuilder()
    builder.add_material(foliage_materials()[0])   # canopy green only
    mb = MeshBuilder()

    n = 9
    for i in range(n):
        yaw = i * (2.0 * math.pi / n) + 0.25 * math.sin(i * 2.7)
        tilt = 0.16 + 0.30 * (0.5 + 0.5 * math.sin(i * 1.7 + 0.4))
        ln = 0.33 * (0.88 + 0.20 * (0.5 + 0.5 * math.cos(i * 1.1)))
        b = blade(length=ln, width=0.05, thickness=0.004, curve=0.06, segs=3)
        b = displace(b, wobble(0.008, 8.0, i * 1.1))
        b = recompute_smooth_normals(b)
        b = rotate(rotate(b, tilt, 0, 0), 0, yaw, 0)
        b = translate(b, (0.014 * math.sin(i * 3.1), 0.0, 0.014 * math.cos(i * 3.1)))
        mb.add(b, "Foliage_Canopy")

    return save_asset(name, builder, mb, budget=GRASS_BUDGET)


# ====================================================================== glow reed
def gen_glow_reed() -> dict:
    """SM_GlowReed — 1.3m marsh flora: 5 swaying stalks + emissive tip bulbs."""
    name = "SM_GlowReed"
    builder = GlbBuilder()
    for m in foliage_materials():
        builder.add_material(m)
    mb = MeshBuilder()

    for i in range(5):
        h = 1.10 + 0.20 * (0.5 + 0.5 * math.sin(i * 1.9 + 0.6))    # 1.10-1.30m
        tilt = 0.06 + 0.10 * (0.5 + 0.5 * math.sin(i * 2.4))       # 3.4-9.2 deg
        yaw = i * (2.0 * math.pi / 5.0) + 0.5 * math.sin(i * 1.3)
        base_off = (0.10 * math.sin(yaw + 1.2), 0.0, 0.10 * math.cos(yaw + 1.2))

        st = cylinder(0.012, h, radial=5, capped=True)
        st = recompute_smooth_normals(displace(st, sway(0.03, 2.2, i)))  # gentle bend
        st = rotate(rotate(st, tilt, 0, 0), 0, yaw, 0)
        st = translate(st, base_off)
        mb.add(st, "Foliage_Canopy")

        top = np.asarray(base_off, dtype=float) + \
            rot_vec(rot_vec((0.0, h, 0.0), tilt, 0, 0), 0, yaw, 0)
        bulb = sphere(0.035, 7, 5)
        bulb = recompute_smooth_normals(displace(bulb, wobble(0.004, 25.0, i)))
        mb.add(translate(bulb, (float(top[0]), float(top[1]) + 0.012, float(top[2]))),
               "Foliage_Glow")

    # small marsh leaf blades at the cluster base
    for i in range(4):
        yaw = i * 1.7 + 0.4 * math.sin(i * 2.2)
        tilt = 0.22 + 0.14 * math.sin(i * 1.5)
        b = blade(length=0.28, width=0.03, thickness=0.003, curve=0.10, segs=2)
        b = rotate(rotate(b, tilt, 0, 0), 0, yaw, 0)
        b = translate(b, (0.04 * math.sin(yaw * 2.0), 0.0, 0.04 * math.cos(yaw * 2.0)))
        mb.add(b, "Foliage_Canopy")

    return save_asset(name, builder, mb, budget=BUSH_BUDGET)


# ------------------------------------------------------------------------ main
def main() -> None:
    print(f"[gen_foliage_undergrowth] output dir: {OUT_DIR}")
    stats = [gen_fern(), gen_spore_bush(), gen_grass_tuft(), gen_glow_reed()]
    total_tris = sum(s["triangles"] for s in stats)
    total_bytes = sum(s["bytes"] for s in stats)
    print(f"[gen_foliage_undergrowth] done: {len(stats)} assets, "
          f"{total_tris} tris total, {total_bytes} bytes total")


if __name__ == "__main__":
    main()
