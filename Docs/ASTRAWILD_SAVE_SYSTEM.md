# ASTRAWILD — Save System (Schema v2)

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-29**
**Primary sources:** `AstrawildSaveSubsystem.h/.cpp`, `AstrawildTypes.h` (save structs),
`AstrawildGameMode.cpp` (autosave), `AstrawildPlayerCharacter.cpp` (F5/F9)

`UAstrawildSaveSubsystem` (GameInstance subsystem) + `UAstrawildSaveGame` (USaveGame payload). Server-only
orchestration (`SaveWorld`/`LoadWorld` reject `NM_Client`). Everything persistent resolves through stable
ids — never object references.

---

## 1. Schema v2 Payload Inventory (`UAstrawildSaveGame`)

**Header**

| Field | Purpose |
|---|---|
| `SaveSchemaVersion = 2` | Constant `CurrentSchemaVersion = 2` |
| `SavedAtUtc` (FDateTime) | Timestamp |
| `IntegrityChecksum` (uint32) | FNV-1a over `"<schema>|<timestamp>"` |

**v2 payload**

| Field | Type | Contents |
|---|---|---|
| `WorldState` | `FAstrawildWorldSaveData` | `ElapsedWorldMinutes`, `DayNumber`, `Weather`, `Seed` |
| `PlayerSurvival` | `FAstrawildSurvivalStats` | HP/stamina/hunger/thirst/temperature/dead |
| `PlayerTransform` | FTransform | Player world transform |
| `EchoRosterV2` | `TArray<FAstrawildEchoInstanceV2>` | Captured Echoes: instance id, definition id, personality, level, XP, trust, bond, needs, transform, in-party |
| `Buildings` | `TArray<FAstrawildBuildingSaveData>` | id, definition id, transform, health, charge, switch state, owner |
| `Research` | `FAstrawildResearchSaveData` | unlocked tech ids + research points |
| `Quests` | `TArray<FAstrawildQuestSaveData>` | quest states incl. per-objective runtime progress |
| `Journal` | `TArray<FAstrawildJournalEntry>` | observation progress + knowledge flags per species |

**v1 payload (kept for migration + legacy snapshot API)**: `PlayerInventory`, `EchoRoster`
(`FAstrawildEchoInstanceSaveData`), `RestPoints`, `ActiveRestPointId`.

---

## 2. Integrity Checksum — FNV-1a

```cpp
Source = "<SaveSchemaVersion>|<SavedAtUtc>"
Hash   = 2166136261
for each char: Hash ^= char; Hash *= 16777619   (FNV-1a, 32-bit)
```

- Written on every save; `LoadWorld` recomputes and **refuses to load** on mismatch (tamper/truncation
  tripwire). `IntegrityChecksum == 0` is treated as absent (legacy tolerance).
- Determinism verified by the automation test `ASTRAWILD.Save.ChecksumDeterminism` (same input → same
  hash; different schema version → different hash; non-zero).
- Scope: header fields only (schema + timestamp) — a full-payload checksum is PLANNED (see §7).

---

## 3. v1 → v2 Migration Path

`MigrateV1ToV2` (runs when `SaveSchemaVersion < 2` during load):

- Every v1 `FAstrawildEchoInstanceSaveData` roster entry lifts into a `FAstrawildEchoInstanceV2`:
  instance id, definition id, level, trust, transform, `bInParty = bInRoster`, and
  **`Personality = Curious`** (v1 files never stored one — default assigned).
- Bumps `SaveSchemaVersion` to 2 and logs the entry count via `LogAstrawildSave`.
- Migration is one-way in-memory during load; the migrated data persists on the next save.

Future migrations follow the same pattern: additive fields + a `MigrateVxToVy` step per gap — never mutate
old payloads destructively.

---

## 4. SaveWorld / LoadWorld Orchestration

### SaveWorld(world, slot) — order

1. Create `UAstrawildSaveGame`; stamp schema version + UTC now + checksum.
2. **World state** from GameState (time, day, weather, seed).
3. **Player**: transform, survival stats, inventory stacks (first player controller — single-player-first);
   **quests** from the PlayerController's QuestComponent.
4. **Roster** (`EchoRosterSubsystem->ExportForSave` — refreshes entries from live party actors first) and
   **research** (GameInstance subsystems).
5. **Buildings**: every `AAstrawildBuildingActor` → `ToSaveData()`.
6. **Journal** entries.
7. `SaveGameToSlot` (engine); success/failure logged with building + roster counts.

### LoadWorld(world, slot) — order

1. Existence + load + checksum verification (refuse on mismatch).
2. Migrate if schema < 2.
3. **World state**: time, day (advance until `DayNumber` matches), weather, seed.
4. **Research** import, **roster** import (GameInstance scope).
5. **Player**: teleport to saved transform, `FullRestore` vitals, `SetItemStacks` inventory; despawn current
   party Echoes and re-import roster data.
6. **Quests** import (restores active/completed + objective progress).
7. **Buildings**: destroy all placed `AAstrawildBuildingActor`s, respawn each from save data
   (`FromSaveData` re-initializes definition, health, power registration).
8. **Journal** import.
9. Log summary (day, building count).

Slots: **`ASTRAWILD_Main`** (manual/quick save) and **`ASTRAWILD_Auto`** (autosave), `UserIndex = 0`.

---

## 5. Autosave & Quick Save/Load

| Trigger | Slot | Code path |
|---|---|---|
| **Autosave every 300 s** (`AutosaveIntervalSeconds`, GameMode timer, 0 disables) | `ASTRAWILD_Auto` | `GameMode::HandleAutosave` → `SaveWorld` |
| **F5** quick save | `ASTRAWILD_Main` | `PlayerCharacter::QuickSave` (authority only) |
| **F9** quick load | `ASTRAWILD_Main` | `PlayerCharacter::QuickLoad` |
| Cheat `AW.SaveNow` / `AW.LoadNow` | `ASTRAWILD_Main` | CheatManager |

---

## 6. Legacy Snapshot API (v1 compatibility)

`SaveSnapshot(inventory, roster, restPoints, activeRestPointId, slot)` / `LoadSnapshot(...)` — retained for
v1 callers; writes only the v1 payload fields (with a v2 header + checksum). `DoesSaveExist`, `DeleteSave`
slot utilities.

---

## 7. Not Implemented / Future Policy (honest)

| Item | Status |
|---|---|
| Full-payload checksum (beyond header) | PLANNED |
| Atomic write (temp file + rename) / backup rotation (≥3 slots) | NOT IMPLEMENTED — relies on engine `SaveGameToSlot` |
| Crash-mid-save protection (transactional save) | NOT IMPLEMENTED |
| Multiple character profiles | NOT IMPLEMENTED — fixed slot names |
| Co-op: saving non-host players | NOT IMPLEMENTED (first player only) |
| Save-file versioning UI ("save is from an older version") | NOT IMPLEMENTED (silent migration) |
| Migration policy going forward | Every schema bump must ship a `MigrateVxToVy` + additive structs only; v1 payload stays until two released versions pass, then may be dropped (policy decision recorded in Assumptions) |
