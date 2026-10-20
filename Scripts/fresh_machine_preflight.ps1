# ============================================================================
#  ASTRAWILD — FRESH MACHINE PRE-FLIGHT (Windows PowerShell 5.1+ compatible)
#
#  Purpose: run this on a brand-new Windows machine BEFORE the engine
#  integration run. It checks every prerequisite the game needs and prints
#  a PASS/FAIL/WARN table with the exact fix reference for every gap.
#  Loop: run -> fix what is FAIL (see Docs/ASTRAWILD_FRESH_MACHINE_PLAYBOOK.md)
#  -> run again until every REQUIRED row is PASS.
#
#  Usage:
#    powershell -ExecutionPolicy Bypass -File Scripts\fresh_machine_preflight.ps1
#    powershell -ExecutionPolicy Bypass -File Scripts\fresh_machine_preflight.ps1 -RepoRoot C:\src\astrawild-game
#
#  Exit codes: 0 = every REQUIRED check passed; 1 = at least one REQUIRED
#  check failed (fix per the playbook, then re-run).
#
#  Task lineage: FMP-1 (v9.6 Fresh-Machine Playbook pack, 2026-09-06).
#  Companion docs: Docs/ASTRAWILD_FRESH_MACHINE_PLAYBOOK.md (the make-ready
#  instructions), Docs/ASTRAWILD_FRESH_MACHINE_CHECKLIST.json (the tick-off
#  list). This script is read-only: it never installs, moves, or edits
#  anything.
# ============================================================================

param(
    [string]$RepoRoot = "",
    [string]$EngineRoot = ""
)

$ErrorActionPreference = 'Continue'
$script:RequiredFailed = 0

function Write-Section([string]$Title) {
    Write-Host ""
    Write-Host "=== $Title ===" -ForegroundColor Cyan
}

function Write-Result([string]$Name, [string]$State, [string]$Detail, [string]$Fix) {
    # State: PASS | FAIL | WARN | INFO
    $tag = ""
    $color = 'Gray'
    if ($State -eq 'PASS') { $tag = ' [PASS]'; $color = 'Green' }
    elseif ($State -eq 'FAIL') { $tag = ' [FAIL]'; $color = 'Red'; $script:RequiredFailed++ }
    elseif ($State -eq 'WARN') { $tag = ' [WARN]'; $color = 'Yellow' }
    else { $tag = ' [INFO]'; $color = 'DarkCyan' }
    Write-Host ("{0,-38}{1}" -f $Name, $tag) -ForegroundColor $color
    if ($Detail) { Write-Host ("{0,40}{1}" -f '', $Detail) -ForegroundColor DarkGray }
    if (($State -eq 'FAIL' -or $State -eq 'WARN') -and $Fix) {
        Write-Host ("{0,40}fix: {1}" -f '', $Fix) -ForegroundColor DarkYellow
    }
}

function Get-FirstCommand([string[]]$Names) {
    foreach ($n in $Names) {
        $cmd = Get-Command $n -ErrorAction SilentlyContinue
        if ($cmd) { return $cmd }
    }
    return $null
}

Write-Host "================================================================" -ForegroundColor Magenta
Write-Host " ASTRAWILD FRESH MACHINE PRE-FLIGHT" -ForegroundColor Magenta
Write-Host " expect: Windows 10/11 x64, UE 5.8.2, VS2022 (C++ game dev)," -ForegroundColor DarkGray
Write-Host " Git+LFS, Python 3.9+, 120+ GB free, 16+ GB RAM" -ForegroundColor DarkGray
Write-Host "================================================================" -ForegroundColor Magenta

# ---------------------------------------------------------------- P0: OS ----
Write-Section "P0 - Operating system / hardware"
$os = [System.Environment]::OSVersion
$osName = (Get-CimInstance Win32_OperatingSystem).Caption
$osOk = ($os.Version.Major -ge 10)
if ($os.Platform -ne 'Win32NT') { $osOk = $false }
Write-Result "OS" $(if ($osOk) { 'PASS' } else { 'FAIL' }) "$osName (build $($os.Version))" "playbook P0.1 - Windows 10 1909+ / Windows 11 x64 required"

$cs = Get-CimInstance Win32_ComputerSystem
$ramGb = [math]::Round($cs.TotalPhysicalMemory / 1GB, 1)
if ($ramGb -ge 31.5) { $ramState = 'PASS' }
elseif ($ramGb -ge 15.5) { $ramState = 'WARN' }
else { $ramState = 'FAIL' }
Write-Result "RAM" $ramState "$ramGb GB (32 recommended, 16 minimum)" "playbook P0.2 - UE 5.8 editor + builds need 16 GB minimum"

$gpus = Get-CimInstance Win32_VideoController | Where-Object { $_.Name -notmatch 'Basic Render|Microsoft' }
if ($gpus) {
    $gpuNames = ($gpus | ForEach-Object { $_.Name }) -join ' | '
    $vramBest = 0
    foreach ($g in $gpus) {
        # AdapterRAM is uint32-capped (~4 GB) - treat as a lower bound only
        if ($g.AdapterRAM -and $g.AdapterRAM -gt $vramBest) { $vramBest = $g.AdapterRAM }
    }
    $vramGb = [math]::Round($vramBest / 1GB, 1)
    $gpuState = 'PASS'
    if ($ramGb -lt 7.5) { $gpuState = 'WARN' }
    Write-Result "GPU (DirectX 12 capable)" $gpuState "$gpuNames (reported VRAM lower bound: $vramGb GB)" "playbook P0.3 - any modern DX12 GPU with 4+ GB VRAM; reference machine was a GTX 1660 Ti"
} else {
    Write-Result "GPU (DirectX 12 capable)" 'FAIL' "no discrete/active video controller found" "playbook P0.3"
}

$drives = Get-CimInstance Win32_LogicalDisk -Filter "DriveType=3"
$minFreeGb = 120
foreach ($d in $drives) {
    $freeGb = [math]::Round($d.FreeSpace / 1GB, 1)
    $totalGb = [math]::Round($d.Size / 1GB, 1)
    $line = "$($d.DeviceID) $freeGb GB free of $totalGb GB"
    if ($freeGb -ge 250) { Write-Result "Disk $($d.DeviceID)" 'PASS' $line }
    elseif ($freeGb -ge $minFreeGb) { Write-Result "Disk $($d.DeviceID)" 'WARN' $line "playbook P0.4 - 250 GB recommended (UE ~60 + VS ~35 + repo ~2 + build ~40 + packaged ~10)" }
    else { Write-Result "Disk $($d.DeviceID)" 'INFO' $line "not required unless you install UE/VS/repo on this drive" }
}
$sysDrive = $drives | Where-Object { $_.DeviceID -eq "$($env:SystemDrive)\" }
if ($sysDrive) {
    $sysFree = [math]::Round($sysDrive.FreeSpace / 1GB, 1)
    if ($sysFree -lt 30) {
        Write-Result "System drive headroom" 'WARN' "$sysFree GB free on $env:SystemDrive (VS + Epic Launcher install here by default)" "playbook P0.4 - clean up or relocate installs"
    }
}

$isAdmin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
Write-Result "Admin rights (for installs)" $(if ($isAdmin) { 'PASS' } else { 'WARN' }) $(if ($isAdmin) { "elevated" } else { "NOT elevated - installs (VS/UE) will prompt or fail" }) "playbook P0.5 - open an elevated PowerShell when installing"

# ------------------------------------------------------- P1: base tools ----
Write-Section "P1 - Base tooling (git / git-lfs / python)"

$git = Get-Command git -ErrorAction SilentlyContinue
if ($git) {
    $gitVer = (git --version) 2>$null
    Write-Result "git" 'PASS' $gitVer
} else {
    Write-Result "git" 'FAIL' "git not on PATH" "playbook P1.1 - winget install --id Git.Git -e --source winget (includes Git LFS + Git Bash)"
}

if ($git) {
    $lfsVer = (git lfs version) 2>$null
    if ($LASTEXITCODE -eq 0 -and $lfsVer) {
        Write-Result "git-lfs" 'PASS' "$lfsVer"
    } else {
        Write-Result "git-lfs" 'FAIL' "git is present but Git LFS is not (repo needs LFS for 586 binary objects)" "playbook P1.2 - reinstall Git for Windows with LFS, or: git lfs install after adding LFS"
    }
}

$py = Get-FirstCommand @('python', 'py')
if ($py) {
    if ($py.Name -eq 'py') { $pyVer = (py -3 --version) 2>$null } else { $pyVer = (python --version) 2>$null }
    if ($pyVer -match '3\.(\d+)\.') {
        $minor = [int]$Matches[1]
        if ($minor -ge 9) { Write-Result "python 3" 'PASS' "$pyVer at $($py.Source)" }
        else { Write-Result "python 3" 'FAIL' "$pyVer - need 3.9+ (validators use modern syntax)" "playbook P1.3 - winget install --id Python.Python.3.12 -e" }
    } else {
        Write-Result "python 3" 'WARN' "python launcher found but version unreadable ($pyVer)" "playbook P1.3 - verify python --version in a fresh shell"
    }
} else {
    Write-Result "python 3" 'FAIL' "python not on PATH" "playbook P1.3 - winget install --id Python.Python.3.12 -e (tick 'Add to PATH')"
}

$winget = Get-Command winget -ErrorAction SilentlyContinue
Write-Result "winget (installer helper)" $(if ($winget) { 'PASS' } else { 'INFO' }) $(if ($winget) { "available" } else { "not available - use the direct installer links in playbook P1.x" })

# ------------------------------------------------- P2: Visual Studio 2022 --
Write-Section "P2 - Visual Studio 2022 (C++ game development)"

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsPath = ""
$msvcOk = $false
if (Test-Path $vswhere) {
    $vsPath = (& $vswhere -products * -requires Microsoft.VisualStudio.Workload.NativeGame -property installationPath) 2>$null
    if ($vsPath) {
        $vsVer = (& $vswhere -products * -requires Microsoft.VisualStudio.Workload.NativeGame -property displayName) 2>$null
        Write-Result "VS2022 'Game development with C++'" 'PASS' "$vsVer ($vsPath)"
        $msvc = (& $vswhere -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath) 2>$null
        if ($msvc) { $msvcOk = $true; Write-Result "MSVC v143 toolset" 'PASS' "Microsoft.VisualStudio.Component.VC.Tools.x86.x64 present" }
        else { Write-Result "MSVC v143 toolset" 'FAIL' "workload present but the v143 x64/x86 build tools component is missing" "playbook P2.1 - VS Installer -> Modify -> 'Game development with C++' -> ensure 'MSVC v143 - VS 2022 C++ x64/x86 build tools'" }
    } else {
        Write-Result "VS2022 'Game development with C++'" 'FAIL' "VS installed but the NativeGame workload is NOT" "playbook P2.1 - winget install --id Microsoft.VisualStudio.2022.Community -e --override `"--quiet --wait --add Microsoft.VisualStudio.Workload.NativeGame --includeRecommended`""
    }
} else {
    Write-Result "VS2022 'Game development with C++'" 'FAIL' "Visual Studio 2022 not detected (vswhere missing)" "playbook P2.1 - install VS2022 Community + 'Game development with C++' workload"
}

# --------------------------------------------------- P3: Unreal Engine ----
Write-Section "P3 - Unreal Engine 5.8 (project EngineAssociation = 5.8)"

$ueCandidates = @()
if ($EngineRoot) { $ueCandidates += $EngineRoot }
if ($env:UE_ROOT) { $ueCandidates += $env:UE_ROOT }
$standardRoots = @('C:\Program Files\Epic Games', 'E:\Epic Games')
foreach ($root in $standardRoots) {
    foreach ($v in @('5.8', '5.7', '5.6', '5.5', '5.4', '5.3')) {
        $ueCandidates += (Join-Path $root "UE_$v")
    }
}
$ueCandidates += 'E:\Epic Games\UnrealEngine'
$ueFound = $null
$ueFoundVersion = ''
$ueFoundRoot = ''
foreach ($p in $ueCandidates) {
    $editor = Join-Path $p 'Engine\Binaries\Win64\UnrealEditor.exe'
    if (Test-Path $editor) {
        if (-not $ueFound) {
            $ueFound = $editor
            $ueFoundRoot = $p
            if ($p -match 'UE_(\d\.\d)') { $ueFoundVersion = $Matches[1] }
        } else {
            Write-Result "UE install (extra copy)" 'INFO' $p ""
        }
    }
}
if ($ueFound) {
    if ($ueFoundVersion -eq '5.8') {
        Write-Result "Unreal Engine 5.8" 'PASS' "$ueFound (root: $ueFoundRoot)"
    } else {
        Write-Result "Unreal Engine 5.8" 'WARN' "found UE $ueFoundVersion at $ueFoundRoot - project canon is UE 5.8.2 (EngineAssociation 5.8); Setup_And_Play.bat can use 5.3+ but build/test evidence should come from 5.8" "playbook P3.1 - install UE 5.8.2 via the Epic Games Launcher, or set UE_ROOT to the 5.8 install"
    }
    $ueCmd = Join-Path (Split-Path $ueFound) 'UnrealEditor-Cmd.exe'
    if (Test-Path $ueCmd) { Write-Result "UnrealEditor-Cmd.exe" 'PASS' $ueCmd }
} else {
    Write-Result "Unreal Engine 5.8" 'FAIL' "no UnrealEditor.exe found (checked -EngineRoot, UE_ROOT, C:\ and E:\ Epic Games paths)" "playbook P3.1 - Epic Games Launcher (free account) -> Library -> Unreal Engine -> install 5.8.2; then set UE_ROOT=<install root> and re-run"
}

$epic = Get-ChildItem 'C:\Program Files\Epic Games\Launcher\Portal\Binaries\Win64\EpicGamesLauncher.exe' -ErrorAction SilentlyContinue
Write-Result "Epic Games Launcher" $(if ($epic) { 'PASS' } else { 'INFO' }) $(if ($epic) { "present" } else { "not found (required to install UE 5.8.2 unless you build from source)" })

# --------------------------------------------------------- P4: repo state --
Write-Section "P4 - ASTRAWILD repository (informational until you clone)"

$repoRoots = @()
if ($RepoRoot) { $repoRoots += $RepoRoot }
if ($env:ASTRAWILD_REPO) { $repoRoots += $env:ASTRAWILD_REPO }
$repoRoots += (Join-Path (Split-Path $PSScriptRoot -Parent) '')
$repoRoots += (Get-Location).Path
$repo = $null
foreach ($r in $repoRoots) {
    if ($r -and (Test-Path (Join-Path $r 'ASTRAWILD.uproject'))) { $repo = $r; break }
}
if ($repo) {
    Write-Result "Repo located" 'INFO' $repo
    Push-Location $repo
    $branch = (git rev-parse --abbrev-ref HEAD) 2>$null
    if ($branch -eq 'final-completion') { Write-Result "Branch" 'PASS' "final-completion (the integration branch)" }
    else { Write-Result "Branch" 'WARN' "on '$branch' - the engine run must execute on final-completion" "playbook P4.2 - git checkout final-completion && git pull origin final-completion" }
    $head = (git rev-parse --short HEAD) 2>$null
    Write-Result "HEAD" 'INFO' $head
    $dirty = (git status --porcelain) 2>$null
    if ($dirty) { Write-Result "Working tree" 'WARN' "dirty - commit or stash before the engine run" "playbook P4.5" }
    else { Write-Result "Working tree" 'PASS' 'clean' }
    $lfsCount = (git lfs ls-files 2>$null | Measure-Object -Line).Lines
    if ($lfsCount -eq 586) { Write-Result "LFS pointers" 'PASS' "586/586 (the v9.6 expected count)" }
    elseif ($lfsCount -gt 0) { Write-Result "LFS pointers" 'WARN' "$lfsCount files tracked - expected 586 at this tip" "playbook P4.3 - git lfs install; git lfs pull origin (or you are on a different commit)" }
    else { Write-Result "LFS pointers" 'FAIL' "git lfs ls-files empty - LFS not installed/initialized or clone missed LFS" "playbook P4.3" }
    $contentCount = (Get-ChildItem 'Content' -Recurse -Include *.uasset,*.umap -ErrorAction SilentlyContinue | Measure-Object).Count
    if ($contentCount -eq 416) { Write-Result "Content packages on disk" 'PASS' "416 (matches the committed content census)" }
    elseif ($contentCount -gt 0) { Write-Result "Content packages on disk" 'INFO' "$contentCount .uasset/.umap files" }
    else { Write-Result "Content packages on disk" 'WARN' "0 - LFS pull missing (builds would read pointer files, not binaries)" "playbook P4.3 - git lfs pull origin" }
    $manifest = Get-Content 'ArtSource\manifest.json' -Raw -ErrorAction SilentlyContinue
    if ($manifest) { Write-Result "ArtSource manifest" 'PASS' 'ArtSource\manifest.json present (expect 189/189 present, 0 pending)' }
    else { Write-Result "ArtSource manifest" 'WARN' 'ArtSource\manifest.json not found in this repo root' "playbook P4.4 - re-checkout with LFS" }
    Pop-Location
} else {
    Write-Result "ASTRAWILD repo" 'INFO' "not located yet (clone it per playbook P4, then re-run with -RepoRoot)" "playbook P4.1 - git clone https://github.com/banksaisuoy/astrawild-game <dir>; git checkout final-completion; git lfs pull"
}

# ------------------------------------------------------------- summary ----
Write-Host ""
Write-Host "================================================================" -ForegroundColor Magenta
if ($script:RequiredFailed -eq 0) {
    Write-Host " RESULT: ALL REQUIRED CHECKS PASSED" -ForegroundColor Green
    Write-Host " Machine is ready for the engine integration run." -ForegroundColor Green
    Write-Host " Next: playbook P5 (generate + build) -> P6 (import) -> P7+ (PIE/tests)." -ForegroundColor DarkGray
    exit 0
} else {
    Write-Host " RESULT: $script:RequiredFailed REQUIRED CHECK(S) FAILED" -ForegroundColor Red
    Write-Host " Fix every [FAIL] row using Docs/ASTRAWILD_FRESH_MACHINE_PLAYBOOK.md," -ForegroundColor Yellow
    Write-Host " then re-run this script until all REQUIRED rows are PASS." -ForegroundColor Yellow
    exit 1
}
