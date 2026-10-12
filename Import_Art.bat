@echo off
rem ============================================================================
rem  ASTRAWILD - Art Pack Import (optional, recommended)
rem  Launches the editor headless-interactive and runs the AwPipeline python
rem  import: ArtSource (112 assets: meshes, textures, audio, materials)
rem  -> /Game content. After this the world uses real 3D trees/rocks/echoes,
rem  the survivor exosuit character and weapon models.
rem
rem  The game is fully playable WITHOUT this step (procedural fallbacks) -
rem  this upgrades the visuals to the art-pack tier.
rem ============================================================================
title ASTRAWILD - Art Pack Import
cd /d "%~dp0"
call "%~dp0_FindEngine.bat" >nul

if not defined UE_ROOT (
    echo [ERROR] Unreal Engine 5.8 not found. Run Launch_ASTRAWILD_Editor.bat
    echo for troubleshooting guidance.
    pause
    exit /b 1
)

echo Engine : %UE_ROOT%
echo Pipeline: Content\Python\AwPipeline\import_all.py
echo.
echo The editor opens and imports 112 assets, builds materials and Niagara
echo systems, then writes a report to Saved\AwPipelineReport\import_report.json.
echo This can take several minutes - watch the log window.
echo.
start "" /wait "%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0ASTRAWILD.uproject" -ExecutePythonScript="%~dp0Content\Python\AwPipeline\import_all.py"
echo.
echo Import pass finished. Check Saved\AwPipelineReport\import_report.json
echo (missing should be 0) and save any dirty packages when prompted.
pause
exit /b 0
