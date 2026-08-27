[CmdletBinding()]
param(
    [string]$ProjectRoot = "",
    [switch]$TryUnreal,
    [switch]$Package,
    [string]$PackageDirectory = "",
    [string]$UnrealExecutable = ""
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
    "Content/Astrawild/Data/Source/DT_QuestObjectives.csv",
    "Content/Astrawild/Data/Source/DT_EchoDex.csv",
    "Content/Astrawild/Data/Source/DT_EchoTraits.csv",
    "Content/Astrawild/Data/Source/DT_BreedingGroups.csv",
    "Content/Astrawild/Data/Source/DT_MountProfiles.csv",
    "Content/Astrawild/Data/Source/DT_TechnologyNodes.csv",
    "Content/Astrawild/Data/Source/DT_Recipes.csv",
    "Content/Astrawild/Data/Source/DT_RangedWeapons.csv",
    "Content/Astrawild/Data/Source/DT_Dungeons.csv",
    "Content/Astrawild/Data/Source/DT_Evolutions.csv",
    "Content/Astrawild/Data/Source/DT_Weather.csv",
    "Source/AstrawildCore/Public/Components/AstrawildSanComponent.h",
    "Source/AstrawildCore/Public/Components/AstrawildColonyWorkComponent.h",
    "Source/AstrawildCore/Public/Components/AstrawildTechnologyComponent.h",
    "Source/AstrawildCore/Public/Components/AstrawildRangedCombatComponent.h",
    "Source/AstrawildCore/Public/World/AstrawildDungeonSubsystem.h",
    "Source/AstrawildCore/Public/UI/AstrawildMasterWidgets.h",
    "Source/AstrawildCore/Public/Components/AstrawildEvolutionComponent.h",
    "Source/AstrawildCore/Public/Data/AstrawildEvolutionData.h",
    "Source/AstrawildCore/Public/Data/AstrawildWeatherData.h",
    "Source/AstrawildCore/Public/World/AstrawildWeatherSubsystem.h",
    "Scripts/validate_runtime_contracts.py",
    "Scripts/validate_generated_headers.py",
    "Docs/M2_EVOLUTION_HANDOFF.md"
)
foreach ($relative in $required) {
    if (-not (Test-Path (Join-Path $ProjectRoot $relative))) { throw "Missing required path: $relative" }
}

$runtimeValidator = Join-Path $ProjectRoot "Scripts\validate_runtime_contracts.py"
if (Get-Command python -ErrorAction SilentlyContinue) {
    & python $runtimeValidator
    if ($LASTEXITCODE -ne 0) { throw "Runtime contract validation failed with exit code $LASTEXITCODE" }
}
else {
    Write-Warning "Python was not found; runtime cross-table validator was not executed."
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

if ($TryUnreal -or $Package) {
    $editorCandidates = @()
    if ($UnrealExecutable) { $editorCandidates += $UnrealExecutable }
    $editorCandidates += @(
        (Get-Command UnrealEditor-Cmd.exe -ErrorAction SilentlyContinue).Source,
        (Get-Command UnrealEditor.exe -ErrorAction SilentlyContinue).Source,
        "$env:ProgramFiles\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"
    )
    $editorCandidates = $editorCandidates | Where-Object { $_ -and (Test-Path $_) }

    if (-not $editorCandidates) {
        Write-Warning "UnrealEditor-Cmd.exe was not found; skipping command-line project check."
        exit 2
    }

    $editor = $editorCandidates[0]
    Write-Host "Unreal command-line executable: $editor"
    if ($TryUnreal) {
        & $editor $uproject -nullrhi -unattended -nop4 -nosplash -NoSound -run=CompileAllBlueprints -Quit
        if ($LASTEXITCODE -ne 0) { throw "Unreal command-line verification failed with exit code $LASTEXITCODE" }
        Write-Host "Blueprint compile command completed." -ForegroundColor Green
    }

    if ($Package) {
        $uatCandidates = @(
            "$env:ProgramFiles\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.bat",
            "$env:ProgramFiles\Epic Games\UE_5.8\Engine\Build\BatchFiles\RunUAT.ps1"
        ) | Where-Object { Test-Path $_ }
        if (-not $uatCandidates) {
            Write-Warning "RunUAT was not found; package step was not executed."
            exit 2
        }
        if ([string]::IsNullOrWhiteSpace($PackageDirectory)) {
            $PackageDirectory = Join-Path $ProjectRoot "Builds\WindowsDevelopment"
        }
        New-Item -ItemType Directory -Force -Path $PackageDirectory | Out-Null
        $uat = $uatCandidates[0]
        Write-Host "Packaging to: $PackageDirectory"
        & cmd.exe /c $uat BuildCookRun "-project=$uproject" -noP4 -platform=Win64 -clientconfig=Development -serverconfig=Development -build -cook -stage -pak -archive "-archivedirectory=$PackageDirectory"
        if ($LASTEXITCODE -ne 0) { throw "Unreal packaging failed with exit code $LASTEXITCODE" }
        Write-Host "Packaging command completed. Inspect the archive and BUILD_STATUS evidence before claiming a shippable build." -ForegroundColor Green
    }
}

Write-Host "ASTRAWILD verification completed." -ForegroundColor Green
