@echo off
setlocal
set UE_ROOT=E:\Epic Games\UnrealEngine
set PROJECT=C:\Users\saisu\OneDrive - kmutnb.ac.th\Documents\game\ASTRAWILD.uproject
set MAP=/Game/Astrawild/Maps/LV_DawnValley_Main
set LOG=%~dp0phase6_pie.log

echo === ASTRAWILD Phase 6: PIE Smoke Test ===
echo EDITOR  : %UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe
echo PROJECT : %PROJECT%
echo MAP     : %MAP%
echo LOG     : %LOG%
echo.

REM Start PIE in standalone game mode (no editor UI), quit after 30s
"%UE_ROOT%\Engine\Binaries\Win64\UnrealEditor.exe" "%PROJECT%" %MAP% -game -unattended -nopause -nosplash -log -stdout -FullStdOutLogOutput -ABSLOG="%LOG%" -ExecCmds="Quit" -NoLiveCoding
set RC=%ERRORLEVEL%
echo === PIE exit code: %RC% ===
if %RC% neq 0 (
    echo === LAST 80 LINES OF LOG ===
    powershell -NoProfile -Command "Get-Content -Path '%LOG%' -Tail 80"
)
endlocal & exit /b %RC%
