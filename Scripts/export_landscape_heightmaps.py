#!/usr/bin/env python3
"""
ASTRAWILD — Shattered Vale landscape heightmap exporter (Batch 7).

Ports the C++ runtime terrain math 1:1 (AAstrawildTerrainTileActor + UAstrawildZoneSubsystem)
into pure Python so the target machine can optionally build high-fidelity editor
Landscape tiles from the SAME height field the runtime world uses.

Outputs one 16-bit RAW heightmap (.r16) per zone into Content/Heightmaps/:
  - Resolution: 505 x 505 (8x8 sections of 63 quads — a valid UE Landscape import size)
  - Mapping: 0..65535 -> -256m..+256m (value 32768 = 0cm), the standard UE r16 import
  - One file per 800m x 800m zone, world-aligned, deterministic per seed

Usage:
  python3 Scripts/export_landscape_heightmaps.py [--seed 1337] [--out Content/Heightmaps]

Editor import (OPTIONAL — the runtime PMC world needs nothing):
  1. Create a Landscape per zone, Section Size 63, Sections Per Side 8 (505 verts).
  2. Select the landscape > Landscape tool > Import from file > pick <zone>.r16.
  3. Scale the landscape actor to 80000 x 80000 cm (X/Y) — Z already maps via r16.

NOTE: The runtime C++ height field is the source of truth. This script mirrors it
best-effort for the optional editor path; run `--selfcheck` to verify the ported
invariants (determinism, partition of unity, seam continuity).
"""

import argparse
import math
import struct
import sys
from pathlib import Path

# ---------------------------------------------------------------------------
# Zone table — MUST match UAstrawildZoneSubsystem::GetAllZones() exactly.
# Bounds in cm on the XY plane: 3 columns x 2 rows of 800m cells.
# ---------------------------------------------------------------------------

ZONES = [
    # (enum, id, name, minX, minY, maxX, maxY, base, amp, ridge, threat)
    ("FrostveilExpanse", "Zone_Frostveil", "Frostveil Expanse", -120000, 0, -40000, 80000, 900.0, 2200.0, 0.8, 3),
    ("Glimmerwood", "Zone_Glimmerwood", "Glimmerwood", -40000, 0, 40000, 80000, 300.0, 900.0, 0.15, 2),
    ("EmberRidge", "Zone_EmberRidge", "Ember Ridge", 40000, 0, 120000, 80000, 500.0, 2600.0, 0.9, 3),
    ("DuskMarsh", "Zone_DuskMarsh", "Dusk Marsh", -120000, -80000, -40000, 0, -60.0, 260.0, 0.0, 2),
    ("DawnFields", "Zone_DawnFields", "Dawn Fields", -40000, -80000, 40000, 0, 220.0, 520.0, 0.0, 1),
    ("HollowApproach", "Zone_HollowApproach", "Hollow Approach", 40000, -80000, 120000, 0, 260.0, 1300.0, 0.5, 4),
]

BASE_NOISE_WAVELENGTH = 51200.0
BASE_NOISE_OCTAVES = 4
MICRO_DETAIL_WAVELENGTH = 9000.0
MICRO_DETAIL_AMPLITUDE = 70.0
ZONE_BLEND_DISTANCE = 6000.0
R16_HALF_RANGE_CM = 51200.0  # r16 maps 0..65535 -> -256m..+256m

MASK32 = 0xFFFFFFFF


def noise_hash(x: int, y: int, seed: int) -> int:
    h = (x * 374761393 + y * 668265263 + seed * 1442695041) & MASK32
    h = ((h ^ (h >> 13)) * 1274126177) & MASK32
    h ^= h >> 16
    return h & MASK32


def smooth_quintic(t: float) -> float:
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0)


def value_noise(px: float, py: float, seed: int) -> float:
    cx0 = math.floor(px)
    cy0 = math.floor(py)
    cx1, cy1 = cx0 + 1, cy0 + 1
    tx = smooth_quintic(px - cx0)
    ty = smooth_quintic(py - cy0)

    def h01(cx: int, cy: int) -> float:
        return noise_hash(int(cx) & 0x7FFFFFFF, int(cy) & 0x7FFFFFFF, seed) / 4294967295.0

    n00 = h01(cx0, cy0)
    n10 = h01(cx1, cy0)
    n01 = h01(cx0, cy1)
    n11 = h01(cx1, cy1)
    return (n00 * (1 - tx) + n10 * tx) * (1 - ty) + (n01 * (1 - tx) + n11 * tx) * ty


def fbm_noise(px: float, py: float, seed: int, octaves: int) -> float:
    total = 0.0
    amplitude = 0.5
    sum_amp = 0.0
    fx, fy = px, py
    for octave in range(octaves):
        total += amplitude * value_noise(fx, fy, (seed + octave * 131) & MASK32)
        sum_amp += amplitude
        amplitude *= 0.5
        fx *= 2.0
        fy *= 2.0
    return total / sum_amp if sum_amp > 0 else 0.0


def eval_base_noise(x: float, y: float, seed: int) -> float:
    return fbm_noise(x / BASE_NOISE_WAVELENGTH, y / BASE_NOISE_WAVELENGTH, seed, BASE_NOISE_OCTAVES) * 2.0 - 1.0


def shape_noise(n: float, ridge: float) -> float:
    ridged = 1.0 - 2.0 * abs(n)
    return n + (ridged - n) * max(0.0, min(1.0, ridge))


def zone_weights(x: float, y: float):
    raw = []
    raw_sum = 0.0
    for z in ZONES:
        dx = max(z[3] - x, x - z[5], 0.0)
        dy = max(z[4] - y, y - z[6], 0.0)
        dist = math.sqrt(dx * dx + dy * dy)
        ratio = dist / ZONE_BLEND_DISTANCE
        w = 1.0 / (1.0 + ratio * ratio)
        raw.append(w)
        raw_sum += w
    if raw_sum <= 1e-8:
        n = len(ZONES)
        return [1.0 / n] * n
    return [w / raw_sum for w in raw]


def eval_world_height(x: float, y: float, seed: int) -> float:
    """World-space terrain height (cm) — mirrors AAstrawildTerrainTileActor::EvalWorldHeight."""
    base = eval_base_noise(x, y, seed)
    weights = zone_weights(x, y)
    height = 0.0
    for z, w in zip(ZONES, weights):
        if w <= 0.0:
            continue
        shaped = shape_noise(base, z[9])  # ridge blend
        height += w * (z[7] + z[8] * shaped)  # base + amplitude * shaped
    height += MICRO_DETAIL_AMPLITUDE * (
        fbm_noise(x / MICRO_DETAIL_WAVELENGTH, y / MICRO_DETAIL_WAVELENGTH, (seed + 7717) & MASK32, 2) * 2.0 - 1.0
    )
    return height


def height_to_r16(h_cm: float) -> int:
    v = int(round((h_cm / R16_HALF_RANGE_CM + 0.5) * 65535.0))
    return max(0, min(65535, v))


def export_zone_r16(zone, seed: int, resolution: int, out_dir: Path) -> Path:
    enum, zid, name, min_x, min_y, max_x, max_y = zone[0], zone[1], zone[2], zone[3], zone[4], zone[5], zone[6]
    size_x = max_x - min_x
    size_y = max_y - min_y
    step_x = size_x / (resolution - 1)
    step_y = size_y / (resolution - 1)

    data = bytearray(resolution * resolution * 2)
    idx = 0
    for j in range(resolution):
        y = min_y + j * step_y
        for i in range(resolution):
            x = min_x + i * step_x
            h = eval_world_height(x, y, seed)
            v = height_to_r16(h)
            data[idx] = v & 0xFF
            data[idx + 1] = (v >> 8) & 0xFF
            idx += 2

    out_path = out_dir / f"{zid}_{resolution}.r16"
    out_path.write_bytes(bytes(data))
    return out_path


def selfcheck(seed: int) -> bool:
    ok = True

    # Determinism.
    h1 = eval_world_height(33750.0, -12250.0, seed)
    h2 = eval_world_height(33750.0, -12250.0, seed)
    if h1 != h2:
        print(f"FAIL determinism: {h1} != {h2}")
        ok = False

    # Partition of unity at tricky points.
    for (x, y) in [(0.0, -40000.0), (40000.0, 0.0), (-120000.0, 80000.0), (400000.0, 400000.0)]:
        w = zone_weights(x, y)
        if any(v < 0.0 for v in w) or abs(sum(w) - 1.0) > 0.001:
            print(f"FAIL partition of unity at ({x},{y}): sum={sum(w)}")
            ok = False

    # Dawn Fields dominates at camp.
    w = zone_weights(0.0, -40000.0)
    dawn_w = w[4]  # ZONES[4] is DawnFields
    if dawn_w <= 0.9:
        print(f"FAIL Dawn Fields dominance at camp: {dawn_w}")
        ok = False

    # Seam continuity across the X=40000 border and Y=0 border.
    for y in (-60000.0, -40000.0, 0.0, 20000.0):
        hw = eval_world_height(39999.0, y, seed)
        he = eval_world_height(40001.0, y, seed)
        if abs(he - hw) >= 50.0:
            print(f"FAIL seam jump at (40000,{y}): {hw} -> {he}")
            ok = False
    for x in (-20000.0, 0.0, 20000.0):
        hs = eval_world_height(x, -1.0, seed)
        hn = eval_world_height(x, 1.0, seed)
        if abs(hn - hs) >= 50.0:
            print(f"FAIL row seam jump at ({x},0): {hs} -> {hn}")
            ok = False

    # Zone personality: marsh low, frostveil high.
    marsh_h = eval_world_height(-80000.0, -40000.0, seed)
    frost_h = eval_world_height(-80000.0, 40000.0, seed)
    if marsh_h >= 600.0:
        print(f"FAIL marsh too high: {marsh_h}")
        ok = False
    if frost_h <= marsh_h:
        print(f"FAIL frostveil not higher than marsh: {frost_h} vs {marsh_h}")
        ok = False

    print(f"Self-check (seed {seed}): {'PASS' if ok else 'FAIL'} "
          f"(camp h={eval_world_height(0.0, -40000.0, seed):.0f}cm, "
          f"marsh h={marsh_h:.0f}cm, frost h={frost_h:.0f}cm)")
    return ok


def main() -> int:
    parser = argparse.ArgumentParser(description="Export Shattered Vale landscape heightmaps (.r16)")
    parser.add_argument("--seed", type=int, default=1337)
    parser.add_argument("--resolution", type=int, default=505)
    parser.add_argument("--out", type=str, default="Content/Heightmaps")
    parser.add_argument("--selfcheck", action="store_true")
    args = parser.parse_args()

    if not selfcheck(args.seed):
        return 1
    if args.selfcheck:
        return 0

    out_dir = Path(args.out)
    out_dir.mkdir(parents=True, exist_ok=True)

    for zone in ZONES:
        path = export_zone_r16(zone, args.seed, args.resolution, out_dir)
        print(f"exported {path.name} ({zone[2]}, {path.stat().st_size} bytes)")

    print(f"\nDone — {len(ZONES)} heightmaps at {args.resolution}x{args.resolution}, seed {args.seed}.")
    print("Import per-zone in the UE editor: Landscape > Section Size 63 > Sections 8x8 > Import from file.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
