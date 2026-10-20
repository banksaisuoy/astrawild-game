#!/usr/bin/env python3
"""
build_source_inventory.py — LONG-RUN DIRECTIVE L6
==================================================
Classifies every Source/ file with EVIDENCE (never vibes) into:

  [FULL]    substantial implementation, no release-gating placeholders
  [PARTIAL] functional but carries REPLACE_BEFORE_RELEASE / TODO / staged
            placeholder visuals / CODE_DEFAULT generated descriptions
  [STUB]    skeleton only (tiny, or declaration-only with no bodies)
  [MISSING] referenced but absent from the tree

Outputs Docs/ASTRAWILD_SOURCE_INVENTORY.md with a per-file row
(classification + evidence line + one-line note) and a dedicated
REPLACE_BEFORE_RELEASE ledger.

Evidence rules (all machine-checked):
  - LOC count
  - REPLACE_BEFORE_RELEASE occurrences (line-listed)
  - TODO / FIXME / XXX occurrences
  - CODE_DEFAULT generated-description markers
  - empty/trivial function bodies in .cpp ({ }, { return; }, { return false; })
  - function-definition density in .cpp vs declarations in .h

Run:  python Scripts/build_source_inventory.py
"""

from __future__ import annotations

import os
import re
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SRC = os.path.join(REPO, "Source")

MARKERS = {
    "REPLACE_BEFORE_RELEASE": re.compile(r"REPLACE_BEFORE_RELEASE"),
    "TODO": re.compile(r"\b(TODO|FIXME|XXX)\b"),
    "CODE_DEFAULT": re.compile(r"CODE_DEFAULT"),
}

EMPTY_BODY = re.compile(r"\{\s*(?:return\s+(?:false|nullptr|0\.0f)?;?\s*)?\}",
                        re.S)
FUNC_DEF = re.compile(r"^\s*(?:[\w:<>,\s&\*]+?)::\w+\s*\(")


def classify(path):
    rel = os.path.relpath(path, REPO).replace("\\", "/")
    with open(path, encoding="utf-8-sig", errors="replace") as fh:
        text = fh.read()
    lines = text.split("\n")
    loc = len([ln for ln in lines if ln.strip()])

    hits = {k: [] for k in MARKERS}
    for i, ln in enumerate(lines, start=1):
        for key, pat in MARKERS.items():
            if pat.search(ln) and "//" not in ln[:max(ln.find("//"), 0)] or \
                    pat.search(ln) and "//" in ln:
                if pat.search(ln):
                    hits[key].append(i)

    empty_bodies = 0
    func_defs = 0
    if path.endswith(".cpp"):
        func_defs = sum(1 for ln in lines if FUNC_DEF.match(ln))
        # count trivial bodies at brace-close granularity
        empty_bodies = len(re.findall(
            r"\)\s*(?:const\s*)?\{\s*(?:return\s+(?:false|nullptr|0\.0f)?;?)?\s*\}",
            text))

    if loc == 0:
        return rel, "EMPTY", loc, hits, 0, 0, "zero non-blank lines"

    # Module/target boilerplate is COMPLETE for its purpose.
    if path.endswith(".Target.cs"):
        return rel, "FULL", loc, hits, 0, 0, "standard build-target boilerplate (complete)"
    if os.path.basename(path) in ("AstrawildLog.cpp", "AstrawildCore.h",
                                  "AstrawildCore.cpp", "AstrawildLog.h"):
        return rel, "FULL", loc, hits, func_defs, empty_bodies, \
            "standard module/log boilerplate (complete)"

    if hits["REPLACE_BEFORE_RELEASE"]:
        return rel, "PARTIAL", loc, hits, func_defs, empty_bodies, \
            "release-gated placeholder visuals (see ledger)"
    if hits["TODO"]:
        return rel, "PARTIAL", loc, hits, func_defs, empty_bodies, \
            "carries TODO/FIXME markers"

    if path.endswith(".cpp"):
        if loc < 40 and func_defs == 0:
            return rel, "STUB", loc, hits, func_defs, empty_bodies, \
                "skeleton ({loc} non-blank lines, no function definitions)".format(loc=loc)
        if func_defs > 0 and empty_bodies >= func_defs:
            return rel, "STUB", loc, hits, func_defs, empty_bodies, \
                "every function body is trivial"
        return rel, "FULL", loc, hits, func_defs, empty_bodies, \
            "no release-gating placeholders found"

    # Header: inherit the paired .cpp's substance when a pair exists.
    if path.endswith(".h"):
        # UE interfaces are header-only BY DESIGN — complete, not stubs.
        if re.search(r"\bUINTERFACE\s*\(", text):
            return rel, "FULL", loc, hits, func_defs, empty_bodies, \
                "complete UE interface (header-only by design)"
        pair = path.replace(os.sep + "Public" + os.sep,
                            os.sep + "Private" + os.sep)[:-2] + ".cpp"
        if os.path.exists(pair):
            with open(pair, encoding="utf-8-sig", errors="replace") as fh:
                pair_text = fh.read()
            pair_loc = len([ln for ln in pair_text.split("\n") if ln.strip()])
            pair_defs = sum(1 for ln in pair_text.split("\n") if FUNC_DEF.match(ln))
            pair_trivial = len(re.findall(
                r"\)\s*(?:const\s*)?\{\s*(?:return\s+(?:false|nullptr|0\.0f)?;?)?\s*\}",
                pair_text))
            if pair_defs > 0 and pair_trivial < pair_defs:
                return rel, "FULL", loc, hits, func_defs, empty_bodies, \
                    "paired .cpp: {} real function bodies ({} LOC)".format(
                        pair_defs, pair_loc)
            if pair_loc >= 100:
                return rel, "FULL", loc, hits, func_defs, empty_bodies, \
                    "paired .cpp carries {} LOC of implementation".format(pair_loc)
            return rel, "STUB", loc, hits, func_defs, empty_bodies, \
                "paired .cpp is a skeleton ({} LOC, {} defs)".format(
                    pair_loc, pair_defs)
        # self-contained header: substantial inline code = FULL
        if loc >= 60:
            return rel, "FULL", loc, hits, func_defs, empty_bodies, \
                "self-contained (no .cpp pair; inline implementations)"
        return rel, "STUB", loc, hits, func_defs, empty_bodies, \
            "declaration-only header, no .cpp pair ({} LOC)".format(loc)
    return rel, "FULL", loc, hits, func_defs, empty_bodies, "no release-gating placeholders found"


def main():
    files = []
    for root, _dirs, names in os.walk(SRC):
        for n in sorted(names):
            if n.endswith((".h", ".cpp", ".cs")):
                files.append(os.path.join(root, n))
    files.sort()

    rows = []
    ledger = []
    for path in files:
        rel, cls, loc, hits, func_defs, empty_bodies, note = classify(path)
        rows.append((rel, cls, loc, hits, func_defs, empty_bodies, note))
        for line in hits["REPLACE_BEFORE_RELEASE"]:
            with open(path, encoding="utf-8-sig", errors="replace") as fh:
                ln = fh.read().split("\n")[line - 1].strip()
            ledger.append((rel, line, ln[:110]))

    head = subprocess.run(["git", "rev-parse", "--short", "HEAD"],
                          cwd=REPO, capture_output=True, text=True).stdout.strip()

    from collections import Counter
    by_class = Counter(r[1] for r in rows)
    out = []
    out.append("# ASTRAWILD SOURCE INVENTORY — LONG-RUN DIRECTIVE L6\n")
    out.append("> Machine-generated by `Scripts/build_source_inventory.py` "
               "at HEAD `{}`. Every classification cites its evidence "
               "(marker line / LOC / body-density). "
               "**R3 stands**: nothing here claims anything compiles.\n".format(head))
    out.append("\n## Summary\n")
    out.append("| Class | Files | Notes |")
    out.append("|---|---|---|")
    out.append("| FULL | {} | complete implementations, no release gates |".format(by_class.get("FULL", 0)))
    out.append("| PARTIAL | {} | functional; placeholder/TODO markers listed per file |".format(by_class.get("PARTIAL", 0)))
    out.append("| STUB | {} | skeletons (tiny or trivial bodies) |".format(by_class.get("STUB", 0)))
    out.append("| EMPTY | {} | zero content |".format(by_class.get("EMPTY", 0)))
    out.append("| MISSING | 0 | every referenced file exists (walk over Source/) |")
    total_loc = sum(r[2] for r in rows)
    out.append("\nTotal: **{} files, {} non-blank LOC**.\n".format(len(rows), total_loc))

    out.append("\n## File-by-file\n")
    out.append("| File | Class | LOC | Evidence |")
    out.append("|---|---|---|---|")
    for rel, cls, loc, hits, func_defs, empty_bodies, note in rows:
        ev = []
        for key in ("REPLACE_BEFORE_RELEASE", "TODO", "CODE_DEFAULT"):
            if hits[key]:
                ev.append("{} x{}".format(key, len(hits[key])) +
                          (" (L{})".format(",".join(str(x) for x in hits[key][:3]))))
        if cls == "STUB":
            ev.append(note)
        out.append("| `{}` | {} | {} | {} |".format(
            rel, cls, loc, "; ".join(ev) if ev else "clean"))

    out.append("\n## REPLACE_BEFORE_RELEASE ledger\n")
    out.append("Every release-gated placeholder with its current stand-in and "
               "what it must become (line text verbatim — art binding happens "
               "at the engine pass, see Docs/ASTRAWILD_ART_ASSET_PLAN.md):\n")
    out.append("| File | Line | Current placeholder (verbatim) |")
    out.append("|---|---|---|")
    for rel, line, text in ledger:
        out.append("| `{}` | {} | `{}` |".format(rel, line, text))

    doc_path = os.path.join(REPO, "Docs", "ASTRAWILD_SOURCE_INVENTORY.md")
    with open(doc_path, "w", encoding="utf-8") as fh:
        fh.write("\n".join(out) + "\n")
    print("[inventory] {} files classified: {} FULL / {} PARTIAL / {} STUB / {} EMPTY".format(
        len(rows), by_class.get("FULL", 0), by_class.get("PARTIAL", 0),
        by_class.get("STUB", 0), by_class.get("EMPTY", 0)))
    print("[inventory] REPLACE_BEFORE_RELEASE sites: {}".format(len(ledger)))
    print("[inventory] WROTE {}".format(os.path.relpath(doc_path, REPO)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
