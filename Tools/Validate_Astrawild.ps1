[CmdletBinding()]
param(
    [string]$ProjectRoot = "",
    [switch]$TryUnreal
)

$ErrorActionPreference = "Stop"

if ([string]::IsNullOrWhiteSpace($ProjectRoot)) {
    if ($PSScriptRoot) {
        $ProjectRoot = (Get-Item (Join-Path $PSScriptRoot "..")).FullName
    } else {
        $ProjectRoot = (Get-Location).Path
    }
}

Write-Host "=== ASTRAWILD verification ===" -ForegroundColor Cyan
Write-Host "Root: $ProjectRoot"
Write-Host "Branch: $(git branch --show-current)"
Write-Host "Commit: $(git log -1 --format='%H')"

$uproject = Join-Path $ProjectRoot "ASTRAWILD.uproject"
if (-not (Test-Path $uproject)) { throw "ASTRAWILD.uproject not found" }

$required = @(
    "Source/AstrawildCore/Public/Characters/AstrawildCharacter.h",
    "Source/AstrawildCore/Public/Components/AstrawildQuestComponent.h",
    "Source/AstrawildCore/Public/Components/AstrawildSurvivalComponent.h",
    "Content/Astrawild/Data/Source/DT_Lore.csv",
    "Content/Astrawild/Data/Source/DT_Quests.csv",
    "Content/Astrawild/Data/Source/DT_QuestObjectives.csv"
)
foreach ($relative in $required) {
    if (-not (Test-Path (Join-Path $ProjectRoot $relative))) { throw "Missing required path: $relative" }
}

$binaryAssets = Get-ChildItem -Path (Join-Path $ProjectRoot "Content") -Recurse -File -Include *.uasset,*.umap -ErrorAction SilentlyContinue
Write-Host "Unreal binary assets: $($binaryAssets.Count)"
if ($binaryAssets.Count -eq 0) {
    Write-Warning "No .uasset/.umap files found. C++/CSV contracts exist, but Editor asset creation is still required."
}

$dirty = git status --short
if ($dirty) {
    Write-Warning "Working tree is dirty:"
    $dirty | Write-Host
}
else {
    Write-Host "Working tree: clean" -ForegroundColor Green
}

if ($TryUnreal) {
    $editorCandidates = @(
        (Get-Command UnrealEditor-Cmd.exe -ErrorAction SilentlyContinue).Source,
        (Get-Command UnrealEditor.exe -ErrorAction SilentlyContinue).Source,
        "$env:ProgramFiles\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
    ) | Where-Object { $_ -and (Test-Path $_) }

    if (-not $editorCandidates) {
        Write-Warning "UnrealEditor-Cmd.exe was not found; skipping command-line project check."
        exit 2
    }

    $editor = $editorCandidates[0]
    Write-Host "Unreal command-line executable: $editor"
    & $editor $uproject -nullrhi -unattended -nop4 -nosplash -NoSound -run=CompileAllBlueprints -Quit
    if ($LASTEXITCODE -ne 0) { throw "Unreal command-line verification failed with exit code $LASTEXITCODE" }
}

Write-Host "ASTRAWILD verification completed." -ForegroundColor Green
