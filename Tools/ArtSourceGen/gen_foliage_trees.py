"""
ASTRAWILD ArtSourceGen — environment foliage: TREES (static meshes).

Bioluminescent sci-fi frontier flora (Glimmerwood-adjacent reads):
  * SM_Tree_Broadleaf    (7m)   — tapered lathe trunk, root flares, 4 branches,
                                  6 displaced canopy spheres, hanging glow pods
  * SM_Tree_Conifer      (8m)   — straight trunk, 6 stacked displaced cone tiers,
                                  snow-free teal-green canopy
  * SM_Tree_SporeCanopy  (5.5m) — signature Glimmerwood tree: dark twisted trunk,
                                  wide flat mushroom-cap canopy, hanging glow
                                  pods under the rim, faint glow dots on top

Conventions: meters, +Y up, origin at GROUND CONTACT (y=0), natural baked tilt.
Material slots: Foliage_Bark / Foliage_Canopy / Foliage_Glow — AwPipeline maps
these onto the M_Foliage_* PBR masters on import.

Run:  python3 gen_foliage_trees.py
Output: ArtSource/Meshes/Environment/SM_Tree_*.glb
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
from aw_shapes import (MeshBuilder, cylinder, lathe, sphere, displace,
                       recompute_smooth_normals, rotate, scale, translate)

OUT_DIR = os.path.abspath(os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "..", "..", "ArtSource", "Meshes", "Environment"))

TREE_BUDGET = 3000  # hard triangle cap for trees


# ------------------------------------------------------- deterministic noise
def wobble(amp: float, freq: float, seed: float):
    """Deterministic organic noise (sin/cos product) for displace()."""
    def fn(pos: np.ndarray) -> np.ndarray:
        return (amp
                * np.sin(freq * pos[:, 0] + seed)
                * np.sin(freq * 0.83 * pos[:, 1] + seed * 1.7 + 0.6)
                * np.cos(freq * 1.19 * pos[:, 2] + seed * 0.31 + 1.1))
    return fn


def rot_vec(v, rx: float = 0.0, ry: float = 0.0, rz: float = 0.0) -> np.ndarray:
    """Apply the same Euler-XYZ rotation the aw_shapes.rotate() helper uses."""
    m = quat_to_matrix(quat_from_euler(rx, ry, rz))
    return m @ np.asarray(v, dtype=float)


# ------------------------------------------------------------- shared plumbing
def tree_materials(bark_rgb=(0.32, 0.27, 0.22)):
    return (
        Material("Foliage_Bark", base_color=(bark_rgb[0], bark_rgb[1], bark_rgb[2], 1.0),
                 roughness=0.9),
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
               budget: int = TREE_BUDGET) -> dict:
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


# ===================================================================== broadleaf
def gen_broadleaf() -> dict:
    """SM_Tree_Broadleaf — 7m broadleaf, teal biolum canopy, glow spore pods."""
    name = "SM_Tree_Broadleaf"
    builder = GlbBuilder()
    for m in tree_materials():
        builder.add_material(m)
    mb = MeshBuilder()

    # --- trunk: tapered lathe r 0.22 -> 0.10 up to 5.4m, organic bark wobble
    prof = [(0.22, 0.0), (0.205, 0.5), (0.185, 1.6), (0.165, 2.8),
            (0.15, 3.8), (0.135, 4.6), (0.112, 5.05), (0.10, 5.4)]
    trunk = lathe(prof, radial=10, cap_top=True, cap_bottom=True)
    trunk = recompute_smooth_normals(displace(trunk, wobble(0.022, 5.0, 1.0)))
    mb.add(trunk, "Foliage_Bark")

    # --- 2 root flare bumps (short lathe cones at the base, leaned outward)
    for k, (yaw, lean) in enumerate(((0.7, 0.55), (2.45, 0.62))):
        flare = lathe([(0.17, 0.0), (0.045, 0.6)], radial=7, cap_top=True)
        flare = recompute_smooth_normals(displace(flare, wobble(0.03, 4.0, 3.0 + k)))
        flare = rotate(rotate(flare, lean, 0, 0), 0, yaw, 0)
        flare = translate(flare, (0.13 * math.sin(yaw), 0.0, 0.13 * math.cos(yaw)))
        mb.add(flare, "Foliage_Bark")

    # --- 4 tapered branches, leaned out 38-55 deg between 3.5 and 5.05 m
    branches = [  # (attach_y, yaw, tilt, length)
        (5.05, 0.35, 0.75, 1.80),
        (4.45, 1.95, 0.88, 1.60),
        (3.95, 3.60, 0.66, 1.50),
        (3.55, 5.05, 0.96, 1.35),
    ]
    tips = []
    for ay, yaw, tilt, ln in branches:
        br = lathe([(0.075, 0.0), (0.058, ln * 0.5), (0.04, ln)],
                   radial=7, cap_top=True)
        br = recompute_smooth_normals(displace(br, wobble(0.013, 6.5, yaw)))
        br = rotate(rotate(br, tilt, 0, 0), 0, yaw, 0)
        attach = (0.09 * math.sin(yaw), ay, 0.09 * math.cos(yaw))
        br = translate(br, attach)
        mb.add(br, "Foliage_Bark")
        tip = np.asarray(attach, dtype=float) + \
            rot_vec(rot_vec((0.0, ln * 0.96, 0.0), tilt, 0, 0), 0, yaw, 0)
        tips.append(tip)

    # --- canopy: 5 displaced spheres at the branch tips + 1 crown
    blob_specs = [(np.array([0.10, 5.90, -0.05]), 1.30)]          # crown
    tip_rs = [1.45, 1.25, 1.70, 1.15]                              # 1.1-1.7m
    for i, tip in enumerate(tips):
        blob_specs.append((tip * np.array([0.88, 0.93, 0.88]), tip_rs[i]))
    mid = (tips[0] + tips[1]) * 0.5                                 # 5th blob
    blob_specs.append((mid, 1.30))

    def canopy_blob(center: np.ndarray, r: float, seed: float):
        s = sphere(r, 14, 10)
        s = displace(s, wobble(0.11 * r, 2.6, seed))
        s = recompute_smooth_normals(s)
        s = scale(s, (1.0 + 0.10 * math.sin(seed),
                      0.80 + 0.08 * math.cos(seed * 1.3),
                      1.0 + 0.08 * math.sin(seed * 0.7)))
        s = recompute_smooth_normals(s)
        return translate(s, (float(center[0]), float(center[1]), float(center[2])))

    for i, (c, r) in enumerate(blob_specs):
        mb.add(canopy_blob(c, r, 2.0 + i * 1.37), "Foliage_Canopy")

    # --- 3 hanging glow pods on thin stems under the canopy
    pods = [  # (x, y, z, pod_r)
        (0.95, 4.72, 0.55, 0.095),
        (-1.30, 4.52, -0.45, 0.080),
        (0.25, 4.42, -1.30, 0.088),
    ]
    for px, py, pz, pr in pods:
        stem = cylinder(0.008, 0.32, radial=4, capped=True)
        mb.add(translate(stem, (px, py, pz)), "Foliage_Bark")
        pod = sphere(pr, 6, 4)
        pod = recompute_smooth_normals(displace(pod, wobble(0.012, 18.0, px)))
        mb.add(translate(pod, (px, py - 0.07, pz)), "Foliage_Glow")

    return save_asset(name, builder, mb)


# ====================================================================== conifer
def gen_conifer() -> dict:
    """SM_Tree_Conifer — 8m stacked-tier conifer, teal-green (snow-free)."""
    name = "SM_Tree_Conifer"
    builder = GlbBuilder()
    for m in tree_materials()[:2]:   # bark + canopy only (no glow parts)
        builder.add_material(m)
    mb = MeshBuilder()

    # --- straight trunk lathe r 0.16 -> 0.05 to 8.0m
    trunk = lathe([(0.16, 0.0), (0.152, 0.8), (0.138, 2.6), (0.12, 4.6),
                   (0.10, 6.2), (0.08, 7.4), (0.05, 8.0)],
                  radial=10, cap_top=True, cap_bottom=True)
    trunk = recompute_smooth_normals(displace(trunk, wobble(0.012, 5.0, 2.0)))
    mb.add(trunk, "Foliage_Bark")

    # --- 6 stacked cone tiers, r 1.6 -> 0.4, each displaced + tilted
    bases = [0.75, 1.90, 3.05, 4.20, 5.35, 6.50]
    hs = [1.70, 1.60, 1.50, 1.40, 1.30, 1.50]
    rs = [1.60, 1.35, 1.10, 0.85, 0.62, 0.42]
    for i, (by, h, r) in enumerate(zip(bases, hs, rs)):
        prof = [(r, 0.0), (r * 0.94, h * 0.20), (r * 0.55, h * 0.60), (0.0, h)]
        tier = lathe(prof, radial=14, cap_bottom=True)
        tier = displace(tier, wobble(0.05 + 0.04 * math.sin(i * 1.7), 2.4, 4.0 + i * 1.9))
        tier = recompute_smooth_normals(tier)
        tier = rotate(tier, 0.045 * math.sin(i * 2.3), 0.5 * i,
                      0.045 * math.cos(i * 1.7))
        tier = translate(tier, (0.06 * math.sin(i * 2.0), by, 0.06 * math.cos(i * 2.7)))
        mb.add(tier, "Foliage_Canopy")

    return save_asset(name, builder, mb)


# ================================================================ spore canopy
def gen_spore_canopy() -> dict:
    """SM_Tree_SporeCanopy — 5.5m signature Glimmerwood mushroom-cap glow tree."""
    name = "SM_Tree_SporeCanopy"
    builder = GlbBuilder()
    # near-black Glimmerwood bark (slot name stays the pipeline contract)
    for m in tree_materials(bark_rgb=(0.15, 0.14, 0.16)):
        builder.add_material(m)
    mb = MeshBuilder()

    # --- dark twisted trunk: 3 lathe segments offset at angles
    segs = [  # (r0, r1, h, rx, rz)
        (0.160, 0.125, 1.60, 0.10, 0.06),
        (0.125, 0.095, 1.50, -0.20, -0.14),
        (0.095, 0.055, 1.30, 0.26, -0.10),
    ]
    base = np.zeros(3)
    for i, (r0, r1, h, rx, rz) in enumerate(segs):
        last = (i == len(segs) - 1)
        seg = lathe([(r0, 0.0), (r0 * 0.9, h * 0.5), (r1, h)],
                    radial=8, cap_top=last, cap_bottom=(i == 0))
        seg = recompute_smooth_normals(displace(seg, wobble(0.02, 4.5, 7.0 + i)))
        seg = rotate(seg, rx, 0.0, rz)
        seg = translate(seg, (float(base[0]), float(base[1]), float(base[2])))
        mb.add(seg, "Foliage_Bark")
        base = base + rot_vec((0.0, h, 0.0), rx, 0.0, rz)

    canopy_c = base + np.array([0.0, 0.28, 0.0])   # canopy center ~y 4.55
    cap_r, cap_flat = 2.2, 0.45

    # --- WIDE flat low canopy: 4 flattened displaced spheres (mushroom cap)
    blobs = [
        (canopy_c, cap_r, (12, 9)),
        (canopy_c + np.array([1.05, -0.18, 0.45]), 1.55, (12, 9)),
        (canopy_c + np.array([-0.95, -0.12, -0.65]), 1.35, (10, 8)),
        (canopy_c + np.array([0.25, -0.30, 1.05]), 1.05, (9, 7)),
    ]
    for c, r, (su, sv) in blobs:
        s = sphere(r, su, sv)
        s = displace(s, wobble(0.13, 2.2, 1.7 + r))
        s = recompute_smooth_normals(s)
        s = scale(s, (1.0, cap_flat, 1.0))
        s = recompute_smooth_normals(s)
        s = translate(s, (float(c[0]), float(c[1]), float(c[2])))
        mb.add(s, "Foliage_Canopy")

    # --- 9 hanging glow pods under the canopy rim on thin stems
    ry = cap_r * cap_flat  # vertical radius of the cap
    for i in range(9):
        a = i * (2.0 * math.pi / 9.0) + 0.32 * math.sin(i * 1.7)
        rad = 1.45 + 0.35 * math.sin(i * 2.3 + 0.5)
        px = canopy_c[0] + rad * math.cos(a)
        pz = canopy_c[2] + rad * math.sin(a)
        rr = min(rad / cap_r, 0.98)
        y_surf = canopy_c[1] - ry * math.sqrt(max(0.0, 1.0 - rr * rr))
        pr = 0.07 + 0.05 * (0.5 + 0.5 * math.sin(i * 1.3))   # 0.07-0.12
        py = y_surf - 0.10 - pr                              # pod center below rim
        stem_h = (y_surf + 0.08) - (py + pr * 0.6)
        stem_h = min(max(stem_h, 0.10), 0.40)
        stem = cylinder(0.007, stem_h, radial=4, capped=True)
        mb.add(translate(stem, (px, py + pr * 0.6, pz)), "Foliage_Bark")
        pod = sphere(pr, 6, 4)
        pod = recompute_smooth_normals(displace(pod, wobble(0.01, 20.0, i * 1.1)))
        mb.add(translate(pod, (px, py, pz)), "Foliage_Glow")

    # --- faint glow dots on the canopy top
    for i in range(6):
        a = i * 1.05 + 0.4
        rad = 0.35 + 1.05 * (0.5 + 0.5 * math.sin(i * 1.9))
        rr = min(rad / cap_r, 0.95)
        y = canopy_c[1] + ry * math.sqrt(max(0.0, 1.0 - rr * rr)) + 0.015
        dot = sphere(0.03, 5, 3)
        mb.add(translate(dot, (canopy_c[0] + rad * math.cos(a), y,
                               canopy_c[2] + rad * math.sin(a))), "Foliage_Glow")

    return save_asset(name, builder, mb)


# ------------------------------------------------------------------------ main
def main() -> None:
    print(f"[gen_foliage_trees] output dir: {OUT_DIR}")
    stats = [gen_broadleaf(), gen_conifer(), gen_spore_canopy()]
    total_tris = sum(s["triangles"] for s in stats)
    total_bytes = sum(s["bytes"] for s in stats)
    print(f"[gen_foliage_trees] done: {len(stats)} trees, "
          f"{total_tris} tris total, {total_bytes} bytes total")


if __name__ == "__main__":
    main()
