@echo off
setlocal
set OUT=%~dp0cleanup_delete.log
set PROJECT=C:\Users\saisu\OneDrive - kmutnb.ac.th\Documents\game
set BACKUP=E:\astrawild_intermediate_backup
echo === Delete Phase started %DATE% %TIME% === > "%OUT%"

REM Verify backup exists
echo === [1/4] Verify backup === >> "%OUT%"
dir "%BACKUP%\Build" | findstr "Dir(s)" >> "%OUT%" 2>&1
dir "%BACKUP%\Crashes" | findstr "Dir(s)" >> "%OUT%" 2>&1
dir "%BACKUP%\Logs" | findstr "Dir(s)" >> "%OUT%" 2>&1
echo. >> "%OUT%"

REM 2) Delete Intermediate\Build
echo === [2/4] Delete Intermediate\Build === >> "%OUT%"
rmdir /S /Q "%PROJECT%\Intermediate\Build" >> "%OUT%" 2>&1
echo RC=%ERRORLEVEL% >> "%OUT%"
echo. >> "%OUT%"

REM 3) Delete Saved\Logs (keep only the active .log, not backups)
echo === [3/4] Delete Saved\Logs backup files (keep latest) === >> "%OUT%"
for /f "delims=" %%f in ('dir /b /a-d /od "%PROJECT%\Saved\Logs\*-backup-*.log" 2^>nul') do (
  del /Q "%PROJECT%\Saved\Logs\%%f" >> "%OUT%" 2>&1
  echo Deleted log: %%f >> "%OUT%"
)
for /f "delims=" %%f in ('dir /b /a-d /od "%PROJECT%\Saved\Logs\*-backup-*.json" 2^>nul') do (
  del /Q "%PROJECT%\Saved\Logs\%%f" >> "%OUT%" 2>&1
  echo Deleted json: %%f >> "%OUT%"
)
echo. >> "%OUT%"

REM 4) Delete Saved\Crashes (keep only 3 most recent)
echo === [4/4] Delete Saved\Crashes (keep 3 most recent) === >> "%OUT%"
set KEEP=3
set COUNT=0
for /f "delims=" %%d in ('dir /b /ad /od "%PROJECT%\Saved\Crashes" 2^>nul') do (
  set /a COUNT+=1
  set /a SKIP=COUNT-KEEP
  if !SKIP! gtr 0 (
    rmdir /S /Q "%PROJECT%\Saved\Crashes\%%d" >> "%OUT%" 2>&1
    echo Deleted crash: %%d >> "%OUT%"
  ) else (
    echo Kept crash: %%d >> "%OUT%"
  )
)
echo. >> "%OUT%"

REM Final free space
echo === Free space AFTER === >> "%OUT%"
powershell -NoProfile -Command "Get-PSDrive C | Select Used,Free,@{N='FreeGB';E={[math]::Round($_.Free/1GB,2)}} | Format-Table -AutoSize | Out-String" >> "%OUT%" 2>&1
powershell -NoProfile -Command "Get-PSDrive E | Select Used,Free,@{N='FreeGB';E={[math]::Round($_.Free/1GB,2)}} | Format-Table -AutoSize | Out-String" >> "%OUT%" 2>&1

type "%OUT%"
endlocal
