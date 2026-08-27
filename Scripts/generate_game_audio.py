"""Generate original ASTRAWILD gameplay SFX as PCM WAV files.

The output is deliberately deterministic and dependency-free: mono, signed
16-bit PCM, 44.1 kHz. These are usable source SFX for Unreal import, not a
substitute for a complete recorded/orchestrated audio mix.

Usage:
    python Scripts/generate_game_audio.py
"""
from __future__ import annotations

import hashlib
import json
import math
import random
import struct
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT_DIR = ROOT / "Content" / "Astrawild" / "Audio" / "SFX"
SAMPLE_RATE = 44_100
SEED = 712_991


def clamp(value: float, low: float = -1.0, high: float = 1.0) -> float:
    return max(low, min(high, value))


def envelope(position: int, length: int, attack: float = 0.01, release: float = 0.12) -> float:
    if length <= 1:
        return 0.0
    attack_samples = max(1, int(length * attack))
    release_samples = max(1, int(length * release))
    if position < attack_samples:
        return position / attack_samples
    if position >= length - release_samples:
        return max(0.0, (length - position) / release_samples)
    return 1.0


def sine_sweep(length: int, start_hz: float, end_hz: float, amplitude: float, attack: float = 0.01, release: float = 0.12) -> list[float]:
    values: list[float] = []
    phase = 0.0
    for index in range(length):
        t = index / max(1, length - 1)
        frequency = start_hz + (end_hz - start_hz) * t
        phase += 2.0 * math.pi * frequency / SAMPLE_RATE
        values.append(math.sin(phase) * amplitude * envelope(index, length, attack, release))
    return values


def tone(length: int, frequency: float, amplitude: float, attack: float = 0.01, release: float = 0.15, vibrato: float = 0.0) -> list[float]:
    values: list[float] = []
    for index in range(length):
        t = index / SAMPLE_RATE
        frequency_now = frequency + math.sin(t * 2.0 * math.pi * 5.0) * vibrato
        values.append(math.sin(t * 2.0 * math.pi * frequency_now) * amplitude * envelope(index, length, attack, release))
    return values


def noise(length: int, amplitude: float, attack: float = 0.005, release: float = 0.15, seed_offset: int = 0) -> list[float]:
    rng = random.Random(SEED + seed_offset)
    values: list[float] = []
    previous = 0.0
    for index in range(length):
        raw = rng.uniform(-1.0, 1.0)
        # A simple one-pole smoothing stage keeps noise useful as a soft whoosh.
        previous = previous * 0.35 + raw * 0.65
        values.append(previous * amplitude * envelope(index, length, attack, release))
    return values


def add_at(target: list[float], source: list[float], offset: int, gain: float = 1.0) -> None:
    for index, value in enumerate(source):
        destination = offset + index
        if 0 <= destination < len(target):
            target[destination] += value * gain


def render_melee_swing() -> list[float]:
    length = int(0.55 * SAMPLE_RATE)
    result = [0.0] * length
    add_at(result, sine_sweep(int(0.42 * SAMPLE_RATE), 240.0, 1_600.0, 0.34, 0.02, 0.20), 0)
    add_at(result, noise(int(0.24 * SAMPLE_RATE), 0.11, 0.01, 0.18, 1), int(0.04 * SAMPLE_RATE))
    return result


def render_melee_hit() -> list[float]:
    length = int(0.48 * SAMPLE_RATE)
    result = [0.0] * length
    add_at(result, tone(int(0.30 * SAMPLE_RATE), 92.0, 0.62, 0.002, 0.30), 0)
    add_at(result, sine_sweep(int(0.18 * SAMPLE_RATE), 1_200.0, 180.0, 0.30, 0.001, 0.14), 0)
    add_at(result, noise(int(0.15 * SAMPLE_RATE), 0.22, 0.001, 0.13, 2), 0)
    return result


def render_capture_throw() -> list[float]:
    length = int(0.62 * SAMPLE_RATE)
    result = [0.0] * length
    add_at(result, sine_sweep(int(0.54 * SAMPLE_RATE), 280.0, 2_400.0, 0.34, 0.02, 0.20), 0)
    add_at(result, tone(int(0.28 * SAMPLE_RATE), 880.0, 0.12, 0.03, 0.18, 7.0), int(0.12 * SAMPLE_RATE))
    return result


def render_capture_success() -> list[float]:
    length = int(1.28 * SAMPLE_RATE)
    result = [0.0] * length
    for offset, frequency in ((0.00, 523.25), (0.22, 659.25), (0.44, 783.99), (0.72, 1_046.50)):
        add_at(result, tone(int(0.48 * SAMPLE_RATE), frequency, 0.28, 0.015, 0.30, 2.0), int(offset * SAMPLE_RATE))
    add_at(result, sine_sweep(int(0.95 * SAMPLE_RATE), 1_600.0, 3_100.0, 0.08, 0.03, 0.45), int(0.20 * SAMPLE_RATE))
    return result


def render_dodge_roll() -> list[float]:
    length = int(0.76 * SAMPLE_RATE)
    result = [0.0] * length
    add_at(result, noise(int(0.58 * SAMPLE_RATE), 0.22, 0.02, 0.26, 3), 0)
    add_at(result, sine_sweep(int(0.52 * SAMPLE_RATE), 1_100.0, 220.0, 0.22, 0.01, 0.25), int(0.03 * SAMPLE_RATE))
    add_at(result, tone(int(0.18 * SAMPLE_RATE), 135.0, 0.16, 0.002, 0.14), int(0.42 * SAMPLE_RATE))
    return result


def render_building_place() -> list[float]:
    length = int(0.66 * SAMPLE_RATE)
    result = [0.0] * length
    add_at(result, tone(int(0.15 * SAMPLE_RATE), 180.0, 0.46, 0.001, 0.14), 0)
    add_at(result, tone(int(0.28 * SAMPLE_RATE), 360.0, 0.24, 0.005, 0.22), int(0.08 * SAMPLE_RATE))
    add_at(result, tone(int(0.42 * SAMPLE_RATE), 720.0, 0.16, 0.04, 0.30), int(0.18 * SAMPLE_RATE))
    add_at(result, noise(int(0.12 * SAMPLE_RATE), 0.10, 0.001, 0.10, 4), int(0.02 * SAMPLE_RATE))
    return result


def render_level_up() -> list[float]:
    length = int(1.55 * SAMPLE_RATE)
    result = [0.0] * length
    notes = ((0.00, 392.00), (0.18, 493.88), (0.36, 587.33), (0.54, 783.99), (0.84, 1_046.50))
    for offset, frequency in notes:
        add_at(result, tone(int(0.60 * SAMPLE_RATE), frequency, 0.23, 0.02, 0.42, 2.5), int(offset * SAMPLE_RATE))
    add_at(result, sine_sweep(int(1.05 * SAMPLE_RATE), 1_200.0, 2_800.0, 0.08, 0.02, 0.55), int(0.24 * SAMPLE_RATE))
    return result


RENDERERS = {
    "SFX_Melee_Swing": render_melee_swing,
    "SFX_Melee_Hit": render_melee_hit,
    "SFX_Capture_Throw": render_capture_throw,
    "SFX_Capture_Success": render_capture_success,
    "SFX_Dodge_Roll": render_dodge_roll,
    "SFX_Building_Place": render_building_place,
    "SFX_LevelUp": render_level_up,
}


def write_wav(path: Path, samples: list[float]) -> dict[str, int | float | str]:
    peak = max((abs(sample) for sample in samples), default=0.0)
    gain = 0.92 / peak if peak > 0.0 else 1.0
    pcm = b"".join(struct.pack("<h", int(clamp(sample * gain) * 32767.0)) for sample in samples)
    with wave.open(str(path), "wb") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(SAMPLE_RATE)
        handle.writeframes(pcm)
    return {
        "file": str(path.relative_to(ROOT)).replace("\\", "/"),
        "sample_rate_hz": SAMPLE_RATE,
        "channels": 1,
        "bits_per_sample": 16,
        "sample_count": len(samples),
        "duration_seconds": round(len(samples) / SAMPLE_RATE, 6),
        "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
    }


def generate() -> dict:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    assets = []
    for name, renderer in RENDERERS.items():
        assets.append(write_wav(OUTPUT_DIR / f"{name}.wav", renderer()))
    manifest = {
        "generator": "Scripts/generate_game_audio.py",
        "generator_version": 1,
        "seed": SEED,
        "format": "mono PCM WAV",
        "sample_rate_hz": SAMPLE_RATE,
        "bits_per_sample": 16,
        "assets": assets,
    }
    manifest_path = OUTPUT_DIR / "ASTRAWILD_SFX_Manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"Generated {len(assets)} WAV SFX in {OUTPUT_DIR}")
    for asset in assets:
        print(f"- {Path(asset['file']).name}: {asset['duration_seconds']}s, {asset['sample_count']} samples")
    print(f"Manifest: {manifest_path}")
    return manifest


if __name__ == "__main__":
    generate()
