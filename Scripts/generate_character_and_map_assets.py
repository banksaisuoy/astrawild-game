"""Generate original ASTRAWILD source meshes for every Echo, player and compact-map kit.

The output is intentionally low-poly static source geometry for Editor import. It is
not a replacement for production skeletal meshes, UVs, rigs, materials or animation.
"""
from __future__ import annotations

import csv
import hashlib
import json
import math
import re
from pathlib import Path
from typing import Iterable

ROOT = Path(__file__).resolve().parents[1]
ECHO_CSV = ROOT / "Content/Astrawild/Data/Source/DT_EchoDex_200.csv"
LEGACY_ECHO_CSV = ROOT / "Content/Astrawild/Data/Source/DT_EchoDex.csv"
ECHO_OUTPUT = ROOT / "Content/Astrawild/Meshes/Echoes"
CHARACTER_OUTPUT = ROOT / "Content/Astrawild/Meshes/Characters"
MAP_OUTPUT = ROOT / "Content/Astrawild/Meshes/MapKit"
SEED_SALT = "ASTRAWILD-original-echo-source-v1"

ELEMENT_MATERIAL = {
    "Solar": "M_Echo_Solar",
    "Torrent": "M_Echo_Torrent",
    "Geo": "M_Echo_Geo",
    "Aether": "M_Echo_Aether",
    "Volt": "M_Echo_Volt",
    "Glacial": "M_Echo_Glacial",
    "Abyssal": "M_Echo_Abyssal",
    "Astra": "M_Echo_Astra",
    "Neutral": "M_Echo_Neutral",
}
MATERIALS = {
    "M_Echo_Solar": (0.95, 0.42, 0.08, 1.0),
    "M_Echo_Torrent": (0.08, 0.62, 0.85, 1.0),
    "M_Echo_Geo": (0.35, 0.52, 0.16, 1.0),
    "M_Echo_Aether": (0.55, 0.34, 0.88, 1.0),
    "M_Echo_Volt": (0.82, 0.88, 0.16, 1.0),
    "M_Echo_Glacial": (0.42, 0.82, 0.92, 1.0),
    "M_Echo_Abyssal": (0.16, 0.09, 0.28, 1.0),
    "M_Echo_Astra": (0.84, 0.95, 1.0, 1.0),
    "M_Echo_Neutral": (0.48, 0.42, 0.30, 1.0),
    "M_Echo_Accent": (0.96, 0.88, 0.42, 1.0),
    "M_Echo_Soft": (0.62, 0.48, 0.34, 1.0),
    "M_Player_Suit": (0.12, 0.18, 0.23, 1.0),
    "M_Player_Lumen": (0.80, 0.92, 0.96, 1.0),
    "M_Map_Stone": (0.28, 0.30, 0.31, 1.0),
    "M_Map_Wood": (0.38, 0.18, 0.07, 1.0),
    "M_Map_Glow": (0.55, 0.90, 0.90, 1.0),
    "M_Map_Soil": (0.20, 0.13, 0.08, 1.0),
}


def stable_seed(text: str) -> int:
    return int.from_bytes(hashlib.sha256(f"{SEED_SALT}:{text}".encode()).digest()[:4], "big")


def clean_name(value: str) -> str:
    value = value.split(".")[-1]
    value = re.sub(r"[^A-Za-z0-9]+", "_", value).strip("_")
    return value or "Unknown"


class MeshBuilder:
    def __init__(self, name: str) -> None:
        self.name = name
        self.vertices: list[tuple[float, float, float]] = []
        self.faces: list[tuple[tuple[int, ...], str]] = []

    def vertex(self, value: tuple[float, float, float]) -> int:
        self.vertices.append(tuple(round(float(c), 5) for c in value))
        return len(self.vertices)

    def face(self, indices: Iterable[int], material: str) -> None:
        polygon = tuple(indices)
        if len(polygon) >= 3:
            self.faces.append((polygon, material))

    @staticmethod
    def sub(a: tuple[float, float, float], b: tuple[float, float, float]) -> tuple[float, float, float]:
        return a[0] - b[0], a[1] - b[1], a[2] - b[2]

    @staticmethod
    def add(a: tuple[float, float, float], b: tuple[float, float, float]) -> tuple[float, float, float]:
        return a[0] + b[0], a[1] + b[1], a[2] + b[2]

    @staticmethod
    def scale(a: tuple[float, float, float], value: float) -> tuple[float, float, float]:
        return a[0] * value, a[1] * value, a[2] * value

    @staticmethod
    def dot(a: tuple[float, float, float], b: tuple[float, float, float]) -> float:
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]

    @classmethod
    def normalize(cls, a: tuple[float, float, float]) -> tuple[float, float, float]:
        length = math.sqrt(cls.dot(a, a))
        return cls.scale(a, 1.0 / length) if length > 1e-8 else (0.0, 0.0, 1.0)

    @classmethod
    def cross(cls, a: tuple[float, float, float], b: tuple[float, float, float]) -> tuple[float, float, float]:
        return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])

    def add_ellipsoid(self, center: tuple[float, float, float], radii: tuple[float, float, float], material: str, rings: int = 5, segments: int = 8) -> None:
        top = self.vertex((center[0], center[1], center[2] + radii[2]))
        ring_ids: list[list[int]] = []
        for ring in range(1, rings):
            phi = math.pi * ring / rings
            current = []
            for segment in range(segments):
                theta = 2.0 * math.pi * segment / segments
                current.append(self.vertex((center[0] + radii[0] * math.sin(phi) * math.cos(theta), center[1] + radii[1] * math.sin(phi) * math.sin(theta), center[2] + radii[2] * math.cos(phi))))
            ring_ids.append(current)
        bottom = self.vertex((center[0], center[1], center[2] - radii[2]))
        for segment in range(segments):
            nxt = (segment + 1) % segments
            self.face((top, ring_ids[0][segment], ring_ids[0][nxt]), material)
        for ring in range(len(ring_ids) - 1):
            current, nxt_ring = ring_ids[ring], ring_ids[ring + 1]
            for segment in range(segments):
                nxt = (segment + 1) % segments
                self.face((current[segment], nxt_ring[segment], nxt_ring[nxt], current[nxt]), material)
        for segment in range(segments):
            nxt = (segment + 1) % segments
            self.face((ring_ids[-1][segment], bottom, ring_ids[-1][nxt]), material)

    def add_rod(self, start: tuple[float, float, float], end: tuple[float, float, float], radius: float, material: str, segments: int = 6) -> None:
        axis = self.normalize(self.sub(end, start))
        reference = (0.0, 0.0, 1.0) if abs(axis[2]) < 0.8 else (0.0, 1.0, 0.0)
        u = self.normalize(self.cross(axis, reference))
        v = self.normalize(self.cross(axis, u))
        bottom: list[int] = []
        top: list[int] = []
        for index in range(segments):
            angle = 2.0 * math.pi * index / segments
            radial = self.add(self.scale(u, math.cos(angle) * radius), self.scale(v, math.sin(angle) * radius))
            bottom.append(self.vertex(self.add(start, radial)))
            top.append(self.vertex(self.add(end, radial)))
        for index in range(segments):
            nxt = (index + 1) % segments
            self.face((bottom[index], bottom[nxt], top[nxt], top[index]), material)
        self.face(tuple(reversed(bottom)), material)
        self.face(tuple(top), material)

    def add_spike(self, base: tuple[float, float, float], tip: tuple[float, float, float], radius: float, material: str, segments: int = 5) -> None:
        axis = self.normalize(self.sub(tip, base))
        reference = (0.0, 0.0, 1.0) if abs(axis[2]) < 0.8 else (0.0, 1.0, 0.0)
        u = self.normalize(self.cross(axis, reference))
        v = self.normalize(self.cross(axis, u))
        base_ids = []
        for index in range(segments):
            angle = 2.0 * math.pi * index / segments
            radial = self.add(self.scale(u, math.cos(angle) * radius), self.scale(v, math.sin(angle) * radius))
            base_ids.append(self.vertex(self.add(base, radial)))
        tip_id = self.vertex(tip)
        for index in range(segments):
            self.face((base_ids[index], base_ids[(index + 1) % segments], tip_id), material)
        self.face(tuple(reversed(base_ids)), material)

    def add_box(self, center: tuple[float, float, float], size: tuple[float, float, float], material: str) -> None:
        cx, cy, cz = center
        sx, sy, sz = (value / 2.0 for value in size)
        corners = [(cx - sx, cy - sy, cz - sz), (cx + sx, cy - sy, cz - sz), (cx + sx, cy + sy, cz - sz), (cx - sx, cy + sy, cz - sz), (cx - sx, cy - sy, cz + sz), (cx + sx, cy - sy, cz + sz), (cx + sx, cy + sy, cz + sz), (cx - sx, cy + sy, cz + sz)]
        ids = [self.vertex(corner) for corner in corners]
        for polygon in ((ids[0], ids[3], ids[2], ids[1]), (ids[4], ids[5], ids[6], ids[7]), (ids[0], ids[1], ids[5], ids[4]), (ids[1], ids[2], ids[6], ids[5]), (ids[2], ids[3], ids[7], ids[6]), (ids[3], ids[0], ids[4], ids[7])):
            self.face(polygon, material)

    def write(self, path: Path, mtllib: str) -> dict[str, int | str]:
        lines = [f"# ASTRAWILD original generated source mesh: {self.name}", f"mtllib {mtllib}", f"o {self.name}"]
        active_material = None
        lines.extend(f"v {x:.5f} {y:.5f} {z:.5f}" for x, y, z in self.vertices)
        for polygon, material in self.faces:
            if material != active_material:
                lines.append(f"usemtl {material}")
                active_material = material
            lines.append("f " + " ".join(str(index) for index in polygon))
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")
        return {"file": str(path.relative_to(ROOT)).replace("\\", "/"), "vertices": len(self.vertices), "faces": len(self.faces)}


def write_mtl(path: Path, materials: Iterable[str]) -> None:
    lines: list[str] = ["# ASTRAWILD original generated material palette"]
    for material in sorted(set(materials)):
        r, g, b, a = MATERIALS[material]
        lines.extend([f"newmtl {material}", f"Kd {r:.4f} {g:.4f} {b:.4f}", f"d {a:.4f}", "illum 2", ""])
    path.write_text("\n".join(lines), encoding="utf-8")


def make_echo_mesh(row: dict[str, str]) -> tuple[MeshBuilder, list[str]]:
    tag = row.get("SpeciesTag", "Echo.Unknown")
    element = row.get("PrimaryElement", "Neutral")
    role = row.get("Role", "Combat")
    seed = stable_seed(tag)
    rng = lambda index: ((seed >> (index * 5 % 24)) & 255) / 255.0
    mesh = MeshBuilder(f"SM_Echo_{clean_name(tag)}_Source")
    body_radius = 42.0 + rng(1) * 28.0
    body_height = 72.0 + rng(2) * 62.0
    body_material = ELEMENT_MATERIAL.get(element, "M_Echo_Neutral")
    mesh.add_ellipsoid((0.0, 0.0, body_height), (body_radius * (0.9 + rng(3) * 0.35), body_radius * (0.75 + rng(4) * 0.35), body_height), body_material)
    mesh.add_ellipsoid((body_radius * 0.35, 0.0, body_height * 1.55), (body_radius * 0.48, body_radius * 0.44, body_radius * 0.50), "M_Echo_Soft")
    limb_count = 2 + int(rng(5) * 3.0)
    for limb in range(limb_count):
        angle = 2.0 * math.pi * limb / limb_count + rng(6) * 0.30
        side = (math.cos(angle), math.sin(angle), 0.0)
        start = (side[0] * body_radius * 0.38, side[1] * body_radius * 0.38, body_height * (0.82 if limb % 2 == 0 else 0.55))
        end = (side[0] * (body_radius * (1.25 + rng(limb + 7) * 0.55)), side[1] * (body_radius * (1.25 + rng(limb + 8) * 0.55)), body_height * (0.20 + rng(limb + 9) * 0.45))
        mesh.add_rod(start, end, max(6.0, body_radius * 0.16), "M_Echo_Soft")
    spike_count = 2 + int(rng(10) * 4.0)
    for spike in range(spike_count):
        angle = 2.0 * math.pi * spike / spike_count + rng(spike + 11) * 0.4
        base = (math.cos(angle) * body_radius * 0.55, math.sin(angle) * body_radius * 0.55, body_height * (1.45 + rng(spike + 12) * 0.35))
        tip = (base[0] * (1.08 + rng(spike + 13) * 0.2), base[1] * (1.08 + rng(spike + 14) * 0.2), base[2] + body_height * (0.35 + rng(spike + 15) * 0.42))
        mesh.add_spike(base, tip, max(5.0, body_radius * 0.16), "M_Echo_Accent")
    if role in {"Mount", "Combat", "Exploration"}:
        mesh.add_rod((body_radius * 0.6, 0.0, body_height * 0.9), (body_radius * (1.5 + rng(16)), 0.0, body_height * 0.75), max(5.0, body_radius * 0.12), "M_Echo_Accent")
    mesh.add_spike((body_radius * 0.28, 0.0, body_height * 1.55), (body_radius * 0.65, 0.0, body_height * 1.75), max(5.0, body_radius * 0.10), "M_Echo_Accent")
    materials = [body_material, "M_Echo_Soft", "M_Echo_Accent"]
    return mesh, materials


def make_player_mesh() -> tuple[MeshBuilder, list[str]]:
    mesh = MeshBuilder("SM_Player_AstralSurveyor_Source")
    mesh.add_ellipsoid((0.0, 0.0, 96.0), (32.0, 24.0, 58.0), "M_Player_Suit")
    mesh.add_ellipsoid((0.0, 0.0, 168.0), (26.0, 23.0, 29.0), "M_Player_Suit")
    for side in (-1.0, 1.0):
        mesh.add_rod((side * 22.0, 0.0, 125.0), (side * 54.0, 0.0, 68.0), 10.0, "M_Player_Suit")
        mesh.add_rod((side * 15.0, 0.0, 60.0), (side * 19.0, 0.0, 8.0), 12.0, "M_Player_Suit")
    mesh.add_ellipsoid((0.0, -23.0, 102.0), (10.0, 4.0, 16.0), "M_Player_Lumen")
    return mesh, ["M_Player_Suit", "M_Player_Lumen"]


def make_alpha_mesh() -> tuple[MeshBuilder, list[str]]:
    mesh = MeshBuilder("SM_Alpha_Solarix_Source")
    mesh.add_ellipsoid((0.0, 0.0, 210.0), (120.0, 92.0, 150.0), "M_Echo_Solar")
    mesh.add_ellipsoid((64.0, 0.0, 330.0), (70.0, 56.0, 76.0), "M_Echo_Accent")
    for side in (-1.0, 1.0):
        mesh.add_rod((side * 74.0, 0.0, 235.0), (side * 185.0, 0.0, 125.0), 24.0, "M_Echo_Soft")
        mesh.add_spike((side * 62.0, 0.0, 300.0), (side * 180.0, 0.0, 390.0), 28.0, "M_Echo_Accent")
    for index in range(8):
        angle = 2.0 * math.pi * index / 8.0
        mesh.add_spike((math.cos(angle) * 94.0, math.sin(angle) * 70.0, 250.0), (math.cos(angle) * 175.0, math.sin(angle) * 135.0, 330.0 + (index % 3) * 35.0), 18.0, "M_Echo_Accent")
    return mesh, ["M_Echo_Solar", "M_Echo_Accent", "M_Echo_Soft"]


def make_map_mesh(name: str, zone: str, index: int) -> tuple[MeshBuilder, list[str]]:
    mesh = MeshBuilder(name)
    if zone == "DawnSpire":
        mesh.add_box((0.0, 0.0, 12.0), (220.0, 220.0, 24.0), "M_Map_Stone")
        mesh.add_rod((0.0, 0.0, 24.0), (0.0, 0.0, 220.0), 34.0, "M_Map_Stone", 8)
        mesh.add_spike((0.0, 0.0, 200.0), (0.0, 0.0, 330.0), 34.0, "M_Map_Glow")
    elif zone == "ResourceGrove":
        for offset in (-90.0, 0.0, 90.0):
            mesh.add_rod((offset, -35.0, 0.0), (offset + 14.0, 32.0, 160.0), 17.0, "M_Map_Wood", 7)
            mesh.add_ellipsoid((offset + 18.0, 42.0, 175.0), (48.0, 42.0, 24.0), "M_Map_Glow")
        mesh.add_ellipsoid((0.0, 0.0, 24.0), (110.0, 86.0, 34.0), "M_Map_Soil")
    elif zone == "RestSanctuary":
        mesh.add_box((0.0, 0.0, 10.0), (260.0, 210.0, 20.0), "M_Map_Wood")
        mesh.add_box((0.0, 0.0, 60.0), (90.0, 45.0, 90.0), "M_Map_Wood")
        mesh.add_ellipsoid((85.0, 35.0, 15.0), (44.0, 35.0, 10.0), "M_Map_Glow")
    else:
        mesh.add_ellipsoid((0.0, 0.0, 10.0), (160.0, 140.0, 18.0), "M_Map_Soil")
        for index in range(5):
            angle = 2.0 * math.pi * index / 5.0
            mesh.add_rod((math.cos(angle) * 105.0, math.sin(angle) * 85.0, 5.0), (math.cos(angle) * 125.0, math.sin(angle) * 100.0, 105.0), 18.0, "M_Map_Stone", 7)
        mesh.add_spike((0.0, 0.0, 30.0), (0.0, 0.0, 135.0), 22.0, "M_Map_Glow")
    return mesh, ["M_Map_Stone", "M_Map_Wood", "M_Map_Glow", "M_Map_Soil"]


def load_rows(path: Path) -> list[dict[str, str]]:
    with path.open(encoding="utf-8-sig", newline="") as handle:
        return list(csv.DictReader(handle))


def main() -> int:
    for directory in (ECHO_OUTPUT, CHARACTER_OUTPUT, MAP_OUTPUT):
        directory.mkdir(parents=True, exist_ok=True)
    echo_rows: dict[str, dict[str, str]] = {}
    for path in (LEGACY_ECHO_CSV, ECHO_CSV):
        for row in load_rows(path):
            tag = row.get("SpeciesTag", "")
            if tag:
                echo_rows[tag] = row

    manifest = {"schema": "ASTRAWILD.EchoSourceMeshes.v1", "seed_salt": SEED_SALT, "assets": []}
    all_materials: set[str] = set()
    for tag, row in sorted(echo_rows.items()):
        mesh, materials = make_echo_mesh(row)
        path = ECHO_OUTPUT / f"SM_Echo_{clean_name(tag)}_Source.obj"
        stats = mesh.write(path, "ASTRAWILD_Echoes.mtl")
        all_materials.update(materials)
        manifest["assets"].append({"kind": "EchoSourceMesh", "source_csv": "DT_EchoDex_200.csv" if row.get("Name", "").startswith("MasterEcho_") else "DT_EchoDex.csv", "row_name": row.get("Name", ""), "species_tag": tag, "species_name": row.get("SpeciesName", ""), "primary_element": row.get("PrimaryElement", "Neutral"), "role": row.get("Role", "Combat"), "seed": stable_seed(tag), "placeholder_static_mesh": True, **stats})

    special = [("Player_AstralSurveyor", make_player_mesh, CHARACTER_OUTPUT), ("Alpha_Solarix", make_alpha_mesh, CHARACTER_OUTPUT)]
    for asset_name, factory, output in special:
        mesh, materials = factory()
        stats = mesh.write(output / f"SM_{asset_name}_Source.obj", "ASTRAWILD_Characters.mtl")
        all_materials.update(materials)
        manifest["assets"].append({"kind": "CharacterSourceMesh", "asset_id": asset_name, "placeholder_static_mesh": True, **stats})

    map_manifest = {"schema": "ASTRAWILD.CompactMapKit.v1", "assets": []}
    for index, zone in enumerate(("DawnSpire", "ResourceGrove", "RestSanctuary", "DangerPit")):
        mesh, materials = make_map_mesh(f"SM_{zone}_Kit_Source", zone, index)
        stats = mesh.write(MAP_OUTPUT / f"SM_{zone}_Kit_Source.obj", "ASTRAWILD_MapKit.mtl")
        all_materials.update(materials)
        map_manifest["assets"].append({"kind": "MapKitSourceMesh", "zone": zone, "placeholder_static_mesh": True, **stats})

    write_mtl(ECHO_OUTPUT / "ASTRAWILD_Echoes.mtl", [name for name in all_materials if name.startswith("M_Echo_")])
    write_mtl(CHARACTER_OUTPUT / "ASTRAWILD_Characters.mtl", [name for name in all_materials if name.startswith("M_Player_") or name.startswith("M_Echo_")])
    write_mtl(MAP_OUTPUT / "ASTRAWILD_MapKit.mtl", [name for name in all_materials if name.startswith("M_Map_")])
    (ECHO_OUTPUT / "ASTRAWILD_EchoSource_Manifest.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    (MAP_OUTPUT / "ASTRAWILD_MapKit_Manifest.json").write_text(json.dumps(map_manifest, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print(f"Generated {len(echo_rows)} Echo source meshes, 2 character source meshes, and {len(map_manifest['assets'])} map-kit meshes.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
