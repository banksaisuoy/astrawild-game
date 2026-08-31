"""
ASTRAWILD ArtSourceGen — STATIC weapon meshes (5 families, Mk1..T5).

Deliverables -> ArtSource/Meshes/Weapons/:
  SM_Weapon_ScrapRifle         0.82m  Mk1 kinetic scrap rifle          amber glow
  SM_Weapon_PlasmaCarbine      0.78m  T2 plasma carbine                magenta glow
  SM_Weapon_ArcCannon          0.88m  T3 arc emitter, copper coils     cyan glow
  SM_Weapon_Railgun            1.05m  T4 dual-rail magnetic launcher   violet glow
  SM_Weapon_SingularityCannon  0.95m  T5 vortex cannon                 astra teal

Conventions: meters, +Y up, +Z forward. Weapon rests with GRIP at y~=0,
body along +Z, MUZZLE at max +Z (Muzzle socket documented in stats/manifest).
Material slots: Weapon_Body / Weapon_Metal / Weapon_Grip / Weapon_Glow
(+ Weapon_Coil copper, ArcCannon only). Triangle budget: <= 1500 each.

Run: python3 gen_weapons.py
"""
from __future__ import annotations

import math
import os
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from aw_gltf import GlbBuilder, Material, validate_glb
from aw_manifest import record
from aw_shapes import (MeshBuilder, box, cone, crystal, cylinder, lathe,
                       rotate, torus, translate, tube)

OUT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__),
                          "..", "..", "ArtSource", "Meshes", "Weapons"))
MAX_TRIS = 1500


# ----------------------------------------------------------------- helpers
def zcyl(r, h, radial=10, capped=True, z0=0.0, y=0.0):
    """Cylinder along +Z (base at z0)."""
    return translate(rotate(cylinder(r, h, radial, capped), math.pi / 2, 0, 0),
                     (0.0, y, z0))


def ztube(outer_r, inner_r, h, radial=10, z0=0.0, y=0.0):
    """Annulus prism along +Z (shroud)."""
    return translate(rotate(tube(outer_r, inner_r, h, radial), math.pi / 2, 0, 0),
                     (0.0, y, z0))


def ztorus(R, r, seg_u=12, seg_v=6, z0=0.0, y=0.0):
    """Torus ring around the +Z axis."""
    return translate(rotate(torus(R, r, seg_u, seg_v), math.pi / 2, 0, 0),
                     (0.0, y, z0))


def xcyl(r, h, radial=8, capped=True):
    """Cylinder along X (base at x=0, extends to x=-h; translate to place)."""
    return rotate(cylinder(r, h, radial, capped), 0, 0, math.pi / 2)


def radial_cone(r, h, radial_seg, base_radius, az, tilt, z0, y=0.0):
    """Cone pointing +Z, tilted `tilt` outward at azimuth `az`, base ring."""
    m = rotate(cone(r, h, radial_seg), math.pi / 2, 0, 0)   # +Y -> +Z
    m = rotate(m, 0, tilt, 0)                               # lean toward +X
    m = rotate(m, 0, 0, az)                                 # swing to azimuth
    return translate(m, (base_radius * math.cos(az),
                         y + base_radius * math.sin(az), z0))


def radial_spike(height, r, sides, base_radius, az, tilt, z0, y=0.0):
    """Crystal shard pointing +Z, tilted outward at azimuth `az`."""
    m = rotate(crystal(height, r, sides), math.pi / 2, 0, 0)
    m = rotate(m, 0, tilt, 0)
    m = rotate(m, 0, 0, az)
    return translate(m, (base_radius * math.cos(az),
                         y + base_radius * math.sin(az), z0))


def weapon_mats(body_rgb, glow_rgb, extra=()):
    mats = [
        Material("Weapon_Body", base_color=(body_rgb[0], body_rgb[1], body_rgb[2], 1.0),
                 metallic=0.75, roughness=0.5),
        Material("Weapon_Metal", base_color=(0.2, 0.22, 0.25, 1.0),
                 metallic=0.9, roughness=0.35),
        Material("Weapon_Grip", base_color=(0.16, 0.15, 0.14, 1.0),
                 metallic=0.05, roughness=0.85),
        Material("Weapon_Glow", base_color=(0.02, 0.02, 0.02, 1.0),
                 metallic=0.0, roughness=0.3, emissive=glow_rgb),
    ]
    return mats + list(extra)


def save_asset(name, mats, mb, muzzle, budget=MAX_TRIS):
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
    stats["sockets"] = [{"name": "Muzzle",
                         "pos": [round(v, 4) for v in muzzle],
                         "rot": [0, 0, 0]}]
    stats["asset_type"] = "static_mesh"
    stats["ue_path"] = "/Game/Weapons/Meshes/" + name
    record("mesh", name, stats)

    pos = np.concatenate([m.positions for m, _ in mb.parts])
    lo, hi = pos.min(axis=0), pos.max(axis=0)
    print(f"[weapon] {name}: tris={stats['triangles']} bytes={stats['bytes']} "
          f"validate={stats['validate']}")
    print(f"         bbox y[{lo[1]:+.3f}..{hi[1]:+.3f}] z[{lo[2]:+.3f}..{hi[2]:+.3f}] "
          f"len_z={hi[2] - lo[2]:.3f}m  muzzle={stats['sockets'][0]['pos']}")
    if problems:
        raise SystemExit(1)
    if stats["triangles"] > budget:
        raise SystemExit(f"{name}: {stats['triangles']} tris > budget {budget}")
    return stats


# ===================================================== 1. ScrapRifle (Mk1)
def build_scraprifle():
    """Boxy scrap-built kinetic rifle, drum magazine, amber glow."""
    mb = MeshBuilder()
    y = 0.105  # barrel axis height
    # receiver + top rail
    mb.add(translate(box((0.072, 0.11, 0.28)), (0, y, 0.0)), "Weapon_Body")
    mb.add(translate(box((0.05, 0.016, 0.26)), (0, 0.168, -0.01)), "Weapon_Metal")
    for z in (-0.09, -0.03, 0.03, 0.09):  # rail notches (greebles)
        mb.add(translate(box((0.054, 0.008, 0.018)), (0, 0.180, z)), "Weapon_Metal")
    # barrel + shroud + clamp + muzzle brake
    mb.add(zcyl(0.022, 0.35, 10, z0=0.14, y=y), "Weapon_Metal")
    mb.add(ztube(0.032, 0.025, 0.16, 10, z0=0.16, y=y), "Weapon_Metal")
    mb.add(zcyl(0.036, 0.02, 10, z0=0.30, y=y), "Weapon_Metal")
    mb.add(zcyl(0.031, 0.07, 10, z0=0.49, y=y), "Weapon_Metal")
    # angled stock + butt pad
    mb.add(translate(rotate(box((0.045, 0.11, 0.20)), -0.20, 0, 0),
                     (0, 0.088, -0.148)), "Weapon_Body")
    mb.add(translate(rotate(box((0.05, 0.115, 0.025)), -0.20, 0, 0),
                     (0, 0.088, -0.240)), "Weapon_Grip")
    # side drum magazine + hub + feed chute
    mb.add(translate(xcyl(0.055, 0.05, 12), (0.055, 0.04, 0.02)), "Weapon_Metal")
    mb.add(translate(xcyl(0.02, 0.03, 8), (0.085, 0.04, 0.02)), "Weapon_Grip")
    mb.add(translate(box((0.035, 0.05, 0.07)), (0.045, 0.06, 0.02)), "Weapon_Metal")
    # grip + trigger
    mb.add(translate(rotate(box((0.042, 0.11, 0.055)), -0.22, 0, 0),
                     (0, 0.055, 0.045)), "Weapon_Grip")
    mb.add(translate(box((0.036, 0.028, 0.006)), (0, 0.032, 0.068)), "Weapon_Metal")
    mb.add(translate(box((0.036, 0.006, 0.078)), (0, 0.019, 0.030)), "Weapon_Metal")
    mb.add(translate(box((0.036, 0.026, 0.006)), (0, 0.030, -0.008)), "Weapon_Metal")
    mb.add(translate(rotate(box((0.008, 0.028, 0.012)), 0.25, 0, 0),
                     (0, 0.048, 0.032)), "Weapon_Metal")
    # iron sights + amber dot
    mb.add(translate(box((0.012, 0.035, 0.012)), (0, 0.198, 0.10)), "Weapon_Metal")
    mb.add(translate(box((0.009, 0.009, 0.006)), (0, 0.214, 0.106)), "Weapon_Glow")
    mb.add(translate(box((0.048, 0.024, 0.02)), (0, 0.192, -0.10)), "Weapon_Metal")
    # 2 bolts (right side)
    for z in (-0.05, 0.07):
        mb.add(translate(xcyl(0.009, 0.014, 6), (0.044, 0.135, z)), "Weapon_Metal")
    # amber ammo strip + left vents + charging handle
    mb.add(translate(box((0.005, 0.02, 0.15)), (0.038, 0.082, -0.02)), "Weapon_Glow")
    for z in (-0.03, 0.02, 0.07):
        mb.add(translate(box((0.006, 0.028, 0.045)), (-0.037, y, z)), "Weapon_Metal")
    mb.add(translate(box((0.022, 0.018, 0.03)), (0.052, 0.15, -0.03)), "Weapon_Metal")
    return mb, (0.0, y, 0.56)


# ================================================= 2. PlasmaCarbine (T2)
def build_plasmacarbine():
    """Sleek polymer-shell carbine, cooling fins, magenta energy cell."""
    mb = MeshBuilder()
    y = 0.105
    # polymer shell
    mb.add(translate(box((0.062, 0.10, 0.28)), (0, y, -0.04)), "Weapon_Body")
    mb.add(translate(box((0.058, 0.03, 0.20)), (0, 0.167, -0.06)), "Weapon_Body")
    mb.add(translate(box((0.05, 0.075, 0.10)), (0, 0.103, 0.13)), "Weapon_Body")
    # barrel + emitter + muzzle
    mb.add(zcyl(0.024, 0.24, 10, z0=0.16, y=y), "Weapon_Metal")
    for z in (0.20, 0.26, 0.32):  # 3 cooling fins
        mb.add(translate(box((0.075, 0.018, 0.016)), (0, y, z)), "Weapon_Metal")
    mb.add(ztube(0.03, 0.026, 0.10, 10, z0=0.36, y=y), "Weapon_Metal")
    mb.add(zcyl(0.018, 0.09, 8, z0=0.46, y=y), "Weapon_Metal")
    mb.add(zcyl(0.021, 0.012, 8, z0=0.545, y=y), "Weapon_Glow")
    # stock + glowing energy cell inserted in the stock
    mb.add(translate(box((0.04, 0.06, 0.06)), (0, 0.095, -0.15)), "Weapon_Body")
    mb.add(translate(box((0.045, 0.08, 0.08)), (0, 0.09, -0.18)), "Weapon_Grip")
    mb.add(translate(xcyl(0.02, 0.055, 10), (0.065, 0.09, -0.17)), "Weapon_Glow")
    mb.add(translate(xcyl(0.027, 0.014, 8), (0.072, 0.09, -0.17)), "Weapon_Metal")
    mb.add(translate(box((0.018, 0.045, 0.045)), (-0.028, 0.09, -0.17)), "Weapon_Metal")
    # curved foregrip (two swept segments)
    mb.add(translate(rotate(box((0.04, 0.07, 0.05)), 0.30, 0, 0),
                     (0, 0.030, 0.14)), "Weapon_Grip")
    mb.add(translate(rotate(box((0.038, 0.07, 0.05)), 0.65, 0, 0),
                     (0, 0.035, 0.19)), "Weapon_Grip")
    # pistol grip + trigger
    mb.add(translate(rotate(box((0.04, 0.10, 0.05)), -0.25, 0, 0),
                     (0, 0.05, -0.10)), "Weapon_Grip")
    mb.add(translate(box((0.036, 0.028, 0.006)), (0, 0.032, -0.035)), "Weapon_Metal")
    mb.add(translate(box((0.036, 0.006, 0.075)), (0, 0.019, -0.072)), "Weapon_Metal")
    mb.add(translate(box((0.036, 0.026, 0.006)), (0, 0.030, -0.110)), "Weapon_Metal")
    mb.add(translate(rotate(box((0.008, 0.028, 0.012)), 0.25, 0, 0),
                     (0, 0.048, -0.075)), "Weapon_Metal")
    # low-profile scope
    mb.add(translate(box((0.036, 0.032, 0.13)), (0, 0.205, -0.06)), "Weapon_Body")
    mb.add(translate(box((0.03, 0.024, 0.006)), (0, 0.205, 0.008)), "Weapon_Glow")
    mb.add(translate(box((0.032, 0.026, 0.02)), (0, 0.205, -0.135)), "Weapon_Metal")
    for z in (-0.10, -0.02):
        mb.add(translate(box((0.02, 0.02, 0.03)), (0, 0.183, z)), "Weapon_Metal")
    # energy port strip + vents
    mb.add(translate(box((0.005, 0.014, 0.10)), (0.033, 0.10, 0.0)), "Weapon_Glow")
    for z in (-0.06, 0.0):
        mb.add(translate(box((0.005, 0.024, 0.05)), (-0.033, 0.10, z)), "Weapon_Metal")
    return mb, (0.0, y, 0.56)


# =================================================== 3. ArcCannon (T3)
def build_arccannon():
    """Heavy emitter: copper coils, 4 tesla prongs, side cable, cyan glow."""
    mb = MeshBuilder()
    y = 0.115
    # housing + capacitor + rear power feed
    mb.add(translate(box((0.095, 0.14, 0.26)), (0, y, -0.12)), "Weapon_Body")
    mb.add(translate(box((0.06, 0.05, 0.12)), (0, 0.19, -0.15)), "Weapon_Metal")
    mb.add(zcyl(0.035, 0.09, 10, z0=-0.34, y=y), "Weapon_Metal")
    # barrel core
    mb.add(zcyl(0.03, 0.36, 12, z0=0.01, y=y), "Weapon_Metal")
    # 3 copper coil rings, growing toward the muzzle + glow sleeves
    for z, R in ((0.13, 0.05), (0.22, 0.06), (0.31, 0.07)):
        mb.add(ztorus(R, 0.014, 12, 6, z0=z, y=y), "Weapon_Coil")
        mb.add(zcyl(0.032, 0.02, 10, z0=z - 0.01, y=y), "Weapon_Glow")
    # prong base ring + 4 tesla prongs + glowing tips
    mb.add(zcyl(0.05, 0.06, 12, z0=0.37, y=y), "Weapon_Metal")
    for i in range(4):
        az = math.radians(45 + 90 * i)
        mb.add(radial_cone(0.014, 0.11, 6, 0.05, az, 0.24, 0.42, y=y), "Weapon_Metal")
        mb.add(radial_cone(0.007, 0.032, 5, 0.076, az, 0.24, 0.525, y=y), "Weapon_Glow")
    # cable tube along the left side + clamps + rear socket
    mb.add(translate(rotate(tube(0.013, 0.009, 0.30, 8), math.pi / 2, 0, 0),
                     (-0.058, 0.05, -0.24)), "Weapon_Grip")
    for z in (-0.18, 0.0):
        mb.add(translate(box((0.024, 0.03, 0.03)), (-0.056, 0.075, z)), "Weapon_Metal")
    mb.add(translate(box((0.03, 0.03, 0.04)), (-0.056, 0.05, -0.26)), "Weapon_Metal")
    # pistol grip + forward brace + strut
    mb.add(translate(rotate(box((0.042, 0.11, 0.055)), -0.20, 0, 0),
                     (0, 0.055, 0.03)), "Weapon_Grip")
    mb.add(translate(rotate(box((0.035, 0.09, 0.04)), 0.12, 0, 0),
                     (0, 0.05, 0.18)), "Weapon_Grip")
    mb.add(translate(rotate(box((0.03, 0.14, 0.025)), -0.60, 0, 0),
                     (0, 0.10, 0.10)), "Weapon_Metal")
    # trigger
    mb.add(translate(box((0.036, 0.028, 0.006)), (0, 0.032, 0.065)), "Weapon_Metal")
    mb.add(translate(box((0.036, 0.006, 0.075)), (0, 0.019, 0.028)), "Weapon_Metal")
    mb.add(translate(rotate(box((0.008, 0.028, 0.012)), 0.25, 0, 0),
                     (0, 0.048, 0.03)), "Weapon_Metal")
    # vents + bolts
    for z in (-0.18, -0.10, -0.02):
        mb.add(translate(box((0.006, 0.03, 0.05)), (-0.049, y, z)), "Weapon_Metal")
    for z in (-0.16, -0.04):
        mb.add(translate(xcyl(0.009, 0.014, 6), (0.053, 0.135, z)), "Weapon_Metal")
    return mb, (0.0, y, 0.56)


# ==================================================== 4. Railgun (T4)
def build_railgun():
    """Long dual-rail barrel, big magnetic housing, tall scope, violet glow."""
    mb = MeshBuilder()
    y = 0.125
    # magnetic housing (bigger than most) + top plate
    mb.add(translate(box((0.11, 0.16, 0.28)), (0, y, -0.19)), "Weapon_Body")
    mb.add(translate(box((0.09, 0.03, 0.22)), (0, 0.222, -0.19)), "Weapon_Metal")
    # dual rails + 5 spacer blocks
    for sx in (-1, 1):
        mb.add(translate(box((0.03, 0.05, 0.55)), (sx * 0.030, y, 0.225)), "Weapon_Metal")
        mb.add(translate(box((0.03, 0.006, 0.50)), (sx * 0.030, 0.153, 0.22)), "Weapon_Body")
    for z in (0.02, 0.125, 0.23, 0.335, 0.44):
        mb.add(translate(box((0.088, 0.06, 0.028)), (0, y, z)), "Weapon_Body")
    # muzzle ring + charge glow
    mb.add(translate(box((0.10, 0.07, 0.05)), (0, y, 0.525)), "Weapon_Metal")
    mb.add(translate(box((0.05, 0.04, 0.012)), (0, y, 0.548)), "Weapon_Glow")
    # underslung battery + violet strips + terminals
    mb.add(translate(box((0.075, 0.085, 0.20)), (0, 0.045, -0.14)), "Weapon_Metal")
    for sx in (-1, 1):
        mb.add(translate(box((0.004, 0.02, 0.15)), (sx * 0.039, 0.045, -0.14)), "Weapon_Glow")
        mb.add(translate(xcyl(0.008, 0.022, 6), (sx * 0.049, 0.045, -0.23)), "Weapon_Metal")
    # stock + butt pad + cheek rest
    mb.add(translate(box((0.05, 0.11, 0.16)), (0, 0.095, -0.39)), "Weapon_Body")
    mb.add(translate(box((0.055, 0.12, 0.025)), (0, 0.09, -0.478)), "Weapon_Grip")
    mb.add(translate(box((0.03, 0.035, 0.11)), (0, 0.168, -0.40)), "Weapon_Body")
    mb.add(translate(box((0.034, 0.012, 0.12)), (0, 0.19, -0.40)), "Weapon_Grip")
    # tall scope
    mb.add(zcyl(0.022, 0.20, 10, z0=-0.24, y=0.28), "Weapon_Body")
    mb.add(zcyl(0.03, 0.03, 10, z0=-0.04, y=0.28), "Weapon_Body")
    mb.add(zcyl(0.028, 0.025, 10, z0=-0.265, y=0.28), "Weapon_Body")
    mb.add(zcyl(0.026, 0.005, 10, z0=-0.013, y=0.28), "Weapon_Glow")
    for z in (-0.20, -0.08):
        mb.add(translate(box((0.025, 0.05, 0.035)), (0, 0.245, z)), "Weapon_Metal")
    # grip + trigger
    mb.add(translate(rotate(box((0.042, 0.11, 0.055)), -0.22, 0, 0),
                     (0, 0.055, -0.16)), "Weapon_Grip")
    mb.add(translate(box((0.036, 0.028, 0.006)), (0, 0.032, -0.125)), "Weapon_Metal")
    mb.add(translate(box((0.036, 0.006, 0.075)), (0, 0.019, -0.162)), "Weapon_Metal")
    mb.add(translate(rotate(box((0.008, 0.028, 0.012)), 0.25, 0, 0),
                     (0, 0.048, -0.165)), "Weapon_Metal")
    # housing vents + bolts + battery conduit
    for sx in (-1, 1):
        for z in (-0.25, -0.13):
            mb.add(translate(box((0.006, 0.04, 0.06)), (sx * 0.056, y, z)), "Weapon_Metal")
        for zy in ((0.07, -0.26), (0.07, -0.08)):
            mb.add(translate(xcyl(0.009, 0.014, 6), (sx * 0.056, zy[0], zy[1])), "Weapon_Metal")
    mb.add(translate(box((0.018, 0.05, 0.018)), (0.032, 0.055, -0.27)), "Weapon_Metal")
    return mb, (0.0, y, 0.56)


# ============================================ 5. SingularityCannon (T5)
def build_singularitycannon():
    """Thick core, vortex ring + 6 emitter spikes at the muzzle, teal glow."""
    mb = MeshBuilder()
    y = 0.11
    # thick cylindrical core + front cap
    mb.add(zcyl(0.07, 0.47, 12, z0=-0.05, y=y), "Weapon_Body")
    mb.add(zcyl(0.062, 0.05, 12, z0=0.42, y=y), "Weapon_Metal")
    # vortex ring at the muzzle (glow) + core windows (glow)
    mb.add(ztorus(0.09, 0.03, 12, 7, z0=0.46, y=y), "Weapon_Glow")
    for z in (0.08, 0.24):
        mb.add(ztube(0.072, 0.066, 0.03, 12, z0=z, y=y), "Weapon_Glow")
    # 6 emitter spikes around the muzzle ring
    for i in range(6):
        az = math.radians(60 * i)
        mb.add(radial_spike(0.12, 0.015, 4, 0.100, az, 0.38, 0.43, y=y), "Weapon_Metal")
    # collar + shoulder stock with vent fins + butt pad
    mb.add(zcyl(0.052, 0.16, 12, z0=-0.21, y=y), "Weapon_Body")
    mb.add(translate(box((0.06, 0.12, 0.17)), (0, 0.10, -0.295)), "Weapon_Body")
    for sx in (-1, 1):
        for z in (-0.36, -0.27):
            mb.add(translate(box((0.006, 0.09, 0.10)), (sx * 0.034, 0.10, z)), "Weapon_Metal")
    mb.add(translate(box((0.065, 0.13, 0.025)), (0, 0.095, -0.385)), "Weapon_Grip")
    # top handle
    mb.add(translate(box((0.045, 0.018, 0.18)), (0, 0.205, -0.08)), "Weapon_Metal")
    for z in (-0.15, -0.01):
        mb.add(translate(box((0.035, 0.035, 0.025)), (0, 0.185, z)), "Weapon_Metal")
    # grip + trigger
    mb.add(translate(rotate(box((0.044, 0.11, 0.06)), -0.22, 0, 0),
                     (0, 0.055, -0.10)), "Weapon_Grip")
    mb.add(translate(box((0.036, 0.028, 0.006)), (0, 0.032, -0.075)), "Weapon_Metal")
    mb.add(translate(box((0.036, 0.006, 0.072)), (0, 0.019, -0.108)), "Weapon_Metal")
    mb.add(translate(rotate(box((0.008, 0.028, 0.012)), 0.25, 0, 0),
                     (0, 0.048, -0.105)), "Weapon_Metal")
    # collar bolts + core side glow strip + ammo port
    for sx in (-1, 1):
        for z in (-0.18, -0.08):
            mb.add(translate(xcyl(0.01, 0.016, 6), (sx * 0.053, 0.11, z)), "Weapon_Metal")
    mb.add(translate(box((0.005, 0.02, 0.10)), (0.071, y, 0.0)), "Weapon_Glow")
    mb.add(translate(box((0.03, 0.03, 0.045)), (0.048, 0.165, -0.03)), "Weapon_Metal")
    return mb, (0.0, y, 0.55)


# --------------------------------------------------------------------- main
def main() -> None:
    print(f"[weapons] output dir: {OUT_DIR}")
    jobs = [
        ("SM_Weapon_ScrapRifle", build_scraprifle,
         weapon_mats((0.5, 0.45, 0.42), (0.95, 0.60, 0.20))),
        ("SM_Weapon_PlasmaCarbine", build_plasmacarbine,
         weapon_mats((0.4, 0.42, 0.46), (0.80, 0.30, 0.90))),
        ("SM_Weapon_ArcCannon", build_arccannon,
         weapon_mats((0.40, 0.37, 0.34), (0.20, 0.85, 0.90),
                     extra=[Material("Weapon_Coil",
                                    base_color=(0.72, 0.45, 0.28, 1.0),
                                    metallic=0.9, roughness=0.35)])),
        ("SM_Weapon_Railgun", build_railgun,
         weapon_mats((0.45, 0.47, 0.50), (0.60, 0.40, 1.00))),
        ("SM_Weapon_SingularityCannon", build_singularitycannon,
         weapon_mats((0.35, 0.38, 0.42), (0.29, 0.86, 0.78))),
    ]
    total = 0
    for name, fn, mats in jobs:
        mb, muzzle = fn()
        stats = save_asset(name, mats, mb, muzzle)
        total += stats["triangles"]
    print(f"[weapons] done: 5 static meshes, {total} tris total")


if __name__ == "__main__":
    main()
