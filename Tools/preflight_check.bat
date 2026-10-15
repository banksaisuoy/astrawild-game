@echo off
setlocal enabledelayedexpansion
:: ============================================================================
:: ASTRAWILD - PREFLIGHT CHECK (LONG-RUN DIRECTIVE L9)
:: ============================================================================
:: ONE script to run on the Windows machine BEFORE anything else.
:: Verifies: engine path, VS2022 workloads, Python plugin, disk space,
:: Git LFS, and git state. Prints a clear GO / NO-GO with the reason.
::
:: Usage (from the repo root, any cmd window):
::     Tools\preflight_check.bat
:: Optional overrides:
::     set UE_ROOT=E:\Epic Games\UnrealEngine
::     set VS_ROOT=C:\Program Files\Microsoft Visual Studio\2022\Community
::
:: Exit code 0 = GO, 1 = NO-GO (first failing check named).

if "%UE_ROOT%"=="" set "UE_ROOT=E:\Epic Games\UnrealEngine"

set /a PASS=0
set /a FAIL=0
set "REASON="

echo ============================================================
echo  ASTRAWILD PREFLIGHT CHECK
echo  Engine root : %UE_ROOT%
echo  Repo        : %~dp0
echo ============================================================
echo.

:: --- 1. Engine ---------------------------------------------------------------
echo [1/7] Unreal Engine 5.8 ...
if exist "%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" (
    echo       OK: UnrealEditor.exe found.
    set /a PASS+=1
) else (
    echo       FAIL: UnrealEditor.exe NOT found under "%UE_ROOT%".
    echo             Set UE_ROOT to your UE 5.8 install root, e.g.:
    echo             set UE_ROOT=E:\Epic Games\UnrealEngine
    set "REASON=engine-not-found"
    set /a FAIL+=1
)
if exist "%UE_ROOT%\Engine\Build\BatchFiles\RunUAT.bat" (
    echo       OK: RunUAT.bat found ^(packaging path intact^).
    set /a PASS+=1
) else (
    echo       WARN: RunUAT.bat not found ^(packaging will fail; editor may still work^).
)

:: --- 2. Project file ---------------------------------------------------------
echo [2/7] Project file ...
if exist "%~dp0ASTRAWILD.uproject" (
    echo       OK: ASTRAWILD.uproject present.
    set /a PASS+=1
) else (
    echo       FAIL: ASTRAWILD.uproject not next to this script.
    set "REASON=project-missing"
    set /a FAIL+=1
)

:: --- 3. Visual Studio 2022 + C++ workload ------------------------------------
echo [3/7] Visual Studio 2022 (C++ game workload) ...
set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
set "VSPATH="
if exist "%VSWHERE%" (
    for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Workload.NativeGame -property installationPath 2^>nul`) do set "VSPATH=%%i"
)
if defined VSPATH (
    echo       OK: VS 2022 with "Game development with C++" at:
    echo            %VSPATH%
    set /a PASS+=1
) else (
    echo       FAIL: no VS 2022 install with the NativeGame workload found.
    echo             Open Visual Studio Installer and tick
    echo             "Game development with C++" (includes MSVC + Windows SDK).
    set "REASON=vs-workload-missing"
    set /a FAIL+=1
)

:: --- 4. Python Editor Script Plugin (declared for the editor) ----------------
echo [4/7] Python Editor Script Plugin ...
findstr /C:"PythonScriptPlugin" "%~dp0ASTRAWILD.uproject" >nul 2>&1
if not errorlevel 1 (
    echo       OK: PythonScriptPlugin listed in ASTRAWILD.uproject.
    set /a PASS+=1
) else (
    echo       FAIL: PythonScriptPlugin not found in ASTRAWILD.uproject -
    echo             the -run=pythonscript commandlets cannot execute.
    set "REASON=python-plugin-missing"
    set /a FAIL+=1
)

:: --- 5. Git + LFS -------------------------------------------------------------
echo [5/7] Git + Git LFS ...
git --version >nul 2>&1
if errorlevel 1 (
    echo       FAIL: git not on PATH.
    set "REASON=git-missing"
    set /a FAIL+=1
) else (
    echo       OK: git on PATH.
    set /a PASS+=1
    git lfs version >nul 2>&1
    if errorlevel 1 (
        echo       FAIL: git-lfs not on PATH ^(586 tracked objects need it^).
        echo             Install from https://git-lfs.com then: git lfs install
        set "REASON=git-lfs-missing"
        set /a FAIL+=1
    ) else (
        echo       OK: git-lfs on PATH.
        set /a PASS+=1
    )
)

:: --- 6. Git state (branch + clean tree) --------------------------------------
echo [6/7] Git state ...
git -C "%~dp0." rev-parse --abbrev-ref HEAD 2>nul | findstr /C:"final-completion" >nul
if errorlevel 1 (
    for /f "delims=" %%b in ('git -C "%~dp0." rev-parse --abbrev-ref HEAD 2^>nul') do echo       WARN: branch is %%b (working branch: final-completion).
) else (
    echo       OK: on branch final-completion.
    set /a PASS+=1
)
for /f "delims=" %%s in ('git -C "%~dp0." status --porcelain 2^>nul ^| find /c /v ""') do set DIRTY=%%s
if "%DIRTY%"=="0" (
    echo       OK: working tree clean.
    set /a PASS+=1
) else (
    echo       WARN: %DIRTY% uncommitted change^(s^) - commit or stash before the release flow.
)

:: --- 7. Disk space (needs ~10 GB) ---------------------------------------------
echo [7/7] Disk space on the project drive ...
for /f "tokens=2 delims==" %%f in ('wmic logicaldisk where "DeviceID='%~d0'" get FreeSpace /value 2^>nul ^| find "="') do set FREE=%%f
if defined FREE (
    set /a FREEGB=!FREE!/1073741824
    if !FREEGB! GEQ 10 (
        echo       OK: ~!FREEGB! GB free on %~d0
        set /a PASS+=1
    ) else (
        echo       FAIL: only ~!FREEGB! GB free on %~d0 ^(need ~10 GB: engine DDC + build + package^).
        set "REASON=disk-space"
        set /a FAIL+=1
    )
) else (
    echo       WARN: could not query free space ^(wmic unavailable^) - check manually.
)

echo.
echo ============================================================
if %FAIL% EQU 0 (
    echo  GO - all hard checks passed. Start at Hour 0-1 of
    echo       Docs\ASTRAWILD_ON_PC_TASKS.md ^(the compile gate^).
    echo  PASS=%PASS%  FAIL=0
    endlocal & exit /b 0
) else (
    echo  NO-GO - %FAIL% hard check^(s^) failed. First reason: %REASON%
    echo  Fix the FAIL lines above, then re-run this script.
    echo  PASS=%PASS%  FAIL=%FAIL%
    endlocal & exit /b 1
)
