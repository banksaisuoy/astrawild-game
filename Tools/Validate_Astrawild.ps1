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
    "Content/Astrawild/Data/Source/DT_Biomes.csv",
    "Content/Astrawild/Data/Source/DT_BossEncounters.csv",
    "Content/Astrawild/Data/Source/DT_BossAttacks.csv",
    "Content/Astrawild/Data/Source/DT_FoliageRules.csv",
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
    "Content/Astrawild/Data/Source/DT_MechaFrames.csv",
    "Content/Astrawild/Data/Source/DT_MechaWeapons.csv",
    "Content/Astrawild/Data/Source/DT_CyberneticEvolutions.csv",
    "Content/Astrawild/Data/Source/DT_MechaAnimationProfiles.csv",
    "Content/Astrawild/Data/Source/DT_MechaVFX.csv",
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
    "Source/AstrawildCore/Public/World/AstrawildBossAIController.h",
    "Source/AstrawildCore/Public/World/AstrawildLandscapeMaterialComponent.h",
    "Source/AstrawildCore/Public/World/AstrawildAudioSubsystem.h",
    "Source/AstrawildCore/Public/Components/AstrawildMechaComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildMechaComponent.cpp",
    "Source/AstrawildCore/Public/Data/AstrawildMechaData.h",
    "Source/AstrawildCore/Public/Data/AstrawildMechaAnimationData.h",
    "Source/AstrawildCore/Public/Data/AstrawildMechaVFXData.h",
    "Source/AstrawildCore/Public/Components/AstrawildMechaVFXComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildMechaVFXComponent.cpp",
    "Source/AstrawildCore/Public/UI/AstrawildCockpitWidget.h",
    "Source/AstrawildCore/Private/UI/AstrawildCockpitWidget.cpp",
    "Source/AstrawildCore/Private/World/AstrawildBossAIController.cpp",
    "Source/AstrawildCore/Private/World/AstrawildLandscapeMaterialComponent.cpp",
    "Source/AstrawildCore/Private/World/AstrawildAudioSubsystem.cpp",
    "Source/AstrawildCore/Public/World/AstrawildWorldClockSubsystem.h",
    "Source/AstrawildCore/Private/World/AstrawildWorldClockSubsystem.cpp",
    "Config/DefaultEngine.ini",
    "Config/DefaultScalability.ini",
    "Config/DefaultGameplayTags.ini",
    "Scripts/import_all_datatables.py",
    "Scripts/setup_project_assets.py",
    "Scripts/validate_content_contracts.py",
    "Scripts/validate_runtime_contracts.py",
    "Scripts/validate_editor_automation.py",
    "Scripts/validate_master_echodex.py",
    "Scripts/validate_generated_assets.py",
    "Scripts/validate_mecha_contracts.py",
    "Scripts/validate_vertical_slice_guards.py",
    "Scripts\validate_generated_headers.py",
    "Scripts\generate_character_and_map_assets.py",
    "Scripts\validate_character_map_assets.py",
    "Scripts\generate_extended_audio_pack.py",
    "Scripts\validate_audio_pack.py",
    "Scripts\validate_importer_coverage.py",
    "Docs/M2_EVOLUTION_HANDOFF.md",
    "Docs/VISUAL_AND_WORLD_POLISH_HANDOFF.md",
    "Docs/P5_ASTRA_EXOSUIT_SYSTEM_SPEC.md",
    "Docs/VERTICAL_SLICE_MAP_20MIN_SPEC.md",
    "Docs/ASSET_PRODUCTION_BIBLE.md",
    "Docs/UNREAL_EDITOR_AUTOMATION_HANDOFF.md",
    "Content/Astrawild/Meshes/Echoes/ASTRAWILD_Echoes.mtl",
    "Content/Astrawild/Meshes/Echoes/ASTRAWILD_EchoSource_Manifest.json",
    "Content/Astrawild/Meshes/Characters/ASTRAWILD_Characters.mtl",
    "Content/Astrawild/Meshes/Characters/SM_Player_AstralSurveyor_Source.obj",
    "Content/Astrawild/Meshes/Characters/SM_Alpha_Solarix_Source.obj",
    "Content/Astrawild/Meshes/MapKit/ASTRAWILD_MapKit.mtl",
    "Content/Astrawild/Meshes/MapKit/ASTRAWILD_MapKit_Manifest.json",
    "Content/Astrawild/Meshes/MapKit/SM_DawnSpire_Kit_Source.obj",
    "Content/Astrawild/Meshes/MapKit/SM_ResourceGrove_Kit_Source.obj",
    "Content/Astrawild/Meshes/MapKit/SM_RestSanctuary_Kit_Source.obj",
    "Content/Astrawild/Meshes/MapKit/SM_DangerPit_Kit_Source.obj",
    "Content/Astrawild/Audio/ASTRAWILD_AudioPack_Manifest.json",
    "Content/Astrawild/Audio/Music/MUS_Astra_Exploration.mp3",
    "Content/Astrawild/Audio/Music/MUS_DangerPit_Encounter.mp3"
)
foreach ($relative in $required) {
    if (-not (Test-Path (Join-Path $ProjectRoot $relative))) { throw "Missing required path: $relative" }
}

$pythonValidators = @(
    "Scripts\validate_content_contracts.py",
    "Scripts\validate_runtime_contracts.py",
    "Scripts\validate_generated_headers.py",
    "Scripts\validate_editor_automation.py",
    "Scripts\validate_master_echodex.py",
    "Scripts\validate_generated_assets.py",
    "Scripts\validate_mecha_contracts.py",
    "Scripts\validate_vertical_slice_guards.py",
    "Scripts\validate_character_map_assets.py",
    "Scripts\validate_audio_pack.py",
    "Scripts\validate_importer_coverage.py"
)
if (Get-Command python -ErrorAction SilentlyContinue) {
    foreach ($validator in $pythonValidators) {
        & python (Join-Path $ProjectRoot $validator)
        if ($LASTEXITCODE -ne 0) { throw "Python validation failed: $validator (exit code $LASTEXITCODE)" }
    }
    & python -m py_compile (Join-Path $ProjectRoot "Scripts\import_all_datatables.py") (Join-Path $ProjectRoot "Scripts\setup_project_assets.py") (Join-Path $ProjectRoot "Scripts\generate_master_echodex_200.py") (Join-Path $ProjectRoot "Scripts\generate_game_audio.py") (Join-Path $ProjectRoot "Scripts\generate_3d_props.py") (Join-Path $ProjectRoot "Scripts\generate_ecosystem_behavior.py") (Join-Path $ProjectRoot "Scripts\generate_gameplay_tag_registry.py") (Join-Path $ProjectRoot "Scripts\generate_character_and_map_assets.py") (Join-Path $ProjectRoot "Scripts\generate_extended_audio_pack.py")
    if ($LASTEXITCODE -ne 0) { throw "Unreal Python script syntax validation failed" }
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
