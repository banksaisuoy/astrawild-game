# ASTRAWILD — Native Gameplay Tags

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-29**
**Primary source:** `AstrawildGameplayTags.h` (declarations) / `AstrawildGameplayTags.cpp` (definitions)

All 77 tags are **native** (`UE_DECLARE_GAMEPLAY_TAG_EXTERN` / `UE_DEFINE_GAMEPLAY_TAG`) — no .ini asset
scanning, refactor-safe in C++. Variable names follow `TAG_Astrawild_<Group>_<Leaf>`.

---

## 1. Complete Taxonomy

### State.Creature.* (14) — Echo AI states (mirror of `EAstrawildEchoAIState`)

| Tag | Consumed by |
|---|---|
| `State.Creature.Idle` | AI state machine |
| `State.Creature.Explore` | AI state machine |
| `State.Creature.SearchFood` | AI state machine |
| `State.Creature.Eat` | reserved (no executor yet) |
| `State.Creature.Sleep` | AI state machine |
| `State.Creature.Socialize` | reserved (Social personality PLANNED) |
| `State.Creature.Investigate` | AI state machine (curious perception) |
| `State.Creature.Flee` | AI state machine |
| `State.Creature.Alert` | reserved |
| `State.Creature.Combat` | AI state machine |
| `State.Creature.Protect` | AI state machine |
| `State.Creature.Follow` | AI state machine |
| `State.Creature.Work` | AI state machine |
| `State.Creature.ReturnHome` | reserved |

### State.Player.* (4)

| Tag | Purpose |
|---|---|
| `State.Player.Dead` | player death state marker |
| `State.Player.Dodging` | i-frame window marker |
| `State.Player.Blocking` | block state marker |
| `State.Player.PlacingBuilding` | placement mode marker |

*(These four are defined for future tag-driven queries; current code uses booleans
(`bIsDead`/`bIsBlocking`) — tags are ready for GAS/StateTree adoption.)*

### Status.* (10) — status effects & vitals warnings

`Status.Poisoned`, `Status.Burning`, `Status.Frozen`, `Status.Wet`, `Status.Soaked`, `Status.Rested`,
`Status.Hungry`, `Status.Thirsty`, `Status.Cold`, `Status.Overheated`

### Element.* (6) — damage/creature elements (mirror of `EAstrawildElementType`)

`Element.None`, `Element.Light`, `Element.Ash`, `Element.Flora`, `Element.Frost`, `Element.Pulse`

### Damage.* (3) — damage channels

`Damage.Physical`, `Damage.Elemental`, `Damage.Fall`

### Item.* (6) — item categories (mirror of `EAstrawildItemCategory`)

`Item.Material`, `Item.Consumable`, `Item.Equipment`, `Item.Creature`, `Item.Building`, `Item.Quest`

### Interaction.* (6) — interaction kinds

`Interaction.Harvest`, `Interaction.Capture`, `Interaction.Craft`, `Interaction.Rest`, `Interaction.Talk`,
`Interaction.Scan`

### Biome.* (5) — world regions (master plan §2)

`Biome.DawnFields`, `Biome.LuminousRainforest`, `Biome.SaltPlains`, `Biome.AzureSnowline`,
`Biome.VeldaraRuins`

### Weather.* (8) — weather states (mirror of `EAstrawildWeatherState`)

`Weather.Clear`, `Weather.Cloudy`, `Weather.Rain`, `Weather.HeavyRain`, `Weather.Storm`, `Weather.Fog`,
`Weather.Heat`, `Weather.Cold`

### Event.* (10) — event bus payload types

| Tag | Published by | Consumed by |
|---|---|---|
| `Event.ItemCollected` | `InventoryComponent::AddItem` | quests (CollectItem) |
| `Event.EchoCaptured` | `EchoCharacter::Capture` | quests (CaptureEcho), roster |
| `Event.EchoDefeated` | `EchoCharacter::ApplyElementalDamage` | quests (DefeatCreature), ecosystem |
| `Event.HostileDefeated` | same (hostile species variant) | quests (DefeatCreature) |
| `Event.BuildingPlaced` | `BuildingComponent::ServerPlaceBuilding` | quests (PlaceBuilding) |
| `Event.TechUnlocked` | `ResearchSubsystem::TryUnlockTech` | quests (UnlockTechnology) |
| `Event.LocationReached` | **no publisher yet** | reserved (ReachLocation quests) |
| `Event.QuestObjectiveCompleted` | **no publisher yet** | reserved (analytics/audio) |
| `Event.RecipeCrafted` | `CraftingComponent` (instant + timed completion) | quests (CraftRecipe) |
| `Event.EchoFed` | `EchoCharacter::Feed` | reserved (journal/audio) |

### Faction.* (3)

`Faction.Wild`, `Faction.Player`, `Faction.Hostile`

### Gameplay.* (2)

`Gameplay.Debug`, `Gameplay.Cheat`

**Total: 77 tags** (14 + 4 + 10 + 6 + 3 + 6 + 6 + 5 + 8 + 10 + 3 + 2).

---

## 2. Naming Conventions

1. `Group.Leaf` — exactly two segments today; a third level is allowed for future granularity
   (e.g. `Event.ItemCollected.Rare`) but no tag uses it yet.
2. Groups are **singular domain nouns** (`State`, `Status`, `Element`, `Event`…); leaves are PascalCase.
3. Enums that exist both as UENUM and tags (AI states, elements, weather, item categories) keep **identical
   leaf names** — `EAstrawildWeatherState::HeavyRain` ↔ `Weather.HeavyRain` — so conversion helpers stay
   trivial (a mapping function is a future utility; today the event bus and weather enum are separate paths).
4. C++ symbol: `TAG_Astrawild_<Group>_<Leaf>` (e.g. `TAG_Astrawild_Event_EchoCaptured`).
5. Tags are **additive** — never rename or delete a shipped tag (save files / network dictionaries may
   reference it); deprecate by comment instead.

---

## 3. How to Add New Tags

1. **Declare** in `AstrawildGameplayTags.h` inside the matching group block:
   ```cpp
   UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Astrawild_Status_Bleeding);
   ```
2. **Define** in `AstrawildGameplayTags.cpp`:
   ```cpp
   UE_DEFINE_GAMEPLAY_TAG(TAG_Astrawild_Status_Bleeding, "Status.Bleeding");
   ```
3. Add a row to the table in this document (keep the doc as the taxonomy source of truth).
4. If the tag is an event type: document publisher + consumers in the Event table; add the publisher call
   (`EventBus->PublishEvent(tag, instigator, targetId, amount, location)`) on the **server**.
5. New *groups* require: header comment section, this-document section, and a check that no `.ini`
   duplicate exists (native tags must not also be declared in `Config/DefaultGameplayTags.ini` — the
   project currently defines zero ini tags by design).

---

## 4. Usage Notes

- Event bus filtering is by exact tag equality today (`Event.EventTag == TAG_Astrawild_Event_*`);
  `MatchesTag`/container queries are available for hierarchical filtering when third-level tags appear.
- Tags currently **drive** the event bus and are **mirrored** by enums for replicated UPROPERTYs
  (enums replicate cheaper than FGameplayTag in UE 5.8 defaults; tags are compile-time constants in C++).
- `Gameplay.Debug`/`Gameplay.Cheat` exist to gate debug draws and cheat verification once systems adopt
  tag-gated execution (not yet consumed).
