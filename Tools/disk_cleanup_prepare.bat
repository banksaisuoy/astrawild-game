@echo off
setlocal
set OUT=%~dp0cleanup.log
set PROJECT=C:\Users\saisu\OneDrive - kmutnb.ac.th\Documents\game
set BACKUP=E:\astrawild_intermediate_backup
echo === Cleanup started %DATE% %TIME% === > "%OUT%"

REM 1) Backup Intermediate\Build to E:
echo === [1/5] Backup Intermediate\Build (13.9 GB) to E:\astrawild_intermediate_backup === >> "%OUT%"
if not exist "%BACKUP%" mkdir "%BACKUP%"
echo Backup source: %PROJECT%\Intermediate\Build >> "%OUT%"
echo Backup dest:   %BACKUP%\Build >> "%OUT%"
robocopy "%PROJECT%\Intermediate\Build" "%BACKUP%\Build" /E /R:2 /W:5 /NP /NFL /NDL >> "%OUT%" 2>&1
echo Robocopy RC=%ERRORLEVEL% >> "%OUT%"
echo. >> "%OUT%"

REM 2) Backup Saved\Crashes to E:
echo === [2/5] Backup Saved\Crashes (10.9 MB / 115 files) to E:\astrawild_intermediate_backup === >> "%OUT%"
robocopy "%PROJECT%\Saved\Crashes" "%BACKUP%\Crashes" /E /R:2 /W:5 /NP /NFL /NDL >> "%OUT%" 2>&1
echo Robocopy RC=%ERRORLEVEL% >> "%OUT%"
echo. >> "%OUT%"

REM 3) Backup Saved\Logs to E:
echo === [3/5] Backup Saved\Logs (3.4 MB / 32 files) to E:\astrawild_intermediate_backup === >> "%OUT%"
robocopy "%PROJECT%\Saved\Logs" "%BACKUP%\Logs" /E /R:2 /W:5 /NP /NFL /NDL >> "%OUT%" 2>&1
echo Robocopy RC=%ERRORLEVEL% >> "%OUT%"
echo. >> "%OUT%"

REM 4) Free space check
echo === [4/5] Free space BEFORE delete === >> "%OUT%"
powershell -NoProfile -Command "Get-PSDrive C | Select Used,Free,@{N='FreeGB';E={[math]::Round($_.Free/1GB,2)}} | Format-Table -AutoSize | Out-String" >> "%OUT%" 2>&1
powershell -NoProfile -Command "Get-PSDrive E | Select Used,Free,@{N='FreeGB';E={[math]::Round($_.Free/1GB,2)}} | Format-Table -AutoSize | Out-String" >> "%OUT%" 2>&1
echo. >> "%OUT%"

REM 5) Deletion summary (after user confirmation; for now just log plan)
echo === [5/5] DELETE PLAN (will run on user OK) === >> "%OUT%"
echo rmdir /S /Q "%PROJECT%\Intermediate\Build" >> "%OUT%"
echo keep "%PROJECT%\Intermediate\CachedAssetRegistry" (UE re-uses) >> "%OUT%"
echo keep "%PROJECT%\Intermediate\PipInstall" >> "%OUT%"
echo keep "%PROJECT%\Intermediate\ProjectFiles" >> "%OUT%"
echo keep "%PROJECT%\Intermediate\ReimportCache" >> "%OUT%"
echo. >> "%OUT%"

echo === Done === >> "%OUT%"
type "%OUT%"
endlocal
