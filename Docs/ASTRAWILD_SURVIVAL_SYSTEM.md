# ASTRAWILD — Survival System

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-30** (wave 6 sync — sprint stamina drain + thirst rate fix + block penalty live, Batch 4)
**Primary sources:** `AstrawildSurvivalComponent.h/.cpp`, `AstrawildPlayerCharacter.cpp` (sprint wiring,
movement-speed modifiers), `AstrawildWeatherSubsystem.cpp` (temperature
profiles), `AstrawildGameMode.cpp` (respawn), `AstrawildContentLibrary.cpp` (food/water items),
`AstrawildRestPoint.cpp`, `AstrawildCombatComponent.h` (BlockSpeedMultiplier)

`UAstrawildSurvivalComponent` owns the player's vitals. It ticks **server-side only**; clients observe the
replicated `Stats` struct via `OnRep_Stats` → `OnStatsChanged`.

Design goal (header comment): hunger/thirst decay slowly enough to support exploration instead of
menu-spam — roughly **20 real minutes** from full to empty at default rates.

---

## 1. Vitals & Actual Rates

`FAstrawildSurvivalStats` (replicated as one struct): Health 100/100, Stamina 100/100, Hunger 100,
Thirst 100, Temperature 20 °C, `bIsDead`.

| Vital | Rate (server tick) | Property |
|---|---|---|
| Hunger decay | **0.083 /s** (≈5/min → 100→0 in ~20.1 min) | `HungerDecayPerSecond` |
| Thirst decay | **0.0833 /s** (≈5/min → 100→0 in ~20.0 min) — **Batch 4 (L-2)**: was 0.14/s (~11.9 min), contradicting the documented ~20-min goal; header comment records the fix | `ThirstDecayPerSecond` |
| Stamina regen | **+14 /s** passive — suspended while the sprint drain is active (see §1.1) | `StaminaRegenPerSecond` |
| Sprint stamina drain | **−7 /s** while sprint-drain is armed AND the owner is actually moving (Batch 4 — M-2a, see §1.1) | `SprintStaminaDrainPerSecond` |
| Starvation damage | **1.5 /s per depleted vital** (both empty = 3.0 /s) | `StarvationHealthDamagePerSecond` |
| Cold exposure damage | 1.0 /s while `Temperature ≤ 4 °C` | `ExposureHealthDamagePerSecond` |
| Heat exposure damage | 1.0 /s while `Temperature ≥ 36 °C` | `ExposureHealthDamagePerSecond` |

Action stamina costs (see Combat doc): heavy attack 25, dodge 22 — plus the continuous sprint drain
below.

### 1.1 Sprint stamina drain (Batch 4 — M-2a)

Sprinting was previously gate-only (speed switch + "cannot sprint below 5 % stamina"), which made the
exhaustion rule unreachable in normal play — sprinting itself never spent stamina. Batch 4 adds a real
drain economy:

- **Drain rate:** `SprintStaminaDrainPerSecond = 7.0` (tunable) → **≈14 s** of sprinting from full
  100 stamina (100 / 7 ≈ 14.3 s).
- **Moving-only drain:** the drain ticks only while `bSprintDrainActive` is armed AND
  `IsOwnerMoving()` reports real movement (velocity² > 25² cm/s — walk 450 / sprint 700 both
  qualify). Holding the sprint key while standing still drains nothing.
- **Drain INSTEAD of regen:** while draining, the passive regen (+14/s) is suspended — otherwise regen
  would out-pace the drain (14 > 7) and sprinting would be free. Regen resumes the tick after the
  drain ends.
- **Exhaustion behavior:** when stamina hits the floor, the component clears the drain flag and
  broadcasts `OnSprintExhausted` **once**; the player character drops `bSprinting`, clears the drain
  request and calls `RefreshMovementSpeed()` — the existing >0.05 stamina-fraction gate keeps
  re-sprint suppressed until stamina recovers.
- **Wiring:** `StartSprint` arms the drain (`SetSprintDrainActive(true)`), `StopSprint` clears it;
  `OnPlayerDied` clears both (respawn `FullRestore` refills stamina — a stale drain request would
  immediately drain it again).
- **Server-side only:** `bSprintDrainActive` is not replicated — it feeds the server stamina economy;
  sprint SPEED is applied locally by `RefreshMovementSpeed` on each client.

### 1.2 Movement-speed modifiers applied by `RefreshMovementSpeed`

| Modifier | Value | Live since |
|---|---|---|
| Walk / Sprint base | 450 / 700 cm/s | foundation |
| Sprint gate | disabled at ≤ 5 % stamina fraction | foundation |
| Sprint drain (stamina economy) | −7 /s while moving (§1.1) | **Batch 4** |
| Block penalty | **×0.45** (`CombatComponent.h:83 BlockSpeedMultiplier`) — applied to the walk/sprint target while `IsBlocking()`; previously DEAD CODE (`OnBlockingChanged` had no listener), now bound via `BeginPlay → OnBlockingChanged(bool) → RefreshMovementSpeed()` | **Batch 4 (M-2b)** — live |
| Stagger | 0 while staggering | Batch 3 |
| Status slows (Chill ×0.5 / Shock ×0.3, multiplicative) | ×multiplier | Batch 3 |

---

## 2. Temperature Model

```
Temperature = 20 °C (temperate base) + WeatherSubsystem.GetTemperatureOffsetCelsius()
```

| Weather | Offset | Felt temp | Player state |
|---|---|---|---|
| Clear | +2 | 22 °C | safe |
| Cloudy | 0 | 20 °C | safe |
| Rain | −4 | 16 °C | safe |
| HeavyRain | −7 | 13 °C | safe |
| Storm | −9 | 11 °C | safe |
| Fog | −3 | 17 °C | safe |
| Heat | +10 | 30 °C | safe |
| Cold | −12 | **8 °C** | ≤ 4 °C required for damage — safe at default base |

**Honest note:** with the fixed 20 °C base, no default weather state actually crosses the 4 °C / 36 °C
thresholds (Cold reaches 8 °C, Heat 30 °C). The damage thresholds are implemented and tested; the *cold
damage path is currently unreachable* with CODE_DEFAULT weather/base values. Biome-based base temperatures
or harsher profiles are the intended future tuning (PLANNED — see Assumptions doc).

---

## 3. Status Effects

`FAstrawildStatusEffect` instances: `StatusId` (FName), `RemainingSeconds`, `DamagePerSecond`,
`SpeedMultiplier` (0..∞, default 1.0).

- `AddStatusEffect` (server): refreshes an existing same-id effect or appends; broadcasts
  `OnStatusEffectApplied(StatusId)`.
- Tick: each effect counts down and applies `DamagePerSecond`; expired effects are removed.
- `HasStatusEffect(StatusId)` query; `FullRestore` clears all effects.
- Native tags `Status.Poisoned/Burning/Frozen/Wet/Soaked/Rested/Hungry/Thirsty/Cold/Overheated` exist for
  future identification.
- **Live since Batch 3:** `SpeedMultiplier` IS consumed — `GetStatusSpeedMultiplier()` multiplies the
  player's movement speed inside `RefreshMovementSpeed`; element statuses are applied by real combat
  hooks (player weapon hits via `EchoCharacter::ApplyElementalDamage`; creature attacks via
  `EchoAIController::TryAttackTarget`). Expiry broadcasts `OnStatusEffectRemoved` → speed refresh
  (incl. the rest/load/cheat restore paths — REVIEW-3 M-2 fix).
- **Honest note:** statuses are transient combat state and are NOT persisted to save data; a
  save/load or full restore clears them.

---

## 4. Death & Respawn Flow

```
Health ≤ 0 (any source)
  └─ SurvivalComponent::Die()            [server]
       ├─ Stats.bIsDead = true, Health = 0
       ├─ OnStatsChanged broadcast
       └─ OnDied broadcast
            └─ PlayerCharacter::OnPlayerDied
                 ├─ DisableInput(PlayerController)
                 └─ GameMode->RequestPlayerRespawn(Controller, 5.0)
                      └─ (timer, min 0.5 s) GameMode::RespawnPlayer
                           ├─ destroy old pawn
                           ├─ RestartPlayer(Controller)             (engine spawn at PlayerStart)
                           └─ Player->HandleRespawn(FTransform(0,0,150))
                                ├─ teleport to origin area
                                ├─ Survival->FullRestore()          (HP/stamina/hunger/thirst/effects)
                                └─ EnableInput
```

- `RespawnDelaySeconds = 5.0` (GameMode tunable).
- While dead, the survival tick early-outs (no further decay) and all combat/vital APIs reject input.
- Respawn position is currently the bootstrapper fallback area (origin + 150 cm Z), not the rest point —
  rest-point-as-respawn-anchor is PLANNED.

---

## 5. Food & Water Items (CODE_DEFAULT)

| Item | Id | Effect (`ApplyConsumption`) | Weight | Stack |
|---|---|---|---|---|
| Glimmer Berry | `Item_Berry` | Food +15, Water +5 (EchoFeedValue 6) | 0.2 | 50 |
| Raw Echo Meat | `Item_RawMeat` | Food +8 (EchoFeedValue 5) | 0.7 | 30 |
| Seared Meat | `Item_CookedMeat` | Food +30 | 0.6 | 30 |
| Dew Flask | `Item_WaterFlask` | Water +40 | 0.9 | 20 |
| Sunfiber Bandage | `Item_Bandage` | Heal +40 | 0.2 | 30 |

`ApplyConsumption(Food, Water, Heal)` (server): clamps Hunger/Thirst to 0–100 and Health to MaxHealth,
broadcasts `OnStatsChanged`. **Honest note:** a player-facing "consume item" keybind is **NOT IMPLEMENTED**
— the consumption API exists and works (cheats/rest interactions aside, eating requires future UI/inventory
screen wiring; the R-key feed path targets Echoes only).

---

## 6. Rest Point

`AAstrawildRestPoint` (interactable): E → `ActivateRestPoint()` (first activation broadcasts
`OnActivated`) + `Survival->FullRestore()` for the interacting player. Full restore = HP/stamina/hunger/
thirst to max, `bIsDead = false`, status effects cleared. Rest points serialize as
`FAstrawildRestPointSaveData` (v1 payload).

---

## 7. God Mode & Debug

- `SetGodMode(bool)` — cheat `AW.God` toggles; `ApplyDamage` returns 0 while enabled.
- `FullRestore()` — cheat `AW.HealAll`; also used by respawn/rest.
- Logging: `LogAstrawildCombat` carries damage/death lines.

---

## 8. Replication

| Property | Mechanism | Client hook |
|---|---|---|
| `Stats` (whole struct) | `ReplicatedUsing = OnRep_Stats` | re-broadcasts `OnStatsChanged` so HUD bars update |
| `StatusEffects` | plain Replicated | read by future UI |

HUD reads `GetHealthFraction()`, `GetStaminaFraction()`, `Stats.Hunger/100`, `Stats.Thirst/100` every
0.15 s.
