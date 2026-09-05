Write-Host "========================================" -ForegroundColor Cyan
Write-Host " ASTRAWILD Packaging Pipeline (Win64)" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan

$RunUATPath = "E:\Epic Games\UnrealEngine\Engine\Build\BatchFiles\RunUAT.bat"
$ProjectPath = "E:\AstrawildGame\ASTRAWILD.uproject"
$ArchiveDir = "E:\Astrawild_Packaged"

$sw = [System.Diagnostics.Stopwatch]::StartNew()
& $RunUATPath -ScriptsForProject="$ProjectPath" BuildCookRun -nocompileeditor -installed -nop4 -project="$ProjectPath" -cook -stage -archive -archivedirectory="$ArchiveDir" -package -clientconfig=Development -targetplatform=Win64 -build -nocompile
$sw.Stop()

Write-Host ">>> Packaging Completed in $($sw.Elapsed.TotalSeconds.ToString("F2"))s <<<" -ForegroundColor Yellow
