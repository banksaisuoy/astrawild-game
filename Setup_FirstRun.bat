@echo off
rem ============================================================================
rem  ASTRAWILD - First-Run Setup
rem  1. Verifies git + git-lfs and pulls the real binary assets (maps,
rem     textures, meshes, audio) - a plain clone without LFS only contains
rem     ~130-byte pointer files and the game cannot boot.
rem  2. Validates the critical map/material files are real UE packages.
rem  Run this ONCE after cloning the repository.
rem ============================================================================
title ASTRAWILD - First-Run Setup
cd /d "%~dp0"
echo ============================================================
echo  ASTRAWILD - First-Run Setup
echo ============================================================
echo.

where git >nul 2>nul
if errorlevel 1 (
    echo [ERROR] git is not installed or not on PATH.
    echo         Install from https://git-scm.com/download/win then re-run.
    pause
    exit /b 1
)

git lfs version >nul 2>nul
if errorlevel 1 (
    echo [ERROR] git-lfs is not installed.
    echo         The repository stores maps/textures/meshes/audio in Git LFS.
    echo         Install git-lfs from https://git-lfs.com then run:
    echo             git lfs install
    echo             git lfs pull
    echo         ...and re-run this script to verify.
    echo.
    echo (You can still open the project without the art pack - the runtime
    echo  world generates procedurally - but the boot maps need LFS.)
    pause
    exit /b 1
)

echo [1/2] Pulling Git LFS assets (maps, textures, meshes, audio)...
git lfs pull
if errorlevel 1 (
    echo [ERROR] git lfs pull failed. Check your network / GitHub access.
    pause
    exit /b 1
)
echo       Done.
echo.

echo [2/2] Verifying critical content files...
set "BROKEN=0"
for %%F in ("Content\ThirdPerson\Lvl_ThirdPerson.umap" "Content\ASTRAWILD\Maps\MainMap.umap" "Content\Materials\M_Master_Surface.uasset") do (
    if not exist "%%~F" (
        echo [ERROR] Missing: %%~F
        set "BROKEN=1"
    ) else if %%~zF LSS 1000 (
        echo [ERROR] %%~F is still an LFS pointer file ^(%%~zF bytes^) - LFS pull incomplete.
        set "BROKEN=1"
    ) else (
        echo        OK: %%~F ^(%%~zF bytes^)
    )
)
if "%BROKEN%"=="1" (
    echo.
    echo [ERROR] Content verification failed - the game cannot boot yet.
    echo         Re-run: git lfs install ^&^& git lfs pull
    pause
    exit /b 1
)

echo.
echo ============================================================
echo  Setup complete.
echo  Next steps:
echo    - Launch_ASTRAWILD_Editor.bat  (open the editor, first compile)
echo    - Import_Art.bat               (optional: import the art pack)
echo    - Launch_ASTRAWILD.bat         (play standalone)
echo    - Package_Game.bat             (build a shipping .exe)
echo ============================================================
pause
exit /b 0
