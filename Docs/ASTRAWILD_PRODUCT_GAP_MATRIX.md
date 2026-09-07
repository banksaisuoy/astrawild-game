# ASTRAWILD — PRODUCT GAP MATRIX (FINAL PRODUCT COMPLETION RUN)

**Run**: PCR (Product Completion Run) — opened by user directive "FINAL PRODUCT
COMPLETION RUN — DO NOT STOP AT SOURCE-COMPLETE". **CLOSED at PCR-6: PG-1..PG-6 all delivered (see §4 below).**
**Baseline**: `final-completion` @ 4e52548 (LCP-8 gate — READY_FOR_FINAL_BUILD
re-declared, docs-side). This run audits the PRODUCT (not the paperwork) and
closes every remaining source-side gap with player value.
**Method**: full-repo audit at the actual HEAD (Source 183 files / ~62k LOC,
Content 432, ArtSource 4,069, Docs 136, both validators PASS at baseline).
**Status vocabulary**: unchanged (MASTER_CONTROL §0). Nothing in this run may
claim engine verification.

---

## 1. Audit method (what was actually inspected)

- Git truth: branch/HEAD/remote HEAD/status/log (all 30 commits of the DP+LCP era).
- MASTER_CONTROL v6.1 + TASK_REGISTRY §J + READINESS §M/§O residual ledger.
- Source: module file census, ArtPack binding model, EchoCharacter opt-in mesh
  contract, Journal/Roster/POI/Zone/WorldEvent subsystem public APIs, widget
  class census, input map (31 keyboard keys + gamepad), PlayerController screen
  management, pause-menu surfaces, automation-test registration pattern.
- ArtSource: Meshes (Echoes 14 / Weapons 5 / Environment 20 / Vehicles 1),
  Audio (36 authored + 3 Kenney packs), Models (14 acquisition packs),
  manifest.json + ASSET_MANIFEST.json.
- Validator gates: 119-test exact gate, 15 census equality gates, check-8
  asset-path resolution, LFS routing (.gitattributes).

## 2. GAP MATRIX (source-side, player-valued — the actionable set)

| ID | AREA | CURRENT (verified) | ACTUAL GAP | PLAYER IMPACT | PRIORITY | DEPENDENCY | STATUS | IMPLEMENTATION | VALIDATION |
| :-- | :-- | :-- | :-- | :-- | :-- | :-- | :-- | :-- | :-- |
| PG-1 | UI/UX — Field Journal (Bestiary) | JournalSubsystem tracks 229 species (scan/food/habitat/weakness flags, observation progress, encounter count), saved, knowledge milestones feed capture bonus + RP — **zero UI consumer** (`GetAllEntries()` has no widget caller) | No payoff surface for the core "scan → discover" loop; the Pokédex moment does not exist | HIGH — the discovery loop is a core pillar; players cannot see progress at all | **P0** | none | **OPEN → PCR-1** | `AstrawildJournalScreenWidget` (list + knowledge state + totals + capture-bonus hint), key **P**, pause-menu button, HUD hint | new contract test (count/flags/registry-surface) + validators |
| PG-2 | UI/UX — Echo Roster/Party | Roster subsystem (per-player partitioned, saved, party spawn/work) + `CyclePartyCommand` on C — **no roster UI** (LCP-5 note: "no client roster UI exists") | Player cannot see captured Echoes' element/role/level/bond/abilities or choose who is in the party; capture payoff is invisible | HIGH — the creature-RPG management surface; "why capture this?" cannot be answered in-game | **P0** | none | **OPEN → PCR-2** | `AstrawildRosterScreenWidget` (row per Echo + party toggle + work affinity display), key **L**, pause-menu button | new contract test + validators |
| PG-3 | UI/UX — World Map | 12 zones with FBox2D bounds + 17 POIs (discovery state saved) + 2 villages + 3 dungeons + live world events + quest tracker — **no map screen** | Open-world navigation is memory-only; zone banners are the only orientation | HIGH — "reason to enter each region" is invisible without a map | **P0** | none | **OPEN → PCR-3** | `AstrawildMapScreenWidget` (12-zone grid, zone name/threat/hazard, discovered POIs, villages/dungeons, player marker, active-event pulses, quest-target zone), key **M**, pause-menu button | new contract test + validators |
| PG-4 | Creatures — Tier-B visual identity | 14 Tier-A bespoke meshes (IMPORT_READY); ~55 Tier-B species (zone signatures, dungeon pools, monolith/colossus line, Huge/Epic+ rarity rule) render on PMC procedural bodies; strategy §6 specifies the full archetype library | Zone signature species share generic procedural bodies — silhouette variety is weak exactly where the player spends the most time | HIGH — every zone encounter stops reading as "a box" | **P1** (explicit MISSING residual in READINESS §M-a) | ArtSourceGen infra (exists) | **OPEN → PCR-4** | 8 body-plan archetype generators + per-species parameter bakes (unique GLB per species) + convention-path opt-in binding (no engine-side code patch) + manifest | validate_glb per file + manifest coherence + new contract test |
| PG-5 | Post-game — hunts | MQ-17 = chain terminus; post-game = world events + economy + dungeons (all continue); READINESS/MASTER_CONTROL claim "hunts" keep running — **no hunt system exists anywhere in source** (grep: 0 hunt/bounty rows) | The post-game claim is UNPROVEN; after the ending the player has no repeatable directed activity tied to the creature systems | MEDIUM-HIGH — post-game purpose; fixes a doc-vs-repo honesty gap | **P1** | Journal/defeat counters | **OPEN → PCR-5** | Repeatable hunt contracts (cull/capture targets by zone, scaling rewards from EXISTING item tables — census unchanged), village bounty board surface, saved per player | new contract test + census gates re-run (must stay unchanged) |
| PG-6 | Docs — claim sync | "post-game: ... hunts ..." claims in active docs | Covered by PG-5 implementation + doc sweep | Honesty | P2 | PG-5 | **OPEN → PCR-5/6** | doc consistency sweep in the final gate | validators |

**Verified NON-gaps** (audited, no action): audio/VFX hooks are live in gameplay
code (weapons fire/impact, capture stinger, echo impact cues, ambience per
biome, VfxActor element palette); NPC affinity + schedules + affinity-gated
dialogue all live; farming/breeding/genetics/mounts/skiff/traversal all live;
save V5 covers every system incl. journal/roster/affinity/coop blocks; the
8 story/boss mesh binding rows are correctly queued engine-side (§20c
assets-first contract — binding before `.uasset` import would fail check 8);
LAN LCP-1..8 closed; performance static scans done (FCR/DP gates).

## 3. Run plan (batches — each: implement + validators + registry + docs + commit + push)

- **PCR-0** (this doc): gap matrix + registry §K opened + MASTER_CONTROL v7.0 header.
- **PCR-1**: PG-1 Field Journal screen (+test 120).
- **PCR-2**: PG-2 Echo Roster screen (+test 121).
- **PCR-3**: PG-3 World Map screen (+test 122).
- **PCR-4**: PG-4 Tier-B archetype library, part 1 (shared rig-kit refactor +
  Quadruped/Amorphous/Floating/Biped + zone-signature bakes).
- **PCR-5**: PG-4 part 2 (Avian/Serpent/Insectoid/Crystalline + remaining rule
  members + convention-path binding + manifest + test 123) and PG-5 hunt
  system (+test 124).
- **PCR-6**: final gate — validators, census equality re-run, doc-consistency
  sweep (test counts + hunt-claim fix + residual ledger rewrite), READINESS
  §P, HANDOFF coherence (Tier-B import expectations + the 3 new screens),
  registry §K close, FREEZE.

Stop condition for the run: PG-1..PG-6 all closed or carry documented
engine-side dispositions, both validators PASS, census gates unchanged
(unless a batch legitimately adds content — then all docs move together).

---

## 4. CLOSURE LEDGER (PCR-6 — every gap dispositioned)

| ID | Disposition | Delivered by |
| :-- | :--- | :--- |
| PG-1 | **CLOSED** — Field Journal (bestiary) screen: 229-species codex, knowledge flags, observation %, encounter counts, registry-derived totals, "???" collection pull, key P + pause button + gamepad path (test 120) | PCR-1 (8dab302) |
| PG-2 | **CLOSED** — Echo Roster screen: bench/deploy party-ring management, bBenched additive field (save-safe), replicated per-player roster mirror for LAN clients, server-authoritative mutations + **ExportForSave ownership-strip defect fix** (co-op saves used to orphan spawned rows) (test 121) | PCR-2 (3c6ab2a) |
| PG-3 | **CLOSED** — World Map screen: 12-zone grid with threat/hazard tints, discovered-POI dots (undiscovered stay hidden), villages, dungeons, active-event pins, player marker, objective line + quest-target highlight, pure projection (test 122) | PCR-3 (9adbaa6) |
| PG-4 | **CLOSED** — Tier-B archetype mesh library: 8 body-plan builders, 39 unique variant-baked GLBs (every zone signature + dungeon pools + monolith/colossus + Huge; 4.8 MB, validate_glb PASS ×39, LFS + manifest), definition-driven convention-path binding with ZERO engine-side patching (test 123 + validator §9b) — the READINESS §M-a "MISSING" residual is executed | PCR-4 (ea14f65) |
| PG-5 | **CLOSED** — Post-game hunt system: 8 repeatable cull contracts (existing species + existing items — census unchanged), event-bus defeat progress, server-authoritative claims with round reset, additive save rows, Hunt Board screen key U (test 124) — the "hunts keep running" doc claim is now TRUE | PCR-5 (7c42eca) |
| PG-6 | **CLOSED** — doc-claim sync: every active doc carries ONE test count (124), the hunt claim is backed by a system, the residual ledger rewritten (READINESS §P), the strategy §10 backlog marks P1/P1.5 EXECUTED, LAN_COOP_SPEC's roster-mirror exception superseded | PCR-1..PCR-6 |

**Stop condition check**: PG-1..PG-6 all closed; both validators PASS (124-test gate,
15 census gates unchanged, 39-species Tier-B coherence gate); no known source-side
product gap remains. The engine-side queue is unchanged: AG-2..AG-5 (§20) + §22
LAN acceptance, now including the 39 Tier-B GLB imports (riding the §20b baseline
pass — no code patch needed) and PIE spot-checks for the four new screens.
