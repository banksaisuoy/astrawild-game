# ASTRAWILD Working Memory

## Verified facts

- GitHub repository is private and currently has `main` as the verified branch.
- The reported `release/vertical-slice-v1` and commit `f8cf5f1` are not present on GitHub at the time of audit.
- Current GitHub `main` has C++ source and docs but no `.uasset` or `.umap` under `Content/ASTRAWILD`.
- The C++ core is a prototype contract; Unreal Compile and Playtest are still required on the target machine.
- Primitive mesh placeholders are allowed for the first playable risk slice.
- `Docs/visual_target_astrawild.png` is concept art used to guide composition, not a final imported asset.

## Verified facts (2026-09-18 completion-stage audit)

- **LFS objects ARE on GitHub's LFS storage** (60 MB, 46 files): all textures, both boot maps, the master material. A clone without git-lfs yields 130-byte pointer files and cannot boot — `Setup_FirstRun.bat` now detects and fixes this.
- Both `Content/ThirdPerson/Lvl_ThirdPerson.umap` and `Content/ASTRAWILD/Maps/MainMap.umap` are genuine UE packages (magic `0x9E2A83C1`).
- Post-`03c2fe6` source drift (5 files) was statically reviewed — no compile blockers found.
- The completion stage (title screen / victory / death overlay) is source-complete but NOT yet engine-compiled: first compile on the UE 5.8 machine is the #1 pending action. Tests now 55 (added `ASTRAWILD.Quest.VictoryChainTermination`).
- `/Game/Characters/Mannequins/...` (referenced by `94a398c`) is local-machine-only content — soft fallbacks to exosuit/procedural body keep the repo self-contained.

## Stable naming

- Module: `AstrawildCore`
- GameMode: `AAstrawildGameMode`
- Player: `AAstrawildPlayerCharacter`
- Echo: `AAstrawildEchoCharacter`
- Resource: `AAstrawildResourceNode`
- Rest Point: `AAstrawildRestPoint`
- Damage target: `AAstrawildDamageTarget`
- Save: `UAstrawildSaveGame` and `UAstrawildSaveSubsystem`
- Title menu: `UAstrawildTitleMenuWidget` (shown on standalone boot; PIE skips)
- Victory: `UAstrawildVictoryScreenWidget` (raised on `Quest_VanguardProtocol`)
- Death overlay: `UAstrawildDeathOverlayWidget` (respawn countdown)

## Decisions

- Build a compact Vertical Slice before open-world expansion.
- Target future co-op for 1–4 players; do not claim multiplayer complete until authority and two-client tests pass.
- Use original ASTRAWILD identity inspired by gameplay categories, not copied characters, names, sounds, maps or assets.
- Keep C++ authoritative for transaction rules and stable save data; use Blueprint/Content for composition and presentation.
- Record every third-party asset license.
- Completion stage bookends stay pure-C++ UMG (no asset dependency) so the zero-asset rule holds; each screen has a `*Class` override point for future UMG/Designer variants.

## Handoff truth

Antigravity must update `Docs/BUILD_STATUS.md`, push its branch/commit to GitHub, and include evidence of Compile and Playtest. A text report alone is not proof that files exist in the repository.
