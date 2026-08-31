"""
ASTRAWILD ArtSourceGen — procedural audio synthesis foundation (44.1kHz/16-bit WAV).

Helpers: noise (white/pink), oscillators, biquad filters (LP/BP/HP), envelopes,
FM, seamless loop crossfade, and WAV writing. Recipes live in gen_audio.py.
All durations in seconds; all amplitudes in [-1, 1] (clipped at write).
"""
from __future__ import annotations

import math
import struct
import wave
from typing import Callable, Optional, Sequence

import numpy as np

SR = 44100


def t(dur: float) -> np.ndarray:
    return np.arange(int(SR * dur), dtype=np.float64) / SR


def osc(freq: float, dur: float, kind: str = "sine",
        detune: float = 0.0, phase: float = 0.0) -> np.ndarray:
    """freq may drift via detune (Hz/s exponential-ish glide)."""
    n = int(SR * dur)
    if detune != 0.0:
        f = freq * np.exp(detune * np.arange(n) / SR)
        ph = 2 * math.pi * np.cumsum(f) / SR + phase
    else:
        ph = 2 * math.pi * freq * np.arange(n) / SR + phase
    if kind == "sine":
        return np.sin(ph)
    if kind == "saw":
        return ((ph / math.pi) % 2.0) - 1.0
    if kind == "square":
        return np.sign(np.sin(ph))
    if kind == "tri":
        return 2.0 * np.abs(((ph / math.pi) % 2.0) - 1.0) - 1.0
    raise ValueError(kind)


def noise(dur: float, kind: str = "white", seed: int = 0) -> np.ndarray:
    rng = np.random.default_rng(seed)
    x = rng.standard_normal(int(SR * dur))
    if kind == "pink":
        # Voss-ish 3-stage filter
        b = [0.049922035, -0.095993537, 0.050612699, -0.004408786]
        a = [1.0, -2.494956002, 2.017265875, -0.522189400]
        y = np.zeros_like(x)
        for i in range(len(x)):
            acc = b[0] * x[i]
            for k in range(1, 4):
                acc += b[k] * x[i - k] if i - k >= 0 else 0.0
                acc -= a[k] * y[i - k] if i - k >= 0 else 0.0
            y[i] = acc
        return y * 8.0
    return x


def normalize(x: np.ndarray, peak: float = 0.9) -> np.ndarray:
    m = np.max(np.abs(x)) or 1.0
    return x * (peak / m)


def env_adsr(dur: float, attack: float, decay: float, sustain: float = 0.5,
             release: float = 0.1, sustain_level: float = 0.6) -> np.ndarray:
    n = int(SR * dur)
    a, d, r = int(SR * attack), int(SR * decay), int(SR * release)
    e = np.zeros(n)
    idx = 0
    if a > 0:
        e[:min(a, n)] = np.linspace(0, 1, min(a, n))
        idx = min(a, n)
    if d > 0 and idx < n:
        end = min(idx + d, n)
        e[idx:end] = np.linspace(1, sustain_level, end - idx)
        idx = end
    if idx < n - max(r, 1):
        e[idx:n - max(r, 1)] = sustain_level
        idx = n - max(r, 1)
    if idx < n:
        e[idx:] = np.linspace(e[idx - 1] if idx > 0 else sustain_level, 0, n - idx)
    return e


def env_exp(dur: float, half_life: float = 0.08) -> np.ndarray:
    n = int(SR * dur)
    return np.exp(-math.log(2.0) * np.arange(n) / (SR * half_life))


def env_attack_decay(dur: float, attack: float, curve: float = 3.0) -> np.ndarray:
    n = int(SR * dur)
    a = max(int(SR * attack), 1)
    e = np.ones(n)
    e[:a] = np.linspace(0, 1, a)
    tail = n - a
    if tail > 0:
        e[a:] = np.linspace(1, 0, tail) ** curve
    return e


class Biquad:
    """Direct-form-1 biquad. Types: lowpass, highpass, bandpass (constant skirt)."""

    def __init__(self, kind: str, freq: float, q: float = 0.707):
        w0 = 2 * math.pi * freq / SR
        cw, sw = math.cos(w0), math.sin(w0)
        alpha = sw / (2 * q)
        if kind == "lowpass":
            b0 = (1 - cw) / 2
            b1, b2 = 1 - cw, b0
            a0, a1, a2 = 1 + alpha, -2 * cw, 1 - alpha
        elif kind == "highpass":
            b0 = (1 + cw) / 2
            b1, b2 = -(1 + cw), b0
            a0, a1, a2 = 1 + alpha, -2 * cw, 1 - alpha
        elif kind == "bandpass":
            b0, b1, b2 = alpha, 0.0, -alpha
            a0, a1, a2 = 1 + alpha, -2 * cw, 1 - alpha
        else:
            raise ValueError(kind)
        self.b = np.array([b0 / a0, b1 / a0, b2 / a0])
        self.a = np.array([a1 / a0, a2 / a0])

    def process(self, x: np.ndarray) -> np.ndarray:
        y = np.zeros_like(x)
        x1 = x2 = y1 = y2 = 0.0
        b0, b1, b2 = self.b
        a1, a2 = self.a
        for i in range(len(x)):
            y[i] = b0 * x[i] + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2
            x2, x1 = x1, x[i]
            y2, y1 = y1, y[i]
        return y


def lowpass(x: np.ndarray, freq: float, q: float = 0.707) -> np.ndarray:
    return Biquad("lowpass", freq, q).process(x)


def highpass(x: np.ndarray, freq: float, q: float = 0.707) -> np.ndarray:
    return Biquad("highpass", freq, q).process(x)


def bandpass(x: np.ndarray, freq: float, q: float = 1.0) -> np.ndarray:
    return Biquad("bandpass", freq, q).process(x)


def fm(carrier: float, dur: float, ratio: float = 2.0, index: float = 3.0,
        index_decay: float = 0.2, detune: float = 0.0) -> np.ndarray:
    n = int(SR * dur)
    time = np.arange(n) / SR
    idx = index * np.exp(-time / max(index_decay, 1e-3))
    f = carrier * np.exp(detune * time)
    ph = 2 * math.pi * np.cumsum(f) / SR
    mod = idx * np.sin(2 * math.pi * ratio * np.cumsum(f) / SR)
    return np.sin(ph + mod)


def mix(*layers: Sequence) -> np.ndarray:
    n = max(len(l) for l in layers)
    out = np.zeros(n)
    for l in layers:
        out[:len(l)] += l
    return out


def crossfade_loop(x: np.ndarray, fade_seconds: float = 0.5) -> np.ndarray:
    """Makes a seamless loop: overlaps the tail onto the head."""
    f = int(SR * fade_seconds)
    if len(x) <= 2 * f:
        return x
    body = x[:-f].copy()
    ramp = np.linspace(0, 1, f)
    body[:f] = body[:f] * (1 - ramp) + x[-f:] * ramp
    return body


def slow_lfo(dur: float, freq: float, phase: float = 0.0) -> np.ndarray:
    n = int(SR * dur)
    return 0.5 + 0.5 * np.sin(2 * math.pi * freq * np.arange(n) / SR + phase)


def write_wav(path: str, x: np.ndarray, peak: float = 0.88) -> None:
    import os
    os.makedirs(os.path.dirname(path), exist_ok=True)
    x = np.clip(x, -peak, peak)
    x = (x * 32767.0).astype("<i2")
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(SR)
        w.writeframes(x.tobytes())


def wav_stats(path: str) -> dict:
    with wave.open(path, "rb") as w:
        return {"frames": w.getnframes(), "seconds": round(w.getnframes() / SR, 3),
                "channels": w.getnchannels(), "samplerate": w.getframerate()}
