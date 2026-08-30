# ASTRAWILD — Dungeon & Boss Encounter Design (Hollow Underlight)

> Status header (honest, per directive §53): source-complete C++, **compile NOT RUN** —
> this sandbox has no UE engine. All claims below are verified **at source level**
> (commit for Batch 6: see `Docs/BUILD_STATUS.md` "Changes in this round").
> Scope: the STEP 22 dungeon/boss vertical slice (wave 3, commit `a0634f6`) hardened by
> **Batch 6** — gates, portals, elemental boss combat, save persistence, Ancient-era
> reward. This doc maps 1:1 to `Docs/ASTRAWILD_ULTIMATE_PRODUCTION_ROADMAP_V3.md`
> directives §21 (Bosses) and §22 (Dungeons).

---

## 1. The Hollow Underlight at a glance

| Property | Value (source) |
|---|---|
| Dungeon id | `Dungeon_HollowUnderlight` |
| Placement | `ArenaSize * 1.4` east of the Dawn Fields camp (WorldBootstrapper) |
| Layout | Linear chain, 5 rooms: Entry → Combat → Puzzle → Elite → Boss |
| Determinism | `FRandomStream(WorldSeed + 777)` — same world seed ⇒ same dungeon |
| Room count clamp | 3..12 (default 5) |
| Room spacing | 2200 cm between centers, ±400 cm lateral zigzag (seeded) |
| Gates | 4 (one per room pair) — sealed until the previous room clears |
| Portals | 2 — entrance pad at the wilds' edge, exit pad beside the entry room |
| Boss | The Underlight Warden (phased boss, derived from `Echo_Gloomfang`) |
| Completion reward | +10 RP, unique tech `Tech_AncientResonance` (force-unlocked) |
| Save persistence | `FAstrawildDungeonSaveData` in the v2 payload (additive) |

## 2. Room chain & templates (hand-authored, directive §23)

| # | RoomTypeId | Half-extents (cm) | Encounter | Clear loot |
|---|---|---|---|---|
| 0 | Entry | 500×500×300 | none (auto-clears) | — |
| 1 | Combat | 650×650×320 | 2× pool creatures | — |
| 2 | Puzzle | 600×600×300 | 1 guard | — |
| 3 | Elite | 700×700×350 | 2× pool creatures | — |
| 4 | Boss | 900×900×400 | Underlight Warden + 1 add offset | `Loot_DungeonBoss` |

Creature pool (CODE_DEFAULT): `Echo_Gloomfang`, `Echo_Gloomfang`, `Echo_Stonehide`
(cycled by room index). Room shells are placeholder floor plates scaled from engine
cubes — walls arrive with the asset pass (`REPLACE_BEFORE_RELEASE`).

## 3. Progression gates (Batch 6 — Item A)

`AAstrawildDungeonGateActor` implements the gate that had only ever been
forward-declared since wave 3:

- **Sealed**: blocking box collision (120×1040×840 cm) + crossbar dropped at chest
  height (Z=60). **Open**: collision disabled + crossbar lifted into the lintel
  (Z=520) — the arch stays as a doorway.
- Gate *i* seals the passage between room *i* and room *i+1*; it opens the moment
  room *i* clears (`HandleRoomCleared → Gates[i]->OpenGate()`), and reopens in sync
  when a save is restored (`ApplySavedState`).
- `bOpen` replicates; `OnRep_bOpen` re-applies collision + crossbar state on
  clients because collision-enabled does **not** replicate by itself.

## 4. Portals (Batch 6 — Item C)

`AAstrawildDungeonPortalActor : AActor, IAstrawildInteractable` — flat resonance pad
(engine cylinder squashed):

| Portal | PortalId (quest TargetId) | Location | Destination |
|---|---|---|---|
| Entrance | `Location_HollowUnderlight` | `ArenaSize*1.05, 0, 100` | dungeon entry room (+Z 150) |
| Exit | `Location_DawnCamp` | beside entry room (+Y 900) | 300 cm west of the entrance pad |

- Interact (E) → server-side teleport with a 600 cm range guard (anti-cheat);
  publishes `Event.LocationReached` with the PortalId.
- This is the **first publisher** for `Event.LocationReached` — and
  `UAstrawildQuestComponent` now matches `ReachLocation` objectives against it
  (the objective type existed in the enum since wave 1 with no producer or matcher).
- Dedicated-client routing deferred to the H-9 MP batch (same policy as
  `PlayerController::OpenShop`).

## 5. The Underlight Warden (boss, directive §21)

Phased boss — **never just extra HP** (all three phases change the behavior pattern):

| Phase | Trigger | Behavior |
|---|---|---|
| 1 | spawn | measured melee pressure: 380 speed, 2.0 s swings, 30 dmg |
| 2 | ≤66% HP | +2 Gloomfang adds on a 350 cm ring, 440 speed, 1.6 s swings, ×1.15 dmg |
| 3 | ≤33% HP | enrage tempo: 560 speed, 1.0 s swings |
| enrage | 180 s timer | forces phase 3 from any state, ×1.4 dmg — fights can't be stalled |

### Batch 6 hardening (all source-verified)

1. **Definition-driven stats** — `InitializeFromBossDefinition(Echo_Gloomfang)`:
   HP = 110 × 5.0 = **550**, ATK = 18 × 1.8 = **32.4**, weakness **Light**,
   element **Ash**. `BossDefinitionId` was cosmetic before (hardcoded 600/30).
2. **Elemental damage path** — `ApplyElementalBossDamage(dmg, element)`:
   weakness **×1.5**, same-element **×0.75**, otherwise ×1.0 — the exact multiplier
   vocabulary of the Echo pipeline. The player's melee cast-ladder now routes
   through it (bosses previously skipped the entire elemental layer).
3. **Status effects on the boss** — the shared element→status factory applies:
   Ember→Burn, Frost→Chill (speed), Flora→Poison, Pulse→Shock. Bosses take
   durations ×`BossStatusDurationMultiplier` (0.5) and never stack (refresh only)
   — CC can't replace the fight. DoT ticks ride `ApplyBossDamage` so a burn kill
   fires the full defeat chain.
4. **Defeat event** — publishes `Event.HostileDefeated` with
   `DefeatEventTargetId = Creature_UnderlightWarden` (distinct from the wild
   species id so quests can require *the boss*, not any Gloomfang).
   `OnBossDefeated` remains for future UMG boss-health-bar consumers.
5. **Testable statics** — `ComputeBossElementalMultiplier`,
   `ComputePhaseForHealthFraction`, `ComputeBossAttackDamage` are pure
   `UFUNCTION(BlueprintPure)` statics exercised by 3 automation tests
   (`ASTRAWILD.Dungeon.*`, 15 total tests).

### Counter-play: the Ancient Resonator

The intended answer to the Ash-element warden: craft the **Light-element**
`Item_AncientResonator` (ATK 18, ×1.5 vs the warden's Light weakness) from the
warden's own drop. See §6.

## 6. Reward economy (roadmap V3 §21 unique technology reward)

```
clear all 5 rooms
  └─> +10 research points (shared pool)
  └─> ForceUnlockTech(Tech_AncientResonance)   ← free, prereq-free, once
        Tech_AncientResonance (Ancient era, 25 RP if researched normally)
          └─ Recipe_AncientResonator @ workbench:
               1× Ancient Core (boss drop, guaranteed)
             + 2× Crystal Shard
             + 1× Echo Resonator
               └─> 1× Ancient Resonator (Light, ATK 18)
```

The **Ancient era enum is now used** — it was reserved for exactly this milestone
since the tech tree grew to 9 nodes (Batch 5). Tree totals: 10 techs / 18 recipes /
28 items / 7 quests. `Loot_DungeonBoss` guaranteed drops: Ancient Core ×1,
Crystal Shard ×2, Ember Ash ×2, Dawn Shard ×3 (75% bonus roll).

## 7. Save persistence (Batch 6 — gap M-7 closed)

**Policy: cleared stays cleared; in-progress rooms respawn fresh.**

- `FAstrawildDungeonSaveData` (additive to the schema-v2 payload — no version bump,
  same precedent as `EquippedArmorId`): `DungeonId`, `ClearedRoomIndices`,
  `RoomsCleared`, `TotalRooms`, `bCompleted`.
- `SaveWorld` snapshots every `AAstrawildDungeonGeneratorActor` via
  `ExportForSave()`.
- `LoadWorld` (which runs **after** the bootstrapper generated the dungeon) applies
  records by `DungeonId` via `ApplySavedState()`:
  cleared rooms run `RestoreClearedState()` — encounter actors are **Destroy()ed
  silently** (no defeat events, no loot, no double quest credit), `bCleared` is set,
  gates reopen in sync, `RoomsCleared` comes from the record. Completion rewards
  never re-fire.

## 8. Quest integration (directive §25)

`Quest_HollowUnderlight` ("The Hollow Underlight") chains after
`Quest_EchoHusbandry` (the 6-quest chain now descends):

1. **ReachLocation** `Location_HollowUnderlight` — "Enter the Hollow Underlight"
2. **DefeatCreature** `Creature_UnderlightWarden` — "Defeat the Underlight Warden"

Rewards: 2× Herbal Salve, 5× Dawn Shard, 15 RP. The portal publishes objective 1;
the boss defeat event publishes objective 2 (distinct id — wild Gloomfang kills
never complete it).

## 9. Known gaps / next steps (honest)

- [ ] Compile + in-engine playtest on the UE 5.8 target machine (whole batch).
- [ ] Boss AI controller: movement is direct `AddMovementInput` (asset-pass hook-in;
      `AAstrawildEchoAIController` exists for regular Echoes).
- [ ] Telegraphs/arena hazards/weak points (roadmap §21): cooldown-gated swings only.
- [ ] Gate visuals are placeholder primitives (crossbar lift reads clearly, but the
      art pass owns the resonance-arch look).
- [ ] MP: portals and shop share the "local controller only" policy until H-9.
- [ ] Dungeon loot still grants to the first player only (single-player-first;
      revisit with the MP batch).

## 10. File map (Batch 6)

| File | Role |
|---|---|
| `Public/AstrawildDungeonGateActor.h` + `.cpp` | NEW — sealed-gate actor |
| `Public/AstrawildDungeonPortalActor.h` + `.cpp` | NEW — interactable portal pair |
| `AstrawildDungeonGeneratorActor.h/.cpp` | gates, DungeonId, RewardTechnologyId, Export/ApplySavedState |
| `AstrawildDungeonRoomActor.h/.cpp` | definition-driven boss spawn, RestoreClearedState |
| `AstrawildEchoBossCharacter.h/.cpp` | elemental path, statuses, definition stats, defeat event, 3 statics |
| `AstrawildCombatComponent.cpp` | cast ladder → ApplyElementalBossDamage |
| `AstrawildSaveSubsystem.h/.cpp` | Dungeons payload + save/load wiring |
| `AstrawildResearchSubsystem.h/.cpp` | ForceUnlockTech |
| `AstrawildQuestComponent.cpp` | ReachLocation matcher |
| `AstrawildContentLibrary.cpp` | +AncientResonator item/recipe, +Tech_AncientResonance, +Quest 7 |
| `AstrawildWorldBootstrapper.cpp` | DungeonId + portal pair spawn |
| `AstrawildAutomationTests.cpp` | +3 dungeon/boss tests (15 total) |
