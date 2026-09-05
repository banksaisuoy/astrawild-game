# ASTRAWILD — LAN CO-OP SPEC (4-PLAYER PERSONAL BUILD)

**Status**: SOURCE WORK IN PROGRESS (LCP-1..LCP-8 batches; engine-verification pending)
**Authoritative for**: every multiplayer/replication decision on `final-completion`
**Companion to**: `Docs/ASTRAWILD_MASTER_CONTROL.md` (v6.0), `Docs/ASTRAWILD_MULTIPLAYER.md`
**Product decision date**: user directive — PERSONAL LAN CO-OP + FREE ASSET PRODUCTION MODE

---

## 1. Product decision (binding — user-issued, cannot be reverted)

ASTRAWILD is a **PRIVATE PERSONAL GAME for the user and 3 friends**.

| Item | Decision |
| :--- | :--- |
| Players | **4 total** (1 listen-server host + 3 LAN clients) |
| Network | **Local Area Network only** |
| Dedicated server | **NOT required** (deferred/future scope) |
| Public matchmaking / accounts / cloud | **NOT required** (deferred/future scope) |
| Commercial release | **NOT a target** |
| Multiplayer model | UE server-authoritative replication on a **listen server** |
| Editor Multi-User | **NEVER used** (that is editor collaboration, not gameplay networking) |

Supported multiplayer targets (also mirrored in MASTER_CONTROL §1b):

```
SINGLE PLAYER          — standalone (the original first-class path, unchanged)
LAN LISTEN SERVER      — host plays + serves; up to 3 remote clients join
                       — 4 players total, one shared authoritative world
```

NOT required for this release (deferred only, do not build): dedicated server,
public matchmaking, online accounts, cloud backend, MMO-scale networking.

**Scope guard**: multiplayer expansion must NOT explode the project. No new
networking architecture, no second replication stack, no MMO guild/party services.
All work extends the existing server-authoritative foundation.

---

## 2. Multiplayer source audit (PART 3 directive — executed at commit `00354da`)

Method: full grep of `DOREPLIFETIME`, `UFUNCTION(Server)`, `UFUNCTION(Client)`,
`UFUNCTION(NetMulticast)`, `GetNetMode`, `GetLocalRole`, `HasAuthority`,
`GetFirstPlayerController` across `Source/AstrawildCore` + line-read of every
interaction entry point. Raw numbers at the audited tip:

- **43 replicated properties across 14 classes** (BuildingActor 5, EchoCharacter 9,
  GameState 6, InventoryComponent 6, EchoBossCharacter 4, SurvivalComponent 2,
  CombatComponent 2, WorkSiteActor 2, CreatureSanityComponent 2, DungeonRoom/Gate,
  ResonancePillar, Skiff, MountComponent 1 each).
- **8 Server RPCs** (BuildingComponent::ServerPlaceBuilding; CombatComponent::
  ServerLightAttack/ServerRangedAttack/ServerHeavyAttack/ServerDodge/ServerSetBlocking;
  CraftingComponent::ServerRequestCraft/ServerRequestCancelCraft).
- **0 Client RPCs, 0 NetMulticast RPCs** (feedback flows via replicated props only).
- **Non-replicated world layer** (server-only actors): terrain tiles, water planes,
  lighting rig, biome dressing, resource nodes, POI markers, villages + NPCs,
  rest points, crafting stations.

### Classification (per directive: WORKS / PARTIAL / BROKEN / MISSING / UNVERIFIED)

| System | Verdict | Evidence / Gap |
| :--- | :--- | :--- |
| **PLAYER movement/camera** | WORKS | PlayerCharacter `bReplicates`, CharacterMovement replication; input local |
| **PLAYER combat** | WORKS | 5 validated Server RPCs; damage server-side (SurvivalComponent authority guard) |
| **PLAYER respawn** | WORKS | GameMode::RequestPlayerRespawn (server) |
| **INVENTORY** | WORKS | No client mutation path; `Items`/equips replicate; weight server-side |
| **CRAFTING** | WORKS | ServerRequestCraft/Cancel (server refunds) |
| **BUILDING placement** | WORKS | ServerPlaceBuilding + overlap revalidation + refund |
| **POWER** | WORKS | server-only ResolveGrid + `bIsPowered` replicated |
| **AUTOMATION (work sites/robots/drone)** | WORKS | replicated actors, server ticks |
| **QUEST progression (host)** | WORKS | per-PC QuestComponent, event-bus driven, server authority |
| **RESEARCH (host)** | WORKS | shared pool in host GameInstance, server-validated purchases |
| **WORLD EVENTS** | WORKS | `UTickableWorldSubsystem` + `NM_Client` early return — single authoritative roll, no per-client generation |
| **TIME / WEATHER / SEED / ENDING** | WORKS | GameState replicated props + OnReps, server-only setters |
| **DUNGEONS** | WORKS | generator/rooms/gates/portals replicated; server generation |
| **BOSSES** | WORKS | EchoBossCharacter replicated (health/phase/enrage) + specials server-side |
| **ECHO AI / wildlife** | WORKS | server AI, EchoCharacter replication |
| **CAPTURE (server path)** | WORKS | TryCapture `ROLE_Authority` guard, Resonator cost server-side |
| **CAPTURE (client trigger)** | PARTIAL | interact is executed locally on the client → remote clients can never *start* a capture; owner id = pawn `GetFName()` (not a stable per-player key) → **LCP-3/LCP-4** |
| **ECHO roster** | PARTIAL | host GameInstance pool only; no `OwnerPlayerId` on roster entries → no per-player partition, no client visibility → **LCP-4/LCP-5** |
| **QUEST (client HUD)** | PARTIAL | QuestComponent state not replicated → owning client sees no objectives → **LCP-5** |
| **RESEARCH (client UI)** | PARTIAL | client's local ResearchSubsystem is a separate empty GameInstance instance (GameInstance subsystems do NOT replicate) → client screens blank → **LCP-5** |
| **NPC / shop / dialogue (clients)** | PARTIAL | NPCs not replicated; interact local; TryBuy/TrySell called locally from client widget → clients cannot see, talk or trade → **LCP-3** |
| **INTERACT routing (all interactables)** | PARTIAL | local trace + direct interface call; correct on host/standalone, dead on remote clients (nodes/stations/rest points/POI/skiff) → **LCP-3** |
| **CHEAT hardening** | PARTIAL | Shipping strips CheatManager, but in dev builds clients can exec `AW.*` locally (local-only desync, no server grant) → host-gate required → **LCP-3** |
| **SAVE / LOAD (co-op)** | **MISSING** | `SaveWorld` serializes `GetFirstPlayerController()` only — player 2..4 state is lost → per-player blocks required → **LCP-4** |
| **CLIENT-VISIBLE WORLD** | **BROKEN** (for clients) | terrain/lighting/dressing/water/villages/NPCs/nodes rest-points/stations are server-only non-replicated actors → remote clients load a near-empty world → **LCP-2** |
| **LAN SESSION FLOW** | **MISSING** | no Host/Find/Join/Direct-IP path, no listen-server travel, no mode indication → **LCP-6** |
| **CLIENT notifications** | **MISSING** | zero Client RPCs; research unlocks / quest events / capture toasts never reach remote clients → **LCP-3/LCP-5** |
| **RECONNECT** | **MISSING** | no late-join player restore path → **LCP-4** |
| **Everything above at runtime** | **UNVERIFIED** | engine-verification class per MASTER_CONTROL §0 — Antigravity owns the verdict; no netcode session has ever been run |

(“WORKS” = source-level server-authoritative correctness, statically verified.
No engine claim is made anywhere in this table.)

---

## 3. Party & authority rules (PART 5 — HOST IS AUTHORITATIVE)

| Domain | Rule |
| :--- | :--- |
| World progression (zones, POIs, events, story state) | **Host-authoritative** (single world truth) |
| Quest progression | **Shared world quest chain, host-driven**; objectives respond to party activity where the event bus publishes it (any party member's kill/discovery/craft counts) |
| Boss progression | **Shared** (boss defeat is world state; one-shot flags replicate) |
| Dungeon progression | **Shared** (room clears / completion are world state) |
| Research | **Host-authoritative pool, shared visibility** (all players see unlocks; purchases validated on host) |
| Base / building ownership | **Party-shared** (any member may build; host validates placement) |
| Inventory / equipment | **Per-player** |
| Echo ownership | **Per-player** (roster entries carry `OwnerPlayerId`) |
| Player death / respawn / stats / XP | **Per-player individual** |
| World save file | **Host-owned** (clients never write world state) |

Documented exceptions (v1, deliberate):
- **NPC affinity is party-shared** (one relationship value per NPC across all
  players — the GDP-4 per-NPC-id design kept world-side). Shared village
  relationships fit the 4-friends fantasy; per-player affinity is a future
  option, not a v1 need.
- **Perishable freshness is world-shared** (the spoilage subsystem ages item
  ids globally). Cosmetic-level divergence in co-op; not progression-critical.
- **Roster ownership is a stable key** (player name / slot), while the LIVE
  actor's `OwnerPlayerId` stays the pawn-name convention the final-audit H-1
  consumers (party passives/commands/work assignment/combat exclusion) rely on.

No MMO guild systems. No party manager UI beyond the existing party mechanics.

---

## 4. Session flow (PART 6)

```
HOST  → Pause menu → "HOST LAN GAME"
        → (world saves first if a session is live)
        → ServerTravel("<CurrentMap>?listen?GameMode=...&autoload=1")
        → host re-enters as listen server + latest save auto-loads (H-3 machinery)
        → LAN beacon starts broadcasting on UDP port 45861

CLIENT → Pause menu → "FIND LAN GAME"
        → beacon listener collects broadcast packets (1 s cadence, 3 s expiry)
        → session list shows: game name + host address + open slots
        → "JOIN" → ClientTravel("IP:7777")

CLIENT → Pause menu → "DIRECT CONNECT"
        → typed address (default "192.168.0.x:7777") → ClientTravel

Mode indication (mandatory, directive PART 6):
        HUD status line + log: "SINGLE PLAYER" / "LAN HOST — you are the
        authoritative world" / "LAN CLIENT — connected to host's world".
```

Beacon design (deliberately minimal — discovery only, NOT gameplay networking):
one-way UDP broadcast from host; payload is a compact validated packet
(magic `AWLAN1`, protocol version, game name, host listen port, player count,
open slots). Gameplay networking stays 100% UE IpConnection replication.
No accounts, no internet matchmaking, no external session services.

---

## 5. Save / load in LAN (PART 7)

- **The HOST owns the authoritative world save** (schema V5 world block — unchanged).
- New additive save payload: **per-player blocks** (schema V6, additive-only):
  each block keyed by a stable `PlayerKey` and holding that player's inventory,
  equipment, survival stats, transform, attributes/skills/loadout (GDP-3),
  Echo ownership (roster entries + party state), quest states, affinity, defeats.
- Legacy single-player fields remain the host block (v5 saves migrate in place).
- Clients never write authoritative world state; client-side saves are not created.
- **Late join / reconnect**: on host `PostLogin`, if a save block exists for the
  joining `PlayerKey`, that player's state restores (inventory, roster, quest,
  position). Reconnect within a live session restores from the in-memory
  per-player state, not a new save file.
- Autosave (host) persists all present players. On load, the host restores the
  world + every saved player that is connected.

---

## 6. Network failure safety (PART 8)

| Failure | Behavior (server-validated everywhere) |
| :--- | :--- |
| Client disconnect | host keeps world; per-player state retained in memory (restored on reconnect within session) |
| Client reconnect | per-player block restore path (above) |
| Client death / respawn | individual, server respawn timer (existing GameMode path) |
| Destroyed Echo | server-only defeat; roster sanitized (existing) |
| Invalid replicated actor in RPC | every server RPC null/validity checks its arguments (fail-closed) |
| Player leaving dungeon | dungeon state is shared world state — unaffected |
| Player joining after progression | world state already replicated; quest chain state syncs via quest replication |
| Duplicated reward attempts | one-shot completion guards already in QuestComponent (server-side) |
| Duplicate capture | `bCaptured` authority guard (server) |
| Duplicate inventory transaction | server-only inventory mutation (no client RPC exists) |
| RPC spam | server RPCs re-validate cooldown/stamina/cost every call |

Clients can NEVER directly grant themselves: items, XP, research, quest
completion, boss rewards, Echo ownership, building ownership — every state
mutation path is server-authoritative (audit §2), and cheat exec is host-gated
(LCP-3).

---

## 7. Performance policy (PART 9)

Target: **4 players maximum** — reliability over scale.

- No replication work for >4 players; no bandwidth budget for 32+.
- Avoid: high-frequency RPC spam, purely-cosmetic replicated state, distant-AI
  full replication, multicast spam. (Current design already complies: VFX/ghosts
  local; feedback via property replication.)
- LAN discovery beacon: 1 Hz, one small UDP datagram.
- Client world build is deterministic-local (seeded) — zero replication cost for
  the static cosmetic layer.
- Gameplay-relevant actors replicate (nodes/NPCs/stations/villages) at default
  update frequency with `NetUpdateFrequency` set low where state is rare
  (resource nodes: 0.5 Hz — state changes only on harvest/respawn).

---

## 8. Implementation batches (LCP ledger)

| Batch | Scope | Tests |
| :--- | :--- | :--- |
| LCP-1 | THIS document + MASTER_CONTROL v6.0 + MULTIPLAYER audit refresh + registry reopen (docs-only) | — |
| LCP-2 | Client world build: bootstrapper client path (seeded deterministic cosmetic layer), gameplay actors replicate (nodes/NPCs/stations/rest points/POI markers/villages) | **DONE** (+2: LCP2.ClientWorldPolicy / LCP2.DressingGate) |
| LCP-3 | Interaction & trade routing: `ServerInteract`, `ClientOpenVendorShop`/`ClientOpenVendorDialogue`/`ClientNotify`, shop trade RPC, capture intent, cheat host-gate | **DONE** (+2: LCP3.ServerRoutingSurface / LCP3.DialogueValidation) |
| LCP-4 | Per-player persistence: additive per-player save blocks + roster owner partition + stable PlayerKey + late-join/reconnect restore | **DONE** (+2: LCP4.RosterPartition / LCP4.CoopSaveBlock) |
| LCP-5 | Client state sync: QuestComponent replication, research + roster mirrors on GameState, client notifications | **DONE** (+2: LCP5.ClientStateSyncSurface / LCP5.ResearchMirrorRoundTrip; roster mirror deferred — no client roster UI exists, party echoes replicate as actors) |
| LCP-6 | LAN session flow: `UAstrawildLANSessionSubsystem` (host/find/join/direct IP) + pause menu LAN panel + HUD mode line | **DONE** (+2: LCP6.BeaconProtocol / LCP6.AddressParsing) |
| LCP-7 | Free-asset ledger doc + approved Quaternius acquisition (separate concern, same session) | — |
| LCP-8 | Final gate: validators, HANDOFF §22 LAN acceptance, READINESS re-affirm, registry close, worklog | — |

All code follows the standing rules: server-authoritative mutation, additive-only
save schema, appended-only enums, world-free automation contracts per fix,
validators green ×2 pre-commit, push per batch, no force-push.

---

## 9. LAN acceptance test (PART 22 — engine-side, Antigravity-owned)

The final build is NOT LAN-ready until: host creates a LAN game; clients 2/3/4
join; and all four can **move, see each other, fight, gather, craft, build, use
Echo, capture Echo, use Echo abilities, progress quests, enter dungeons, fight
bosses, receive rewards, save, reconnect, continue progression** — with the host
authoritative throughout. Full checklist: `ASTRAWILD_FINAL_BUILD_HANDOFF.md` §22.
