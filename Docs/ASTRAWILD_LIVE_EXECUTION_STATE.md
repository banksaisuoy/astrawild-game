# ASTRAWILD — LIVE EXECUTION STATE

> **CANONICAL LIVE EXECUTION STATE (directive: MASTER SYNCHRONIZATION + SINGLE
> EXECUTION QUEUE LOCK, 2026-09-06).** This is the **ONLY** active task-state
> document. `Docs/ASTRAWILD_MASTER_CONTROL.md` remains the canonical
> GAME/PRODUCT rulebook; this file is the canonical LIVE EXECUTION STATE.
> When any other document's status block disagrees with this file, THIS FILE
> wins for execution state. Historical documents are evidence, never status.
>
> **Custodian**: GLM 5.3 (Lead Programmer / Game Architect). Roles are LOCKED
> (see §6). No agent may silently modify another agent's role.
> **Sync discipline**: every task batch MUST end with
> COMMIT → PUSH → UPDATE THIS FILE (§1 + §5) → UPDATE TASK REGISTRY.

---

## 1. LIVE STATE SNAPSHOT

| Field | Value (evidence-derived at this sync) |
|---|---|
| **CURRENT_HEAD** | `2637c1390f8c6756796ed553fbe34cbfa8e10b06` (v9.3 ASSET OVERHAUL — the product/content tip this state describes) · repo tip = that + the v9.4 SYNC-LOCK docs-only commits (see §5; a live-state file always trails its own last commit by one docs-only commit) |
| **ACTIVE_BRANCH** | `final-completion` (authoritative until real final engine acceptance; `main` @ `94a398c939678963e1d0e0a8baac8cddba2e8d90` is the frozen baseline mirror — never the dev base, never merged into during sync passes) |
| **LAST_SYNC_TIME** | 2026-09-06 07:59 UTC (MASTER SYNC session — this file created) |
| **CURRENT_PHASE** | DCP (DEFERRED COMPLETION PACK) — user-authorized re-opening of every deferred item; playable-first ordering; ENGINE-RUN-1 remains the queue head for engine evidence |
| **CURRENT_TASK** | **DCP-1..DCP-7 — DEFERRED COMPLETION PACK** (user directive 2026-09-06, quote: "งานที่ถูก defer ทำให้ครบหมด... ตีกรอบเอง เอาที่เล่นได้ก่อน"): re-open ALL deferred-by-design items, implement them source-side, playable-first. LIVE at §3 |
| **NEXT_TASK** | V2-36 engine run (see §3 NOW) |
| **BLOCKED_TASKS** | V2-29..V2-36 (external: no UE5/MSVC on the Linux source sandbox — the Windows UE 5.8.2 machine is the exclusive runtime authority) |
| **CURRENT_TEST_COUNT** | 126 world-free automation contracts (validator 126-test gate PASS at tip) |
| **CURRENT_CONTENT_COUNT** | 416 genuine UE packages in `Content/` (magic 0x9E2A83C1 re-walked; 0 bad) |
| **CURRENT_LFS_COUNT** | 586/586 pointers resolve with on-disk objects (491 pre-v9.3 + 95 v9.3 real-mesh source files); `git lfs push --dry-run` = 0 pending upload; see the fsck caveat in §8 |
| **CURRENT_ASSET_IMPORT_PENDING** | Manifest (file-truth basis): **189/189 `present` / 0 pending**; engine import of the 109 real meshes + clips = **NOT_RUN** (V2-36 one-click `Setup_And_Play.bat`); TRUE_MISSING = 0 |
| **CURRENT_KNOWN_RISKS** | see §8 |

**CURRENT_PRODUCT_STATE**: SOURCE-COMPLETE + ENGINE-UNVERIFIED. Not "100%
verified", not "fully playable", not "released". Git existence ≠ UE5 runtime
verification — an LFS pointer is not the binary, an unimported raw GLB is not
an engine-usable asset.

---

## 2. STATUS VOCABULARY (binding state machine — no substitute words)

```
PLANNED → IN_PROGRESS → IMPLEMENTED → STATIC_VERIFIED → ENGINE_UNVERIFIED → ENGINE_VERIFIED → ACCEPTED
                    ↘ BLOCKED                                        ↘ SUPERSEDED (any state, with reason)
```

| State | Meaning |
|---|---|
| `PLANNED` | registered, dependencies not yet read, no implementation |
| `IN_PROGRESS` | actively being implemented right now (must appear in §3) |
| `BLOCKED` | cannot proceed — external dependency or evidence missing (reason mandatory) |
| `IMPLEMENTED` | code/content exists and is syntactically coherent |
| `STATIC_VERIFIED` | passed the static validators (validate_final_run.py + validate_repository.sh) at its tip |
| `ENGINE_UNVERIFIED` | source-side done, engine-side claims NOT allowed (the default for everything runtime) |
| `ENGINE_VERIFIED` | raw engine evidence exists at a pinned SHA (Antigravity-issued only) |
| `ACCEPTED` | final engine acceptance occurred (product gate) |
| `SUPERSEDED` | replaced by a newer directive/task — history preserved, never deleted |

Completion definition per task class: SOURCE task = IMPLEMENTED + static
validation; ENGINE task = ENGINE_VERIFIED + raw engine evidence; PRODUCT task
= implemented + player-facing evidence; CONTENT task = asset exists + license
verified + manifest + correct intended integration state; DOCUMENTATION task
= current-facing state agrees with Git/source truth. A dashboard showing
"100%" while engine work is ENGINE_UNVERIFIED is a defect, not a result.

---

## 3. ACTIVE TASK QUEUE (the single execution queue)

### NOW (in flight — DCP user directive 2026-09-06)
| ID | Task | State | Owner | Notes |
|---|---|---|---|---|
| **DCP-1** | SQ-23 post-game side-quest batch (side quests are real content the game currently lacks — playable-first) | IN_PROGRESS | GLM | BuildPostGameQuests() + dialogue activation gated on Quest_FirstDawnAgain |
| **DCP-2** | NG+ rules (New Game Plus carryover + reset path — replayability) | PLANNED | GLM | additive save fields + StartNewGamePlus + pause entry |
| **DCP-3** | Ending cinematics (pure-C++ staged camera + letterbox + fade; no Sequencer) | PLANNED | GLM | OnEndingTriggered has zero subscribers today; OnRep_EndingState reserved |
| **DCP-4** | Vess/Ione Act 3 NPCs with distinct real-mesh visuals | PLANNED | GLM | two new NPC rows + VisualMesh field + spawn + trees (census 11→13) |
| **DCP-5** | UI toast sounds + journal per-species detail view | PLANNED | GLM | A_UI_* cues exist unreferenced; journal rows read-only today |
| **DCP-6** | Gamepad smart-cast chord | PLANNED | GLM | UInputModifierChordAction; INPUT_REFERENCE update |
| **DCP-7** | Unique-mesh coverage expansion (14 unused SK_Boss_* spares → priority species via explicit art rows) | PLANNED | GLM | Wavecrest/Undertowray/Voidwing/Verdantbloom/Voltmaw + authored story species |
| **ENGINE-RUN-1** | One-time engine integration: `Setup_And_Play.bat` → import 109 real meshes + clips (`import_report.json` total_missing == 0 incl. AM_ clips) → showcase map PIE (`/Game/Maps/L_Showcase_ArtOverhaul`) → build → 126+DCP tests → PIE golden path §2 → package → V2-29..V2-36 rows | **BLOCKED (external)** | Antigravity | Windows UE 5.8.2 machine only. Runbook: `Docs/ASTRAWILD_FINAL_BUILD_HANDOFF.md` §20/§20b/§20c/§20d/§20e + queue rows. Evidence: report files + clips + BUILD_STATUS playtest table. Fix-forward on FAIL, never silently defer. |

### NEXT (after ENGINE-RUN-1 clears, or if it surfaces real defects)
| ID | Task | State | Owner | Notes |
|---|---|---|---|---|
| LAN-RUNTIME-1 | LAN 4-player listen-server session validation (HANDOFF §22) | ENGINE_UNVERIFIED | Antigravity | source implemented, runtime unverified |
| POST-ACCEPT-1 | final acceptance → merge `final-completion` → `main` (ONLY after real engine acceptance) | PLANNED | GLM + user | merge is forbidden before acceptance |

### BLOCKED
| ID | Task | Blocker |
|---|---|---|
| ENGINE-RUN-1 (= V2-29..V2-36 + build + test + PIE + package) | no UE5/MSVC on the Linux source sandbox — proven external dependency, never faked |
| Mutation runtime visuals (V2-35 acceptance) | same external blocker |
| Showcase-map PIE (V2-36 acceptance) | same external blocker |

### DONE (source-side; latest first — full ledger in MASTER_TASK_REGISTRY §A..§N)
| ID | Task | Final state |
|---|---|---|
| SYNC-LOCK (this session) | MASTER SYNCHRONIZATION + LIVE EXECUTION_STATE created + false-100%/stale-count fixes + registry §N | STATIC_VERIFIED |
| AO-1..AO-5 (v9.3 ASSET OVERHAUL) | 109 real unique CC0 meshes; manifest 189/189 present/0 pending; import_all/showcase/Setup_And_Play.bat; Tier-B 39→36 | STATIC_VERIFIED (commit `2637c13`) |
| FINAL-EXEC-2 (v9.2) | truth re-verification: LFS 459→491, manifest 159→175, 5 more false-100% banners | STATIC_VERIFIED (commit `08af72b`) |
| SCI-1..SCI-5 (v9.0/v9.1) | 204-spec Sci-Fantasy mutation system + 16 SK_Base + runtime theme-material wiring | STATIC_VERIFIED (commits `0b55072`, `4daa113`) |
| FPP-1..FPP-3 / PCR-0..PCR-6 / LCP-1..LCP-8 / DP-1..DP-10 / SCP / GDP / FINAL-AUDIT A-D / FR-1..14 / BATCH-0..3 | the full final-completion run (see MASTER_CONTROL baseline chain) | STATIC_VERIFIED |

### DEFERRED (was explicit non-goals — **re-opened by user directive 2026-09-06** "ทำให้ครบหมด", now tracked as DCP-1..DCP-7 above; nothing here stays deferred except the two rows below)
| Item | Reason |
|---|---|
| full 1:1 unique mesh for every species beyond the 109-model pool | mathematically impossible without new sources (109 unique models < 229 species); DCP-7 binds every remaining real spare instead |
| NG+ *cinematic re-intro* (beyond the ending cinematic) | NG+ re-entry uses the existing new-game flow; no extra Sequencer-style content |

---

## 4. TASK PERSISTENCE RULE (mandatory)

A task NEVER disappears merely because a new prompt arrives. When a new user
instruction lands: **assign an ID → append to this file's queue (§3) →
determine dependencies → continue the current task unless explicitly
superseded → mark the previous task IN_PROGRESS / BLOCKED / SUPERSEDED with a
reason.** Switching is allowed only when the current task is complete, is
blocked, or the user explicitly supersedes it. Every task follows:
REGISTER → READ DEPENDENCIES → IMPLEMENT → VALIDATE → COMMIT → PUSH →
UPDATE LIVE EXECUTION STATE → UPDATE TASK REGISTRY → NEXT TASK.

---

## 5. EXECUTION-STATE CHANGE LOG (append-only)

| Date | Change | Commit |
|---|---|---|
| 2026-09-06 | FILE CREATED by the MASTER SYNCHRONIZATION directive: snapshot at `2637c13`, queue reconciled, registry §N back-registered, stale-491→586 fixed in live surfaces (HANDOFF §3 pre-flight included), readiness/BUILD_STATUS/PLAYABLE pointers refreshed, Next.js console KPI fallback re-synced + era-classified, fsck honest correction recorded in §8 | `38733dc` |
| 2026-09-06 | prior state: ASSET-OVERHAUL v9.3 delivered (`2637c13`) — recorded here retroactively because the v9.3 session updated MASTER_CONTROL/asset-truth/manifest/queue/README but did NOT register its tasks in MASTER_TASK_REGISTRY (fixed by §N this session) | `2637c13` |
| 2026-09-06 | change-log hygiene: pinned the creation row to `38733dc`; no state change | `69349e9` |
| 2026-09-06 | FINAL sync commit of this session — §5 rows for the sync's own commits + repo-tip convention documented; validators re-run ALL PASS before push; remote HEAD verified after each push | this commit |
| 2026-09-06 | USER DIRECTIVE — DEFERRED COMPLETION PACK (DCP-1..DCP-7): user re-opened ALL deferred-by-design items ("งานที่ถูก defer ทำให้ครบหมด... ตีกรอบเอง เอาที่เล่นได้ก่อน"); playable-first order = SQ-23 quests → NG+ → ending cinematics → Vess/Ione → toast sounds/journal detail → gamepad chord → mesh coverage; registry §O opened | (this row) |

---

## 6. AGENT OWNERSHIP (LOCKED)

| Agent | Role | May NOT |
|---|---|---|
| **GLM** | Lead Programmer / Game Architect / Source + Content Integration Owner | claim runtime PASS |
| **Qwen** | Technical Art Specialist only (difficult materials, shaders, rigging, animation, Niagara, difficult 3D processing) | independently redefine gameplay architecture |
| **Sonnet** | Independent reviewer; findings classified REAL BUG / STALE DOCUMENT / UNPROVEN CLAIM / SCOPE SUGGESTION | change active execution (only REAL BUG / STALE DOCUMENT / UNPROVEN CLAIM may) |
| **Antigravity** | Exclusive UE5/Windows/runtime authority: import, build, compile, engine tests, PIE, package, performance, LAN runtime, engine-only integration fixes | redesign gameplay architecture |

**Branch ownership (until final engine acceptance):** GLM → `final-completion`;
Qwen → `qwen/<task-id>` only; Sonnet → no production branch unless explicitly
needed; Antigravity → engine verification branches. Never force-push, never
rewrite history, never delete unrelated work, push every logical batch with
task ID + commit SHA + state + next task.

---

## 7. DIRECTIVE-BASIS RECONCILIATION (sync-session evidence table)

The 2026-09-06 synchronization directive pinned these values; fresh
repository evidence at the actual tip re-derived every one of them (commands,
not docs — full outputs in the session worklog):

| Directive value | Fresh evidence at `2637c13` | Verdict |
|---|---|---|
| remote HEAD `08af72b` | `git ls-remote origin final-completion` = `2637c139...` | **superseded by evidence** — the v9.3 ASSET-OVERHAUL session landed + pushed `2637c13` on top |
| Master Control v9.2 | header reads v9.3 (asset overhaul) | **superseded by evidence** |
| LFS 491/491 | `git lfs ls-files` = **586 pointers**, 586/586 on-disk objects, `git lfs push --dry-run` = 0 pending | **superseded by evidence** (+95: the v9.3 real-mesh source files) |
| manifest 175 entries / 112 present / 63 pending | manifest.json = **189 entries / 189 `present` / 0 pending** (v9.3 regenerated the manifest on the file-truth basis; engine import of the 109 real meshes is tracked by V2-36, not by manifest `pending`) | **superseded by evidence** — the 63-pending concept is superseded by the v9.3 architecture (RAW file-truth manifest + one-click V2-36 engine import) |
| 47 GLB-backed Echo IDs + 16 SK_Base engine-import pending | all 63 of those rows are inside the 189 present sources; engine import for ALL 109 real meshes = NOT_RUN (V2-36) | superseded vocabulary — see row above |
| Content 416 genuine UE packages | re-walked = **416** | confirmed |
| automation contracts 126 | grep = **126** test macros; validator 126-test gate PASS | confirmed |
| Echo species 229 / bestiary rows 204 / hero assets 6 | validator census gates PASS (229; 204-row table; 6 hero) | confirmed |
| V2-29..V2-35 NOT_RUN | queue rows NOT_RUN + no report/clip artifacts at tip | confirmed (V2-36 also NOT_RUN) |
| UE5/MSVC unavailable on this Linux sandbox | re-verified (`/home/z`, `/opt`, `/usr/local`, no Windows mounts) | confirmed → ENGINE-RUN-1 stays BLOCKED (external) |
| `main` 94a398c ≠ `final-completion` | verified; main untouched by this sync | confirmed |

Historical-document rule enforced: every "100% VERIFIED / GREEN / 54/54 /
115/115 / packaged executable / 8313c61 / 03c2fe6 / agent/antigravity-ue5-v2"
occurrence in current-facing docs is either era-bannered history or a
factually-true fraction — none presents as current status after this sync.

---

## 8. CURRENT KNOWN RISKS (honest ledger)

1. **Engine import NOT_RUN** (the only product-critical blocker): no engine
   packages exist yet for the 109 real meshes — sources are 100% present on
   disk (manifest 189/189). Conversion gate = V2-36 evidence
   (`import_report.json` total_missing == 0 incl. clips + showcase PIE).
2. **LFS fsck convention mismatch (pre-existing, documented honestly)**:
   `git lfs fsck` exits 1 with ~3,954 "should have been a pointer" flags —
   ALL in raw pack-source dirs (`ArtSource/Textures/Kenney*`, `Models/Kenney`,
   `Audio/Kenney`, `Models/Quaternius`); ZERO under `ArtSource/Meshes` after
   v9.3 (that commit converted the previous raw mesh/texture files to proper
   LFS pointers and reduced the flag count by 79). Root cause: earlier
   acquisition batches committed pack sources as plain git objects while
   `.gitattributes` declares those extensions LFS-tracked. It is a convention
   mismatch, not corruption — every current pointer's object resolves
   OID-matched. The v9.2-era "fsck exit 0" statement was inaccurate (corrected
   here; era text preserved in asset-truth §10b history).
3. **Historical import evidence is stale vs the current tip**: the committed
   `import_report.json` (115/115, total_missing 0) was produced at `8313c61`
   on `agent/antigravity-ue5-v2` — it predates Tier-B + SCI + v9.3. The final
   tip requires its one-time re-import inside the §20/V2-36 sequence
   (idempotent — the report re-derives).
4. **Runtime mutation visuals unproven**: theme-material swap, attachments,
   VFX and per-part scaling are wired and static-verified but their runtime
   appearance is ENGINE-UNVERIFIED (V2-35).
5. **Landscape material assignment** is an editor-only manual step
   (HANDOFF §19) before V2-32 can pass.
6. **Test count vocabulary**: "126 world-free contracts" = static gate; the
   54/54-era engine PASS numbers belong to their historical SHAs only.

---

## 9. CURRENT-FACING DOCUMENT MAP (post-sync)

| Surface | Role |
|---|---|
| **THIS FILE** | canonical LIVE EXECUTION STATE (task queue, head, risks) |
| `Docs/ASTRAWILD_MASTER_CONTROL.md` (v9.3 + sync amendment) | canonical GAME/PRODUCT rulebook |
| `Docs/ASTRAWILD_MASTER_TASK_REGISTRY.md` §A..§N | task ledger (persistent history) |
| `Docs/ASTRAWILD_ENGINE_VERIFICATION_QUEUE.md` | engine rows V2-29..V2-36 + status notes |
| `Docs/ASTRAWILD_FINAL_BUILD_HANDOFF.md` | Antigravity runbook (§20/§20b-e, §22 LAN) |
| `Docs/ASTRAWILD_CURRENT_ASSET_TRUTH.md` | asset truth (§12 = v9.3 real-mesh truth; §1-§10b era-correct history) |
| `Docs/ASTRAWILD_FINAL_READINESS_REPORT.md` | readiness verdict (top status synced to v9.3) |
| `README.md` + `Docs/BUILD_STATUS.md` + `Docs/PLAYABLE_BUILD_STATUS.md` | entry points, pointers synced |
| Next.js Production Console (`/home/z/my-project`) | progress dashboard; live API + re-synced static fallback |

Everything else in `Docs/` is HISTORICAL evidence unless explicitly listed
here. Product canon (v9.5 DCP update): 12 zones · 229 Echo species · 22 quests (17 MQ + 5 post-game DCP-1) · 17
techs · 58 recipes · 26 building pieces · 11 NPCs · 3 dungeons · 4 bosses ·
Ending A/B + post-game · private LAN 4-player.
