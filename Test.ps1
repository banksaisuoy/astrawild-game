# v9.6 env-adaptive paths (FMP-1): UE_ROOT / ASTRAWILD_UPROJECT /
# ASTRAWILD_AUTOMATION_OUTPUT override the legacy layout for non-default
# installs (fresh machines); with no env vars set the legacy E:\ values are
# preserved exactly.
$EngineRoot = if ($env:UE_ROOT) { $env:UE_ROOT } else { "E:\Epic Games\UnrealEngine" }
$ProjectPath = if ($env:ASTRAWILD_UPROJECT) { $env:ASTRAWILD_UPROJECT } else { "E:\AstrawildGame\ASTRAWILD.uproject" }
$OutputFile = if ($env:ASTRAWILD_AUTOMATION_OUTPUT) { $env:ASTRAWILD_AUTOMATION_OUTPUT } else { Join-Path (Split-Path $ProjectPath -Parent) "Automation_Output.txt" }
Write-Host "========================================" -ForegroundColor Cyan
Write-Host " ASTRAWILD Automation Test Suite (QA Pipeline)" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host " Editor      : $EngineRoot"
Write-Host " Project     : $ProjectPath"
Write-Host " Output file : $OutputFile"

$EditorCmdPath = Join-Path $EngineRoot "Engine\Binaries\Win64\UnrealEditor-Cmd.exe"

$sw = [System.Diagnostics.Stopwatch]::StartNew()
& $EditorCmdPath $ProjectPath -ExecCmds="Automation RunTests Astrawild; Quit" -nullrhi -unattended -nopause -testexit="Automation Test Queue Empty" -stdout -NoUBA | Out-File -FilePath $OutputFile -Encoding utf8
$sw.Stop()

$success = (Select-String -Path $OutputFile -Pattern "Result=\{Success\}").Count
$fail = (Select-String -Path $OutputFile -Pattern "Result=\{Fail\}").Count

Write-Host ">>> Test Execution Completed in $($sw.Elapsed.TotalSeconds.ToString("F2"))s <<<" -ForegroundColor Yellow
if ($fail -eq 0 -and $success -gt 0) {
    Write-Host ">>> ALL TESTS PASSED ($success / $success PASS - 100% GREEN) <<<" -ForegroundColor Green
} else {
    Write-Host ">>> SOME TESTS FAILED ($success Passed, $fail Failed) <<<" -ForegroundColor Red
}
