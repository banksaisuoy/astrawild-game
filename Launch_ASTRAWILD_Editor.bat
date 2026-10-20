@echo off
rem ============================================================================
rem  ASTRAWILD - Open in Unreal Engine 5 Editor
rem  Engine auto-detection: registry first, then common install paths.
rem  Override with:  set UE_ROOT=E:\Epic Games\UnrealEngine
rem ============================================================================
title ASTRAWILD - Unreal Engine 5 Editor
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
echo.
echo First launch compiles the AstrawildCore C++ module (needs Visual Studio
echo 2022 with the "Game development with C++" workload). This can take a few
echo minutes - one time only.
echo.
start "" "%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "%~dp0ASTRAWILD.uproject"
exit /b 0
