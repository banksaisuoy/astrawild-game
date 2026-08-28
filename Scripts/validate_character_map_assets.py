"""Validate deterministic character, Echo and compact-map source mesh coverage."""
from __future__ import annotations

import csv
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
ECHO_DIR = ROOT / "Content/Astrawild/Meshes/Echoes"
CHARACTER_DIR = ROOT / "Content/Astrawild/Meshes/Characters"
MAP_DIR = ROOT / "Content/Astrawild/Meshes/MapKit"
FORBIDDEN = ("pokemon", "gundam", "wingzero", "palworld", "tekmecha", "buster", "funnel")


def read_tags(path: Path) -> set[str]:
    with path.open(encoding="utf-8-sig", newline="") as handle:
        return {row.get("SpeciesTag", "") for row in csv.DictReader(handle) if row.get("SpeciesTag")}


def validate_obj(path: Path) -> list[str]:
    errors: list[str] = []
    if not path.is_file() or path.stat().st_size < 100:
        return [f"missing or empty OBJ: {path.relative_to(ROOT)}"]
    text = path.read_text(encoding="utf-8", errors="replace")
    if "mtllib " not in text or "\no " not in text or "\nv " not in text or "\nf " not in text:
        errors.append(f"OBJ is missing required geometry/material records: {path.relative_to(ROOT)}")
    if any(term in path.name.lower() for term in FORBIDDEN):
        errors.append(f"forbidden external-IP term in asset name: {path.name}")
    return errors


def validate_manifest(path: Path, expected_files: set[str], kind: str) -> list[str]:
    errors: list[str] = []
    if not path.is_file():
        return [f"missing manifest: {path.relative_to(ROOT)}"]
    data = json.loads(path.read_text(encoding="utf-8"))
    files = {entry.get("file", "") for entry in data.get("assets", []) if entry.get("kind") == kind}
    if files != expected_files:
        errors.append(f"manifest coverage mismatch for {kind}: {path.relative_to(ROOT)}")
    return errors


def main() -> int:
    errors: list[str] = []
    expected_tags = read_tags(ROOT / "Content/Astrawild/Data/Source/DT_EchoDex.csv") | read_tags(ROOT / "Content/Astrawild/Data/Source/DT_EchoDex_200.csv")
    echo_files = {str((ECHO_DIR / f"SM_Echo_{tag.split('.')[-1]}_Source.obj").relative_to(ROOT)).replace("\\", "/") for tag in expected_tags}
    actual_echo_files = {str(path.relative_to(ROOT)).replace("\\", "/") for path in ECHO_DIR.glob("SM_Echo_*_Source.obj")}
    if actual_echo_files != echo_files:
        missing = sorted(echo_files - actual_echo_files)
        extra = sorted(actual_echo_files - echo_files)
        errors.append(f"Echo mesh coverage mismatch: missing={missing[:5]} extra={extra[:5]}")
    for path in ECHO_DIR.glob("*.obj"):
        errors.extend(validate_obj(path))
    errors.extend(validate_manifest(ECHO_DIR / "ASTRAWILD_EchoSource_Manifest.json", actual_echo_files, "EchoSourceMesh"))

    for filename in ("SM_Player_AstralSurveyor_Source.obj", "SM_Alpha_Solarix_Source.obj"):
        errors.extend(validate_obj(CHARACTER_DIR / filename))
    for filename in ("SM_DawnSpire_Kit_Source.obj", "SM_ResourceGrove_Kit_Source.obj", "SM_RestSanctuary_Kit_Source.obj", "SM_DangerPit_Kit_Source.obj"):
        errors.extend(validate_obj(MAP_DIR / filename))
    map_files = {str(path.relative_to(ROOT)).replace("\\", "/") for path in MAP_DIR.glob("SM_*_Kit_Source.obj")}
    errors.extend(validate_manifest(MAP_DIR / "ASTRAWILD_MapKit_Manifest.json", map_files, "MapKitSourceMesh"))

    if errors:
        print("ASTRAWILD character/map asset validation failed:")
        for error in errors:
            print(f"- {error}")
        return 1
    print(f"ASTRAWILD character/map asset validation passed ({len(actual_echo_files)} Echo meshes, 2 character meshes, 4 map-kit meshes).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
