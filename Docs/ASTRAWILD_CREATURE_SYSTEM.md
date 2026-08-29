# ASTRAWILD — Echo Creature System

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-29**
**Primary sources:** `AstrawildEchoCharacter.h/.cpp`, `AstrawildDataAssets.h` (UAstrawildEchoDefinition),
`AstrawildContentLibrary.cpp`, `AstrawildCaptureComponent.cpp`, `AstrawildJournalSubsystem.cpp`,
`AstrawildEchoRosterSubsystem.cpp`, `AstrawildWorkSiteActor.cpp`

Echoes are the heart of ASTRAWILD. One class — `AAstrawildEchoCharacter` — serves both **wild** and
**captured** instances. Behavior is the product of a *Definition* (species template) and *Instance state*
(personality roll, needs, trust, bond, commands).

---

## 1. Definition vs Instance

### 1.1 Species template — `UAstrawildEchoDefinition` (data asset)

| Field group | Fields (defaults from C++ struct) |
|---|---|
| Identity | `DefinitionId`, `DisplayName`, `Description`, `Element` (None/Light/Ash/Flora/Frost/Pulse), `Role` (Explorer/Combat/Base/Support) |
| Combat stats | `BaseStats` (`FAstrawildEchoStats`): MaxHealth 100, AttackPower 10, Defense 5, MoveSpeed 300, Stamina 100, CaptureResilience 0.35 |
| Capture | `CaptureDifficulty` (0..1, default 0.4), `TrustGainOnCapture` (default 10), `PreferredFoodIds`, `PreferredWeather` |
| Behavior | `DominantPersonality`, `ActivityPattern` (Diurnal/Nocturnal/Crepuscular), `HabitatBiomeIds`, `bHostileToPlayers` |
| Elemental combat | `WeaknessElement` (×1.5 damage taken), `ElementalResistance` (default 0.2 flat reduction vs own element) |
| Work | `WorkAffinities[]` (`EAstrawildWorkType` + `Affinity` 0..2, 1 = baseline) |
| Needs | `HungerDecayPerHour` (default 4.0), `EnergyDecayPerHour` (default 6.0) — per in-world hour |
| Growth | `BaseExperienceToLevel` (default 100, scales ×level) |
| Loot | `DefeatLoot[]` (`FAstrawildItemStack` array) |
| AI | `SightRadius` (default 1500 cm), `LoseSightRadius` (default 2200 cm) |
| Visuals | `SkeletalMesh`, `Icon` (soft pointers — empty for CODE_DEFAULT species; engine sphere placeholder used) |

### 1.2 Instance state — `AAstrawildEchoCharacter`

| State | Type | Notes |
|---|---|---|
| `InstanceId` | FGuid | Stable across save/load |
| `Level` / `Experience` | int/float | Level curve: `BaseExperienceToLevel × Level`; level-up full-heals; +10 % max HP and +8 % attack **per level above 1** |
| `Personality` | enum (replicated) | Rolled on spawn: **70 %** species `DominantPersonality`, **30 %** random archetype |
| `Trust` | float | Grows via feeding (+definition `TrustGainOnCapture` on capture); feeds capture chance & obedience |
| `Bond` | 0..100 | +25 % of each feed's trust gain; grows passively (+0.2/in-world-hour) while captured; raises obedience |
| `Needs` | `FAstrawildEchoNeeds` (replicated) | Hunger 100 / Energy 100 / Mood 80 at spawn; critical when Hunger ≤ 15 or Energy ≤ 10 |
| `bCaptured` | bool | Flips AI from wild tree to command tree |
| `ActiveCommand` | enum (replicated) | Follow/Attack/Defend/Stay/Retreat/HoldPosition/Work |
| `OwnerPlayerId` | FName (replicated) | Set at capture; gates party command broadcast |
| `AssignedWorkSite` | weak ptr | Set by `AAstrawildWorkSiteActor::AssignWorker` |
| `CurrentAIState` | enum (replicated) | Drives client-side feedback (e.g. nameplates) |

**Needs decay math** (server only, throttled by ecosystem LOD tier): decay-per-in-world-hour is converted
using `TimeSubsystem.MinutesPerRealSecond` (1.0 → 1 real sec = 1 world min). Captured Echoes burn needs at
×0.6 (cared for). Mood drops 2.0/in-world-hour while Hunger < 30, else slowly recovers (−0.5). Critical needs
deal 1.0 HP/s (never below 1 HP — soft pressure, not death).

---

## 2. The 5 CODE_DEFAULT Species

All values below are read from `AstrawildContentLibrary.cpp::BuildEchoes()`. `CaptureResilience` is derived:
`Clamp(CaptureDifficulty × 0.8, 0.05, 0.95)`.

| | Lumewisp | Stonehide | Voltling | Duskmoth | Gloomfang |
|---|---|---|---|---|---|
| `DefinitionId` | `Echo_Lumewisp` | `Echo_Stonehide` | `Echo_Voltling` | `Echo_Duskmoth` | `Echo_Gloomfang` |
| Element | Light | Ash | Pulse | Flora | Ash |
| Role | Support | Combat | Base | Support | Combat |
| MaxHealth | 60 | 140 | 55 | 45 | 110 |
| AttackPower | 8 | 16 | 10 | 6 | 18 |
| Defense | 2 | 8 | 3 | 2 | 4 |
| MoveSpeed | 320 | 260 | 380 | 300 | 420 |
| Dominant personality | Curious | Brave | Energetic | Timid | Aggressive |
| Activity pattern | Diurnal | Diurnal | Nocturnal | Crepuscular | Nocturnal |
| Preferred food | Glimmer Berry | Glimmer Berry | Glimmer Berry | Glimmer Berry | — (none) |
| CaptureDifficulty | 0.25 | 0.55 | 0.45 | 0.35 | 0.85 |
| CaptureResilience (derived) | 0.20 | 0.44 | 0.36 | 0.28 | 0.68 |
| Weakness element | Ash | Light | Frost | Frost | Light |
| Hostile to players | No | No | No | No | **Yes** |
| Preferred weather | Clear, Cloudy | — | — | — | — |
| Habitat | `Biome_DawnFields` | — | — | — | — |
| Work affinity | Gathering ×1.2 | Mining ×1.8 | PowerGeneration ×2.0 | ResearchAssist ×1.6 | — |
| Defeat loot | 1 Sunfiber | 2 Fieldstone | — | — | 2 Raw Meat + 1 Dawn Crystal Shard |
| Design intent | First companion (docile, curious) | Brave tank | Power synergy (nocturnal dynamo) | Shy researcher | First hostile — night stalker |

Notes:
- Species with no `PreferredWeather`/`HabitatBiomeIds` set simply don't get those capture bonuses yet.
- Voltling's PowerGeneration ×2.0 pairs with the `Building_Generator` ("Echo Dynamo") concept; the work
  site power bonus is grid-level (×1.5 production when powered), not per-species.
- All species share `TrustGainOnCapture = 10`.

---

## 3. Personality — 10 Archetypes

Enum `EAstrawildPersonality` (AstrawildTypes.h). Effects are real multipliers consumed by the AI controller
and work sites — not flavor text.

| Archetype | Flee health threshold multiplier | Aggro radius multiplier | Work speed multiplier | Base obedience | Other coded behavior |
|---|---|---|---|---|---|
| **Brave** | **0.4** (stands ground) | **1.2** | 1.0 | 0.8 | — |
| **Timid** | **1.8** (flees early) | **0.5** | 1.0 | 0.8 | — |
| **Aggressive** | **0.5** | **1.5** (aggro from farther) | 1.0 | 0.8 | — |
| **Curious** | 1.0 | 1.0 | 1.0 | 0.8 | Investigates unknown players instead of reacting (AI perception hook) |
| **Loyal** | 1.0 | 1.0 | **1.15** | **1.0** | — |
| **Lazy** | 1.0 | 1.0 | **0.6** | **0.6** | — |
| **Energetic** | 1.0 | 1.0 | **1.4** | 0.8 | — |
| **Protective** | **0.6** | 1.0 | 1.0 | **0.95** | `Defend` command prioritizes intercepting hostiles near owner |
| **Independent** | 1.0 | 1.0 | 1.0 | **0.5** | — |
| **Social** | 1.0 | 1.0 | 1.0 | 0.8 | Herd/mood-buff behavior PLANNED (enum ready, not yet wired) |

- Flee threshold base = 0.30 health fraction (`BaseFleeHealthFraction` on the AI controller), clamped 0.05–0.9
  after personality scaling. Timid ×1.8 → flees at 54 % HP; Brave ×0.4 → flees at 12 % HP.
- Aggro radius multiplies the species `SightRadius`/`LoseSightRadius` at perception configuration time.
- **Obedience** adds trust/bond modifiers: `+Clamp(Trust/200, 0, 0.25) + Clamp(Bond/400, 0, 0.25)`, clamped
  0.1–1.0. `IssueCommand` rolls against obedience — disloyal Echoes ignore commands.

---

## 4. Feeding, Trust & Bond

`Feed(FoodItemId, FeedValue)` (server):

- Preferred food (species `PreferredFoodIds`) → **×2.0 multiplier**; otherwise ×1.0.
- Trust += `FeedValue × multiplier` (player feed action uses 8.0 base for preferred items; registry
  `EchoFeedValue` used as fallback, e.g. Glimmer Berry = 6.0, Raw Meat = 5.0).
- Bond += 25 % of the trust gain (clamped 0–100).
- Hunger += 30 × multiplier, Mood += 10 × multiplier (clamped 0–100).
- Publishes `Event.EchoFed` on the bus.

Player-side `FeedTarget` (R key): consumes the first preferred food in inventory, else the first item with
`EchoFeedValue > 0`.

---

## 5. Capture Pipeline (summary — see also AI doc)

Chance = `ComputeCaptureChance()` on the Echo + component bonuses:

```
Base        = 0.05 × (1 − 0.5 × Difficulty)
WeakenBonus = (1 − HealthFraction) × (1 − Resilience) × (1 − 0.5 × Difficulty)
TrustBonus  = Clamp(Trust/100) × 0.5
Situational = +0.10 if current weather ∈ species PreferredWeather
              +0.05 if current time is inside the species activity window
Chance      = clamp(Base + Weaken + Trust + Situational, 0.02, 0.95)
```

Component adds: **+0.15 × (journal ObservationProgress / 100)** and **+0.05** while tracking (within 900 cm),
final clamp 0.95. Requirements: an `Item_Resonator` is consumed per attempt (`CaptureCooldownSeconds = 1.0`);
defeated Echoes can never be captured (chance 0). Design invariants: *never* capturable reliably at full
health with zero trust; weakening, feeding, observing and tracking all stack toward success.

---

## 6. Field Journal (observation = progression)

`UAstrawildJournalSubsystem` (server): for each player, any Echo within **1400 cm** and a **≥0.75 dot**
(~41° half-angle view cone) accrues observation progress at **5 %/s**. Milestones per species:

| Observation | Unlock | Reward |
|---|---|---|
| 25 % | `bScanned` | +2 research points |
| 50 % | `bFoodDiscovered` | +2 research points |
| 75 % | `bWeaknessDiscovered` | +2 research points |
| 100 % | `bHabitatDiscovered` | +2 research points (and the full +15 % capture bonus via the capture component) |

Journal entries (`FAstrawildJournalEntry`) save with the world (schema v2).

---

## 7. Roster & Party

`UAstrawildEchoRosterSubsystem` (GameInstance scope):

- `AddToRoster` — requires `bCaptured` + valid `InstanceId`; stores `FAstrawildEchoInstanceV2`;
  tracks spawned party actors up to **`MaxPartySize = 3`**.
- `ExportForSave` refreshes stored entries from live actors (current transform/needs), then saves.
- `ImportFromSave` restores roster data; `LoadWorld` despawns the current party and lets roster data persist.
- Party commands: player cycles `Follow → Attack → Defend → Stay → Work → …` (C key); broadcast to every
  captured Echo with matching `OwnerPlayerId`; each Echo rolls obedience.

---

## 8. Echo Work (base jobs)

`AAstrawildWorkSiteActor` production math per tick (server):

```
rate  = Affinity(species) × PersonalityMultiplier × MoodMult(0.5..1) × EnergyMult(0.5..1) × PowerMult
PowerMult = 1.5 if grid-powered, 1.0 if power not required, 0.0 if required and unpowered
Affinity  = species WorkAffinities match → its value, else 0.5 baseline
```

- Working drains 0.5 energy/s. Every `SecondsPerOutput` accumulated seconds produce 1 item into
  `StoredOutput` (replicated) with an `OnWorkProduced` broadcast. Bootstrapper sites: Gathering →
  Sunfiber @10 s; Farming → Glimmer Berry @14 s (both `bRequiresPower = false`, `WorkRange = 250` cm).
- `CollectOutput()` returns and clears accumulated output (player-facing pickup interact **not yet wired**).

---

## 9. Save Data

Each Echo serializes to `FAstrawildEchoInstanceV2` (instance id, definition id, personality, level, XP,
trust, bond, needs, transform, in-party flag). Wild Echoes are **not** individually persisted — only the
captured roster is; wild populations re-spawn through the bootstrapper each session. v1 saves migrate
roster entries to v2 with default `Curious` personality (see Save doc).

---

## 10. Known Gaps (honest)

| Gap | Status |
|---|---|
| Visuals | Engine sphere placeholder (`/Engine/BasicShapes/Sphere`, scale 0.8) for all species — REPLACE_BEFORE_RELEASE |
| Social herd behavior, mood buff | PLANNED (enum exists, no logic) |
| Field abilities (Explore role traversal etc.) | PLANNED — `AbilityIds` array exists on the definition, no runtime consumer |
| Loot tables (`UAstrawildLootTableDefinition`) | Data contract only; species use `DefeatLoot` directly |
| Echo-attacks-player elemental mitigation vs player element | Players take mitigated raw damage (block/dodge only); player-side elemental resistance NOT IMPLEMENTED |
