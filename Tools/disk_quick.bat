@echo off
setlocal
set OUT=%~dp0disk_quick.log
echo === Disk Quick Scan %DATE% %TIME% === > "%OUT%"

powershell -NoProfile -Command "$ErrorActionPreference='SilentlyContinue'; $paths=@('C:\Users\saisu\AppData\Local\Temp','C:\Windows\Temp','C:\Users\saisu\AppData\Local\CrashDumps','C:\Users\saisu\AppData\Local\UnrealBuildTool','C:\Users\saisu\AppData\Local\UnrealEngine','C:\Users\saisu\AppData\Local\UnrealEngineLauncher','C:\Users\saisu\AppData\Roaming\Unreal Engine','C:\Users\saisu\AppData\Local\Crashpad','C:\Users\saisu\AppData\Local\Google\Chrome\User Data\Default\Cache','C:\Users\saisu\AppData\Local\Google\Chrome\User Data\Default\Code Cache','C:\Users\saisu\AppData\Local\pip\cache','C:\Users\saisu\.cache','C:\Users\saisu\.npm','C:\Users\saisu\.nuget','C:\Users\saisu\Downloads'); foreach($p in $paths){if(Test-Path $p){$s=(Get-ChildItem $p -Recurse -File -ErrorAction SilentlyContinue | Measure-Object Length -Sum).Sum;Write-Host (\"{0,-80} {1,8:N2} GB\" -f $p, ($s/1GB))}else{Write-Host (\"{0,-80} (missing)\" -f $p)}}" >> "%OUT%" 2>&1

powershell -NoProfile -Command "Get-PSDrive C | Select Used,Free,@{N='FreeGB';E={[math]::Round($_.Free/1GB,2)}} | Format-Table -AutoSize | Out-String" >> "%OUT%" 2>&1

type "%OUT%"
endlocal
