"""
ASTRAWILD ArtSourceGen — rig, proximity auto-skinning and animation helpers.

Bones are added with bind TRANSLATION ONLY (identity rotation) so animation
rotations are world-axis-aligned deltas — simple and predictable. Each bone
declares a segment (start = bind world pos, end = pos + dir*length) used for
proximity skinning: weight_ij = 1/(dist(v_i, seg_j)+eps)^power, top-4 kept,
renormalized. All primitives of one rig share one skin.
"""
from __future__ import annotations

import math
from dataclasses import dataclass
from typing import Dict, List, Optional, Sequence, Tuple

import numpy as np

from aw_gltf import (GlbBuilder, Material, Primitive, quat_from_euler,
                     quat_mul, quat_normalize)
from aw_shapes import MeshBuilder


@dataclass
class Bone:
    name: str
    node: int
    parent: Optional[str]
    bind_position: np.ndarray        # world bind position (meters)
    segment_start: np.ndarray
    segment_end: np.ndarray


class Rig:
    def __init__(self, builder: GlbBuilder, root_name: str = "Root",
                 root_position: Sequence[float] = (0.0, 0.0, 0.0)):
        self.builder = builder
        self.bones: Dict[str, Bone] = {}
        self.order: List[str] = []
        self.root_name = root_name
        node = builder.add_node(root_name, parent=-1, translation=tuple(root_position))
        # Root must never win proximity weights: park its segment far below ground.
        sheath = np.array(root_position, dtype=np.float64) + np.array([0.0, -4.0, 0.0])
        self.bones[root_name] = Bone(root_name, node, None,
                                     np.array(root_position, dtype=np.float64),
                                     sheath, sheath + np.array([0.0, -0.5, 0.0]))
        self.order.append(root_name)

    def add_bone(self, name: str, parent: str, position: Sequence[float],
                 direction: Sequence[float] = (0.0, -1.0, 0.0),
                 length: float = 0.25) -> str:
        """position = LOCAL offset from parent bone. direction/length define the
        skinning segment in world space (bind pose)."""
        assert parent in self.bones, f"parent bone {parent} missing"
        parent_bone = self.bones[parent]
        local = np.asarray(position, dtype=np.float64)
        world = parent_bone.bind_position + local
        node = self.builder.add_node(name, parent=parent_bone.node,
                                     translation=tuple(local))
        d = np.asarray(direction, dtype=np.float64)
        n = np.linalg.norm(d)
        d = d / n if n > 1e-9 else np.array([0.0, -1.0, 0.0])
        self.bones[name] = Bone(name, node, parent, world, world,
                                world + d * length)
        self.order.append(name)
        return name

    def bone_index(self, name: str) -> int:
        return self.order.index(name)

    def joint_nodes(self) -> List[int]:
        return [self.bones[n].node for n in self.order]

    def bind_quat(self, name: str) -> np.ndarray:
        n = self.builder.nodes[self.bones[name].node]
        return n.rotation if n.rotation is not None else np.array([1.0, 0.0, 0.0, 0.0])

    # ------------------------------------------------------------ skinning
    def build_skin(self, mesh_builder: MeshBuilder, power: float = 3.0,
                   min_weight_ratio: float = 0.02) -> Tuple[int, List[Primitive]]:
        """Creates the skin + fills JOINTS_0/WEIGHTS_0 on every primitive via
        proximity to bone segments. Returns (skin_idx, skinned primitives)."""
        names = self.order
        seg_starts = np.array([self.bones[n].segment_start for n in names])
        seg_ends = np.array([self.bones[n].segment_end for n in names])
        prims = mesh_builder.build_primitives()
        for prim in prims:
            joints, weights = self._skin_positions(prim.positions, seg_starts, seg_ends,
                                                   power, min_weight_ratio)
            prim.joints = joints
            prim.weights = weights
        skin_idx = self.builder.add_skin(self.joint_nodes())
        return skin_idx, prims

    def _skin_positions(self, positions: np.ndarray, seg_starts: np.ndarray,
                        seg_ends: np.ndarray, power: float,
                        min_weight_ratio: float) -> Tuple[np.ndarray, np.ndarray]:
        n = len(positions)
        nb = len(seg_starts)
        v = positions.astype(np.float64)                    # (N,3)
        a = seg_starts                                     # (B,3)
        b = seg_ends                                       # (B,3)
        ab = b - a                                         # (B,3)
        ab_len2 = np.einsum("bi,bi->b", ab, ab)             # (B,)
        ab_len2 = np.where(ab_len2 < 1e-12, 1.0, ab_len2)
        # projection t of v onto each segment
        av = v[:, None, :] - a[None, :, :]                  # (N,B,3)
        t = np.einsum("nbi,bi->nb", av, ab) / ab_len2[None, :]  # (N,B)
        t = np.clip(t, 0.0, 1.0)
        closest = a[None, :, :] + t[:, :, None] * ab[None, :, :]   # (N,B,3)
        dist = np.linalg.norm(v[:, None, :] - closest, axis=2)    # (N,B)
        w = 1.0 / (np.power(dist + 0.015, power))
        # top-4 selection
        top = np.argsort(-w, axis=1)[:, :4]                 # (N,4)
        top_w = np.take_along_axis(w, top, axis=1)          # (N,4)
        max_w = np.maximum(top_w[:, :1], 1e-12)
        top_w = np.where(top_w > min_weight_ratio * max_w, top_w, 0.0)
        total = top_w.sum(axis=1, keepdims=True)
        total = np.where(total < 1e-12, 1.0, total)
        top_w = top_w / total
        joints = top.astype(np.uint8)
        weights = top_w.astype(np.float32)
        return np.ascontiguousarray(joints), np.ascontiguousarray(weights)

    # ------------------------------------------------------------ animation
    def add_rotation_channel(self, anim, bone: str, keys: Sequence[Tuple[float, Sequence[float]]],
                             space: str = "delta") -> None:
        """keys = [(time, euler(rx, ry, rz) radians)]. `delta` composes onto bind
        (identity here, so delta == local). Values slerp'd by the engine."""
        node = self.bones[bone].node
        times = np.array([k[0] for k in keys], dtype=np.float32)
        values = []
        bind = self.bind_quat(bone)
        for _, euler in keys:
            q = quat_from_euler(*euler)
            values.append(quat_mul(bind, q) if space == "delta" else q)
        self.builder.add_channel(anim, node, "rotation", times,
                                 np.array(values, dtype=np.float64))

    def add_translation_channel(self, anim, bone: str,
                                keys: Sequence[Tuple[float, Sequence[float]]]) -> None:
        node = self.bones[bone].node
        base = np.array(self.builder.nodes[node].translation, dtype=np.float64)
        times = np.array([k[0] for k in keys], dtype=np.float32)
        values = np.array([base + np.asarray(k[1], dtype=np.float64) for k in keys],
                          dtype=np.float64)
        self.builder.add_channel(anim, node, "translation", times, values)


# --------------------------------------------------------------- animation utils
def make_cycle_keys(n_keys: int, duration: float) -> List[float]:
    """Times for a looping cycle: 0..duration, endpoints duplicated implicitly
    by engine wrap (we emit n_keys keys over [0, duration))."""
    return [duration * i / n_keys for i in range(n_keys)]


def swing(angle_deg: float, phase_offset: float = 0.0) -> float:
    """Helper: sine swing in radians."""
    return math.radians(angle_deg)


def sampled_sine(n_keys: int, amplitude_rad: float, phase: float = 0.0) -> List[float]:
    """n_keys samples of a full sine cycle (radians), used for limb swings."""
    return [amplitude_rad * math.sin(2.0 * math.pi * (i / n_keys) + phase)
            for i in range(n_keys)]


def ease_out(t: float) -> float:
    return 1.0 - (1.0 - t) * (1.0 - t)
