#!/usr/bin/env python3
"""
ASTRAWILD ArtSourceGen — procedural game-audio pack generator (44.1kHz/16-bit mono WAV).

Generates the full audio pack into <repo>/ArtSource/Audio/ (every file A_<Name>.wav):
  weapons(8)  footsteps(5)  creatures(7)  ambience loops(5)  ui(7)  player(4)  = 36 assets

Run standalone:
  python3 gen_audio.py            # all categories + validation
  python3 gen_audio.py weapons    # one category: weapons|footsteps|creatures|ambience|ui|player
  python3 gen_audio.py validate   # validate + report existing files only

Deterministic (fixed seeds everywhere). Long ambience beds use white noise + biquad
lowpass (no slow Python pink filter) per aw_audio guidance.

Loop seamlessness strategy:
- Noise carriers: aw_audio.crossfade_loop (spec tool) — verified safe only for rough/
  wide-band noise (charge-loop BP layers; robust local-median click detector confirms
  cuts are in-distribution there). For the SMOOTH lowpassed ambience beds the shipped
  function leaves an audible cut at the blend end (measured 0.15-0.25 sample step vs
  ~0.008 local median, once per 30s loop), so ambience beds use loop_overlap() below.
- Tonal loops: exact integer-cycle periodicity (Voltpylon hum 160/240Hz over 2.0s,
  Night_Crystal hum 110Hz over 30s, Heartbeat silence-wrapped thumps).
- NOTE (finding for lead): aw_audio.crossfade_loop ramps head->tail, i.e. the output
  OPENING morphs into the material tail and leaves raw cuts at the loop wrap and at
  the end of the blend region (verified empirically: max|diff| spike exactly at blend
  end + large wrap step). Harmless for stochastic/wide-band carriers, audible for
  smooth or tonal content. loop_overlap() is the corrected tail-onto-head variant.
  Foundation file intentionally untouched (out of scope); flagged in the report.
"""
from __future__ import annotations

import math
import os
import sys
import wave

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
if HERE not in sys.path:
    sys.path.insert(0, HERE)

from aw_audio import (SR, t, osc, noise, normalize, env_exp, env_attack_decay,  # noqa: E402
                      lowpass, highpass, bandpass, fm, mix, crossfade_loop,
                      slow_lfo, write_wav, wav_stats)
from aw_manifest import record  # noqa: E402

ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(ROOT, "ArtSource", "Audio")
LOG2 = math.log(2.0)

EXPECTED = [
    # weapons
    "Weapon_Scrap_Fire", "Weapon_Plasma_Fire", "Weapon_Arc_Fire", "Weapon_Rail_Fire",
    "Weapon_Singularity_Fire", "Weapon_Impact_Kinetic", "Weapon_Impact_Energy",
    "Weapon_Charge_Loop",
    # footsteps
    "Footstep_Grass", "Footstep_Stone", "Footstep_Sand", "Footstep_Water", "Footstep_Metal",
    # creatures
    "Echo_Terraquill_Call", "Echo_Cindermule_Grunt", "Echo_Voltpylon_Hum",
    "Echo_Bastionbeetle_Skitter", "Echo_Mistmender_Chime", "Echo_Deepdelver_Rumble",
    "Echo_Capture_Success",
    # ambience
    "Amb_Wind_Gentle", "Amb_Forest_Dawn", "Amb_Marsh_Dusk", "Amb_Night_Crystal",
    "Amb_Water_Lake",
    # ui
    "UI_Click", "UI_Hover", "UI_Confirm", "UI_Cancel", "UI_Craft_Done",
    "UI_Research_Done", "UI_Warning",
    # player
    "Player_Jump", "Player_Land", "Player_Scan_Ping", "Player_Heartbeat_Low",
]

LOOPS = {
    "A_Weapon_Charge_Loop", "A_Echo_Voltpylon_Hum", "A_Player_Heartbeat_Low",
    "A_Amb_Wind_Gentle", "A_Amb_Forest_Dawn", "A_Amb_Marsh_Dusk",
    "A_Amb_Night_Crystal", "A_Amb_Water_Lake",
}

# Peak tiers (all <= 0.88 hard cap; ambience bed level per spec).
PEAK = {"weapon": 0.88, "footstep": 0.70, "creature": 0.85, "ambience": 0.50,
        "ui": 0.70, "player": 0.80}


# ---------------------------------------------------------------- helpers
def buf(dur: float) -> np.ndarray:
    return np.zeros(int(SR * dur))


def place(x: np.ndarray, at: float, y: np.ndarray, gain: float = 1.0,
          fade: bool = True) -> None:
    """Add layer y into buffer x at time `at`. A tiny tail fade (<=6ms) is applied
    to the placed layer so tonal layers never end on a non-zero step (click guard)."""
    i0 = int(SR * at)
    if i0 >= len(x) or len(y) == 0:
        return
    i1 = min(len(x), i0 + len(y))
    seg = y[: i1 - i0]
    if fade:
        nf = min(int(SR * 0.006), max(len(seg) // 5, 1))
        if nf > 1:
            seg = seg.copy()
            seg[-nf:] *= np.linspace(1.0, 0.0, nf)
    x[i0:i1] += seg * gain


def to_rms(x: np.ndarray, rms: float) -> np.ndarray:
    r = float(np.sqrt(np.mean(np.square(x))))
    if r < 1e-12:
        return np.zeros_like(x)
    return x * (rms / r)


def env_aexp(dur: float, attack: float, half_life: float) -> np.ndarray:
    """Linear attack -> exponential decay (click-safe tone envelope)."""
    n = int(SR * dur)
    tt = np.arange(n) / SR
    na = max(int(SR * attack), 1)
    lin = np.minimum(np.arange(n) / na, 1.0)
    return lin * np.exp(-LOG2 * tt / half_life)


def trim_tail(x: np.ndarray, db: float = -50.0, hold: float = 0.012) -> np.ndarray:
    peak = float(np.max(np.abs(x))) or 1.0
    thr = peak * 10.0 ** (db / 20.0)
    idx = np.nonzero(np.abs(x) > thr)[0]
    if len(idx) == 0:
        return x
    return x[: min(len(x), int(idx[-1]) + int(SR * hold))]


def fade_out(x: np.ndarray, sec: float = 0.025) -> np.ndarray:
    n = int(SR * sec)
    if n >= len(x):
        n = len(x) - 1
    if n <= 1:
        return x
    x = x.copy()
    x[-n:] *= np.linspace(1.0, 0.0, n)
    return x


def close_fade(x: np.ndarray, sec: float) -> np.ndarray:
    """Linear fade over the LAST `sec` of a layer (natural ring-out, no gated click)."""
    n = int(SR * sec)
    if n >= len(x):
        n = len(x) - 1
    if n <= 1:
        return x
    y = x.copy()
    y[-n:] *= np.linspace(1.0, 0.0, n)
    return y


def loop_overlap(x: np.ndarray, fade: float) -> np.ndarray:
    """Seamless loop crossfade with the CORRECT ramp direction: the material tail
    (which naturally continues the loop end) fades OUT over the output head.
    Clickless by construction for any content. (See module docstring note.)"""
    f = int(SR * fade)
    n = len(x)
    if n <= 2 * f:
        return x.copy()
    L = n - f
    w = np.linspace(0.0, 1.0, f)
    y = x[:L].copy()
    y[:f] = x[L:] * (1.0 - w) + x[:f] * w
    return y


def scatter(rng: np.random.Generator, lo: float, hi: float, count: int, min_gap: float):
    ts = np.sort(rng.uniform(lo, hi, count))
    for i in range(1, len(ts)):
        if ts[i] - ts[i - 1] < min_gap:
            ts[i] = ts[i - 1] + min_gap
    return np.clip(ts, lo, hi)


def emit(name: str, x: np.ndarray, peak: float, cat: str, loop: bool = False) -> None:
    path = os.path.join(OUT, f"A_{name}.wav")
    x = np.asarray(x, dtype=np.float64)
    if not loop:
        x = fade_out(trim_tail(x))
    x = normalize(x, peak)
    write_wav(path, x, peak)
    st = wav_stats(path)
    record("audio", f"A_{name}",
           {**st, "bytes": os.path.getsize(path), "category": cat,
            "ue_path": f"/Game/Audio/A_{name}"})
    print(f"  A_{name}.wav  {st['seconds']:7.3f}s  {os.path.getsize(path) / 1024:7.1f} KB")


# ---------------------------------------------------------------- weapons
def gen_weapons() -> None:
    print("[weapons]")
    # -- Scrap_Fire (0.35s): LP noise burst + 110Hz thump + HP click at t=0
    burst = lowpass(noise(0.35, "white", 101) * env_exp(0.35, 0.05), 2600)
    thump = osc(110.0, 0.35) * env_exp(0.35, 0.09) * 0.85
    click = highpass(noise(0.012, "white", 102), 3000) * env_exp(0.012, 0.004) * 0.9
    emit("Weapon_Scrap_Fire", mix(to_rms(burst, 0.30), thump, click), PEAK["weapon"], "weapon")

    # -- Plasma_Fire (0.4s): FM zap + bandpassed noise layer
    body = fm(320.0, 0.4, ratio=2.7, index=4.0, index_decay=0.08, detune=1.8) \
        * env_attack_decay(0.4, 0.004, 2.5)
    layer = bandpass(noise(0.4, "white", 104) * env_exp(0.4, 0.12), 900, 3.0) * 0.2
    emit("Weapon_Plasma_Fire", mix(body, to_rms(layer, 0.07)), PEAK["weapon"], "weapon")

    # -- Arc_Fire (0.5s): 6 HP-noise ticks (2-3.4kHz) + 55Hz square hum
    x = buf(0.5)
    for i, at in enumerate((0.0, 0.03, 0.07, 0.11, 0.16, 0.22)):
        tick = bandpass(noise(0.02, "white", 110 + i) * env_exp(0.02, 0.007),
                        2200.0 + 600.0 * (i % 3), 2.0)
        place(x, at, to_rms(tick, 0.30), 0.95 - 0.07 * i)
    hum = osc(55.0, 0.5, "square") * env_exp(0.5, 0.25) * 0.3
    emit("Weapon_Arc_Fire", x + close_fade(hum, 0.06), PEAK["weapon"], "weapon")

    # -- Rail_Fire (0.7s): saw sweep down 700Hz, then metallic ring + sub
    x = buf(0.7)
    sweep = osc(700.0, 0.15, "saw", detune=-2.5) * env_attack_decay(0.15, 0.002, 2.0) * 0.55
    place(x, 0.0, sweep)
    ring_env = close_fade(env_exp(0.55, 0.45), 0.08)     # spec env_exp 0.45, ring-out
    ring = (osc(1180.0, 0.55) * 0.50 + osc(1560.0, 0.55) * 0.38
            + osc(1184.0, 0.55) * 0.18) * ring_env       # 1184 gives slow 4Hz beat
    place(x, 0.15, ring)
    sub = close_fade(osc(70.0, 0.5) * env_exp(0.5, 0.22) * 0.5, 0.08)
    place(x, 0.15, sub)
    emit("Weapon_Rail_Fire", x, PEAK["weapon"], "weapon")

    # -- Singularity_Fire (0.9s): FM sub drone + shimmer + 2kHz swell tone
    drone = fm(62.0, 0.9, ratio=1.5, index=6.0, index_decay=0.3) \
        * env_attack_decay(0.9, 0.05, 1.8)
    shim = bandpass(noise(0.9, "white", 121), 2400, 8.0) * slow_lfo(0.9, 3.0) * 0.5
    tone = osc(2000.0, 0.9) * np.sin(math.pi * t(0.9) / 0.9) * 0.22
    emit("Weapon_Singularity_Fire",
         mix(drone, to_rms(shim, 0.16), tone), PEAK["weapon"], "weapon")

    # -- Impact_Kinetic (0.3s)
    hit = lowpass(noise(0.3, "white", 131) * env_exp(0.3, 0.06), 1400)
    knock = osc(180.0, 0.3) * env_exp(0.3, 0.07) * 0.8
    emit("Weapon_Impact_Kinetic", mix(to_rms(hit, 0.30), knock), PEAK["weapon"], "weapon")

    # -- Impact_Energy (0.35s)
    zap = fm(480.0, 0.36, ratio=3.5, index=7.0, index_decay=0.05, detune=-3.0) \
        * env_attack_decay(0.36, 0.002, 2.0)
    emit("Weapon_Impact_Energy", zap, PEAK["weapon"], "weapon")

    # -- Charge_Loop (1.6s seamless LOOP): rising BP-noise sweep 500->1200 (2 layers,
    #    AM slow_lfo ~0.6Hz = 0.625Hz exact 1 cycle/loop) + rising sine 220->440
    L, fade = 1.6, 0.4
    M = L + fade
    low = crossfade_loop(bandpass(noise(M, "white", 141), 520, 1.6), fade)
    high = crossfade_loop(bandpass(noise(M, "white", 142), 1180, 1.6), fade)
    rise = np.linspace(0.0, 1.0, int(SR * L))
    am = 0.55 + 0.45 * slow_lfo(L, 1.0 / L)              # 0.625Hz, wraps at 0.55
    bed = (to_rms(low, 0.16) * (1.0 - rise) + to_rms(high, 0.16) * rise) * am
    sine_mat = osc(220.0, M, detune=LOG2 / M)            # 220 -> 440 over material
    sine = loop_overlap(sine_mat, fade) * 0.42
    emit("Weapon_Charge_Loop", bed + sine, 0.80, "weapon", loop=True)


# ---------------------------------------------------------------- footsteps
def gen_footsteps() -> None:
    print("[footsteps]")
    # Grass: soft LP noise + faint 2.5kHz swish
    soft = lowpass(noise(0.15, "white", 201), 900) * env_exp(0.15, 0.035)
    swish = bandpass(noise(0.15, "white", 202), 2500, 1.2) * env_aexp(0.15, 0.004, 0.05)
    emit("Footstep_Grass", mix(to_rms(soft, 0.30), to_rms(swish, 0.09)),
         PEAK["footstep"], "footstep")

    # Stone: sharp BP 1400 noise + short 150Hz knock + grit
    grit = bandpass(noise(0.13, "white", 211), 1400, 1.2) * env_exp(0.13, 0.028)
    knock = osc(150.0, 0.1) * env_aexp(0.1, 0.002, 0.028) * 0.8
    tick = bandpass(noise(0.05, "white", 212), 3000, 2.0) * env_exp(0.05, 0.01)
    emit("Footstep_Stone", mix(to_rms(grit, 0.30), knock, to_rms(tick, 0.10)),
         PEAK["footstep"], "footstep")

    # Sand: dull LP 600 + low rustle (LP 300, slower)
    dull = lowpass(noise(0.17, "white", 221), 600) * env_exp(0.17, 0.05)
    rustle = lowpass(noise(0.17, "white", 222), 300) * env_aexp(0.17, 0.01, 0.08)
    emit("Footstep_Sand", mix(to_rms(dull, 0.28), to_rms(rustle, 0.10)),
         PEAK["footstep"], "footstep")

    # Water: BP splash sweep (low band -> high band) + 900Hz drip blip at 60ms
    x = buf(0.18)
    sp1 = bandpass(noise(0.18, "white", 231), 700, 1.0) * env_exp(0.18, 0.05)
    sp2 = bandpass(noise(0.18, "white", 232), 1500, 1.2) * env_aexp(0.18, 0.02, 0.05)
    place(x, 0.0, mix(to_rms(sp1, 0.30), to_rms(sp2, 0.12)))
    drip = osc(900.0, 0.05, detune=-6.0) * env_aexp(0.05, 0.002, 0.012) * 0.45
    place(x, 0.06, drip)
    emit("Footstep_Water", x, PEAK["footstep"], "footstep")

    # Metal: BP 1800 Q6 noise + 620Hz decay ring (slight pitch drop)
    ringy = bandpass(noise(0.15, "white", 241), 1800, 6.0) * env_aexp(0.15, 0.002, 0.05)
    ring = osc(620.0, 0.15, detune=-1.0) * env_aexp(0.15, 0.002, 0.05) * 0.55
    emit("Footstep_Metal", mix(to_rms(ringy, 0.26), ring), PEAK["footstep"], "footstep")


# ---------------------------------------------------------------- creatures
def gen_creatures() -> None:
    print("[creatures]")
    # Terraquill_Call (0.8s): 3 soft FM chirps at 0/0.28/0.55 with pitch variation
    x = buf(0.8)
    for at, car, det, g in ((0.0, 600.0, 1.6, 0.80), (0.28, 660.0, 2.1, 0.70),
                            (0.55, 575.0, 1.3, 0.62)):
        chirp = fm(car, 0.22, ratio=2.0, index=2.5, index_decay=0.09, detune=det) \
            * env_aexp(0.22, 0.012, 0.06)
        place(x, at, chirp, g)
    emit("Echo_Terraquill_Call", x, PEAK["creature"], "creature")

    # Cindermule_Grunt (0.5s): LP 350 saw 95Hz growl + breath noise at end
    grunt = lowpass(osc(95.0, 0.5, "saw", detune=-0.8) * env_aexp(0.5, 0.015, 0.18), 350)
    breath = lowpass(noise(0.16, "white", 251), 500) * env_aexp(0.16, 0.03, 0.05)
    x = buf(0.5)
    place(x, 0.0, to_rms(grunt, 0.32), 0.9)
    place(x, 0.32, to_rms(breath, 0.12), 0.4)
    emit("Echo_Cindermule_Grunt", x, PEAK["creature"], "creature")

    # Voltpylon_Hum (2.0s LOOP): 160+240Hz beating hum + sparse HP crackle ticks.
    # Sines have exactly integer cycles over 2.0s; material is 2.05s so the spec
    # crossfade_loop(0.05) is an exact identity for the periodic part -> perfect loop.
    L, fade = 2.0, 0.05
    M = L + fade
    x = osc(160.0, M) * 0.55 + osc(240.0, M) * 0.32
    for i, at in enumerate((0.13, 0.41, 0.77, 1.02, 1.31, 1.58, 1.83)):
        tick = highpass(noise(0.006, "white", 260 + i), 2500) * env_exp(0.006, 0.002)
        place(x, at, to_rms(tick, 0.10), 0.35 + 0.06 * (i % 3))
    x = crossfade_loop(x, fade)
    emit("Echo_Voltpylon_Hum", x, 0.55, "creature", loop=True)

    # Bastionbeetle_Skitter (0.6s): 10 BP 3kHz ticks in 4-3-3 cluster rhythm
    x = buf(0.6)
    times = (0.0, 0.045, 0.09, 0.135, 0.25, 0.295, 0.34, 0.46, 0.505, 0.55)
    gains = (1.0, 0.85, 0.95, 0.8, 0.9, 0.78, 0.86, 0.95, 0.82, 0.72)
    for i, (at, g) in enumerate(zip(times, gains)):
        tick = bandpass(noise(0.008, "white", 270 + i), 3000, 2.0) * env_exp(0.008, 0.0025)
        place(x, at, to_rms(tick, 0.26), g)
    emit("Echo_Bastionbeetle_Skitter", x, PEAK["creature"], "creature")

    # Mistmender_Chime (1.1s): 3 staggered FM bells 880/1320/1760
    x = buf(1.1)
    for i, (car, g) in enumerate(((880.0, 0.8), (1320.0, 0.6), (1760.0, 0.5))):
        bell = fm(car, 0.7, ratio=1.4, index=1.2, index_decay=0.4) \
            * env_aexp(0.7, 0.004, 0.15)
        place(x, i * 0.18, bell, g)
    emit("Echo_Mistmender_Chime", x, PEAK["creature"], "creature")

    # Deepdelver_Rumble (1.4s): LP 150 saw 55Hz with 2.2Hz AM + 38Hz sub
    rum = lowpass(osc(55.0, 1.5, "saw", detune=-0.5), 150) \
        * (0.30 + 0.70 * slow_lfo(1.5, 2.2)) * env_attack_decay(1.5, 0.08, 2.2)
    sub = osc(38.0, 1.5) * env_attack_decay(1.5, 0.1, 2.2) * 0.5
    emit("Echo_Deepdelver_Rumble", mix(to_rms(rum, 0.30), sub), PEAK["creature"], "creature")

    # Capture_Success (1.2s): rising 3-note chime 523/659/784 + 5kHz sparkle fade
    x = buf(1.2)
    for f, at, g in ((523.0, 0.0, 0.75), (659.0, 0.15, 0.80), (784.0, 0.30, 0.85)):
        note = osc(f, 0.35) * env_aexp(0.35, 0.02, 0.11)
        place(x, at, note, g)
    sparkle = bandpass(noise(1.2, "white", 280), 5000, 2.0) * env_aexp(1.2, 0.03, 0.3)
    place(x, 0.0, to_rms(sparkle, 0.05))
    emit("Echo_Capture_Success", x, PEAK["creature"], "creature")


# ---------------------------------------------------------------- ambience
def gen_ambience() -> None:
    print("[ambience]  (30s seamless loops, quiet bed level 0.5 peak)")
    L, fade = 30.0, 2.0
    M = L + fade

    # -- Wind_Gentle: LP 420 @0.0667Hz LFO + LP 900 @0.0333Hz LFO (phase offset)
    # Beds use loop_overlap (corrected crossfade; see module docstring) — the shipped
    # crossfade_loop leaves an audible blend-end cut on smooth LP carriers.
    a = loop_overlap(lowpass(noise(M, "white", 11), 420), fade)
    b = loop_overlap(lowpass(noise(M, "white", 12), 900), fade)
    e1 = 0.45 + 0.55 * slow_lfo(L, 2.0 / L)              # 0.0667Hz (spec ~0.07)
    e2 = 0.30 + 0.70 * slow_lfo(L, 1.0 / L, math.pi / 2)  # 0.0333Hz (spec ~0.03)
    x = to_rms(a, 0.22) * e1 + to_rms(b, 0.10) * e2
    emit("Amb_Wind_Gentle", x, PEAK["ambience"], "ambience", loop=True)

    # -- Forest_Dawn: quieter wind bed + ~18 alien FM chirps + leaf rustle ticks
    w1 = loop_overlap(lowpass(noise(M, "white", 21), 500), fade)
    w2 = loop_overlap(lowpass(noise(M, "white", 22), 900), fade)
    x = to_rms(w1, 0.12) * (0.4 + 0.6 * slow_lfo(L, 2.0 / L)) \
        + to_rms(w2, 0.05) * (0.3 + 0.7 * slow_lfo(L, 1.0 / L, 1.2))
    rng = np.random.default_rng(23)
    for at in scatter(rng, 2.5, 28.2, 18, 0.8):
        car = float(rng.uniform(1400, 2200))
        ratio = float(rng.uniform(3.0, 5.0))
        dur = float(rng.uniform(0.10, 0.16))
        det = float(rng.uniform(0.8, 2.2))
        g = float(rng.uniform(0.22, 0.40))
        chirp = fm(car, dur, ratio=ratio, index=2.2, index_decay=0.06, detune=det) \
            * env_aexp(dur, 0.008, 0.05)
        place(x, float(at), chirp, g)
    for i in range(26):
        at = float(rng.uniform(2.5, 28.5))
        tick = bandpass(noise(0.018, "white", 40 + i), 3000, 1.5) \
            * env_aexp(0.018, 0.002, 0.006)
        place(x, at, to_rms(tick, 0.05), float(rng.uniform(0.9, 1.6)))
    emit("Amb_Forest_Dawn", x, PEAK["ambience"], "ambience", loop=True)

    # -- Marsh_Dusk: low wind + grouped frog saw-blips (LP 600) + 4kHz insect bed
    wind = loop_overlap(lowpass(noise(M, "white", 31), 300), fade)
    ins = loop_overlap(bandpass(noise(M, "white", 32), 4000, 3.0), fade)
    x = to_rms(wind, 0.16) * (0.45 + 0.55 * slow_lfo(L, 1.0 / L)) \
        + to_rms(ins, 0.035) * (0.55 + 0.45 * slow_lfo(L, 6.0))
    rng = np.random.default_rng(33)
    for gi, g0 in enumerate(scatter(rng, 3.0, 25.5, 5, 2.0)):
        for j in range(3 + (gi % 2)):
            at = float(g0) + j * float(rng.uniform(0.13, 0.19))
            f = float(rng.uniform(165, 205))
            blip = lowpass(osc(f, 0.09, "saw", detune=-1.5), 600) \
                * env_aexp(0.09, 0.006, 0.03)
            place(x, at, blip, float(rng.uniform(0.20, 0.34)))
    emit("Amb_Marsh_Dusk", x, PEAK["ambience"], "ambience", loop=True)

    # -- Night_Crystal: very low wind + sparse crystal sines 1560/2093/2637 + 110Hz hum
    wind = loop_overlap(lowpass(noise(M, "white", 41), 250), fade)
    x = to_rms(wind, 0.10) * (0.4 + 0.6 * slow_lfo(L, 1.0 / L)) \
        + osc(110.0, L) * 0.05                            # 110*30=3300 int cycles
    rng = np.random.default_rng(43)
    freqs = (1560.0, 2093.0, 2637.0)
    for i, at in enumerate(scatter(rng, 3.0, 24.0, 9, 1.4)):
        f = freqs[i % 3] * float(rng.uniform(0.998, 1.002))
        tone = osc(f, 4.0) * env_aexp(4.0, 0.05, 1.05)
        place(x, float(at), tone, float(rng.uniform(0.10, 0.16)))
    emit("Amb_Night_Crystal", x, PEAK["ambience"], "ambience", loop=True)

    # -- Water_Lake: LP 700 bed, wave swells 0.05/0.0833Hz + lap splashes BP 900
    bed = loop_overlap(lowpass(noise(M, "white", 51), 700), fade)
    # LFOs: 0.05Hz (1.5 cyc) and 0.0833Hz (2.5 cyc), both phase k*pi so the envelope
    # value is continuous at the loop wrap (half-integer cycles need sin(phase)=0).
    swell = 0.30 + 0.42 * slow_lfo(L, 1.5 / L) + 0.28 * slow_lfo(L, 2.5 / L, math.pi)
    x = to_rms(bed, 0.20) * swell
    rng = np.random.default_rng(53)
    for i, at in enumerate(scatter(rng, 2.5, 28.4, 12, 0.9)):
        splash = bandpass(noise(0.08, "white", 60 + i), 900, 1.2) \
            * env_aexp(0.08, 0.004, 0.025)
        place(x, float(at), to_rms(splash, 0.10), float(rng.uniform(0.8, 1.6)))
    emit("Amb_Water_Lake", x, PEAK["ambience"], "ambience", loop=True)


# ---------------------------------------------------------------- ui
def gen_ui() -> None:
    print("[ui]")
    # Click: 2ms HP click + 1900Hz 30ms blip
    click = highpass(noise(0.004, "white", 301), 2800) * env_exp(0.004, 0.0015)
    tone = osc(1900.0, 0.03) * env_aexp(0.03, 0.001, 0.009) * 0.6
    emit("UI_Click", mix(to_rms(click, 0.25), tone), PEAK["ui"], "ui")

    # Hover: soft 1400Hz 60ms fade in/out
    x = osc(1400.0, 0.06) * np.sin(math.pi * t(0.06) / 0.06) * 0.5
    emit("UI_Hover", x, PEAK["ui"], "ui")

    # Confirm: 880 -> 1174
    x = buf(0.2)
    place(x, 0.0, osc(880.0, 0.09) * env_aexp(0.09, 0.003, 0.03), 0.7)
    place(x, 0.08, osc(1174.0, 0.11) * env_aexp(0.11, 0.003, 0.035), 0.75)
    emit("UI_Confirm", x, PEAK["ui"], "ui")

    # Cancel: 700 -> 520 (descending)
    x = buf(0.2)
    place(x, 0.0, osc(700.0, 0.09) * env_aexp(0.09, 0.003, 0.03), 0.7)
    place(x, 0.08, osc(520.0, 0.11) * env_aexp(0.11, 0.003, 0.035), 0.75)
    emit("UI_Cancel", x, PEAK["ui"], "ui")

    # Craft_Done: bright 3-note chime 660/880/1320 + 5kHz sparkle
    x = buf(0.32)
    for f, at, g in ((660.0, 0.0, 0.65), (880.0, 0.07, 0.70), (1320.0, 0.14, 0.75)):
        place(x, at, osc(f, 0.12) * env_aexp(0.12, 0.004, 0.045), g)
    spark = bandpass(noise(0.18, "white", 302), 5000, 2.0) * env_aexp(0.18, 0.004, 0.06)
    place(x, 0.14, to_rms(spark, 0.05))
    emit("UI_Craft_Done", x, PEAK["ui"], "ui")

    # Research_Done: FM bell carrier 990 ratio 2.2 index 3
    bell = fm(990.0, 0.35, ratio=2.2, index=3.0, index_decay=0.12) \
        * env_aexp(0.35, 0.003, 0.12)
    emit("UI_Research_Done", bell, PEAK["ui"], "ui")

    # Warning: square 300Hz double pulse
    x = buf(0.26)
    for at in (0.0, 0.14):
        pulse = lowpass(osc(300.0, 0.09, "square"), 3200) * env_aexp(0.09, 0.004, 0.03)
        place(x, at, pulse, 0.55)
    emit("UI_Warning", x, PEAK["ui"], "ui")


# ---------------------------------------------------------------- player
def gen_player() -> None:
    print("[player]")
    # Jump (0.3s): BP whoosh sweep up (600->1200 staggered bands) + soft LP saw grunt
    w1 = bandpass(noise(0.3, "white", 401), 600, 1.2) * env_aexp(0.3, 0.06, 0.09)
    w2 = bandpass(noise(0.3, "white", 402), 1200, 1.2) * env_aexp(0.3, 0.10, 0.10)
    x = buf(0.3)
    place(x, 0.0, mix(to_rms(w1, 0.22), to_rms(w2, 0.12)))
    grunt = lowpass(osc(140.0, 0.08, "saw", detune=-2.0), 800) * env_aexp(0.08, 0.006, 0.03)
    place(x, 0.06, to_rms(grunt, 0.10), 0.4)
    emit("Player_Jump", x, PEAK["player"], "player")

    # Land (0.4s): 90Hz thump + LP 500 noise burst
    thump = osc(90.0, 0.4, detune=-1.5) * env_aexp(0.4, 0.004, 0.09) * 0.9
    burst = lowpass(noise(0.4, "white", 411), 500) * env_exp(0.4, 0.07)
    emit("Player_Land", mix(thump, to_rms(burst, 0.25)), PEAK["player"], "player")

    # Scan_Ping (1.2s): sonar 880Hz + echoes at 0.45 (0.4x) and 0.9 (0.16x)
    x = buf(1.2)
    for at, g, hl in ((0.0, 0.85, 0.07), (0.45, 0.34, 0.09), (0.9, 0.14, 0.11)):
        place(x, at, osc(880.0, 0.3) * env_aexp(0.3, 0.004, hl), g)
    emit("Player_Scan_Ping", x, PEAK["player"], "player")

    # Heartbeat_Low (2.0s LOOP): two LP 55Hz thumps at 0.0 / 0.62, silence-wrapped
    x = buf(2.0)
    for at, g in ((0.0, 1.0), (0.62, 0.8)):
        th = osc(55.0, 0.12, detune=-2.5) * env_aexp(0.12, 0.008, 0.035)
        place(x, at, th, g)
    emit("Player_Heartbeat_Low", x, 0.70, "player", loop=True)


# ---------------------------------------------------------------- validation
def read_wav(path: str) -> np.ndarray:
    with wave.open(path, "rb") as w:
        n = w.getnframes()
        return np.frombuffer(w.readframes(n), dtype="<i2").astype(np.float64) / 32767.0


def validate() -> int:
    os.makedirs(OUT, exist_ok=True)
    present = sorted(f[:-4] for f in os.listdir(OUT)
                     if f.startswith("A_") and f.endswith(".wav"))
    missing = [n for n in EXPECTED if ("A_" + n) not in present]
    extra = [n for n in present if n[2:] not in EXPECTED]

    print("\n=== AUDIO PACK VALIDATION ===")
    w50 = int(0.05 * SR)
    hdr = f"{'name':32s} {'sec':>7s} {'KB':>8s} {'peak':>6s}  cat"
    print(hdr)
    print("-" * len(hdr))
    total = 0
    problems = []
    cat_of = {}
    import json
    man = {}
    mpath = os.path.join(ROOT, "ArtSource", "manifest.json")
    if os.path.exists(mpath):
        with open(mpath, "r", encoding="utf-8") as f:
            man = json.load(f).get("assets", {})
    for name in present:
        p = os.path.join(OUT, name + ".wav")
        x = read_wav(p)
        secs = len(x) / SR
        kb = os.path.getsize(p) / 1024.0
        total += os.path.getsize(p)
        peak = float(np.max(np.abs(x))) if len(x) else 0.0
        cat = man.get(name, {}).get("category", "?")
        cat_of[name] = cat
        print(f"{name:32s} {secs:7.3f} {kb:8.1f} {peak:6.3f}  {cat}")
        if peak > 0.881:
            problems.append(f"{name}: peak {peak:.3f} > 0.88")
        if name not in LOOPS:
            w = min(w50, max(len(x) // 4, 441))          # never most of a short file
            tail_rms = float(np.sqrt(np.mean(np.square(x[-w:])))) if len(x) >= w else 0.0
            if tail_rms > 0.25 * max(peak, 1e-9) or (len(x) and abs(x[-1]) > 0.005):
                problems.append(f"{name}: one-shot tail not tight (rms {tail_rms:.4f},"
                                f" end {abs(x[-1]) if len(x) else 0:.4f})")
    print("-" * len(hdr))
    print(f"files: {len(present)}  total: {total / 1024 / 1024:.2f} MB (budget 40 MB)")
    if total > 40 * 1024 * 1024:
        problems.append("total pack exceeds 40MB")
    if missing:
        problems.append(f"missing {len(missing)}: {missing}")
    if extra:
        problems.append(f"unexpected: {extra}")

    # --- loop seamlessness: head/tail energies, wrap step, correlation
    print("\n--- loop seam checks (first/last 50ms) ---")
    w50 = int(0.05 * SR)
    for name in sorted(LOOPS):
        if name not in cat_of:
            continue
        x = read_wav(os.path.join(OUT, name + ".wav"))
        head, tail = x[:w50], x[-w50:]
        rh = float(np.sqrt(np.mean(np.square(head))))
        rt = float(np.sqrt(np.mean(tail ** 2)))
        peak = float(np.max(np.abs(x)))
        diffs = np.abs(np.diff(x))
        p99 = float(np.percentile(diffs, 99)) if len(diffs) else 1e-9
        wrap = float(abs(x[0] - x[-1]))
        limit = max(3.0 * p99, 0.03)
        if rt < 1e-4 * max(peak, 1e-9):                    # silence-wrapped loop
            ratio_s, verdict = "SIL", "OK" if abs(x[0]) < 0.02 * max(peak, 1e-9) else "FAIL"
        else:
            ratio = rh / rt if rt > 1e-12 else float("inf")
            ratio_s = f"{ratio:5.2f}"
            verdict = "OK" if (0.7 <= ratio <= 1.43 and wrap <= limit) else "FAIL"
        if rh > 1e-9 and rt > 1e-9:
            corr = float(np.dot(head, tail)
                         / (np.linalg.norm(head) * np.linalg.norm(tail)))
        else:
            corr = float("nan")
        print(f"{name:32s} headRMS {rh:.4f}  tailRMS {rt:.4f}  ratio {ratio_s:>5s}"
              f"  corr {corr:+.2f}  wrap {wrap:.4f} (lim {limit:.4f})  {verdict}")
        if verdict == "FAIL":
            why = []
            if rt > 1e-4 * peak and not (0.7 <= rh / max(rt, 1e-12) <= 1.43):
                why.append("energy mismatch")
            if wrap > limit:
                why.append("wrap step")
            problems.append(f"{name}: loop seam ({', '.join(why) or '?'})")

    if problems:
        print("\nPROBLEMS:")
        for pr in problems:
            print("  ! " + pr)
    else:
        print("\nALL CHECKS PASSED")
    print("note: raw head/tail correlation is ~1.0 for periodic tonal loops and"
          " ~0 for stochastic noise carriers by nature; noise seams are proven by"
          " matched energies + in-distribution wrap step (noise cuts are inaudible).")
    return 1 if problems else 0


# ---------------------------------------------------------------- main
def main() -> int:
    which = sys.argv[1] if len(sys.argv) > 1 else "all"
    if which == "validate":
        return validate()
    os.makedirs(OUT, exist_ok=True)
    cats = {"weapons": gen_weapons, "footsteps": gen_footsteps,
            "creatures": gen_creatures, "ambience": gen_ambience,
            "ui": gen_ui, "player": gen_player}
    if which == "all":
        for fn in cats.values():
            fn()
    elif which in cats:
        cats[which]()
    else:
        print("usage: gen_audio.py [weapons|footsteps|creatures|ambience|ui|player|all|validate]")
        return 2
    return validate()


if __name__ == "__main__":
    sys.exit(main())
