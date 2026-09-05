# ASTRAWILD — Multiplayer & Replication

**Status (LCP-1 re-audit, 2026): source re-audited at `00354da` for the LAN co-op scope —
the authoritative co-op spec, PART-3 audit and LCP work ledger now live in
`Docs/ASTRAWILD_LAN_COOP_SPEC.md` (this file is the replication inventory + rules reference).**
**Date: LCP-2 (2026) — re-audit wave 5 + client-world build landed (61 replicated props /
21 classes / 8 Server RPCs / 0 Client RPCs — the first Client RPCs arrive with LCP-3);
original inventory waves 1-4 dated 2026-08-30.**
**Primary sources:** every `GetLifetimeReplicatedProps` + `UFUNCTION(Server)` in `Source/AstrawildCore`
(grep `DOREPLIFETIME`, `UFUNCTION(Server)`), `AstrawildGameMode.cpp`, subsystem authority guards

---

## 0. LCP re-audit deltas (wave 5 — read this first)

The wave-1..4 tables below are corrected by this wave:

- Replication inventory grew to **61 properties across 21 classes** (wave-5 audit found
  43/14; LCP-2 then added): `AAstrawildGameState.bWorldSeedSynced` (client build gate),
  `AAstrawildResourceNode` (NodeDefinitionId/RemainingQuantity/bInfiniteResource + OnRep
  depleted visual mirror — the v1 "harvested locally" simplification is closed),
  `AAstrawildNPCCharacter` (NpcDefinitionId + registry-resolved appearance — NPCs now
  replicate at all), `AAstrawildVillageActor` (identity props; huts rebuild locally),
  `AAstrawildRestPoint` (WorldObjectId/bActive), `AAstrawildCraftingStationActor`
  (StationId), `AAstrawildPOIMarkerActor` (PoiId + OnRep beacon),
  `AAstrawildDungeonRoomActor` (Template + RoomIndex — client themed shells close the
  DP-9 "Template not replicated" gap), `AAstrawildDungeonPortalActor` (PortalId/PromptText).
  Earlier waves: `AAstrawildSkiffActor`, `UAstrawildMountComponent`,
  `UAstrawildCreatureSanityComponent`, `AAstrawildDungeonGateActor`,
  `AAstrawildResonancePillarActor` gained replication during SCP/GDP/DP batches.
- Server RPC inventory grew to **8** (was 5): `ServerRangedAttack` (final run) +
  `ServerRequestCraft`/`ServerRequestCancelCraft` (SCP-era — UMG screens now route craft
  requests from any net role).
- Client/NetMulticast RPCs: **still zero** — feedback flows via replicated properties.
  The LCP-3 batch adds the first Client RPCs (shop/dialogue/notify).
- Co-op architecture status: **replaced by LAN_COOP_SPEC §2** (LCP-2 CLOSED the
  client-visible-world gap: every client now builds the deterministic cosmetic world
  (lighting/terrain/sea/landmarks/dressing) from the replicated seed and receives the
  gameplay actors through replication; co-op save / client state sync / session flow
  remain LCP-4..LCP-6).
- The open work list (§5 below) is superseded by the LCP ledger in LAN_COOP_SPEC §8.

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

## 4. Co-op Architecture Status (target: 4 players LAN — see LAN_COOP_SPEC §2 for the live verdicts)

| Aspect | Status (LCP-1) |
|---|---|
| Target | **4 players LAN, host/listen server authoritative** (user product decision; single player unchanged) |
| Session classes | ✅ GameMode/GameState/PlayerController authority guards in place; LAN session flow = LCP-6 (MISSING at audit) |
| World state sharing | ✅ GameState replication (time/weather/seed/ending) |
| Client-visible world | ✅ LCP-2 CLOSED — deterministic client cosmetic build (seed-gated) + replicated gameplay actors (nodes/NPCs/villages/stations/rest points/POI markers/dungeon rooms+portals); ENGINE-UNVERIFIED |
| Research pool | ⚠️ host pool correct; client visibility/notifications NOT IMPLEMENTED → LCP-5 |
| Echo roster | ⚠️ host-side pool; no per-player partition, no client visibility → LCP-4/LCP-5 |
| Player-specific save | ❌ first-player-only → per-player blocks = LCP-4 |
| Quests per player | ⚠️ host-side per-PC correct; client HUD replication MISSING → LCP-5 |
| Dedicated server | ❌ out of scope by design (MASTER_CONTROL §1b) |
| Cheat manager | ⚠️ host-gate MISSING (dev builds) → LCP-3 |
| Netcode playtest | ❌ NOT RUN — engine-verification class (Antigravity §22) |

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
