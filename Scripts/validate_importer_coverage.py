#!/usr/bin/env python3
"""Validate that the Unreal Editor importer covers every source asset family."""
from __future__ import annotations

import ast
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE_CSV = ROOT / "Content/Astrawild/Data/Source"
IMPORTER = ROOT / "Scripts/import_all_datatables.py"

EXPECTED_DESTINATIONS = {
    "DESTINATION_PATH": "/Game/Astrawild/Data/Imported",
    "MESH_DESTINATION_PATH": "/Game/Astrawild/Meshes/Props",
    "ECHO_MESH_DESTINATION_PATH": "/Game/Astrawild/Meshes/Echoes",
    "CHARACTER_MESH_DESTINATION_PATH": "/Game/Astrawild/Meshes/Characters",
    "MAP_KIT_DESTINATION_PATH": "/Game/Astrawild/Meshes/MapKit",
    "AUDIO_DESTINATION_PATH": "/Game/Astrawild/Audio/SFX",
    "AMBIENCE_DESTINATION_PATH": "/Game/Astrawild/Audio/Ambience",
    "MUSIC_DESTINATION_PATH": "/Game/Astrawild/Audio/Music",
}

EXPECTED_COUNTS = {
    "props": 9,
    "echoes": 218,
    "characters": 2,
    "map_kit": 4,
    "sfx": 31,
    "ambience": 9,
    "music": 2,
}


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8", errors="replace")


def literal_assignment(tree: ast.Module, name: str):
    for node in tree.body:
        if isinstance(node, ast.Assign):
            for target in node.targets:
                if isinstance(target, ast.Name) and target.id == name:
                    return ast.literal_eval(node.value)
    raise ValueError(f"{name} assignment was not found")


def manifest_count(path: Path, kind: str) -> int:
    payload = json.loads(read(path))
    entries = payload.get("assets", payload.get("entries", []))
    return sum(1 for entry in entries if entry.get("kind") == kind)


def main() -> int:
    errors: list[str] = []
    if not IMPORTER.is_file():
        errors.append("missing Scripts/import_all_datatables.py")
        print("ASTRAWILD importer coverage validation failed:")
        print("- missing Scripts/import_all_datatables.py")
        return 1

    try:
        tree = ast.parse(read(IMPORTER), filename=str(IMPORTER))
        assignments = {
            name: literal_assignment(tree, name)
            for name in ("TABLE_MAPPING", "MESH_FILES", "AUDIO_FILES")
        }
    except (SyntaxError, ValueError, TypeError) as exc:
        print("ASTRAWILD importer coverage validation failed:")
        print(f"- could not parse importer literals: {exc}")
        return 1

    importer_text = read(IMPORTER)
    mapping = assignments["TABLE_MAPPING"]
    csv_names = {path.name for path in SOURCE_CSV.glob("*.csv")}
    if len(csv_names) != 33:
        errors.append(f"source CSV count must be 33; found {len(csv_names)}")
    if set(mapping) != csv_names:
        errors.append("TABLE_MAPPING does not exactly match source CSV filenames")

    for name, expected in EXPECTED_DESTINATIONS.items():
        marker = f'{name} = "{expected}"'
        if marker not in importer_text:
            errors.append(f"missing destination contract: {marker}")

    prop_dir = ROOT / "Content/Astrawild/Meshes/Props"
    echo_dir = ROOT / "Content/Astrawild/Meshes/Echoes"
    character_dir = ROOT / "Content/Astrawild/Meshes/Characters"
    map_dir = ROOT / "Content/Astrawild/Meshes/MapKit"
    sfx_dir = ROOT / "Content/Astrawild/Audio/SFX"
    ambience_dir = ROOT / "Content/Astrawild/Audio/Ambience"
    music_dir = ROOT / "Content/Astrawild/Audio/Music"
    actual_counts = {
        "props": len(list(prop_dir.glob("*.obj"))),
        "echoes": len(list(echo_dir.glob("*.obj"))),
        "characters": len(list(character_dir.glob("*.obj"))),
        "map_kit": len(list(map_dir.glob("*.obj"))),
        "sfx": len(list(sfx_dir.glob("*.wav"))),
        "ambience": len(list(ambience_dir.glob("*.wav"))),
        "music": len(list(music_dir.glob("*.mp3"))),
    }
    for family, expected in EXPECTED_COUNTS.items():
        if actual_counts[family] != expected:
            errors.append(f"{family} source count must be {expected}; found {actual_counts[family]}")

    for filename in assignments["MESH_FILES"]:
        if not (prop_dir / filename).is_file():
            errors.append(f"explicit prop contract is missing source file: {filename}")
    for filename in assignments["AUDIO_FILES"]:
        if not (sfx_dir / filename).is_file():
            errors.append(f"explicit core SFX contract is missing source file: {filename}")

    manifest_specs = (
        (ROOT / "Content/Astrawild/Meshes/Echoes/ASTRAWILD_EchoSource_Manifest.json", "EchoSourceMesh", EXPECTED_COUNTS["echoes"]),
        (ROOT / "Content/Astrawild/Meshes/MapKit/ASTRAWILD_MapKit_Manifest.json", "MapKitSourceMesh", EXPECTED_COUNTS["map_kit"]),
    )
    for path, kind, expected in manifest_specs:
        if not path.is_file():
            errors.append(f"missing mesh manifest: {path.relative_to(ROOT)}")
            continue
        try:
            found = manifest_count(path, kind)
        except (json.JSONDecodeError, KeyError, TypeError) as exc:
            errors.append(f"invalid mesh manifest {path.relative_to(ROOT)}: {exc}")
            continue
        if found != expected:
            errors.append(f"{path.name} {kind} count must be {expected}; found {found}")

    audio_manifest = ROOT / "Content/Astrawild/Audio/ASTRAWILD_AudioPack_Manifest.json"
    if not audio_manifest.is_file():
        errors.append("missing ASTRAWILD_AudioPack_Manifest.json")
    else:
        try:
            entries = json.loads(read(audio_manifest)).get("assets", [])
            manifest_counts = {
                "SFX": sum(1 for entry in entries if entry.get("kind") == "SFX"),
                "Ambience": sum(1 for entry in entries if entry.get("kind") == "Ambience"),
                "Music": sum(1 for entry in entries if entry.get("kind") == "Music"),
            }
            if manifest_counts != {"SFX": 24, "Ambience": 9, "Music": 2}:
                errors.append(f"audio manifest counts drifted: {manifest_counts}")
        except (json.JSONDecodeError, TypeError) as exc:
            errors.append(f"invalid audio manifest: {exc}")

    for token in (
        "Meshes/Echoes",
        "Meshes/Characters",
        "Meshes/MapKit",
        "Audio/Ambience",
        "Audio/Music",
        'glob("*.obj")',
        'glob("*.wav")',
        'glob("*.mp3")',
        "GeneratedAssetImportReport.json",
        "GeneratedAssetRegistry.json",
        "asset_tools.import_asset_tasks(tasks)",
        "if failed:",
    ):
        if token not in importer_text:
            errors.append(f"missing importer implementation marker: {token}")

    if errors:
        print("ASTRAWILD importer coverage validation failed:")
        for error in errors:
            print(f"- {error}")
        return 1

    print(
        "ASTRAWILD importer coverage validation passed "
        f"(33 CSV mappings; {actual_counts['echoes']} Echo, "
        f"{actual_counts['characters']} character, {actual_counts['map_kit']} map-kit, "
        f"{actual_counts['sfx']} SFX, {actual_counts['ambience']} ambience, "
        f"{actual_counts['music']} music sources)."
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
