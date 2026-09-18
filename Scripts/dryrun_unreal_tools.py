#!/usr/bin/env python3
"""
dryrun_unreal_tools.py — LONG-RUN DIRECTIVE L3 harness
=======================================================
Executes every script in Tools/Python/ end-to-end against the recording
mock engine (Tools/Python/_mock_unreal/) and PROVES:

  P1  no exceptions — every tool's logic runs to completion
  P2  every asset a tool claims to create ("Created X" log line) was
      actually requested through AssetTools.create_asset
  P3  idempotence — running a tool twice from identical fresh state
      produces IDENTICAL call logs (determinism), and a third run on top
      of the persisted state CONVERGES to the same final editor state
      (assets + actor labels + actor properties)
  P4  every unreal.* symbol the tools touch exists in the mock (missing
      lookups are recorded by the mock's module __getattr__ and must be
      empty — tools' try/except would otherwise hide them)

Then prints the mock's symbol table with VERIFIED / UNVERIFIED evidence
for the ON_PC conformance pass.

Run:  python Scripts/dryrun_unreal_tools.py
Exit: 0 == all proofs passed.
"""

from __future__ import annotations

import importlib
import os
import re
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOLS = os.path.join(REPO, "Tools", "Python")
sys.path.insert(0, TOOLS)

import _mock_unreal  # noqa: E402

TOOL_MODULES = [
    # (module name, calls main() at import scope?)
    ("verify_environment", True),
    ("build_prototype_map", False),
    ("setup_input_assets", False),
    ("generate_datatables", False),
    ("build_showcase_map", False),
    ("wire_gamemode", False),
    ("first_day_orchestrator", True),
]

FAILURES = []
PROOFS = 0


def check(ok, label, detail=""):
    global PROOFS
    PROOFS += 1
    if ok:
        return True
    FAILURES.append("{}{}".format(label, (" :: " + detail) if detail else ""))
    return False


def fresh_import(name, self_executing):
    """Import a tool module fresh (no cache), running its entry point."""
    for mod in list(sys.modules):
        if mod in ("unreal", "_mock_unreal"):
            continue
        if mod in [t[0] for t in TOOL_MODULES] or mod.startswith("_mock"):
            del sys.modules[mod]
    mod = importlib.import_module(name)
    if not self_executing and hasattr(mod, "main"):
        mod.main()
    return mod


def calls_of(log):
    """Comparable form of the call log."""
    return [(e["symbol"], e["args"], tuple(sorted(e["kwargs"].items())))
            for e in log]


def run_tool(name, self_executing):
    """One full proof cycle for one tool. Returns per-tool detail dict."""
    unreal_mock = _mock_unreal.install()
    sys.modules["unreal"] = unreal_mock

    # ---- run 1: fresh state -------------------------------------------------
    unreal_mock._reset_all()
    exc1 = None
    try:
        fresh_import(name, self_executing)
    except SystemExit as se:
        if se.code not in (0, None):
            exc1 = "SystemExit({})".format(se.code)
    except Exception as exc:  # noqa: BLE001
        exc1 = "{}: {}".format(type(exc).__name__, exc)
    log1 = calls_of(unreal_mock._CALL_LOG())
    miss1 = unreal_mock._MISSING_LOOKUPS()
    claimed1 = created_from_log(log1)
    created1 = created_from_calls(log1)
    snap1 = unreal_mock._STATE.snapshot()

    # ---- run 2: fresh state again (determinism) -----------------------------
    unreal_mock._reset_all()
    exc2 = None
    try:
        fresh_import(name, self_executing)
    except SystemExit as se:
        if se.code not in (0, None):
            exc2 = "SystemExit({})".format(se.code)
    except Exception as exc:  # noqa: BLE001
        exc2 = "{}: {}".format(type(exc).__name__, exc)
    log2 = calls_of(unreal_mock._CALL_LOG())
    miss2 = unreal_mock._MISSING_LOOKUPS()
    snap2 = unreal_mock._STATE.snapshot()

    # ---- run 3: stateful re-run (convergence) -------------------------------
    exc3 = None
    try:
        fresh_import(name, self_executing)
    except SystemExit as se:
        if se.code not in (0, None):
            exc3 = "SystemExit({})".format(se.code)
    except Exception as exc:  # noqa: BLE001
        exc3 = "{}: {}".format(type(exc).__name__, exc)
    snap3 = unreal_mock._STATE.snapshot()

    # ---- proofs --------------------------------------------------------------
    check(exc1 is None, "P1 no exception [{} run1]".format(name), str(exc1))
    check(exc2 is None, "P1 no exception [{} run2]".format(name), str(exc2))
    check(exc3 is None, "P1 no exception [{} run3]".format(name), str(exc3))

    check(claimed1 == created1, "P2 claimed==created [{}]".format(name),
          "claimed {} vs created {}".format(sorted(claimed1),
                                            sorted(created1)))

    check(log1 == log2, "P3 identical call logs [{}]".format(name),
          "{} vs {} entries".format(len(log1), len(log2)))

    check(snap3 == snap2, "P3 stateful convergence [{}]".format(name),
          "final state drifted after re-run on persisted state")

    check(not miss1 and not miss2, "P4 no missing symbols [{}]".format(name),
          "missing: {}".format(sorted(set(miss1 + miss2))[:8]))

    return {
        "name": name,
        "calls": len(log1),
        "created": sorted(created1),
        "actors": len(snap1.get("actors", [])),
        "assets": len(snap1.get("assets", [])),
    }


def created_from_log(log):
    """Asset names the tool CLAIMS to create (its own 'Created X' logs)."""
    out = set()
    for symbol, args, _kw in log:
        if symbol in ("log", "log_warning") and args and isinstance(args[0], str):
            m = re.match(r"^\[?\w*\]?\s*Created (\w+)$", args[0])
            if m:
                out.add(m.group(1))
    return out


def created_from_calls(log):
    """Asset names actually requested through AssetTools.create_asset."""
    out = set()
    for symbol, args, _kw in log:
        if symbol == "AssetTools.create_asset" and args:
            out.add(args[0])
    return out


def symbol_table():
    """The mock's VERIFIED/UNVERIFIED annotations."""
    rows = []
    for name in _mock_unreal.__all__:
        obj = getattr(_mock_unreal, name)
        status = getattr(obj, "_UE_STATUS",
                         ("UNVERIFIED", "no annotation"))
        rows.append((name, status[0], status[1]))
    return rows


def main():
    print("=" * 72)
    print("ASTRAWILD dry-run harness — mock Unreal, real tool logic (L3)")
    print("=" * 72)

    results = []
    for name, self_exec in TOOL_MODULES:
        print("\n--- {} ---".format(name))
        r = run_tool(name, self_exec)
        results.append(r)
        print("    calls recorded: {:>5}".format(r["calls"]))
        print("    assets created: {:>5} {}".format(
            len(r["created"]),
            "(" + ", ".join(r["created"][:6]) + ("..." if len(r["created"]) > 6 else "") + ")"
            if r["created"] else ""))
        print("    final actors:   {:>5}   final assets: {}".format(
            r["actors"], r["assets"]))

    print("\n" + "=" * 72)
    print("MOCK SYMBOL TABLE (UE 5.8 conformance checklist)")
    print("=" * 72)
    rows = symbol_table()
    verified = [r for r in rows if r[1] == "VERIFIED"]
    unverified = [r for r in rows if r[1] != "VERIFIED"]
    for name, status, _evidence in rows:
        print("  {:<32} {}".format(name, status))
    print("\n  VERIFIED: {}   UNVERIFIED: {}".format(
        len(verified), len(unverified)))
    for name, _s, evidence in unverified:
        print("  UNVERIFIED {} :: {}".format(name, evidence))

    print("\n" + "=" * 72)
    print("{}/{} proofs passed".format(PROOFS - len(FAILURES), PROOFS))
    if FAILURES:
        print("FAILURES:")
        for f in FAILURES:
            print("  - " + f)
        print("*** DRY-RUN FAILED ***")
        return 1
    print("ALL DRY-RUN PROOFS PASSED — tool logic executes; engine "
          "conformance still needs the real UE 5.8 run (ON_PC hour 0)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
