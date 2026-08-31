Write-Host "========================================" -ForegroundColor Cyan
Write-Host " ASTRAWILD Automation Test Suite (QA Pipeline)" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan

$EditorCmdPath = "E:\Epic Games\UnrealEngine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
$ProjectPath = "E:\AstrawildGame\ASTRAWILD.uproject"
$OutputFile = "E:\AstrawildGame\Automation_Output.txt"

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
