"""Validate ASTRAWILD multi-terrain vehicle and modular garage contracts."""
from __future__ import annotations

import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VEHICLES = ROOT / "Content/Astrawild/Data/Source/DT_Vehicles.csv"
PARTS = ROOT / "Content/Astrawild/Data/Source/DT_VehicleParts.csv"
DATA_HEADER = ROOT / "Source/AstrawildCore/Public/Components/AstrawildVehicleData.h"
DATA_FORWARD_HEADER = ROOT / "Source/AstrawildCore/Public/Data/AstrawildVehicleData.h"
COMPONENT_HEADER = ROOT / "Source/AstrawildCore/Public/Components/AstrawildVehicleComponent.h"
COMPONENT_SOURCE = ROOT / "Source/AstrawildCore/Private/Components/AstrawildVehicleComponent.cpp"
BASE_HEADER = ROOT / "Source/AstrawildCore/Public/Vehicles/AstrawildVehicleBase.h"

REQUIRED_VEHICLES = {
    "Vehicle.Hoverbike.Striker": 2800.0,
    "Vehicle.SandSkiff.DuneRider": 2200.0,
    "Vehicle.Monowheel.GyroStriker": 2400.0,
    "Vehicle.Rover.AstraExplorer": 1600.0,
    "Vehicle.SiegeTank.TitanCrawler": 900.0,
    "Vehicle.MobileBase.MammothHauler": 800.0,
    "Vehicle.JetSki.HydroGlider": 3200.0,
    "Vehicle.MiniSub.Nautilus": 1100.0,
    "Vehicle.BattleCruiser.AstraFrigate": 1800.0,
    "Vehicle.Gyrocopter.Zephyr": 2400.0,
    "Vehicle.VTOLExplorer.Skyhawk": 3800.0,
    "Vehicle.Airship.SkyGalleon": 1200.0,
}
REQUIRED_PARTS = {
    "Part.Engine.BioFuel",
    "Part.Engine.ElectricBattery",
    "Part.Engine.AstraFusion",
    "Part.Armor.HeavyTitanium",
    "Part.Armor.ForcefieldShield",
    "Part.Weapon.Autocannon",
    "Part.Weapon.MiningLaser",
    "Part.Weapon.MissilePod",
    "Part.Utility.SonarScanner",
    "Part.Utility.NitrousTank",
    "Part.Utility.CargoCrate",
    "Part.Utility.RepairDrone",
}


def load(path: Path) -> list[dict[str, str]]:
    with path.open(encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle))


def main() -> int:
    errors: list[str] = []
    for path in (VEHICLES, PARTS, DATA_HEADER, DATA_FORWARD_HEADER, COMPONENT_HEADER, COMPONENT_SOURCE, BASE_HEADER):
        if not path.is_file():
            errors.append(f"missing vehicle contract file: {path.relative_to(ROOT)}")
    if errors:
        print("ASTRAWILD vehicle contract validation failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    vehicle_rows = load(VEHICLES)
    part_rows = load(PARTS)
    vehicle_tags = {row.get("VehicleTag", "") for row in vehicle_rows}
    if len(vehicle_rows) != 12 or vehicle_tags != set(REQUIRED_VEHICLES):
        errors.append(f"DT_Vehicles.csv must contain exactly the 12 required vehicle tags; found {len(vehicle_rows)} rows")
    for row in vehicle_rows:
        tag = row.get("VehicleTag", "")
        try:
            speed = float(row["MaxSpeedCentimetersPerSecond"])
            boost = float(row["BoostSpeedCentimetersPerSecond"])
            seats = int(row["PassengerSeatCount"])
            handling = float(row["Handling"])
            depth = float(row["MaxDepthMeters"])
            cargo = float(row["CargoCapacityKilograms"])
            if speed != REQUIRED_VEHICLES.get(tag, speed) or speed <= 0 or boost < speed or seats < 1 or handling <= 0 or depth < 0 or cargo < 0:
                errors.append(f"invalid vehicle tuning in row {row.get('Name', '<unknown>')}")
        except (KeyError, ValueError):
            errors.append(f"non-numeric vehicle tuning in row {row.get('Name', '<unknown>')}")
    part_tags = {row.get("PartTag", "") for row in part_rows}
    if len(part_rows) != 12 or part_tags != REQUIRED_PARTS:
        errors.append(f"DT_VehicleParts.csv must contain exactly the 12 required part tags; found {len(part_rows)} rows")
    slots = {row.get("Slot", "") for row in part_rows}
    if slots != {"Engine", "Armor", "Weapon", "Utility"}:
        errors.append("vehicle parts must cover Engine, Armor, Weapon and Utility slots")
    for row in part_rows:
        try:
            if float(row["SpeedMultiplier"]) <= 0 or float(row["FuelConsumptionMultiplier"]) <= 0 or float(row["ArmorBonus"]) < 0 or float(row["BatteryCapacity"]) < 0 or float(row["BoostMultiplier"]) <= 0 or float(row["WeaponPower"]) < 0:
                errors.append(f"invalid vehicle-part tuning in row {row.get('Name', '<unknown>')}")
        except (KeyError, ValueError):
            errors.append(f"non-numeric vehicle-part tuning in row {row.get('Name', '<unknown>')}")

    data_text = DATA_HEADER.read_text(encoding="utf-8", errors="replace")
    component_text = COMPONENT_HEADER.read_text(encoding="utf-8", errors="replace") + COMPONENT_SOURCE.read_text(encoding="utf-8", errors="replace")
    for token in ("EAstrawildVehicleType", "EAstrawildVehicleSlot", "FAstrawildVehicleRow", "FAstrawildVehiclePartRow"):
        if token not in data_text:
            errors.append(f"vehicle data header missing {token}")
    for token in ("EnterVehicle", "ExitVehicle", "ApplyThrottle", "ActivateNitroBoost", "FireVehicleWeapon", "InstallPart", "GetFuelPercent", "GetHealthPercent", "ServerApplyControlInput", "DOREPLIFETIME"):
        if token not in component_text:
            errors.append(f"vehicle component missing operational API token: {token}")
    if "AAstrawildVehicleBase" not in BASE_HEADER.read_text(encoding="utf-8", errors="replace"):
        errors.append("vehicle base Pawn contract is missing")

    if errors:
        print("ASTRAWILD vehicle contract validation failed:")
        for error in errors:
            print(f"- {error}")
        return 1
    print("ASTRAWILD vehicle contract validation passed (12 vehicles, 12 modular parts, API and authority guards).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
