# ASTRAWILD Working Memory

## Verified facts

- GitHub repository is private and currently has `main` as the verified branch.
- The reported `release/vertical-slice-v1` and commit `f8cf5f1` are not present on GitHub at the time of audit.
- Current GitHub `main` has C++ source and docs but no `.uasset` or `.umap` under `Content/ASTRAWILD`.
- The C++ core is a prototype contract; Unreal Compile and Playtest are still required on the target machine.
- Primitive mesh placeholders are allowed for the first playable risk slice.
- `Docs/visual_target_astrawild.png` is concept art used to guide composition, not a final imported asset.

## Stable naming

- Module: `AstrawildCore`
- GameMode: `AAstrawildGameMode`
- Player: `AAstrawildPlayerCharacter`
- Echo: `AAstrawildEchoCharacter`
- Resource: `AAstrawildResourceNode`
- Rest Point: `AAstrawildRestPoint`
- Damage target: `AAstrawildDamageTarget`
- Save: `UAstrawildSaveGame` and `UAstrawildSaveSubsystem`

## Decisions

- Build a compact Vertical Slice before open-world expansion.
- Target future co-op for 1–4 players; do not claim multiplayer complete until authority and two-client tests pass.
- Use original ASTRAWILD identity inspired by gameplay categories, not copied characters, names, sounds, maps or assets.
- Keep C++ authoritative for transaction rules and stable save data; use Blueprint/Content for composition and presentation.
- Record every third-party asset license.

## Handoff truth

Antigravity must update `Docs/BUILD_STATUS.md`, push its branch/commit to GitHub, and include evidence of Compile and Playtest. A text report alone is not proof that files exist in the repository.
