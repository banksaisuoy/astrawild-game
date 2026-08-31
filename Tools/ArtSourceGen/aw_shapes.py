"""
ASTRAWILD ArtSourceGen — primitive mesh library.

Convention: +Y is UP, +Z is FORWARD, units = meters. Shapes are authored in
local space; use translate/rotate/scale and MeshBuilder to compose scenes.
All generators MUST build UVs (0..1 per shape face wrap) so PBR textures tile.
"""
from __future__ import annotations

import math
from dataclasses import dataclass, field
from typing import Callable, List, Optional, Sequence, Tuple

import numpy as np


@dataclass
class MeshData:
    positions: np.ndarray   # (N,3) float32
    normals: np.ndarray     # (N,3) float32
    uvs: np.ndarray         # (N,2) float32
    triangles: np.ndarray   # (M,3) int32
    skin_tag: Optional[str] = None   # primary bone hint for auto-skin


# ---------------------------------------------------------------- transforms
def translate(m: MeshData, t: Sequence[float]) -> MeshData:
    t = np.asarray(t, dtype=np.float32)
    return MeshData(m.positions + t, m.normals.copy(), m.uvs.copy(),
                    m.triangles.copy(), m.skin_tag)


def rotate(m: MeshData, rx: float = 0.0, ry: float = 0.0, rz: float = 0.0) -> MeshData:
    """Rotates vertices AND normals. Angles in radians, Euler XYZ."""
    from aw_gltf import quat_to_matrix
    r = quat_to_matrix(_euler_quat(rx, ry, rz))
    return MeshData((m.positions @ r.T).astype(np.float32),
                    (m.normals @ r.T).astype(np.float32),
                    m.uvs.copy(), m.triangles.copy(), m.skin_tag)


def scale(m: MeshData, s: Sequence[float]) -> MeshData:
    s = np.asarray(s, dtype=np.float32)
    inv = np.where(np.abs(s) < 1e-9, 1.0, s)
    return MeshData((m.positions * s).astype(np.float32),
                    (m.normals / inv).astype(np.float32),
                    m.uvs.copy(), m.triangles.copy(), m.skin_tag)


def mirror_x(m: MeshData, flip_tag: Optional[str] = None) -> MeshData:
    """Mirrors across the YZ plane (x -> -x). Fixes winding, keeps normals valid."""
    pos = m.positions.copy()
    pos[:, 0] *= -1
    nor = m.normals.copy()
    nor[:, 0] *= -1
    tri = m.triangles[:, ::-1].copy()
    return MeshData(pos.astype(np.float32), nor.astype(np.float32),
                    m.uvs.copy(), tri, flip_tag or m.skin_tag)


def _euler_quat(rx: float, ry: float, rz: float):
    from aw_gltf import quat_from_euler
    return quat_from_euler(rx, ry, rz)


def merge(meshes: Sequence[MeshData], skin_tag: Optional[str] = None) -> MeshData:
    pos, nor, uv, tri = [], [], [], []
    base = 0
    for m in meshes:
        pos.append(m.positions)
        nor.append(m.normals)
        uv.append(m.uvs)
        tri.append(m.triangles + base)
        base += len(m.positions)
    return MeshData(np.concatenate(pos), np.concatenate(nor),
                    np.concatenate(uv), np.concatenate(tri), skin_tag)


def recompute_smooth_normals(m: MeshData) -> MeshData:
    """Angle-free vertex-normal averaging (positions are welded by exact match)."""
    normals = np.zeros_like(m.positions)
    for a, b, c in m.triangles:
        pa, pb, pc = m.positions[a], m.positions[b], m.positions[c]
        n = np.cross(pb - pa, pc - pa)
        normals[a] += n
        normals[b] += n
        normals[c] += n
    lens = np.linalg.norm(normals, axis=1, keepdims=True)
    lens[lens < 1e-12] = 1.0
    return MeshData(m.positions, (normals / lens).astype(np.float32),
                    m.uvs, m.triangles, m.skin_tag)


def displace(m: MeshData, fn: Callable[[np.ndarray], np.ndarray]) -> MeshData:
    """Displaces vertices along their normals by fn(position) -> (N,) offsets."""
    offsets = np.asarray(fn(m.positions), dtype=np.float32)
    pos = m.positions + m.normals * offsets[:, None]
    return MeshData(pos, m.normals, m.uvs, m.triangles, m.skin_tag)


# ------------------------------------------------------------------- shapes
def box(size: Sequence[float] = (1, 1, 1), uv_scale: float = 1.0) -> MeshData:
    w, h, d = (float(v) * 0.5 for v in size)
    faces = [
        # (+normal, 4 corners CCW seen from outside, u axis, v axis)
        ((1, 0, 0),  [( w, -h, -d), ( w, -h,  d), ( w,  h,  d), ( w,  h, -d)], (0, 0, 1), (0, 1, 0)),
        ((-1, 0, 0), [(-w, -h,  d), (-w, -h, -d), (-w,  h, -d), (-w,  h,  d)], (0, 0, 1), (0, 1, 0)),
        ((0, 1, 0),  [(-w,  h,  d), ( w,  h,  d), ( w,  h, -d), (-w,  h, -d)], (1, 0, 0), (0, 0, 1)),
        ((0, -1, 0), [(-w, -h, -d), ( w, -h, -d), ( w, -h,  d), (-w, -h,  d)], (1, 0, 0), (0, 0, 1)),
        ((0, 0, 1),  [(-w, -h,  d), ( w, -h,  d), ( w,  h,  d), (-w,  h,  d)], (1, 0, 0), (0, 1, 0)),
        ((0, 0, -1), [( w, -h, -d), (-w, -h, -d), (-w,  h, -d), ( w,  h, -d)], (1, 0, 0), (0, 1, 0)),
    ]
    pos: List[np.ndarray] = []
    nor: List[np.ndarray] = []
    uv: List[np.ndarray] = []
    tri: List[np.ndarray] = []
    base = 0
    for normal, corners, uax, vax in faces:
        n = np.array(normal, dtype=np.float32)
        for c in corners:
            pos.append(np.array(c, dtype=np.float32))
            nor.append(n)
        # planar uv from projection onto u/v axes
        ua = np.array(uax, dtype=np.float32)
        va = np.array(vax, dtype=np.float32)
        for c in corners:
            p = np.array(c, dtype=np.float32)
            su = 0.5 + float(np.dot(p, ua)) * uv_scale
            sv = 0.5 + float(np.dot(p, va)) * uv_scale
            uv.append(np.array([su, sv], dtype=np.float32))
        tri.append(np.array([[base, base + 1, base + 2], [base, base + 2, base + 3]], dtype=np.int32))
        base += 4
    return MeshData(np.array(pos), np.array(nor), np.array(uv), np.concatenate(tri))


def lathe(profile: Sequence[Tuple[float, float]], radial: int = 12,
          smooth: bool = True, cap_top: bool = False, cap_bottom: bool = False,
          hard_u_seam: bool = False) -> MeshData:
    """Revolves a 2D profile (r, y) around the Y axis. r=0 allowed (poles).
    UV: u wraps 0..1 around, v follows profile index 0..1."""
    profile = [(float(r), float(y)) for r, y in profile]
    rings = len(profile)
    verts_per_ring = radial if (profile[0][0] > 1e-9 or True) else radial
    pos: List[np.ndarray] = []
    nor: List[np.ndarray] = []
    uv: List[np.ndarray] = []
    tri: List[np.ndarray] = []
    # analytic profile normals
    pn: List[Tuple[float, float]] = []
    for i in range(rings):
        if i == 0:
            dr = profile[1][0] - profile[0][0]
            dy = profile[1][1] - profile[0][1]
        elif i == rings - 1:
            dr = profile[i][0] - profile[i - 1][0]
            dy = profile[i][1] - profile[i - 1][1]
        else:
            dr = profile[i + 1][0] - profile[i - 1][0]
            dy = profile[i + 1][1] - profile[i - 1][1]
        n = np.array([dy, -dr])
        n = n / (np.linalg.norm(n) + 1e-12)
        pn.append((float(n[0]), float(n[1])))
    for i, (r, y) in enumerate(profile):
        if r < 1e-9:
            # pole: single vertex
            pos.append(np.array([0.0, y, 0.0], dtype=np.float32))
            p = pn[i]
            nor.append(np.array([0.0, p[1], 0.0], dtype=np.float32))
            uv.append(np.array([0.5, i / (rings - 1)], dtype=np.float32))
            continue
        nr, ny = pn[i]
        for j in range(radial):
            a = 2.0 * math.pi * j / radial
            ca, sa = math.cos(a), math.sin(a)
            pos.append(np.array([r * ca, y, r * sa], dtype=np.float32))
            if smooth:
                nor.append(np.array([nr * ca, ny, nr * sa], dtype=np.float32))
            else:
                am = 2.0 * math.pi * (j + 0.5) / radial
                nor.append(np.array([nr * math.cos(am), ny, nr * math.sin(am)], dtype=np.float32))
            uv.append(np.array([j / radial, i / (rings - 1)], dtype=np.float32))
    def vidx(i: int, j: int) -> int:
        # index of vertex in ring i at angle slot j
        offset = 0
        for k in range(i):
            offset += 1 if profile[k][0] < 1e-9 else radial
        if profile[i][0] < 1e-9:
            return offset
        return offset + (j % radial)
    for i in range(rings - 1):
        r0, r1 = profile[i][0], profile[i + 1][0]
        for j in range(radial):
            jn = (j + 1) % radial
            if r0 < 1e-9:   # bottom pole -> fan
                a, b = vidx(i, 0), vidx(i + 1, j)
                c = vidx(i + 1, jn)
                tri.append(np.array([a, b, c], dtype=np.int32))
            elif r1 < 1e-9:  # top pole -> fan
                a, b, c = vidx(i, j), vidx(i, jn), vidx(i + 1, 0)
                tri.append(np.array([a, b, c], dtype=np.int32))
            else:
                a, b, c, d = vidx(i, j), vidx(i + 1, j), vidx(i + 1, jn), vidx(i, jn)
                tri.append(np.array([a, b, c], dtype=np.int32))
                tri.append(np.array([a, c, d], dtype=np.int32))
    mesh = MeshData(np.array(pos), np.array(nor), np.array(uv), np.array(tri))
    if cap_top and profile[-1][0] > 1e-9:
        mesh = _cap_annulus(mesh, profile, radial, top=True)
    if cap_bottom and profile[0][0] > 1e-9:
        mesh = _cap_annulus(mesh, profile, radial, top=False)
    return mesh


def _cap_annulus(m: MeshData, profile, radial: int, top: bool) -> MeshData:
    r, y = profile[-1] if top else profile[0]
    center = np.array([0.0, y, 0.0], dtype=np.float32)
    nrm = np.array([0.0, 1.0 if top else -1.0, 0.0], dtype=np.float32)
    verts = [center]
    normals = [nrm]
    uvs = [np.array([0.5, 0.5], dtype=np.float32)]
    for j in range(radial):
        a = 2.0 * math.pi * j / radial
        verts.append(np.array([r * math.cos(a), y, r * math.sin(a)], dtype=np.float32))
        normals.append(nrm)
        uvs.append(np.array([0.5 + 0.5 * math.cos(a), 0.5 + 0.5 * math.sin(a)], dtype=np.float32))
    tris = []
    base = len(m.positions)
    for j in range(radial):
        jn = (j + 1) % radial
        if top:
            tris.append([base, base + 1 + jn, base + 1 + j])
        else:
            tris.append([base, base + 1 + j, base + 1 + jn])
    return MeshData(np.concatenate([m.positions, np.array(verts)]),
                    np.concatenate([m.normals, np.array(normals)]),
                    np.concatenate([m.uvs, np.array(uvs)]),
                    np.concatenate([m.triangles, np.array(tris, dtype=np.int32)]),
                    m.skin_tag)


def cylinder(r: float, h: float, radial: int = 12, capped: bool = True) -> MeshData:
    m = lathe([(r, 0.0), (r, h)], radial, smooth=True)
    if capped:
        m = _cap_annulus(m, [(r, 0.0), (r, h)], radial, top=True)
        m = _cap_annulus(m, [(r, 0.0), (r, h)], radial, top=False)
    return m


def cone(r: float, h: float, radial: int = 10) -> MeshData:
    return lathe([(r, 0.0), (0.0, h)], radial, smooth=True)


def sphere(r: float, seg_u: int = 16, seg_v: int = 12) -> MeshData:
    profile = []
    for i in range(seg_v + 1):
        phi = math.pi * i / seg_v
        profile.append((r * math.sin(phi), r * math.cos(phi)))
    return lathe(profile, seg_u, smooth=True)


def capsule(r: float, length: float, seg_v: int = 8, radial: int = 12) -> MeshData:
    """Total height = length + 2r, centered at origin along Y."""
    profile = []
    quarter = seg_v
    for i in range(quarter + 1):  # bottom hemisphere
        a = math.pi * 0.5 * i / quarter
        profile.append((r * math.sin(a), -length * 0.5 - r * math.cos(a)))
    for i in range(1, quarter + 1):  # top hemisphere
        a = math.pi * 0.5 * i / quarter
        profile.append((r * math.sin(a), length * 0.5 + r * math.cos(a)))
    return lathe(profile, radial, smooth=True)


def crystal(height: float, r: float, sides: int = 6, tip_ratio: float = 0.45) -> MeshData:
    """Hexagonal shard, hard edges. Flat facets — rough sci-fi crystal."""
    body_h = height * (1.0 - tip_ratio)
    profile = [(r * 0.75, 0.0), (r, body_h * 0.35), (r * 0.92, body_h),
               (r * 0.55, body_h + height * tip_ratio * 0.5), (0.0, height)]
    m = lathe(profile, sides, smooth=False)
    return m


def torus(R: float, r: float, seg_u: int = 16, seg_v: int = 8) -> MeshData:
    pos, nor, uv, tri = [], [], [], []
    for i in range(seg_v):
        v = 2.0 * math.pi * i / seg_v
        for j in range(seg_u):
            u = 2.0 * math.pi * j / seg_u
            cx, cy = R + r * math.cos(v), r * math.sin(v)
            p = np.array([cx * math.cos(u), cy, cx * math.sin(u)], dtype=np.float32)
            n = np.array([math.cos(v) * math.cos(u), math.sin(v), math.cos(v) * math.sin(u)], dtype=np.float32)
            pos.append(p)
            nor.append(n)
            uv.append(np.array([j / seg_u, i / seg_v], dtype=np.float32))
    for i in range(seg_v):
        for j in range(seg_u):
            a = i * seg_u + j
            b = i * seg_u + (j + 1) % seg_u
            c = ((i + 1) % seg_v) * seg_u + (j + 1) % seg_u
            d = ((i + 1) % seg_v) * seg_u + j
            tri.append(np.array([a, b, c], dtype=np.int32))
            tri.append(np.array([a, c, d], dtype=np.int32))
    return MeshData(np.array(pos), np.array(nor), np.array(uv), np.array(tri))


def tube(outer_r: float, inner_r: float, h: float, radial: int = 12,
         wall_segs: int = 2) -> MeshData:
    """Annulus prism (barrel shroud / ring) along Y."""
    profile = []
    profile.append((inner_r, 0.0))
    for k in range(wall_segs + 1):
        profile.append((outer_r, h * k / wall_segs))
    profile.append((inner_r, h))
    profile.append((inner_r, 0.0))
    return lathe(profile, radial, smooth=True)


def blade(length: float, width: float, thickness: float,
          curve: float = 0.0, segs: int = 4, double_sided_tip: bool = True) -> MeshData:
    """Leaf/fern blade along +Y, curving on +Z by `curve` (meters at tip)."""
    pos, nor, uv, tri = [], [], [], []
    for i in range(segs + 1):
        t = i / segs
        y = length * t
        z = curve * t * t
        half = width * 0.5 * (1.0 - 0.85 * t * t)
        p0 = np.array([-half, y, z], dtype=np.float32)
        p1 = np.array([half, y, z], dtype=np.float32)
        pos.extend([p0, p1])
        t0 = np.array([0, length, curve * 2 * t], dtype=np.float32)
        tangent = t0 / (np.linalg.norm(t0) + 1e-9)
        n = np.array([0, -tangent[2], tangent[1]], dtype=np.float32)
        nor.extend([n, n])
        uv.extend([np.array([0, t], dtype=np.float32), np.array([1, t], dtype=np.float32)])
    for i in range(segs):
        a, b = i * 2, i * 2 + 1
        c, d = (i + 1) * 2, (i + 1) * 2 + 1
        tri.append(np.array([a, c, d], dtype=np.int32))
        tri.append(np.array([a, d, b], dtype=np.int32))
    # thickness: duplicate with offset
    m = MeshData(np.array(pos), np.array(nor), np.array(uv), np.array(tri))
    if thickness > 1e-6:
        back = MeshData(m.positions.copy(), m.normals.copy() * -1, m.uvs.copy(),
                        m.triangles[:, ::-1].copy())
        back_pos = back.positions.copy()
        back_pos[:, 2] -= thickness
        back = MeshData(back_pos, back.normals, back.uvs, back.triangles)
        return merge([m, back])
    return m


def plate(width: float, depth: float, thickness: float, taper: float = 0.0) -> MeshData:
    """Flat armor plate lying in the XZ plane (Y = thickness). Taper narrows one end."""
    b = box((width, thickness, depth))
    if taper != 0.0:
        pos = b.positions.copy()
        zmax = pos[:, 2].max()
        t = np.clip((pos[:, 2] - zmin(pos)) / max(1e-9, zmax - zmin(pos)), 0, 1) if depth != 0 else 0
        pos[:, 0] *= (1.0 - taper * t)
        b = MeshData(pos, b.normals, b.uvs, b.triangles)
    return b


def zmin(pos: np.ndarray) -> float:
    return float(pos[:, 2].min())


# ---------------------------------------------------------------- mesh sink
class MeshBuilder:
    """Collects MeshData parts, each with a material slot name (and optional
    primary bone tag). `build_primitives()` -> aw_gltf.Primitive list."""

    def __init__(self):
        self.parts: List[Tuple[MeshData, str]] = []

    def add(self, mesh: MeshData, material: str, skin_tag: Optional[str] = None) -> "MeshBuilder":
        tag = skin_tag or mesh.skin_tag
        if tag:
            mesh = MeshData(mesh.positions, mesh.normals, mesh.uvs, mesh.triangles, tag)
        self.parts.append((mesh, material))
        return self

    def triangle_count(self) -> int:
        return int(sum(len(m.triangles) for m, _ in self.parts))

    def build_primitives(self):
        from aw_gltf import Primitive
        prims = []
        for m, mat in self.parts:
            prims.append(Primitive(
                positions=np.ascontiguousarray(m.positions, dtype=np.float32),
                normals=np.ascontiguousarray(m.normals, dtype=np.float32),
                uvs=np.ascontiguousarray(m.uvs, dtype=np.float32),
                indices=np.ascontiguousarray(m.triangles.astype(np.int64)),
                material=mat))
        return prims
