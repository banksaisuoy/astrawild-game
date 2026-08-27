"""Generate original ASTRAWILD low-poly prop meshes as deterministic OBJ files.

The script intentionally uses only the Python standard library, so it can run in
CI/sandbox environments without Blender. The generated OBJ files are source
assets for Editor import, not finished AAA art. If Blender is available later,
these meshes can be remeshed, UV-unwrapped, textured, and re-exported there.

Usage:
    python Scripts/generate_3d_props.py
"""
from __future__ import annotations

import json
import math
from pathlib import Path
from typing import Iterable

ROOT = Path(__file__).resolve().parents[1]
OUTPUT_DIR = ROOT / "Content" / "Astrawild" / "Meshes" / "Props"
SEED = 28427

MATERIALS = {
    "M_Bark_Sunwood": (0.20, 0.07, 0.025),
    "M_Wood_End": (0.38, 0.18, 0.06),
    "M_LumenStone": (0.18, 0.50, 0.65),
    "M_AstraCrystal": (0.35, 0.85, 1.00),
    "M_AstraCore": (0.95, 0.95, 0.40),
    "M_Charcoal": (0.035, 0.03, 0.025),
    "M_Ember": (1.00, 0.22, 0.025),
    "M_PrimalStone": (0.24, 0.27, 0.29),
    "M_PrimalWood": (0.30, 0.12, 0.04),
    "M_Iron": (0.08, 0.10, 0.12),
    "M_WallWood": (0.29, 0.13, 0.045),
}


class MeshBuilder:
    """Minimal indexed OBJ writer with deterministic vertex/face ordering."""

    def __init__(self, name: str) -> None:
        self.name = name
        self.vertices: list[tuple[float, float, float]] = []
        self.faces: list[tuple[tuple[int, ...], str]] = []

    def vertex(self, value: tuple[float, float, float]) -> int:
        self.vertices.append(tuple(round(float(component), 6) for component in value))
        return len(self.vertices)

    def face(self, indices: Iterable[int], material: str) -> None:
        polygon = tuple(indices)
        if len(polygon) >= 3:
            self.faces.append((polygon, material))

    @staticmethod
    def add(a: tuple[float, float, float], b: tuple[float, float, float]) -> tuple[float, float, float]:
        return a[0] + b[0], a[1] + b[1], a[2] + b[2]

    @staticmethod
    def sub(a: tuple[float, float, float], b: tuple[float, float, float]) -> tuple[float, float, float]:
        return a[0] - b[0], a[1] - b[1], a[2] - b[2]

    @staticmethod
    def scale(value: tuple[float, float, float], scalar: float) -> tuple[float, float, float]:
        return value[0] * scalar, value[1] * scalar, value[2] * scalar

    @staticmethod
    def dot(a: tuple[float, float, float], b: tuple[float, float, float]) -> float:
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]

    @staticmethod
    def cross(a: tuple[float, float, float], b: tuple[float, float, float]) -> tuple[float, float, float]:
        return (
            a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0],
        )

    @classmethod
    def normalize(cls, value: tuple[float, float, float]) -> tuple[float, float, float]:
        length = math.sqrt(cls.dot(value, value))
        if length <= 1e-8:
            return 0.0, 0.0, 1.0
        return cls.scale(value, 1.0 / length)

    def add_box(self, center: tuple[float, float, float], size: tuple[float, float, float], material: str) -> None:
        cx, cy, cz = center
        sx, sy, sz = (component / 2.0 for component in size)
        corners = [
            (cx - sx, cy - sy, cz - sz),
            (cx + sx, cy - sy, cz - sz),
            (cx + sx, cy + sy, cz - sz),
            (cx - sx, cy + sy, cz - sz),
            (cx - sx, cy - sy, cz + sz),
            (cx + sx, cy - sy, cz + sz),
            (cx + sx, cy + sy, cz + sz),
            (cx - sx, cy + sy, cz + sz),
        ]
        ids = [self.vertex(corner) for corner in corners]
        for polygon in ((ids[0], ids[3], ids[2], ids[1]), (ids[4], ids[5], ids[6], ids[7]),
                        (ids[0], ids[1], ids[5], ids[4]), (ids[1], ids[2], ids[6], ids[5]),
                        (ids[2], ids[3], ids[7], ids[6]), (ids[3], ids[0], ids[4], ids[7])):
            self.face(polygon, material)

    def add_cylinder_between(
        self,
        start: tuple[float, float, float],
        end: tuple[float, float, float],
        radius: float,
        segments: int,
        side_material: str,
        cap_material: str | None = None,
    ) -> None:
        axis = self.normalize(self.sub(end, start))
        reference = (0.0, 0.0, 1.0) if abs(axis[2]) < 0.85 else (0.0, 1.0, 0.0)
        basis_u = self.normalize(self.cross(axis, reference))
        basis_v = self.normalize(self.cross(axis, basis_u))
        bottom: list[int] = []
        top: list[int] = []
        for index in range(segments):
            angle = 2.0 * math.pi * index / segments
            radial = self.add(self.scale(basis_u, math.cos(angle) * radius), self.scale(basis_v, math.sin(angle) * radius))
            bottom.append(self.vertex(self.add(start, radial)))
            top.append(self.vertex(self.add(end, radial)))
        for index in range(segments):
            next_index = (index + 1) % segments
            self.face((bottom[index], bottom[next_index], top[next_index], top[index]), side_material)
        if cap_material:
            self.face(tuple(reversed(bottom)), cap_material)
            self.face(tuple(top), cap_material)

    def add_uv_sphere(
        self,
        center: tuple[float, float, float],
        radii: tuple[float, float, float],
        rings: int,
        segments: int,
        material: str,
    ) -> None:
        cx, cy, cz = center
        rx, ry, rz = radii
        top = self.vertex((cx, cy, cz + rz))
        ring_ids: list[list[int]] = []
        for ring in range(1, rings):
            phi = math.pi * ring / rings
            current: list[int] = []
            for segment in range(segments):
                theta = 2.0 * math.pi * segment / segments
                current.append(self.vertex((cx + rx * math.sin(phi) * math.cos(theta), cy + ry * math.sin(phi) * math.sin(theta), cz + rz * math.cos(phi))))
            ring_ids.append(current)
        bottom = self.vertex((cx, cy, cz - rz))
        for segment in range(segments):
            next_segment = (segment + 1) % segments
            self.face((top, ring_ids[0][segment], ring_ids[0][next_segment]), material)
        for ring in range(len(ring_ids) - 1):
            current, next_ring = ring_ids[ring], ring_ids[ring + 1]
            for segment in range(segments):
                next_segment = (segment + 1) % segments
                self.face((current[segment], next_ring[segment], next_ring[next_segment], current[next_segment]), material)
        for segment in range(segments):
            next_segment = (segment + 1) % segments
            self.face((ring_ids[-1][segment], bottom, ring_ids[-1][next_segment]), material)

    def add_rock(self, center: tuple[float, float, float], radius: float, height: float, segments: int, material: str, seed_offset: int) -> None:
        # Deterministic pseudo-random radial variation without importing random state.
        rings = ((-0.50, 0.28), (-0.28, 0.72), (0.06, 1.00), (0.32, 0.68), (0.50, 0.22))
        ring_ids: list[list[int]] = []
        for ring_index, (z_factor, radius_factor) in enumerate(rings):
            current: list[int] = []
            for segment in range(segments):
                wobble = 0.90 + 0.10 * math.sin((seed_offset + ring_index * 17 + segment * 31) * 0.73)
                angle = 2.0 * math.pi * segment / segments
                current.append(self.vertex((center[0] + math.cos(angle) * radius * radius_factor * wobble, center[1] + math.sin(angle) * radius * radius_factor * wobble, center[2] + z_factor * height)))
            ring_ids.append(current)
        bottom = self.vertex((center[0], center[1], center[2] - height * 0.56))
        top = self.vertex((center[0], center[1], center[2] + height * 0.56))
        for segment in range(segments):
            next_segment = (segment + 1) % segments
            self.face((bottom, ring_ids[0][next_segment], ring_ids[0][segment]), material)
            self.face((top, ring_ids[-1][segment], ring_ids[-1][next_segment]), material)
        for ring in range(len(ring_ids) - 1):
            current, next_ring = ring_ids[ring], ring_ids[ring + 1]
            for segment in range(segments):
                next_segment = (segment + 1) % segments
                self.face((current[segment], current[next_segment], next_ring[next_segment], next_ring[segment]), material)

    def add_crystal(self, center: tuple[float, float, float], radius: float, height: float, material: str) -> None:
        segments = 6
        rings = ((-0.50, 0.05), (-0.36, 0.68), (0.31, 0.82), (0.50, 0.04))
        ring_ids: list[list[int]] = []
        for z_factor, radius_factor in rings:
            current: list[int] = []
            for segment in range(segments):
                angle = 2.0 * math.pi * segment / segments + math.pi / 6.0
                current.append(self.vertex((center[0] + math.cos(angle) * radius * radius_factor, center[1] + math.sin(angle) * radius * radius_factor, center[2] + z_factor * height)))
            ring_ids.append(current)
        for ring in range(len(ring_ids) - 1):
            for segment in range(segments):
                next_segment = (segment + 1) % segments
                self.face((ring_ids[ring][segment], ring_ids[ring][next_segment], ring_ids[ring + 1][next_segment], ring_ids[ring + 1][segment]), material)

    def write(self, path: Path) -> dict[str, int | str]:
        lines = [f"# ASTRAWILD original generated prop: {self.name}", "mtllib ASTRAWILD_Props.mtl", f"o {self.name}"]
        active_material = None
        for vertex in self.vertices:
            lines.append(f"v {vertex[0]:.6f} {vertex[1]:.6f} {vertex[2]:.6f}")
        for polygon, material in self.faces:
            if material != active_material:
                lines.append(f"usemtl {material}")
                active_material = material
            lines.append("f " + " ".join(str(index) for index in polygon))
        path.write_text("\n".join(lines) + "\n", encoding="utf-8")
        return {"name": self.name, "vertices": len(self.vertices), "faces": len(self.faces), "file": str(path.relative_to(ROOT)).replace("\\", "/")}


def make_sunwood_log() -> MeshBuilder:
    mesh = MeshBuilder("SM_SunwoodLog")
    mesh.add_cylinder_between((-85.0, 0.0, 0.0), (85.0, 0.0, 0.0), 28.0, 12, "M_Bark_Sunwood", "M_Wood_End")
    for x in (-52.0, 0.0, 52.0):
        mesh.add_cylinder_between((x, -0.5, 25.0), (x, -0.5, 30.0), 31.0, 12, "M_Bark_Sunwood")
    return mesh


def make_lumen_rock() -> MeshBuilder:
    mesh = MeshBuilder("SM_LumenRock")
    mesh.add_rock((0.0, 0.0, 34.0), 70.0, 76.0, 9, "M_LumenStone", SEED)
    return mesh


def make_astra_crystal() -> MeshBuilder:
    mesh = MeshBuilder("SM_AstraCrystal")
    mesh.add_crystal((0.0, 0.0, 70.0), 34.0, 150.0, "M_AstraCrystal")
    mesh.add_crystal((0.0, 0.0, 70.0), 12.0, 112.0, "M_AstraCore")
    return mesh


def make_campfire_base() -> MeshBuilder:
    mesh = MeshBuilder("SM_CampfireBase")
    mesh.add_cylinder_between((0.0, 0.0, 4.0), (0.0, 0.0, 14.0), 58.0, 12, "M_Charcoal", "M_Charcoal")
    for angle in (0.0, math.pi / 2.0, math.pi / 4.0, -math.pi / 4.0):
        direction = (math.cos(angle), math.sin(angle), 0.0)
        start = (-direction[0] * 42.0, -direction[1] * 42.0, 25.0)
        end = (direction[0] * 42.0, direction[1] * 42.0, 25.0)
        mesh.add_cylinder_between(start, end, 10.0, 8, "M_Wood_End", "M_Charcoal")
    mesh.add_crystal((0.0, 0.0, 40.0), 20.0, 34.0, "M_Ember")
    return mesh


def make_primal_axe() -> MeshBuilder:
    mesh = MeshBuilder("SM_PrimalAxe")
    mesh.add_cylinder_between((0.0, 0.0, -70.0), (0.0, 0.0, 70.0), 8.0, 8, "M_PrimalWood", "M_PrimalWood")
    mesh.add_box((20.0, 0.0, 55.0), (42.0, 12.0, 58.0), "M_PrimalStone")
    mesh.add_box((5.0, 0.0, 53.0), (24.0, 17.0, 70.0), "M_PrimalStone")
    return mesh


def make_primal_pick() -> MeshBuilder:
    mesh = MeshBuilder("SM_PrimalPick")
    mesh.add_cylinder_between((0.0, 0.0, -70.0), (0.0, 0.0, 65.0), 8.0, 8, "M_PrimalWood", "M_PrimalWood")
    mesh.add_cylinder_between((-38.0, 0.0, 65.0), (38.0, 0.0, 65.0), 10.0, 8, "M_PrimalStone", "M_PrimalStone")
    mesh.add_crystal((-38.0, 0.0, 65.0), 14.0, 55.0, "M_PrimalStone")
    mesh.add_crystal((38.0, 0.0, 65.0), 14.0, 55.0, "M_PrimalStone")
    return mesh


def make_resonator() -> MeshBuilder:
    mesh = MeshBuilder("SM_AstraResonator")
    mesh.add_uv_sphere((0.0, 0.0, 0.0), (42.0, 42.0, 42.0), 8, 12, "M_AstraCore")
    for axis in ("x", "y"):
        if axis == "x":
            mesh.add_cylinder_between((-55.0, 0.0, 0.0), (55.0, 0.0, 0.0), 4.0, 12, "M_Iron", "M_Iron")
        else:
            mesh.add_cylinder_between((0.0, -55.0, 0.0), (0.0, 55.0, 0.0), 4.0, 12, "M_Iron", "M_Iron")
    return mesh


def make_wall() -> MeshBuilder:
    mesh = MeshBuilder("SM_Wall_Wood")
    mesh.add_box((0.0, 0.0, 120.0), (240.0, 24.0, 240.0), "M_WallWood")
    for x in (-90.0, 0.0, 90.0):
        mesh.add_box((x, -15.0, 120.0), (10.0, 10.0, 220.0), "M_PrimalWood")
    mesh.add_box((0.0, -16.0, 38.0), (210.0, 8.0, 10.0), "M_PrimalWood")
    mesh.add_box((0.0, -16.0, 202.0), (210.0, 8.0, 10.0), "M_PrimalWood")
    return mesh


def make_door() -> MeshBuilder:
    mesh = MeshBuilder("SM_Door_Wood")
    mesh.add_box((0.0, 0.0, 112.0), (174.0, 20.0, 224.0), "M_WallWood")
    for x in (-62.0, 0.0, 62.0):
        mesh.add_box((x, -14.0, 112.0), (9.0, 8.0, 210.0), "M_PrimalWood")
    mesh.add_box((0.0, -17.0, 112.0), (160.0, 8.0, 10.0), "M_Iron")
    mesh.add_box((0.0, -17.0, 38.0), (160.0, 8.0, 8.0), "M_Iron")
    return mesh


def write_mtl() -> None:
    lines = ["# ASTRAWILD original generated prop materials"]
    for name, color in MATERIALS.items():
        lines.extend([f"newmtl {name}", f"Kd {color[0]:.4f} {color[1]:.4f} {color[2]:.4f}", "Ns 24.0", ""])
    (OUTPUT_DIR / "ASTRAWILD_Props.mtl").write_text("\n".join(lines), encoding="utf-8")


def generate() -> dict:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    write_mtl()
    makers = (
        make_sunwood_log,
        make_lumen_rock,
        make_astra_crystal,
        make_campfire_base,
        make_primal_axe,
        make_primal_pick,
        make_resonator,
        make_wall,
        make_door,
    )
    assets = []
    for maker in makers:
        mesh = maker()
        assets.append(mesh.write(OUTPUT_DIR / f"{mesh.name}.obj"))
    manifest = {
        "generator": "Scripts/generate_3d_props.py",
        "generator_version": 1,
        "seed": SEED,
        "format": "OBJ + shared MTL",
        "coordinate_units": "centimeters",
        "originality": "ASTRAWILD procedural primitives; no external game assets",
        "assets": assets,
    }
    manifest_path = OUTPUT_DIR / "ASTRAWILD_Props_Manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    print(f"Generated {len(assets)} OBJ props in {OUTPUT_DIR}")
    for asset in assets:
        print(f"- {asset['name']}: {asset['vertices']} vertices, {asset['faces']} faces")
    print(f"Manifest: {manifest_path}")
    return manifest


if __name__ == "__main__":
    generate()
