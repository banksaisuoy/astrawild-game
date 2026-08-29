@echo off
title ASTRAWILD: One-Click Engine Compiler & Game Launcher
color 0A
cls
echo ===============================================================================
echo     ASTRAWILD: Echoes of the First Dawn - Autonomous Game Launcher
echo ===============================================================================
echo.

set PROJECT_ROOT=%~dp0
set PROJECT_FILE=%PROJECT_ROOT%ASTRAWILD.uproject

echo [1/4] Detecting Unreal Engine 5 Installation...
set UE_PATH=

if exist "E:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" set UE_PATH=E:\Epic Games\UE_5.8
if exist "E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" set UE_PATH=E:\UE_5.8
if exist "D:\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" set UE_PATH=D:\Epic Games\UE_5.8
if exist "C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe" set UE_PATH=C:\Program Files\Epic Games\UE_5.8

if exist "E:\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe" set UE_PATH=E:\Epic Games\UE_5.7
if exist "D:\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe" set UE_PATH=D:\Epic Games\UE_5.7
if exist "C:\Program Files\Epic Games\UE_5.7\Engine\Binaries\Win64\UnrealEditor.exe" set UE_PATH=C:\Program Files\Epic Games\UE_5.7

if exist "E:\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe" set UE_PATH=E:\Epic Games\UE_5.6
if exist "D:\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe" set UE_PATH=D:\Epic Games\UE_5.6
if exist "C:\Program Files\Epic Games\UE_5.6\Engine\Binaries\Win64\UnrealEditor.exe" set UE_PATH=C:\Program Files\Epic Games\UE_5.6

if exist "E:\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe" set UE_PATH=E:\Epic Games\UE_5.5
if exist "D:\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe" set UE_PATH=D:\Epic Games\UE_5.5
if exist "C:\Program Files\Epic Games\UE_5.5\Engine\Binaries\Win64\UnrealEditor.exe" set UE_PATH=C:\Program Files\Epic Games\UE_5.5

if "%UE_PATH%"=="" (
    echo [ERROR] Unreal Engine 5 was not detected automatically.
    echo Please open Epic Games Launcher and complete the Unreal Engine download to Drive E:.
    echo Once downloaded, run this batch file again!
    echo.
    pause
    exit /b 1
)

echo [OK] Found Unreal Engine at: %UE_PATH%
echo.

echo [2/4] Compiling 178 C++ Files with UnrealBuildTool...
call "%UE_PATH%\Engine\Build\BatchFiles\Build.bat" ASTRAWILDEditor Win64 Development "%PROJECT_FILE%" -WaitMutex
if %ERRORLEVEL% NEQ 0 (
    echo [ERROR] C++ Compilation encountered an issue. Exit code: %ERRORLEVEL%
    pause
    exit /b %ERRORLEVEL%
)

echo [OK] C++ Compilation Successful!
echo.

echo [3/4] Launching Unreal Editor & Running Asset Auto-Importer...
start "" "%UE_PATH%\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJECT_FILE%" -ExecOnSuccess="py %PROJECT_ROOT%Scripts\import_all_datatables.py"
echo.
echo ===============================================================================
echo     ASTRAWILD is launching! Press Play (Alt+P) in Editor to play!
echo ===============================================================================
pause