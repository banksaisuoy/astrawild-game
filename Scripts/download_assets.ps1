# ASTRAWILD — Free Asset Acquisition Pipeline (PowerShell wrapper)
# Runs Scripts/download_assets.py with the same defaults.
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File Scripts\download_assets.ps1
#   powershell -ExecutionPolicy Bypass -File Scripts\download_assets.ps1 -DryRun
#   powershell -ExecutionPolicy Bypass -File Scripts\download_assets.ps1 -Packs "impact-sounds,nature-kit"
#
# The download cache defaults to <repo>\..\asset_download_cache (OUTSIDE the
# repository — never committed). Requires Python 3.9+; ffmpeg/ffprobe are only
# needed for the OGG -> WAV conversion step (records are marked BLOCKED and
# can be re-run later where ffmpeg is available).

param(
    [string]$RepoRoot = "",
    [string]$CacheDir = "",
    [string]$Packs = "",
    [switch]$DryRun
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $RepoRoot) { $RepoRoot = Split-Path -Parent $scriptDir }

# Locate a Python 3 interpreter (python, then the Windows py launcher)
$py = Get-Command python -ErrorAction SilentlyContinue
$pyCmd = @()
if ($py) {
    $pyCmd = @($py.Source)
} else {
    $pyl = Get-Command py -ErrorAction SilentlyContinue
    if ($pyl) {
        $pyCmd = @($pyl.Source, "-3")
    } else {
        Write-Error "Python 3 not found. Install Python 3.9+ and ensure 'python' or 'py' is on PATH."
        exit 1
    }
}

$args = @("$RepoRoot\Scripts\download_assets.py", "--repo-root", $RepoRoot)
if ($CacheDir) { $args += @("--cache-dir", $CacheDir) }
if ($Packs)    { $args += @("--packs", $Packs) }
if ($DryRun)   { $args += "--dry-run" }

Write-Host "[assets] running: $($pyCmd -join ' ') $($args -join ' ')"
& $pyCmd[0] @($pyCmd[1..($pyCmd.Count - 1)] | Where-Object { $_ }) @args
exit $LASTEXITCODE
