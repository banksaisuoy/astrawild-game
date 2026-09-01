$ErrorActionPreference = "Stop"
$ProjectPath = "E:\AstrawildGame\ASTRAWILD.uproject"
$EditorExe = "E:\Epic Games\UnrealEngine\Engine\Binaries\Win64\UnrealEditor.exe"
$LogPath = "E:\AstrawildGame\Saved\Logs\Runtime_Verification.log"

Write-Host "========================================"
Write-Host " ASTRAWILD Runtime Verification & Benchmark"
Write-Host "========================================"

if (Test-Path $LogPath) {
    Remove-Item $LogPath -Force
}

$Args = @(
    "`"$ProjectPath`"",
    "-game",
    "-windowed",
    "-ResX=1920",
    "-ResY=1080",
    "-log",
    "-abslog=`"$LogPath`"",
    "-ExecCmds=`"stat fps; stat unit; stat gpu; stat game; stat anim`"",
    "-unattended"
)

Write-Host "Starting Unreal Game Mode session..."
$proc = Start-Process -FilePath $EditorExe -ArgumentList $Args -PassThru

$RunDurationSeconds = 25
Write-Host "Running game session for $RunDurationSeconds seconds..."
for ($i = 0; $i -lt $RunDurationSeconds; $i += 5) {
    Start-Sleep -Seconds 5
    if ($proc.HasExited) {
        Write-Host "Process exited early with code $($proc.ExitCode)"
        break
    }
    Write-Host "  ... session running ($($i + 5)s / ${RunDurationSeconds}s)"
}

if (-not $proc.HasExited) {
    Write-Host "Stopping session cleanly..."
    Stop-Process -Id $proc.Id -Force
    Start-Sleep -Seconds 2
}

Write-Host "`nParsing Log Output: $LogPath"
if (Test-Path $LogPath) {
    $LogContent = Get-Content $LogPath
    Write-Host "Total Log Lines: $($LogContent.Count)"
    
    $KeyLogPatterns = @(
        "LogAstrawild",
        "AstrawildWorldBootstrapper",
        "Bestiary",
        "ZoneSubsystem",
        "PowerSubsystem",
        "ResearchSubsystem",
        "JournalSubsystem",
        "Dungeon",
        "Village",
        "Skiff",
        "SaveSubsystem",
        "WeatherSubsystem",
        "Error",
        "Warning"
    )
    
    foreach ($pat in $KeyLogPatterns) {
        $matches = $LogContent | Select-String -Pattern $pat
        Write-Host "Pattern [$pat]: $($matches.Count) entries found"
    }
} else {
    Write-Warning "Log file not found at $LogPath"
}
