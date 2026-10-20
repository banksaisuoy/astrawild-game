"""
ASTRAWILD ArtSourceGen — SK_Echo_Lumewisp (Light Echo, starter lantern spirit).

Original ASTRAWILD creature: the Lumewisp — THE starter companion (MQ-02 "A
Friend in the Fields", Dawn Fields): the first Echo every player bonds with.
Canon: Light element, Spirit family (translucent veil surface), Floating body
plan, Tiny size class (runtime BodyScaleForSize 0.45 on top; authored at
~0.65x of the hero Small floater base per CREATURE_VISUAL_STRATEGY §7 — the
smallest authored echo mesh).

Design: a small lantern-core spirit — an organic seed-pod teardrop whose husk
is an open lattice (9 vertical ribs + 3 band rings) so the warm-ivory Light
membrane inside glows through the openings; a stem collar with a floating
halo ring above; 3 drifting petal-wisp fins (2 side petals on 2-bone chains
+ 1 dorsal petal); a short trailing mote ribbon below (2 segments + 3 motes);
two bright close-set eyes in the front lattice gap. NO legs.

Rig: Root + Body + Halo + Petal_L_01/02 + Petal_R_01/02 + PetalTop +
Ribbon_01/02. 10 bones.
Anims: AM_Lumewisp_Idle (3.6s gentle hover bob + petal ripple + halo drift),
AM_Lumewisp_Move (1.2s glide: petals flutter alternating, body pitch lean,
ribbon wave), AM_Lumewisp_Hit (0.35s petals tuck, halo drops, body dips).
Material slots: Echo_Body / Echo_Armor / Echo_Emissive / Echo_Eye.

Run:  python3 gen_echo_lumewisp.py
Output: ArtSource/Meshes/Echoes/SK_Echo_Lumewisp.glb
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
OUT_PATH = os.path.join(OUT_DIR, "SK_Echo_Lumewisp.glb")
SPECIES = "Lumewisp"
DEG = math.pi / 180.0

# ----------------------------------------------------------------- materials
BODY = (0.90, 0.87, 0.74, 1.0)       # pale ivory seed husk (Spirit veil)
ARMOR = (0.72, 0.66, 0.50, 1.0)      # pale gold-grey spar/collar
IVORY = (1.00, 0.92, 0.62)           # Light warm-IVORY emissive (story richer)

MATS = {
    "Echo_Body":     Material("Echo_Body", base_color=BODY, roughness=0.7, metallic=0.05),
    "Echo_Armor":    Material("Echo_Armor", base_color=ARMOR, roughness=0.45, metallic=0.45),
    "Echo_Emissive": Material("Echo_Emissive", base_color=(0.06, 0.05, 0.02, 1.0),
                              roughness=0.4, metallic=0.0, emissive=IVORY),
    "Echo_Eye":      Material("Echo_Eye", base_color=(0.08, 0.07, 0.03, 1.0),
                              roughness=0.25, metallic=0.0,
                              emissive=(1.0, 0.97, 0.80)),
}


# ------------------------------------------------------------- local helpers
def _align_y(m, ax: float, ay: float, az: float):
    """Rotates a Y-aligned shape so its +Y axis points along (ax, ay, az)."""
    n = math.sqrt(ax * ax + ay * ay + az * az)
    if n < 1e-9:
        return m
    ax, ay, az = ax / n, ay / n, az / n
    rx = math.asin(max(-1.0, min(1.0, az)))
    rz = math.atan2(-ax, ay)
    return rotate(m, rx, 0.0, rz)


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
# World anchors (creature faces +Z, floats with root at y=0.62).
ROOT_Y = 0.62
BODY_TOP = (0.0, 0.74, 0.0)            # Body bone bind (at root + 0.12)
POD_CENTER = (0.0, 0.64, 0.0)          # seed-pod membrane center

PETAL_L_BASE = np.array((-0.048, 0.742, -0.03))
PETAL_R_BASE = np.array((0.048, 0.742, -0.03))
PETAL_L_END1 = PETAL_L_BASE + np.array((-0.055, -0.062, -0.019))
PETAL_L_END2 = PETAL_L_END1 + np.array((-0.059, -0.043, -0.014))
PETAL_TOP_BASE = np.array((0.0, 0.765, -0.05))
PETAL_TOP_END = PETAL_TOP_BASE + np.array((0.0, 0.045, -0.085))

RIBBON_BASE = np.array((0.0, 0.545, -0.075))
RIBBON_MID = RIBBON_BASE + np.array((0.0, -0.040, -0.095))
RIBBON_END = RIBBON_MID + np.array((0.0, -0.030, -0.095))
HALO_POS = np.array((0.0, 0.845, 0.0))
HALO_BIND = np.array((0.0, 0.855, 0.0))


def build_rig(builder: GlbBuilder) -> Rig:
    rig = Rig(builder, root_name="Root", root_position=(0, ROOT_Y, 0))
    top = np.array(BODY_TOP)
    rig.add_bone("Body", "Root", tuple(top - np.array((0, ROOT_Y, 0))),
                 direction=(0, -1, 0.12), length=0.24)
    # side petal chains (2 bones each for the ripple bend) — anchored at the
    # stem collar, flared out-down clear of the pod bulge so the husk ribs
    # keep their Body binding (V25-C1 skin-binding QA lesson)
    for base, end1, tag in ((PETAL_L_BASE, PETAL_L_END1, "L"),
                            (PETAL_R_BASE, np.array((0.103, 0.680, -0.049)), "R")):
        s = -1.0 if tag == "L" else 1.0
        d1 = end1 - base
        rig.add_bone(f"Petal_{tag}_01", "Body", tuple(base - top),
                     direction=tuple(d1), length=float(np.linalg.norm(d1)))
        d2 = np.array((s * 0.075, -0.055, -0.018))
        rig.add_bone(f"Petal_{tag}_02", f"Petal_{tag}_01", tuple(d2 / np.linalg.norm(d2) * 0.078),
                     direction=tuple(d2), length=float(np.linalg.norm(d2)))
    # dorsal petal (single bone, sways behind the stem)
    rig.add_bone("PetalTop", "Body", tuple(PETAL_TOP_BASE - top),
                 direction=tuple(PETAL_TOP_END - PETAL_TOP_BASE),
                 length=float(np.linalg.norm(PETAL_TOP_END - PETAL_TOP_BASE)))
    # trailing mote ribbon (2-bone chain below the pod tip)
    rig.add_bone("Ribbon_01", "Body", tuple(RIBBON_BASE - top),
                 direction=tuple(RIBBON_MID - RIBBON_BASE),
                 length=float(np.linalg.norm(RIBBON_MID - RIBBON_BASE)))
    rig.add_bone("Ribbon_02", "Ribbon_01",
                 tuple((RIBBON_END - RIBBON_MID) / np.linalg.norm(RIBBON_END - RIBBON_MID) * 0.10),
                 direction=tuple(RIBBON_END - RIBBON_MID),
                 length=float(np.linalg.norm(RIBBON_END - RIBBON_MID)))
    # floating halo above the stem (segment parked above the stem tip so the
    # stem tip cone stays with Body — halo flexes at the stem's mid)
    rig.add_bone("Halo", "Body", tuple(HALO_BIND - top),
                 direction=(0, 1, 0), length=0.04)
    return rig


def build_body(mb: MeshBuilder) -> None:
    T = translate

    # --- inner Light membrane: organic teardrop seed-pod core (glows through
    #     the lattice openings of the husk)
    prof = [(0.018, 0.115), (0.055, 0.09), (0.088, 0.045), (0.098, 0.0),
            (0.085, -0.05), (0.055, -0.09), (0.0, -0.115)]
    mb.add(T(lathe(prof, 12, smooth=True), POD_CENTER), "Echo_Emissive")

    # --- husk lattice: 9 vertical ribs (skip the front gap where the eyes sit)
    for k in range(9):
        a = math.radians(20.0 + 40.0 * k)
        r = 0.094
        rib = blade(0.21, 0.02, 0.009, curve=0.014, segs=3)
        rib = rotate(rib, 0.0, a, 0.0)
        mb.add(T(rib, (r * math.sin(a), 0.535, r * math.cos(a))), "Echo_Body")
    # --- 3 husk band rings (follow the teardrop bulge)
    for (y, r) in ((0.71, 0.082), (0.64, 0.104), (0.57, 0.088)):
        mb.add(T(tube(r, r - 0.014, 0.016, 9, 1), (0, y, 0)), "Echo_Body")
    # --- bottom root-point crystal (the pod's seed tip glow)
    mb.add(T(rotate(crystal(0.055, 0.02, 5), math.pi, 0, 0), (0, 0.50, 0)),
           "Echo_Emissive")

    # --- stem collar + stem (Armor spars on top of the pod; short so the
    #     tip stays nearer Body than Halo)
    mb.add(T(tube(0.03, 0.02, 0.03, 8, 1), (0, 0.745, 0)), "Echo_Armor")
    mb.add(T(cylinder(0.011, 0.045, 6), (0, 0.7775, 0)), "Echo_Armor")
    mb.add(T(cone(0.013, 0.032, 6), (0, 0.813, 0)), "Echo_Armor")

    # --- 3 petal-wisp fins: side petals (2-bone blades) + dorsal petal
    for base, end2, spar_end in ((PETAL_L_BASE, PETAL_L_END2, PETAL_L_END1),
                                 (PETAL_R_BASE, np.array((0.161, 0.637, -0.063)),
                                  np.array((0.103, 0.680, -0.049)))):
        mb.add(_blade_between(base, end2, 0.06, 0.009, 0.03, 6), "Echo_Body")
        # petal spar (Armor midrib on the inner half)
        d = np.asarray(spar_end) - np.asarray(base)
        spar = _align_y(box((0.012, float(np.linalg.norm(d)), 0.016)), d[0], d[1], d[2])
        mb.add(T(spar, tuple(np.asarray(base) + d * 0.5)), "Echo_Armor")
    mb.add(_blade_between(PETAL_TOP_BASE, PETAL_TOP_END, 0.05, 0.008, 0.025, 5),
           "Echo_Body")

    # --- halo ring + 2 orbiter motes above the stem
    mb.add(T(torus(0.05, 0.006, 10, 4), tuple(HALO_POS)), "Echo_Emissive")
    for ang in (40.0, 220.0):
        a = math.radians(ang)
        mb.add(T(sphere(0.011, 6, 3),
                 (HALO_POS[0] + 0.062 * math.cos(a), HALO_POS[1] + 0.008,
                  HALO_POS[2] + 0.062 * math.sin(a))), "Echo_Emissive")

    # --- trailing mote ribbon (thin veil strip + 3 motes)
    mb.add(T(_align_y(box((0.03, 0.012, 0.105)), *(RIBBON_MID - RIBBON_BASE)),
             tuple((RIBBON_BASE + RIBBON_MID) * 0.5)), "Echo_Body")
    mb.add(T(_align_y(box((0.024, 0.010, 0.105)), *(RIBBON_END - RIBBON_MID)),
             tuple((RIBBON_MID + RIBBON_END) * 0.5)), "Echo_Body")
    for k, t in enumerate((0.35, 0.7, 1.05)):
        p = RIBBON_BASE + (RIBBON_END - RIBBON_BASE) * t
        mb.add(T(sphere(0.010 + 0.002 * (1 - k), 6, 3), tuple(p)), "Echo_Emissive")

    # --- eyes: two bright close-set eyes in the front lattice gap
    for sx in (-1, 1):
        mb.add(T(sphere(0.013, 7, 5), (sx * 0.026, 0.685, 0.082)), "Echo_Eye")


# ----------------------------------------------------------------- animation
def anim_idle(builder: GlbBuilder, rig: Rig) -> None:
    """3.6s — gentle hover bob, petal ripple, halo drift, ribbon sway."""
    anim = builder.add_animation("AM_Lumewisp_Idle")
    dur, n = 3.6, 10
    _tr_sine(rig, anim, "Root", dur, n, 0.035)
    for tag, ph in (("L", 0.0), ("R", 1.1)):
        _rot_sine(rig, anim, f"Petal_{tag}_01", dur, n, 5.0, ph, "y")
        _rot_sine(rig, anim, f"Petal_{tag}_02", dur, n, 7.0, ph + 0.6, "y")
        _rot_sine(rig, anim, f"Petal_{tag}_01", dur, n, 3.0, ph + 0.3, "z")
    _rot_sine(rig, anim, "PetalTop", dur, n, 6.0, 0.8, "y")
    _rot_sine(rig, anim, "PetalTop", dur, n, 3.5, 1.5)
    _rot_sine(rig, anim, "Ribbon_01", dur, n, 6.0, 1.2, "y")
    _rot_sine(rig, anim, "Ribbon_02", dur, n, 9.0, 1.8, "y")
    _rot_sine(rig, anim, "Ribbon_01", dur, n, 4.0, 0.6)
    _tr_sine(rig, anim, "Halo", dur, n, 0.012, phase=0.9)
    _rot_sine(rig, anim, "Halo", dur, n, 4.0, 0.4, "x")
    _rot_sine(rig, anim, "Body", dur, n, 2.0, 0.5)


def anim_move(builder: GlbBuilder, rig: Rig) -> None:
    """1.2s — glide: petals flutter alternating, body pitch lean, ribbon wave."""
    anim = builder.add_animation("AM_Lumewisp_Move")
    dur, n = 1.2, 10
    _tr_sine(rig, anim, "Root", dur, n, 0.02, freq=2.0, phase=0.5)
    for tag, ph in (("L", 0.0), ("R", math.pi)):
        _rot_sine(rig, anim, f"Petal_{tag}_01", dur, n, 14.0, ph, "y")
        _rot_sine(rig, anim, f"Petal_{tag}_02", dur, n, 18.0, ph + 0.5, "y")
        _rot_sine(rig, anim, f"Petal_{tag}_01", dur, n, 6.0, ph, "z")
    _rot_sine(rig, anim, "PetalTop", dur, n, 10.0, 0.3, "y")
    _rot_sine(rig, anim, "Ribbon_01", dur, n, 10.0, 1.0, "y")
    _rot_sine(rig, anim, "Ribbon_02", dur, n, 14.0, 1.6, "y")
    _rot_sine(rig, anim, "Body", dur, n, 4.0, 0.3)
    _tr_sine(rig, anim, "Halo", dur, n, 0.008, freq=2.0, phase=1.2)
    _rot_sine(rig, anim, "Halo", dur, n, 8.0, 0.9, "x")


def anim_hit(builder: GlbBuilder, rig: Rig) -> None:
    """0.35s — petals tuck in, halo drops, body dips, ribbon kicks up."""
    anim = builder.add_animation("AM_Lumewisp_Hit")
    deg = DEG
    ts = [0.0, 0.07, 0.14, 0.24, 0.35]
    for tag, s in (("L", -1), ("R", 1)):
        rig.add_rotation_channel(anim, f"Petal_{tag}_01", list(zip(ts, [
            (0, 0, 0), (0, 0, s * 16 * deg), (0, 0, s * 11 * deg),
            (0, 0, s * 3 * deg), (0, 0, 0)])))
        rig.add_rotation_channel(anim, f"Petal_{tag}_02", list(zip(ts, [
            (0, 0, 0), (0, 0, s * 22 * deg), (0, 0, s * 15 * deg),
            (0, 0, s * 4 * deg), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "PetalTop", list(zip(ts, [
        (0, 0, 0), (-14 * deg, 0, 0), (-9 * deg, 0, 0), (-2 * deg, 0, 0), (0, 0, 0)])))
    rig.add_translation_channel(anim, "Root", [
        (0.0, (0, 0, 0)), (0.07, (0, -0.06, 0)), (0.14, (0, -0.04, 0)),
        (0.24, (0, -0.012, 0)), (0.35, (0, 0, 0))])
    rig.add_translation_channel(anim, "Halo", [
        (0.0, (0, 0, 0)), (0.07, (0, -0.03, 0)), (0.14, (0, -0.02, 0)),
        (0.24, (0, -0.005, 0)), (0.35, (0, 0, 0))])
    rig.add_rotation_channel(anim, "Ribbon_01", list(zip(ts, [
        (0, 0, 0), (10 * deg, 0, 0), (7 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Ribbon_02", list(zip(ts, [
        (0, 0, 0), (14 * deg, 0, 0), (9 * deg, 0, 0), (2 * deg, 0, 0), (0, 0, 0)])))
    rig.add_rotation_channel(anim, "Body", list(zip(ts, [
        (0, 0, 0), (-6 * deg, 0, 0), (-4 * deg, 0, 0), (1 * deg, 0, 0), (0, 0, 0)])))


def main() -> None:
    builder = GlbBuilder()
    for m in MATS.values():
        builder.add_material(m)

    rig = build_rig(builder)
    mb = MeshBuilder()
    build_body(mb)
    print(f"[{SPECIES}] body parts: {len(mb.parts)}, tris: {mb.triangle_count()}")

    skin_idx, prims = rig.build_skin(mb, power=3.0)
    mesh_node = builder.add_node("SK_Echo_Lumewisp", parent=0,
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
    stats["element"] = "Light"
    stats["ue_path"] = "/Game/Characters/Echoes/SK_Echo_Lumewisp"
    record("mesh", "SK_Echo_Lumewisp", stats)
    print(f"[{SPECIES}] {OUT_PATH}")
    print(f"[{SPECIES}] bones={stats['bones']} tris={stats['triangles']} "
          f"anims={len(stats['animations'])} bytes={stats['bytes']} validate={stats['validate']}")
    if problems:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
