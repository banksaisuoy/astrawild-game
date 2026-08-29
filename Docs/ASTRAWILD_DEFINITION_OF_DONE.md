# ASTRAWILD — Definition of Done

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-29**

This is the quality bar for the project and the **honest ledger** of where every system stands against it.
Nothing may be reported as complete that has not passed the bar. Fake completion is forbidden (§51 — see §4).

---

## 1. The Quality Bar

A system (or milestone) is **DONE** only when all seven pillars pass:

| Pillar | Requirement | Verified how |
|---|---|---|
| **Functional** | The system performs its designed behavior end-to-end in-game | Manual playtest checklist steps (`ASTRAWILD_TEST_PLAN.md` §2) |
| **Stable** | No crashes, no assert failures, no log spam above Warning during its use cases | Playtest Output Log review |
| **Tested** | Automation tests green (where world-free logic exists) + the relevant manual steps recorded with results | Session Frontend run + filled `BUILD_STATUS.md` Playtest table |
| **SaveLoad** | System state round-trips through `SaveWorld`/`LoadWorld` without loss or corruption | F5/F9 + restart verification (steps 14–17) |
| **Multiplayer-aware** | All mutations run server-side behind authority guards; client paths send intent only | Code review vs `ASTRAWILD_MULTIPLAYER.md` §1 rules; listen-host smoke test for the loop |
| **Performant** | No per-frame allocations/loops beyond the documented budget; cost measured with Insights where relevant | Performance doc §6 capture procedure |
| **Documented** | A doc exists that matches the code (values cited from source, gaps labeled PLANNED/NOT IMPLEMENTED) | This docs suite; doc drift = not done |

**Global precondition:** the repository compiles (`ASTRAWILDEditor Win64 Development`) on the target
machine. **No system can be DONE before this precondition passes**, because Functional/Stable/Tested are
unverifiable without a running editor.

---

## 2. Current Honest Status Per System

Legend: **CODE-COMPLETE** = implemented in C++, pending compile+playtest verification (cannot be DONE yet).
**PARTIAL** = implemented with listed gaps. **PLANNED** = not started.

| System | Code status | vs the 7 pillars | Blocking for DONE |
|---|---|---|---|
| Foundation (types/tags/logging/data assets) | CODE-COMPLETE | Functional/Stable/Tested **unverified** | Compile; tests 1–7 |
| World state (time/weather/GameState) | CODE-COMPLETE | — | Compile + steps 5, 17 |
| Procedural Dawn Fields (bootstrapper) | CODE-COMPLETE | — | Compile + step 1 |
| Player character + runtime input | CODE-COMPLETE | — | Compile + step 4 |
| Survival | CODE-COMPLETE | Cold/heat path unreachable by default content (T-4); no consume keybind (T-5) | Compile + steps 5, 17 + T-5 decision |
| Combat | CODE-COMPLETE | Hit feedback/VFX absent (documented) | Compile + step 7 |
| Echo creature (needs/personality/growth) | CODE-COMPLETE | — | Compile + steps 6–8 |
| Echo AI | PARTIAL | 6 of 16 states without executors; LOD think rate not applied (T-2) | Compile + T-2 fix |
| Capture pipeline | CODE-COMPLETE | — | Compile + step 8 |
| Field journal | CODE-COMPLETE | Per-frame loop hotspot (T-6) | Compile + Insights check |
| Roster/party | CODE-COMPLETE | — | Compile + step 8 |
| Inventory | CODE-COMPLETE | No UI | Compile + step 9 (stopgap acceptable this round) |
| Item registry + content library | CODE-COMPLETE | — | Compile (implicit in everything) |
| Crafting | CODE-COMPLETE | Station-interact stopgap UX | Compile + step 11 |
| Building placement + actors | CODE-COMPLETE | No ghost tint, no deconstruct | Compile + step 12 |
| Power grid | CODE-COMPLETE | Simplified proximity grid v1 | Compile + step 12 (build dynamo + lamp) |
| Echo work sites | CODE-COMPLETE | Output pickup not wired | Compile + step 13 |
| Research/tech tree | CODE-COMPLETE | No UI | Compile + step 12 (Spark quest path) |
| Quests | PARTIAL | 3 objective types unwired (T-1) | Compile + T-1 fix |
| Save v2 | CODE-COMPLETE | No atomic write/backups | Compile + steps 14–17 |
| Multiplayer readiness | PARTIAL | Authority + replication in code; **zero netcode testing**; quests/save per-player missing | M10 phase |
| HUD | CODE-COMPLETE | Cosmetic gaps (temp label) | Compile + steps 5–8 |
| Debug/cheats | CODE-COMPLETE | — | Compile + cheat matrix (Test Plan §3) |
| NPCs | PARTIAL (shell) | No content, no schedule | M11 |
| Automation tests | CODE-COMPLETE (8) | Never executed | Compile + suite run |
| Docs suite (23 files) | DONE* | *accurate to code as of 2026-08-29; will drift as code changes — update rule below | — |

**Project-level status: `PARTIAL` — source-complete, uncompiled, unplaytested.** Overall progress claim
that may be made honestly: *"The full vertical-slice foundation is implemented in C++ in one module and
documented; it awaits compile + playtest validation on the target machine (M7)."*

---

## 3. Doc Update Rule

Docs are DONE only while they match the code. Whenever a system changes: update its doc in the same
commit/round (values cited from source must stay true). The Test Plan §5 issue list and
`BUILD_STATUS.md` are the living trackers between rounds.

---

## 4. What Counts as Fake Completion (FORBIDDEN)

The following claims/actions are **banned** on this project:

1. **Claiming a system "works"** because the code exists — uncompiled code is unverified code.
2. **Marking Compile/Playtest rows as PASS without running them** on the target machine (BUILD_STATUS
   stays `NOT_RUN` until real results exist).
3. **Writing docs that describe features that aren't in the code** (invented features) — every doc claim
   here cites source files; keep it that way.
4. **Hiding known gaps** — every doc in this suite has a "Not Implemented (honest)" section; removing or
   omitting known issues to look complete is fake completion.
5. **Test results invented or extrapolated** ("should pass", "8/8 expected" ≠ "8/8 passed" — expectations
   are labeled as expectations until executed).
6. **Declaring the vertical slice / milestone COMPLETE before the §1 pillars pass** — especially M7
   (compile) which gates everything this round.
7. **"Playable" claims based on the cancelled web prototype** — the web slice is LEGACY, design-validation
   only (see `WEB_PLAYABLE_SLICE.md` header).
8. **Silent scope cuts** — if a directive step is descoped (e.g. module split deferred), it is recorded in
   `ASTRAWILD_ASSUMPTIONS.md`, not quietly dropped.

**Why this section exists:** the sandbox cannot compile UE5, so the honest boundary of this delivery is
"source-complete + documented + self-audited". Crossing that boundary in reporting would poison every
downstream decision (see worklog Task 0, §53 constraint).
