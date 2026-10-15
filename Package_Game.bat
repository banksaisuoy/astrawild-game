@echo off
rem ============================================================================
rem  ASTRAWILD - Package Shipping Build (Win64)
rem  Produces: Packaged\Windows\ASTRAWILD.exe  (standalone shipping game)
rem  Requires: UE 5.8 + Visual Studio 2022 (compiles the game target).
rem ============================================================================
title ASTRAWILD - Package Shipping Build
cd /d "%~dp0"
call "%~dp0_FindEngine.bat" >nul

if not defined UE_ROOT (
    echo [ERROR] Unreal Engine 5.8 not found.
    echo         Install it or set UE_ROOT, then re-run.
    pause
    exit /b 1
)

set "UAT=%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat"
if not exist "%UAT%" (
    echo [ERROR] RunUAT.bat not found under the engine directory.
    pause
    exit /b 1
)

echo Engine  : %UE_ROOT%
echo Target  : ASTRAWILD Win64 Shipping
echo Output  : %~dp0Packaged\Windows
echo.
echo This compiles, cooks all maps and stages a monolithic pak - expect
echo 10-30 minutes depending on machine.
echo.

call "%UAT%" -ScriptsForProject="%~dp0ASTRAWILD.uproject" BuildCookRun ^
    -nocompileeditor -platform=Win64 -clientconfig=Shipping ^
    -project="%~dp0ASTRAWILD.uproject" -cook -allmaps -build -stage -pak ^
    -archive -archiveddirectory="%~dp0Packaged\Windows"

if errorlevel 1 (
    echo.
    echo [ERROR] Packaging failed - see the log above.
    pause
    exit /b 1
)

echo.
echo ============================================================
echo  Shipping build ready: Packaged\Windows\ASTRAWILD.exe
echo ============================================================
pause
exit /b 0
