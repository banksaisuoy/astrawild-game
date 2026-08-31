"""ASTRAWILD ArtSourceGen — shared manifest recorder.
Every generator records its outputs here; the master manifest (ArtSource/manifest.json)
is the QA contract consumed by Content/Python/AwPipeline/import_all.py and the
Antigravity runbook (import verification must cover every entry)."""
from __future__ import annotations

import json
import os
from typing import Any, Dict

REPO = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
MANIFEST_PATH = os.path.join(REPO, "ArtSource", "manifest.json")


def load() -> Dict[str, Any]:
    if os.path.exists(MANIFEST_PATH):
        with open(MANIFEST_PATH, "r", encoding="utf-8") as f:
            return json.load(f)
    return {"generator": "ASTRAWILD ArtSourceGen 1.0", "assets": {}}


def record(category: str, asset_name: str, stats: Dict[str, Any]) -> None:
    data = load()
    data["assets"][asset_name] = {"category": category, **stats}
    os.makedirs(os.path.dirname(MANIFEST_PATH), exist_ok=True)
    with open(MANIFEST_PATH, "w", encoding="utf-8") as f:
        json.dump(data, f, indent=2, sort_keys=True)


def assets_by_category(category: str) -> Dict[str, Any]:
    return {k: v for k, v in load()["assets"].items() if v.get("category") == category}
