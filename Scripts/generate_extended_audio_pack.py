"""Generate original ASTRAWILD gameplay SFX and ambience with the Python standard library.

The result is deterministic source audio for Unreal import. It is not a substitute for
recorded creature performances, final foley, a full adaptive music system or mastering.
"""
from __future__ import annotations

import hashlib
import json
import math
import random
import struct
import subprocess
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SFX_DIR = ROOT / "Content/Astrawild/Audio/SFX"
AMBIENCE_DIR = ROOT / "Content/Astrawild/Audio/Ambience"
MUSIC_DIR = ROOT / "Content/Astrawild/Audio/Music"
SAMPLE_RATE = 44_100
SEED = 2_026_082_8


def clamp(value: float) -> float:
    return max(-1.0, min(1.0, value))


def env(index: int, length: int, attack: float = 0.02, release: float = 0.18) -> float:
    attack_n = max(1, int(length * attack))
    release_n = max(1, int(length * release))
    if index < attack_n:
        return index / attack_n
    if index >= length - release_n:
        return max(0.0, (length - index) / release_n)
    return 1.0


def tone(length: int, frequency: float, amplitude: float, phase: float = 0.0, attack: float = 0.02, release: float = 0.18) -> list[float]:
    return [math.sin(phase + 2.0 * math.pi * frequency * i / SAMPLE_RATE) * amplitude * env(i, length, attack, release) for i in range(length)]


def sweep(length: int, start: float, end: float, amplitude: float) -> list[float]:
    values = []
    phase = 0.0
    for index in range(length):
        fraction = index / max(1, length - 1)
        frequency = start + (end - start) * fraction
        phase += 2.0 * math.pi * frequency / SAMPLE_RATE
        values.append(math.sin(phase) * amplitude * env(index, length, 0.01, 0.20))
    return values


def noise(length: int, amplitude: float, seed_offset: int, smoothing: float = 0.55) -> list[float]:
    rng = random.Random(SEED + seed_offset)
    previous = 0.0
    values = []
    for index in range(length):
        previous = previous * smoothing + rng.uniform(-1.0, 1.0) * (1.0 - smoothing)
        values.append(previous * amplitude * env(index, length, 0.01, 0.20))
    return values


def mix(*parts: list[float]) -> list[float]:
    length = max((len(part) for part in parts), default=0)
    result = [0.0] * length
    for part in parts:
        for index, value in enumerate(part):
            result[index] += value
    return result


def add_at(target: list[float], source: list[float], offset: int, gain: float = 1.0) -> None:
    for index, value in enumerate(source):
        destination = offset + index
        if 0 <= destination < len(target):
            target[destination] += value * gain


def hit(low: float, high: float, duration: float, seed_offset: int) -> list[float]:
    length = int(duration * SAMPLE_RATE)
    return mix(tone(length, low, 0.46, release=0.28), sweep(length, high, low * 0.7, 0.30), noise(length, 0.18, seed_offset))


def render_sfx(name: str, index: int) -> list[float]:
    if name == "SFX_Player_Footstep":
        length = int(0.16 * SAMPLE_RATE)
        return mix(noise(length, 0.12, index), tone(length, 72.0, 0.20, release=0.40))
    if name == "SFX_Player_Jump":
        length = int(0.42 * SAMPLE_RATE)
        return mix(sweep(length, 120.0, 820.0, 0.20), noise(length, 0.08, index))
    if name == "SFX_Harvest_Wood":
        return hit(96.0, 620.0, 0.28, index)
    if name == "SFX_Harvest_Ore":
        return hit(145.0, 1_900.0, 0.32, index)
    if name == "SFX_Harvest_Fiber":
        return mix(noise(int(0.30 * SAMPLE_RATE), 0.20, index, 0.72), sweep(int(0.25 * SAMPLE_RATE), 700.0, 1_450.0, 0.12))
    if name == "SFX_Craft_Start":
        length = int(0.48 * SAMPLE_RATE)
        return mix(sweep(length, 240.0, 780.0, 0.16), tone(length, 420.0, 0.12))
    if name == "SFX_Craft_Complete":
        length = int(0.90 * SAMPLE_RATE)
        result = [0.0] * length
        for offset, frequency in ((0.0, 392.0), (0.16, 523.25), (0.32, 659.25)):
            add_at(result, tone(int(0.48 * SAMPLE_RATE), frequency, 0.20), int(offset * SAMPLE_RATE))
        return result
    if name == "SFX_Capture_Fail":
        return mix(sweep(int(0.42 * SAMPLE_RATE), 980.0, 160.0, 0.28), noise(int(0.38 * SAMPLE_RATE), 0.12, index))
    if name == "SFX_EchoCall_Solar":
        return mix(sweep(int(0.82 * SAMPLE_RATE), 330.0, 1_220.0, 0.20), tone(int(0.60 * SAMPLE_RATE), 660.0, 0.12))
    if name == "SFX_EchoCall_Torrent":
        return mix(sweep(int(0.84 * SAMPLE_RATE), 540.0, 210.0, 0.17), noise(int(0.75 * SAMPLE_RATE), 0.10, index, 0.86))
    if name == "SFX_EchoCall_Geo":
        return mix(tone(int(0.65 * SAMPLE_RATE), 110.0, 0.34), hit(180.0, 760.0, 0.45, index))
    if name == "SFX_Boss_Telegraph":
        length = int(0.85 * SAMPLE_RATE)
        result = [0.0] * length
        for offset in (0.0, 0.24, 0.48):
            add_at(result, tone(int(0.20 * SAMPLE_RATE), 180.0 + offset * 240.0, 0.24), int(offset * SAMPLE_RATE))
        add_at(result, noise(int(0.55 * SAMPLE_RATE), 0.10, index), int(0.12 * SAMPLE_RATE))
        return result
    if name == "SFX_Boss_PhaseShift":
        length = int(1.25 * SAMPLE_RATE)
        return mix(sweep(length, 160.0, 1_680.0, 0.26), noise(length, 0.16, index, 0.68), tone(length, 55.0, 0.20))
    if name == "SFX_Boss_Victory":
        length = int(1.25 * SAMPLE_RATE)
        result = [0.0] * length
        for offset, frequency in ((0.0, 392.0), (0.18, 493.88), (0.36, 587.33), (0.60, 783.99)):
            add_at(result, tone(int(0.65 * SAMPLE_RATE), frequency, 0.24), int(offset * SAMPLE_RATE))
        return result
    if name == "SFX_Boss_Defeat":
        return mix(sweep(int(1.05 * SAMPLE_RATE), 480.0, 72.0, 0.28), tone(int(0.85 * SAMPLE_RATE), 64.0, 0.24))
    if name == "SFX_Mecha_Flight":
        return mix(noise(int(0.80 * SAMPLE_RATE), 0.17, index, 0.70), sweep(int(0.78 * SAMPLE_RATE), 180.0, 920.0, 0.12))
    if name == "SFX_Mecha_Overboost":
        return mix(noise(int(1.05 * SAMPLE_RATE), 0.24, index, 0.42), sweep(int(0.98 * SAMPLE_RATE), 240.0, 1_600.0, 0.22))
    if name == "SFX_Mecha_Beam":
        return mix(tone(int(0.58 * SAMPLE_RATE), 220.0, 0.22), sweep(int(0.52 * SAMPLE_RATE), 1_900.0, 360.0, 0.26))
    if name == "SFX_Mecha_Plasma":
        return mix(noise(int(0.44 * SAMPLE_RATE), 0.17, index), sweep(int(0.40 * SAMPLE_RATE), 2_700.0, 460.0, 0.28))
    if name == "SFX_Mecha_Shutdown":
        return mix(sweep(int(1.10 * SAMPLE_RATE), 1_050.0, 82.0, 0.22), tone(int(0.95 * SAMPLE_RATE), 58.0, 0.18))
    if name == "SFX_Cooking_Complete":
        return mix(tone(int(0.70 * SAMPLE_RATE), 523.25, 0.18), sweep(int(0.54 * SAMPLE_RATE), 580.0, 1_240.0, 0.12))
    if name == "SFX_Eat_Drink":
        return mix(noise(int(0.48 * SAMPLE_RATE), 0.12, index, 0.80), tone(int(0.45 * SAMPLE_RATE), 260.0, 0.14))
    if name == "SFX_UI_Hover":
        return tone(int(0.09 * SAMPLE_RATE), 1_100.0, 0.10, release=0.60)
    if name == "SFX_UI_Confirm":
        return mix(tone(int(0.18 * SAMPLE_RATE), 780.0, 0.16), tone(int(0.24 * SAMPLE_RATE), 1_170.0, 0.13))
    raise KeyError(name)


SFX_NAMES = (
    "SFX_Player_Footstep", "SFX_Player_Jump", "SFX_Harvest_Wood", "SFX_Harvest_Ore", "SFX_Harvest_Fiber",
    "SFX_Craft_Start", "SFX_Craft_Complete", "SFX_Capture_Fail", "SFX_EchoCall_Solar", "SFX_EchoCall_Torrent",
    "SFX_EchoCall_Geo", "SFX_Boss_Telegraph", "SFX_Boss_PhaseShift", "SFX_Boss_Victory", "SFX_Boss_Defeat",
    "SFX_Mecha_Flight", "SFX_Mecha_Overboost", "SFX_Mecha_Beam", "SFX_Mecha_Plasma", "SFX_Mecha_Shutdown",
    "SFX_Cooking_Complete", "SFX_Eat_Drink", "SFX_UI_Hover", "SFX_UI_Confirm",
)


AMBIENCE_PROFILES = {
    "AMB_DawnMeadows_Day": (210.0, 0.035, 0.18, 1),
    "AMB_DawnMeadows_Night": (138.0, 0.026, 0.12, 2),
    "AMB_SylvanRainforest_Day": (290.0, 0.060, 0.25, 3),
    "AMB_SylvanRainforest_Night": (196.0, 0.050, 0.20, 4),
    "AMB_ScorchedObsidianCaldera_Day": (82.0, 0.075, 0.32, 5),
    "AMB_ScorchedObsidianCaldera_Night": (61.0, 0.055, 0.23, 6),
    "AMB_GlacialZenith_Day": (368.0, 0.030, 0.15, 7),
    "AMB_GlacialZenith_Night": (246.0, 0.024, 0.11, 8),
    "AMB_DangerPit_Combat": (72.0, 0.095, 0.40, 9),
}


def render_ambience(name: str, base_frequency: float, noise_gain: float, pulse_gain: float, seed_offset: int) -> list[float]:
    length = 20 * SAMPLE_RATE
    result = [0.0] * length
    rng = random.Random(SEED + seed_offset)
    for index in range(length):
        time = index / SAMPLE_RATE
        low = math.sin(2.0 * math.pi * base_frequency * time) * noise_gain
        shimmer = math.sin(2.0 * math.pi * (base_frequency * 1.47) * time + math.sin(time * 0.07) * 0.8) * noise_gain * 0.35
        pulse = math.sin(2.0 * math.pi * 0.08 * time + seed_offset) * pulse_gain * 0.25
        grain = rng.uniform(-1.0, 1.0) * noise_gain * 0.10
        result[index] = low + shimmer + pulse + grain
    fade = int(0.75 * SAMPLE_RATE)
    for index in range(fade):
        result[index] *= index / fade
        result[-1 - index] *= index / fade
    return result


def write_wav(path: Path, samples: list[float], loop: bool = False) -> dict[str, str | int | float | bool]:
    peak = max((abs(value) for value in samples), default=0.0)
    gain = 0.92 / peak if peak > 0.0 else 1.0
    pcm = b"".join(struct.pack("<h", int(clamp(value * gain) * 32767.0)) for value in samples)
    with wave.open(str(path), "wb") as handle:
        handle.setnchannels(1)
        handle.setsampwidth(2)
        handle.setframerate(SAMPLE_RATE)
        handle.writeframes(pcm)
    return {"file": str(path.relative_to(ROOT)).replace("\\", "/"), "sample_rate_hz": SAMPLE_RATE, "channels": 1, "bits_per_sample": 16, "duration_seconds": round(len(samples) / SAMPLE_RATE, 6), "loop_candidate": loop, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def main() -> int:
    SFX_DIR.mkdir(parents=True, exist_ok=True)
    AMBIENCE_DIR.mkdir(parents=True, exist_ok=True)
    MUSIC_DIR.mkdir(parents=True, exist_ok=True)
    assets: list[dict] = []
    for index, name in enumerate(SFX_NAMES, start=40):
        assets.append({"kind": "SFX", "cue_id": name.replace("SFX_", "SFX."), **write_wav(SFX_DIR / f"{name}.wav", render_sfx(name, index))})
    for name, profile in AMBIENCE_PROFILES.items():
        assets.append({"kind": "Ambience", "cue_id": name.replace("AMB_", "Ambience."), **write_wav(AMBIENCE_DIR / f"{name}.wav", render_ambience(name, *profile), loop=True)})
    for path in sorted(list(MUSIC_DIR.glob("MUS_*.wav")) + list(MUSIC_DIR.glob("MUS_*.mp3"))):
        if path.suffix.lower() == ".wav":
            with wave.open(str(path), "rb") as handle:
                sample_rate = handle.getframerate()
                channels = handle.getnchannels()
                bits_per_sample = handle.getsampwidth() * 8
                duration = handle.getnframes() / max(1, handle.getframerate())
        else:
            probe = subprocess.run(["ffprobe", "-v", "error", "-show_entries", "stream=sample_rate,channels,duration", "-of", "json", str(path)], check=True, capture_output=True, text=True)
            stream = json.loads(probe.stdout).get("streams", [{}])[0]
            sample_rate = int(stream.get("sample_rate", 0) or 0)
            channels = int(stream.get("channels", 0) or 0)
            bits_per_sample = 0
            duration = float(stream.get("duration", 0.0) or 0.0)
        assets.append({"kind": "Music", "cue_id": path.stem.replace("MUS_", "Music."), "file": str(path.relative_to(ROOT)).replace("\\", "/"), "sample_rate_hz": sample_rate, "channels": channels, "bits_per_sample": bits_per_sample, "duration_seconds": round(duration, 6), "loop_candidate": True, "sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    manifest = {"schema": "ASTRAWILD.AudioPack.v1", "generator": "Scripts/generate_extended_audio_pack.py", "seed": SEED, "assets": assets, "notes": ["Generated WAV is original source material for Editor import.", "Replace or augment Echo calls, foley, ambience and music with original recorded or licensed content before final release.", "SoundCue routing, attenuation, concurrency and adaptive music state remain Unreal Editor integration work."]}
    output = ROOT / "Content/Astrawild/Audio/ASTRAWILD_AudioPack_Manifest.json"
    output.write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"Generated {len([a for a in assets if a['kind'] == 'SFX'])} SFX, {len([a for a in assets if a['kind'] == 'Ambience'])} ambience loops, and {len([a for a in assets if a['kind'] == 'Music'])} music files.")
    print(f"Manifest: {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
