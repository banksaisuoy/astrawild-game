"""
ASTRAWILD ArtSourceGen — STATIC resource node meshes (4 types).

Deliverables -> ArtSource/Meshes/Environment/:
  SM_Node_Astraite      teal emissive crystal cluster (6 shards + 2 satellites)
  SM_Node_Pyronite      ember emissive cluster (5 shards) on scorched rock
  SM_Node_Voidstone     violet: dark 5-sided monolith + 3 floating shards
  SM_Node_AncientVein   gold shards growing through a cracked ruin slab

Conventions: meters, +Y up, base origin at ground (y=0). Slots:
  Node_Rock   (grey-brown, roughness 0.9)   Node_Crystal (near-black + emissive)
Cluster shards: 5-8 crystal() shards, heights 0.25-0.85m, tilted up to 20 deg,
on a displaced/flattened rock base. Budget <= 1800 tris each.

Run: python3 gen_resource_nodes.py
"""
from __future__ import annotations

import math
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from aw_gltf import GlbBuilder, Material, validate_glb
from aw_manifest import record
from aw_shapes import (MeshBuilder, box, crystal, lathe, displace,
                       recompute_smooth_normals, rotate, scale, sphere,
                       translate)

OUT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__),
                          "..", "..", "ArtSource", "Meshes", "Environment"))
MAX_TRIS = 1800
TILT_MAX = math.radians(20.0)


def rock_base(radius, height, seed, lumpy=0.05, seg_u=16, seg_v=10):
    """Displaced sphere flattened to `height` tall, sitting on y=0."""
    m = sphere(radius, seg_u, seg_v)
    m = displace(m, lambda p: (
        lumpy * np.sin(3.1 * p[:, 0] + seed) * np.sin(2.7 * p[:, 2] + 1.3 * seed)
        + 0.5 * lumpy * np.sin(5.3 * p[:, 1] + 2.1 * seed)))
    m = scale(m, (1.0, height / (2.0 * radius), 1.0))
    m = recompute_smooth_normals(m)
    ymin = float(m.positions[:, 1].min())
    return translate(m, (0.0, -ymin - 0.015, 0.0))


def shard(mb, slot, height, r, x, y, z, tilt_rad=0.0, sides=6):
    """Crystal shard at (x, y, z), leaning away from the cluster origin."""
    m = crystal(height, r, sides)
    if tilt_rad:
        az = math.atan2(-z, x)          # lean direction = away from origin
        m = rotate(m, 0, 0, -tilt_rad)  # lean toward +X
        m = rotate(m, 0, az, 0)         # swing lean outward
    mb.add(translate(m, (x, y, z)), slot)


def node_mats(rock_rgb, glow_rgb, rock_rough=0.9):
    return [
        Material("Node_Rock", base_color=(rock_rgb[0], rock_rgb[1], rock_rgb[2], 1.0),
                 metallic=0.05, roughness=rock_rough),
        Material("Node_Crystal", base_color=(0.04, 0.04, 0.05, 1.0),
                 metallic=0.1, roughness=0.25, emissive=glow_rgb),
    ]


def save_asset(name, mats, mb, budget=MAX_TRIS):
    builder = GlbBuilder()
    for m in mats:
        builder.add_material(m)
    node = builder.add_node(name, parent=-1)
    builder.set_mesh(node, mb.build_primitives())
    os.makedirs(OUT_DIR, exist_ok=True)
    path = os.path.join(OUT_DIR, name + ".glb")
    stats = builder.save_glb(path)
    problems = validate_glb(path)
    stats["validate"] = "PASS" if not problems else "FAIL: " + "; ".join(problems)
    stats["asset_type"] = "static_mesh"
    stats["ue_path"] = "/Game/Environment/ResourceNodes/" + name
    record("mesh", name, stats)
    pos = np.concatenate([m.positions for m, _ in mb.parts])
    lo, hi = pos.min(axis=0), pos.max(axis=0)
    print(f"[node] {name}: tris={stats['triangles']} bytes={stats['bytes']} "
          f"validate={stats['validate']} "
          f"bbox y[{lo[1]:+.3f}..{hi[1]:+.3f}] footprint "
          f"x[{lo[0]:+.2f}..{hi[0]:+.2f}] z[{lo[2]:+.2f}..{hi[2]:+.2f}]")
    if problems:
        raise SystemExit(1)
    if stats["triangles"] > budget:
        raise SystemExit(f"{name}: {stats['triangles']} tris > budget {budget}")
    return stats


# ------------------------------------------------------------- Astraite
def build_astraite():
    """Teal crystal cluster: 6 shards + 2 tiny satellites on rock."""
    mb = MeshBuilder()
    mb.add(rock_base(0.55, 0.35, seed=1.7, lumpy=0.05), "Node_Rock")
    shards = [  # (h, r, x, z, y_base, tilt)
        (0.72, 0.100, 0.16, 0.10, 0.16, 0.20),
        (0.55, 0.075, -0.14, 0.14, 0.15, 0.16),
        (0.85, 0.115, 0.02, -0.18, 0.20, 0.14),
        (0.45, 0.060, -0.20, -0.10, 0.12, 0.28),
        (0.38, 0.055, 0.24, -0.05, 0.10, 0.22),
        (0.30, 0.050, -0.05, 0.24, 0.14, 0.18),
    ]
    for h, r, x, z, y, t in shards:
        shard(mb, "Node_Crystal", h, r, x, y, z, t)
    # 2 tiny satellite shards on the rock apron
    shard(mb, "Node_Crystal", 0.16, 0.030, 0.42, 0.06, -0.20, 0.35, sides=5)
    shard(mb, "Node_Crystal", 0.13, 0.025, -0.38, 0.05, 0.30, 0.35, sides=5)
    return mb, node_mats((0.45, 0.42, 0.38), (0.25, 0.9, 0.8))


# ------------------------------------------------------------ Pyronite
def build_pyronite():
    """Ember cluster: 5 shards on scorched (darker) rock."""
    mb = MeshBuilder()
    mb.add(rock_base(0.52, 0.32, seed=3.9, lumpy=0.07), "Node_Rock")
    shards = [
        (0.65, 0.095, 0.10, 0.12, 0.15, 0.20),
        (0.50, 0.070, -0.16, 0.10, 0.13, 0.30),
        (0.78, 0.105, 0.05, -0.15, 0.18, 0.14),
        (0.42, 0.055, -0.18, -0.12, 0.10, 0.25),
        (0.32, 0.050, 0.22, -0.02, 0.09, 0.35),
    ]
    for h, r, x, z, y, t in shards:
        shard(mb, "Node_Crystal", h, r, x, y, z, t)
    # ember glints in the scorched rock
    mb.add(translate(box((0.06, 0.012, 0.10)), (0.30, 0.24, 0.12)), "Node_Crystal")
    mb.add(translate(box((0.05, 0.012, 0.08)), (-0.33, 0.22, -0.10)), "Node_Crystal")
    return mb, node_mats((0.28, 0.25, 0.23), (1.0, 0.45, 0.1), rock_rough=0.95)


# ----------------------------------------------------------- Voidstone
def build_voidstone():
    """Dark angular 5-sided monolith (1.3m) + 3 floating violet shards."""
    mb = MeshBuilder()
    mb.add(rock_base(0.45, 0.22, seed=5.5, lumpy=0.05), "Node_Rock")
    monolith = lathe(
        [(0.30, 0.00), (0.34, 0.10), (0.28, 0.55), (0.33, 0.95),
         (0.20, 1.22), (0.00, 1.30)],
        radial=5, smooth=False)
    mb.add(translate(monolith, (0.02, 0.0, 0.03)), "Node_Rock")
    # 3 floating small shards around the monolith
    shard(mb, "Node_Crystal", 0.22, 0.050, 0.55, 0.72, 0.10, 0.30, sides=5)
    shard(mb, "Node_Crystal", 0.18, 0.040, -0.50, 0.55, -0.25, 0.25, sides=5)
    shard(mb, "Node_Crystal", 0.20, 0.045, 0.10, 0.95, -0.48, 0.35, sides=5)
    # 2 ground shards marking the base
    shard(mb, "Node_Crystal", 0.14, 0.030, 0.34, 0.10, 0.26, 0.30, sides=5)
    shard(mb, "Node_Crystal", 0.12, 0.028, -0.28, 0.08, -0.30, 0.30, sides=5)
    return mb, node_mats((0.17, 0.16, 0.21), (0.6, 0.4, 1.0))


# -------------------------------------------------------- AncientVein
def build_ancientvein():
    """Gold shards growing through a cracked ruin-stone slab."""
    mb = MeshBuilder()
    # broken ruin-stone base slabs
    mb.add(translate(rotate(box((0.95, 0.16, 0.60)), 0, 0, 0.08),
                     (0.12, 0.095, 0.02)), "Node_Rock")
    mb.add(translate(rotate(box((0.45, 0.16, 0.55)), 0, 0, -0.15),
                     (-0.42, 0.115, -0.08)), "Node_Rock")
    mb.add(translate(rotate(box((0.30, 0.14, 0.35)), 0.10, 0, 0.20),
                     (0.55, 0.09, -0.22)), "Node_Rock")
    # rubble
    for r, h, x, z, s in ((0.16, 0.10, 0.62, 0.22, 9.1),
                          (0.13, 0.08, -0.58, 0.30, 4.4),
                          (0.11, 0.07, 0.05, 0.60, 7.7)):
        mb.add(translate(rock_base(r, h, seed=s, seg_u=10, seg_v=6),
                         (x, 0.0, z)), "Node_Rock")
    # 4 gold crystal shards growing through the cracks
    shard(mb, "Node_Crystal", 0.55, 0.080, 0.02, 0.15, 0.24, 0.10)
    shard(mb, "Node_Crystal", 0.42, 0.060, -0.25, 0.10, -0.05, 0.18)
    shard(mb, "Node_Crystal", 0.68, 0.090, 0.30, 0.08, -0.15, 0.15)
    shard(mb, "Node_Crystal", 0.35, 0.050, 0.62, 0.03, -0.22, 0.30)
    return mb, node_mats((0.5, 0.48, 0.45), (0.95, 0.8, 0.45), rock_rough=0.85)


def main() -> None:
    print(f"[nodes] output dir: {OUT_DIR}")
    jobs = [
        ("SM_Node_Astraite", build_astraite),
        ("SM_Node_Pyronite", build_pyronite),
        ("SM_Node_Voidstone", build_voidstone),
        ("SM_Node_AncientVein", build_ancientvein),
    ]
    total = 0
    for name, fn in jobs:
        mb, mats = fn()
        stats = save_asset(name, mats, mb)
        total += stats["triangles"]
    print(f"[nodes] done: 4 static meshes, {total} tris total")


if __name__ == "__main__":
    main()
