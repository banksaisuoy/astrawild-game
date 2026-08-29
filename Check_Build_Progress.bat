@echo off
title ASTRAWILD Engine Build Progress Monitor
color 0A
mode con: cols=85 lines=20
cls
powershell -NoProfile -ExecutionPolicy Bypass -Command ^
    "$logFile = 'E:\Epic Games\UnrealEngine\Engine\Programs\UnrealBuildTool\Log.txt'; " ^
    "while($true) { " ^
    "    Clear-Host; " ^
    "    Write-Host '==========================================================================' -ForegroundColor Cyan; " ^
    "    Write-Host '             ASTRAWILD: REAL-TIME COMPILATION MONITOR                     ' -ForegroundColor Yellow; " ^
    "    Write-Host '==========================================================================' -ForegroundColor Cyan; " ^
    "    Write-Host ''; " ^
    "    if (Test-Path $logFile) { " ^
    "        $lastMatch = Get-Content $logFile | Select-String -Pattern '\[([0-9]+)/([0-9]+)\]' | Select-Object -Last 1; " ^
    "        if ($lastMatch -match '\[([0-9]+)/([0-9]+)\]\s*(.*)') { " ^
    "            $curr = [int]$matches[1]; $total = [int]$matches[2]; $task = $matches[3]; " ^
    "            $pct = [math]::Round(($curr / $total) * 100, 1); " ^
    "            $barLen = 35; $filled = [int]($barLen * ($curr / $total)); $empty = $barLen - $filled; " ^
    "            $bar = ('[' + ('#' * $filled) + ('-' * $empty) + ']'); " ^
    "            Write-Host ('  Current Progress: ' + $curr + ' / ' + $total + ' Actions (' + $pct + '%)') -ForegroundColor Green; " ^
    "            Write-Host ('  ' + $bar) -ForegroundColor White; " ^
    "            Write-Host ''; " ^
    "            Write-Host ('  Active Task: ' + $task) -ForegroundColor Gray; " ^
    "        } else { Write-Host '  Parsing build log...' -ForegroundColor Gray; } " ^
    "    } else { Write-Host '  Waiting for build log...' -ForegroundColor Red; } " ^
    "    Write-Host ''; " ^
    "    Write-Host '==========================================================================' -ForegroundColor Cyan; " ^
    "    Write-Host '  (Auto-refreshes every 3 seconds. Press Ctrl+C to close)' -ForegroundColor DarkGray; " ^
    "    Start-Sleep -Seconds 3; " ^
    "} "