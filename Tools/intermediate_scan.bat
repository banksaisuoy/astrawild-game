@echo off
setlocal
set OUT=%~dp0intermediate_scan.log
echo === Intermediate Scan %DATE% %TIME% === > "%OUT%"
powershell -NoProfile -Command "$ErrorActionPreference='SilentlyContinue'; $dirs=@('Intermediate\Build','Intermediate\CachedAssetRegistry','Intermediate\PipInstall','Intermediate\ProjectFiles','Intermediate\ReimportCache','Intermediate\Sandboxes','Saved\Astrawild','Saved\Logs','Saved\Crashes','Saved\Crashes\UECC-Windows-4BFF13AA42591A28924D3CA714618159_0000'); foreach($d in $dirs){$p=Join-Path '%CD%' $d; if(Test-Path $p){$s=(Get-ChildItem $p -Recurse -File -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum;$c=(Get-ChildItem $p -Recurse -File -ErrorAction SilentlyContinue | Measure-Object).Count;Write-Host (\"{0,-80} {1,8:N2} MB  ({2} files)\" -f $d, ($s/1MB), $c)}else{Write-Host (\"{0,-80} (missing)\" -f $d)}}" >> "%OUT%" 2>&1
powershell -NoProfile -Command "$ErrorActionPreference='SilentlyContinue'; Get-ChildItem 'Saved\Crashes' -Directory -ErrorAction SilentlyContinue | Select-Object -First 5 Name,LastWriteTime,@{N='SizeMB';E={[math]::Round(((Get-ChildItem $_.FullName -Recurse -File -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum/1MB),2)}} | Format-Table -AutoSize | Out-String" >> "%OUT%" 2>&1
powershell -NoProfile -Command "$ErrorActionPreference='SilentlyContinue'; Get-ChildItem 'Saved\Logs' -File -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 5 Name,LastWriteTime,@{N='SizeMB';E={[math]::Round($_.Length/1MB,2)}} | Format-Table -AutoSize | Out-String" >> "%OUT%" 2>&1
type "%OUT%"
endlocal
