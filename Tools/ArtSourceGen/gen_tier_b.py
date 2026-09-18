"""
ASTRAWILD ArtSourceGen — Tier-B species variant bakes (strategy §6, PCR-4/PCR-5).

Generates one unique GLB per Tier-B species (zone-signature / dungeon-pool /
event-boost / Huge / Epic+Legendary rule minus the 14 bespoke Tier-A species)
into ArtSource/Meshes/Echoes/SK_Echo_<Name>.glb, manifest-recorded.

Species data is parsed from the ACTUAL source tables (never hand-copied):
  - Source/.../AstrawildBestiaryData.cpp  (204 rows: plan/size/element/colors)
  - Source/.../AstrawildProductionContent.cpp (authored MakeProductionEcho calls)
  - Source/.../AstrawildWorldBootstrapper.cpp (zone wildlife + dungeon pools)
  - world-event species-boost payloads (ProductionContent.cpp)

Run:  python3 gen_tier_b.py
Output: ArtSource/Meshes/Echoes/SK_Echo_<Name>.glb (+ manifest records)
"""
from __future__ import annotations

import math
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))

from aw_archetypes import ELEMENT_TINTS, SIZE_SCALES, _spec_hash, add_clips, build_species, materials_for
from aw_gltf import GlbBuilder, validate_glb
from aw_manifest import record

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
OUT_DIR = os.path.join(ROOT, "ArtSource", "Meshes", "Echoes")

# The 14 Tier-A species with bespoke meshes (never bake over them).
TIER_A = {
    "Echo_Terraquill", "Echo_Cindermule", "Echo_Voltpylon", "Echo_Bastionbeetle",
    "Echo_Mistmender", "Echo_Deepdelver",
    "Echo_Lumewisp", "Echo_Sprigling", "Echo_Gloomfang", "Echo_Auroraling",
    "Echo_Dawnfang", "Echo_GlassTyrant", "Echo_EyeSentinel", "Echo_DrownedSovereign",
}


def read(rel: str) -> str:
    with open(os.path.join(ROOT, rel), encoding="utf-8") as fh:
        return fh.read()


# ------------------------------------------------------------------ parsers

def parse_bestiary() -> dict:
    """204 rows -> {id: dict(name, plan, size, element, colors)}."""
    src = read(os.path.join("Source", "AstrawildCore", "Private", "AstrawildBestiaryData.cpp"))
    rows = re.findall(
        r'\{ TEXT\("(Echo_\w+)"\), TEXT\("(\w+)"\),\s*'
        r'EAstrawildEchoFamily::(\w+), EAstrawildBodyPlan::(\w+), EAstrawildSizeClass::(\w+),\s*'
        r'EAstrawildElementType::(\w+), EAstrawildElementType::(\w+),\s*'
        r'EAstrawildEchoRole::(\w+), EAstrawildZone::(\w+), TEXT\("Zone_\w+"\),\s*'
        r'EAstrawildPersonality::(\w+), EAstrawildActivityPattern::(\w+),\s*'
        r'[\d.]+f, [\d.]+f, [\d.]+f, [\d.]+f, [\d.]+f, (?:true|false),\s*'
        r'([\d.]+)f, ([\d.]+)f, ([\d.]+)f, ([\d.]+)f, ([\d.]+)f, ([\d.]+)f,',
        src)
    out = {}
    for (rid, name, fam, plan, size, elem, weak, role, zone, pers, act,
         pr, pg, pb, sr, sg, sb) in rows:
        out[rid] = {
            "name": name,
            "plan": plan.lower(),
            "size": size,
            "element": elem,
            "colors": ((float(pr), float(pg), float(pb)),
                       (float(sr), float(sg), float(sb))),
        }
    return out


def parse_authored() -> dict:
    """Authored MakeProductionEcho calls -> {id: dict(name, plan, size, element, rarity)}."""
    src = read(os.path.join("Source", "AstrawildCore", "Private", "AstrawildProductionContent.cpp"))
    calls = re.findall(
        r'MakeProductionEcho\(Registry, TEXT\("(Echo_\w+)"\), TEXT\("(\w+)"\),\s*'
        r'EAstrawildElementType::(\w+),\s*EAstrawildEchoRole::(\w+),\s*'
        r'[\d.]+f, [\d.]+f, [\d.]+f, [\d.]+f,\s*'
        r'EAstrawildPersonality::(\w+),\s*EAstrawildActivityPattern::(\w+),\s*'
        r'[\s\S]*?EAstrawildEchoFamily::(\w+),\s*EAstrawildBodyPlan::(\w+),\s*EAstrawildSizeClass::(\w+),\s*'
        r'EAstrawildZone::(\w+),\s*EAstrawildRarity::(\w+),',
        src)
    out = {}
    for (rid, name, elem, role, pers, act, fam, plan, size, zone, rarity) in calls:
        out[rid] = {"name": name, "plan": plan.lower(), "size": size,
                    "element": elem, "rarity": rarity}
    return out


def parse_appearance() -> dict:
    """ContentLibrary appearance retrofit table (hand-authored ten) -> {id: {plan, size, colors}}."""
    src = read(os.path.join("Source", "AstrawildCore", "Private", "AstrawildContentLibrary.cpp"))
    rows = re.findall(
        r'\{ TEXT\("(Echo_\w+)"\),\s*EAstrawildEchoFamily::\w+,\s*EAstrawildBodyPlan::(\w+),\s*EAstrawildSizeClass::(\w+),\s*'
        r'EAstrawildZone::\w+,\s*([\d.]+)f, ([\d.]+)f, ([\d.]+)f, ([\d.]+)f, ([\d.]+)f, ([\d.]+)f \},',
        src)
    out = {}
    for (rid, plan, size, pr, pg, pb, sr, sg, sb) in rows:
        # Element from the MakeEcho call site (same file, one line above the block).
        elem = "None"
        m = re.search(r'TEXT\("' + re.escape(rid) + r'"\), TEXT\("\w+"\), EAstrawildElementType::(\w+)', src)
        if m:
            elem = m.group(1)
        out[rid] = {
            "name": rid.replace("Echo_", ""),
            "plan": plan.lower(),
            "size": size,
            "element": elem,
            "colors": ((float(pr), float(pg), float(pb)),
                       (float(sr), float(sg), float(sb))),
        }
    return out


def parse_spawn_species() -> set:
    """Zone-wildlife rows + dungeon creature pools + event species boosts."""
    wb = read(os.path.join("Source", "AstrawildCore", "Private", "AstrawildWorldBootstrapper.cpp"))
    pc = read(os.path.join("Source", "AstrawildCore", "Private", "AstrawildProductionContent.cpp"))
    ids = set(re.findall(r'TEXT\("(Echo_\w+)"\), \d+ \}', wb))  # wildlife rows carry counts
    ids |= set(re.findall(r'CreaturePoolIds = \{ ([^}]+) \};', wb)[0] and
               re.findall(r'TEXT\("(Echo_\w+)"\)', " ".join(re.findall(r'CreaturePoolIds = \{ ([^}]+) \};', wb))) or set())
    ids |= set(re.findall(r'SpeciesBoostId = TEXT\("(Echo_\w+)"\);', pc))
    return ids


# ---------------------------------------------------------------- selection

def select_tier_b(bestiary: dict, authored: dict, appearance: dict, spawn_ids: set) -> list:
    """Strategy §5 rule: spawn-table species ∪ Huge ∪ Epic/Legendary ∪ the
    monolith/colossus name family (visually signature creatures) — minus Tier-A."""
    selected = set(spawn_ids)
    for rid, row in bestiary.items():
        name = row["name"].lower()
        if row["size"] == "Huge" or "monolith" in name or "colossus" in name:
            selected.add(rid)
    for rid, row in authored.items():
        if row["size"] == "Huge" or row.get("rarity") in ("Epic", "Legendary", "Mythic"):
            selected.add(rid)
    selected -= TIER_A
    # The appearance table feeds data for legacy authored species (Lumewisp etc.
    # are Tier-A and excluded; Stonehide/Duskmoth/Emberfang/Rimefang/Voltmaw stay).
    selected = {rid for rid in selected if rid in bestiary or rid in appearance or rid in authored}
    return sorted(selected)


# --------------------------------------------------------------------- bake

def derive_colors(rid: str, bestiary: dict, authored: dict, appearance: dict):
    """Row colors when a table defines them; element-derived otherwise."""
    for table in (bestiary, appearance):
        if rid in table and "colors" in table[rid]:
            primary, secondary = table[rid]["colors"]
            if primary and secondary:
                return primary, secondary
    row = authored.get(rid, {})
    elem = row.get("element", "None")
    tint = ELEMENT_TINTS.get(elem, (0.7, 0.7, 0.7))
    base = tuple(0.28 + 0.3 * c for c in tint)
    return base, tuple(0.5 + 0.25 * c for c in base)


def bake(rid: str, bestiary: dict, authored: dict, appearance: dict) -> dict:
    row = bestiary.get(rid) or appearance.get(rid) or authored[rid]
    name = row["name"]
    plan = row["plan"]
    size = row.get("size", "Medium")
    element = row.get("element", "None")
    primary, secondary = derive_colors(rid, bestiary, authored, appearance)

    spec = {
        "name": name,
        "plan": plan,
        "size": SIZE_SCALES.get(size, 1.0),
        "size_class": size,
        "body_color": primary,
        "armor_color": secondary,
        "element_color": ELEMENT_TINTS.get(element, (0.85, 0.85, 0.85)),
        "seed": _spec_hash(name),
    }

    asset = f"SK_Echo_{name}"
    out_path = os.path.join(OUT_DIR, asset + ".glb")

    builder = GlbBuilder()
    for mat in materials_for(spec).values():
        builder.add_material(mat)

    rig, skin_idx, prims = build_species(builder, spec)
    mesh_node = builder.add_node(asset, parent=0, translation=(0, 0, 0))
    builder.assign_skin(mesh_node, skin_idx, prims)

    add_clips(builder, rig, spec)

    os.makedirs(OUT_DIR, exist_ok=True)
    stats = builder.save_glb(out_path)
    problems = validate_glb(out_path)
    stats["validate"] = "PASS" if not problems else problems
    stats["bones"] = len(rig.bones)
    stats["asset_type"] = "skeletal_mesh"
    stats["species"] = name
    stats["element"] = element
    stats["body_plan"] = plan
    stats["tier"] = "B"
    stats["ue_path"] = f"/Game/Characters/Echoes/{asset}"
    record("mesh", asset, stats)
    return {"name": name, "plan": plan, "stats": stats, "problems": problems}


def main() -> None:
    bestiary = parse_bestiary()
    authored = parse_authored()
    appearance = parse_appearance()
    spawn_ids = parse_spawn_species()
    tier_b = select_tier_b(bestiary, authored, appearance, spawn_ids)

    print(f"[tier-b] bestiary={len(bestiary)} authored={len(authored)} appearance={len(appearance)} "
          f"spawn-table={len(spawn_ids)} selected={len(tier_b)}")

    failed = []
    for rid in tier_b:
        result = bake(rid, bestiary, authored, appearance)
        s = result["stats"]
        status = "PASS" if not result["problems"] else result["problems"]
        print(f"[tier-b] {result['name']:20s} plan={result['plan']:12s} "
              f"bones={s['bones']} tris={s['triangles']} anims={len(s['animations'])} "
              f"bytes={s['bytes']} validate={status}")
        if result["problems"]:
            failed.append(result["name"])

    print(f"[tier-b] baked {len(tier_b) - len(failed)}/{len(tier_b)} species; failures: {failed or 'none'}")
    if failed:
        raise SystemExit(1)


if __name__ == "__main__":
    main()
