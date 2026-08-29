# ASTRAWILD — Survival System

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-29**
**Primary sources:** `AstrawildSurvivalComponent.h/.cpp`, `AstrawildWeatherSubsystem.cpp` (temperature
profiles), `AstrawildGameMode.cpp` (respawn), `AstrawildContentLibrary.cpp` (food/water items),
`AstrawildRestPoint.cpp`

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
| Thirst decay | **0.14 /s** (≈8.4/min → 100→0 in ~11.9 min) | `ThirstDecayPerSecond` |
| Stamina regen | **+14 /s** passive (always, when alive) | `StaminaRegenPerSecond` |
| Starvation damage | **1.5 /s per depleted vital** (both empty = 3.0 /s) | `StarvationHealthDamagePerSecond` |
| Cold exposure damage | 1.0 /s while `Temperature ≤ 4 °C` | `ExposureHealthDamagePerSecond` |
| Heat exposure damage | 1.0 /s while `Temperature ≥ 36 °C` | `ExposureHealthDamagePerSecond` |

Stamina drains (see Combat doc): heavy attack 25, dodge 22, sprint gate (sprint disabled below 5 %
stamina — stamina is not continuously drained by sprinting in v2; only gated).

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
- **Honest note:** `SpeedMultiplier` is stored in the struct but is **not yet applied to movement** — no
  runtime consumer (movement-integration PLANNED). Nothing in CODE_DEFAULT content applies status effects
  yet; the system is a working data + tick pipeline awaiting content.

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
