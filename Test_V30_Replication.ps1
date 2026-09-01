$PackagedExe = "E:\Astrawild_Packaged\Windows\ASTRAWILD\Binaries\Win64\ASTRAWILD.exe"
$ServerLog = "E:\AstrawildGame\Saved\Logs\Server_Replication.log"
$ClientLog = "E:\AstrawildGame\Saved\Logs\Client_Replication.log"

if (Test-Path $ServerLog) { Remove-Item $ServerLog -Force }
if (Test-Path $ClientLog) { Remove-Item $ClientLog -Force }

Write-Host "=================================================="
Write-Host " V-30 REPLICATION & 2-CLIENT LISTEN SERVER TEST"
Write-Host "=================================================="

Write-Host "Step 1: Launching Listen Server on port 7777..."
$ServerArgs = @(
    "?listen",
    "-game",
    "-windowed",
    "-ResX=960",
    "-ResY=540",
    "-WinX=0",
    "-WinY=50",
    "-log",
    "-abslog=`"$ServerLog`""
)
$ServerProc = Start-Process -FilePath $PackagedExe -ArgumentList $ServerArgs -PassThru
Write-Host "  -> Server started (PID: $($ServerProc.Id))"

Start-Sleep -Seconds 5

Write-Host "`nStep 2: Launching Client connecting to 127.0.0.1..."
$ClientArgs = @(
    "127.0.0.1:7777",
    "-game",
    "-windowed",
    "-ResX=960",
    "-ResY=540",
    "-WinX=960",
    "-WinY=50",
    "-log",
    "-abslog=`"$ClientLog`""
)
$ClientProc = Start-Process -FilePath $PackagedExe -ArgumentList $ClientArgs -PassThru
Write-Host "  -> Client started (PID: $($ClientProc.Id))"

Write-Host "`nAllowing 2-client replication session to run for 15 seconds..."
for ($i = 0; $i -lt 15; $i += 5) {
    Start-Sleep -Seconds 5
    Write-Host "  ... session running ($($i + 5)s / 15s) [Server: $(!$ServerProc.HasExited), Client: $(!$ClientProc.HasExited)]"
}

Write-Host "`nStep 3: Stopping server and client cleanly..."
if (-not $ClientProc.HasExited) { Stop-Process -Id $ClientProc.Id -Force }
if (-not $ServerProc.HasExited) { Stop-Process -Id $ServerProc.Id -Force }
Start-Sleep -Seconds 2

Write-Host "`n=================================================="
Write-Host " REPLICATION LOG VERIFICATION"
Write-Host "=================================================="

if (Test-Path $ServerLog) {
    $SLog = Get-Content $ServerLog
    Write-Host "Server Log Lines: $($SLog.Count)"
    $ServerMatches = $SLog | Select-String -Pattern "LogNet|NotifyAcceptingConnection|Join request|Join succeeded|LogAstrawild|DOREPLIFETIME|PlayerController|Pawn"
    Write-Host "Found $($ServerMatches.Count) Server Network / Replication Events:"
    foreach ($m in $ServerMatches | Select-Object -First 15) {
        Write-Host "  [Server] $m"
    }
}

if (Test-Path $ClientLog) {
    $CLog = Get-Content $ClientLog
    Write-Host "`nClient Log Lines: $($CLog.Count)"
    $ClientMatches = $CLog | Select-String -Pattern "LogNet|Connected to server|Join request|Join succeeded|LogAstrawild|PlayerController|Pawn"
    Write-Host "Found $($ClientMatches.Count) Client Network / Replication Events:"
    foreach ($m in $ClientMatches | Select-Object -First 15) {
        Write-Host "  [Client] $m"
    }
}
