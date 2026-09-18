#!/usr/bin/env python3
# ---------------------------------------------------------------------------
# ASTRAWILD — Design Data Extractor (Master Directive v1, roadmap P0-T0.1)
#
# PURPOSE
#   Extracts every authored design value from the C++ source into
#   Design/design_data.json so that NOT ONE number in the design layer is
#   hand-typed. Every value carries a full source trace:
#     { "file": <repo-relative path>, "line": <1-based line>, ... }
#
# WHAT IT EXTRACTS (all from Source/, nothing invented — rule R1)
#   1. tunables  — every UPROPERTY(...)-editable member with a default value
#                  across all Public headers (class, category, clamp meta,
#                  doc comment, type, value, file, line).
#   2. bestiary  — the generated FBestiaryRow table (204 species): identity,
#                  stats, colors, loot, work — each row traced to its line.
#   3. items     — every RegisterItem(MakeItem(...)) call in the content
#                  library (id, name, category, weight, max stack, line).
#   4. recipes   — every RegisterRecipe(MakeRecipe(...)) call (id, name,
#                  inputs, outputs, duration, tech gate, station, line).
#   5. counts    — headline repo counts with traces (automation tests,
#                  species rows, zones, source files, LOC).
#
# USAGE (pure sandbox Python — no Unreal required)
#   python Scripts/extract_design_data.py [--out Design/design_data.json]
#
# OUTPUT SCHEMA  astrawild-design-data/1  (see Design/README.md)
# ---------------------------------------------------------------------------
import argparse
import datetime
import json
import os
import re
import subprocess
import sys

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SOURCE_PUBLIC = os.path.join(REPO_ROOT, "Source", "AstrawildCore", "Public")
SOURCE_PRIVATE = os.path.join(REPO_ROOT, "Source", "AstrawildCore", "Private")

UPROPERTY_RE = re.compile(r"^\s*UPROPERTY\((.*)\)\s*$")
MEMBER_RE = re.compile(
    r"^\s*(?:mutable\s+)?(float|double|int32|int64|uint8|uint32|bool|FVector|FVector2D|FString|FName|FText)"
    r"\s+(\w+)\s*(?:=\s*(.+?))?\s*;\s*(//.*)?$"
)
CLASS_RE = re.compile(r"^\s*class\s+\w*[\w:]*\s*(?:ASTRAWILDCORE_API\s*)?(\w+)\s*:\s*public")
STRUCT_RE = re.compile(r"^\s*struct\s+(?:\w+\s+)?(\w+)")
ENUM_RE = re.compile(r"^\s*UENUM\(.*\)\s*$")
COMMENT_RE = re.compile(r"^\s*/\*\*(.*?)\*/\s*$", re.S)
CATEGORY_RE = re.compile(r'Category\s*=\s*"([^"]+)"')
CLAMP_RE = re.compile(r'(ClampMin|ClampMax)\s*=\s*"([^"]*)"')

VALUE_MAP = {
    "true": True, "false": False, "nullptr": None,
}

TEXT_TOKEN_RE = re.compile(r'TEXT\("((?:[^"\\]|\\.)*)"\)')
ENUM_TOKEN_RE = re.compile(r"E\w+::(\w+)")
NULLPTR_TOKEN_RE = re.compile(r"\bnullptr\b")


def parse_scalar(raw):
    """Parse a C++ default value token into a JSON-safe value."""
    raw = raw.strip().rstrip(";").strip()
    if raw.endswith("f"):
        raw = raw[:-1]
    if raw in VALUE_MAP:
        return VALUE_MAP[raw]
    try:
        if re.match(r"^-?\d+$", raw):
            return int(raw)
        return float(raw)
    except ValueError:
        return raw


def extract_tunables():
    """Pass A — all UPROPERTY default-valued members in Public headers."""
    entries = []
    files_scanned = 0
    for fname in sorted(os.listdir(SOURCE_PUBLIC)):
        if not fname.endswith(".h"):
            continue
        files_scanned += 1
        path = os.path.join(SOURCE_PUBLIC, fname)
        rel = os.path.relpath(path, REPO_ROOT).replace("\\", "/")
        with open(path, encoding="utf-8-sig", errors="replace") as fh:
            lines = fh.readlines()
        current_class = None
        pending_prop = None       # (spec, line_no)
        pending_comment = None
        for i, line in enumerate(lines, start=1):
            cmatch = COMMENT_RE.match(line)
            if cmatch:
                pending_comment = cmatch.group(1).strip()
                continue
            umatch = UPROPERTY_RE.match(line)
            if umatch:
                pending_prop = (umatch.group(1), i)
                continue
            if pending_prop:
                mmatch = MEMBER_RE.match(line)
                if mmatch and mmatch.group(3):
                    spec, prop_line = pending_prop
                    ctype, name, value = mmatch.group(1), mmatch.group(2), mmatch.group(3)
                    cat = CATEGORY_RE.search(spec)
                    clamps = {k: v for k, v in CLAMP_RE.findall(spec)}
                    entries.append({
                        "file": rel,
                        "line": i,
                        "class": current_class,
                        "name": name,
                        "type": ctype,
                        "value": parse_scalar(value),
                        "category": cat.group(1) if cat else None,
                        "clamp": clamps if clamps else None,
                        "comment": pending_comment,
                        "uproperty_line": prop_line,
                    })
                    pending_prop = None
                    pending_comment = None
                    continue
                elif line.strip() and not line.strip().startswith("//"):
                    # Non-member line after UPROPERTY (e.g. another UPROPERTY
                    # or a function) — drop the pending state.
                    if not umatch:
                        pending_prop = None
                        pending_comment = None
            cl = CLASS_RE.match(line)
            if cl:
                current_class = cl.group(1)
                continue
            st = STRUCT_RE.match(line)
            if st:
                current_class = st.group(1)
                continue
    return {"files_scanned": files_scanned, "count": len(entries), "entries": entries}


BESTIARY_ROW_FIELDS = [
    "id", "name", "family", "body_plan", "size_class", "element", "weakness",
    "role", "home_zone", "home_zone_id", "personality", "activity",
    "hp", "atk", "def", "speed", "capture_difficulty", "hostile",
    "primary_r", "primary_g", "primary_b",
    "secondary_r", "secondary_g", "secondary_b",
    "food_a", "food_b", "loot_a", "loot_a_qty", "loot_b", "loot_b_qty",
    "work_a", "work_b", "sight_radius", "work_affinity_a",
]


def extract_bestiary():
    """Pass B — the generated species table in AstrawildBestiaryData.cpp."""
    path = os.path.join(SOURCE_PRIVATE, "AstrawildBestiaryData.cpp")
    rel = os.path.relpath(path, REPO_ROOT).replace("\\", "/")
    with open(path, encoding="utf-8-sig", errors="replace") as fh:
        text = fh.read()
    lines = text.split("\n")

    # Locate the Rows[] table body.
    start = None
    for i, ln in enumerate(lines):
        if re.search(r"static\s+const\s+FBestiaryRow\s+Rows\[\]\s*=\s*\{", ln):
            start = i
            break
    if start is None:
        return {"source": rel, "error": "Rows[] table not found"}

    # Walk balanced braces; record each row as an inclusive 0-based line range.
    depth = 0
    row_ranges = []           # (start_idx, end_idx) inclusive
    row_start_idx = None
    for i in range(start, len(lines)):
        for ch in lines[i]:
            if ch == "{":
                depth += 1
                if depth == 2:
                    row_start_idx = i
            elif ch == "}":
                if depth == 2 and row_start_idx is not None:
                    row_ranges.append((row_start_idx, i))
                    row_start_idx = None
                depth -= 1
                if depth == 0:
                    return _finish_bestiary(rel, lines, row_ranges, start + 1)
    return _finish_bestiary(rel, lines, row_ranges, start + 1)


def _finish_bestiary(rel, lines, row_ranges, table_line):
    species = []
    for start_idx, end_idx in row_ranges:
        chunk = "\n".join(lines[start_idx:end_idx + 1])
        toks = []
        for m in TEXT_TOKEN_RE.finditer(chunk):
            toks.append(("text", m.group(1), m.start()))
        for m in ENUM_TOKEN_RE.finditer(chunk):
            toks.append(("enum", m.group(1), m.start()))
        for m in NULLPTR_TOKEN_RE.finditer(chunk):
            toks.append(("nullptr", None, m.start()))
        for m in re.finditer(r"(?<![\w.])(-?\d+\.\d+f?|-?\d+f?|true|false)(?![\w])", chunk):
            toks.append(("scalar", m.group(1), m.start()))
        toks.sort(key=lambda t: t[2])
        vals = [t[1] for t in toks]
        if len(vals) < len(BESTIARY_ROW_FIELDS):
            continue  # not a full row (or parse drift) — skip rather than invent
        row = {}
        for field, v in zip(BESTIARY_ROW_FIELDS, vals):
            if field in ("hp", "atk", "def", "speed", "capture_difficulty",
                         "primary_r", "primary_g", "primary_b",
                         "secondary_r", "secondary_g", "secondary_b"):
                row[field] = parse_scalar(v)
            elif field in ("loot_a_qty", "loot_b_qty"):
                row[field] = int(parse_scalar(v))
            elif field == "hostile":
                row[field] = v == "true"
            else:
                row[field] = v
        row["file"] = rel
        row["line"] = start_idx + 1
        species.append(row)

    agg = {
        "species_count": len(species),
        "hostile_count": sum(1 for s in species if s["hostile"]),
        "families": {},
        "zones": {},
        "elements": {},
    }
    for s in species:
        agg["families"][s["family"]] = agg["families"].get(s["family"], 0) + 1
        agg["zones"][s["home_zone"]] = agg["zones"].get(s["home_zone"], 0) + 1
        agg["elements"][s["element"]] = agg["elements"].get(s["element"], 0) + 1
    return {
        "source": rel,
        "table_line": table_line,
        "aggregates": agg,
        "species": species,
    }


def _collapse_calls(lines, marker):
    """Yield (start_line_1based, collapsed_statement) for every line-spanning
    statement containing marker, with continuations joined by single spaces."""
    i = 0
    while i < len(lines):
        if marker in lines[i]:
            start = i
            stmt = lines[i].strip()
            depth = stmt.count("(") - stmt.count(")")
            j = i
            while depth > 0 and j + 1 < len(lines):
                j += 1
                stmt += " " + lines[j].strip()
                depth += lines[j].count("(") - lines[j].count(")")
            yield start + 1, stmt
            i = j + 1
        else:
            i += 1


def extract_items():
    """Pass C1 — every item definition in the content library.

    Two authored shapes are captured:
      (a) inline  : Registry->RegisterItem(MakeItem(Outer, TEXT(...), ...));
      (b) variable: UAstrawildItemDefinition* Var = MakeItem(...);
                    Var->Prop = value;   (0..n authored attribute lines)
                    Registry->RegisterItem(Var);
    Attribute setter lines carry their own file:line trace so that per-item
    numbers (FoodValue, AttackPower, PerishableSeconds, ...) are first-class
    traced design values.
    """
    path = os.path.join(SOURCE_PRIVATE, "AstrawildContentLibrary.cpp")
    rel = os.path.relpath(path, REPO_ROOT).replace("\\", "/")
    with open(path, encoding="utf-8-sig", errors="replace") as fh:
        lines = fh.read().split("\n")
    entries = []
    pat = re.compile(
        r'RegisterItem\(\s*MakeItem\(\s*Outer\s*,\s*TEXT\("(.*?)"\)\s*,\s*TEXT\("(.*?)"\)\s*,\s*'
        r"(\w+)::(\w+)\s*,\s*([0-9.]+)f?\s*,\s*(\d+)\s*\)"
    )
    for line_no, stmt in _collapse_calls(lines, "RegisterItem("):
        m = pat.search(stmt)
        if m:
            entries.append({
                "file": rel, "line": line_no,
                "id": m.group(1), "name": m.group(2),
                "category": m.group(4),
                "weight": float(m.group(5)),
                "max_stack": int(m.group(6)),
                "attributes": [],
            })

    # Variable-style definitions + their attribute setters.
    decl_pat = re.compile(
        r'UAstrawildItemDefinition\*\s*(\w+)\s*=\s*MakeItem\(\s*Outer\s*,\s*TEXT\("(.*?)"\)\s*,\s*TEXT\("(.*?)"\)\s*,\s*'
        r"(\w+)::(\w+)\s*,\s*([0-9.]+)f?\s*,\s*(\d+)\s*\)"
    )
    setter_pat = re.compile(r"^(\w+)->(\w+)\s*=\s*([^;]+);")
    pending = {}          # var -> entry dict (collecting attributes)
    for i, line in enumerate(lines, start=1):
        dm = decl_pat.search(line)
        if dm:
            pending[dm.group(1)] = {
                "file": rel, "line": i,
                "id": dm.group(2), "name": dm.group(3),
                "category": dm.group(5),
                "weight": float(dm.group(6)),
                "max_stack": int(dm.group(7)),
                "attributes": [],
            }
            continue
        sm = setter_pat.match(line.strip())
        if sm and sm.group(1) in pending:
            pending[sm.group(1)]["attributes"].append({
                "file": rel, "line": i,
                "prop": sm.group(2),
                "value": parse_scalar(sm.group(3).strip()),
            })
            continue
        rm = re.search(r"RegisterItem\(\s*(\w+)\s*\)", line)
        if rm and rm.group(1) in pending:
            entries.append(pending.pop(rm.group(1)))
    # Any declared-but-unregistered leftovers stay out (never invented).
    return {"source": rel, "count": len(entries), "entries": entries}


def extract_recipes():
    """Pass C2 — RegisterRecipe(MakeRecipe(...)) multi-line calls."""
    path = os.path.join(SOURCE_PRIVATE, "AstrawildContentLibrary.cpp")
    rel = os.path.relpath(path, REPO_ROOT).replace("\\", "/")
    with open(path, encoding="utf-8-sig", errors="replace") as fh:
        lines = fh.read().split("\n")
    entries = []
    for line_no, stmt in _collapse_calls(lines, "RegisterRecipe("):
        m = re.search(
            r'RegisterRecipe\(\s*MakeRecipe\(\s*Outer\s*,\s*TEXT\("(.*?)"\)\s*,\s*TEXT\("(.*?)"\)\s*,\s*'
            r"(\{[^{}]*\})\s*,\s*(\{[^{}]*\})\s*,\s*([0-9.]+)f?\s*,\s*"
            r'(?:TEXT\("(.*?)"\)|\w+)\s*,\s*(?:TEXT\("(.*?)"\)|\w+)\s*\)',
            stmt,
        )
        if m:
            def stacks(chunk):
                out = []
                for mm in re.finditer(r'Stack\(\s*TEXT\("(.*?)"\)\s*,\s*(\d+)\s*\)', chunk):
                    out.append({"item": mm.group(1), "qty": int(mm.group(2))})
                return out
            entries.append({
                "file": rel, "line": line_no,
                "id": m.group(1), "name": m.group(2),
                "inputs": stacks(m.group(3)),
                "outputs": stacks(m.group(4)),
                "duration": float(m.group(5)),
                "tech_gate": m.group(6),
                "station": m.group(7),
            })
    return {"source": rel, "count": len(entries), "entries": entries}


def trace(path_rel, pattern, kind="count", flags=0):
    """Find the first line matching pattern; return a traced count fact."""
    path = os.path.join(REPO_ROOT, path_rel)
    with open(path, encoding="utf-8-sig", errors="replace") as fh:
        for i, line in enumerate(fh, start=1):
            if re.search(pattern, line, flags):
                return {"file": path_rel, "line": i, "kind": kind}
    return {"file": path_rel, "line": None, "kind": kind}


def extract_counts():
    """Pass D — headline repo facts, each with a source trace."""
    tests_path = "Source/AstrawildCore/Private/AstrawildAutomationTests.cpp"
    with open(os.path.join(REPO_ROOT, tests_path), encoding="utf-8-sig", errors="replace") as fh:
        test_count = sum(
            1 for line in fh
            if re.search(r"IMPLEMENT_(SIMPLE|CUSTOM)_AUTOMATION_TEST", line)
        )
    t = trace(tests_path, r"IMPLEMENT_(SIMPLE|CUSTOM)_AUTOMATION_TEST")
    src_files = 0
    src_loc = 0
    for root, _dirs, files in os.walk(os.path.join(REPO_ROOT, "Source")):
        for f in files:
            if f.endswith((".h", ".cpp", ".cs")):
                src_files += 1
                with open(os.path.join(root, f), encoding="utf-8-sig", errors="replace") as fh:
                    src_loc += sum(1 for _ in fh)
    return {
        "automation_tests": {"value": test_count, "file": t["file"], "line": t["line"]},
        "source_files": {"value": src_files},
        "source_loc": {"value": src_loc},
    }


def repo_head():
    try:
        out = subprocess.run(
            ["git", "rev-parse", "HEAD"], cwd=REPO_ROOT,
            capture_output=True, text=True, timeout=15,
        )
        return out.stdout.strip()
    except Exception:
        return None


def main():
    ap = argparse.ArgumentParser(description="ASTRAWILD design data extractor")
    ap.add_argument("--out", default=os.path.join(REPO_ROOT, "Design", "design_data.json"))
    args = ap.parse_args()

    print("[extract] pass A: UPROPERTY tunables across Public headers ...")
    tunables = extract_tunables()
    print(f"[extract]   -> {tunables['count']} tunables from {tunables['files_scanned']} headers")

    print("[extract] pass B: bestiary species table ...")
    bestiary = extract_bestiary()
    print(f"[extract]   -> {bestiary.get('aggregates', {}).get('species_count', 0)} species")

    print("[extract] pass C1: item registry calls ...")
    items = extract_items()
    print(f"[extract]   -> {items['count']} items")

    print("[extract] pass C2: recipe registry calls ...")
    recipes = extract_recipes()
    print(f"[extract]   -> {recipes['count']} recipes")

    print("[extract] pass D: headline counts ...")
    counts = extract_counts()
    print(f"[extract]   -> {counts['automation_tests']['value']} automation tests traced")

    doc = {
        "schema": "astrawild-design-data/1",
        "generated": datetime.datetime.now().isoformat(timespec="seconds"),
        "generator": "Scripts/extract_design_data.py",
        "repo_head": repo_head(),
        "rule": "Every value below was parsed from Source/ by the generator. "
                "No value is hand-typed (Master Directive v1 rule R1).",
        "domains": {
            "tunables": tunables,
            "bestiary": bestiary,
            "items": items,
            "recipes": recipes,
            "counts": counts,
        },
    }
    os.makedirs(os.path.dirname(args.out), exist_ok=True)
    with open(args.out, "w", encoding="utf-8") as fh:
        json.dump(doc, fh, indent=1, ensure_ascii=False)
        fh.write("\n")
    size = os.path.getsize(args.out)
    print(f"[extract] WROTE {os.path.relpath(args.out, REPO_ROOT)} ({size} bytes)")
    print("[extract] DONE")


if __name__ == "__main__":
    sys.exit(main())
