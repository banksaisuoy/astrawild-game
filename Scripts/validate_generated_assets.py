"""Validate ASTRAWILD generated OBJ props and WAV SFX without Unreal or Blender."""
from __future__ import annotations

import hashlib
import json
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PROPS_DIR = ROOT / "Content/Astrawild/Meshes/Props"
AUDIO_DIR = ROOT / "Content/Astrawild/Audio/SFX"
PROP_MANIFEST = PROPS_DIR / "ASTRAWILD_Props_Manifest.json"
AUDIO_MANIFEST = AUDIO_DIR / "ASTRAWILD_SFX_Manifest.json"
EXPECTED_PROP_NAMES = {
    "SM_SunwoodLog",
    "SM_LumenRock",
    "SM_AstraCrystal",
    "SM_CampfireBase",
    "SM_PrimalAxe",
    "SM_PrimalPick",
    "SM_AstraResonator",
    "SM_Wall_Wood",
    "SM_Door_Wood",
}
EXPECTED_AUDIO_NAMES = {
    "SFX_Melee_Swing",
    "SFX_Melee_Hit",
    "SFX_Capture_Throw",
    "SFX_Capture_Success",
    "SFX_Dodge_Roll",
    "SFX_Building_Place",
    "SFX_LevelUp",
}


def main() -> int:
    errors: list[str] = []
    if not PROP_MANIFEST.is_file():
        errors.append(f"missing {PROP_MANIFEST.relative_to(ROOT)}")
    if not AUDIO_MANIFEST.is_file():
        errors.append(f"missing {AUDIO_MANIFEST.relative_to(ROOT)}")

    if PROP_MANIFEST.is_file():
        manifest = json.loads(PROP_MANIFEST.read_text(encoding="utf-8"))
        assets = manifest.get("assets", [])
        names = {item.get("name") for item in assets}
        if names != EXPECTED_PROP_NAMES:
            errors.append(f"OBJ manifest names mismatch: {sorted(names)}")
        for item in assets:
            relative = item.get("file", "")
            path = ROOT / relative
            if not path.is_file():
                errors.append(f"missing OBJ asset {relative}")
                continue
            text = path.read_text(encoding="utf-8", errors="replace")
            vertex_count = sum(line.startswith("v ") for line in text.splitlines())
            face_count = sum(line.startswith("f ") for line in text.splitlines())
            if vertex_count < 4 or face_count < 4:
                errors.append(f"OBJ asset {relative} has insufficient topology")
            if vertex_count != int(item.get("vertices", -1)) or face_count != int(item.get("faces", -1)):
                errors.append(f"OBJ manifest count mismatch for {relative}")
            if "mtllib ASTRAWILD_Props.mtl" not in text:
                errors.append(f"OBJ asset {relative} does not reference shared MTL")
        if not (PROPS_DIR / "ASTRAWILD_Props.mtl").is_file():
            errors.append("missing shared ASTRAWILD_Props.mtl")

    if AUDIO_MANIFEST.is_file():
        manifest = json.loads(AUDIO_MANIFEST.read_text(encoding="utf-8"))
        assets = manifest.get("assets", [])
        names = {Path(item.get("file", "")).stem for item in assets}
        if names != EXPECTED_AUDIO_NAMES:
            errors.append(f"WAV manifest names mismatch: {sorted(names)}")
        for item in assets:
            relative = item.get("file", "")
            path = ROOT / relative
            if not path.is_file():
                errors.append(f"missing WAV asset {relative}")
                continue
            try:
                with wave.open(str(path), "rb") as handle:
                    channels = handle.getnchannels()
                    sample_width = handle.getsampwidth()
                    rate = handle.getframerate()
                    frames = handle.getnframes()
                if channels != 1 or sample_width != 2 or rate != 44100 or frames <= 0:
                    errors.append(f"WAV metadata mismatch for {relative}: channels={channels}, width={sample_width}, rate={rate}, frames={frames}")
                if channels != int(item.get("channels", -1)) or sample_width * 8 != int(item.get("bits_per_sample", -1)) or rate != int(item.get("sample_rate_hz", -1)) or frames != int(item.get("sample_count", -1)):
                    errors.append(f"WAV manifest count/format mismatch for {relative}")
            except (OSError, EOFError, wave.Error) as exc:
                errors.append(f"unreadable WAV {relative}: {exc}")
            digest = hashlib.sha256(path.read_bytes()).hexdigest()
            if digest != item.get("sha256"):
                errors.append(f"WAV SHA-256 mismatch for {relative}")

    if errors:
        print("ASTRAWILD generated asset validation failed:")
        for error in errors:
            print(f"- {error}")
        return 1
    print(f"ASTRAWILD generated asset validation passed ({len(EXPECTED_PROP_NAMES)} OBJ props, {len(EXPECTED_AUDIO_NAMES)} WAV SFX).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
