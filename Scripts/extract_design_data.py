#!/usr/bin/env python3
# ---------------------------------------------------------------------------
# ASTRAWILD — Design Data Extractor v2 (LONG-RUN DIRECTIVE L2)
#
# PURPOSE
#   Extracts every authored design value from the C++ source into
#   Design/design_data.json so that NOT ONE number in the design layer is
#   hand-typed. Every value carries a full source trace:
#     { "file": <repo-relative path>, "line": <1-based line>, ... }
#
# WHAT IT EXTRACTS (all from Source/, nothing invented — rule R1)
#   tunables       — every UPROPERTY(...)-editable member with a default value
#                    across all Public headers.
#   bestiary       — the generated 204-row species table.
#   species        — the 25 non-bestiary species: 10 CL authored + 9 PC
#                    production heroes + 6 evolution targets.
#   items (78)     — 49 CL + 29 PC, with per-attribute setter traces.
#   recipes (58)   — 32 CL + 26 PC.
#   buildings(26)  weapons(8)  technologies(17)  quests(22)  npcs(13)
#   loot_tables(11) world_events(16) pois(17) resource_nodes(10)
#   work_sites(8)  robots(3)   dialogue_trees(13) zones(12)
#   weather(8)     hunt_contracts(8) abilities(53) mutations(204)
#   dungeons(3)    bosses(4)   save_schema(29 fields + 20 record structs)
#   counts         — headline repo facts + census reconciliation.
#
# USAGE (pure sandbox Python — no Unreal required)
#   python Scripts/extract_design_data.py [--out Design/design_data.json]
#
# OUTPUT SCHEMA  astrawild-design-data/2  (see Design/README.md)
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

CL = "Source/AstrawildCore/Private/AstrawildContentLibrary.cpp"
PC = "Source/AstrawildCore/Private/AstrawildProductionContent.cpp"
BD = "Source/AstrawildCore/Private/AstrawildBestiaryData.cpp"
WB = "Source/AstrawildCore/Private/AstrawildWorldBootstrapper.cpp"
ZS = "Source/AstrawildCore/Private/AstrawildZoneSubsystem.cpp"
WS = "Source/AstrawildCore/Private/AstrawildWeatherSubsystem.cpp"
HS = "Source/AstrawildCore/Private/AstrawildHuntSubsystem.cpp"
AL = "Source/AstrawildCore/Private/AstrawildAbilityLibrary.cpp"
MD = "Source/AstrawildCore/Private/AstrawildEchoMutationData.cpp"
SS = "Source/AstrawildCore/Public/AstrawildSaveSubsystem.h"

UPROPERTY_RE = re.compile(r"^\s*UPROPERTY\((.*)\)\s*$")
MEMBER_RE = re.compile(
    r"^\s*(?:mutable\s+)?(float|double|int32|int64|uint8|uint32|bool|FVector|FVector2D|FString|FName|FText)"
    r"\s+(\w+)\s*(?:=\s*(.+?))?\s*;\s*(//.*)?$"
)
CLASS_RE = re.compile(r"^\s*class\s+\w*[\w:]*\s*(?:ASTRAWILDCORE_API\s*)?(\w+)\s*:\s*public")
STRUCT_RE = re.compile(r"^\s*struct\s+(?:\w+\s+)?(\w+)")
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


def _load(rel):
    path = os.path.join(REPO_ROOT, rel)
    with open(path, encoding="utf-8-sig", errors="replace") as fh:
        return fh.read().split("\n")


# ---------------------------------------------------------------------------
# Generic call-argument machinery (string-literal aware)
# ---------------------------------------------------------------------------

def _split_top_args(s):
    """Split a comma-separated C++ argument list at TOP level only
    (parentheses/braces nest; string literals are respected)."""
    args, buf, depth = [], [], 0
    in_str = False
    i = 0
    while i < len(s):
        ch = s[i]
        if in_str:
            buf.append(ch)
            if ch == '"' and s[i - 1] != "\\":
                in_str = False
        elif ch == '"':
            in_str = True
            buf.append(ch)
        elif ch in "({":
            depth += 1
            buf.append(ch)
        elif ch in ")}":
            depth -= 1
            buf.append(ch)
        elif ch == "," and depth == 0:
            args.append("".join(buf).strip())
            buf = []
        else:
            buf.append(ch)
        i += 1
    tail = "".join(buf).strip()
    if tail:
        args.append(tail)
    return args


_STACK_RE = re.compile(r'^Stack\(\s*TEXT\("(.*)"\)\s*,\s*(\d+)\s*\)$')
_TEXTARG_RE = re.compile(r'^TEXT\("((?:[^"\\]|\\.)*)"\)$')
_ENUMARG_RE = re.compile(r"^([A-Z]\w*)::(\w+)$")
_FLIN_RE = re.compile(r"^FLinearColor\((.*)\)$")
_FVEC2_RE = re.compile(r"^FVector2D\((.*)\)$")


def _parse_value(arg):
    """Parse one C++ call argument into a JSON-safe value (never invents)."""
    arg = arg.strip().rstrip(",").strip()
    if not arg:
        return None
    if arg in ("NAME_None", "nullptr"):
        return None
    if arg == "true":
        return True
    if arg == "false":
        return False
    m = _TEXTARG_RE.match(arg)
    if m:
        return m.group(1)
    m = _ENUMARG_RE.match(arg)
    if m:
        return m.group(2)
    m = _STACK_RE.match(arg)
    if m:
        return {"item": m.group(1), "qty": int(m.group(2))}
    if re.match(r"^0x[0-9A-Fa-f]+$", arg):
        return int(arg, 16)
    if re.match(r"^-?\d+$", arg):
        return int(arg)
    if re.match(r"^-?\d+\.\d+f?$", arg):
        return float(arg.rstrip("f"))
    m = _FLIN_RE.match(arg)
    if m:
        return [float(x.strip().rstrip("f")) for x in m.group(1).split(",") if x.strip()][:4]
    m = _FVEC2_RE.match(arg)
    if m:
        return [float(x.strip().rstrip("f")) for x in m.group(1).split(",") if x.strip()][:2]
    if arg.startswith("{") and arg.endswith("}"):
        inner = arg[1:-1].strip()
        if not inner:
            return []
        return [_parse_value(x) for x in _split_top_args(inner)]
    # FText::FromString(TEXT("...")) and friends — take the first TEXT token.
    tm = TEXT_TOKEN_RE.search(arg)
    if tm and arg.count("TEXT(") == 1:
        return tm.group(1)
    return arg  # raw fallback (still traced, never invented)


def _call_args_span(lines, helper):
    """Yield (line_no_1based, args_list) for every `helper(...)` CALL site
    (multi-line, string-safe). Definition lines are skipped by detecting a
    typed first parameter (`UObject*`, `UAstrawild...* Registry`, `const ...&`)."""
    i = 0
    n = len(lines)
    while i < n:
        m = re.search(r"\b" + re.escape(helper) + r"\s*\(", lines[i])
        if not m:
            i += 1
            continue
        # collapse the statement (string-aware paren balance)
        start = i
        depth = 0
        started = False
        j = i
        while j < n:
            s = lines[j]
            k = 0
            in_str = False
            while k < len(s):
                ch = s[k]
                if in_str:
                    if ch == '"' and s[k - 1] != "\\":
                        in_str = False
                elif ch == '"':
                    in_str = True
                elif ch == "(":
                    depth += 1
                    started = True
                elif ch == ")":
                    depth -= 1
                k += 1
            if started and depth <= 0:
                break
            j += 1
        stmt = " ".join(x.strip() for x in lines[start:j + 1])
        # locate the helper's own opening paren
        hm = re.search(r"\b" + re.escape(helper) + r"\s*\(", stmt)
        if hm:
            inner_start = hm.end()
            # depth-walk to the matching close paren (string-aware)
            d = 0
            in_str = False
            end = None
            k = inner_start
            while k < len(stmt):
                ch = stmt[k]
                if in_str:
                    if ch == '"' and stmt[k - 1] != "\\":
                        in_str = False
                elif ch == '"':
                    in_str = True
                elif ch == "(":
                    d += 1
                elif ch == ")":
                    if d == 0:
                        end = k
                        break
                    d -= 1
                k += 1
            if end is not None:
                raw_args = stmt[inner_start:end]
                args = _split_top_args(raw_args)
                # skip helper DEFINITION lines: first param is TYPED
                # (e.g. `UObject* Outer`, `const EAstrawildZone Zone`,
                #  `const TCHAR* Id`) — two identifier tokens, no call syntax.
                first = args[0] if args else ""
                is_definition = bool(
                    re.match(r"^(?:const\s+)?[A-Za-z_]\w*(?:\s*\*\s*|\s+)[A-Za-z_]\w*$",
                             first)
                ) and "::" not in first and "(" not in first
                if not is_definition:
                    yield start + 1, args
        i = j + 1


def _helper_domain(rel, helper, fields, numeric=(), lists=()):
    """Extract a helper-call domain: one entry per call. `fields` names the
    args AFTER the (Registry/Outer, id) pair; the id itself lands in
    entry["id"]. Every entry is traced to the call's first line."""
    lines = _load(rel)
    entries = []
    for line_no, args in _call_args_span(lines, helper):
        if len(args) < 3:                     # Registry/Outer + id + >=1 field
            continue                          # parse drift — skip, never invent
        entry = {"file": rel, "line": line_no, "id": _parse_value(args[1])}
        for idx, field in enumerate(fields):
            if idx + 2 >= len(args):
                entry[field] = None           # defaulted param — absent at call site
                continue
            v = _parse_value(args[idx + 2])
            if field in numeric and isinstance(v, (int, float)):
                v = float(v)
            entry[field] = v
        entries.append(entry)
    return {"source": rel, "helper": helper, "count": len(entries), "entries": entries}


def _setter_value(raw):
    """Parse the RHS of `Var->Prop = raw;` into a JSON value."""
    raw = raw.strip().rstrip(";").strip()
    return _parse_value(raw)


def _var_style_domain(rel, decl_type, id_prop, register_call, extra_sub=None):
    """Extract a variable-style domain:
        <decl_type>* Var = <NewObject|Make...>(...);
        Var->Prop = value;      (every line traced)
        [sub-struct blocks + Var->List.Add(Obj) handled via extra_sub]
        Registry->RegisterX(Var);
    Returns entries: {file, line(id line), id, attributes: [{prop, value, line}...]}.

    GUARD: entries whose <id_prop> RHS is not a TEXT("...") literal are
    dropped -- those are helper-function bodies assigning their parameters
    (e.g. `Item->ItemId = Id;` inside MakeItem), never real content.
    """
    lines = _load(rel)
    entries = []
    current = None            # var -> entry
    decl_re = re.compile(
        r"^\s*" + re.escape(decl_type) + r"\s*\*\s*(\w+)\s*=")
    setter_re = re.compile(r"^(\w+)->(\w+)\s*=\s*(.+);$")
    register_re = re.compile(register_call)
    pending = {}
    for i, line in enumerate(lines, start=1):
        dm = decl_re.match(line)
        if dm:
            pending[dm.group(1)] = {"var": dm.group(1), "line": i,
                                    "attributes": [], "id": None,
                                    "id_literal": False,
                                    "adds": []}
            continue
        sm = setter_re.match(line.strip())
        if sm and sm.group(1) in pending:
            prop, raw = sm.group(2), sm.group(3)
            val = _setter_value(raw)
            pending[sm.group(1)]["attributes"].append(
                {"prop": prop, "value": val, "line": i})
            if prop == id_prop:
                if _TEXTARG_RE.match(raw.strip()):
                    pending[sm.group(1)]["id"] = val
                    pending[sm.group(1)]["id_literal"] = True
                    pending[sm.group(1)]["id_line"] = i
            continue
        am = re.match(r"^(\w+)->(\w+)\.Add\((.+)\);$", line.strip())
        if am and am.group(1) in pending:
            pending[am.group(1)]["adds"].append(
                {"list": am.group(2), "value": _parse_value(am.group(3)),
                 "line": i})
            continue
        rm = register_re.search(line)
        if rm:
            var = rm.group(1)
            if var in pending:
                e = pending.pop(var)
                if e["id_literal"]:
                    entries.append(e)
    return {"source": rel, "count": len(entries), "entries": entries}


# ---------------------------------------------------------------------------
# Pass A — tunables (unchanged from v1)
# ---------------------------------------------------------------------------

def extract_tunables():
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
        pending_prop = None
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


# ---------------------------------------------------------------------------
# Pass B — bestiary (unchanged core from v1)
# ---------------------------------------------------------------------------

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
    lines = _load(BD)
    start = None
    for i, ln in enumerate(lines):
        if re.search(r"static\s+const\s+FBestiaryRow\s+Rows\[\]\s*=\s*\{", ln):
            start = i
            break
    if start is None:
        return {"source": BD, "error": "Rows[] table not found"}
    depth = 0
    row_ranges = []
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
                    return _finish_bestiary(BD, lines, row_ranges, start + 1)
    return _finish_bestiary(BD, lines, row_ranges, start + 1)


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
            continue
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

# ---------------------------------------------------------------------------
# Pass C — species (25 non-bestiary), items (78), recipes (58)
# ---------------------------------------------------------------------------

MAKE_ECHO_FIELDS = [
    "name", "element", "role", "hp", "atk", "def", "speed", "personality",
    "activity", "food", "capture_difficulty", "weakness", "hostile",
]


def extract_species():
    """The 25 non-bestiary species: 10 CL MakeEcho + 9 PC MakeProductionEcho
    + 6 evolution targets (born from the FEvolutionSpec table)."""
    entries = []

    # --- CL authored (MakeEcho, variable-style with setters) ---
    lines = _load(CL)
    for line_no, args in _call_args_span(lines, "MakeEcho"):
        if len(args) < len(MAKE_ECHO_FIELDS) + 2:
            continue
        vals = args[2:]   # drop Outer + id
        e = {"file": CL, "line": line_no, "id": _parse_value(args[1]),
             "origin": "authored"}
        for idx, field in enumerate(MAKE_ECHO_FIELDS):
            e[field] = _parse_value(vals[idx])
        e["attributes"] = []
        entries.append(e)
    # CL species attribute setters (PreferredWeather, AbilityIds, ...) —
    # scan var-style setters between decl and RegisterEcho.
    _attach_setters(CL, entries, r"UAstrawildEchoDefinition\s*\*\s*(\w+)\s*=\s*MakeEcho\(",
                    r"Registry->RegisterEcho\(\s*(\w+)\s*\)")

    # --- PC production heroes (MakeProductionEcho + setters) ---
    lines = _load(PC)
    PC_ECHO_FIELDS = [
        "name", "element", "role", "hp", "atk", "def", "speed",
        "personality", "activity", "food", "capture_difficulty",
        "weakness", "family", "body_plan", "size_class", "home_zone",
        "rarity", "passive", "work_affinities", "loot",
    ]
    for line_no, args in _call_args_span(lines, "MakeProductionEcho"):
        if len(args) < len(PC_ECHO_FIELDS) + 2:
            continue
        vals = args[2:]
        e = {"file": PC, "line": line_no, "id": _parse_value(args[1]),
             "origin": "production"}
        for idx, field in enumerate(PC_ECHO_FIELDS):
            e[field] = _parse_value(vals[idx])
        e["attributes"] = []
        entries.append(e)
    _attach_setters(PC, entries,
                    r"UAstrawildEchoDefinition\s*\*\s*(\w+)\s*=\s*MakeProductionEcho\(",
                    r"Registry->RegisterEcho\(\s*(\w+)\s*\)")

    # --- Evolution targets (FEvolutionSpec rows + the loop that registers) ---
    evo = _extract_evolution_targets()
    entries.extend(evo)

    return {"sources": [CL, PC], "count": len(entries), "entries": entries}


def _attach_setters(rel, entries, decl_pat, register_pat):
    """Attach `Var->Prop = value;` lines (each traced) to matching entries by
    variable name, between the entry's decl line and its Register line."""
    lines = _load(rel)
    decl_re = re.compile(decl_pat)
    setter_re = re.compile(r"^(\w+)->(\w+)\s*=\s*(.+);$")
    add_re = re.compile(r"^(\w+)->(\w+)\.Add\((.+)\);$")
    register_re = re.compile(register_pat)
    var_to_entry = {}
    for i, line in enumerate(lines, start=1):
        dm = decl_re.match(line)
        if dm:
            var = dm.group(1)
            # find the entry whose line matches this decl line
            for e in entries:
                if e["file"] == rel and e["line"] == i:
                    var_to_entry[var] = e
                    break
            continue
        sm = setter_re.match(line.strip())
        if sm and sm.group(1) in var_to_entry:
            var_to_entry[sm.group(1)]["attributes"].append({
                "file": rel, "line": i, "prop": sm.group(2),
                "value": _setter_value(sm.group(3)),
            })
            continue
        am = add_re.match(line.strip())
        if am and am.group(1) in var_to_entry:
            var_to_entry[am.group(1)]["attributes"].append({
                "file": rel, "line": i, "prop": am.group(2) + ".Add",
                "value": _parse_value(am.group(3)),
            })
            continue
        rm = register_re.search(line)
        if rm and rm.group(1) in var_to_entry:
            del var_to_entry[rm.group(1)]


def _extract_evolution_targets():
    """The 6 evolution targets born from the FEvolutionSpec table. Stats are
    DERIVED by the registration loop (formulas, not literals) — traced to the
    loop lines; the gates (level/bond) are literals traced to the spec rows."""
    lines = _load(PC)
    entries = []
    # spec table rows
    row_re = re.compile(
        r'\{\s*TEXT\("(Echo_\w+)"\)\s*,\s*TEXT\("(Echo_\w+)"\)\s*,\s*'
        r'TEXT\("(.*)"\)\s*,\s*TEXT\("(.*)"\)\s*,\s*(\d+)\s*,\s*'
        r"([\d.]+)f?\s*\}")
    for i, line in enumerate(lines, start=1):
        m = row_re.search(line)
        if m:
            entries.append({
                "file": PC, "line": i,
                "id": m.group(2),
                "base_species": m.group(1),
                "name": m.group(3),
                "evolved_from": m.group(4),
                "level_gate": int(m.group(5)),
                "bond_gate": float(m.group(6)),
                "origin": "evolution_target",
                "note": "stats derived at runtime by BuildEvolutionTargets "
                        "loop (PC lines ~1435-1494: +35..40% HP, ~+30% "
                        "ATK/DEF, rarity/size promotion, affinity +0.1) — "
                        "formulas, not literals",
            })
    return entries


def extract_items():
    """78 items: 49 CL + 29 PC.

    ALL items (inline or variable-style) pass through a MakeItem(...) call,
    which _call_args_span captures positionally; variable-style items then
    get their post-decl setter lines attached as traced attributes.
    """
    entries = []
    ITEM_FIELDS = ["name", "category", "weight", "max_stack"]
    for rel in (CL, PC):
        lines = _load(rel)
        for line_no, args in _call_args_span(lines, "MakeItem"):
            if len(args) < len(ITEM_FIELDS) + 2:
                continue
            e = {"file": rel, "line": line_no, "id": _parse_value(args[1])}
            for idx, field in enumerate(ITEM_FIELDS):
                v = _parse_value(args[idx + 2])
                if field == "weight":
                    v = float(v)
                elif field == "max_stack":
                    v = int(v)
                e[field] = v
            e["attributes"] = []
            entries.append(e)
        # variable-style post-decl setters (each traced)
        _attach_setters(rel, entries,
                        r"UAstrawildItemDefinition\s*\*\s*(\w+)\s*=\s*MakeItem\(",
                        r"Registry->RegisterItem\(\s*(\w+)\s*\)")
    return {"sources": [CL, PC], "count": len(entries), "entries": entries}


def extract_recipes():
    """58 recipes: 32 CL + 26 PC."""
    entries = []
    RECIPE_FIELDS = ["name", "inputs", "outputs", "duration",
                     "tech_gate", "station"]
    for rel in (CL, PC):
        lines = _load(rel)
        for line_no, args in _call_args_span(lines, "MakeRecipe"):
            if len(args) < len(RECIPE_FIELDS) + 2:
                continue
            e = {"file": rel, "line": line_no, "id": _parse_value(args[1])}
            for idx, field in enumerate(RECIPE_FIELDS):
                v = _parse_value(args[idx + 2])
                if field == "duration" and isinstance(v, (int, float)):
                    v = float(v)
                e[field] = v
            entries.append(e)
    return {"sources": [CL, PC], "count": len(entries), "entries": entries}


def extract_buildings():
    """26 buildings (CL inline MakeBuilding)."""
    return _helper_domain(
        CL, "MakeBuilding",
        ["name", "category", "required_item", "item_count", "tech", "hp",
         "power_role", "generation", "draw", "battery", "work_type"],
        numeric=("hp", "generation", "draw", "battery"))


def extract_weapons():
    """8 weapons (PC MakeWeapon + setter attributes)."""
    dom = _helper_domain(
        PC, "MakeWeapon",
        ["name", "family", "tier", "rarity", "fire_mode", "damage",
         "fire_interval", "element", "ammo"],
        numeric=("damage", "fire_interval"))
    _attach_setters(PC, dom["entries"],
                    r"UAstrawildWeaponDefinition\s*\*\s*(\w+)\s*=\s*MakeWeapon\(",
                    r"Registry->RegisterWeapon\(\s*(\w+)\s*\)")
    return dom


def extract_technologies():
    """17 technologies: 10 CL (MakeTech w/ Outer, 8 fields) + 7 PC
    (MakeTech w/ Registry + branch field)."""
    entries = []
    lines = _load(CL)
    CL_FIELDS = ["name", "era", "cost", "prereqs", "recipes", "buildings"]
    for line_no, args in _call_args_span(lines, "MakeTech"):
        if len(args) < len(CL_FIELDS) + 2:
            continue
        e = {"file": CL, "line": line_no, "id": _parse_value(args[1])}
        for idx, field in enumerate(CL_FIELDS):
            v = _parse_value(args[idx + 2])
            if field == "cost":
                v = int(v) if isinstance(v, (int, float)) else v
            e[field] = v
        entries.append(e)
    lines = _load(PC)
    PC_FIELDS = ["name", "era", "branch", "cost", "prereqs", "recipes"]
    for line_no, args in _call_args_span(lines, "MakeTech"):
        if len(args) < len(PC_FIELDS) + 2:
            continue
        e = {"file": PC, "line": line_no, "id": _parse_value(args[1])}
        for idx, field in enumerate(PC_FIELDS):
            v = _parse_value(args[idx + 2])
            if field == "cost":
                v = int(v) if isinstance(v, (int, float)) else v
            e[field] = v
        entries.append(e)
    return {"sources": [CL, PC], "count": len(entries), "entries": entries}

# ---------------------------------------------------------------------------
# Pass D — variable-style domains (quests, npcs, sites, robots, dialogue)
# ---------------------------------------------------------------------------

QUEST_SETTER_MAP = {
    "Title": "title", "Summary": "summary", "NextQuestId": "next_quest",
    "RewardResearchPoints": "reward_research_points",
    "Act": "act", "bPostGameOnly": "post_game_only",
}


def extract_quests():
    """22 quests: 10 CL + 12 PC variable-style. Captures id, title, summary,
    objectives (type/target/count/text), reward items, next quest — every
    value traced to its setter line."""
    entries = []
    for rel in (CL, PC):
        lines = _load(rel)
        current_var = None
        current = None
        obj_var = None
        obj = None
        decl_re = re.compile(
            r"^\s*UAstrawildQuestDefinition\s*\*\s*(\w+)\s*=")
        obj_decl_re = re.compile(r"^\s*FAstrawildQuestObjective\s+(\w+)\s*;")
        setter_re = re.compile(r"^(\w+)->(\w+)\s*=\s*(.+);$")
        obj_setter_re = re.compile(r"^(\w+)\.(\w+)\s*=\s*(.+);$")
        obj_add_re = re.compile(r"^(\w+)->Objectives\.Add\((\w+)\);$")
        reward_add_re = re.compile(
            r"^(\w+)->RewardItems\.Add\(\s*Stack\(\s*TEXT\(\"(.*)\"\)\s*,\s*(\d+)\s*\)\s*\);$")
        register_re = re.compile(r"RegisterQuest\(\s*(\w+)\s*\)")
        id_set_re = re.compile(
            r'^(\w+)->QuestId\s*=\s*TEXT\("(.*)"\);$')
        for i, line in enumerate(lines, start=1):
            s = line.strip()
            dm = decl_re.match(line)
            if dm:
                current_var = dm.group(1)
                current = {"file": rel, "line": i, "id": None,
                           "objectives": [], "reward_items": [],
                           "attributes": []}
                continue
            im = id_set_re.match(s)
            if im and current is not None and im.group(1) == current_var:
                current["id"] = im.group(2)
                current["line"] = i      # trace anchors on the QuestId line
                continue
            od = obj_decl_re.match(line)
            if od and current is not None:
                obj_var = od.group(1)
                obj = {"file": rel, "line": i, "type": None, "target": None,
                       "count": None, "text": None}
                continue
            oa = obj_add_re.match(s)
            if oa and current is not None and oa.group(1) == current_var and obj is not None:
                current["objectives"].append(obj)
                obj = None
                continue
            ra = reward_add_re.match(s)
            if ra and current is not None and ra.group(1) == current_var:
                current["reward_items"].append(
                    {"item": ra.group(2), "qty": int(ra.group(3)), "line": i})
                continue
            os = obj_setter_re.match(s)
            if os and obj is not None and os.group(1) == obj_var:
                prop, val = os.group(2), _setter_value(os.group(3))
                key = {"Type": "type", "TargetId": "target",
                       "RequiredCount": "count", "ObjectiveText": "text"}.get(prop)
                if key:
                    obj[key] = val if key != "count" else int(val)
                continue
            sm = setter_re.match(s)
            if sm and current is not None and sm.group(1) == current_var:
                prop, val = sm.group(2), _setter_value(sm.group(3))
                key = QUEST_SETTER_MAP.get(prop)
                if key:
                    current[key] = val
                else:
                    current["attributes"].append(
                        {"file": rel, "line": i, "prop": prop, "value": val})
                continue
            rm = register_re.search(line)
            if rm and current is not None and rm.group(1) == current_var:
                if current["id"] is not None:
                    entries.append(current)
                current = None
                current_var = None
    return {"sources": [CL, PC], "count": len(entries), "entries": entries}


def extract_npcs():
    """13 NPCs (CL variable-style)."""
    dom = _var_style_domain(CL, "UAstrawildNPCDefinition", "NpcId",
                            r"RegisterNPC\(\s*(\w+)\s*\)")
    return _shape_var_domain(dom, CL, {
        "DisplayName": "name", "OfferedQuestId": "offered_quest",
        "DialogueTreeId": "dialogue_tree", "Role": "role",
        "VillageId": "village", "PrimaryTint": "tint", "Greeting": "greeting",
    }, id_prop="NpcId")


def _shape_var_domain(dom, rel, prop_map, id_prop=None):
    """Reshape a _var_style_domain result: named fields from setters,
    remaining setters become traced attributes. The id-prop setter itself
    is dropped from attributes (it IS the id)."""
    out = []
    for e in dom["entries"]:
        entry = {"file": rel, "line": e["id_line"], "id": e["id"]}
        attrs = []
        for a in e["attributes"]:
            if id_prop and a["prop"] == id_prop:
                continue
            if a["prop"] in prop_map:
                entry[prop_map[a["prop"]]] = a["value"]
            else:
                attrs.append({"file": rel, "line": a["line"],
                              "prop": a["prop"], "value": a["value"]})
        entry["attributes"] = attrs
        out.append(entry)
    return {"source": rel, "count": len(out), "entries": out}


def extract_work_sites():
    """8 work sites (PC variable-style)."""
    dom = _var_style_domain(PC, "UAstrawildWorkSiteDefinition", "SiteId",
                            r"RegisterWorkSite\(\s*(\w+)\s*\)")
    return _shape_var_domain(dom, PC, {
        "DisplayName": "name", "WorkType": "work_type",
        "OutputItemId": "output_item", "OutputQuantity": "output_qty",
        "InputItems": "input_items", "SecondsPerOutput": "seconds_per_output",
        "Zone": "zone", "OffsetFromZoneCenter": "offset",
    }, id_prop="SiteId")


def extract_robots():
    """3 robots (PC variable-style NewObject + setters)."""
    dom = _var_style_domain(PC, "UAstrawildRobotDefinition", "RobotId",
                            r"RegisterRobot\(\s*(\w+)\s*\)")
    return _shape_var_domain(dom, PC, {
        "DisplayName": "name", "Description": "description",
        "PrimaryWorkType": "primary_work_type",
        "SpecialistWorkRate": "specialist_rate", "GenericWorkRate": "generic_rate",
        "MoveSpeedMultiplier": "move_speed_mult", "PrimaryTint": "tint",
        "VendorPrice": "vendor_price",
    }, id_prop="RobotId")


def extract_dialogue_trees():
    """13 dialogue trees (PC variable-style) — id + node count + line traces."""
    lines = _load(PC)
    entries = []
    decl_re = re.compile(
        r"^\s*UAstrawildDialogueTreeDefinition\s*\*\s*(\w+)\s*=")
    id_re = re.compile(r'^(\w+)->DialogueId\s*=\s*TEXT\("(.*)"\);$')
    node_add_re = re.compile(r"^(\w+)->Nodes\.Add\((\w+)\);$")
    register_re = re.compile(r"RegisterDialogueTree\(\s*(\w+)\s*\)")
    current = None
    for i, line in enumerate(lines, start=1):
        s = line.strip()
        dm = decl_re.match(line)
        if dm:
            current = {"var": dm.group(1), "file": PC, "line": i,
                       "id": None, "id_line": None, "nodes": 0}
            continue
        im = id_re.match(s)
        if im and current and im.group(1) == current["var"]:
            current["id"] = im.group(2)
            current["id_line"] = i
            continue
        na = node_add_re.match(s)
        if na and current and na.group(1) == current["var"]:
            current["nodes"] += 1
            continue
        rm = register_re.search(line)
        if rm and current and rm.group(1) == current["var"]:
            if current["id"]:
                entries.append({
                    "file": PC, "line": current["id_line"],
                    "id": current["id"], "node_count": current["nodes"],
                })
            current = None
    return {"source": PC, "count": len(entries), "entries": entries}


# ---------------------------------------------------------------------------
# Pass E — helper-call domains in PC + table domains
# ---------------------------------------------------------------------------

def extract_loot_tables():
    """11 loot tables: 5 CL variable-style + 6 PC MakeLoot."""
    entries = []
    dom = _var_style_domain(CL, "UAstrawildLootTableDefinition", "LootTableId",
                            r"RegisterLootTable\(\s*(\w+)\s*\)")
    for e in dom["entries"]:
        entry = {"file": CL, "line": e["id_line"], "id": e["id"],
                 "guaranteed_drops": [], "bonus_chance": None}
        for a in e["attributes"]:
            if a["prop"] == "GuaranteedDrops":
                v = a["value"]
                entry["guaranteed_drops"] = [
                    x for x in (v if isinstance(v, list) else [v])
                    if isinstance(x, dict)]
                entry["drops_line"] = a["line"]
            elif a["prop"] == "BonusRollChance":
                entry["bonus_chance"] = float(a["value"])
                entry["bonus_line"] = a["line"]
        entries.append(entry)
    pc = _helper_domain(PC, "MakeLoot", ["drops", "bonus_chance"],
                        numeric=("bonus_chance",))
    for e in pc["entries"]:
        entries.append({
            "file": PC, "line": e["line"], "id": e["id"],
            "guaranteed_drops": e["drops"] if isinstance(e["drops"], list) else [],
            "bonus_chance": e["bonus_chance"],
        })
    return {"sources": [CL, PC], "count": len(entries), "entries": entries}


def extract_world_events():
    """16 world events (PC MakeWorldEvent + post-call setter attributes)."""
    dom = _helper_domain(
        PC, "MakeWorldEvent",
        ["name", "kind", "weight", "cooldown_hours", "min_day",
         "duration_minutes", "zone", "requires_night"],
        numeric=("weight", "cooldown_hours", "duration_minutes"))
    _attach_setters(PC, dom["entries"],
                    r"UAstrawildWorldEventDefinition\s*\*\s*(\w+)\s*=\s*MakeWorldEvent\(",
                    r"Registry->RegisterWorldEvent\(\s*(\w+)\s*\)")
    return dom


def extract_pois():
    """17 POIs (PC MakePOI)."""
    return _helper_domain(
        PC, "MakePOI",
        ["name", "lore", "type", "zone", "offset", "discovery_radius",
         "loot_table", "research_reward", "requires_scanner"],
        numeric=("discovery_radius",))


def extract_resource_nodes():
    """10 resource nodes (PC MakeNode)."""
    return _helper_domain(
        PC, "MakeNode",
        ["name", "item", "rarity", "qty_per_harvest", "max_qty",
         "respawn_seconds", "tint", "hidden"])


def extract_zones():
    """12 zones (Zones.Add(MakeZone(...)) in the zone subsystem)."""
    lines = _load(ZS)
    entries = []
    FIELDS = ["zone", "id", "name", "subtitle", "center_x", "center_y",
              "tint", "light_color", "base", "amplitude", "ridge",
              "threat", "hazard", "hazard_pressure"]
    for line_no, args in _call_args_span(lines, "MakeZone"):
        if len(args) < 12:                    # zone..threat mandatory; hazard pair defaulted
            continue
        e = {"file": ZS, "line": line_no}
        for idx, field in enumerate(FIELDS):
            if idx >= len(args):
                e[field] = None               # defaulted hazard/hazard_pressure
                continue
            v = _parse_value(args[idx])
            if field in ("center_x", "center_y", "base", "amplitude",
                         "ridge", "hazard_pressure") and isinstance(v, (int, float)):
                v = float(v)
            elif field == "threat" and isinstance(v, (int, float)):
                v = int(v)
            e[field] = v
        entries.append(e)
    return {"source": ZS, "count": len(entries), "entries": entries}


def extract_weather():
    """8 weather profiles (static TMap in the weather subsystem)."""
    lines = _load(WS)
    entries = []
    pat = re.compile(
        r"\{\s*EAstrawildWeatherState::(\w+)\s*,\s*MakeProfile\(\s*"
        r"(-?[\d.]+)f?\s*,\s*(-?[\d.]+)f?\s*,\s*([\d.]+)f?\s*\)\s*\}")
    table_line = None
    for i, line in enumerate(lines, start=1):
        m = pat.search(line)
        if m:
            if table_line is None:
                table_line = i
            entries.append({
                "file": WS, "line": i, "state": m.group(1),
                "temp_offset": float(m.group(2)),
                "weight": float(m.group(3)),
                "visibility": float(m.group(4)),
            })
    return {"source": WS, "table_line": table_line, "count": len(entries),
            "entries": entries}


def extract_hunt_contracts():
    """8 hunt contracts (static table in the hunt subsystem)."""
    lines = _load(HS)
    entries = []
    pat = re.compile(
        r'\{\s*TEXT\("(Hunt_\w+)"\)\s*,\s*TEXT\("(Echo_\w+)"\)\s*,\s*(\d+)\s*,\s*'
        r'TEXT\("(Item_\w+)"\)\s*,\s*(\d+)\s*\}')
    for i, line in enumerate(lines, start=1):
        m = pat.search(line)
        if m:
            entries.append({
                "file": HS, "line": i, "id": m.group(1),
                "species": m.group(2), "required_count": int(m.group(3)),
                "reward_item": m.group(4), "reward_qty": int(m.group(5)),
            })
    return {"source": HS, "count": len(entries), "entries": entries}


def extract_abilities():
    """53 abilities (Table.Add(TEXT(id), MakeAbility(...)))."""
    lines = _load(AL)
    entries = []
    FIELDS = ["name", "description", "category", "element", "power",
              "cooldown", "range", "unlock_level", "status_id",
              "status_seconds", "status_speed"]
    for line_no, args in _call_args_span(lines, "MakeAbility"):
        # MakeAbility(id, name, ...) — first arg is the id itself
        if len(args) < 5:
            continue
        e = {"file": AL, "line": line_no, "id": _parse_value(args[0])}
        for idx, field in enumerate(FIELDS):
            if idx + 1 >= len(args):
                e[field] = None   # defaulted param — absent at call site
                continue
            v = _parse_value(args[idx + 1])
            if field in ("power", "cooldown", "range", "status_seconds",
                         "status_speed") and isinstance(v, (int, float)):
                v = float(v)
            elif field == "unlock_level" and isinstance(v, (int, float)):
                v = int(v)
            e[field] = v
        entries.append(e)
    return {"source": AL, "count": len(entries), "entries": entries}


def extract_mutations():
    """204 mutation specs (generated table in AstrawildEchoMutationData.cpp).

    Row layout matches FEchoMutationSpec (AstrawildEchoMutator.h:85):
    species, theme, base mesh, 4 part scales, attachment mask, material
    theme, pattern tint, vfx type, sound set.
    """
    lines = _load(MD)
    entries = []
    FIELDS = ["species", "theme", "base_mesh", "head_scale", "torso_scale",
              "limb_scale", "tail_scale", "attachment_mask",
              "material_theme", "pattern_tint", "vfx_type", "sound_set"]
    # anchor on the array declaration (NOT the function signature)
    start = None
    for i, ln in enumerate(lines):
        if re.search(r"static\s+const\s+TArray<FEchoMutationSpec>\s+Rows\s*=", ln):
            start = i
            break
    if start is None:
        # fall back: first line that opens a top-level table body
        for i, ln in enumerate(lines):
            if re.search(r"=\s*\{\s*$", ln) and "Spec" in ln:
                start = i
                break
    if start is None:
        return {"source": MD, "error": "mutation table not found",
                "count": 0, "entries": []}
    depth = 0
    row_start = None
    for i in range(start, len(lines)):
        for ch in lines[i]:
            if ch == "{":
                depth += 1
                if depth == 2:
                    row_start = i
            elif ch == "}":
                if depth == 2 and row_start is not None:
                    chunk = "\n".join(lines[row_start:i + 1])
                    e = _parse_mutation_row(chunk, MD, row_start + 1, FIELDS)
                    if e:
                        entries.append(e)
                    row_start = None
                depth -= 1
                if depth == 0:
                    return {"source": MD, "table_line": start + 1,
                            "count": len(entries), "entries": entries}
    return {"source": MD, "table_line": start + 1,
            "count": len(entries), "entries": entries}


def _parse_mutation_row(chunk, rel, line, fields):
    toks = []
    for m in TEXT_TOKEN_RE.finditer(chunk):
        toks.append(("text", m.group(1), m.start()))
    for m in ENUM_TOKEN_RE.finditer(chunk):
        toks.append(("enum", m.group(1), m.start()))
    for m in re.finditer(r"\b0x[0-9A-Fa-f]+\b", chunk):
        toks.append(("hex", int(m.group(0), 16), m.start()))
    for m in re.finditer(r"(?<![\w.])(-?\d+\.\d+)f?(?![\w])", chunk):
        toks.append(("scalar", float(m.group(1)), m.start()))
    toks.sort(key=lambda t: t[2])
    vals = [t[1] for t in toks]
    if len(vals) < len(fields):
        return None
    row = {}
    for f, v in zip(fields, vals):
        row[f] = v
    row["file"] = rel
    row["line"] = line
    return row


# ---------------------------------------------------------------------------
# Pass F — bootstrapper domains (dungeons, bosses) + save schema
# ---------------------------------------------------------------------------

DUNGEON_PROPS = ("DungeonId", "RoomCount", "BossDefinitionId",
                 "BossDefeatEventId", "RewardTechnologyId",
                 "DungeonCompletionResearchPoints")


def extract_dungeons():
    """3 dungeons (WorldBootstrapper property-assignment blocks)."""
    lines = _load(WB)
    entries = []
    id_re = re.compile(r'^\s*(\w+)->DungeonId\s*=\s*TEXT\("(Dungeon_\w+)"\);')
    prop_re = re.compile(r"^\s*(\w+)->(\w+)\s*=\s*([^;]+);\s*(?://.*)?$")
    current = None
    for i, line in enumerate(lines, start=1):
        m = id_re.match(line)
        if m:
            current = {"var": m.group(1), "id": m.group(2)}
            entries.append({
                "file": WB, "line": i, "id": m.group(2),
                "properties": [],
            })
            continue
        p = prop_re.match(line)
        if p and current and p.group(1) == current["var"] and p.group(2) in DUNGEON_PROPS:
            entries[-1]["properties"].append({
                "prop": p.group(2), "value": _setter_value(p.group(3)),
                "line": i})
            if p.group(2) == "BossDefinitionId":
                entries[-1]["boss"] = _setter_value(p.group(3))
            elif p.group(2) == "RoomCount":
                entries[-1]["room_count"] = int(_setter_value(p.group(3)))
            continue
        # end of block heuristic: blank line resets only if next id line comes
        if current and line.strip() and not p and "->" not in line and "SpawnActor" in line:
            current = None
    return {"source": WB, "count": len(entries), "entries": entries}


def extract_bosses():
    """4 bosses: 3 dungeon BossDefinitionId + 1 world BossSpeciesId."""
    lines = _load(WB)
    entries = []
    d_re = re.compile(r'^\s*(\w+)->BossDefinitionId\s*=\s*TEXT\("(Echo_\w+)"\);')
    w_re = re.compile(r'^\s*(\w+)->BossSpeciesId\s*=\s*TEXT\("(Echo_\w+)"\);')
    seen = set()
    for i, line in enumerate(lines, start=1):
        m = d_re.match(line)
        if m and m.group(2) not in seen:
            seen.add(m.group(2))
            entries.append({"file": WB, "line": i, "id": m.group(2),
                            "kind": "dungeon"})
            continue
        m = w_re.match(line)
        if m and m.group(2) not in seen:
            seen.add(m.group(2))
            entries.append({"file": WB, "line": i, "id": m.group(2),
                            "kind": "world"})
    # world boss tuning lines (traced)
    for e in entries:
        if e["kind"] == "world":
            tuning = []
            for i, line in enumerate(lines, start=1):
                if re.search(r"BossHealthScale|BossDamageScale", line):
                    tuning.append({"line": i, "raw": line.strip()})
            e["tuning"] = tuning
    return {"source": WB, "count": len(entries), "entries": entries}


def extract_save_schema():
    """Save schema: 29 top-level UPROPERTY fields on UAstrawildSaveGame +
    the 20 save-record struct types (each traced)."""
    src = open(os.path.join(REPO_ROOT, SS), encoding="utf-8-sig",
               errors="replace").read()
    lines = src.split("\n")
    m = re.search(r"class ASTRAWILDCORE_API UAstrawildSaveGame[^{]*\{(.*?)\n\};",
                  src, re.S)
    fields = []
    if m:
        body_start_line = src[:m.start(1)].count("\n") + 1
        body_lines = m.group(1).split("\n")
        prop_re = re.compile(r"UPROPERTY\(([^)]*)\)\s*$")
        field_re = re.compile(r"^\s*([\w<>:, ]+?)\s+(\w+)\s*;")
        pending = None
        for j, ln in enumerate(body_lines):
            pm = prop_re.search(ln.strip())
            if pm and ln.strip().startswith("UPROPERTY"):
                pending = pm.group(1)
                continue
            if pending:
                fm = field_re.match(ln)
                if fm:
                    fields.append({
                        "file": SS, "line": body_start_line + j + 1,
                        "name": fm.group(2), "type": fm.group(1).strip(),
                        "category": pending,
                    })
                    pending = None
                    continue
                if ln.strip() and not ln.strip().startswith("//"):
                    pending = None
    # record struct types (Types.h)
    TH = "Source/AstrawildCore/Public/AstrawildTypes.h"
    th_src = open(os.path.join(REPO_ROOT, TH), encoding="utf-8-sig",
                  errors="replace").read()
    th_lines = th_src.split("\n")
    struct_re = re.compile(r"USTRUCT\([^)]*\)\s*$")
    name_re = re.compile(r"^\s*struct\s+ASTRAWILDCORE_API\s+(FAstrawild\w+)")
    records = []
    pending = False
    for i, ln in enumerate(th_lines, start=1):
        if struct_re.match(ln):
            pending = True
            continue
        if pending:
            nm = name_re.match(ln)
            if nm:
                name = nm.group(1)
                if re.search(r"Save|Journal|Hunt|Coop|Attribute|Instance|"
                             r"RestPoint|Drone|Robot|ItemStack|WorldEvent|"
                             r"Dialogue|Affinity", name):
                    records.append({"file": TH, "line": i, "struct": name})
                pending = False
            elif ln.strip() and not ln.strip().startswith("//"):
                pending = False
    return {
        "source": SS, "record_source": TH,
        "top_level_fields": len(fields), "fields": fields,
        "record_structs": len(records), "records": records,
    }

# ---------------------------------------------------------------------------
# Pass G — headline counts + census reconciliation
# ---------------------------------------------------------------------------

# The authoritative census from Scripts/validate_final_run.py EXPECTED_CENSUS
# (the repo's enforced truth — see Docs/ASTRAWILD_COVERAGE_REPORT.md).
EXPECTED_CENSUS = {
    "items": 78, "recipes": 58, "species": 229, "buildings": 26,
    "techs": 17, "quests": 22, "loot_tables": 11, "npcs": 13,
    "weapons": 8, "resource_nodes": 10, "work_sites": 8,
    "world_events": 16, "pois": 17, "dialogue_trees": 13, "robots": 3,
}


def extract_counts(bestiary, species, items, recipes, buildings, weapons,
                   technologies, quests, npcs, loot, events, pois, nodes,
                   sites, robots, dialogue, zones, weather, hunts, abilities,
                   mutations, dungeons, bosses, save_schema):
    tests_path = "Source/AstrawildCore/Private/AstrawildAutomationTests.cpp"
    with open(os.path.join(REPO_ROOT, tests_path), encoding="utf-8-sig",
              errors="replace") as fh:
        test_count = sum(
            1 for line in fh
            if re.search(r"IMPLEMENT_(SIMPLE|CUSTOM)_AUTOMATION_TEST", line)
        )
    src_files = 0
    src_loc = 0
    for root, _dirs, files in os.walk(os.path.join(REPO_ROOT, "Source")):
        for f in files:
            if f.endswith((".h", ".cpp", ".cs")):
                src_files += 1
                with open(os.path.join(root, f), encoding="utf-8-sig",
                          errors="replace") as fh:
                    src_loc += sum(1 for _ in fh)

    total_species = (bestiary.get("aggregates", {}).get("species_count", 0)
                     + species["count"])
    actual = {
        "items": items["count"], "recipes": recipes["count"],
        "species": total_species, "buildings": buildings["count"],
        "techs": technologies["count"], "quests": quests["count"],
        "loot_tables": loot["count"], "npcs": npcs["count"],
        "weapons": weapons["count"], "resource_nodes": nodes["count"],
        "work_sites": sites["count"], "world_events": events["count"],
        "pois": pois["count"], "dialogue_trees": dialogue["count"],
        "robots": robots["count"],
    }
    census = {}
    for k, expected in EXPECTED_CENSUS.items():
        census[k] = {"expected": expected, "extracted": actual[k],
                     "match": actual[k] == expected}

    return {
        "automation_tests": {"value": test_count},
        "source_files": {"value": src_files},
        "source_loc": {"value": src_loc},
        "census_vs_validate_final_run": census,
        "census_all_match": all(v["match"] for v in census.values()),
        "non_census_domains": {
            "zones": zones["count"], "weather_profiles": weather["count"],
            "hunt_contracts": hunts["count"], "abilities": abilities["count"],
            "mutations": mutations["count"], "dungeons": dungeons["count"],
            "bosses": bosses["count"],
            "save_top_level_fields": save_schema["top_level_fields"],
            "save_record_structs": save_schema["record_structs"],
            "authored_species": species["count"],
            "bestiary_species": bestiary.get("aggregates", {}).get("species_count", 0),
        },
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
    ap = argparse.ArgumentParser(description="ASTRAWILD design data extractor v2")
    ap.add_argument("--out", default=os.path.join(REPO_ROOT, "Design", "design_data.json"))
    args = ap.parse_args()

    print("[extract] pass A: UPROPERTY tunables across Public headers ...")
    tunables = extract_tunables()
    print(f"[extract]   -> {tunables['count']} tunables from {tunables['files_scanned']} headers")

    print("[extract] pass B: bestiary species table ...")
    bestiary = extract_bestiary()
    print(f"[extract]   -> {bestiary.get('aggregates', {}).get('species_count', 0)} bestiary species")

    print("[extract] pass C: species (authored + evolution) ...")
    species = extract_species()
    print(f"[extract]   -> {species['count']} non-bestiary species "
          f"({sum(1 for e in species['entries'] if e['origin'] == 'authored')} authored, "
          f"{sum(1 for e in species['entries'] if e['origin'] == 'production')} production, "
          f"{sum(1 for e in species['entries'] if e['origin'] == 'evolution_target')} evolution)")

    print("[extract] pass C: items ...")
    items = extract_items()
    print(f"[extract]   -> {items['count']} items")

    print("[extract] pass C: recipes ...")
    recipes = extract_recipes()
    print(f"[extract]   -> {recipes['count']} recipes")

    print("[extract] pass D: variable-style domains ...")
    quests = extract_quests()
    print(f"[extract]   -> {quests['count']} quests")
    npcs = extract_npcs()
    print(f"[extract]   -> {npcs['count']} NPCs")
    sites = extract_work_sites()
    print(f"[extract]   -> {sites['count']} work sites")
    robots = extract_robots()
    print(f"[extract]   -> {robots['count']} robots")
    dialogue = extract_dialogue_trees()
    print(f"[extract]   -> {dialogue['count']} dialogue trees")

    print("[extract] pass E: helper + table domains ...")
    buildings = extract_buildings()
    print(f"[extract]   -> {buildings['count']} buildings")
    weapons = extract_weapons()
    print(f"[extract]   -> {weapons['count']} weapons")
    technologies = extract_technologies()
    print(f"[extract]   -> {technologies['count']} technologies")
    loot = extract_loot_tables()
    print(f"[extract]   -> {loot['count']} loot tables")
    events = extract_world_events()
    print(f"[extract]   -> {events['count']} world events")
    pois = extract_pois()
    print(f"[extract]   -> {pois['count']} POIs")
    nodes = extract_resource_nodes()
    print(f"[extract]   -> {nodes['count']} resource nodes")
    zones = extract_zones()
    print(f"[extract]   -> {zones['count']} zones")
    weather = extract_weather()
    print(f"[extract]   -> {weather['count']} weather profiles")
    hunts = extract_hunt_contracts()
    print(f"[extract]   -> {hunts['count']} hunt contracts")
    abilities = extract_abilities()
    print(f"[extract]   -> {abilities['count']} abilities")
    mutations = extract_mutations()
    print(f"[extract]   -> {mutations['count']} mutation specs")

    print("[extract] pass F: bootstrapper + save schema ...")
    dungeons = extract_dungeons()
    print(f"[extract]   -> {dungeons['count']} dungeons")
    bosses = extract_bosses()
    print(f"[extract]   -> {bosses['count']} bosses")
    save_schema = extract_save_schema()
    print(f"[extract]   -> save schema: {save_schema['top_level_fields']} fields "
          f"+ {save_schema['record_structs']} record structs")

    print("[extract] pass G: counts + census reconciliation ...")
    counts = extract_counts(bestiary, species, items, recipes, buildings,
                            weapons, technologies, quests, npcs, loot, events,
                            pois, nodes, sites, robots, dialogue, zones,
                            weather, hunts, abilities, mutations, dungeons,
                            bosses, save_schema)
    if counts["census_all_match"]:
        print("[extract]   -> CENSUS: 15/15 metrics MATCH validate_final_run.py")
    else:
        bad = [f"{k}: {v['extracted']} != {v['expected']}"
               for k, v in counts["census_vs_validate_final_run"].items()
               if not v["match"]]
        print(f"[extract]   -> CENSUS MISMATCH: {'; '.join(bad)}")

    doc = {
        "schema": "astrawild-design-data/2",
        "generated": datetime.datetime.now().isoformat(timespec="seconds"),
        "generator": "Scripts/extract_design_data.py",
        "repo_head": repo_head(),
        "rule": "Every value below was parsed from Source/ by the generator. "
                "No value is hand-typed (Master Directive v1 rule R1).",
        "domains": {
            "tunables": tunables,
            "bestiary": bestiary,
            "species": species,
            "items": items,
            "recipes": recipes,
            "buildings": buildings,
            "weapons": weapons,
            "technologies": technologies,
            "quests": quests,
            "npcs": npcs,
            "loot_tables": loot,
            "world_events": events,
            "pois": pois,
            "resource_nodes": nodes,
            "work_sites": sites,
            "robots": robots,
            "dialogue_trees": dialogue,
            "zones": zones,
            "weather": weather,
            "hunt_contracts": hunts,
            "abilities": abilities,
            "mutations": mutations,
            "dungeons": dungeons,
            "bosses": bosses,
            "save_schema": save_schema,
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
    return 0


if __name__ == "__main__":
    sys.exit(main())
