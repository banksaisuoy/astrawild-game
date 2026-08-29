@echo off
setlocal
set OUT=%~dp0cleanup_delete2.log
set PROJECT=C:\Users\saisu\OneDrive - kmutnb.ac.th\Documents\game
echo === Delete Phase 2 started %DATE% %TIME% === > "%OUT%"

REM 1) Verify Intermediate\Build deleted
echo === [1/3] Verify Intermediate\Build deletion === >> "%OUT%"
if exist "%PROJECT%\Intermediate\Build" (
    echo WARNING: Build dir still exists, trying rmdir... >> "%OUT%"
    rmdir /S /Q "%PROJECT%\Intermediate\Build" >> "%OUT%" 2>&1
    echo RC=%ERRORLEVEL% >> "%OUT%"
) else (
    echo OK: Intermediate\Build already gone >> "%OUT%"
)
echo. >> "%OUT%"

REM 2) Delete Saved\Crashes (keep 3 most recent by LastWriteTime)
echo === [2/3] Delete Saved\Crashes (keep 3 most recent) === >> "%OUT%"
powershell -NoProfile -Command "$ErrorActionPreference='SilentlyContinue'; $dirs = Get-ChildItem '%PROJECT%\Saved\Crashes' -Directory | Sort-Object LastWriteTime -Descending; $i=0; foreach($d in $dirs) { $i++; if($i -gt 3) { Write-Host \"Deleting: $($d.Name) (age: $((Get-Date) - $d.LastWriteTime).Days)d\"; Remove-Item $d.FullName -Recurse -Force } else { Write-Host \"Keeping : $($d.Name) (age: $((Get-Date) - $d.LastWriteTime).Days)d\" } }" >> "%OUT%" 2>&1
echo. >> "%OUT%"

REM 3) Delete Intermediate\ProjectFiles (53MB, can be regenerated)
echo === [3/3] Delete Intermediate\ProjectFiles (53MB, regenerable) === >> "%OUT%"
rmdir /S /Q "%PROJECT%\Intermediate\ProjectFiles" >> "%OUT%" 2>&1
echo RC=%ERRORLEVEL% >> "%OUT%"
echo. >> "%OUT%"

REM Final free space
echo === Free space AFTER === >> "%OUT%"
powershell -NoProfile -Command "Get-PSDrive C | Select Used,Free,@{N='FreeGB';E={[math]::Round($_.Free/1GB,2)}} | Format-Table -AutoSize | Out-String" >> "%OUT%" 2>&1
powershell -NoProfile -Command "Get-PSDrive E | Select Used,Free,@{N='FreeGB';E={[math]::Round($_.Free/1GB,2)}} | Format-Table -AutoSize | Out-String" >> "%OUT%" 2>&1

type "%OUT%"
endlocal
