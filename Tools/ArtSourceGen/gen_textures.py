"""
ASTRAWILD ArtSourceGen — PBR texture pack (tiling, deterministic).

Layers per surface: BaseColor (_D, sRGB), Normal (_N, linear), packed
ORM (R=AO, G=Roughness, B=Metallic, linear), emissive masks (_E/_M).
All noise is tileable (wrapped lattice). UE import flags are set by
Content/Python/AwPipeline/import_all.py (sRGB only for _D).

Run: python3 gen_textures.py
Output: /ArtSource/Textures/T_*.png  (39 maps)
"""
from __future__ import annotations

import math
import os
import sys

import numpy as np
from PIL import Image

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from aw_manifest import record

OUT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "ArtSource", "Textures"))


# ------------------------------------------------------------------ noise core
def _hash01(ix: int, iy: int, seed: int) -> float:
    h = ix * 374761393 + iy * 668265263 + seed * 144665477
    h = (h ^ (h >> 13)) * 1274126177
    h = h ^ (h >> 16)
    return (h & 0x7FFFFFFF) / 0x7FFFFFFF


def value_noise(shape, period: int, seed: int) -> np.ndarray:
    h, w = shape
    idx = np.arange(period)
    vh = np.array([[_hash01(i, j, seed) for j in idx] for i in idx])
    yy, xx = np.mgrid[0:h, 0:w]
    fx = xx / w * period
    fy = yy / h * period
    x0f = np.floor(fx)
    y0f = np.floor(fy)
    x0 = x0f.astype(int) % period
    y0 = y0f.astype(int) % period
    x1 = (x0 + 1) % period
    y1 = (y0 + 1) % period
    tx = fx - x0f
    ty = fy - y0f
    sx = tx * tx * (3 - 2 * tx)
    sy = ty * ty * (3 - 2 * ty)
    v00 = vh[y0, x0]
    v10 = vh[y0, x1]
    v01 = vh[y1, x0]
    v11 = vh[y1, x1]
    return (v00 * (1 - sx) * (1 - sy) + v10 * sx * (1 - sy) +
            v01 * (1 - sx) * sy + v11 * sx * sy)


def fbm(shape, base_period: int, octaves: int, seed: int, gain: float = 0.5) -> np.ndarray:
    total = np.zeros(shape, dtype=np.float64)
    amp = 1.0
    norm = 0.0
    p = base_period
    for o in range(octaves):
        total += amp * value_noise(shape, p, seed + o * 131)
        norm += amp
        amp *= gain
        p = max(2, p * 2)
    return total / norm


def ridged(shape, base_period: int, octaves: int, seed: int) -> np.ndarray:
    n = fbm(shape, base_period, octaves, seed)
    return 1.0 - np.abs(2.0 * n - 1.0)


def worley(shape, cells: int, seed: int) -> np.ndarray:
    """Tileable worley distance (F1), 0 at cell centers. Fully vectorized."""
    h, w = shape
    idx = np.arange(cells)
    r = np.array([[_hash01(i, j, seed) for j in idx] for i in idx])
    g = np.array([[_hash01(i, j, seed + 7) for j in idx] for i in idx])
    yy, xx = np.mgrid[0:h, 0:w]
    fx = xx / w * cells
    fy = yy / h * cells
    x0 = np.floor(fx).astype(int)
    y0 = np.floor(fy).astype(int)
    dist = np.full(shape, 1e9)
    for di in (-1, 0, 1):
        for dj in (-1, 0, 1):
            gx = (x0 + di) % cells
            gy = (y0 + dj) % cells
            px = gx + r[gy, gx]
            py = gy + g[gy, gx]
            d = np.sqrt((fx - px) ** 2 + (fy - py) ** 2)
            dist = np.minimum(dist, d)
    return np.clip(dist / 1.35, 0, 1)


def normal_from_height(h: np.ndarray, strength: float = 2.2) -> np.ndarray:
    """Tiling normal (RGB, 0-255) from a height field via wrapped gradients."""
    gx = np.roll(h, -1, axis=1) - np.roll(h, 1, axis=1)
    gy = np.roll(h, -1, axis=0) - np.roll(h, 1, axis=0)
    n = np.dstack([-gx * strength * 32, -gy * strength * 32, np.ones_like(h)])
    n /= np.linalg.norm(n, axis=2, keepdims=True)
    return ((n * 0.5 + 0.5) * 255).astype(np.uint8)


def ao_from_height(h: np.ndarray, levels: int = 3) -> np.ndarray:
    ao = np.zeros_like(h)
    for l in range(1, levels + 1):
        k = 2 ** l
        blur = h
        for _ in range(l * 2):
            blur = (np.roll(blur, 1, 0) + np.roll(blur, -1, 0) +
                    np.roll(blur, 1, 1) + np.roll(blur, -1, 1)) * 0.25
        ao += np.clip(blur - h, 0, 1)
    return np.clip(1.0 - ao * 1.4, 0, 1)


def to_rgb(arrs) -> Image.Image:
    return Image.merge("RGB", [Image.fromarray(a.astype(np.uint8)) for a in arrs])


def save(arr, name: str, category: str, srgb: bool = True, extra: dict = None) -> str:
    path = os.path.join(OUT, name + ".png")
    if isinstance(arr, Image.Image):
        img = arr
    else:
        img = Image.fromarray(arr.astype(np.uint8))
    img.save(path, optimize=True)
    record("texture", name, {"path": path, "srgb": srgb, "size": list(img.size),
                             "bytes": os.path.getsize(path),
                             "ue_path": f"/Game/Textures/{name}",
                             "category": category, **(extra or {})})
    print(f"[tex] {name} {img.size} {os.path.getsize(path)//1024}KB")
    return path


def lerp_color(c1, c2, t):
    t = np.clip(t, 0, 1)
    return np.array(c1)[None, None, :] * (1 - t[..., None]) + np.array(c2)[None, None, :] * t[..., None]


# ------------------------------------------------------------------- recipes
def gen_landscape():
    S = (1024, 1024)
    # Grass — bioluminescent teal-green with blade streaks
    base = fbm(S, 8, 5, 11)
    streaks = value_noise(S, 96, 31)
    blades = np.sin(np.mgrid[0:S[0], 0:S[1]][1] * 0.7 + streaks * 24) * 0.5 + 0.5
    h = base * 0.6 + blades * 0.4
    d = lerp_color((52, 84, 74), (96, 148, 118), base * 0.5 + blades * 0.3)
    glow = np.clip(ridged(S, 12, 4, 55) - 0.82, 0, 1) * 3.5
    d[..., 1] += glow * 60
    d[..., 2] += glow * 34
    save(to_rgb([d[..., 0], d[..., 1], d[..., 2]]), "T_Landscape_Grass_D", "landscape")
    save(normal_from_height(h, 2.6), "T_Landscape_Grass_N", "landscape", srgb=False)
    # Soil — fertile loam
    pebbles = np.clip(worley(S, 24, 77) - 0.55, 0, 1) * 2
    soiln = fbm(S, 6, 5, 42)
    d = lerp_color((94, 74, 54), (126, 102, 74), soiln)
    d[..., 0] += pebbles * 26
    save(to_rgb([d[..., 0], d[..., 1], d[..., 2]]), "T_Landscape_Soil_D", "landscape")
    save(normal_from_height(soiln * 0.5 + pebbles * 0.5, 2.0), "T_Landscape_Soil_N", "landscape", srgb=False)
    # Sand — ripples
    yy, xx = np.mgrid[0:S[0], 0:S[1]]
    rip = np.sin(xx * 0.11 + np.sin(yy * 0.045) * 3.2 + fbm(S, 5, 3, 9) * 5)
    sand = rip * 0.35 + fbm(S, 8, 4, 12) * 0.65
    d = lerp_color((196, 178, 140), (228, 212, 172), sand)
    save(to_rgb([d[..., 0], d[..., 1], d[..., 2]]), "T_Landscape_Sand_D", "landscape")
    save(normal_from_height(sand, 1.6), "T_Landscape_Sand_N", "landscape", srgb=False)
    # Granite — weathered cliff
    wor = worley(S, 16, 90)
    cracks = np.clip(0.5 - wor, 0, 1) * 3.0
    grain = fbm(S, 32, 5, 66)
    d = lerp_color((116, 114, 118), (148, 146, 150), grain * 0.6 + (1 - wor) * 0.4)
    d[..., 0] -= cracks * 42
    d[..., 1] -= cracks * 44
    d[..., 2] -= cracks * 40
    save(to_rgb([d[..., 0], d[..., 1], d[..., 2]]), "T_Landscape_Granite_D", "landscape")
    save(normal_from_height((1 - wor) * 0.55 + grain * 0.45 + cracks * 0.8, 3.2),
         "T_Landscape_Granite_N", "landscape", srgb=False)
    # shared landscape ORM (AO from granite height, roughness variation)
    hgt = (1 - wor) * 0.5 + grain * 0.5
    orm = np.dstack([ao_from_height(hgt) * 255,
                     (np.full(S, 0.86) - grain * 0.12) * 255,
                     np.zeros(S)])
    save(orm, "T_Landscape_ORM", "landscape", srgb=False)


def gen_survivor():
    S = (2048, 2048)
    # Panels: softsuit fabric zones + seams + wear
    cell = 8
    panel = np.zeros(S)
    for i in range(0, S[0], S[0] // cell):
        panel[:, i:i + 3] = 1.0
    grid = value_noise(S, 16, 21)
    fabric = fbm(S, 64, 4, 22)
    seam = np.clip(np.abs(grid - 0.5) * 2 - 0.86, 0, 1) * 6
    wear = np.clip(ridged(S, 48, 4, 25) - 0.7, 0, 1) * 3
    d = lerp_color((64, 72, 60), (96, 104, 84), fabric)
    d[..., 0] -= seam * 30 + wear * 14
    d[..., 1] -= seam * 30 + wear * 16
    d[..., 2] -= seam * 28 + wear * 12
    # amber accent stripes on a diagonal band
    yy, xx = np.mgrid[0:S[0], 0:S[1]]
    band = np.clip(np.sin((xx + yy) * 0.004) - 0.7, 0, 1) * 3
    d[..., 0] += band * 90
    d[..., 1] += band * 50
    d[..., 2] += band * 6
    save(to_rgb([d[..., 0], d[..., 1], d[..., 2]]), "T_Survivor_Exosuit_D", "survivor")
    save(normal_from_height(fabric[::2, ::2] * 0.7 + seam[::2, ::2] * 0.5 + panel[::2, ::2] * 0.15, 1.8),
         "T_Survivor_Exosuit_N", "survivor", srgb=False)
    metal_mask = np.clip(seam * 0.4 + (grid > 0.62) * 0.8, 0, 1)
    orm = np.dstack([ao_from_height((fabric + seam)[::2, ::2]) * 255,
                     (np.full((1024, 1024), 0.62) + fabric[::2, ::2] * 0.2 - metal_mask[::2, ::2] * 0.3) * 255,
                     metal_mask[::2, ::2] * 255])
    save(orm, "T_Survivor_ORM", "survivor", srgb=False)
    # Visor emissive — teal scanline gradient
    vy = np.mgrid[0:1024, 0:1024][0] / 1024
    scan = (np.sin(np.mgrid[0:1024, 0:1024][0] * math.pi * 2 / 26) * 0.5 + 0.5)
    e = np.clip(vy * 1.4 - 0.25, 0, 1) * (0.45 + scan * 0.55)
    emis = np.dstack([e * 70, e * 235, e * 215])
    save(emis, "T_Survivor_Visor_E", "survivor", srgb=False)


def gen_weapons():
    S = (1024, 1024)
    # Scraps — welded plates, scratches, rust
    wor = worley(S, 12, 51)
    plates = 1.0 - wor
    scratch = np.clip(ridged(S, 24, 4, 52) - 0.78, 0, 1) * 4
    rust = np.clip(fbm(S, 10, 4, 53) - 0.62, 0, 1) * 2.2
    d = lerp_color((112, 106, 96), (158, 152, 138), plates)
    d[..., 0] += rust * 52 + scratch * 60
    d[..., 1] += rust * 22 + scratch * 55
    d[..., 2] += rust * 6 + scratch * 45
    save(to_rgb([d[..., 0], d[..., 1], d[..., 2]]), "T_Weapon_Scraps_D", "weapons")
    save(normal_from_height(plates * 0.5 + scratch * 0.9 + rust * 0.4, 2.8),
         "T_Weapon_Scraps_N", "weapons", srgb=False)
    orm = np.dstack([ao_from_height(plates + scratch) * 255,
                     (np.full(S, 0.55) - plates * 0.18 + rust * 0.3) * 255,
                     (np.full(S, 0.72) - rust * 0.6) * 255])
    save(orm, "T_Weapon_Scraps_ORM", "weapons", srgb=False)
    # Tech — clean panel lines + vents
    yy, xx = np.mgrid[0:S[0], 0:S[1]]
    plines = np.clip(np.abs(np.sin(xx * math.pi / 128)) - 0.985, 0, 1) * 40
    vents = np.clip(np.sin(yy * math.pi / 40) - 0.6, 0, 1)
    smudge = fbm(S, 20, 4, 61)
    d = lerp_color((70, 74, 82), (104, 110, 120), smudge * 0.6 + vents * 0.2)
    d[..., 0] -= plines
    d[..., 1] -= plines
    d[..., 2] -= plines * 0.8
    d[..., 1] -= vents * 12
    save(to_rgb([d[..., 0], d[..., 1], d[..., 2]]), "T_Weapon_Tech_D", "weapons")
    save(normal_from_height(plines * 0.6 + vents * 0.5 + smudge * 0.3, 2.0),
         "T_Weapon_Tech_N", "weapons", srgb=False)
    orm = np.dstack([ao_from_height(smudge + plines) * 255,
                     (np.full(S, 0.42) + smudge * 0.15) * 255,
                     (np.full(S, 0.85) - vents * 0.5) * 255])
    save(orm, "T_Weapon_Tech_ORM", "weapons", srgb=False)
    # Weapon glow emissive mask (512)
    g = np.mgrid[0:512, 0:512][1] / 512
    e = np.clip(1.0 - np.abs(g - 0.5) * 2.4, 0, 1) ** 1.6
    save(np.dstack([e * 255, e * 255, e * 255]), "T_Weapon_Glow_E", "weapons", srgb=False)


def gen_echo():
    S = (1024, 1024)
    # Chitin/fur hybrid: hex-ish cells + soft underlayer
    wor = worley(S, 20, 71)
    cells = np.clip(wor * 1.6, 0, 1)
    fur = fbm(S, 48, 5, 72)
    d = lerp_color((78, 70, 62), (124, 112, 98), fur * 0.6 + cells * 0.4)
    d[..., 2] += cells * 18
    save(to_rgb([d[..., 0], d[..., 1], d[..., 2]]), "T_Echo_Body_D", "echo")
    save(normal_from_height(fur * 0.4 + (1 - cells) * 0.6, 2.4), "T_Echo_Body_N", "echo", srgb=False)
    orm = np.dstack([ao_from_height(cells + fur) * 255,
                     (np.full(S, 0.55) + fur * 0.25) * 255,
                     np.zeros(S)])
    save(orm, "T_Echo_Body_ORM", "echo", srgb=False)
    # Emissive vein mask (512): fissure network
    r = ridged((512, 512), 6, 5, 81)
    veins = np.clip(r - 0.74, 0, 1) * 4.2
    save(np.dstack([veins * 255, veins * 255, veins * 255]),
         "T_Echo_Emissive_M", "echo", srgb=False)


def gen_environment():
    S = (1024, 1024)
    # Canopy leaves
    wor = worley(S, 28, 101)
    leaf = 1.0 - np.clip(wor * 1.5, 0, 1)
    v = fbm(S, 16, 4, 102)
    d = lerp_color((34, 76, 58), (72, 128, 92), leaf * 0.7 + v * 0.3)
    glow = np.clip(ridged(S, 40, 3, 103) - 0.86, 0, 1) * 4
    d[..., 1] += glow * 40
    d[..., 2] += glow * 24
    save(to_rgb([d[..., 0], d[..., 1], d[..., 2]]), "T_Foliage_Canopy_D", "environment")
    save(normal_from_height(leaf * 0.8 + v * 0.2, 2.6), "T_Foliage_Canopy_N", "environment", srgb=False)
    # Bark
    yy, xx = np.mgrid[0:S[0], 0:S[1]]
    striate = np.sin(yy * 0.55 + fbm(S, 8, 3, 111) * 9)
    bark = striate * 0.4 + fbm(S, 24, 4, 112) * 0.6
    d = lerp_color((74, 62, 50), (108, 92, 72), bark)
    save(to_rgb([d[..., 0], d[..., 1], d[..., 2]]), "T_Bark_D", "environment")
    save(normal_from_height(bark, 3.0), "T_Bark_N", "environment", srgb=False)
    # Rock granite (darker than landscape cliffs)
    wor2 = worley(S, 14, 120)
    grain = fbm(S, 40, 5, 121)
    d = lerp_color((96, 94, 92), (132, 130, 128), grain)
    cracks = np.clip(0.5 - wor2, 0, 1) * 2.4
    d[..., 0] -= cracks * 36
    d[..., 1] -= cracks * 38
    d[..., 2] -= cracks * 34
    save(to_rgb([d[..., 0], d[..., 1], d[..., 2]]), "T_Rock_D", "environment")
    save(normal_from_height((1 - wor2) * 0.5 + grain * 0.5 + cracks * 0.6, 3.0),
         "T_Rock_N", "environment", srgb=False)
    # Ruin stone — cut blocks + faint runes
    yy, xx = np.mgrid[0:S[0], 0:S[1]]
    mortar = np.clip(np.abs(np.sin(xx * math.pi / 170)) - 0.96, 0, 1) * 18
    mortar += np.clip(np.abs(np.sin(yy * math.pi / 130)) - 0.965, 0, 1) * 14
    runes = np.clip(fbm(S, 7, 4, 131) - 0.66, 0, 1) * 1.8
    grain2 = fbm(S, 30, 4, 132)
    d = lerp_color((128, 124, 116), (158, 154, 144), grain2)
    d[..., 0] -= mortar + runes * 20
    d[..., 1] -= mortar + runes * 26
    d[..., 2] -= mortar + runes * 12
    d[..., 1] += runes * 70
    d[..., 2] += runes * 44
    save(to_rgb([d[..., 0], d[..., 1], d[..., 2]]), "T_Ruin_D", "environment")
    save(normal_from_height(grain2 * 0.4 + mortar * 0.6 + runes * 0.3, 2.2),
         "T_Ruin_N", "environment", srgb=False)
    # Crystal — faceted
    facet = np.clip(value_noise(S, 6, 140) * 1.5, 0, 1)
    edge = np.clip(ridged(S, 5, 4, 141) - 0.75, 0, 1) * 4
    d = lerp_color((38, 60, 58), (72, 108, 104), facet)
    d[..., 2] += edge * 40
    d[..., 1] += edge * 24
    save(to_rgb([d[..., 0], d[..., 1], d[..., 2]]), "T_Crystal_D", "environment")
    save(normal_from_height(facet * 0.4 + edge * 0.6, 4.0), "T_Crystal_N", "environment", srgb=False)
    e = np.clip(ridged(S, 6, 4, 142) - 0.7, 0, 1) * 3.4
    save(np.dstack([e * 80, e * 235, e * 210]), "T_Crystal_E", "environment", srgb=False)
    # Spore glow dots (canopy emissive accents)
    dots = np.clip(worley(S, 36, 151) - 0.62, 0, 1) * 2.4
    save(np.dstack([dots * 60, dots * 220, dots * 185]),
         "T_Foliage_Spore_E", "environment", srgb=False)


def gen_vehicle():
    S = (1024, 1024)
    plines = np.clip(np.abs(np.sin(np.mgrid[0:S[0], 0:S[1]][1] * math.pi / 140)) - 0.97, 0, 1) * 20
    wear = np.clip(ridged(S, 18, 4, 161) - 0.8, 0, 1) * 3
    smudge = fbm(S, 12, 4, 162)
    d = lerp_color((196, 188, 172), (232, 226, 210), smudge)
    d[..., 0] -= plines + wear * 26
    d[..., 1] -= plines + wear * 24
    d[..., 2] -= plines * 0.7 + wear * 18
    # amber deck stripes
    yy, xx = np.mgrid[0:S[0], 0:S[1]]
    stripe = np.clip(np.sin((yy - xx) * 0.02) - 0.93, 0, 1) * 8
    d[..., 0] += stripe * 60
    d[..., 1] += stripe * 34
    save(to_rgb([d[..., 0], d[..., 1], d[..., 2]]), "T_Vehicle_Hull_D", "vehicle")
    save(normal_from_height(plines * 0.6 + smudge * 0.3 + wear * 0.5, 1.8),
         "T_Vehicle_Hull_N", "vehicle", srgb=False)
    orm = np.dstack([ao_from_height(smudge + plines) * 255,
                     (np.full(S, 0.5) + smudge * 0.18 + wear * 0.22) * 255,
                     (np.full(S, 0.25)) * 255])
    save(orm, "T_Vehicle_Hull_ORM", "vehicle", srgb=False)
    g = np.mgrid[0:512, 0:512][0] / 512
    e = np.clip(1.0 - np.abs(g - 0.5) * 2.2, 0, 1) ** 1.4
    save(np.dstack([e * 60, e * 230, e * 210]), "T_Vehicle_Glow_E", "vehicle", srgb=False)


def gen_fx():
    # 8x8 flipbooks (1024x1024 = 128px cells)
    S = 1024
    N = 8
    cell = S // N
    yy, xx = np.mgrid[0:S, 0:S]
    cu = (xx // cell) + 0.5
    cv = (yy // cell) + 0.5
    t = (cu + cv * 0.35) / (N + N * 0.35)  # diagonal-ish progression
    # smoke: soft blob grows + fades
    px = (xx % cell) / cell - 0.5
    py = (yy % cell) / cell - 0.5
    r = np.sqrt(px ** 2 + py ** 2)
    blob = np.clip(1.0 - r * (2.6 - t * 1.2), 0, 1)
    fade = np.sin(np.clip(t, 0, 1) * math.pi) * 0.9 + 0.1
    swirl = fbm((S, S), 24, 4, 171) * 0.3 + 0.7
    smoke = np.clip(blob * fade * swirl, 0, 1)
    save(np.dstack([smoke * 255] * 3), "T_FX_Smoke_FB", "fx", srgb=False,
         extra={"flipbook": "8x8"})
    # sparks: shrinking bright core + streaks
    core = np.clip(1.0 - r * (7.0 - t * 4.5), 0, 1) ** 2
    streak = np.clip(0.5 - np.abs(py) * 26, 0, 1) * np.clip(0.16 - np.abs(r - t * 0.4), 0, 1) * 3
    spark = np.clip(core + streak * (1 - t * 0.5), 0, 1) * fade
    save(np.dstack([spark * 255, spark * 235, spark * 190]), "T_FX_Sparks_FB", "fx",
         srgb=False, extra={"flipbook": "8x8"})
    # flare (single 512): radial + horizontal anamorphic
    g2y, g2x = np.mgrid[0:512, 0:512]
    fx = (g2x - 256) / 256
    fy = (g2y - 256) / 256
    rr = np.sqrt(fx ** 2 + fy ** 2)
    radial = np.clip(1.0 - rr * 2.6, 0, 1) ** 2
    ana = np.clip(0.5 - np.abs(fy) * 18, 0, 1) * np.clip(0.9 - np.abs(fx) * 0.7, 0, 1)
    flare = np.clip(radial * 0.9 + ana * 0.7, 0, 1)
    save(np.dstack([flare * 255, flare * 250, flare * 235]), "T_FX_Flare_E", "fx", srgb=False)


def main() -> None:
    os.makedirs(OUT, exist_ok=True)
    gen_landscape()
    gen_survivor()
    gen_weapons()
    gen_echo()
    gen_environment()
    gen_vehicle()
    gen_fx()
    files = sorted(f for f in os.listdir(OUT) if f.endswith(".png"))
    total = sum(os.path.getsize(os.path.join(OUT, f)) for f in files)
    print(f"\n[tex] {len(files)} textures, total {total // 1024}KB")


if __name__ == "__main__":
    main()
