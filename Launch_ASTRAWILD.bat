@echo off
rem ============================================================================
rem  ASTRAWILD - Standalone Game Launcher
rem  Boots straight into the game (title screen -> New Journey / Continue).
rem  Engine auto-detection: registry first, then common install paths.
rem  Override with:  set UE_ROOT=E:\Epic Games\UnrealEngine
rem ============================================================================
title ASTRAWILD - Standalone Game
cd /d "%~dp0"
call "%~dp0_FindEngine.bat" >nul

if not defined UE_ROOT (
    echo [ERROR] Unreal Engine 5.8 not found.
    echo.
    echo Fix: install UE 5.8 via the Epic Games Launcher, or set UE_ROOT manually:
    echo     setx UE_ROOT "E:\Epic Games\UnrealEngine"
    echo ...then re-run this script.
    pause
    exit /b 1
)

echo Engine : %UE_ROOT%
echo Project: %~dp0ASTRAWILD.uproject
echo Mode   : Standalone game (-game)
echo.
echo Tip: run Setup_FirstRun.bat once after cloning, and let the editor
echo compile the project at least once before playing standalone.
echo.
start "" "%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0ASTRAWILD.uproject" -game -windowed -ResX=1920 -ResY=1080 -log
exit /b 0
