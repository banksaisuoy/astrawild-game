@echo off
setlocal
set UE_ROOT=E:\Epic Games\UnrealEngine
set PROJECT=C:\Users\saisu\OneDrive - kmutnb.ac.th\Documents\game\ASTRAWILD.uproject

REM Force-touch the public data header so UHT re-emits the generated.h
copy /b "Source\AstrawildCore\Public\World\AstrawildWorldData.h" +,, >nul 2>&1
copy /b "Source\AstrawildCore\Public\AstrawildTypes.h" +,, >nul 2>&1
copy /b "Source\AstrawildCore\Public\World\AstrawildUnderwaterData.h" +,, >nul 2>&1

echo === ASTRAWILD Phase 4b: rebuild after touching data headers (no UBA) ===
"%UE_ROOT%\Engine\Build\BatchFiles\Build.bat" ASTRAWILDEditor Win64 Development -Project="%PROJECT%" -WaitMutex -NoHotReload -NoUBA
set RC=%ERRORLEVEL%
echo === Build exit code: %RC% ===
endlocal & exit /b %RC%
