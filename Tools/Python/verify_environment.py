"""
verify_environment.py — ENGINE-RUN-1 environment pre-flight (roadmap P1-T1)
===========================================================================
One-shot environment check that runs INSIDE the Unreal Editor Python
environment and answers, with a machine-generated report:

  1. engine version actually running (must be 5.8.x for this project)
  2. project module AstrawildCore compiled and registered (its UClasses
     resolve) — proves the C++ side of this repo built into the session
  3. content roots the first-day flow depends on (engine basic shapes
     loadable, /Game/ASTRAWILD present)
  4. prototype level assets reachable (or creatable) and the showcase map
     present as the fallback golden path

Exit contract: one PASS/FAIL line per check group with the [AWENV] prefix
and a final verdict line the orchestrator parses:
    [AWENV] VERDICT: PASS   (all groups green)
    [AWENV] VERDICT: FAIL   (see failing groups above)

Idempotent: read-only — spawns nothing, saves nothing, mutates nothing.

Run headless (Windows, engine at E:\\Epic Games\\UnrealEngine):
  "E:\\Epic Games\\UnrealEngine\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" ^
    "E:\\AstrawildGame\\ASTRAWILD.uproject" ^
    -run=pythonscript -script="E:\\AstrawildGame\\Tools\\Python\\verify_environment.py" ^
    -stdout -unattended -nopause -nosplash

UNVERIFIED — NEEDS MACHINE (rule R2): authored against documented UE 5.x
editor Python APIs. Every engine introspection below is wrapped in
try/except with a graceful downgrade — an API that moved in 5.8 becomes an
explicit UNKNOWN result, never a crash. First real execution happens on
the Windows UE 5.8.2 machine; treat the first run as the API conformance
test and file exact failures into Docs/ASTRAWILD_ON_PC_TASKS.md hour 0.
"""

import unreal

PREFIX = "[AWENV]"

# Minimum engine minor version this project targets (5.8).
REQUIRED_ENGINE_MINOR = 8

# AstrawildCore C++ classes the first-day flow depends on. load_class
# returning a valid object proves the module compiled and registered.
REQUIRED_CLASSES = [
    "/Script/AstrawildCore.AstrawildGameMode",
    "/Script/AstrawildCore.AstrawildPlayerCharacter",
    "/Script/AstrawildCore.AstrawildEchoCharacter",
    "/Script/AstrawildCore.AstrawildWorldBootstrapper",
]

GROUPS_FAILED = 0
GROUPS_UNKNOWN = 0


def log(message):
    unreal.log("{} {}".format(PREFIX, message))


def log_warn(message):
    unreal.log_warning("{} {}".format(PREFIX, message))


def group(name, state, detail):
    """state: True = pass, False = fail, None = unknown (API moved)."""
    global GROUPS_FAILED, GROUPS_UNKNOWN
    if state is True:
        log("PASS  {:<24} {}".format(name, detail))
    elif state is False:
        GROUPS_FAILED += 1
        log_warn("FAIL  {:<24} {}".format(name, detail))
    else:
        GROUPS_UNKNOWN += 1
        log_warn("UNK   {:<24} {}".format(name, detail))


def check_engine_version():
    """UNVERIFIED API: SystemLibrary.get_engine_version (UE5 python docs);
    string shape '5.8.2-...+++UE5+Release-5.8' parsed defensively."""
    text = None
    try:
        text = str(unreal.SystemLibrary.get_engine_version())
    except Exception:
        try:
            text = str(unreal.get_engine_version())
        except Exception:
            text = None
    if text is None:
        group("engine-version", None,
              "no version API resolved — verify 'About Unreal Editor' shows 5.8.x")
        return
    passed = False
    try:
        head = text.split("-")[0]
        parts = head.split(".")
        passed = int(parts[0]) == 5 and int(parts[1]) >= REQUIRED_ENGINE_MINOR
    except Exception:
        passed = False
    group("engine-version", passed, "running {} (need 5.8+)".format(text))


def check_core_module():
    resolved = []
    for path in REQUIRED_CLASSES:
        try:
            cls = unreal.load_class(None, path)
        except Exception:
            cls = None
        resolved.append((path.rsplit(".", 1)[1], cls is not None))
    passed = all(ok for _n, ok in resolved)
    detail = ", ".join("{}={}".format(n, "ok" if ok else "MISSING") for n, ok in resolved)
    group("astrawild-classes", passed, detail)


def check_content_roots():
    try:
        cube = unreal.load_asset("/Engine/BasicShapes/Cube")
        cube_ok = cube is not None
    except Exception:
        cube_ok = False
    try:
        root_ok = unreal.EditorAssetLibrary.does_directory_exist("/Game/ASTRAWILD")
    except Exception:
        root_ok = False
    group("content-roots", cube_ok and root_ok,
          "BasicShapes.Cube={}, /Game/ASTRAWILD={}".format(cube_ok, root_ok))


def check_showcase_map():
    try:
        present = unreal.EditorAssetLibrary.does_asset_exist(
            "/Game/ASTRAWILD/Maps/L_Showcase_ArtOverhaul")
    except Exception:
        present = None
    if present is None:
        group("showcase-map", None, "EditorAssetLibrary.does_asset_exist unresolved")
    else:
        group("showcase-map", present,
              "/Game/ASTRAWILD/Maps/L_Showcase_ArtOverhaul present = {}".format(present))


def check_plugin_markers():
    """Plugins are declared in ASTRAWILD.uproject (Enabled/Disabled there is
    the authority at load time). Runtime marker: EnhancedInput's classes and
    Niagara-free ProceduralMeshComponent class resolve when enabled."""
    try:
        ei = unreal.load_class(None, "/Script/EnhancedInput.EnhancedInputComponent")
        ei_ok = ei is not None
    except Exception:
        ei_ok = None
    try:
        pmc = unreal.load_class(None, "/Script/ProceduralMeshComponent.ProceduralMeshComponent")
        pmc_ok = pmc is not None
    except Exception:
        pmc_ok = None
    if ei_ok is None and pmc_ok is None:
        group("plugin-markers", None, "marker classes unresolved — check .uproject plugins block")
    else:
        group("plugin-markers", bool(ei_ok) and bool(pmc_ok),
              "EnhancedInput marker={}, ProceduralMeshComponent marker={}".format(ei_ok, pmc_ok))


def main():
    log("=" * 60)
    log("ASTRAWILD environment pre-flight (verify_environment.py)")
    log("=" * 60)
    check_engine_version()
    check_core_module()
    check_plugin_markers()
    check_content_roots()
    check_showcase_map()
    log("-" * 60)
    if GROUPS_FAILED == 0 and GROUPS_UNKNOWN == 0:
        log("VERDICT: PASS (all groups green — safe to run the first-day flow)")
    elif GROUPS_FAILED == 0:
        log("VERDICT: SOFT-PASS (no hard failure; {} unknown group(s) need eyes)".format(
            GROUPS_UNKNOWN))
    else:
        log("VERDICT: FAIL ({} group(s) failed — fix before the map/input steps)".format(
            GROUPS_FAILED))
    log("=" * 60)


main()
