#!/usr/bin/env python3
"""Validate Design/design_data.json against the C++ source it claims to describe.

Two layers of proof (Master Directive v1: R3/R4 — nothing is "verified" by vibes):

  1. ROUND-TRIP — every traced entry is re-opened at its {file, line} and the
     id/name/value is re-read from source. If the JSON drifted from the code,
     this fails loudly.
  2. CROSS-REFERENCE — every reference between domains (recipe->item,
     bestiary loot->item, bestiary food->item, zone ids) must resolve.

Plus independent recounts (grep-level) of the headline numbers.

Sandbox-safe: pure text round-trips over Source/. No Unreal required.

Exit code 0 == ALL CHECKS PASSED. Anything else == failures printed.
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


# ---------------------------------------------------------------------------
# helpers
# ---------------------------------------------------------------------------

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
    """True if a numeric literal equal to `value` appears in `text`."""
    if isinstance(value, bool):
        return ("true" in text) if value else ("false" in text)
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


def token_in_text(text: str, tid: str) -> bool:
    return f'TEXT("{tid}")' in text


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


def validate_species(species: list[dict]) -> None:
    bad = 0
    for s in species:
        # rows span multiple lines; 12-line window from the traced start
        text = window(s["file"], s["line"], 12)
        if not token_in_text(text, s["id"]):
            bad += 1
            if bad <= 5:
                check(False, "roundtrip/species",
                      f"{s['file']}:{s['line']} id={s['id']}")
    check(bad == 0, "roundtrip/species", f"{bad} drifted of {len(species)}")


def validate_items(items: list[dict]) -> None:
    bad = 0
    for it in items:
        text = window(it["file"], it["line"], 25)
        if not token_in_text(text, it["id"]):
            bad += 1
            if bad <= 5:
                check(False, "roundtrip/item",
                      f"{it['file']}:{it['line']} id={it['id']}")
    check(bad == 0, "roundtrip/items", f"{bad} drifted of {len(items)}")

    bad_attr = 0
    n_attr = 0
    for it in items:
        for a in it.get("attributes", []):
            n_attr += 1
            line = window(a["file"], a["line"], 1)
            prop_ok = a["prop"] in line
            val_ok = (num_in_text(line, a["value"])
                      or str(a["value"]).strip('"') in line)
            if not (prop_ok and val_ok):
                bad_attr += 1
                if bad_attr <= 5:
                    check(False, "roundtrip/item-attr",
                          f"{a['file']}:{a['line']} {a['prop']}={a['value']}")
    check(bad_attr == 0, "roundtrip/item-attrs",
          f"{bad_attr} drifted of {n_attr}")


def validate_recipes(recipes: list[dict]) -> None:
    bad = 0
    for r in recipes:
        text = window(r["file"], r["line"], 30)
        if not token_in_text(text, r["id"]):
            bad += 1
            if bad <= 5:
                check(False, "roundtrip/recipe",
                      f"{r['file']}:{r['line']} id={r['id']}")
    check(bad == 0, "roundtrip/recipes", f"{bad} drifted of {len(recipes)}")


# ---------------------------------------------------------------------------
# layer 2: cross-references
# ---------------------------------------------------------------------------

def validate_crossrefs(domains: dict) -> None:
    item_ids = {it["id"] for it in domains["items"]["entries"]}
    species = domains["bestiary"]["species"]
    recipes = domains["recipes"]["entries"]

    # recipes -> items
    dangling = set()
    for r in recipes:
        for io_list in (r.get("inputs", []), r.get("outputs", [])):
            for st in io_list:
                if st.get("item") and st["item"] not in item_ids:
                    dangling.add(f"{r['id']}->{st['item']}")
    check(not dangling, "xref/recipe->item", f"dangling: {sorted(dangling)[:8]}")

    # bestiary loot + food -> items (food may legitimately be a species echo id
    # per the companion-pal design; loot must be items)
    loot_bad, food_notes = set(), 0
    species_ids = {s["id"] for s in species}
    for s in species:
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
          f"{food_notes} unresolved (should be 0; food may be item or species echo)")

    # zone consistency: 1:1 name<->id bijection, and every zone id must
    # actually appear in the zone table (round-trip vs MakeZone lines)
    zones = {s["home_zone"] for s in species}
    zone_ids = {s["home_zone_id"] for s in species}
    check(len(zones) == len(zone_ids), "xref/zone-name<->zone-id bijection",
          f"{len(zones)} names vs {len(zone_ids)} ids")
    zone_src = "\n".join(lines_of("Source/AstrawildCore/Private/AstrawildZoneSubsystem.cpp"))
    unknown_zones = sorted(z for z in zone_ids if f'TEXT("{z}")' not in zone_src)
    check(not unknown_zones, "xref/zone-id registered in AstrawildZoneSubsystem.cpp",
          f"not found in MakeZone table: {unknown_zones}")

    # aggregates
    agg = domains["bestiary"]["aggregates"]
    check(agg["species_count"] == len(species), "agg/species_count")
    check(sum(agg["families"].values()) == len(species), "agg/families sum",
          f"{sum(agg['families'].values())} vs {len(species)}")
    check(sum(agg["zones"].values()) == len(species), "agg/zones sum",
          f"{sum(agg['zones'].values())} vs {len(species)}")
    hostile = sum(1 for s in species if s.get("hostile"))
    check(agg["hostile_count"] == hostile, "agg/hostile_count",
          f"{agg['hostile_count']} vs {hostile}")


# ---------------------------------------------------------------------------
# layer 3: independent recounts (grep-level, not extractor logic)
# ---------------------------------------------------------------------------

def validate_recounts(domains: dict) -> None:
    def rg_count(pattern: str, path: str) -> int:
        out = subprocess.run(
            ["grep", "-c", pattern, str(REPO / path)],
            capture_output=True, text=True)
        return int(out.stdout.strip() or 0)

    n_reg_item = rg_count(r"RegisterItem(", "Source/AstrawildCore/Private/AstrawildContentLibrary.cpp")
    check(n_reg_item == domains["items"]["count"],
          "recount/RegisterItem", f"grep {n_reg_item} vs json {domains['items']['count']}")

    n_reg_recipe = rg_count(r"RegisterRecipe(", "Source/AstrawildCore/Private/AstrawildContentLibrary.cpp")
    check(n_reg_recipe == domains["recipes"]["count"],
          "recount/RegisterRecipe", f"grep {n_reg_recipe} vs json {domains['recipes']['count']}")

    n_tests = rg_count(r"IMPLEMENT_SIMPLE_AUTOMATION_TEST",
                       "Source/AstrawildCore/Private/AstrawildAutomationTests.cpp")
    json_tests = domains["counts"]["automation_tests"]["value"]
    check(n_tests == json_tests, "recount/automation_tests",
          f"grep {n_tests} vs json {json_tests}")

    n_echo = rg_count(r'TEXT("Echo_', "Source/AstrawildCore/Private/AstrawildBestiaryData.cpp")
    n_species = len(domains["bestiary"]["species"])
    check(n_echo == n_species, "recount/bestiary rows",
          f"grep {n_echo} vs json {n_species}")

    n_tun = domains["tunables"]["count"]
    check(n_tun == len(domains["tunables"]["entries"]),
          "recount/tunables count field", f"{n_tun} vs {len(domains['tunables']['entries'])}")

    src_files = sum(1 for p in (REPO / "Source").rglob("*")
                    if p.suffix in (".h", ".cpp", ".cs"))
    check(src_files == domains["counts"]["source_files"]["value"],
          "recount/source_files", f"walk {src_files} vs json {domains['counts']['source_files']['value']}")


# ---------------------------------------------------------------------------
# layer 0: envelope
# ---------------------------------------------------------------------------

def validate_envelope(doc: dict) -> None:
    check(doc.get("schema") == "astrawild-design-data/1", "env/schema",
          str(doc.get("schema")))
    head = subprocess.run(["git", "rev-parse", "HEAD"], cwd=REPO,
                          capture_output=True, text=True).stdout.strip()
    if doc.get("repo_head") == head:
        check(True, "env/repo_head")
    else:
        # The JSON commit itself moves HEAD. Soft-pass iff the recorded head
        # is an ancestor of HEAD AND no Source/ file changed in between.
        anc = subprocess.run(["git", "merge-base", "--is-ancestor",
                              doc.get("repo_head", ""), head], cwd=REPO)
        diff = subprocess.run(["git", "diff", "--name-only", doc.get("repo_head", ""), head,
                               "--", "Source"], cwd=REPO,
                              capture_output=True, text=True).stdout.strip()
        check(anc.returncode == 0 and not diff, "env/repo_head",
              f"json {str(doc.get('repo_head'))[:8]} is not a clean ancestor of "
              f"HEAD {head[:8]} (Source/ changed: {diff.splitlines()[:3]}) — "
              "regenerate")


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
    validate_species(domains["bestiary"]["species"])
    validate_items(domains["items"]["entries"])
    validate_recipes(domains["recipes"]["entries"])

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
