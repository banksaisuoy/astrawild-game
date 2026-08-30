# ASTRAWILD — Quest System

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-30** (wave 3 sync — camp NPCs wired to definitions; wave 2's quest 6 synced)
**Primary sources:** `AstrawildQuestComponent.h/.cpp`, `AstrawildQuestDefinition` (AstrawildDataAssets.h),
`AstrawildContentLibrary.cpp::BuildQuests()`, `AstrawildNPCCharacter.cpp`

Quests are **fully event-driven**: gameplay systems publish events on the bus; the quest component
translates matching events into objective progress. It lives on the **PlayerController** so quest state
survives death/respawn.

---

## 1. Design

- **No polling, no hooks in gameplay code** — quests subscribe to `EventBus->OnGameplayEvent` and filter
  by `Event.EventTag` + `Event.TargetId`.
- Definitions are data assets (`UAstrawildQuestDefinition`): objectives, rewards, chain link.
- One **active quest** at a time (`ActiveQuestId`); `CompletedQuestIds` history prevents re-starting.
- `StartingQuestId = "Quest_FirstLight"` auto-activates on first BeginPlay (new game) and re-activates
  after loading a save with no quest state.
- Rewards: item stacks (to inventory), research points (to the research subsystem), optional instant tech
  unlock (`RewardTechId`).
- Chaining: on completion, `NextQuestId` auto-starts.

---

## 2. Objective Types (9 in the enum)

`EAstrawildQuestObjectiveType` — with current event wiring status:

| Type | Event wiring | Matching event |
|---|---|---|
| `CollectItem` | ✅ WIRED | `Event.ItemCollected` (target = item id) |
| `CaptureEcho` | ✅ WIRED | `Event.EchoCaptured` (target = echo definition id) |
| `DefeatCreature` | ✅ WIRED | `Event.EchoDefeated` **or** `Event.HostileDefeated` (target = echo definition id) |
| `CraftRecipe` | ✅ WIRED | `Event.RecipeCrafted` (target = recipe id) |
| `PlaceBuilding` | ✅ WIRED | `Event.BuildingPlaced` (target = building definition id) |
| `UnlockTechnology` | ✅ WIRED | `Event.TechUnlocked` (target = tech id) |
| `ObserveEcho` | ✅ WIRED (fixed 2026-08-29, was T-1) | `Event.EchoObserved` (target = echo definition id; published by the journal at the 25 % first-scan milestone) |
| `ReachLocation` | ⛔ NOT WIRED — enum + data support only (no event publisher yet) |
| `SurviveTime` | ⛔ NOT WIRED — no publisher |

**Honest consequence:** 7 of 9 objective types are wired. `ReachLocation` and `SurviveTime` have no
`Event.*` publisher yet (no CODE_DEFAULT quest uses them today — the enum + data path exist for future
content). The former T-1 gap (Quest_FirstEcho's "Observe a Lumewisp") was fixed by publishing
`Event.EchoObserved` from the journal's first-scan milestone; verify on the target machine once compiled
(Test Plan §5).

Progress math: `ProgressCount = min(RequiredCount, Progress + max(1, Event.Amount))`; a quest completes
when **all** objectives are complete; completion is idempotent.

---

## 3. The First Dawn Chain (6 CODE_DEFAULT quests)

From `AstrawildContentLibrary.cpp::BuildQuests()`:

### Quest 1 — First Light (`Quest_FirstLight`)
> *"Gather materials from Dawn Fields to prepare for the journey ahead."*

| # | Type | Target | Count | Text |
|---|---|---|---|---|
| 1 | CollectItem | `Item_Wood` | 10 | Collect 10 Dawnwood |
| 2 | CollectItem | `Item_Stone` | 5 | Collect 5 Fieldstone |

Rewards: **2 × Echo Resonator**, **+5 RP**. Next: `Quest_FirstEcho`.

### Quest 2 — A Friend in the Fields (`Quest_FirstEcho`)
> *"Observe a wild Lumewisp, gain its trust, and welcome your first Echo."*

| # | Type | Target | Count | Text |
|---|---|---|---|---|
| 1 | ObserveEcho | `Echo_Lumewisp` | 1 | Observe a Lumewisp |
| 2 | CaptureEcho | `Echo_Lumewisp` | 1 | Capture a Lumewisp |

Rewards: **10 × Glimmer Berry**, **+10 RP**. Next: `Quest_Homeground`.

### Quest 3 — Homeground (`Quest_Homeground`)
> *"Raise the first foundations of your camp."*

| # | Type | Target | Count | Text |
|---|---|---|---|---|
| 1 | PlaceBuilding | `Building_Foundation` | 1 | Place a Foundation |
| 2 | PlaceBuilding | `Building_Workbench` | 1 | Place a Workbench |

Rewards: **+10 RP** (no items). Next: `Quest_Spark`.

### Quest 4 — The Spark (`Quest_Spark`)
> *"Research Electrical Foundations and bring light to your camp."*

| # | Type | Target | Count | Text |
|---|---|---|---|---|
| 1 | UnlockTechnology | `Tech_Electrical` | 1 | Unlock Electrical Foundations |
| 2 | PlaceBuilding | `Building_Generator` | 1 | Build an Echo Dynamo |

Rewards: **+15 RP**. Next: `Quest_DawnGuard`.

### Quest 5 — Dawn Guard (`Quest_DawnGuard`)
> *"Gloomfangs stalk the fields at night. Protect the dawn."*

| # | Type | Target | Count | Text |
|---|---|---|---|---|
| 1 | DefeatCreature | `Echo_Gloomfang` | 3 | Defeat 3 Gloomfangs |

Rewards: **1 × Ancient Core**, **+20 RP**. Next: `Quest_ShepherdsDawn`.

### Quest 6 — Shepherd's Dawn (`Quest_ShepherdsDawn`) — wave 2
> *"Sprigling herds graze the meadows. Learn the ways of Echo husbandry."*

| # | Type | Target | Count | Text |
|---|---|---|---|---|
| 1 | UnlockTechnology | `Tech_Husbandry` | 1 | Unlock Echo Husbandry |
| 2 | CaptureEcho | `Echo_Sprigling` | 1 | Capture a Sprigling |
| 3 | PlaceBuilding | `Building_FeedTrough` | 1 | Place an Echo Feed Trough |
| 4 | CollectItem | `Item_FeedMix` | 3 | Craft 3 Echo Feed Mix |

Rewards: **5 × Echo Feed Mix**, **2 × Dawnbloom Salve**, **+20 RP**. Next: none — chain ends (open end for
the next story arc).

---

## 4. Runtime Flow

```
BeginPlay (server)
  └─ EventBus->OnGameplayEvent += HandleGameplayEvent
  └─ no quest state + StartingQuestId set → StartQuest("Quest_FirstLight")

Gameplay event arrives
  └─ ApplyEventToQuest:
       find active quest state → for each incomplete objective:
         type ↔ event tag match && TargetId match?
           → ProgressCount += max(1, Event.Amount)   (clamped to required)
           → OnObjectiveProgress(quest, index, progress, required) broadcast
       all objectives complete? → CompleteQuest:
           bCompleted = true, CompletedQuestIds += quest
           GrantRewards(definition)   [items → inventory, RP → research, RewardTechId → TryUnlockTech]
           OnQuestStateChanged(questId, true) broadcast
           NextQuestId valid → StartQuest(next)
```

HUD integration: `GetActiveObjectives()` renders the top-left tracker (`[ ] Collect 10 Dawnwood (3/10)`).

---

## 5. Save Integration

- `ExportForSave` → `TArray<FAstrawildQuestSaveData>` (quest id, objectives **with runtime progress**,
  active/completed flags) into `SaveGame->Quests` (schema v2).
- `ImportFromSave` restores states, rebuilds `CompletedQuestIds` + `ActiveQuestId`; if the save contains no
  quests at all, the starting quest re-activates (new-game fallback).
- Objectives store `ProgressCount` at runtime — definitions stay pristine (progress is reset when a quest
  starts by copying definition objectives and zeroing counters).

---

## 6. NPCs (quest hooks)

`AAstrawildNPCCharacter` (interactable): E → `Quests->StartQuest(NpcDefinition->OfferedQuestId)` when the
definition carries a quest. Prompt shows the NPC display name. NPC definitions (`UAstrawildNPCDefinition`)
support `ShopLootTableId`. Schedule/dialogue/faction behavior is PLANNED.

**Wave 3:** two CODE_DEFAULT NPCs are registered by the content library and spawned at the starting camp
by `AAstrawildWorldBootstrapper::SpawnPointsOfInterest`:

| NPC | Id | Definition hook | Spawn |
|---|---|---|---|
| Warden Maren | `NPC_WardenMaren` | `OfferedQuestId = Quest_FirstLight` (talk to (re-)start the first quest) | (630, −630, 100) |
| Trader Tam | `NPC_VendorTam` | `ShopLootTableId = Loot_VendorStarter` (definition-level stock; purchase logic NOT IMPLEMENTED) | (−630, −630, 100) |

Bodies still use the engine capsule placeholder (see Asset Manifest §8/§9).

---

## 7. Not Implemented (honest)

| Feature | Status |
|---|---|
| Event publishers for ReachLocation / SurviveTime | NOT IMPLEMENTED (see §2). `ObserveEcho` is wired via journal milestones. |
| Quest selection/abandonment UI | NOT IMPLEMENTED (single auto-chained active quest) |
| Multiple simultaneous active quests | NOT IMPLEMENTED (single `ActiveQuestId`) |
| Quest text presentation beyond the HUD tracker | NOT IMPLEMENTED (no dialogue UI) |
| Per-player quest replication for co-op clients | NOT IMPLEMENTED — component is not replicated; host/SP only today |
| NPC dialogue/schedule screens, vendor purchase flow | NOT IMPLEMENTED — 2 CODE_DEFAULT NPCs exist (Warden Maren, Trader Tam); interaction is quest-start only; shop table is a data hook |

---

## 8. Quest 5 chain completion (wave 4)

Quest 5 "Defeat 3 Gloomfang" (`Quest_DawnGuard` — §3 above) now **chain-completes organically** after
Batch 2 / commit `d5d23c2`:

- `UAstrawildHostileSpawnerSubsystem` (NEW `UTickableWorldSubsystem`, server-only Tick @ 25 s)
  refills `Echo_Gloomfang` population around the player pawn every 25 s up to
  `TargetGloomfangPopulation = 4` (and `Echo_Emberfang` up to `TargetEmberfangPopulation = 2`).
- Spawn placement: ring biased outward — minimum 30 % of `SpawnRadius = 1800 cm`, max 100 %, so
  hostiles never spawn directly on top of the player; angle is uniform-random; the spawn stream is
  `FRandomStream` seeded from `AAstrawildGameState::WorldSeed` for deterministic SP placement.
- The hostile death pipeline already existed pre-Batch-2 — `AAstrawildEchoCharacter::ApplyElementalDamage → OnDefeated → EventBus TAG_Astrawild_Event_HostileDefeated` (published when `EchoDefinition->bHostileToPlayers == true`) — so kills auto-increment the quest counter via `UAstrawildQuestComponent::ApplyEventToQuest` matching the `DefeatCreature` objective's `TargetId = Echo_Gloomfang`.
- **No new quest wiring was required** — the spawn subsystem is purely additive; the existing
  `DefeatCreature` event path is the integration point.
- REVIEW-2 caught a MEDIUM-risk population-clamp race (BeginPlay's `RegisterWithEcosystem` ran
  before `EchoDefinition` was set, so the species `WildCount` bump was being skipped →
  `GetWildPopulation` would have returned 0 forever → indefinite population leak). The one-line
  fix landed inline in `d5d23c2`: re-`RegisterEcho` immediately after `Echo->InitializeFromDefinition(Definition)` so the species `WildCount` bumps on the second call
  (the predicate check skips the redundant `RegisteredEchoes.Add` but the bump block is outside
  that predicate). See `ASTRAWILD_UE5_PRODUCTION_AUDIT.md` §22 for the full root-cause analysis.

Compile + runtime verification still required on the target machine.
