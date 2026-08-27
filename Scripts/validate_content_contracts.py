#!/usr/bin/env python3
"""Validate ASTRAWILD text content contracts without requiring Unreal Editor."""
from __future__ import annotations

import csv
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
REQUIRED_CSV = {
    ROOT / "Content/Astrawild/Data/Source/DT_Lore.csv": {"Name", "LoreId", "Title", "Body", "RegionTag", "SortOrder", "bUnlockedByDefault"},
    ROOT / "Content/Astrawild/Data/Source/DT_Quests.csv": {"Name", "QuestId", "Title", "Description", "RegionTag", "PrerequisiteQuestTag", "bMainQuest"},
    ROOT / "Content/Astrawild/Data/Source/DT_QuestObjectives.csv": {"Name", "QuestId", "ObjectiveId", "Type", "TargetTag", "RequiredQuantity", "Description"},
}
REQUIRED_PATHS = [
    "Source/AstrawildCore/Public/Animation/AstrawildAnimInstance.h",
    "Source/AstrawildCore/Private/Animation/AstrawildAnimInstance.cpp",
    "Source/AstrawildCore/Public/Components/AstrawildFeedbackComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildFeedbackComponent.cpp",
    "Source/AstrawildCore/Public/Components/AstrawildQuestComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildQuestComponent.cpp",
    "Source/AstrawildCore/Public/Components/AstrawildSurvivalComponent.h",
    "Source/AstrawildCore/Private/Components/AstrawildSurvivalComponent.cpp",
    "Source/AstrawildCore/Public/UI/AstrawildGameplayWidgets.h",
    "Source/AstrawildCore/Private/UI/AstrawildGameplayWidgets.cpp",
    "Source/AstrawildCore/Public/Echoes/AstrawildAlphaEcho.h",
    "Source/AstrawildCore/Private/Echoes/AstrawildAlphaEcho.cpp",
]

errors: list[str] = []
for relative in REQUIRED_PATHS:
    if not (ROOT / relative).is_file():
        errors.append(f"missing required source: {relative}")

for path, required_columns in REQUIRED_CSV.items():
    if not path.is_file():
        errors.append(f"missing CSV: {path.relative_to(ROOT)}")
        continue
    with path.open(newline="", encoding="utf-8-sig") as handle:
        reader = csv.DictReader(handle)
        columns = set(reader.fieldnames or [])
        missing = required_columns - columns
        if missing:
            errors.append(f"{path.relative_to(ROOT)} missing columns: {sorted(missing)}")
        rows = list(reader)
        if not rows:
            errors.append(f"{path.relative_to(ROOT)} has no rows")
        names = [row.get("Name", "") for row in rows]
        if len(names) != len(set(names)):
            errors.append(f"{path.relative_to(ROOT)} has duplicate Name values")

for header in (ROOT / "Source").rglob("*.h"):
    text = header.read_text(encoding="utf-8", errors="replace")
    if "UCLASS(" in text or "USTRUCT(" in text or "UENUM(" in text:
        if "generated.h" not in text:
            errors.append(f"reflection header missing generated include: {header.relative_to(ROOT)}")

for source in (ROOT / "Source").rglob("*.cpp"):
    text = source.read_text(encoding="utf-8", errors="replace")
    if text.count("{") != text.count("}"):
        errors.append(f"brace count mismatch: {source.relative_to(ROOT)}")
    if text.count("(") != text.count(")"):
        errors.append(f"parenthesis count mismatch: {source.relative_to(ROOT)}")

if errors:
    print("ASTRAWILD content contract validation failed:")
    print("\n".join(f"- {error}" for error in errors))
    sys.exit(1)

print("ASTRAWILD content contract validation passed.")
