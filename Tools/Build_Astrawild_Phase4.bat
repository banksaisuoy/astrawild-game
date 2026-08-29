@echo off
setlocal
set UE_ROOT=E:\Epic Games\UnrealEngine
set PROJECT=C:\Users\saisu\OneDrive - kmutnb.ac.th\Documents\game\ASTRAWILD.uproject
set TARGET=ASTRAWILDEditor
set PLATFORM=Win64
set CONFIG=Development
set LOG=%~dp0build_phase4.log

echo === ASTRAWILD Phase 4: UE 5.8 Compile ===
echo UE_ROOT  : %UE_ROOT%
echo PROJECT  : %PROJECT%
echo TARGET   : %TARGET% %PLATFORM% %CONFIG%
echo LOG      : %LOG%
echo.

"%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" %TARGET% %PLATFORM% %CONFIG% -Project="%PROJECT%" -WaitMutex -NoHotReload >> "%LOG%" 2>&1
set RC=%ERRORLEVEL%
echo.
echo === Build exit code: %RC% ===
if %RC% neq 0 (
    echo === LAST 80 LINES OF LOG ===
    powershell -NoProfile -Command "Get-Content -Path '%LOG%' -Tail 80"
)
endlocal & exit /b %RC%
