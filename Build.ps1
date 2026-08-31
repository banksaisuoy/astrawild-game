$env:DOTNET_ROOT = "E:\dotnet"
$env:PATH = "E:\dotnet;" + $env:PATH
Write-Host "========================================" -ForegroundColor Cyan
Write-Host " ASTRAWILD UE5 Build Pipeline" -ForegroundColor Green
Write-Host " Target: ASTRAWILDEditor Win64 Development" -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Cyan

$UbtPath = "E:\Epic Games\UnrealEngine\Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"
$ProjectPath = "E:\AstrawildGame\ASTRAWILD.uproject"

$sw = [System.Diagnostics.Stopwatch]::StartNew()
& $UbtPath ASTRAWILDEditor Win64 Development "-Project=$ProjectPath" -WaitMutex -FromMsBuild -NoUBA
$sw.Stop()

if ($LASTEXITCODE -eq 0) {
    Write-Host ">>> BUILD SUCCESSFUL (Took $($sw.Elapsed.TotalSeconds.ToString("F2"))s) <<<" -ForegroundColor Green
} else {
    Write-Host ">>> BUILD FAILED (Exit Code: $LASTEXITCODE) <<<" -ForegroundColor Red
}
exit $LASTEXITCODE
