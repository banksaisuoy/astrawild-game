# ASTRAWILD — Recorded Assumptions & Decisions

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-29**

Every non-obvious decision made during the V2 foundation round, with rationale and consequence. These are
the project's "we meant to do that" records — review each before treating the related system as final.

---

## Environment & Delivery Constraints

### 1. Sandbox cannot compile UE5 — source-complete delivery
- **Assumption/decision:** the development sandbox (Linux, no UE5/MSVC/GPU) cannot compile or run the
  project. This round delivers **source-complete, compile-conservative C++** plus docs.
- **Rationale:** worklog Task 0 / directive §53; the user's Windows machine (Antigravity) is the compile
  target.
- **Consequence:** every "DONE (code)" status in the Roadmap and DoD is provisional until the target
  machine compiles (`ASTRAWILD_TEST_PLAN.md` §4). No completion claims before that (§51).

### 2. Single runtime module (`AstrawildCore`) instead of the master plan's 8-module split
- **Decision:** keep one runtime module with folder-level organization (audit §F).
- **Rationale:** fewer cross-module UBT failure surfaces for a codebase that has never compiled; module
  split is a refactor *after* first successful compile.
- **Consequence:** master plan §8's module list (AstrawildGameplay/World/Building/…) is deferred, not
  cancelled. The Build.cs already carries the needed dependency set (AIModule, NavigationSystem, UMG,
  GameplayTags, EnhancedInput, GameplayAbilities, GameplayTasks).

---

## Gameplay Decisions

### 3. Grid-snap simplified building (v1)
- **Decision:** free 200 cm grid placement with a single yaw rotation (15° steps) and a box-overlap
  validation; no piece-to-piece snapping, no structural integrity, no deconstruct.
- **Rationale:** prove the loop first; full modular snap is a content-alpha system.
- **Consequence:** buildings can float on slopes (ground-aligned Z only); documented in Building doc §8.

### 4. Shared co-op research pool
- **Decision:** `UAstrawildResearchSubsystem` is GameInstance-scoped — one research pool shared by all
  players in a session.
- **Rationale:** co-op is collaborative (master plan §7: host owns world unlocks); shared pool avoids
  double-charging teams for the same tech; simplest correct model for listen-server co-op.
- **Consequence:** visiting players do **not** keep research when they leave (world unlocks stay with the
  host world — consistent with master plan). Per-player knowledge/personal progression would need a v3
  save + PlayerState split. Recorded in Multiplayer doc §4.

### 5. Station-crafts-first-recipe as the crafting UI stopgap
- **Decision:** interacting with a crafting station crafts the first craftable recipe requiring it.
- **Rationale:** makes gather→craft→use playable with zero UI work; the component API is the real
  contract for the future UMG screen.
- **Consequence:** players cannot *choose* recipes yet; documented as a stopgap everywhere it's mentioned
  (Crafting doc §4).

### 6. IMC runtime construction (no input assets required)
- **Decision:** `BuildRuntimeInputDefaults()` creates 15 input actions + one mapping context in code when
  no editor IMC is assigned.
- **Rationale:** zero-asset playability (audit critical gap #9); the project must be playable from an
  empty Content folder.
- **Consequence:** no in-game remapping until real IMC assets + settings UI; code log line says "16
  actions" (cosmetic off-by-one — 15 are created). Editor IMC assignment cleanly disables the runtime path.

### 7. Placeholder engine basic shapes for all visuals
- **Decision:** every mesh is an engine cube/sphere/cylinder/capsule/plane; every Echo is a sphere.
- **Rationale:** zero-asset playability; art is a later milestone; gameplay readability is temporarily
  sacrificed for provability.
- **Consequence:** species are visually indistinguishable (personality/element distinguish them logically);
  `REPLACE_BEFORE_RELEASE` tracked in the Asset Manifest.

### 8. Personality roll: 70 % species-dominant / 30 % random archetype
- **Decision:** `RollPersonalityFromDefinition` picks the species' dominant personality with 70 %
  probability, else a uniform random archetype.
- **Rationale:** species identity stays recognizable while individuals vary (directive §5 "creatures feel
  varied").
- **Consequence:** occasionally odd pairings (a Timid Gloomfang flees early) — accepted as emergent flavor.

### 9. Wild Echoes are not individually persisted
- **Decision:** only the captured roster saves; wild populations re-spawn via the bootstrapper each
  session (seeded by the world seed).
- **Rationale:** save size and determinism; ecosystem population counters capture the aggregate state.
- **Consequence:** a weakened wild Echo seen before quitting is at full health after load; accepted for
  the vertical slice.

### 10. Materials consumed client-side before the building RPC, refunded server-side on failure
- **Decision:** `ConfirmPlacement` consumes items locally, then sends `ServerPlaceBuilding`; on server
  re-validation failure the server refunds.
- **Rationale:** avoids a double round-trip; refund path keeps the player whole.
- **Consequence:** in high-latency co-op a client could briefly show materials consumed for a placement
  that fails server-side (refund corrects it). Revisit during netcode testing (Multiplayer doc §5).

### 11. Power grid is one proximity-shared grid (v1)
- **Decision:** all power-role buildings within `ConnectivityRadius` (1200 cm) form one grid;
  `IsLocationPowered` ≈ "near any generator".
- **Rationale:** base scale is tiny; a full graph/electrical simulation is overkill now.
- **Consequence:** no sub-grids or wire routing; documented in Building doc §7.

### 12. Ecosystem tier distances chosen for the arena scale
- **Decision:** 3000/8000/20000 cm tier boundaries.
- **Rationale:** tuned to the 160 m bootstrapper arena (Tier0 ≈ close encounter range).
- **Consequence:** in a full biome these boundaries will need retuning — they're EditAnywhere properties,
  no code change needed.

### 13. Quest component on PlayerController, not replicated
- **Decision:** quests live per-PlayerController server-side; no replication of quest state.
- **Rationale:** single-player-first; death/respawn survival of quest state; simplest correct scope.
- **Consequence:** co-op clients have no quest HUD feed yet (Multiplayer doc §5 item 4).

### 14. Save = first player controller only
- **Decision:** `SaveWorld` serializes player data for the first player controller.
- **Rationale:** single-player-first architecture (header comment says so).
- **Consequence:** co-op saves are incorrect-by-omission today; per-player save blocks are a schema v3
  feature.

### 15. Temperature model: flat 20 °C base + weather offset
- **Decision:** felt temperature = 20 °C + weather offset; no time-of-day or biome modulation.
- **Rationale:** simplest model that exercises the cold/heat damage thresholds.
- **Consequence (known):** with CODE_DEFAULT weather, no state actually crosses the 4 °C/36 °C damage
  thresholds (Cold peaks at 8 °C, Heat at 30 °C) — the damage code is reachable only after tuning (Test
  Plan T-4). Recorded as a tuning task, not a bug.

### 16. Status effect speed multiplier stored but not applied
- **Decision:** `FAstrawildStatusEffect.SpeedMultiplier` exists in data; movement integration is NOT
  IMPLEMENTED.
- **Rationale:** damage-tick pipeline first; movement coupling touches prediction paths.
- **Consequence:** future content applying Slow/Haste will need the movement hook added.

### 17. Journal observation = automatic (no scan key)
- **Decision:** observation progress accrues passively while an Echo is in the view cone (1400 cm, ~41°).
- **Rationale:** observation-as-gameplay rewards looking; feeds capture without a button.
- **Consequence:** the `Interaction.Scan` tag and scan key are reserved for future explicit scanning.

### 18. v1 payloads stay in the save struct
- **Decision:** v1 fields (`EchoRoster`, `RestPoints`, `ActiveRestPointId`) remain in `UAstrawildSaveGame`
  beside v2 fields.
- **Rationale:** in-place v1→v2 migration needs the source data present.
- **Consequence:** save files carry some redundancy; drop after two released versions (policy in Save doc §7).

---

## Documentation/Process Assumptions

### 19. Values cited in docs are read from source at doc-writing time
- Docs quote defaults from constructors and the ContentLibrary as of 2026-08-29. If a value changes,
  the owning doc must change in the same round (DoD §3).

### 20. The web prototype is LEGACY
- `WEB_PLAYABLE_SLICE.md` describes a cancelled browser prototype; its header now marks it superseded by
  the UE5 C++ slice. No gameplay rule may cite it as source of truth (the C++ module is).

### 21. "16 actions" log line
- The runtime input log says 16 actions; 15 are created. Cosmetic; noted here so nobody "fixes" the docs
  to match the log instead of the code.
