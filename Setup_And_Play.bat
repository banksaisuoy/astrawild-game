@echo off
REM ============================================================================
REM  ASTRAWILD — SETUP AND PLAY (one click, Windows)
REM
REM  Runs the full ASSET OVERHAUL pipeline in the Unreal Editor and leaves the
REM  editor open on the showcase map (/Game/Maps/L_Showcase_ArtOverhaul):
REM    1. Imports every real CC0 mesh (109 unique models: 3 survivor armor
REM       tiers, 42 echo species, 16 base archetypes, 17 bosses, 5 weapons,
REM       4 vehicles, 4 resource nodes, 25 environment props) + textures/audio.
REM    2. Renames animation clips to the runtime AM_ convention (clip_map).
REM    3. Builds PBR materials (ore-node emissive, master instances).
REM    4. Adds sockets (weapon Muzzle, survivor Weapon_R).
REM    5. Places PlayerStart + every mesh on the showcase level ground.
REM  Then: just press Play (PIE) in the editor.
REM
REM  Requirements: Unreal Engine 5.8 (or 5.4+) installed; this repo checked
REM  out with Git LFS (the CC0 packs live in ArtSource/Models/).
REM ============================================================================

setlocal enabledelayedexpansion
cd /d "%~dp0"

echo [1/3] Locating Unreal Editor...

set "UE_EDITOR="
if defined UE_ROOT (
    if exist "%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" (
        set "UE_EDITOR=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe"
    )
)
if not defined UE_EDITOR (
    for %%V in (5.8 5.7 5.6 5.5 5.4 5.3) do (
        if not defined UE_EDITOR (
            if exist "C:\Program Files\Epic Games\UE_%%V\Engine\Binaries\Win64\UnrealEditor.exe" (
                set "UE_EDITOR=C:\Program Files\Epic Games\UE_%%V\Engine\Binaries\Win64\UnrealEditor.exe"
                echo       found UE %%V
            )
        )
    )
)
if not defined UE_EDITOR (
    echo   ERROR: UnrealEditor.exe not found.
    echo   Set the UE_ROOT environment variable to your engine install root
    echo   ^(e.g. C:\Program Files\Epic Games\UE_5.8^) and run this again.
    pause
    exit /b 1
)
echo       using !UE_EDITOR!

echo [2/3] Verifying the real-asset catalog...

if not exist "ArtSource\manifest.json" (
    echo   ERROR: ArtSource\manifest.json missing — re-checkout with Git LFS.
    pause
    exit /b 1
)
python Scripts\fetch_free_assets.py >nul 2>&1
if errorlevel 1 (
    echo   WARNING: fetch_free_assets.py reported missing sources — continuing
    echo   with whatever is present; check Docs\ASTRAWILD_ASSET_OVERHAUL_REPORT.json
) else (
    echo       catalog OK — 109 unique real CC0 meshes, manifest 100%% present
)

echo [3/3] Launching the editor with the overhaul pipeline...

"!UE_EDITOR!" "%~dp0ASTRAWILD.uproject" -ExecutePythonScript="%~dp0Content\Python\AwPipeline\run_overhaul.py"

echo.
echo Done — the editor opened the showcase map. Press Play (PIE) to test.
echo Report: Saved\AwPipelineReport\import_report.json
pause
