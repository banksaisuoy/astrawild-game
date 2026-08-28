"""Validate the original ASTRAWILD generated audio package."""
from __future__ import annotations

import json
import subprocess
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "Content/Astrawild/Audio/ASTRAWILD_AudioPack_Manifest.json"


def probe_audio(path: Path) -> bool:
    try:
        if path.suffix.lower() == ".wav":
            with wave.open(str(path), "rb") as handle:
                return handle.getnchannels() >= 1 and handle.getframerate() > 0 and handle.getnframes() > 0
        subprocess.run(["ffprobe", "-v", "error", "-show_entries", "format=duration", "-of", "default=nw=1:nk=1", str(path)], check=True, capture_output=True, text=True)
        return True
    except (OSError, subprocess.CalledProcessError, wave.Error):
        return False


def main() -> int:
    errors: list[str] = []
    if not MANIFEST.is_file():
        print(f"Missing audio manifest: {MANIFEST.relative_to(ROOT)}")
        return 1
    data = json.loads(MANIFEST.read_text(encoding="utf-8"))
    assets = data.get("assets", [])
    counts = {kind: sum(1 for asset in assets if asset.get("kind") == kind) for kind in ("SFX", "Ambience", "Music")}
    expected = {"SFX": 24, "Ambience": 9, "Music": 2}
    if counts != expected:
        errors.append(f"unexpected audio counts: expected={expected}, actual={counts}")
    seen: set[str] = set()
    for asset in assets:
        relative_file = asset.get("file", "")
        if relative_file in seen:
            errors.append(f"duplicate manifest file: {relative_file}")
        seen.add(relative_file)
        path = ROOT / relative_file
        if not path.is_file() or path.stat().st_size < 64:
            errors.append(f"missing or empty audio file: {relative_file}")
        elif not probe_audio(path):
            errors.append(f"audio probe failed: {relative_file}")
        if asset.get("duration_seconds", 0.0) <= 0.0:
            errors.append(f"non-positive duration: {relative_file}")
    if errors:
        print("ASTRAWILD audio pack validation failed:")
        for error in errors:
            print(f"- {error}")
        return 1
    print("ASTRAWILD audio pack validation passed (24 SFX, 9 ambience loops, 2 music files).")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
