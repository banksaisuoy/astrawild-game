# v9.6 env-adaptive paths (FMP-1): UE_ROOT / ASTRAWILD_UPROJECT /
# ASTRAWILD_ARCHIVE override the legacy layout for non-default installs
# (fresh machines); with no env vars set the legacy E:\ values are
# preserved exactly.
$EngineRoot = if ($env:UE_ROOT) { $env:UE_ROOT } else { "E:\Epic Games\UnrealEngine" }
$ProjectPath = if ($env:ASTRAWILD_UPROJECT) { $env:ASTRAWILD_UPROJECT } else { "E:\AstrawildGame\ASTRAWILD.uproject" }
$ArchiveDir = if ($env:ASTRAWILD_ARCHIVE) { $env:ASTRAWILD_ARCHIVE } else { "E:\Astrawild_Packaged" }
Write-Host "========================================" -ForegroundColor Cyan
Write-Host " ASTRAWILD Packaging Pipeline (Win64)" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host " Engine root : $EngineRoot"
Write-Host " Project     : $ProjectPath"
Write-Host " Archive dir : $ArchiveDir"

$RunUATPath = Join-Path $EngineRoot "Engine\Build\BatchFiles\RunUAT.bat"

$sw = [System.Diagnostics.Stopwatch]::StartNew()
& $RunUATPath -ScriptsForProject="$ProjectPath" BuildCookRun -nocompileeditor -installed -nop4 -project="$ProjectPath" -cook -stage -archive -archivedirectory="$ArchiveDir" -package -clientconfig=Development -targetplatform=Win64 -build -nocompile
$sw.Stop()

Write-Host ">>> Packaging Completed in $($sw.Elapsed.TotalSeconds.ToString("F2"))s <<<" -ForegroundColor Yellow
