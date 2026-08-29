@echo off
setlocal
set OUT=%~dp0disk_scan.log
echo === Disk C: scan at %DATE% %TIME% === > "%OUT%"
echo. >> "%OUT%"

echo === Top 25 dirs C:\Users\saisu (1-level) === >> "%OUT%"
powershell -NoProfile -Command "$ErrorActionPreference='SilentlyContinue'; Get-ChildItem 'C:\Users\saisu' -Directory | ForEach-Object { $sb=(Get-ChildItem $_.FullName -Recurse -File -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum; [PSCustomObject]@{Path=$_.FullName;SizeGB=[math]::Round($sb/1GB,2)} } | Sort SizeGB -Desc | Select -First 25 Path,SizeGB | Format-Table -AutoSize | Out-String -Width 4096" >> "%OUT%" 2>&1
echo. >> "%OUT%"

echo === Top 20 dirs C:\Users\saisu\AppData\Local (1-level) === >> "%OUT%"
powershell -NoProfile -Command "$ErrorActionPreference='SilentlyContinue'; Get-ChildItem 'C:\Users\saisu\AppData\Local' -Directory | ForEach-Object { $sb=(Get-ChildItem $_.FullName -Recurse -File -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum; [PSCustomObject]@{Path=$_.FullName;SizeGB=[math]::Round($sb/1GB,2)} } | Sort SizeGB -Desc | Select -First 20 Path,SizeGB | Format-Table -AutoSize | Out-String -Width 4096" >> "%OUT%" 2>&1
echo. >> "%OUT%"

echo === Top 15 dirs C:\Users\saisu\AppData\Roaming (1-level) === >> "%OUT%"
powershell -NoProfile -Command "$ErrorActionPreference='SilentlyContinue'; Get-ChildItem 'C:\Users\saisu\AppData\Roaming' -Directory | ForEach-Object { $sb=(Get-ChildItem $_.FullName -Recurse -File -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum; [PSCustomObject]@{Path=$_.FullName;SizeGB=[math]::Round($sb/1GB,2)} } | Sort SizeGB -Desc | Select -First 15 Path,SizeGB | Format-Table -AutoSize | Out-String -Width 4096" >> "%OUT%" 2>&1
echo. >> "%OUT%"

echo === Top 10 Program Files (x86) === >> "%OUT%"
powershell -NoProfile -Command "$ErrorActionPreference='SilentlyContinue'; Get-ChildItem 'C:\Program Files (x86)' -Directory | ForEach-Object { $sb=(Get-ChildItem $_.FullName -Recurse -File -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum; [PSCustomObject]@{Path=$_.FullName;SizeGB=[math]::Round($sb/1GB,2)} } | Sort SizeGB -Desc | Select -First 10 Path,SizeGB | Format-Table -AutoSize | Out-String -Width 4096" >> "%OUT%" 2>&1
echo. >> "%OUT%"

echo === Top 10 Program Files === >> "%OUT%"
powershell -NoProfile -Command "$ErrorActionPreference='SilentlyContinue'; Get-ChildItem 'C:\Program Files' -Directory | ForEach-Object { $sb=(Get-ChildItem $_.FullName -Recurse -File -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum; [PSCustomObject]@{Path=$_.FullName;SizeGB=[math]::Round($sb/1GB,2)} } | Sort SizeGB -Desc | Select -First 10 Path,SizeGB | Format-Table -AutoSize | Out-String -Width 4096" >> "%OUT%" 2>&1
echo. >> "%OUT%"

echo === Files > 200MB on C: (top 30) === >> "%OUT%"
powershell -NoProfile -Command "$ErrorActionPreference='SilentlyContinue'; Get-ChildItem 'C:\Users\saisu','C:\ProgramData','C:\Windows\Temp','C:\Users\saisu\AppData\Local\Temp','C:\Users\saisu\AppData\Local\Microsoft','C:\$Recycle.Bin' -Recurse -File -Force -ErrorAction SilentlyContinue | Where-Object { $_.Length -gt 200MB } | Sort Length -Desc | Select -First 30 FullName,@{N='SizeGB';E={[math]::Round($_.Length/1GB,2)}} | Format-Table -AutoSize | Out-String -Width 4096" >> "%OUT%" 2>&1
echo. >> "%OUT%"

echo === Temp dirs size === >> "%OUT%"
powershell -NoProfile -Command "$ErrorActionPreference='SilentlyContinue'; $paths='C:\Users\saisu\AppData\Local\Temp','C:\Windows\Temp','C:\Users\saisu\AppData\Local\CrashDumps','C:\Users\saisu\AppData\Local\UnrealBuildTool','C:\Users\saisu\AppData\Local\UnrealEngine','C:\Users\saisu\AppData\Local\UnrealEngineLauncher','C:\Users\saisu\AppData\Roaming\Unreal Engine'; foreach($p in $paths){if(Test-Path $p){$s=(Get-ChildItem $p -Recurse -File -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum;Write-Host (\"{0,-70} {1,8:N2} GB\" -f $p, ($s/1GB))}else{Write-Host (\"{0,-70} (missing)\" -f $p)}}" >> "%OUT%" 2>&1
echo. >> "%OUT%"

echo === Free space === >> "%OUT%"
powershell -NoProfile -Command "Get-PSDrive C | Select Used,Free,@{N='FreeGB';E={[math]::Round($_.Free/1GB,2)}},@{N='UsedGB';E={[math]::Round($_.Used/1GB,2)}} | Format-Table -AutoSize | Out-String -Width 4096" >> "%OUT%" 2>&1
echo. >> "%OUT%"

echo === Done === >> "%OUT%"
type "%OUT%"
endlocal
