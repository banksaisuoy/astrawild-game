# ASTRAWILD — Multiplayer & Replication

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine) — server-authoritative rules
and replication are in code; **multiplayer has NOT been play-tested** (no second client / no netcode test
run in this round). Single player & listen-server host paths are the validated-by-design targets.**
**Date: 2026-08-30** (wave 4 sync — `AAstrawildBuildingActor::bIsPowered` replication; +1 prop / 9 classes)
**Primary sources:** every `GetLifetimeReplicatedProps` + `UFUNCTION(Server)` in `Source/AstrawildCore`
(grep `DOREPLIFETIME`, `UFUNCTION(Server)`), `AstrawildGameMode.cpp`, subsystem authority guards

---

## 1. Server-Authoritative Rules

**The golden rule (directive §28): gameplay state is computed on the server. Clients send intent; servers
validate, mutate, and replicate results.**

### What a client can NEVER decide

| Domain | Client-side action | Server-side authority |
|---|---|---|
| **Damage** | `Request*Attack/Dodge/Block` (intent only) | `ServerLightAttack/ServerHeavyAttack/ServerDodge/ServerSetBlocking` re-check cooldown/stamina/alive, run the sweep, apply mitigation & damage; `SurvivalComponent::ApplyDamage` rejects non-authority |
| **Inventory** | none (no add/remove RPC exists) | `AddItem/RemoveItem/ConsumeItems` gate on weight server-side; `Items` replicates down |
| **Capture** | interact triggers the server path | `TryCapture` requires `ROLE_Authority`: Resonator consumption, chance roll, `Echo->Capture` (authority-checked), roster add |
| **Building** | ghost preview, snap/rotate, confirm | `ServerPlaceBuilding(DefId, Location, Yaw)`: definition lookup, **overlap re-validation**, refund on failure, spawn, event |
| **Quests** | none | `QuestComponent` only progresses on `GetOwnerRole() == Authority` events; rewards granted server-side |
| **Time / weather / world** | none | Subsystems tick only on non-client netmode; GameState setters reject non-authority (`LogAstrawildNetwork` warning) |
| **Echo simulation** | none | needs decay, capture, growth, commands, AI — all authority-guarded on the Echo/controller |

Client-side conveniences (prediction-lite, no authority): local cooldown checks before *sending* attack
requests; ghost placement math; HUD reads of replicated state; dodge direction normalization.

---

## 2. Replication Inventory (every replicated property, per class)

Grep-verified against `DOREPLIFETIME(...)` calls — **26 replicated properties across 9 classes**.

> **Count reconciliation (2026-08-30):** this table previously said "20 properties / 7 classes". The
> dungeon/boss round added 4 properties (`AAstrawildEchoBossCharacter` ×3,
> `AAstrawildDungeonRoomActor` ×1) that were never synced into this doc, and wave 3 adds
> `EquippedShieldItemId`. Wave 4 adds `AAstrawildBuildingActor::bIsPowered` (Item C — `d5d23c2`).
> All four gaps are corrected here — 20 + 4 + 1 + 1 = 26 across 9 classes.

### `AAstrawildGameState` (GameStateBase — replicated by default; `bReplicates` engine-managed)
| Property | Mode |
|---|---|
| `TimeOfDayMinutes` | `ReplicatedUsing=OnRep_TimeOfDayMinutes` |
| `DayNumber` | Replicated |
| `WeatherState` | `ReplicatedUsing=OnRep_WeatherState` |
| `WorldSeed` | Replicated |

### `AAstrawildEchoCharacter` (`bReplicates = true`, replicating movement)
| Property | Mode |
|---|---|
| `Personality` | Replicated |
| `Needs` (struct) | Replicated |
| `Experience` | Replicated |
| `CurrentAIState` | Replicated |
| `ActiveCommand` | Replicated |
| `OwnerPlayerId` | Replicated |

### `AAstrawildBuildingActor` (`bReplicates = true`)
| Property | Mode |
|---|---|
| `bIsSwitchedOn` | Replicated |
| `CurrentHealth` | Replicated |
| `StoredCharge` | Replicated |
| `bIsPowered` | Replicated (wave 4 — Item C / `d5d23c2`; written by `UAstrawildPowerSubsystem::ResolveGrid` every 2 s tick; UE net driver short-circuits unchanged values — no extra bandwidth unless a consumer's power state actually changes) |

### `AAstrawildWorkSiteActor` (`bReplicates = true`)
| Property | Mode |
|---|---|
| `StoredOutput` | Replicated |

### `UAstrawildSurvivalComponent` (`SetIsReplicatedByDefault(true)`)
| Property | Mode |
|---|---|
| `Stats` (whole vitals struct) | `ReplicatedUsing=OnRep_Stats` (re-broadcasts `OnStatsChanged`) |
| `StatusEffects` (array) | Replicated |

### `UAstrawildCombatComponent` (`SetIsReplicatedByDefault(true)`)
| Property | Mode |
|---|---|
| `bIsBlocking` | Replicated |
| `bReplicatedDodgeTimer` (float) | Replicated |

### `UAstrawildInventoryComponent` (`SetIsReplicatedByDefault(true)`)
| Property | Mode |
|---|---|
| `Items` (TMap<FName,int32>) | Replicated |
| `EquippedItemId` | Replicated (weapon slot) |
| `EquippedShieldItemId` | Replicated (shield slot — wave 3; feeds block mitigation + HUD) |

### `AAstrawildPlayerCharacter` (`bReplicates = true`, replicating movement)
No custom properties — movement replication + component replication above.

### `AAstrawildEchoBossCharacter` (`bReplicates = true` — added with the dungeon/boss round)
| Property | Mode |
|---|---|
| `CurrentHealth` | Replicated |
| `CurrentPhase` | Replicated |
| `bEnraged` | Replicated |

### `AAstrawildDungeonRoomActor` (`bReplicates = true` — added with the dungeon/boss round)
| Property | Mode |
|---|---|
| `bCleared` | Replicated |

### Not replicated (deliberate or gap)
`AAstrawildResourceNode` (`SetReplicates(false)` — harvested locally is a known v1 simplification),
rest points, damage targets, crafting stations (server-only interact via the host's client in listen
server; **dedicated-server interact routing NOT IMPLEMENTED**), QuestComponent (host-only state), the
placement preview ghost (owner-only visual), research points/tech unlocks (GameInstance, host process
only — no client notification RPC).

---

## 3. RPC Inventory (all `UFUNCTION(Server)` in the module)

| Class | RPC | Spec | Payload | Validation server-side |
|---|---|---|---|---|
| `UAstrawildCombatComponent` | `ServerLightAttack` | Server, Reliable | — | alive, cooldown, sweep |
| | `ServerHeavyAttack` | Server, Reliable | — | alive, cooldown, stamina 25, sweep |
| | `ServerDodge` | Server, Reliable | `FVector_NetQuantizeNormal` | alive, cooldown, stamina 22, impulse |
| | `ServerSetBlocking` | Server, Reliable | `bool` | alive check |
| `UAstrawildBuildingComponent` | `ServerPlaceBuilding` | Server, Reliable | `FName DefinitionId, FVector_NetQuantize Location, float Yaw` | definition exists, overlap re-validation, refund, spawn |

No Client/NetMulticast RPCs exist yet (client feedback flows through replicated properties + delegates).

---

## 4. Co-op Architecture Status (target 1–4 players)

| Aspect | Status |
|---|---|
| Target | Co-op 1–4 players, Host/Listen Server first (master plan §1) |
| Session classes | ✅ `AAstrawildGameMode` sets PlayerCharacter/GameState/PlayerController/CheatManager; all authority guards in place |
| World state sharing | ✅ GameState replication (time/weather/seed) |
| **Research pool** | ✅ **Shared by design** — `UAstrawildResearchSubsystem` is a GameInstance subsystem; all players in a session draw from/write to one pool. Documented decision (Assumptions doc #4). Unlock notifications to non-host clients are NOT IMPLEMENTED. |
| Echo roster | ⚠️ GameInstance-scoped like research → shared roster; per-player rosters (`OwnerPlayerId` exists on Echoes) are data-ready but the roster subsystem does not partition by player |
| Player-specific save | ❌ `SaveWorld` serializes the **first player controller** only — co-op saves are NOT IMPLEMENTED |
| Quests per player | ❌ QuestComponent runs on each PlayerController but is not replicated; client players' quest state is host-side only; progress events are not routed per-player |
| Dedicated server | ❌ Not tested; crafting-station interaction executes on the interacting actor's authority path — needs a Server RPC wrapper for dedicated servers |
| Cheat manager | Engine `CheatManager` — non-shipping builds only |
| Netcode playtest | ❌ **NOT RUN** — zero multiplayer sessions have been executed (no toolchain in sandbox) |

---

## 5. Open Multiplayer Work List

Ordered by priority for the co-op milestone:

1. **Compile + PIE single-player validation first** (blocks everything — target machine).
2. Listen-server 2-player smoke test: movement/interaction/capture/quests on host vs client.
3. Route crafting-station & resource-node interaction through Server RPCs (currently authority-local).
4. Replicate quest state per player (PlayerState or replicated QuestComponent) + client HUD feed.
5. Client-facing unlock notifications (Client RPC for `Event.TechUnlocked`, `Event.QuestObjectiveCompleted`).
6. Co-op save schema: per-player blocks in `UAstrawildSaveGame` (v3) + migration.
7. Per-player Echo roster partition by `OwnerPlayerId`.
8. Cheat hardening: disable `AW.*` for non-host (currently engine-stripped in Shipping only).
9. Bandwidth pass: relevancy (`bOnlyRelevantToOwner` where applicable), NetUpdateFrequency tuning for Echoes.
10. 4-player performance/net profile with Unreal Insights network stats.

---

## 6. Honest Status Summary

- **Implemented in code:** authority guards everywhere, 26 replicated properties across 9 classes, 5 server
  RPCs, shared research pool, replicated world state.
- **Not implemented:** client-side prediction of results (only request-gating), per-player quest/save/roster,
  dedicated-server routing, any actual multiplayer test session.
- Single player and listen-server-host flows are the current first-class citizens.

> **Wave 4 note (Item B — `DismantleBuilding`):** single-player only at present. The dismantle path uses
> direct method calls gated on `GetLocalRole() == ROLE_Authority` (mirrors the existing placement path's
> authority rule). For a remote client to dismantle, the player intent must be carried to the server via
> a Server RPC (e.g. `ServerDismantleBuilding(AActor*)` with overlap re-validation); the multiplayer
> client→server RPC layer (H-12) remains pending in Batch 3 / MP batch.
>
> **Wave 4 note (Item A — `HostileSpawnerSubsystem`):** server-only tick (`World->GetNetMode() == NM_Client`
> early-return at `.cpp:52`). Clients never run the spawn sweep — they see populated hostiles only via
> the existing `AAstrawildEchoCharacter` replication (movement + 6 props), which is already correct for
> listen-server co-op. A dedicated-server build (no `.Target.cs` yet — L-3) is the prerequisite for true
> dedicated-host testing.
>
> **Wave 4 note (Item C — power persistence):** `ResolveGridNow()` is server-only. Clients receive correct
> state via the new `bIsPowered` replicated UPROPERTY — `ResolveGrid`'s server-only early return
> (`World->GetNetMode() == NM_Client`) at `PowerSubsystem.cpp:55` guards the path; the property-write happens
> only on the server, the net driver replicates the value to clients.
