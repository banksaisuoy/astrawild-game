@echo off
rem ============================================================================
rem  ASTRAWILD - Compile C++ (Development Editor target)
rem  Equivalent to the old Build.ps1 without the hardcoded paths.
rem  Requires: UE 5.8 + Visual Studio 2022.
rem ============================================================================
title ASTRAWILD - Compile AstrawildCore
cd /d "%~dp0"
call "%~dp0_FindEngine.bat" >nul

if not defined UE_ROOT (
    echo [ERROR] Unreal Engine 5.8 not found.
    echo         Install it or set UE_ROOT, then re-run.
    pause
    exit /b 1
)

set "UBT=%UE_ROOT%\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
if not exist "%UBT%" (
    echo [ERROR] UnrealBuildTool.exe not found under the engine directory.
    pause
    exit /b 1
)

echo Engine  : %UE_ROOT%
echo Target  : ASTRAWILDEditor Win64 Development
echo.

"%UBT%" ASTRAWILDEditor Win64 Development -Project="%~dp0ASTRAWILD.uproject" -WaitMutex
if errorlevel 1 (
    echo.
    echo [ERROR] Build failed - fix the errors above and re-run.
    pause
    exit /b 1
)

echo.
echo Build succeeded. The editor will hot-reload the module next launch
echo (or is already running it if launched from the editor).
pause
exit /b 0
