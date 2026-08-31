"""
ASTRAWILD ArtSourceGen — glTF 2.0 / GLB writer with skeletal skinning + animations.

CONVENTIONS (project-wide, every generator MUST use these):
  * Units: meters. Survivor ~1.8m tall.
  * Axis: +Y up, +Z forward (glTF standard). UE Interchange auto-converts to Z-up/cm.
  * Quaternions in Python API: (w, x, y, z). Serialized to glTF as [x, y, z, w].
  * One GLB per rig: skeleton nodes (bind pose) + skinned mesh + N animation clips.
  * Material names are SLOT contracts consumed by Content/Python/AwPipeline
    (import_all.py maps them onto real PBR master materials).

Spec refs: glTF 2.0 (Khronos) — skins, animation samplers, GLB container layout.
"""
from __future__ import annotations

import json
import math
import struct
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Sequence, Tuple

import numpy as np

GLB_MAGIC = 0x46546C67
CHUNK_JSON = 0x4E4F534A
CHUNK_BIN = 0x004E4942

COMPONENT_FLOAT = 5126
COMPONENT_UINT16 = 5123
COMPONENT_UINT8 = 5121
COMPONENT_UINT32 = 5125

ATTR_TYPE_COUNT = {"SCALAR": 1, "VEC2": 2, "VEC3": 3, "VEC4": 4, "MAT4": 16}


def quat_normalize(q: Sequence[float]) -> np.ndarray:
    q = np.asarray(q, dtype=np.float64)
    n = np.linalg.norm(q)
    if n < 1e-12:
        return np.array([1.0, 0.0, 0.0, 0.0])
    return q / n


def quat_from_axis_angle(axis: Sequence[float], angle_rad: float) -> np.ndarray:
    """Returns (w, x, y, z)."""
    axis = np.asarray(axis, dtype=np.float64)
    n = np.linalg.norm(axis)
    if n < 1e-12:
        return np.array([1.0, 0.0, 0.0, 0.0])
    axis = axis / n
    half = angle_rad * 0.5
    s = math.sin(half)
    return np.array([math.cos(half), axis[0] * s, axis[1] * s, axis[2] * s])


def quat_from_euler(rx: float, ry: float, rz: float) -> np.ndarray:
    """Euler XYZ (radians) -> (w, x, y, z)."""
    cx, sx = math.cos(rx * 0.5), math.sin(rx * 0.5)
    cy, sy = math.cos(ry * 0.5), math.sin(ry * 0.5)
    cz, sz = math.cos(rz * 0.5), math.sin(rz * 0.5)
    # R = Rx * Ry * Rz
    w = cx * cy * cz + sx * sy * sz
    x = sx * cy * cz - cx * sy * sz
    y = cx * sy * cz + sx * cy * sz
    z = cx * cy * sz - sx * sy * cz
    return quat_normalize([w, x, y, z])


def quat_mul(a: Sequence[float], b: Sequence[float]) -> np.ndarray:
    """(w,x,y,z) quaternion product a*b."""
    aw, ax, ay, az = a
    bw, bx, by, bz = b
    return np.array([
        aw * bw - ax * bx - ay * by - az * bz,
        aw * bx + ax * bw + ay * bz - az * by,
        aw * by - ax * bz + ay * bw + az * bx,
        aw * bz + ax * by - ay * bx + az * bw,
    ])


def quat_to_matrix(q: Sequence[float]) -> np.ndarray:
    """(w,x,y,z) -> 3x3 rotation matrix (row-major math, column-vector convention)."""
    w, x, y, z = quat_normalize(q)
    return np.array([
        [1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y)],
        [2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x)],
        [2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y)],
    ])


def trs_matrix(t: Sequence[float], q: Optional[Sequence[float]], s: Sequence[float]) -> np.ndarray:
    """Local TRS -> 4x4 (column-vector convention, numpy row-storage)."""
    m = np.eye(4)
    if q is not None:
        m[:3, :3] = quat_to_matrix(q)
    m[0, 0] *= s[0]
    m[1, 1] *= s[1]
    m[2, 2] *= s[2]
    m[:3, 3] = t
    return m


@dataclass
class Material:
    name: str
    base_color: Tuple[float, float, float, float] = (0.8, 0.8, 0.8, 1.0)
    metallic: float = 0.0
    roughness: float = 0.8
    emissive: Tuple[float, float, float] = (0.0, 0.0, 0.0)
    emissive_strength: float = 1.0
    double_sided: bool = False


@dataclass
class Primitive:
    positions: np.ndarray            # (N,3) float32
    normals: np.ndarray              # (N,3) float32
    uvs: Optional[np.ndarray]        # (N,2) float32
    indices: np.ndarray              # (M,3) int
    material: str
    joints: Optional[np.ndarray] = None   # (N,4) uint8  (indices into skin.joints)
    weights: Optional[np.ndarray] = None  # (N,4) float32 (sum to 1)


@dataclass
class Node:
    name: str
    parent: int = -1
    translation: Tuple[float, float, float] = (0.0, 0.0, 0.0)
    rotation: Optional[np.ndarray] = None   # (w,x,y,z)
    scale: Tuple[float, float, float] = (1.0, 1.0, 1.0)
    primitives: Optional[List[Primitive]] = None
    bone_tag: str = ""  # informational: skeleton bone name (for socket scripts)


@dataclass
class Skin:
    joints: List[int]                          # node indices
    inverse_bind_matrices: np.ndarray          # (J,4,4) float32 column-major storage
    skeleton_root: int = -1


@dataclass
class AnimChannel:
    node: int
    path: str            # "translation" | "rotation" | "scale"
    times: np.ndarray    # (K,) float32
    values: np.ndarray   # (K,3) or (K,4)


@dataclass
class Animation:
    name: str
    channels: List[AnimChannel] = field(default_factory=list)


class GlbBuilder:
    def __init__(self, generator: str = "ASTRAWILD ArtSourceGen 1.0"):
        self.materials: Dict[str, Material] = {}
        self.nodes: List[Node] = []
        self.mesh_nodes: List[int] = []
        self.skins: List[Skin] = []
        self.animations: List[Animation] = []
        self.generator = generator
        self._bin = bytearray()
        self._buffer_views: List[dict] = []
        self._accessors: List[dict] = []

    # ---------------------------------------------------------------- nodes
    def add_material(self, mat: Material) -> None:
        self.materials[mat.name] = mat

    def add_node(self, name: str, parent: int = -1,
                 translation: Tuple[float, float, float] = (0.0, 0.0, 0.0),
                 rotation: Optional[Sequence[float]] = None,
                 scale: Tuple[float, float, float] = (1.0, 1.0, 1.0),
                 bone_tag: str = "") -> int:
        assert parent < len(self.nodes), "parent node not yet added"
        node = Node(name=name, parent=parent, translation=tuple(translation),
                    rotation=None if rotation is None else quat_normalize(rotation),
                    scale=tuple(scale), bone_tag=bone_tag or name)
        self.nodes.append(node)
        return len(self.nodes) - 1

    def set_mesh(self, node_idx: int, primitives: Sequence[Primitive]) -> None:
        assert 0 <= node_idx < len(self.nodes)
        self.nodes[node_idx].primitives = list(primitives)
        self.mesh_nodes.append(node_idx)

    def node_world(self, node_idx: int) -> np.ndarray:
        """World 4x4 of a node in the current (bind) hierarchy."""
        chain: List[int] = []
        i = node_idx
        while i >= 0:
            chain.append(i)
            i = self.nodes[i].parent
        world = np.eye(4)
        for idx in reversed(chain):
            n = self.nodes[idx]
            world = world @ trs_matrix(n.translation, n.rotation, n.scale)
        return world

    def node_world_position(self, node_idx: int) -> np.ndarray:
        return self.node_world(node_idx)[:3, 3]

    # ---------------------------------------------------------------- skins
    def add_skin(self, joints: Sequence[int]) -> int:
        """Computes inverse bind matrices from the CURRENT node transforms."""
        root = joints[0]
        ibms = np.zeros((len(joints), 4, 4), dtype=np.float32)
        for i, j in enumerate(joints):
            world = self.node_world(j)
            ibms[i] = np.linalg.inv(world).T  # store column-major
        skin = Skin(joints=list(joints), inverse_bind_matrices=ibms, skeleton_root=root)
        self.skins.append(skin)
        return len(self.skins) - 1

    def assign_skin(self, node_idx: int, skin_idx: int, primitives: Sequence[Primitive]) -> None:
        """Attaches skinned primitives (with joints/weights) to a node."""
        jcount = len(self.skins[skin_idx].joints)
        for p in primitives:
            assert p.joints is not None and p.weights is not None, "primitive lacks skin data"
            assert p.joints.shape[1] == 4 and p.weights.shape[1] == 4
            if p.joints.max() >= jcount or p.joints.min() < 0:
                raise ValueError(f"joint index out of range 0..{jcount - 1}")
            sums = p.weights.sum(axis=1)
            if not np.allclose(sums, 1.0, atol=1e-3):
                raise ValueError(f"skin weights must sum to 1 (max dev {np.abs(sums - 1).max():.4f})")
        self.set_mesh(node_idx, primitives)
        setattr(self.nodes[node_idx], "skin_index", skin_idx)

    # ------------------------------------------------------------ animation
    def add_animation(self, name: str) -> Animation:
        anim = Animation(name=name)
        self.animations.append(anim)
        return anim

    def add_channel(self, anim: Animation, node: int, path: str,
                    times: Sequence[float], values: np.ndarray) -> None:
        times = np.asarray(times, dtype=np.float32)
        if path == "rotation":
            values = np.asarray(values, dtype=np.float64)
            assert values.shape[1] == 4, "rotation values must be (K,4) (w,x,y,z)"
            values = np.array([quat_normalize(v) for v in values], dtype=np.float32)
        else:
            values = np.asarray(values, dtype=np.float32)
            assert values.shape[1] == 3, f"{path} values must be (K,3)"
        assert len(times) == len(values), "times/values length mismatch"
        if len(times) < 2:
            raise ValueError("animation channel needs >= 2 keyframes")
        if np.any(np.diff(times) < 0):
            raise ValueError("animation times must be non-decreasing")
        anim.channels.append(AnimChannel(node=node, path=path, times=times, values=values))

    # ----------------------------------------------------------------- save
    def _push_bytes(self, data: bytes, target: Optional[int] = None) -> Tuple[int, int, int]:
        while len(self._bin) % 4 != 0:
            self._bin += b"\x00"
        offset = len(self._bin)
        self._bin += data
        length = len(data)  # exact data length (glTF 2.0); pad lives between views
        while len(self._bin) % 4 != 0:
            self._bin += b"\x00"
        view = {"buffer": 0, "byteOffset": offset, "byteLength": length}
        if target is not None:
            view["target"] = target
        self._buffer_views.append(view)
        return len(self._buffer_views) - 1, offset, length

    def _accessor(self, arr: np.ndarray, component: int, atype: str,
                  target: Optional[int] = None, minmax: bool = False) -> int:
        arr = np.ascontiguousarray(arr)
        count = arr.shape[0]
        ncomp = ATTR_TYPE_COUNT[atype]
        flat = arr.reshape(count, ncomp)
        if component == COMPONENT_FLOAT:
            data = flat.astype("<f4").tobytes()
        elif component == COMPONENT_UINT16:
            data = flat.astype("<u2").tobytes()
        elif component == COMPONENT_UINT8:
            data = flat.astype("<u1").tobytes()
        elif component == COMPONENT_UINT32:
            data = flat.astype("<u4").tobytes()
        else:
            raise ValueError(f"unsupported component {component}")
        view_idx, _, _ = self._push_bytes(data, target)
        acc = {"bufferView": view_idx, "componentType": component,
               "count": count, "type": atype}
        if minmax:
            acc["min"] = [round(float(v), 6) for v in flat.min(axis=0)]
            acc["max"] = [round(float(v), 6) for v in flat.max(axis=0)]
        self._accessors.append(acc)
        return len(self._accessors) - 1

    def save_glb(self, path: str) -> dict:
        doc: dict = {"asset": {"version": "2.0", "generator": self.generator}}
        scene_nodes = [i for i, n in enumerate(self.nodes) if n.parent == -1]
        doc["scenes"] = [{"nodes": scene_nodes}]
        doc["scene"] = 0

        # materials
        doc["materials"] = []
        mat_index: Dict[str, int] = {}
        for name, m in self.materials.items():
            idx = len(doc["materials"])
            mat_index[name] = idx
            entry = {
                "name": m.name,
                "pbrMetallicRoughness": {
                    "baseColorFactor": [round(v, 5) for v in m.base_color],
                    "metallicFactor": round(m.metallic, 4),
                    "roughnessFactor": round(m.roughness, 4),
                },
            }
            if any(v > 0 for v in m.emissive):
                entry["emissiveFactor"] = [round(v, 5) for v in m.emissive]
            if m.double_sided:
                entry["doubleSided"] = True
            doc["materials"].append(entry)

        # nodes
        doc["nodes"] = []
        for node in self.nodes:
            node_entry: dict = {"name": node.name}
            node_entry["translation"] = [round(v, 6) for v in node.translation]
            if node.rotation is not None and not np.allclose(node.rotation, [1, 0, 0, 0]):
                q = node.rotation  # (w,x,y,z) -> glTF [x,y,z,w]
                node_entry["rotation"] = [round(q[1], 6), round(q[2], 6),
                                          round(q[3], 6), round(q[0], 6)]
            if not np.allclose(node.scale, 1.0):
                node_entry["scale"] = [round(v, 6) for v in node.scale]
            doc["nodes"].append(node_entry)

        # children wiring
        children: Dict[int, List[int]] = {}
        for i, n in enumerate(self.nodes):
            if n.parent >= 0:
                children.setdefault(n.parent, []).append(i)
        for i, entry in enumerate(doc["nodes"]):
            if i in children:
                entry["children"] = children[i]

        # skins
        doc["skins"] = []
        for skin in self.skins:
            view_idx, _, _ = self._push_bytes(
                skin.inverse_bind_matrices.astype("<f4").tobytes())
            acc = {"bufferView": view_idx, "componentType": COMPONENT_FLOAT,
                   "count": len(skin.joints), "type": "MAT4"}
            self._accessors.append(acc)
            entry = {"joints": list(skin.joints),
                     "inverseBindMatrices": len(self._accessors) - 1}
            if skin.skeleton_root >= 0:
                entry["skeleton"] = skin.skeleton_root
            doc["skins"].append(entry)

        # mesh primitives per mesh node
        doc["meshes"] = []
        for n_idx in self.mesh_nodes:
            node = self.nodes[n_idx]
            mesh_id = len(doc["meshes"])
            prims_json = []
            for p in node.primitives:
                attrs = {}
                pos = np.asarray(p.positions, dtype="<f4")
                attrs["POSITION"] = self._accessor(pos, COMPONENT_FLOAT, "VEC3", 34962, minmax=True)
                attrs["NORMAL"] = self._accessor(np.asarray(p.normals, dtype="<f4"),
                                                 COMPONENT_FLOAT, "VEC3", 34962)
                if p.uvs is not None:
                    attrs["TEXCOORD_0"] = self._accessor(
                        np.asarray(p.uvs, dtype="<f4"), COMPONENT_FLOAT, "VEC2", 34962)
                if p.joints is not None:
                    attrs["JOINTS_0"] = self._accessor(
                        np.asarray(p.joints, dtype=np.uint8), COMPONENT_UINT8, "VEC4", 34962)
                if p.weights is not None:
                    attrs["WEIGHTS_0"] = self._accessor(
                        np.asarray(p.weights, dtype="<f4"), COMPONENT_FLOAT, "VEC4", 34962)
                idx = np.asarray(p.indices, dtype=np.int64).reshape(-1)
                component = COMPONENT_UINT16 if idx.max() < 65536 else COMPONENT_UINT32
                acc_idx = self._accessor(
                    idx.astype("<u2" if component == COMPONENT_UINT16 else "<u4"),
                    component, "SCALAR", 34963)
                prims_json.append({"attributes": attrs, "indices": acc_idx,
                                   "material": mat_index[p.material], "mode": 4})
            doc["meshes"].append({"primitives": prims_json, "name": f"M_{node.name}"})
            doc["nodes"][n_idx]["mesh"] = mesh_id
            if hasattr(node, "skin_index"):
                doc["nodes"][n_idx]["skin"] = getattr(node, "skin_index")

        # animations
        doc["animations"] = []
        for anim in self.animations:
            channels_json = []
            samplers_json = []
            for ch in anim.channels:
                acc_in = self._accessor(ch.times, COMPONENT_FLOAT, "SCALAR")
                if ch.path == "rotation":
                    # Internal API quaternions are (w, x, y, z); glTF serializes
                    # animation sampler outputs (like node rotations) as [x, y, z, w].
                    vals = np.asarray(ch.values, dtype=np.float64)[:, [1, 2, 3, 0]]
                else:
                    vals = ch.values
                acc_out = self._accessor(vals, COMPONENT_FLOAT,
                                         "VEC4" if ch.path == "rotation" else "VEC3")
                samplers_json.append({"input": acc_in, "output": acc_out,
                                      "interpolation": "linear"})
                channels_json.append({"sampler": len(samplers_json) - 1,
                                      "target": {"node": ch.node, "path": ch.path}})
            doc["animations"].append({"name": anim.name,
                                      "channels": channels_json,
                                      "samplers": samplers_json})

        doc["buffers"] = [{"byteLength": len(self._bin)}]
        if self._accessors:
            doc["accessors"] = self._accessors
        if self._buffer_views:
            doc["bufferViews"] = self._buffer_views

        json_bytes = json.dumps(doc, separators=(",", ":")).encode("utf-8")
        pad = (4 - len(json_bytes) % 4) % 4
        json_bytes += b" " * pad
        bin_chunk = bytes(self._bin)
        total = 12 + 8 + len(json_bytes) + (8 + len(bin_chunk) if bin_chunk else 0)
        with open(path, "wb") as f:
            f.write(struct.pack("<III", GLB_MAGIC, 2, total))
            f.write(struct.pack("<II", len(json_bytes), CHUNK_JSON))
            f.write(json_bytes)
            if bin_chunk:
                f.write(struct.pack("<II", len(bin_chunk), CHUNK_BIN))
                f.write(bin_chunk)

        return {
            "path": path,
            "nodes": len(self.nodes),
            "meshes": len(self.mesh_nodes),
            "materials": list(self.materials.keys()),
            "skins": len(self.skins),
            "joints": len(self.skins[0].joints) if self.skins else 0,
            "animations": [a.name for a in self.animations],
            "triangles": int(sum(len(p.indices) for n in self.mesh_nodes
                                 for p in self.nodes[n].primitives)),
            "bytes": total,
        }


# --------------------------------------------------------------------- parser
def validate_glb(path: str, verbose: bool = True) -> List[str]:
    """Reparses a GLB and checks structural + skin + animation contracts.
    Returns list of problems (empty == pass). Mirrors UE Interchange expectations."""
    problems: List[str] = []
    with open(path, "rb") as f:
        magic, version, total = struct.unpack("<III", f.read(12))
        if magic != GLB_MAGIC:
            return [f"{path}: bad magic"]
        data = f.read()
    json_len, json_type = struct.unpack("<II", data[:8])
    if json_type != CHUNK_JSON:
        return [f"{path}: first chunk not JSON"]
    doc = json.loads(data[8:8 + json_len].decode("utf-8"))

    nodes = doc.get("nodes", [])
    accs = doc.get("accessors", [])
    views = doc.get("bufferViews", [])
    meshes = doc.get("meshes", [])
    bin_offset = 8 + json_len + 8  # after glb header + json chunk header + json

    def acc_data(i: int) -> np.ndarray:
        a = accs[i]
        v = views[a["bufferView"]]
        off = bin_offset + v.get("byteOffset", 0)
        end = off + v["byteLength"]
        raw = data[off:end]
        comp = a["componentType"]
        n = a["count"]
        shape = (n, ATTR_TYPE_COUNT[a["type"]])
        if comp == COMPONENT_FLOAT:
            return np.frombuffer(raw, dtype="<f4").reshape(shape)
        if comp == COMPONENT_UINT16:
            return np.frombuffer(raw, dtype="<u2").reshape(shape)
        if comp == COMPONENT_UINT8:
            return np.frombuffer(raw, dtype="<u1").reshape(shape)
        return np.frombuffer(raw, dtype="<u4").reshape(shape)

    # map mesh_id -> skin id via nodes
    mesh_skin: Dict[int, int] = {}
    for nd in nodes:
        if "mesh" in nd and "skin" in nd:
            mesh_skin[nd["mesh"]] = nd["skin"]
    joint_counts: Dict[int, int] = {}
    for si, skin in enumerate(doc.get("skins", [])):
        joint_counts[si] = len(skin["joints"])
        ibm = acc_data(skin["inverseBindMatrices"])
        if ibm.shape[0] != len(skin["joints"]):
            problems.append(f"{path}: IBM count mismatch")

    for mi, mesh in enumerate(meshes):
        for prim in mesh["primitives"]:
            attrs = prim["attributes"]
            n = accs[attrs["POSITION"]]["count"]
            idx = acc_data(prim["indices"]).reshape(-1)
            if len(idx) and int(idx.max()) >= n:
                problems.append(f"{path}: index {int(idx.max())} >= vertex count {n}")
            if attrs.get("JOINTS_0") is not None:
                joints = acc_data(attrs["JOINTS_0"])
                weights = acc_data(attrs["WEIGHTS_0"])
                if joints.shape[0] != n or weights.shape[0] != n:
                    problems.append(f"{path}: skin attr count != position count")
                else:
                    if np.any(np.abs(weights.sum(axis=1) - 1.0) > 1e-2):
                        problems.append(f"{path}: weights do not sum to 1")
                    if mi in mesh_skin and joints.max(initial=0) >= joint_counts[mesh_skin[mi]]:
                        problems.append(
                            f"{path}: JOINTS_0 {int(joints.max())} exceeds skin joints "
                            f"{joint_counts[mesh_skin[mi]]}")

    for anim in doc.get("animations", []):
        for ch in anim["channels"]:
            node = ch["target"]["node"]
            if node >= len(nodes):
                problems.append(f"{path}: anim targets missing node {node}")
            sampler = anim["samplers"][ch["sampler"]]
            out = acc_data(sampler["output"])
            times = acc_data(sampler["input"])
            if ch["target"]["path"] == "rotation":
                norms = np.linalg.norm(out.astype(np.float64), axis=1)
                if np.any(np.abs(norms - 1.0) > 1e-3):
                    problems.append(f"{path}: anim quat not normalized")
            if np.any(np.diff(times) <= -1e-6):
                problems.append(f"{path}: anim times decreasing")
            if not np.isclose(float(times[0]), 0.0):
                problems.append(f"{path}: anim does not start at t=0 ({float(times[0])})")
    if verbose:
        print(f"[validate] {path}: {'PASS' if not problems else 'FAIL ' + '; '.join(problems)}")
    return problems
