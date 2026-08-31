# ANTIGRAVITY FIX LOG — BATCH 3 INTEGRATION

**Project**: ASTRAWILD  
**Date**: 2026-08-31  

---

## Resolved Issues in Batch 3

1. **Missing FindNode Implementation in `UAstrawildDialogueTreeDefinition` (`LNK2019`)**:
   - **Root Cause**: GLM declared `FindNode(FName NodeId) const` in `AstrawildDataAssets.h` but did not provide the implementation.
   - **Fix**: Implemented `FindNode` inline in `AstrawildDataAssets.h` iterating over `Nodes`.

2. **Format String Validator in HUD Widget (`C7595` / `NotEnoughArguments`)**:
   - **Root Cause**: In UE 5.8 `FormatStringSan`, `TEXT("%s | DMG %.0f | %.1fs")` parsed `s` as format specifier `%s`.
   - **Fix**: Changed format string to `TEXT("%s | DMG %.0f | Rate %.1f")` and included `AstrawildCombatComponent.h`.

3. **Incomplete Type in SoftObjectPtr Test (`C2027` / `C2672`)**:
   - **Root Cause**: `Profile->MuzzleFlashVfx.IsValid()` attempted to dynamic_cast the forward-declared `UNiagaraSystem`.
   - **Fix**: Included `NiagaraSystem.h` and used `Profile->MuzzleFlashVfx.IsNull()` in `AstrawildAutomationTests.cpp`.
