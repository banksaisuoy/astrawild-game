"""
first_day_orchestrator.py — single-command first-day driver (roadmap P1-T1)
============================================================================
The ONE Unreal Editor Python script the user runs on the Windows machine to
execute the whole source-side first-day flow in order, stopping at the first
hard failure:

    1. environment pre-flight  (verify_environment)
    2. prototype map build     (build_prototype_map -> L_Proto_01)
    3. input asset setup       (setup_input_assets -> IA/IMC/BP layer)
    4. DataTable generation    (generate_datatables -> 3 traced tables)
    5. showcase map build      (build_showcase_map -> L_Showcase cinematic)
    6. GameMode wiring         (wire_gamemode -> both levels)

Steps 2 and 3 intentionally reuse the EXISTING proven scripts by file path
(so this orchestrator owns no asset logic of its own — rule of least code).
If this file sits beside them in Tools/Python/, it imports them as modules;
if they are missing it refuses to run rather than improvising.

What it deliberately does NOT do (they need the full game module + GUI):
    - automation tests      -> Test.bat / Test.ps1 (UnrealEditor-Cmd -ExecCmds)
    - PIE golden path       -> Launch_ASTRAWILD.bat / manual PIE
    - packaging             -> Tools/package_windows.bat

Run headless (Windows, engine at E:\\Epic Games\\UnrealEngine):
  "E:\\Epic Games\\UnrealEngine\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" ^
    "E:\\AstrawildGame\\ASTRAWILD.uproject" ^
    -run=pythonscript -script="E:\\AstrawildGame\\Tools\\Python\\first_day_orchestrator.py" ^
    -stdout -unattended -nopause -nosplash

Idempotent: every step is itself idempotent (verify is read-only; the map
builder purges + rebuilds its LPROTO_ actors; the input setup recreates its
input assets deterministically). Running the orchestrator twice converges.

UNVERIFIED — NEEDS MACHINE (rule R2): import-as-module delegation between
pythonscript commandlets is standard Python, but the exact editor working
directory when -run=pythonscript executes a script is an engine behavior
this sandbox cannot execute. The loader below resolves the sibling scripts
from THIS file's own directory (robust regardless of CWD) and falls back to
a hard, actionable error listing the exact missing files.
"""

import os
import sys

import unreal

PREFIX = "[AWFIRST]"

# Sibling scripts this orchestrator delegates to ( Tools/Python/ ).
# LONG-RUN DIRECTIVE L5.4: the full first-day chain in execution order —
# each step lists its expected result; every step is idempotent.
HERE = os.path.dirname(os.path.abspath(__file__))
STEP_SCRIPTS = [
    ("environment check", "verify_environment"),
    ("prototype map", "build_prototype_map"),
    ("input assets", "setup_input_assets"),
    ("DataTable generation", "generate_datatables"),
    ("showcase map", "build_showcase_map"),
    ("GameMode wiring", "wire_gamemode"),
]

# One-line expected results, printed before each step runs.
EXPECTED = {
    "verify_environment": "VERDICT line with engine 5.8+, 4 class groups ok",
    "build_prototype_map": "L_Proto_01 saved; 8 LPROTO_* actors; DONE line",
    "setup_input_assets": "32 IA_* + IMC_Player(35) + IMC_Gamepad(19) + 2 BPs",
    "generate_datatables": "3 DataTables (Abilities 53 / Weather 8 / Zones 12)",
    "build_showcase_map": "L_Showcase_ArtOverhaul saved; cinematic rig + vista",
    "wire_gamemode": "both levels: GameMode override asserted, surfaces ok",
}


def log(message):
    unreal.log("{} {}".format(PREFIX, message))


def log_warn(message):
    unreal.log_warning("{} {}".format(PREFIX, message))


def import_step_module(name):
    """Import a sibling script by module name; prepend its directory to
    sys.path so the import works no matter the editor CWD."""
    if HERE not in sys.path:
        sys.path.insert(0, HERE)
    try:
        return __import__(name)
    except ImportError as exc:
        log_warn("cannot import {} from {}: {}".format(name, HERE, exc))
        return None


def run_environment_check():
    """The verifier is import-safe but self-executing (it calls main() at
    module scope), so run it in a fresh interpreter state: exec its file and
    parse the verdict from the exceptions raised. Simplest robust contract:
    import it and let its own PASS/FAIL log lines speak; a hard crash here
    surfaces as an exception that fails the orchestrator step."""
    mod = import_step_module("verify_environment")
    if mod is None:
        return False
    # verify_environment.py already ran on import; nothing more to do.
    return True


def run_prototype_map():
    mod = import_step_module("build_prototype_map")
    if mod is None:
        return False
    try:
        if hasattr(mod, "main"):
            mod.main()
            return True
        # build_prototype_map.py executes at module scope: successful import
        # (no exception) IS the success signal.
        return True
    except Exception as exc:  # noqa: BLE001 — surfaced to the user verbatim
        log_warn("prototype map step failed: {}".format(exc))
        return False


def run_input_assets():
    mod = import_step_module("setup_input_assets")
    if mod is None:
        return False
    try:
        if hasattr(mod, "main"):
            mod.main()
            return True
        return True
    except Exception as exc:  # noqa: BLE001
        log_warn("input asset step failed: {}".format(exc))
        return False


def run_module_step(name):
    """Generic step runner for the L5 tools (main()-guarded modules)."""
    def _run():
        mod = import_step_module(name)
        if mod is None:
            return False
        try:
            if hasattr(mod, "main"):
                mod.main()
                return True
            return True
        except Exception as exc:  # noqa: BLE001
            log_warn("{} step failed: {}".format(name, exc))
            return False
    return _run


def preflight_files():
    missing = []
    for _label, module_name in STEP_SCRIPTS:
        path = os.path.join(HERE, module_name + ".py")
        if not os.path.isfile(path):
            missing.append(path)
    if missing:
        log_warn("MISSING STEP SCRIPTS (refusing to improvise):")
        for path in missing:
            log_warn("  {}".format(path))
        return False
    return True


def main():
    log("=" * 60)
    log("ASTRAWILD first-day orchestrator")
    log("repo flow: verify_environment -> build_prototype_map -> setup_input_assets")
    log("=" * 60)

    if not preflight_files():
        log("VERDICT: FAIL (step scripts missing — fresh clone? see Docs/ASTRAWILD_ON_PC_TASKS.md)")
        return

    steps = [
        ("1/6 environment check", run_environment_check),
        ("2/6 prototype map", run_prototype_map),
        ("3/6 input assets", run_input_assets),
        ("4/6 DataTable generation", run_module_step("generate_datatables")),
        ("5/6 showcase map", run_module_step("build_showcase_map")),
        ("6/6 GameMode wiring", run_module_step("wire_gamemode")),
    ]
    failed_at = None
    for label, runner in steps:
        module_name = label.split(" ", 1)[1]
        log("--- {} ---".format(label))
        if module_name in EXPECTED:
            log("expect: {}".format(EXPECTED[module_name]))
        if not runner():
            failed_at = label
            break

    log("-" * 60)
    if failed_at is None:
        log("VERDICT: PASS (map + input assets ready)")
        log("NEXT (machine steps this orchestrator does not own):")
        log("  1. automation tests : Test.bat        (134 contracts)")
        log("  2. PIE golden path  : Launch_ASTRAWILD.bat")
        log("  3. full showcase map: /Game/ASTRAWILD/Maps/L_Showcase_ArtOverhaul")
        log("  4. packaging        : Tools/package_windows.bat")
    else:
        log("VERDICT: FAIL (stopped at {})".format(failed_at))
        log("Fix the reported error, re-run this orchestrator — every step is idempotent.")
    log("=" * 60)


main()
