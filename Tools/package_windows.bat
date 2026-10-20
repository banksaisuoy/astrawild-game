@echo off
setlocal
:: ===========================================================================
:: ASTRAWILD - one-click Win64 SHIPPING packaging (ENGINE-RUN-1 / TASK 4)
::
:: Runs Unreal Automation Tool (RunUAT BuildCookRun) for:
::     platform=Win64  clientconfig=Shipping  cook -allmaps -stage -pak
:: and archives the staged build to:
::     %ASTRAWILD_ARCHIVE%\Windows\ASTRAWILD\Binaries\Win64\ASTRAWILD.exe
::
:: Environment overrides (all optional - defaults match the documented
:: machine layout, same convention as Build.ps1 / Build_Package.ps1):
::     UE_ROOT            engine root   (default: E:\Epic Games\UnrealEngine)
::     ASTRAWILD_ARCHIVE  archive root  (default: <repo>\Build)
:: ===========================================================================

if "%UE_ROOT%"=="" set "UE_ROOT=E:\Epic Games\UnrealEngine"
set "PROJECT=%~dp0ASTRAWILD.uproject"
if "%ASTRAWILD_ARCHIVE%"=="" set "ASTRAWILD_ARCHIVE=%~dp0Build"
set "RUNUAT=%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat"

if not exist "%PROJECT%" (
    echo [ERROR] ASTRAWILD.uproject not found next to this script: %~dp0
    exit /b 1
)
if not exist "%RUNUAT%" (
    echo [ERROR] RunUAT.bat not found under engine root "%UE_ROOT%".
    echo         Set UE_ROOT to your Unreal Engine 5.8 install root.
    exit /b 1
)

echo ========================================
echo  ASTRAWILD - Win64 Shipping packaging
echo  Engine  : %UE_ROOT%
echo  Project : %PROJECT%
echo  Archive : %ASTRAWILD_ARCHIVE%\Windows\ASTRAWILD
echo ========================================

call "%RUNUAT%" -ScriptsForProject="%PROJECT%" BuildCookRun ^
 -nocompileeditor -installed -nop4 ^
 -project="%PROJECT%" ^
 -cook -allmaps -stage -pak -package ^
 -clientconfig=Shipping ^
 -platform=Win64 ^
 -build ^
 -archive -archivedirectory="%ASTRAWILD_ARCHIVE%"

if errorlevel 1 (
    echo.
    echo [ERROR] Packaging FAILED - see the UAT output above.
    exit /b 1
)

echo.
echo [OK] Packaging succeeded.
echo      Game executable: %ASTRAWILD_ARCHIVE%\Windows\ASTRAWILD\Binaries\Win64\ASTRAWILD.exe
echo      (runs on any Windows 10/11 x64 PC - no Unreal Engine required)
endlocal
