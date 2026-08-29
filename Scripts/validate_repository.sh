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
  Source/AstrawildCore/Public/AstrawildGameState.h \
  Source/AstrawildCore/Public/AstrawildTimeSubsystem.h \
  Source/AstrawildCore/Public/AstrawildWeatherSubsystem.h \
  Source/AstrawildCore/Public/AstrawildEventBusSubsystem.h \
  Source/AstrawildCore/Public/AstrawildEcosystemSubsystem.h \
  Source/AstrawildCore/Public/AstrawildEchoCharacter.h \
  Source/AstrawildCore/Public/AstrawildEchoAIController.h \
  Source/AstrawildCore/Public/AstrawildEchoRosterSubsystem.h \
  Source/AstrawildCore/Public/AstrawildSurvivalComponent.h \
  Source/AstrawildCore/Public/AstrawildCombatComponent.h \
  Source/AstrawildCore/Public/AstrawildCaptureComponent.h \
  Source/AstrawildCore/Public/AstrawildJournalSubsystem.h \
  Source/AstrawildCore/Public/AstrawildItemRegistrySubsystem.h \
  Source/AstrawildCore/Public/AstrawildContentLibrary.h \
  Source/AstrawildCore/Public/AstrawildCraftingComponent.h \
  Source/AstrawildCore/Public/AstrawildCraftingStationActor.h \
  Source/AstrawildCore/Public/AstrawildBuildingActor.h \
  Source/AstrawildCore/Public/AstrawildBuildingComponent.h \
  Source/AstrawildCore/Public/AstrawildPowerSubsystem.h \
  Source/AstrawildCore/Public/AstrawildResearchSubsystem.h \
  Source/AstrawildCore/Public/AstrawildQuestComponent.h \
  Source/AstrawildCore/Public/AstrawildPlayerController.h \
  Source/AstrawildCore/Public/AstrawildHudWidget.h \
  Source/AstrawildCore/Public/AstrawildGameMode.h \
  Source/AstrawildCore/Public/AstrawildWorldBootstrapper.h \
  Source/AstrawildCore/Public/AstrawildDungeonRoomActor.h \
  Source/AstrawildCore/Public/AstrawildDungeonGeneratorActor.h \
  Source/AstrawildCore/Public/AstrawildEchoBossCharacter.h \
  Source/AstrawildCore/Public/AstrawildCheatManager.h \
  Source/AstrawildCore/Public/AstrawildNPCCharacter.h \
  Source/AstrawildCore/Private/AstrawildWorldBootstrapper.cpp \
  Source/AstrawildCore/Private/AstrawildDungeonRoomActor.cpp \
  Source/AstrawildCore/Private/AstrawildDungeonGeneratorActor.cpp \
  Source/AstrawildCore/Private/AstrawildEchoBossCharacter.cpp \
  Source/AstrawildCore/Private/AstrawildAutomationTests.cpp \
  Docs/ASTRAWILD_UE5_ARCHITECTURE_AUDIT.md \
  Docs/ASTRAWILD_UE5_ARCHITECTURE_V2.md \
  Docs/ANTIGRAVITY_BUILD_CHECKLIST.md; do
  test -f "$required" || { echo "ERROR: missing $required" >&2; exit 1; }
done

for generated in Binaries Intermediate Saved DerivedDataCache; do
  test ! -d "$generated" || { echo "ERROR: generated directory must not be committed: $generated" >&2; exit 1; }
done

# Every public header must have a matching .cpp or be header-only by design.
while IFS= read -r header; do
  base="$(basename "$header" .h)"
  if [ "$base" != "AstrawildTypes" ] && [ "$base" != "AstrawildDataAssets" ] \
     && [ "$base" != "AstrawildLog" ] && [ "$base" != "AstrawildGameplayTags" ] \
     && [ "$base" != "AstrawildInteractable" ] && [ "$base" != "AstrawildCore" ]; then
    test -f "Source/AstrawildCore/Private/${base}.cpp" \
      || { echo "ERROR: header without implementation: ${base}" >&2; exit 1; }
  fi
done < <(find Source/AstrawildCore/Public -name '*.h' | sort)

if grep -RInE 'AAstrawwild|TODO|FIXME' Source; then
  echo "ERROR: suspicious marker found in Source" >&2
  exit 1
fi

# Replication sanity: gameplay state classes must opt into replication.
grep -q "bReplicates = true" Source/AstrawildCore/Private/AstrawildPlayerCharacter.cpp \
  || { echo "ERROR: player character is not replicated" >&2; exit 1; }
grep -q "bReplicates = true" Source/AstrawildCore/Private/AstrawildEchoCharacter.cpp \
  || { echo "ERROR: echo character is not replicated" >&2; exit 1; }
grep -q "DOREPLIFETIME(AAstrawildGameState" Source/AstrawildCore/Private/AstrawildGameState.cpp \
  || { echo "ERROR: game state does not replicate world state" >&2; exit 1; }

git diff --check
printf '%s\n' "ASTRAWILD repository validation passed (v2 ruleset). Unreal compile/playtest still required on the target machine."
