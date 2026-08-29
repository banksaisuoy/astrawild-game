# ASTRAWILD — Echo AI Architecture

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-29**
**Primary sources:** `AstrawildEchoAIController.h/.cpp`, `AstrawildEcosystemSubsystem.cpp`,
`AstrawildEchoCharacter.cpp`, `AstrawildEchoAIController` tunables

`AAstrawildEchoAIController` is a **server-driven, asset-free AI**: AI Perception (sight) + a C++ state
machine. It requires **zero Behavior Tree assets** to function; future BT/StateTree assets can drive the same
states through the documented blackboard contract (§6).

---

## 1. Sight Perception Configuration

Configured in the controller constructor, then **re-scaled on possess** from the species definition ×
personality aggro multiplier:

| Parameter | Constructor default | On possess |
|---|---|---|
| `SightRadius` | 1500 cm | `EchoDefinition->SightRadius × GetAggroRadiusMultiplier()` |
| `LoseSightRadius` | 2200 cm | `EchoDefinition->LoseSightRadius × GetAggroRadiusMultiplier()` |
| `PeripheralVisionAngleDegrees` | 75° | fixed |
| `DetectionByAffiliation` | Enemies + Neutrals + Friendlies = true | fixed |
| `MaxAge` (stimulus memory) | 6.0 s | fixed |
| Dominant sense | Sight | fixed |

Personality examples (aggro multiplier): Aggressive ×1.5, Brave ×1.2, default ×1.0, Timid ×0.5.

**Perception handler** (`HandlePerception`): only successfully-sensed `AAstrawildPlayerCharacter` actors
register as `bPerceivedThreat`. A **wild Curious** Echo (health > 0.8, currently Idle/Explore) transitions to
`Investigate` and stores the player as `TargetActor` instead of reacting — the observation-friendly first
companion behavior.

---

## 2. The 16-State Machine

`EAstrawildEchoAIState`: `Idle`, `Explore`, `SearchFood`, `Eat`, `Sleep`, `Socialize`, `Investigate`,
`Flee`, `Alert`, `Combat`, `Protect`, `Follow`, `Work`, `ReturnHome`, `Injured`, `Dead`.

Implemented executors (state → behavior):

| State | Executor behavior |
|---|---|
| `Idle` / `Stay` / `HoldPosition` | `StopMovement()` |
| `Explore` | Wander to a random point within `ExploreRadius` (1500 cm) of `HomeLocation`, new target every 2–6 s |
| `Flee` | If a player is within 3000 cm: move 1200 cm directly away; else retreat to `HomeLocation` |
| `Combat` | Acquire nearest player (≤ 4000 cm) as target; chase to `AttackRange` (180 cm), then attack on cooldown 1.6 s for `AttackPower × AttackDamageMultiplier` damage using the species element |
| `Follow` | Captured: follow owner at `FollowDistance` 280 cm (stop when close, catch up when > 1.5×). Wild investigate: approach `TargetActor`, stop within 250 cm |
| `Protect` | Find nearest hostile Echo within 800 cm of the owner → intercept and attack; otherwise stay near owner |
| `Work` | Move to `AssignedWorkSite`, stop inside `WorkRange` — production math runs on the site itself |
| `Sleep` | Stop movement; recover Energy **+2 per think-second** |
| `SearchFood` | Explore wander + grazing trickle (+0.1 hunger per think) |
| `Investigate` | Routes to `ExecuteFollow` (moves toward the investigate target) |
| `Eat`, `Socialize`, `Alert`, `ReturnHome`, `Injured`, `Dead` | Enum values exist; no dedicated executor yet (default → stop movement) — PLANNED refinement |

The state lives on the **Echo** (`CurrentAIState`, replicated for client-side feedback) and the controller
writes it through `TransitionTo` on every think.

---

## 3. Decision Tree — `DecideState()`

Evaluated top-down each think; first match wins.

```
1. CAPTURED? (bCaptured)
   └─ Command switch (directive §10 — commands override everything):
        Follow ──────────────► Follow
        Stay / HoldPosition ─► Idle
        Retreat ─────────────► Follow
        Defend ──────────────► Protect
        Attack ──────────────► Combat (if TargetActor valid) else Follow
        Work ────────────────► Work (if AssignedWorkSite valid) else Follow

2. WILD:
   a. Needs.IsCritical() (Hunger ≤ 15 or Energy ≤ 10)  ──► SearchFood   (overrides everything)
   b. HealthFraction ≤ clamp(0.30 × FleeHealthThresholdMultiplier, 0.05, 0.90) ──► Flee
   c. Hostile species (bHostileToPlayers) AND bPerceivedThreat ──► Combat
   d. Outside species activity window (IsCurrentlyActiveTime == false) ──► Sleep
   e. Hunger < 40 ──► SearchFood
   f. Currently Investigating with valid TargetActor ──► Investigate (sticky)
   g. default ──► Explore
```

Activity windows (`IsCurrentlyActiveTime`, hour = world time of day):
- **Diurnal**: 05:30 ≤ hour < 19:30
- **Nocturnal**: hour < 05:30 or hour ≥ 19:30
- **Crepuscular**: 05:00–08:00 or 17:00–20:30

---

## 4. Personality Modulation in AI

| Hook | Where | Effect |
|---|---|---|
| `GetFleeHealthThresholdMultiplier()` | decision 2b | Timid 1.8 → flees at 54 % HP; Brave 0.4 → 12 %; Aggressive 0.5; Protective 0.6; others 1.0 (30 %) |
| `GetAggroRadiusMultiplier()` | perception config | Aggressive 1.5, Brave 1.2, Timid 0.5 — literally resizes sight radii |
| Curious | perception handler | Wild curious Echoes investigate instead of fleeing/aggroing |
| `GetCommandObedience()` | `IssueCommand` (character) | Command acceptance roll; Independent 0.5 / Lazy 0.6 / Protective 0.95 / Loyal 1.0 base, + trust/bond bonuses |
| Damage events | `OnDamaged` delegate | Exposed for future reaction logic (currently the flee check in the decision tree reacts next think) |

---

## 5. Think Loop & LOD Rates

- The controller does **not** tick (`PrimaryActorTick.bCanEverTick = false`); `Think()` is timer-scheduled.
- Base `ThinkIntervalSeconds = 0.25` (4 Hz). `DeltaThink` passed to executors is this constant.
- LOD: `Think` queries `UAstrawildEcosystemSubsystem::GetRecommendedUpdateInterval(GetTierForEcho(Echo))`
  and computes `Interval = max(ThinkIntervalSeconds, TierInterval)`.

Tier intervals (`GetRecommendedUpdateInterval`):

| Ecosystem tier | Distance (nearest player) | Recommended interval | Effective think rate |
|---|---|---|---|
| Tier 0 — Full | ≤ 3000 cm | 0.0 s | 0.25 s (4 Hz) |
| Tier 1 — Reduced | ≤ 8000 cm | 0.25 s | 0.25 s (4 Hz) |
| Tier 2 — Statistical | ≤ 20000 cm | 1.0 s (movement disabled) | 1.0 s (1 Hz) |
| Tier 3 — World | > 20000 cm | 5.0 s | 5.0 s |

**Known implementation gap (honest):** the computed `Interval` is currently **not applied** to the
reschedule — `Think()` re-arms with `SetTimerForNextTick`, so the think step actually runs once per frame and
the `Interval` local is dead code. Consequence: AI decision cost is per-frame for all Echoes (bounded by the
cheap decision function), and executor deltas use the 0.25 s constant. Fix planned: replace
`SetTimerForNextTick` with `SetTimer(Interval)` on the target machine iteration — tracked in
`ASTRAWILD_PERFORMANCE.md` §Known Issues.

---

## 6. Blackboard Key Contract (future BT assets)

When Behavior Tree / StateTree assets are authored, they must read/write exactly these keys:

| Key | Type | Meaning |
|---|---|---|
| `TargetActor` | `AActor*` | Combat/follow/investigate target (player, hostile Echo) |
| `HomeLocation` | `FVector` | Spawn anchor captured on possess; wander center |
| `AIState` | `uint8` (as `EAstrawildEchoAIState`) | Current state — mirrors the replicated `CurrentAIState` on the Echo |

The C++ controller already maintains these three values internally (`TargetActor`, `HomeLocation`,
`GetAIState()`), so a BT can be swapped in by writing them into the blackboard on possess and delegating
state execution to BT tasks. No BT asset exists yet — **PLANNED**.

---

## 7. Combat Interaction

`TryAttackTarget(Target, DeltaThinkSeconds)`:
- Cooldown gate: `AttackCooldownSeconds = 1.6` (per controller, shared across targets).
- Damage: `Echo->GetAttackPower() × AttackDamageMultiplier (1.0)`, delivered through
  `ApplyElementalDamage` with the **species element** (so Light-element Echoes hit Frost-weak targets harder).
- Vs players: routes through `CombatComponent->GetMitigatedIncomingDamage` (dodge i-frames → 0; block →
  ×(1 − 0.65)) then `SurvivalComponent->ApplyDamage`.

Hostile-vs-protective interplay: `ExecuteProtect` scans all Echoes for hostiles within 800 cm of the owner
and intercepts — captured Stonehide/Lumewisp bodyguards are emergent from this.

---

## 8. Configuration Reference (controller tunables)

| Property | Default | Note |
|---|---|---|
| `ThinkIntervalSeconds` | 0.25 | Base think cadence |
| `BaseFleeHealthFraction` | 0.30 | Wild flee threshold before personality |
| `AttackRange` | 180 cm | Melee reach |
| `AttackCooldownSeconds` | 1.6 | Attack cadence |
| `AttackDamageMultiplier` | 1.0 | Global damage scale |
| `FollowDistance` | 280 cm | Party follow distance |
| `ExploreRadius` | 1500 cm | Wander radius from home |
