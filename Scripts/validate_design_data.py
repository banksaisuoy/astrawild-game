#!/usr/bin/env python3
"""Validate Design/design_data.json (schema astrawild-design-data/2) against
the C++ source it claims to describe.

Layers of proof (Master Directive v1: R3/R4 — nothing is "verified" by vibes):

  0. ENVELOPE    — schema id + repo_head (ancestor + no Source/ drift = pass)
  1. ROUND-TRIP  — every traced entry re-opened at its {file, line}, id/value
                   re-read from source
  2. CROSS-REF   — every inter-domain reference resolves (recipe->item,
                   loot->item, weapon->ammo, poi->loot, event->loot,
                   hunt->species/item, quest->item/npc, tech->tech/recipe,
                   species->zone, census==validate_final_run.py)
  3. RECOUNT     — independent grep-level recounts of the headline numbers

Sandbox-safe: pure text round-trips over Source/. No Unreal required.
Exit code 0 == ALL CHECKS PASSED.
"""

from __future__ import annotations

import json
import re
import subprocess
import sys
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
DESIGN = REPO / "Design" / "design_data.json"

FAILURES: list[str] = []
CHECKS = 0


def check(ok: bool, label: str, detail: str = "") -> bool:
    global CHECKS
    CHECKS += 1
    if ok:
        return True
    FAILURES.append(f"{label}{(' :: ' + detail) if detail else ''}")
    return False


_LINE_CACHE: dict[str, list[str]] = {}


def lines_of(rel: str) -> list[str]:
    if rel not in _LINE_CACHE:
        p = REPO / rel
        _LINE_CACHE[rel] = p.read_text(encoding="utf-8", errors="replace").splitlines()
    return _LINE_CACHE[rel]


def window(rel: str, start: int, span: int) -> str:
    ls = lines_of(rel)
    a = max(0, start - 1)
    b = min(len(ls), start - 1 + span)
    return "\n".join(ls[a:b])


_NUM_RE = re.compile(r"-?\d+(?:\.\d+)?(?:[eE][-+]?\d+)?")


def num_in_text(text: str, value) -> bool:
    if isinstance(value, bool):
        return ("true" in text) if value else ("false" in text)
    if value is None:
        return True
    try:
        want = float(value)
    except (TypeError, ValueError):
        return False
    for m in _NUM_RE.finditer(text):
        try:
            if float(m.group(0)) == want:
                return True
        except ValueError:
            continue
    return False


def token_in_text(text: str, tid) -> bool:
    return f'TEXT("{tid}")' in text


# ---------------------------------------------------------------------------
# layer 0: envelope
# ---------------------------------------------------------------------------

def validate_envelope(doc: dict) -> None:
    check(doc.get("schema") == "astrawild-design-data/2", "env/schema",
          str(doc.get("schema")))
    head = subprocess.run(["git", "rev-parse", "HEAD"], cwd=REPO,
                          capture_output=True, text=True).stdout.strip()
    if doc.get("repo_head") == head:
        check(True, "env/repo_head")
    else:
        anc = subprocess.run(["git", "merge-base", "--is-ancestor",
                              doc.get("repo_head", ""), head], cwd=REPO)
        diff = subprocess.run(["git", "diff", "--name-only", doc.get("repo_head", ""),
                               head, "--", "Source"], cwd=REPO,
                              capture_output=True, text=True).stdout.strip()
        check(anc.returncode == 0 and not diff, "env/repo_head",
              f"json head {str(doc.get('repo_head'))[:8]} not a clean ancestor "
              f"of HEAD {head[:8]} (Source/ changed: {diff.splitlines()[:3]}) — "
              "regenerate")


# ---------------------------------------------------------------------------
# layer 1: round-trip
# ---------------------------------------------------------------------------

def validate_tunables(entries: list[dict]) -> None:
    bad = 0
    for e in entries:
        line = window(e["file"], e["line"], 1)
        name_ok = e["name"] in line
        val_ok = num_in_text(line, e["value"]) or str(e["value"]).strip('"') in line
        if not (name_ok and val_ok):
            bad += 1
            if bad <= 5:
                check(False, "roundtrip/tunable",
                      f"{e['file']}:{e['line']} name={e['name']} value={e['value']}")
    check(bad == 0, "roundtrip/tunables", f"{bad} drifted of {len(entries)}")


def validate_bestiary(species: list[dict]) -> None:
    bad = 0
    for s in species:
        text = window(s["file"], s["line"], 12)
        if not token_in_text(text, s["id"]):
            bad += 1
            if bad <= 5:
                check(False, "roundtrip/species",
                      f"{s['file']}:{s['line']} id={s['id']}")
    check(bad == 0, "roundtrip/bestiary", f"{bad} drifted of {len(species)}")


def validate_entries_id(domains: dict, name: str, span: int = 25) -> None:
    """Generic id round-trip for helper/variable-style domains.
    Weather entries key on `state` (an enum value, not a TEXT token)."""
    dom = domains.get(name)
    if not check(dom is not None, f"roundtrip/{name}", "domain missing"):
        return
    entries = dom.get("entries", [])
    bad = 0
    for e in entries:
        key = e.get("id") or e.get("state")
        if key is None:
            continue
        text = window(e["file"], e["line"], span)
        if name == "weather":
            ok = f"EAstrawildWeatherState::{key}" in text
        else:
            ok = token_in_text(text, key)
        if not ok:
            bad += 1
            if bad <= 5:
                check(False, f"roundtrip/{name}",
                      f"{e['file']}:{e['line']} id={key}")
    check(bad == 0, f"roundtrip/{name}", f"{bad} drifted of {len(entries)}")


def validate_item_attrs(items: list[dict]) -> None:
    bad = 0
    n_attr = 0
    for it in items:
        for a in it.get("attributes", []):
            n_attr += 1
            line = window(a["file"], a["line"], 1)
            prop_ok = a["prop"].replace(".Add", "") in line
            val = a["value"]
            val_ok = (num_in_text(line, val)
                      or (isinstance(val, str) and f'TEXT("{val}")' in line)
                      or (isinstance(val, str) and val in line)
                      or (val is None)
                      or (isinstance(val, list) and all(
                          num_in_text(line, x) or isinstance(x, (str, dict))
                          for x in val)))
            if not (prop_ok and val_ok):
                bad += 1
                if bad <= 5:
                    check(False, "roundtrip/item-attr",
                          f"{a['file']}:{a['line']} {a['prop']}={val!r}")
    check(bad == 0, "roundtrip/item-attrs", f"{bad} drifted of {n_attr}")


# ---------------------------------------------------------------------------
# layer 2: cross-references
# ---------------------------------------------------------------------------

def validate_crossrefs(domains: dict) -> None:
    item_ids = {it["id"] for it in domains["items"]["entries"]}
    species_ids = ({s["id"] for s in domains["bestiary"]["species"]}
                   | {s["id"] for s in domains["species"]["entries"]})
    tech_ids = {t["id"] for t in domains["technologies"]["entries"]}
    loot_ids = {l["id"] for l in domains["loot_tables"]["entries"]}
    zone_ids = {z["id"] for z in domains["zones"]["entries"]}
    recipe_ids = {r["id"] for r in domains["recipes"]["entries"]}
    quest_ids = {q["id"] for q in domains["quests"]["entries"]}
    npc_ids = {n["id"] for n in domains["npcs"]["entries"]}
    dialogue_ids = {d["id"] for d in domains["dialogue_trees"]["entries"]}
    boss_ids = {b["id"] for b in domains["bosses"]["entries"]}
    # dungeon defeat-event targets (Creature_*) are quest-objective targets
    defeat_event_ids = {
        pr["value"] for d in domains["dungeons"]["entries"]
        for pr in d.get("properties", [])
        if pr["prop"] == "BossDefeatEventId" and isinstance(pr["value"], str)
    }
    building_ids = {b["id"] for b in domains["buildings"]["entries"]}
    poi_ids = {p["id"] for p in domains["pois"]["entries"]}
    event_ids = {ev["id"] for ev in domains["world_events"]["entries"]}
    dungeon_ids = {d["id"] for d in domains["dungeons"]["entries"]}
    site_ids = {si["id"] for si in domains["work_sites"]["entries"]}
    robot_ids = {r["id"] for r in domains["robots"]["entries"]}
    weapon_ids = {w["id"] for w in domains["weapons"]["entries"]}
    creature_ids = ({f"Creature_{b.split('_', 1)[1]}" for b in boss_ids}
                    | {f"Creature_{s.split('_', 1)[1]}" for s in species_ids})

    # recipes -> items
    dangling = set()
    for r in domains["recipes"]["entries"]:
        for io_list in (r.get("inputs", []), r.get("outputs", [])):
            for st in io_list:
                if isinstance(st, dict) and st.get("item") and st["item"] not in item_ids:
                    dangling.add(f"{r['id']}->{st['item']}")
    check(not dangling, "xref/recipe->item", f"dangling: {sorted(dangling)[:8]}")

    # bestiary loot + food -> items/species
    loot_bad, food_notes = set(), 0
    for s in domains["bestiary"]["species"]:
        for k in ("loot_a", "loot_b"):
            v = s.get(k)
            if v and v not in item_ids:
                loot_bad.add(f"{s['id']}.{k}->{v}")
        for k in ("food_a", "food_b"):
            v = s.get(k)
            if v and v not in item_ids and v not in species_ids:
                food_notes += 1
    check(not loot_bad, "xref/bestiary.loot->item", f"dangling: {sorted(loot_bad)[:8]}")
    check(food_notes == 0, "xref/bestiary.food->item|species",
          f"{food_notes} unresolved")

    # authored species food/loot/zone. Food may be a local TArray variable
    # (e.g. `BerryFood`) or an inline TArray literal — only TEXT ids are
    # cross-checkable; zone may be an enum name (display-name mismatch ok).
    sp_loot_bad = set()
    zone_enum_names = {z["zone"] for z in domains["zones"]["entries"]}
    bestiary_zones = set(domains["bestiary"]["aggregates"]["zones"].keys())
    for s in domains["species"]["entries"]:
        food = s.get("food")
        food_items = food if isinstance(food, list) else [food]
        for f in food_items:
            if isinstance(f, str) and (f.startswith("Item_") or f.startswith("Echo_")) \
                    and f not in item_ids and f not in species_ids:
                sp_loot_bad.add(f"{s['id']}.food->{f}")
        loot = s.get("loot")
        if isinstance(loot, list):
            for st in loot:
                if isinstance(st, dict) and st.get("item") not in item_ids:
                    sp_loot_bad.add(f"{s['id']}.loot->{st.get('item')}")
        zone = s.get("home_zone")
        if zone is not None and zone != "None" \
                and zone not in zone_enum_names and zone not in bestiary_zones:
            sp_loot_bad.add(f"{s['id']}.zone->{zone}")
    check(not sp_loot_bad, "xref/species.food/loot/zone",
          f"dangling: {sorted(sp_loot_bad)[:8]}")

    # weapons -> ammo items
    ammo_bad = set()
    for w in domains["weapons"]["entries"]:
        ammo = w.get("ammo")
        if ammo and ammo not in item_ids and ammo != w["id"]:
            ammo_bad.add(f"{w['id']}.ammo->{ammo}")
    check(not ammo_bad, "xref/weapon.ammo->item", f"dangling: {sorted(ammo_bad)[:8]}")

    # pois -> loot tables
    poi_bad = set()
    for p in domains["pois"]["entries"]:
        lt = p.get("loot_table")
        if lt and lt not in loot_ids:
            poi_bad.add(f"{p['id']}->{lt}")
    check(not poi_bad, "xref/poi.loot->loot_table", f"dangling: {sorted(poi_bad)[:8]}")

    # world events -> loot tables (attributes or direct)
    ev_bad = set()
    for e in domains["world_events"]["entries"]:
        for a in e.get("attributes", []):
            if a["prop"] == "RewardLootTableId" and isinstance(a["value"], str) \
                    and a["value"] not in loot_ids:
                ev_bad.add(f"{e['id']}->{a['value']}")
    check(not ev_bad, "xref/event.loot->loot_table", f"dangling: {sorted(ev_bad)[:8]}")

    # hunt contracts -> species + reward items
    hunt_bad = set()
    for h in domains["hunt_contracts"]["entries"]:
        if h["species"] not in species_ids:
            hunt_bad.add(f"{h['id']}.species->{h['species']}")
        if h["reward_item"] not in item_ids:
            hunt_bad.add(f"{h['id']}.reward->{h['reward_item']}")
    check(not hunt_bad, "xref/hunt->species|item", f"dangling: {sorted(hunt_bad)[:8]}")

    # quests -> every objective/reward target must resolve in SOME domain
    resolvable = (item_ids | species_ids | quest_ids | building_ids
                  | tech_ids | zone_ids | poi_ids | event_ids | loot_ids
                  | npc_ids | dialogue_ids | dungeon_ids | site_ids
                  | robot_ids | recipe_ids | weapon_ids | creature_ids
                  | defeat_event_ids)
    q_bad = set()
    for q in domains["quests"]["entries"]:
        for r in q.get("reward_items", []):
            if r["item"] not in item_ids:
                q_bad.add(f"{q['id']}.reward->{r['item']}")
        for o in q.get("objectives", []):
            t = o.get("target")
            if t and t not in resolvable \
                    and not t.startswith(("Location_", "Village_", "NPC_")):
                q_bad.add(f"{q['id']}.objective->{t}")
        nq = q.get("next_quest")
        if nq and nq not in quest_ids:
            q_bad.add(f"{q['id']}.next->{nq}")
    check(not q_bad, "xref/quest targets resolve",
          f"dangling: {sorted(q_bad)[:10]}")

    # npcs -> quests + dialogue trees
    dialogue_ids = {d["id"] for d in domains["dialogue_trees"]["entries"]}
    npc_bad = set()
    for n in domains["npcs"]["entries"]:
        oq = n.get("offered_quest")
        if oq and oq not in quest_ids:
            npc_bad.add(f"{n['id']}.quest->{oq}")
        dt = n.get("dialogue_tree")
        if dt and dt not in dialogue_ids:
            npc_bad.add(f"{n['id']}.dialogue->{dt}")
    check(not npc_bad, "xref/npc->quest|dialogue", f"dangling: {sorted(npc_bad)[:8]}")

    # technologies -> prereq techs + unlocked recipes
    recipe_ids = {r["id"] for r in domains["recipes"]["entries"]}
    tech_bad = set()
    for t in domains["technologies"]["entries"]:
        for pr in (t.get("prereqs") or []):
            if pr not in tech_ids:
                tech_bad.add(f"{t['id']}.prereq->{pr}")
        for rc in (t.get("recipes") or []):
            if rc not in recipe_ids:
                tech_bad.add(f"{t['id']}.recipe->{rc}")
    check(not tech_bad, "xref/tech->tech|recipe", f"dangling: {sorted(tech_bad)[:8]}")

    # work sites -> items
    site_bad = set()
    for s in domains["work_sites"]["entries"]:
        if s.get("output_item") and s["output_item"] not in item_ids:
            site_bad.add(f"{s['id']}->{s['output_item']}")
        inp = s.get("input_items")
        if isinstance(inp, list):
            for st in inp:
                if isinstance(st, dict) and st.get("item") not in item_ids:
                    site_bad.add(f"{s['id']}.in->{st.get('item')}")
    check(not site_bad, "xref/worksite->item", f"dangling: {sorted(site_bad)[:8]}")

    # dungeons -> boss species; bosses -> species
    boss_ids = {b["id"] for b in domains["bosses"]["entries"]}
    dg_bad = set()
    for d in domains["dungeons"]["entries"]:
        if d.get("boss") and d["boss"] not in species_ids:
            dg_bad.add(f"{d['id']}->{d['boss']}")
    check(not dg_bad, "xref/dungeon.boss->species", f"dangling: {sorted(dg_bad)[:8]}")
    check(boss_ids <= species_ids, "xref/boss->species",
          f"dangling: {sorted(boss_ids - species_ids)[:8]}")

    # resource nodes -> items
    node_bad = set()
    for nd in domains["resource_nodes"]["entries"]:
        if nd.get("item") and nd["item"] not in item_ids:
            node_bad.add(f"{nd['id']}->{nd['item']}")
    check(not node_bad, "xref/node->item", f"dangling: {sorted(node_bad)[:8]}")

    # bestiary aggregates
    agg = domains["bestiary"]["aggregates"]
    check(agg["species_count"] == len(domains["bestiary"]["species"]),
          "agg/species_count")
    check(sum(agg["families"].values()) == len(domains["bestiary"]["species"]),
          "agg/families sum")
    check(sum(agg["zones"].values()) == len(domains["bestiary"]["species"]),
          "agg/zones sum")
    hostile = sum(1 for s in domains["bestiary"]["species"] if s.get("hostile"))
    check(agg["hostile_count"] == hostile, "agg/hostile_count")

    # census block == validate_final_run.py EXPECTED_CENSUS
    census = domains["counts"]["census_vs_validate_final_run"]
    check(domains["counts"]["census_all_match"] is True and
          all(v["match"] for v in census.values()),
          "xref/census==validate_final_run",
          "; ".join(f"{k}:{v['extracted']}!={v['expected']}" for k, v in
                    census.items() if not v["match"]))

    # zone bijection with bestiary home zones (display names may contain
    # spaces — compare space-stripped)
    zone_names = {z["name"].replace(" ", "") for z in domains["zones"]["entries"]}
    bestiary_zones = set(agg["zones"].keys())
    check(zone_names == bestiary_zones, "xref/zones<->bestiary-home-zones",
          f"sym-diff: {sorted(zone_names ^ bestiary_zones)[:6]}")


# ---------------------------------------------------------------------------
# layer 3: independent recounts
# ---------------------------------------------------------------------------

def validate_recounts(domains: dict) -> None:
    def rg_count(literal: str, path: str) -> int:
        """Fixed-string grep count (BRE-safe: patterns contain parens/quotes)."""
        out = subprocess.run(
            ["grep", "-c", "-F", literal, str(REPO / path)],
            capture_output=True, text=True)
        return int(out.stdout.strip() or 0)

    n_reg_item = rg_count(r"RegisterItem(", "Source/AstrawildCore/Private/AstrawildContentLibrary.cpp")
    check(n_reg_item == 49, "recount/CL RegisterItem",
          f"grep {n_reg_item} vs expected 49 (CL direct calls)")

    n_reg_recipe = rg_count(r"RegisterRecipe(", "Source/AstrawildCore/Private/AstrawildContentLibrary.cpp")
    check(n_reg_recipe == 32, "recount/CL RegisterRecipe",
          f"grep {n_reg_recipe} vs expected 32")

    n_tests = rg_count(r"IMPLEMENT_SIMPLE_AUTOMATION_TEST",
                       "Source/AstrawildCore/Private/AstrawildAutomationTests.cpp")
    json_tests = domains["counts"]["automation_tests"]["value"]
    check(n_tests == json_tests == 134, "recount/automation_tests",
          f"grep {n_tests} vs json {json_tests}")

    n_echo = rg_count('TEXT("Echo_', "Source/AstrawildCore/Private/AstrawildBestiaryData.cpp")
    n_best = len(domains["bestiary"]["species"])
    check(n_echo == n_best == 204, "recount/bestiary rows",
          f"grep {n_echo} vs json {n_best}")

    n_mut = rg_count('TEXT("Echo_', "Source/AstrawildCore/Private/AstrawildEchoMutationData.cpp")
    n_mut_json = domains["mutations"]["count"]
    check(n_mut == n_mut_json == 204, "recount/mutation rows",
          f"grep {n_mut} vs json {n_mut_json}")

    n_ab = rg_count('Table.Add(TEXT("Ability_', "Source/AstrawildCore/Private/AstrawildAbilityLibrary.cpp")
    check(n_ab == domains["abilities"]["count"] == 53, "recount/abilities",
          f"grep {n_ab} vs json {domains['abilities']['count']}")

    n_zones = rg_count('Zones.Add(MakeZone(', "Source/AstrawildCore/Private/AstrawildZoneSubsystem.cpp")
    check(n_zones == domains["zones"]["count"] == 12, "recount/zones",
          f"grep {n_zones} vs json {domains['zones']['count']}")

    n_dg = rg_count('DungeonId = TEXT(', "Source/AstrawildCore/Private/AstrawildWorldBootstrapper.cpp")
    check(n_dg == domains["dungeons"]["count"] == 3, "recount/dungeons",
          f"grep {n_dg} vs json {domains['dungeons']['count']}")

    total_species = n_best + domains["species"]["count"]
    check(total_species == 229, "recount/total species (204 bestiary + 25)",
          f"got {total_species}")

    # the repo's own authoritative gate re-run
    vfr = subprocess.run([sys.executable, "Scripts/validate_final_run.py"],
                         cwd=REPO, capture_output=True, text=True)
    check(vfr.returncode == 0 and "ALL CHECKS PASSED" in vfr.stdout,
          "recount/validate_final_run.py re-run",
          vfr.stdout.strip().splitlines()[-1] if vfr.stdout else "no output")


def main() -> int:
    if not DESIGN.exists():
        print("[validate] FAIL: Design/design_data.json missing — run Scripts/extract_design_data.py")
        return 1
    doc = json.loads(DESIGN.read_text(encoding="utf-8"))
    domains = doc["domains"]

    print("[validate] layer 0: envelope ...")
    validate_envelope(doc)

    print("[validate] layer 1: round-trip (re-open every traced line) ...")
    validate_tunables(domains["tunables"]["entries"])
    validate_bestiary(domains["bestiary"]["species"])
    for dom_name, span in (("species", 30), ("items", 25), ("recipes", 30),
                           ("buildings", 30), ("weapons", 30),
                           ("technologies", 30), ("npcs", 60),
                           ("loot_tables", 30), ("world_events", 25),
                           ("pois", 30), ("resource_nodes", 30),
                           ("work_sites", 40), ("robots", 40),
                           ("dialogue_trees", 5), ("zones", 30),
                           ("weather", 3), ("hunt_contracts", 3),
                           ("abilities", 30), ("mutations", 15),
                           ("dungeons", 60), ("bosses", 3)):
        validate_entries_id(domains, dom_name, span)
    validate_item_attrs(domains["items"]["entries"])

    print("[validate] layer 2: cross-references ...")
    validate_crossrefs(domains)

    print("[validate] layer 3: independent recounts ...")
    validate_recounts(domains)

    print(f"[validate] {CHECKS - len(FAILURES)}/{CHECKS} checks passed")
    if FAILURES:
        print("[validate] FAILURES:")
        for f in FAILURES:
            print(f"  - {f}")
        print("[validate] *** VALIDATION FAILED ***")
        return 1
    print("[validate] ALL CHECKS PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
