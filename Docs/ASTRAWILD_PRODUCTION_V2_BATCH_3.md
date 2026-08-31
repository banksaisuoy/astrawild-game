# ASTRAWILD — PRODUCTION V2 BATCH 3: Dialogue System + Production Content Pack Foundation

**Status:** SOURCE_IMPLEMENTED / SOURCE_TESTED (51 automation tests — compile pending
UE5 verification on the target machine). Baseline: `5eb6b9e` (Batches 1–2 UE5-VERIFIED
48/48, engine logs in `Docs/ENGINE_LOGS/`).

Two deliveries in one batch:

1. **Batch 3 (P0 — completed interrupted work):** the dialogue system the previous
   session left half-written (headers/registry written, controller implementations
   missing → the tree would not have compiled). Now complete end-to-end.
2. **Production Content Pack (user directive):** the 10-area content specification
   (`Docs/CONTENT_PACK/CP-00..CP-10`) + the C++ binding hooks the specs need
   (weapon Niagara/audio soft refs, Echo evolution, equipment visual overrides).

---

## 1. What shipped — C++ / data

### 1.1 Dialogue system (P12 Story/NPC)

| File | Change |
|---|---|
| `AstrawildDataAssets.h` | (already written last session) `FAstrawildDialogueLine/Choice/Node`, `UAstrawildDialogueTreeDefinition`, `UAstrawildNPCDefinition::DialogueTreeId` |
| `AstrawildItemRegistrySubsystem.h/.cpp` | (already written) Register/Find/GetAll dialogue trees |
| `AstrawildDialogueComponent.h/.cpp` | (already written) story flags + condition evaluation + consequence routing |
| `AstrawildDialogueWidget.h/.cpp` | (already written) pure-C++ UMG conversation screen |
| `AstrawildPlayerController.h/.cpp` | **completed:** `DialogueComponent` subobject creation, `OpenDialogue`/`CloseDialogue`/`IsDialogueOpen` implementations (registry-resolved tree, UI-only input, ESC via widget), sibling-screen close coordination, `IsAnyScreenOpen` includes dialogue |
| `AstrawildNPCCharacter.cpp` | interact routes to the dialogue screen when `DialogueTreeId` resolves (falls back to legacy quest-toast + shop when unbound; unregistered id = warning log) |
| `AstrawildSaveSubsystem.h/.cpp` | save v4 payload extension: `DialogueFlags` (additive — older v4 saves deserialize empty) |
| `AstrawildProductionContent.h/.cpp` | **NEW content:** `BuildDialogueTrees` — 6 trees (Maren/Tam/Rowan/Kael/Sela/Perry) covering quest offers, lore nodes, one-time beats (ForbiddenFlag), chained flags, item grant, research grants, vendor hand-off (bOpenShop) |
| `AstrawildContentLibrary.cpp` | 6 NPC definitions bound to trees (Maren/Tam/Rowan/Kael/Sela/Perry) |

### 1.2 Content Pack binding hooks (CP-01/02/03/05/06)

| File | Change |
|---|---|
| `AstrawildDataAssets.h` | `UAstrawildWeaponDefinition` += `MuzzleFlashVfx`/`ImpactVfx`/`ProjectileTrailVfx` (TSoftObjectPtr<UNiagaraSystem>) + `FireSound`/`ImpactSound` (TSoftObjectPtr<USoundBase>); `UAstrawildItemDefinition` += `EquipMeshOverride`/`EquipMaterialOverride`; `UAstrawildEchoDefinition` += `EvolveToDefinitionId`/`EvolveRequiredLevel`/`EvolveRequiredBond` |
| `AstrawildEchoRosterSubsystem.h/.cpp` | `CanEvolveInstance` (static pure gate check), `CanEvolve`, `EvolveInstance` (roster swap + spawned-actor rebuild via `InitializeFromDefinition`, identity-preserving) |
| `AstrawildProductionContent.cpp` | `BuildEvolutionTargets`: 6 evolved species (Terraquill Verdant, Cindermule Pyre, Voltpylon Tempest, Bastionbeetle Bulwark, Mistmender Rime, Deepdelver Abyssal) + chain links + per-chain gates |
| `AstrawildCombatComponent.cpp` | Niagara-first dispatch: `SpawnWeaponMuzzleFlash` (Niagara-if-loaded else procedural + fire sound) / `SpawnWeaponImpact` (impact FX + sound) wired at all 4 muzzle sites + beam terminal + arc first contact; projectile launch passes bindings |
| `AstrawildProjectileActor.h/.cpp` | `TrailVfxAsset`/`ImpactVfxAsset` + `SetWeaponVfxAssets` (attached trail spawn, no sync load) + impact burst in `OnHit` (pre-authority so all machines see it) |
| `AstrawildCore.Build.cs` | + `"Niagara"` module dependency |
| `AstrawildAutomationTests.cpp` | +5 tests (below) |

### 1.3 Tests (48 → 51)

- `ASTRAWILD.Dialogue.TreeContract` — node lookup, unique ids, goto resolution, no
  ambiguous end+goto, asset-id type.
- `ASTRAWILD.Dialogue.ChoiceConditions` — flag gates, forbidden flags, AND semantics,
  quest gates fail without quest state, round-trip export/import.
- `ASTRAWILD.Dialogue.Consequences` — flag consequence applies; unknown quest
  start fails hard; navigation-only succeeds.
- `ASTRAWILD.Echo.EvolutionGates` — fail-closed (nulls, dangling link, self-cycle,
  mismatched target), dual gate level AND bond, exact boundary passes, final forms.
- `ASTRAWILD.Weapon.AssetBindingContract` — zero-asset fallback defaults + FName id
  contract round-trip + CP-01 equip overrides default unset.

## 2. Data definitions added

- 6 `UAstrawildDialogueTreeDefinition` (Dialogue_WardenMaren, Dialogue_TraderTam,
  Dialogue_ElderRowan, Dialogue_SkiffWardenKael, Dialogue_GuardSela,
  Dialogue_OldSaltPerry) — 18 nodes, 40 choices.
- 6 `UAstrawildEchoDefinition` evolution targets + evolution fields on the 6 base
  production Echoes.
- Save v4 payload: `DialogueFlags` (additive; no schema bump — old v4 saves load clean).

## 3. What Antigravity must do (per `Docs/ASTRAWILD_ENGINE_VERIFICATION_QUEUE.md`)

1. Pull + rebuild (note: **new module dependency `Niagara`** — UBT will ask once).
2. Run the 51 automation tests (`V-5`) — expect 51/51 (three dialogue + evolution
   gates + weapon binding contract are new).
3. Walk V2-21..V2-27: dialogue screen, condition gating, one-time beats, vendor
   hand-off, Perry's chained flags, evolution end-to-end, Niagara dispatch, save
   round-trip.
4. Content Pack production: start with `Docs/CONTENT_PACK/CP-00_INDEX.md` suggested
   order (CP-07+CP-04 Dawn Fields first). Every pack lists binding steps + budgets +
   acceptance criteria.

## 4. Known limitations

- Dialogue voice-over + portraits are post-launch (CP-09 §2 roadmap).
- Weapon impact FX is a single system per profile (no physical-material surface
  switching yet — CP-05 §9 stretch).
- `EquipMeshOverride` is data-contract only this batch (BP equipment rig consumes it
  next batch; procedural silhouette remains the fallback).
- Evolution is reachable via cheat manager (level/bond grind is honest but slow);
  roster UI "Ready to Evolve" chip is CP-10 scope.
- MP note: dialogue screen is local-controller only; consequences route through
  server authority (single-player/listen-server verified path).

## 5. Honest status labels

- Dialogue system: SOURCE_IMPLEMENTED (+3 SOURCE_TESTED at pure-logic level).
- Evolution: SOURCE_TESTED (gates) + SOURCE_IMPLEMENTED (roster/actor swap).
- Weapon bindings: SOURCE_IMPLEMENTED (dispatch tested via contract test; assets
  themselves are UE5_INTEGRATION_REQUIRED).
- Content pack docs: SPEC_READY (assets authored by Antigravity per pack).
- Nothing here is RUNTIME_VERIFIED until the queue rows pass in-engine.
