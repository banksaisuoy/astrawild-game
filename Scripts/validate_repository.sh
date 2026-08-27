#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

command -v jq >/dev/null 2>&1 || { echo "ERROR: jq is required" >&2; exit 1; }
jq empty ASTRAWILD.uproject

for required in \
  Source/AstrawildCore/AstrawildCore.Build.cs \
  Source/AstrawildCore/Public/AstrawildTypes.h \
  Source/AstrawildCore/Public/AstrawildDataAssets.h \
  Source/AstrawildCore/Public/AstrawildInventoryComponent.h \
  Source/AstrawildCore/Public/AstrawildSaveSubsystem.h \
  Docs/ANTIGRAVITY_BUILD_CHECKLIST.md; do
  test -f "$required" || { echo "ERROR: missing $required" >&2; exit 1; }
done

for generated in Binaries Intermediate Saved DerivedDataCache; do
  test ! -d "$generated" || { echo "ERROR: generated directory must not be committed: $generated" >&2; exit 1; }
done

if grep -RInE 'AAstrawwild|TODO|FIXME' Source; then
  echo "ERROR: suspicious marker found in Source" >&2
  exit 1
fi

git diff --check
printf '%s\n' "ASTRAWILD repository validation passed. Unreal compile/playtest still required on the target machine."
