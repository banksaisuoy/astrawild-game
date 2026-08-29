@echo off
setlocal
set UE_ROOT=E:\Epic Games\UnrealEngine
set PROJECT=C:\Users\saisu\OneDrive - kmutnb.ac.th\Documents\game\ASTRAWILD.uproject
set EDITOR_EXE=%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe
set SCRIPT_PATH=C:\Users\saisu\OneDrive - kmutnb.ac.th\Documents\game\Scripts\import_all_datatables.py
set LOG=%~dp0phase5_import.log

echo === ASTRAWILD Phase 5: DataTable import + map load (headless commandlet) ===
echo EDITOR  : %EDITOR_EXE%
echo PROJECT : %PROJECT%
echo SCRIPT  : %SCRIPT_PATH%
echo LOG     : %LOG%
echo.

"%EDITOR_EXE%" "%PROJECT%" -run=pythonscript -script="%SCRIPT_PATH%" -unattended -nopause -nullrhi -nosplash -log -stdout -FullStdOutLogOutput -ABSLOG="%LOG%" -NoLiveCoding
set RC=%ERRORLEVEL%
echo.
echo === Editor commandlet exit code: %RC% ===
if %RC% neq 0 (
    echo === LAST 120 LINES OF LOG ===
    powershell -NoProfile -Command "Get-Content -Path '%LOG%' -Tail 120"
)
endlocal & exit /b %RC%
