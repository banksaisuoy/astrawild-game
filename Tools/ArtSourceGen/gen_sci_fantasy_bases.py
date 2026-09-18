"""
ASTRAWILD ArtSourceGen — Sci-Fantasy base archetype bakes (directive SCI Phase 3/4).

Bakes the 16 SK_Base_* GLBs (two per Sci-Fantasy theme) into
ArtSource/Meshes/Echoes/BaseMeshes/, manifest-recorded with
ue_path = /Game/Characters/Echoes/BaseMeshes/<id>.

Each base reuses the Tier-B archetype builders (rig + proximity skinning +
Idle/Move/Hit clips — the proven, engine-import-compatible pipeline), then
adds THEME identity geometry on top (plates, cores, fronds, tendrils, vents)
so every base reads as a Sci-Fantasy monster archetype, never a plain animal.

The runtime side (FAstrawildEchoMutator + AAstrawildEchoCharacter) binds these
bases to the 204-species roster by convention path — opt-in, fail-closed
(before import: mutated PMC body; after import: skinned base + mutations).

Run:  python3 Tools/ArtSourceGen/gen_sci_fantasy_bases.py
"""
from __future__ import annotations

import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from aw_archetypes import (_BUILDERS, _spec_hash, add_clips, materials_for)
from aw_gltf import GlbBuilder, validate_glb
from aw_manifest import record
from aw_rig import Rig
from aw_shapes import MeshBuilder, box, cone, sphere, translate, rotate

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT_DIR = os.path.join(ROOT, "ArtSource", "Meshes", "Echoes", "BaseMeshes")

# ---------------------------------------------------------------------------
# The 16 base archetypes: (id, plan, theme, body/armor/emissive palettes).
# Palette = Sci-Fantasy theme identity (mirrors THEME_TINTS in
# Scripts/generate_echo_mutations.py + FAstrawildEchoMutator::ResolveThemeTint).
# ---------------------------------------------------------------------------
BASES = [
    # Ancient Constructs — golem quadruped + monolith colossus biped
    ("SK_Base_GolemQuadruped", "quadruped", "AncientConstruct",
     (0.62, 0.58, 0.50), (0.42, 0.40, 0.37), (0.95, 0.78, 0.38)),
    ("SK_Base_MonolithColossus", "biped", "AncientConstruct",
     (0.55, 0.52, 0.46), (0.38, 0.36, 0.34), (0.62, 0.86, 0.92)),
    # Elemental Beasts — ember drake quadruped + elemental wisp floater
    ("SK_Base_ElemDrake", "quadruped", "ElementalBeast",
     (0.95, 0.62, 0.30), (0.58, 0.22, 0.12), (1.00, 0.48, 0.20)),
    ("SK_Base_ElemWisp", "floating", "ElementalBeast",
     (0.55, 0.88, 0.98), (0.30, 0.55, 0.70), (0.55, 0.88, 0.98)),
    # Mutated Fauna — mutant beast quadruped + mutant avian
    ("SK_Base_MutantBeast", "quadruped", "MutatedFauna",
     (0.55, 0.48, 0.38), (0.40, 0.34, 0.28), (0.72, 0.95, 0.40)),
    ("SK_Base_MutantAvian", "avian", "MutatedFauna",
     (0.50, 0.55, 0.60), (0.36, 0.40, 0.46), (0.98, 0.42, 0.55)),
    # Armored Organics — armored beetle insectoid + armored crab quadruped
    ("SK_Base_ArmoredBeetle", "insectoid", "ArmoredOrganic",
     (0.45, 0.52, 0.42), (0.28, 0.33, 0.26), (0.85, 0.65, 0.20)),
    ("SK_Base_ArmoredCrab", "quadruped", "ArmoredOrganic",
     (0.58, 0.46, 0.36), (0.33, 0.26, 0.22), (0.95, 0.42, 0.20)),
    # Ethereal Spirits — spirit wisp + spirit orb
    ("SK_Base_SpiritWisp", "floating", "EtherealSpirit",
     (0.72, 0.82, 0.95), (0.55, 0.68, 0.85), (0.98, 0.95, 0.72)),
    ("SK_Base_SpiritOrb", "amorphous", "EtherealSpirit",
     (0.62, 0.74, 0.92), (0.45, 0.58, 0.78), (0.98, 0.95, 0.72)),
    # Mechanical Hybrids — cyborg beast + cyborg serpent
    ("SK_Base_CyborgBeast", "quadruped", "MechanicalHybrid",
     (0.58, 0.63, 0.70), (0.30, 0.34, 0.40), (0.36, 0.92, 0.86)),
    ("SK_Base_CyborgSerpent", "serpent", "MechanicalHybrid",
     (0.50, 0.55, 0.62), (0.26, 0.30, 0.36), (0.36, 0.92, 0.86)),
    # Plant Monsters — plant maw biped + mushroomling floater
    ("SK_Base_PlantMaw", "biped", "PlantMonster",
     (0.42, 0.71, 0.33), (0.26, 0.48, 0.22), (0.72, 0.95, 0.40)),
    ("SK_Base_Mushroomling", "floating", "PlantMonster",
     (0.60, 0.48, 0.38), (0.48, 0.30, 0.28), (0.42, 0.90, 0.55)),
    # Void Abominations — void blob + void tentacle
    ("SK_Base_VoidBlob", "amorphous", "VoidAbomination",
     (0.38, 0.30, 0.44), (0.20, 0.14, 0.28), (0.72, 0.40, 1.00)),
    ("SK_Base_VoidTentacle", "serpent", "VoidAbomination",
     (0.34, 0.26, 0.40), (0.18, 0.12, 0.26), (0.62, 0.34, 0.90)),
]

_T = translate


def _theme_features(mb: MeshBuilder, theme: str, plan: str, s: float) -> None:
    """Sci-Fantasy identity geometry layered onto the archetype body.

    Everything rides the existing material slots (Echo_Armor for structure,
    Echo_Emissive for glow) so the proximity skinner weights them with the
    nearby body bones — decorations MOVE with the animation clips.
    """
    # Heights differ per plan; the decorators hug the upper body band where
    # every archetype keeps its mass (0.3-0.5 of a 1.0-scale body).
    if theme == "AncientConstruct":
        # Shoulder pauldrons + rune core + anklets.
        for sx in (-1, 1):
            mb.add(_T(box((0.10 * s, 0.06 * s, 0.14 * s)), (sx * 0.15 * s, 0.40 * s, 0.10 * s)), "Echo_Armor")
        mb.add(_T(rotate(box((0.06 * s, 0.06 * s, 0.06 * s)), 0.785, 0.785, 0.0),
                  (0.0, 0.42 * s, -0.02 * s)), "Echo_Emissive")  # rune core
        for sx in (-1, 1):
            mb.add(_T(box((0.05 * s, 0.05 * s, 0.05 * s)), (sx * 0.11 * s, 0.18 * s, 0.16 * s)), "Echo_Armor")
    elif theme == "ElementalBeast":
        # Energy mane nodes down the spine + burning chest core.
        for k in range(5):
            mb.add(_T(sphere(0.035 * s, 8, 6), (0.0, 0.44 * s, (-0.12 + 0.07 * k) * s)), "Echo_Emissive")
        mb.add(_T(sphere(0.055 * s, 8, 6), (0.0, 0.34 * s, 0.10 * s)), "Echo_Emissive")
    elif theme == "MutatedFauna":
        # Asymmetric growths + extra spikes + third-eye stalk.
        mb.add(_T(sphere(0.05 * s, 8, 6), (0.07 * s, 0.40 * s, -0.10 * s)), "Echo_Body")
        mb.add(_T(sphere(0.03 * s, 7, 5), (-0.06 * s, 0.45 * s, 0.02 * s)), "Echo_Body")
        for k in range(3):
            mb.add(_T(rotate(cone(0.022 * s, 0.10 * s, 6), math.pi, 0, 0),
                      (0.0, 0.47 * s, (0.02 + 0.07 * k) * s)), "Echo_Emissive")
        mb.add(_T(rotate(cone(0.02 * s, 0.08 * s, 5), 0.9, 0, 0),
                  (0.05 * s, 0.52 * s, 0.44 * s)), "Echo_Emissive")  # eye stalk
    elif theme == "ArmoredOrganic":
        # Carapace back plate + rim spikes.
        mb.add(_T(box((0.20 * s, 0.05 * s, 0.26 * s)), (0.0, 0.42 * s, -0.02 * s)), "Echo_Armor")
        for k in range(4):
            for sx in (-1, 1):
                mb.add(_T(rotate(cone(0.02 * s, 0.07 * s, 5), math.pi, 0, 0),
                          (sx * (0.13 + 0.01 * k) * s, 0.46 * s, (-0.10 + 0.06 * k) * s)), "Echo_Emissive")
    elif theme == "EtherealSpirit":
        # Orbiting wisps + halo ring.
        for k in range(3):
            a = k * 2.094
            mb.add(_T(sphere(0.032 * s, 8, 6),
                      (0.16 * s * math.cos(a), 0.46 * s + 0.03 * s * math.sin(a), 0.40 * s)), "Echo_Emissive")
        mb.add(_T(rotate(cone(0.02 * s, 0.14 * s, 6), math.pi / 2, 0, 0),
                  (0.0, 0.55 * s, 0.42 * s)), "Echo_Emissive")
    elif theme == "MechanicalHybrid":
        # Exhaust vents + glow coil + antenna mast.
        mb.add(_T(box((0.14 * s, 0.05 * s, 0.10 * s)), (0.0, 0.44 * s, -0.14 * s)), "Echo_Armor")
        mb.add(_T(rotate(cone(0.012 * s, 0.18 * s, 5), -0.35, 0, 0),
                  (0.0, 0.52 * s, 0.44 * s)), "Echo_Armor")  # antenna
        mb.add(_T(sphere(0.024 * s, 6, 5), (0.0, 0.62 * s, 0.50 * s)), "Echo_Emissive")
        for k in range(3):
            mb.add(_T(sphere(0.02 * s, 6, 5), (0.09 * s, (0.36 + 0.05 * k) * s, 0.02 * s)), "Echo_Emissive")
    elif theme == "PlantMonster":
        # Leaf fronds + spore bulbs + bark collar.
        for k in range(4):
            ang = -0.9 + 0.6 * k
            mb.add(_T(rotate(cone(0.03 * s, 0.16 * s, 5), ang, 0, 0),
                      (0.0, 0.46 * s, 0.06 * s)), "Echo_Body")  # fronds
        for k in range(3):
            mb.add(_T(sphere(0.028 * s, 7, 5),
                      (0.07 * (k - 1) * s, 0.40 * s, (-0.14 + 0.08 * k) * s)), "Echo_Emissive")
        mb.add(_T(box((0.13 * s, 0.05 * s, 0.08 * s)), (0.0, 0.36 * s, 0.14 * s)), "Echo_Armor")
    elif theme == "VoidAbomination":
        # Tendrils + void cores + jagged shards.
        for k in range(4):
            sx = -1 if k % 2 == 0 else 1
            mb.add(_T(rotate(cone(0.016 * s, 0.17 * s, 5), 1.1 * sx, 0, 0.4),
                      (sx * 0.09 * s, 0.44 * s, (0.08 - 0.08 * (k // 2)) * s)), "Echo_Body")
        mb.add(_T(sphere(0.045 * s, 8, 6), (0.0, 0.40 * s, 0.02 * s)), "Echo_Emissive")
        mb.add(_T(sphere(0.03 * s, 6, 5), (0.06 * s, 0.30 * s, 0.10 * s)), "Echo_Emissive")


def bake_base(base_id: str, plan: str, theme: str,
              body: tuple, armor: tuple, emissive: tuple) -> dict:
    spec = {
        "name": base_id,  # clip names derive: AM_<base_id>_Idle/Move/Hit
        "plan": plan,
        "size": 1.0,  # species scale applies at runtime (BodyScaleForSize)
        "size_class": "Medium",
        "body_color": body,
        "armor_color": armor,
        "element_color": emissive,
        "seed": _spec_hash(theme),  # deterministic theme-level archetype jitter
    }

    builder = GlbBuilder()
    for mat in materials_for(spec).values():
        builder.add_material(mat)

    rig = Rig(builder, root_name="Root", root_position=(0, 0, 0))
    mb = MeshBuilder()
    _BUILDERS[plan](rig, mb, spec)
    _theme_features(mb, theme, plan, 1.0)
    skin_idx, prims = rig.build_skin(mb, power=3.3)

    mesh_node = builder.add_node(base_id, parent=0, translation=(0, 0, 0))
    builder.assign_skin(mesh_node, skin_idx, prims)

    add_clips(builder, rig, spec)

    out_path = os.path.join(OUT_DIR, base_id + ".glb")
    stats = builder.save_glb(out_path)
    problems = validate_glb(out_path)
    stats["validate"] = "PASS" if not problems else problems
    stats["bones"] = len(rig.bones)
    stats["asset_type"] = "skeletal_mesh"
    stats["theme"] = theme
    stats["body_plan"] = plan
    stats["tier"] = "S"
    stats["ue_path"] = f"/Game/Characters/Echoes/BaseMeshes/{base_id}"
    record("mesh", base_id, stats)
    return {"id": base_id, "stats": stats, "problems": problems}


def main() -> None:
    os.makedirs(OUT_DIR, exist_ok=True)
    failed = []
    for base_id, plan, theme, body, armor, emissive in BASES:
        result = bake_base(base_id, plan, theme, body, armor, emissive)
        s = result["stats"]
        status = "PASS" if not result["problems"] else result["problems"]
        print(f"[sci-base] {base_id:28s} plan={plan:11s} theme={theme:18s} "
              f"bones={s['bones']} tris={s['triangles']} anims={len(s['animations'])} "
              f"bytes={s['bytes']} validate={status}")
        if result["problems"]:
            failed.append(base_id)

    print(f"[sci-base] baked {len(BASES) - len(failed)}/{len(BASES)} bases; failures: {failed or 'none'}")
    if failed:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
