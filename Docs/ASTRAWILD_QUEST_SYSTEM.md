# ASTRAWILD — Quest System

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-29**
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
| `ReachLocation` | ⛔ NOT WIRED — enum + data support only (no event publisher yet) |
| `ObserveEcho` | ⛔ NOT WIRED — quest data uses it (see §3, Quest 2) but no `Event.*` publisher exists for observation today |
| `SurviveTime` | ⛔ NOT WIRED — no publisher |

**Honest consequence:** Quest_FirstEcho's "Observe a Lumewisp" objective will not progress through the
event bus in the current build; the quest is completable only via its capture objective + `AW.CaptureAll`
testing. Wiring `Event.*` publishers for these three types is on the fix list (see Test Plan §5).

Progress math: `ProgressCount = min(RequiredCount, Progress + max(1, Event.Amount))`; a quest completes
when **all** objectives are complete; completion is idempotent.

---

## 3. The First Dawn Chain (5 CODE_DEFAULT quests)

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

Rewards: **1 × Ancient Core**, **+20 RP**. Next: none — chain ends (open end for the next story arc).

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
support `ShopLootTableId` (future). Schedule/dialogue/faction behavior is PLANNED — the class is an
architecture-ready shell; no CODE_DEFAULT NPC definition or spawned NPC exists yet.

---

## 7. Not Implemented (honest)

| Feature | Status |
|---|---|
| Event publishers for ReachLocation / ObserveEcho / SurviveTime | NOT IMPLEMENTED (see §2) |
| Quest selection/abandonment UI | NOT IMPLEMENTED (single auto-chained active quest) |
| Multiple simultaneous active quests | NOT IMPLEMENTED (single `ActiveQuestId`) |
| Quest text presentation beyond the HUD tracker | NOT IMPLEMENTED (no dialogue UI) |
| Per-player quest replication for co-op clients | NOT IMPLEMENTED — component is not replicated; host/SP only today |
| NPC content | PLANNED (class + definition ready, no instances) |
