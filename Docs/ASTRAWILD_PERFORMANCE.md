# ASTRAWILD — Performance Design & Plan

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine) — design is in code; **no
profiling has been run** (no UE toolchain in the sandbox). All numbers below are code values, not
measurements.**
**Date: 2026-08-29**
**Primary sources:** grep of `PrimaryActorTick` / `TickInterval` / tickable subsystems across
`Source/AstrawildCore`; `AstrawildEcosystemSubsystem.cpp`; `AstrawildPowerSubsystem.cpp`;
`AstrawildHudWidget.cpp`
**Target:** 60 FPS on a mid-range PC (master plan §11: Game Thread ≤ 6.0 ms, Render Thread ≤ 3.5 ms,
GPU ≤ 12.0 ms, ≥15 % memory headroom, no severe streaming hitches).

---

## 1. Tick Budgets — Who Ticks, How Often

### 1.1 Actors with ticking enabled

| Class | Tick config | Actual work per tick |
|---|---|---|
| `AAstrawildEchoCharacter` | every frame | server: needs decay (throttled by LOD accumulator — Tier0 per-frame, Tier1 every 0.25 s, Tier2 1 s, Tier3 5 s) |
| `AAstrawildWorkSiteActor` | every frame (server guard) | worker loop: affinity × personality × mood × energy × power accumulation; early-outs when no workers / unpowered |
| `AAstrawildWorldBootstrapper` | **`TickInterval = 0.25 s`** | sun rotation + intensity lerp only (4 Hz, cheap) |

### 1.2 Components with ticking enabled

| Component | Work per tick | Notes |
|---|---|---|
| `UAstrawildSurvivalComponent` | vitals decay math (server only, ~10 float ops) | every frame by design (needs smooth starvation/temperature curves) |
| `UAstrawildCombatComponent` | dodge i-frame countdown | every frame; near-zero cost when idle |
| `UAstrawildCraftingComponent` | active-craft countdown | every frame; early-out when not crafting |
| `UAstrawildBuildingComponent` | preview update **only while placing** | every frame; early-out when not in placement mode |

### 1.3 Tickable world subsystems (UTickableWorldSubsystem)

| Subsystem | Per-frame work | Internal throttle |
|---|---|---|
| `UAstrawildTimeSubsystem` | `DeltaTime × 1.0` accumulate + whole-minute apply | trivial math per frame; hour/day broadcasts only on boundaries |
| `UAstrawildWeatherSubsystem` | absolute-minute compare | transition roll only every **90 in-world minutes** |
| `UAstrawildEcosystemSubsystem` | accumulator | **tier sweep every 1.0 s** (distance loop over registered Echoes) |
| `UAstrawildPowerSubsystem` | battery flow integration | **grid re-solve every 2.0 s** (`ResolveIntervalSeconds`) |
| `UAstrawildJournalSubsystem` | per-player observation cone pass | iterates all Echoes **every frame** — see §4 known hotspots |

### 1.4 Explicitly non-ticking (constructor `bCanEverTick = false`)

`AAstrawildPlayerCharacter`, `AAstrawildPlayerController`, `AAstrawildGameMode`,
`AAstrawildEchoAIController` (timer-driven instead), `AAstrawildNPCCharacter`, `AAstrawildBuildingActor`,
`AAstrawildCraftingStationActor`, `AAstrawildResourceNode`, `AAstrawildRestPoint`,
`AAstrawildDamageTarget`, `UAstrawildInventoryComponent`, `UAstrawildCaptureComponent`,
`UAstrawildQuestComponent` (event-driven).

### 1.5 UI

`UAstrawildHudWidget::NativeTick` runs per frame but refreshes bound state only every **0.15 s**
(≈6.7 Hz); includes one interaction line trace for the prompt. The HUD log line claims "10 Hz" — the
actual constant is 0.15 s (cosmetic comment drift).

---

## 2. Ecosystem Simulation LOD

Echoes register with the ecosystem; a **1 s sweep** assigns tiers by nearest-player distance:

| Tier | Distance | AI think | Needs decay | Movement |
|---|---|---|---|---|
| Tier 0 — Full | ≤ 3000 cm | 0.25 s | per frame | full |
| Tier 1 — Reduced | ≤ 8000 cm | 0.25 s | every 0.25 s | full |
| Tier 2 — Statistical | ≤ 20000 cm | 1.0 s (movement disabled per tier contract) | every 1.0 s | none |
| Tier 3 — World | > 20000 cm | 5.0 s | every 5.0 s | none |

Population bookkeeping (`WildCount/CapturedCount/DefeatedCount`) is O(1) map updates on capture/defeat.
Dawn Fields population (11 Echoes) is far below any budget concern; the tiering is architecture for the
content-alpha world.

---

## 3. Power Grid

- Energy integration per frame (one float), **topology + brownout re-solve every 2.0 s** — sorts consumers
  by priority class, aggregates generation/draw/capacity. O(n log n) in building count, n ≈ tens.
- `IsLocationPowered` is a proximity scan over generators — called by work sites per tick; acceptable at
  current scale, flagged for an accelerated structure (grid id per location) when bases grow.

---

## 4. No-Per-Frame-Allocation Policy & Known Exceptions

Policy: steady-state ticks must not allocate. **Honest exceptions in current code** (fix list for the
target-machine iteration):

| Hotspot | File | Issue |
|---|---|---|
| `UAstrawildJournalSubsystem::Tick` | JournalSubsystem.cpp | builds a `TArray<AActor*>` (Reserve 16) **every frame** and iterates all Echoes per player — the heaviest known per-frame loop |
| `AAstrawildEchoAIController::ExecuteProtect` | EchoAIController.cpp | `GetAllActorsOfClass` allocation per think while in Protect state |
| `AAstrawildPlayerCharacter::CyclePartyCommand` | PlayerCharacter.cpp | `GetAllActorsOfClass` per keypress (input-rate, not per-frame — acceptable) |
| `UAstrawildEcosystemSubsystem::RunTierSweep` | EcosystemSubsystem.cpp | `RemoveAll` lambda sweep per second — small, bounded |
| AI think reschedule | EchoAIController.cpp `Think()` | computed LOD interval is not applied — `SetTimerForNextTick` re-arms **every frame** (dead `Interval` variable). Fix: `SetTimer(Interval, …)` — changes think cost from per-frame to 4 Hz/1 Hz/0.2 Hz by tier |

All five are on the first profiling-driven fix pass (§6) — none are silent; each is listed here precisely
because the policy is "no per-frame allocations, no hidden per-frame loops".

---

## 5. Replication Bandwidth (design)

- 20 replicated properties across 7 classes (see Multiplayer doc §2); the largest chatty items are
  `Survival::Stats` (struct, changes ~every frame while vitals decay) and `Echo::Needs`.
- Mitigations available when profiling demands: property condition flags (`COND_SkipOwner`), push-model
  replication, coarser needs quantization. NOT APPLIED yet — measure first.

---

## 6. Profiling Plan (target machine, after first compile)

**Baseline capture procedure** (repeat after every milestone):

1. `Unreal Insights` trace session — 60 s in Dawn Fields: spawn (`AW.SpawnEcho Echo_Lumewisp` ×10 to stress
   the Echo path), fight, capture, build ×5, wait for one autosave.
2. `stat unit` / `stat unitgraph` — frame split (Game/Render/GPU/RHIT) vs the 6.0/3.5/12.0 ms budgets.
3. `stat game` / `stat ticks` — tick cost ranking: expect Survival/Echo ticks + Journal loop on top.
4. `stat gpu` + `stat foliage/foliagerendering` equivalents for the placeholder ground plane (trivial).
5. `MemReport -full` after 10 minutes of play — memory headroom check.
6. Networking (when co-op testing starts): `stat net`, packet size/rate with 2–4 clients.
7. Log the numbers into `BUILD_STATUS.md` → Playtest section; regressions >10 % vs the previous capture
   open an issue before merge (master plan §11 rule).

**Expected bottlenecks (predictions, to be verified):** journal per-frame actor iteration; Echo per-frame
needs tick at Tier0 with many Echoes; AI `SetTimerForNextTick` loop (§4); nothing on the render thread
beyond engine basics with placeholder shapes.

---

## 7. What "Performant" Means for Definition-of-Done

- 60 FPS sustained in Dawn Fields on the mid-range reference machine during the standard capture procedure.
- No GC spikes > 1 ms sustained during the capture (allocation policy enforced after §4 fixes).
- Autosave hitch measured and bounded (target < 100 ms frame spike on the reference machine).
- These are acceptance criteria, **not achieved results** — nothing has been measured yet.
