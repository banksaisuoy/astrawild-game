@echo off
rem ============================================================================
rem  ASTRAWILD - engine locator helper
rem  Sets UE_ROOT (root of the Unreal Engine installation) for the caller.
rem  Detection order:
rem    1. Existing UE_ROOT environment variable (manual override)
rem    2. Launcher registry entry for UE 5.8 (HKLM)
rem    3. Source-build registry entries (HKCU\...\Unreal Engine\Builds)
rem    4. Well-known install locations
rem  Call from another script:   call "%~dp0_FindEngine.bat"
rem ============================================================================
setlocal enabledelayedexpansion
if defined UE_ROOT goto :done

rem --- 2) Launcher registry (pinned engine version 5.8) ----------------------
for /f "skip=1 tokens=2*" %%A in ('reg query "HKLM\SOFTWARE\Epic Games\Unreal Engine\5.8" /v InstalledDirectory 2^>nul') do (
    if exist "%%B\Engine\Binaries\Win64\UnrealEditor.exe" set "UE_ROOT=%%B"
)
if defined UE_ROOT goto :done

rem --- 3) Source-build registry (any registered source engine) ----------------
for /f "skip=1 tokens=2*" %%A in ('reg query "HKCU\Software\Epic Games\Unreal Engine\Builds" 2^>nul') do (
    if exist "%%B\Engine\Binaries\Win64\UnrealEditor.exe" set "UE_ROOT=%%B"
)
if defined UE_ROOT goto :done

rem --- 4) Well-known locations ------------------------------------------------
for %%D in (
    "E:\Epic Games\UnrealEngine"
    "C:\Program Files\Epic Games\UE_5.8"
    "D:\Program Files\Epic Games\UE_5.8"
    "E:\Program Files\Epic Games\UE_5.8"
    "D:\Epic Games\UE_5.8"
    "C:\Program Files\Epic Games\UE_5.7"
    "D:\Epic Games\UE_5.7"
    "E:\Epic Games\UE_5.8"
) do (
    if not defined UE_ROOT if exist "%%~D\Engine\Binaries\Win64\UnrealEditor.exe" set "UE_ROOT=%%~D"
)

:done
endlocal & set "UE_ROOT=%UE_ROOT%"
exit /b 0
