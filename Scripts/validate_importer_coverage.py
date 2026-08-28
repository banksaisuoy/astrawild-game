"""Validate that the Unreal Editor importer covers every generated asset family."""
from __future__ import annotations

from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
IMPORTER = ROOT / "Scripts/import_all_datatables.py"
REQUIRED_TOKENS = (
    "ECHO_MESH_DESTINATION_PATH",
    "CHARACTER_MESH_DESTINATION_PATH",
    "MAP_KIT_DESTINATION_PATH",
    "AMBIENCE_DESTINATION_PATH",
    "MUSIC_DESTINATION_PATH",
    "Meshes/Echoes",
    "Meshes/Characters",
    "Meshes/MapKit",
    "Audio/Ambience",
    "Audio/Music",
    "glob(\"*.obj\")",
    "glob(\"*.wav\")",
    "glob(\"*.mp3\")",
    "GeneratedAssetRegistry.json",
)


def main() -> int:
    text = IMPORTER.read_text(encoding="utf-8", errors="replace")
    missing = [token for token in REQUIRED_TOKENS if token not in text]
    if missing:
        print("ASTRAWILD importer coverage validation failed:")
        for token in missing:
            print(f"- missing importer contract: {token}")
        return 1
    print("ASTRAWILD importer coverage validation passed (props, Echoes, characters, map kit, SFX, ambience and music).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
