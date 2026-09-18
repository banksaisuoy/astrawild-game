"""
ASTRAWILD AwPipeline — ASSET OVERHAUL one-shot runner (Windows Setup_And_Play.bat
entry point). Executes the full pipeline in-editor:

  1. import_all.py      — import every real CC0 mesh/texture/audio from
                          ArtSource/manifest.json (status present, 0 pending),
                          rename clips to the AM_ convention via clip_map,
                          build PBR materials + node emissive + sockets.
  2. build_showcase_map.py — create/open /Game/Maps/L_Showcase_ArtOverhaul with
                          PlayerStart + every real mesh placed on real ground.

Launched by:
    Setup_And_Play.bat
      UnrealEditor.exe ASTRAWILD.uproject -ExecutePythonScript="Content/Python/AwPipeline/run_overhaul.py"
The editor stays open afterwards — press Play (PIE) in the showcase level.
"""
import os
import sys
import traceback

_HERE = os.path.dirname(os.path.abspath(__file__))
if _HERE not in sys.path:
    sys.path.insert(0, _HERE)

import unreal  # noqa: E402


def run() -> int:
    import import_all
    import build_showcase_map
    ok = 0
    try:
        import_all.main()
    except Exception as e:  # noqa: BLE001
        unreal.log_error(f"[Overhaul] import_all crashed: {e}\n{traceback.format_exc()}")
        ok = 1
    try:
        if build_showcase_map.main() != 0:
            ok = 1
    except Exception as e:  # noqa: BLE001
        unreal.log_error(f"[Overhaul] build_showcase_map crashed: {e}\n{traceback.format_exc()}")
        ok = 1
    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    except Exception as e:  # noqa: BLE001
        unreal.log_warning(f"[Overhaul] final save: {e}")
    unreal.log("[Overhaul] pipeline complete — the editor is open on the showcase map; "
               "press Play (PIE) to test with the real meshes.")
    return ok


if __name__ == "__main__":
    sys.exit(run())
