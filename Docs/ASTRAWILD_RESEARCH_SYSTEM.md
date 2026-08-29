# ASTRAWILD — Research & Technology System

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-29**
**Primary sources:** `AstrawildResearchSubsystem.h/.cpp`, `AstrawildContentLibrary.cpp::BuildTechnologies()`,
`AstrawildJournalSubsystem.cpp` (point sources), `AstrawildQuestComponent.cpp` (quest rewards)

`UAstrawildResearchSubsystem` is a **GameInstance-scoped** subsystem: one research pool per session.
This is the documented co-op decision — in multiplayer, the pool is **shared by all players** (see
`ASTRAWILD_MULTIPLAYER.md` §4 and `ASTRAWILD_ASSUMPTIONS.md`).

---

## 1. Research Point Sources

| Source | Amount | Code path |
|---|---|---|
| Field journal knowledge milestones | **+2 per milestone** (4 milestones per species: 25 %, 50 %, 75 %, 100 % observation) | `JournalSubsystem::GrantKnowledgeMilestones` → `AddResearchPoints(2)` |
| Quest rewards | Per quest (see §3) | `QuestComponent::GrantRewards` → `AddResearchPoints(RewardResearchPoints)` |
| Cheat | `AW.ResearchPoints N` | testing only |

Full-journaling all 7 species = 56 points total; the First Dawn quest chain awards 80 points across its
six quests (wave 2 adds `Tech_Husbandry` at 10 RP — Cooking 5 + Husbandry 10 + Electrical 15 = 30 points
for the tree, comfortably affordable through moderate journaling and quests).

---

## 2. The Tech Tree (5 CODE_DEFAULT nodes)

From `AstrawildContentLibrary.cpp::BuildTechnologies()`:

| Tech | Id | Era | Cost (RP) | Prerequisites | Unlocks recipes | Unlocks buildings |
|---|---|---|---|---|---|---|
| Basic Crafting | `Tech_BasicCrafting` | Primitive | **0** | — (free/implicit) | — | — |
| Cooking | `Tech_Cooking` | Primitive | **5** | `Tech_BasicCrafting` | `Recipe_CookedMeat` | — |
| Electrical Foundations | `Tech_Electrical` | Electrical | **15** | `Tech_BasicCrafting` | — | `Building_Generator`, `Building_Battery`, `Building_LampPost` |
| Advanced Energy | `Tech_AdvancedEnergy` | AdvancedEnergy | **30** | `Tech_Electrical` | — (future content) | — (future content) |
| Echo Husbandry | `Tech_Husbandry` | Primitive | **10** | `Tech_Cooking` | `Recipe_FeedMix`, `Recipe_HerbalSalve` | `Building_FeedTrough` |

`Tech_BasicCrafting` costs 0 and gates nothing itself — it is the root identity that other nodes reference.

---

## 3. Quest Research Rewards (First Dawn chain)

| Quest | RewardResearchPoints |
|---|---|
| Quest_FirstLight | 5 |
| Quest_FirstEcho | 10 |
| Quest_Homeground | 10 |
| Quest_Spark | 15 |
| Quest_DawnGuard | 20 |
| **Total** | **60** |

`Quest_Spark` also *requires* unlocking `Tech_Electrical` as an objective, and quest definitions can carry
`RewardTechId` for instant story-driven unlocks (unused by current content — data path exists and is tested
by `GrantRewards`).

---

## 4. TryUnlockTech Flow

```
TryUnlockTech(TechId)                       [BlueprintCallable; no explicit authority guard —
                                            GameInstance subsystem, intended for server/SP use]
  └─ CanUnlockTech?
        ├─ already unlocked?           → false
        ├─ GetMissingPrerequisites(TechId) non-empty? → false
        │     (missing = prereqs listed on the definition not in UnlockedTechIds;
        │      unknown TechId → returns [TechId] itself → blocked)
        └─ ResearchPoints ≥ Tech->ResearchCost?  → else false
  ├─ ResearchPoints −= cost
  ├─ UnlockedTechIds.Add(TechId)
  ├─ OnTechUnlocked(TechId, Definition) broadcast
  ├─ OnResearchPointsChanged broadcast
  └─ EventBus: Event.TechUnlocked        [drives quest objective "Unlock Technology"]
```

Delegates: `OnTechUnlocked` (for UI toast/tree refresh), `OnResearchPointsChanged`.
Logging: `LogAstrawildEconomy`.

---

## 5. What Tech Gates

| Gated thing | Check | Effect |
|---|---|---|
| Recipes (`RequiredTechId`) | `CraftingComponent::CanCraft` → `IsTechUnlocked` | Recipe uncraftable until unlocked (e.g. Seared Meat needs `Tech_Cooking`) |
| Buildings (`RequiredTechId`) | `ItemRegistrySubsystem::GetUnlockedBuildings` → placement list | Building not placeable until unlocked (Generator/Battery/Lamp need `Tech_Electrical`) |
| Quest objectives | `Event.TechUnlocked` matching objective `TargetId` | Quest_Spark "Unlock Electrical Foundations" |
| `RewardTechId` on quests | `TryUnlockTech(RewardTechId)` | Instant unlock on quest completion (data path ready) |

---

## 6. Persistence

`ExportForSave` / `ImportFromSave` round-trip `FAstrawildResearchSaveData { UnlockedTechIds, ResearchPoints }`
inside schema v2 (`SaveGame->Research`). The GameInstance scope means points survive level transitions and
respawns, and (per the co-op decision) are shared across players in a session.

---

## 7. Not Implemented (honest)

| Feature | Status |
|---|---|
| Tech tree UI | NOT IMPLEMENTED (unlocks happen through play; `AW.UnlockTech` cheat for testing) |
| Era progression beyond 4 nodes | PLANNED (AdvancedEnergy unlocks nothing yet — placeholder for future content) |
| Research by Echo assist (Duskmoth ResearchAssist ×1.6 affinity) | Work sites produce items; a research-point-producing site is NOT IMPLEMENTED |
| Per-player research in co-op | DELIBERATELY NOT per-player — shared pool decision (see Assumptions) |
