#!/usr/bin/env python3
"""Validate the operational contract between ASTRAWILD handoff docs and runners.

This is a repository-only check. It does not import Unreal and cannot prove that
UE 5.8 accepted a Python API call, compiled C++, imported an asset, ran PIE, or
produced a package.
"""
from __future__ import annotations

import ast
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
HANDOFF = ROOT / "Docs/UNREAL_EDITOR_AUTOMATION_HANDOFF.md"
IMPORTER = ROOT / "Scripts/import_all_datatables.py"
SCAFFOLD = ROOT / "Scripts/setup_project_assets.py"
VALIDATOR_RUNNER = ROOT / "Tools/Validate_Astrawild.ps1"
PACKAGE_RUNNER = ROOT / "Tools/Package_Astrawild.ps1"
CATALOG = ROOT / "Docs/VALIDATION_CATALOG.md"

VALIDATORS = (
    "validate_content_contracts.py",
    "validate_runtime_contracts.py",
    "validate_generated_headers.py",
    "validate_editor_automation.py",
    "validate_master_echodex.py",
    "validate_generated_assets.py",
    "validate_mecha_contracts.py",
    "validate_vertical_slice_guards.py",
    "validate_character_map_assets.py",
    "validate_audio_pack.py",
    "validate_importer_coverage.py",
    "validate_vehicle_contracts.py",
    "validate_handoff_contracts.py",
)
REPORTS = (
    "DataTableImportReport.json",
    "GeneratedAssetImportReport.json",
    "GeneratedAssetRegistry.json",
    "AssetScaffoldReport.json",
)


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def importer_mapping() -> dict[str, str]:
    tree = ast.parse(read(IMPORTER), filename=str(IMPORTER))
    for node in tree.body:
        if isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name) and target.id == "TABLE_MAPPING":
                    value = ast.literal_eval(node.value)
                    if isinstance(value, dict):
                        return value
    raise ValueError("TABLE_MAPPING assignment was not found")


def ordered_positions(text: str, tokens: tuple[str, ...]) -> list[int]:
    return [text.find(token) for token in tokens]


def main() -> int:
    errors: list[str] = []
    required_files = (HANDOFF, CATALOG, IMPORTER, SCAFFOLD, VALIDATOR_RUNNER, PACKAGE_RUNNER, ROOT / "Docs/SPRINT_2_SPACE_GUILD_DYES_HANDOFF.md", ROOT / "Docs/SPRINT_3_DISASTER_KAIJU_VEHICLE_HANDOFF.md")
    for path in required_files:
        if not path.is_file():
            errors.append(f"missing handoff contract file: {path.relative_to(ROOT)}")

    if errors:
        print("ASTRAWILD handoff contract validation failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    handoff = read(HANDOFF)
    catalog = read(CATALOG)
    importer = read(IMPORTER)
    underwater_source = read(ROOT / "Source/AstrawildCore/Public/World/AstrawildUnderwaterSubsystem.h") + read(ROOT / "Source/AstrawildCore/Private/World/AstrawildUnderwaterSubsystem.cpp")
    fishing_source = read(ROOT / "Source/AstrawildCore/Public/Components/AstrawildFishingComponent.h") + read(ROOT / "Source/AstrawildCore/Private/Components/AstrawildFishingComponent.cpp")
    dye_source = read(ROOT / "Source/AstrawildCore/Public/Data/AstrawildDyeData.h")
    disaster_source = read(ROOT / "Source/AstrawildCore/Public/World/AstrawildDisasterData.h") + read(ROOT / "Source/AstrawildCore/Public/World/AstrawildDisasterSubsystem.h") + read(ROOT / "Source/AstrawildCore/Private/World/AstrawildDisasterSubsystem.cpp")
    kaiju_source = read(ROOT / "Source/AstrawildCore/Public/Data/AstrawildWorldKaijuBossData.h")
    vehicle_source = read(ROOT / "Source/AstrawildCore/Public/Components/AstrawildVehicleData.h") + read(ROOT / "Source/AstrawildCore/Public/Components/AstrawildVehicleComponent.h") + read(ROOT / "Source/AstrawildCore/Private/Components/AstrawildVehicleComponent.cpp") + read(ROOT / "Source/AstrawildCore/Public/Vehicles/AstrawildVehicleBase.h") + read(ROOT / "Source/AstrawildCore/Private/Vehicles/AstrawildVehicleBase.cpp")
    racing_source = read(ROOT / "Source/AstrawildCore/Public/World/AstrawildRacingData.h") + read(ROOT / "Source/AstrawildCore/Public/World/AstrawildRacingSubsystem.h") + read(ROOT / "Source/AstrawildCore/Private/World/AstrawildRacingSubsystem.cpp")
    space_source = read(ROOT / "Source/AstrawildCore/Public/World/AstrawildSpaceFlightData.h") + read(ROOT / "Source/AstrawildCore/Public/World/AstrawildSpaceFlightSubsystem.h") + read(ROOT / "Source/AstrawildCore/Private/World/AstrawildSpaceFlightSubsystem.cpp") + read(ROOT / "Source/AstrawildCore/Public/Environment/AstrawildLaunchPad.h") + read(ROOT / "Source/AstrawildCore/Private/Environment/AstrawildLaunchPad.cpp")
    guild_source = read(ROOT / "Source/AstrawildCore/Public/World/AstrawildGuildData.h") + read(ROOT / "Source/AstrawildCore/Public/World/AstrawildGuildSubsystem.h") + read(ROOT / "Source/AstrawildCore/Private/World/AstrawildGuildSubsystem.cpp") + read(ROOT / "Source/AstrawildCore/Public/Environment/AstrawildGuildTotem.h") + read(ROOT / "Source/AstrawildCore/Private/Environment/AstrawildGuildTotem.cpp")
    scaffold = read(SCAFFOLD)
    runner = read(VALIDATOR_RUNNER)
    package_runner = read(PACKAGE_RUNNER)

    try:
        mapping = importer_mapping()
    except (SyntaxError, ValueError) as exc:
        mapping = {}
        errors.append(f"could not parse importer TABLE_MAPPING: {exc}")

    csv_source_dir = ROOT / "Content/Astrawild/Data/Source"
    csv_files = {path.name for path in csv_source_dir.glob("*.csv")}
    if len(csv_files) != 38:
        errors.append(f"source CSV count must be 38; found {len(csv_files)}")
    if set(mapping) != csv_files:
        errors.append("handoff/importer CSV set mismatch")

    for validator in VALIDATORS:
        validator_path = ROOT / "Scripts" / validator
        if not validator_path.is_file():
            errors.append(f"missing validator file: Scripts/{validator}")
        if validator not in runner:
            errors.append(f"PowerShell runner does not invoke {validator}")
        if validator not in handoff:
            errors.append(f"handoff preflight does not name {validator}")
        if validator not in catalog:
            errors.append(f"validation catalog does not document {validator}")

    if "VALIDATION_CATALOG.md" not in handoff:
        errors.append("handoff does not link VALIDATION_CATALOG.md")

    for report in REPORTS:
        if report not in importer and report not in scaffold:
            errors.append(f"report is not produced/referenced by importer or scaffold: {report}")
        if report not in handoff:
            errors.append(f"handoff does not require inspection of {report}")

    for path in (
        ROOT / "Content/Astrawild/Data/Source/DT_UnderwaterZones.csv",
        ROOT / "Source/AstrawildCore/Public/World/AstrawildUnderwaterData.h",
        ROOT / "Source/AstrawildCore/Public/World/AstrawildUnderwaterSubsystem.h",
        ROOT / "Source/AstrawildCore/Private/World/AstrawildUnderwaterSubsystem.cpp",
        ROOT / "Source/AstrawildCore/Public/Data/AstrawildDyeData.h",
        ROOT / "Source/AstrawildCore/Public/Data/AstrawildWorldKaijuBossData.h",
        ROOT / "Source/AstrawildCore/Public/Components/AstrawildVehicleData.h",
        ROOT / "Source/AstrawildCore/Public/Data/AstrawildVehicleData.h",
        ROOT / "Source/AstrawildCore/Public/Components/AstrawildVehicleComponent.h",
        ROOT / "Source/AstrawildCore/Private/Components/AstrawildVehicleComponent.cpp",
        ROOT / "Source/AstrawildCore/Public/Vehicles/AstrawildVehicleBase.h",
        ROOT / "Source/AstrawildCore/Private/Vehicles/AstrawildVehicleBase.cpp",
        ROOT / "Source/AstrawildCore/Public/World/AstrawildDisasterData.h",
        ROOT / "Source/AstrawildCore/Public/World/AstrawildDisasterSubsystem.h",
        ROOT / "Source/AstrawildCore/Private/World/AstrawildDisasterSubsystem.cpp",
        ROOT / "Source/AstrawildCore/Public/Components/AstrawildFishingComponent.h",
        ROOT / "Source/AstrawildCore/Private/Components/AstrawildFishingComponent.cpp",
        ROOT / "Source/AstrawildCore/Public/World/AstrawildRacingData.h",
        ROOT / "Source/AstrawildCore/Public/World/AstrawildRacingSubsystem.h",
        ROOT / "Source/AstrawildCore/Private/World/AstrawildRacingSubsystem.cpp",
        ROOT / "Content/Astrawild/Data/Source/DT_WorldKaijuBosses.csv",
        ROOT / "Content/Astrawild/Data/Source/DT_Vehicles.csv",
        ROOT / "Content/Astrawild/Data/Source/DT_VehicleParts.csv",
        ROOT / "Source/AstrawildCore/Public/World/AstrawildSpaceFlightData.h",
        ROOT / "Source/AstrawildCore/Public/World/AstrawildSpaceFlightSubsystem.h",
        ROOT / "Source/AstrawildCore/Private/World/AstrawildSpaceFlightSubsystem.cpp",
        ROOT / "Source/AstrawildCore/Public/Environment/AstrawildLaunchPad.h",
        ROOT / "Source/AstrawildCore/Private/Environment/AstrawildLaunchPad.cpp",
        ROOT / "Source/AstrawildCore/Public/World/AstrawildGuildData.h",
        ROOT / "Source/AstrawildCore/Public/World/AstrawildGuildSubsystem.h",
        ROOT / "Source/AstrawildCore/Private/World/AstrawildGuildSubsystem.cpp",
        ROOT / "Source/AstrawildCore/Public/Environment/AstrawildGuildTotem.h",
        ROOT / "Source/AstrawildCore/Private/Environment/AstrawildGuildTotem.cpp",
    ):
        if not path.is_file():
            errors.append(f"missing Underwater source contract: {path.relative_to(ROOT)}")

    for token in ("AbyssalTrenchMinDepthMeters", "CalculatePressureDamagePerSecond"):
        if token not in underwater_source:
            errors.append(f"Underwater subsystem missing operational token: {token}")
    for token in ("FAstrawildFishRow", "StartFishingAuthority", "CommitCatchAuthority", "ServerUpdateReelInput_Implementation"):
        if token not in fishing_source:
            errors.append(f"Fishing component missing operational token: {token}")
    if "FAstrawildDyeRow" not in dye_source:
        errors.append("Dye data contract missing FAstrawildDyeRow")
    for token in ("EAstrawildDisasterType", "RegisterDisasterDefinition", "StartRandomDisaster", "OnDisasterStarted", "Cooldowns.FindRef"):
        if token not in disaster_source:
            errors.append(f"Disaster contract missing operational token: {token}")
    for token in ("FAstrawildWorldKaijuBossRow", "DisasterAffinityTag", "RequiredArenaTag", "EncounterRadius"):
        if token not in kaiju_source:
            errors.append(f"World Kaiju contract missing operational token: {token}")
    for token in ("FAstrawildVehicleRow", "FAstrawildVehiclePartRow", "ServerApplyControlInput", "InstalledParts", "CurrentFuel", "CurrentDurability", "AAstrawildVehicleBase"):
        if token not in vehicle_source:
            errors.append(f"Vehicle contract missing operational token: {token}")
    sprint2_handoff = read(ROOT / "Docs/SPRINT_2_SPACE_GUILD_DYES_HANDOFF.md")
    sprint3_handoff = read(ROOT / "Docs/SPRINT_3_DISASTER_KAIJU_VEHICLE_HANDOFF.md")
    for token in ("LowGravityScale", "VacuumEmergency", "GuildSubsystem", "DT_Dyes.csv", "exactly 16 original dye rows", "Network PIE"):
        if token not in sprint2_handoff:
            errors.append(f"Sprint 2 handoff missing operational token: {token}")
    for token in ("World Kaiju", "Magmatitan", "SkyColossus", "AbyssalLeviathan", "Meteor", "Tornado", "Volcanic Ash", "Aurora", "Hoverbike", "Mini-Submarine", "DT_VehicleParts.csv", "12 unique vehicle types", "Network PIE"):
        if token.lower() not in sprint3_handoff.lower():
            errors.append(f"Sprint 3 handoff missing operational token: {token}")
    for token in ("FAstrawildRaceCheckpoint", "SubmitCheckpoint", "ActivateBoostPad", "OnRaceFinished.Broadcast"):
        if token not in racing_source:
            errors.append(f"Racing subsystem missing operational token: {token}")
    for token in ("LowGravityScale", "VacuumPressureLossKPaPerSecond", "RequestLaunch", "UpdateFlightInput", "SetPilotState"):
        if token not in space_source:
            errors.append(f"Space Flight contract missing operational token: {token}")
    for token in ("RegisterBuffNode", "CaptureTerritory", "RegisterArenaTeam", "StartArenaMatch", "ArenaTeamSize"):
        if token not in guild_source:
            errors.append(f"Guild contract missing operational token: {token}")

    for token in (
        "DESTINATION_PATH = \"/Game/Astrawild/Data/Imported\"",
        "DT_UnderwaterZones.csv",
        "DT_Dyes.csv",
        "FAstrawildUnderwaterZoneRow",
        "ECHO_MESH_DESTINATION_PATH",
        "CHARACTER_MESH_DESTINATION_PATH",
        "MAP_KIT_DESTINATION_PATH",
        "AMBIENCE_DESTINATION_PATH",
        "MUSIC_DESTINATION_PATH",
        "asset_tools.import_asset_tasks(tasks)",
        "if failed:",
        "raise RuntimeError(f\"DataTable import failed",
    ):
        if token not in importer:
            errors.append(f"importer missing operational token: {token}")

    for token in (
        '"created": []',
        '"existing": []',
        '"configured": []',
        '"skipped": []',
        '"failed": []',
        "AssetScaffoldReport.json",
        "A successful script run does not prove C++ compilation",
    ):
        if token not in scaffold:
            errors.append(f"scaffold missing report/boundary token: {token}")

    for token in (
        "-TryUnreal",
        "CompileAllBlueprints",
        "BuildCookRun",
        "-platform=Win64",
        "-clientconfig=Development",
        "-cook",
        "-stage",
        "-pak",
        "-archive",
    ):
        if token not in runner and token not in package_runner:
            errors.append(f"Windows execution contract missing token: {token}")

    validator_block_match = re.search(r"\$pythonValidators\s*=\s*@\((.*?)\n\)", runner, flags=re.DOTALL)
    validator_block = validator_block_match.group(1) if validator_block_match else ""
    validator_order = ordered_positions(validator_block, tuple(f"validate_{name}.py" for name in (
        "content_contracts",
        "runtime_contracts",
        "generated_headers",
        "editor_automation",
        "master_echodex",
        "generated_assets",
        "mecha_contracts",
        "vertical_slice_guards",
        "character_map_assets",
        "audio_pack",
        "importer_coverage",
        "vehicle_contracts",
        "handoff_contracts",
    )))
    if any(position < 0 for position in validator_order):
        errors.append("PowerShell validator order cannot be resolved")
    elif validator_order != sorted(validator_order):
        errors.append("PowerShell validator order is not deterministic")

    handoff_lower = handoff.lower()
    for phrase in (
        "underwater",
        "abyssal trench",
        "source/config contract prepared; ue binary content and runtime verification pending",
        "does not prove c++ compilation",
        "does not prove blueprint compilation",
        "do not prove pie",
        "do not prove network pie",
        "do not prove packaging",
        "full-hunger",
        "two-player network pie",
        "development cook/package",
    ):
        if phrase not in handoff_lower:
            errors.append(f"handoff missing explicit boundary/test phrase: {phrase}")

    if re.search(r"\b(compile|import|PIE|package)\s+(passed|complete|successful)\b", handoff, flags=re.IGNORECASE):
        errors.append("handoff contains an unqualified success claim for a UE-only gate")

    if errors:
        print("ASTRAWILD handoff contract validation failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print("ASTRAWILD handoff contract validation passed (38 CSVs, validator order, reports, UE command gates, and evidence boundaries).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
