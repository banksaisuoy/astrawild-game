# v9.6 env-adaptive paths (FMP-1): UE_ROOT / ASTRAWILD_UPROJECT override the
# legacy layout for non-default installs (fresh machines); with no env vars
# set the legacy E:\ values are preserved exactly. DOTNET_ROOT is only
# forced when the legacy E:\dotnet exists.
$EngineRoot = if ($env:UE_ROOT) { $env:UE_ROOT } else { "E:\Epic Games\UnrealEngine" }
$ProjectPath = if ($env:ASTRAWILD_UPROJECT) { $env:ASTRAWILD_UPROJECT } else { "E:\AstrawildGame\ASTRAWILD.uproject" }
if (-not $env:DOTNET_ROOT -and (Test-Path "E:\dotnet")) {
    $env:DOTNET_ROOT = "E:\dotnet"
    $env:PATH = "E:\dotnet;" + $env:PATH
}
Write-Host "========================================" -ForegroundColor Cyan
Write-Host " ASTRAWILD UE5 Build Pipeline" -ForegroundColor Green
Write-Host " Target: ASTRAWILDEditor Win64 Development" -ForegroundColor Yellow
Write-Host "========================================" -ForegroundColor Cyan
Write-Host " Engine root : $EngineRoot"
Write-Host " Project     : $ProjectPath"

$UbtPath = Join-Path $EngineRoot "Engine\Binaries\DotNET\UnrealBuildTool\UnrealBuildTool.exe"

$sw = [System.Diagnostics.Stopwatch]::StartNew()
& $UbtPath ASTRAWILDEditor Win64 Development "-Project=$ProjectPath" -WaitMutex -FromMsBuild -NoUBA
$sw.Stop()

if ($LASTEXITCODE -eq 0) {
    Write-Host ">>> BUILD SUCCESSFUL (Took $($sw.Elapsed.TotalSeconds.ToString("F2"))s) <<<" -ForegroundColor Green
} else {
    Write-Host ">>> BUILD FAILED (Exit Code: $LASTEXITCODE) <<<" -ForegroundColor Red
}
exit $LASTEXITCODE
