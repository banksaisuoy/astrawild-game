"""Validate Unreal Editor automation contracts without importing Unreal Python."""
from __future__ import annotations

import ast
import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "Content/Astrawild/Data/Source"
IMPORTER = ROOT / "Scripts/import_all_datatables.py"
SCAFFOLD = ROOT / "Scripts/setup_project_assets.py"

EXPECTED_STRUCTS = {
    "DT_BreedingFusions.csv": "FAstrawildBreedingFusionRow",
    "DT_CampaignChapters.csv": "FAstrawildCampaignChapterRow",
    "DT_CookingRecipes.csv": "FAstrawildCookingRecipeRow",
    "DT_EcosystemBehavior.csv": "FAstrawildEcosystemBehaviorRow",
    "DT_BossAttacks.csv": "FAstrawildBossAttackRow",
    "DT_EchoDex_200.csv": "FAstrawildMasterEchoRow",
    "DT_FishDex.csv": "FAstrawildFishRow",
    "DT_Dyes.csv": "FAstrawildDyeRow",
    "DT_WorldKaijuBosses.csv": "FAstrawildWorldKaijuBossRow",
    "DT_Vehicles.csv": "FAstrawildVehicleRow",
    "DT_VehicleParts.csv": "FAstrawildVehiclePartRow",
    "DT_BossEncounters.csv": "FAstrawildBossEncounterRow",
    "DT_CyberneticEvolutions.csv": "FAstrawildCyberneticEvolutionRow",
    "DT_FoliageRules.csv": "FAstrawildFoliageRuleRow",
    "DT_MechaAnimationProfiles.csv": "FAstrawildMechaAnimationProfileRow",
    "DT_MechaVFX.csv": "FAstrawildMechaVFXBindingRow",
    "DT_MechaFrames.csv": "FAstrawildMechaFrameRow",
    "DT_MechaWeapons.csv": "FAstrawildMechaWeaponRow",
    "DT_PlayerPerks.csv": "FAstrawildPlayerPerkRow",
    "DT_PowerGrid.csv": "FAstrawildPowerGridNodeRow",
    "DT_UnderwaterZones.csv": "FAstrawildUnderwaterZoneRow",
    "DT_WorldEvents.csv": "FAstrawildWorldEventRow",
}
EXPECTED_IMPORTER_MARKERS = (
    "MESH_FILES",
    "AUDIO_FILES",
    "MESH_DESTINATION_PATH",
    "AUDIO_DESTINATION_PATH",
    "GeneratedAssetImportReport.json",
    "GeneratedAssetRegistry.json",
    "UAstrawildAudioSubsystem::PlaySFX",
    "DA_GeneratedAssetRegistry",
    "AstrawildGeneratedMeshBinding",
    "AstrawildGeneratedAudioBinding",
)
EXPECTED_SCAFFOLD_MARKERS = (
    "MPC_AstrawildLandscape",
    "PhysicsAssetFactory",
    "NiagaraSystemFactoryNew",
    "SoundCueFactoryNew",
    "AW_LandscapeWetness",
    "AW_RainIntensity",
    "AW_WindStrength",
    "AssetScaffoldReport.json",
    "MECHA_VFX_SCAFFOLDS",
    "MECHA_ANIMATION_CONTRACT_PATHS",
    "NS_AstraBeamLine",
    "NS_OverboostThrusterTrail",
    "NS_PlasmaEdgeSparks",
    "NS_AstraMuzzleFlash",
    "NS_ExosuitShutdown",
)
EXPECTED_CONFIG_MARKERS = {
    ROOT / "Config/DefaultEngine.ini": (
        "r.DynamicGlobalIlluminationMethod=1",
        "r.ReflectionMethod=1",
        "r.Lumen.HardwareRayTracing=1",
        "r.VolumetricFog=1",
        "r.VolumetricCloud=1",
    ),
    ROOT / "Config/DefaultScalability.ini": (
        "[GlobalIllUMINATIONQUALITY@3]",  # normalized case check below
        "[ReflectionQuality@3]",
        "[VolumetricFogQuality@3]",
        "[FoliageQuality@3]",
    ),
}


def find_mapping() -> dict[str, str]:
    tree = ast.parse(IMPORTER.read_text(encoding="utf-8"), filename=str(IMPORTER))
    for node in tree.body:
        if isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name) and target.id == "TABLE_MAPPING":
                    value = ast.literal_eval(node.value)
                    if not isinstance(value, dict):
                        raise ValueError("TABLE_MAPPING is not a dict")
                    return value
    raise ValueError("TABLE_MAPPING assignment was not found")


def main() -> int:
    errors: list[str] = []
    if not IMPORTER.is_file():
        errors.append("missing Scripts/import_all_datatables.py")
    if not SCAFFOLD.is_file():
        errors.append("missing Scripts/setup_project_assets.py")

    if IMPORTER.is_file():
        try:
            mapping = find_mapping()
        except (SyntaxError, ValueError) as exc:
            mapping = {}
            errors.append(f"could not parse TABLE_MAPPING: {exc}")
        csv_names = {path.name for path in SOURCE.glob("*.csv")}
        if set(mapping) != csv_names:
            errors.append(f"import mapping/source CSV mismatch: mapped={sorted(mapping)}, source={sorted(csv_names)}")
        for csv_name, expected_struct in EXPECTED_STRUCTS.items():
            if mapping.get(csv_name) != expected_struct:
                errors.append(f"{csv_name} must map to {expected_struct}; found {mapping.get(csv_name)}")

    if IMPORTER.is_file():
        importer_text = IMPORTER.read_text(encoding="utf-8")
        for marker in EXPECTED_IMPORTER_MARKERS:
            if marker not in importer_text:
                errors.append(f"import_all_datatables.py missing marker {marker}")

    if SCAFFOLD.is_file():
        scaffold_text = SCAFFOLD.read_text(encoding="utf-8")
        for marker in EXPECTED_SCAFFOLD_MARKERS:
            if marker not in scaffold_text:
                errors.append(f"setup_project_assets.py missing marker {marker}")

    for config_path, markers in EXPECTED_CONFIG_MARKERS.items():
        if not config_path.is_file():
            errors.append(f"missing config {config_path.relative_to(ROOT)}")
            continue
        config_text = config_path.read_text(encoding="utf-8", errors="replace")
        normalized = config_text.lower()
        for marker in markers:
            if marker.lower() not in normalized:
                errors.append(f"{config_path.relative_to(ROOT)} missing marker {marker}")

    if errors:
        print("ASTRAWILD Editor automation contract validation failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print(f"ASTRAWILD Editor automation contract validation passed ({len(find_mapping())} CSV mappings).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
