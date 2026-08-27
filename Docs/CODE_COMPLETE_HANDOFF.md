# ASTRAWILD Code-Complete Handoff

## Current branch

Use `release/vertical-slice-v1`. The latest code-complete commit is recorded by GitHub after the progression/save pass. Always pull the branch before running the editor.

## What is code-ready

The branch contains the C++ contracts for player and Echo animation references, runtime animation variables, Niagara/Sound event feedback, UMG inventory and crafting base widgets, Alpha Echo phase logic, Lore/Quest row schemas and CSV sources, quest progression, survival meters, and SaveSubsystem capture/restore for quest and survival state.

## What still needs Unreal Editor

The repository cannot create binary Unreal assets without the Editor. The target machine must create or import Skeletal Meshes, Skeletons, Animation Blueprints, animation montages, Niagara Systems, Sound Cues, Widget Blueprints, DataTables, Player/Echo Blueprints, Alpha Echo Blueprint/Data Asset, and the prototype `.umap`. The absence of these files is a content gap, not a C++ code gap.

## Required sequence on the Windows machine

1. Pull `release/vertical-slice-v1` and confirm the working tree is clean.
2. Run `python Scripts/validate_content_contracts.py` if Python is available.
3. Run `powershell -ExecutionPolicy Bypass -File Tools/Validate_Astrawild.ps1`.
4. Open `ASTRAWILD.uproject` in Unreal Engine 5.8 and allow project files/generated headers to update.
5. Compile `ASTRAWILDEditor Win64 Development`.
6. Create the Content Browser assets from `Docs/P0_Animation_Contract.md` through `Docs/P4_Lore_Quest_Data_Contract.md`.
7. Assign the created assets to Player/Echo Blueprint defaults and DataTables.
8. Run the core loop: movement → dodge → harvest → inventory → craft → fight → type/status feedback → capture → quest progress → survival warning → rest/build → save/load.
9. Run the two-client test only after the single-player loop passes.
10. Update `Docs/BUILD_STATUS.md` with the exact engine/compiler version, compile result, asset paths, PIE result, screenshots and known issues, then commit and push the report to the same branch.

## Verification rule

A static source validation pass is not an Unreal build. A code-complete handoff becomes playable only after the target machine provides a successful compile, actual `.uasset/.umap` paths and a recorded PIE test. Do not delete the primitive fallback path until the imported skeletal assets and animation graphs pass the same loop.
