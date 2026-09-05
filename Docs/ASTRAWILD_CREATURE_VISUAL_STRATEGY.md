# ASTRAWILD — CREATURE VISUAL STRATEGY (Tier A / B / C)

**Document Version**: 1.2
**Custodian**: GLM 5.3 — Lead Programmer / Game Architect (source-side)
**Status**: SOURCE-VERIFIED strategy — all deliverables below are `IMPORT_READY` at most; **nothing here is `UE5_VERIFIED`**. Engine import, binding and PIE verification belong to the Antigravity run per `ASTRAWILD_FINAL_BUILD_HANDOFF.md` §20/§20b.
**Origin**: Wayfinder ticket 07 (user directive 2026-09-05 — "create a REAL CREATURE VISUAL STRATEGY as the next post-Batch-2 task"). v1.1 (V25-C1): §10 P0 boss-mesh row executed — 4 bespoke boss meshes delivered (A7-A10). v1.2 (V25-C2): §10 P0 story-species row executed — 4 bespoke story meshes delivered (A11-A14); Tier-A bespoke set now complete at 14 meshes.
**Baseline**: `final-completion` @ `a09e566` (post batch-2 acquisition).

---

## 1. Objective

229 Echo species must be visually distinguishable **where it matters** — without commissioning 229 unique high-cost meshes and without shipping indistinguishable placeholders. The player's test for every species that actually appears in the world:

> "That Echo is useful because it does X" — never "that is another recolor."

The strategy is a **tiered investment curve**: bespoke art where story pressure is highest, shared archetype rigs where encounter frequency is highest, and a strengthened procedural system for the long tail. It slots into the existing runtime path — `UAstrawildEchoDefinition::SkeletalMesh/IdleAnimation/MoveAnimation` soft refs with automatic procedural-mesh fallback — so no new binding architecture is created.

## 2. Census baseline (what exists today)

| Fact | Value | Source |
|---|---|---|
| Total species | **229** = 10 authored starters + 6 production heroes + 3 final-run boss species + 6 evolution targets + 204 bestiary rows | `AstrawildContentLibrary.cpp:1426-1470`, `AstrawildProductionContent.cpp:2343-2366`, `AstrawildBestiaryData.cpp:53-2094`; enforced by `validate_final_run.py` census gate |
| Species with skinned+animated meshes | **14** (Terraquill, Cindermule, Voltpylon, Bastionbeetle, Mistmender, Deepdelver + the 4 boss meshes DrownedSovereign/GlassTyrant/Dawnfang/EyeSentinel + the 4 story meshes Lumewisp/Sprigling/Gloomfang/Auroraling — each a bespoke `Tools/ArtSourceGen/gen_echo_*.py` with Idle/Move/Hit clips) + 6 evolution targets reusing them as tint/scale variants | `ArtSource/Meshes/Echoes/`, `AstrawildArtPack.cpp:72-84` |
| Species rendering as procedural bodies | **215** (8 body-plan archetypes × 5 size classes × 2 tints, vertex-colored PMC) | `AstrawildEchoCharacter.cpp:216-381` |
| Boss visuals | **4 boss fights still render as the engine Cone placeholder at runtime** (scale 2.4 + sphere weak point) — the source-side replacement meshes now exist (A7-A10) and await Antigravity import/binding per §9 | `AstrawildEchoBossCharacter.cpp:32-60` |
| Species ever actually spawned | **~46** (zone-wildlife table + camp spawns + hostile-spawner floors + dungeon pools + one-per-world Auroraling) | `WorldBootstrapper.cpp:64-104,589,638-655,836`; `HostileSpawnerSubsystem.h:36-48` |
| Body-plan distribution | Quadruped 55 · Amorphous 36 · Floating 28 · Biped 24 · Avian 21 · Serpent 18 · Insectoid 16 · Crystalline 14 | `AstrawildBestiaryData.cpp` rows |
| Locomotion classes | `Auto / Land / Water / Flying` derived from family+body-plan+zone | `AstrawildTypes.h:1142-1149`, `AstrawildEchoCharacter.cpp:2003-2028` |
| Creature animation clips | Idle/Move wired; **Hit authored but unwired** | `AstrawildArtPack.h:36-42` |
| External CC0 3D-creature sources | **None usable**: Kenney creature packs are 2D; Quaternius Ultimate series is CC0 but delivery is a Google Drive folder (deferred — no direct URL); Quaternius newer packs are QAL (forbidden) | Wayfinder research tickets 01/02 |

Implications: (a) the long tail is registry/codex data only — per-body-plan coverage beats per-species coverage; (b) the biggest visible win per hour of work is **bespoke boss + story meshes**, not more prop species; (c) a shared archetype rig library removes the current one-script-per-species cost curve.

## 3. Tier definitions

| Tier | Definition | Visual treatment | Count |
|---|---|---|---|
| **A — Hero / story / boss** | Starter companion, quest-critical species, dungeon/world/final bosses, one-per-world rares, production heroes and their evolutions | Bespoke `ArtSourceGen` mesh + dedicated material set + authored Idle/Move(/Hit) clips + unique silhouette; evolution variants = tint/scale/glow escalation of the base mesh | **20** |
| **B — Major wild species** | Every species referenced by a runtime spawn surface (zone wildlife, camp spawns, hostile-spawner floors, dungeon pools, world-event boosts) **plus** every Huge size-class species and every Epic/Legendary rarity | Shared **archetype rig library** (8 body-plan rigs with Idle/Move/Hit) + per-species variation parameters: proportions, feature toggles (horns/crests/plates/fins), scale band, tint pair, element emissive | **≈55** (rule-computed; named roster in §5) |
| **C — Common species** | The remaining long tail (codex/registry species not currently spawned) | Procedural PMC body (existing system) + **strengthened identity**: guaranteed silhouette-family separation, element→emissive palette, family→surface material, pattern/accent variation, rarity ring | **≈154** |

Tier membership is **deterministic and code-greppable** (spawn tables, `SizeClass == Huge`, `Rarity >= Epic`) — never a taste call. Promotion rule: when a world-depth batch starts spawning a C species (e.g. a new zone event), that species is promoted to B in the same batch and gets archetype-rig coverage in the next ArtSourceGen batch.

## 4. Tier A roster (20 species — full list)

| # | Species | Story anchor | Visual source | Priority |
|---|---|---|---|---|
| A1 | **Terraquill** (+TerraquillVerdant) | Hero companion, Dawn Fields, evolution chain | `SK_Echo_Terraquill.glb` **EXISTS**; evolution = Verdant tint + scale +1 band + emissive boost | done (IMPORT_READY) |
| A2 | **Cindermule** (+CindermulePyre) | Hero, Ember Ridge | `SK_Echo_Cindermule.glb` **EXISTS**; Pyre variant rule same | done |
| A3 | **Voltpylon** (+VoltpylonTempest) | Hero, Glimmerwood | `SK_Echo_Voltpylon.glb` **EXISTS** | done |
| A4 | **Bastionbeetle** (+BastionbeetleBulwark) | Hero, Verdant Reach | `SK_Echo_Bastionbeetle.glb` **EXISTS** | done |
| A5 | **Mistmender** (+MistmenderRime) | Hero, Dusk Marsh (flying) | `SK_Echo_Mistmender.glb` **EXISTS** | done |
| A6 | **Deepdelver** (+DeepdelverAbyssal) | Hero, Stormcrest Highlands | `SK_Echo_Deepdelver.glb` **EXISTS** | done |
| A7 | **DrownedSovereign** | **MQ-16 FINAL BOSS** (3 phases, Eye of the Maelstrom) | `SK_Echo_DrownedSovereign.glb` **EXISTS — IMPORT_READY** (V25-C1) | done |
| A8 | **GlassTyrant** | **MQ-14 world boss** (Sunscar) | `SK_Echo_GlassTyrant.glb` **EXISTS — IMPORT_READY** (V25-C1) | done |
| A9 | **Dawnfang** | **Sunken Vault dungeon boss** (MQ-10) | `SK_Echo_Dawnfang.glb` **EXISTS — IMPORT_READY** (V25-C1) | done |
| A10 | **EyeSentinel** | Eye dungeon adds + Sovereign summons (MQ-15/16) | `SK_Echo_EyeSentinel.glb` **EXISTS — IMPORT_READY** (V25-C1) | done |
| A11 | **Gloomfang** | MQ-05 hunt target **+ Underlight Warden (dungeon-1 boss)** + night-raid raider | `SK_Echo_Gloomfang.glb` **EXISTS — IMPORT_READY** (V25-C2) | done |
| A12 | **Lumewisp** | **Starter companion (MQ-02)** — the first Echo every player bonds with | `SK_Echo_Lumewisp.glb` **EXISTS — IMPORT_READY** (V25-C2) | done |
| A13 | **Sprigling** | MQ-06 capture lesson + Great Migration event | `SK_Echo_Sprigling.glb` **EXISTS — IMPORT_READY** (V25-C2) | done |
| A14 | **Auroraling** | One-per-world Ancient rare + Rare Echo Bloom event | `SK_Echo_Auroraling.glb` **EXISTS — IMPORT_READY** (V25-C2) | done |

Evolution variants (6) reuse the base hero mesh with the documented escalation rule (tint shift toward element saturation, +1 size band, emissive ×1.5, optional crest attachment) — no new meshes. All 14 bespoke meshes land in `ArtSource/Meshes/Echoes/` (bosses can sit in `ArtSource/Meshes/Echoes/Bosses/`) and import to `Content/Characters/Echoes/` via the existing AwPipeline — **no second importer**.

## 5. Tier B roster (≈55 — rule + named anchors)

Rule (compute at implementation, verify in review): `species ∈ B` iff referenced by `WorldBootstrapper` zone-wildlife/camp/dungeon-pool spawn tables, `HostileSpawnerSubsystem` floors, world-event species-boost payloads, **or** `SizeClass == Huge`, **or** `Rarity ∈ {Epic, Legendary}`.

Named anchors from today's tables:
- **Zone signatures (17 non-A):** Stonehide, Duskmoth, Emberfang, Rimefang, Voltmaw, Brinefin, Saltcrest, Wavecrest, Mistwing, Sunhide, Glimmerhornet, Sunhorn, Geargolem, Verdantbloom, Fernthorn, Coralray, Pearlcrest
- **Dungeon pools (2):** Lagoonfin, Saltray (with Wavecrest above)
- **Huge-class monolith/colossus line:** Vespermonolith, Monolithprimarch, Primemonolith, Astralmonolith, Eldermonolith, Reliccolossus, Hallowedcolossus, Monolithcolossus, Forgottencolossus (+ Tidewyrm Huge)
- **Remaining zone-wildlife table rows** (~20: Undertowray, Voidwing, Pyreblaze, Pistongolem, Magmawing, Frostblaze, Ghostshade, Sunpaw, Abyssjelly, Embershade, Pyreshard, Downsong, Cometplume, Abysswing, Thermalwing, Voltcore, Wireweevil, Reefskimmer, Voltheart, etc.)

## 6. Body-plan archetype rig library (Tier B engine)

One parameterized generator per body plan, built on a **shared rig kit** (extend `Tools/ArtSourceGen/aw_rig` — today each bespoke script re-declares its own rig; the library factors the 4 chain patterns the 6 heroes already use):

| Body plan | Species covered | Rig spec | Variation parameters |
|---|---|---|---|
| Quadruped | 55 | Root/Hips/Spine×2/Neck/Head/Tail×2/2×(front+rear leg chains) — matches Terraquill/Cindermule rigs | leg length, body barrel scale, neck length, tail length, horn/crest/plate toggles |
| Amorphous | 36 | Root/Core + 6 lattice blobs (Deepdelver pattern) | blob spread, core scale, surface spikes |
| Floating | 28 | Root/Core + 3 orbitals + veil plane (Mistmender pattern) | orbital count/radius, veil alpha, lantern core |
| Biped | 24 | Root/Hips/Spine/Head/2×(arm+leg chains) (Voltpylon pattern) | arm bulk, head crest, hunched stance |
| Avian | 21 | Root/Body/Neck/Head/2×wing chains/Tail | wingspan, tail streamer, plume crest |
| Serpent | 18 | Root/Spine×8 segments + Head + frill | segment count, hood, fin ribbon |
| Insectoid | 16 | Root/Abdomen/Thorax/Head/2×(antenna+4 leg stubs) (Bastionbeetle pattern) | abdomen bulk, mandibles, carapace ridges |
| Crystalline | 14 | Root/Core + shard cluster (convergent with GlassTyrant bespoke) | shard count/length, facet emissive |

Each archetype ships `SK_Plan_<Name>.glb` with 3 authored clips (`Idle/Move/Hit`) and a parameter manifest consumed by per-species variant bakes (`SK_<Species>.glb` = archetype + parameters + tint map) so every B species still gets a **unique GLB with unique proportions/features** — never a plain recolor. Per-zone guarantee (hard rule for new content): a zone's signature species must span ≥3 distinct body plans, so zone encounters stay silhouette-separable.

## 7. Material & color identity system (all tiers)

- **Element → emissive palette** (drives the glow channel on every creature): Light = warm ivory · Ember = magma orange · Frost = glacial cyan · Flora = chlorophyll green · Pulse = electric violet · Ash = ember-grey with dying-coal flicker. WeaknessElement stays a data/canon axis, never a color.
- **Family → surface** (drives base material selection): Beast matte organic · Flora subsurface leaf · Elemental energy-glass · Spirit translucent veil · Dragon scaled hide · Aquatic wet sheen · Avian feather velvet · Insectoid chitin gloss · Construct machined metal · Ancient weathered relic-metal.
- **Tint pair** (Primary/Secondary fields already on every definition) + pattern tier: A = bespoke texture set (4 slots Echo_Body/Echo_Armor/Echo_Emissive/Echo_Eye as the 6 heroes already use); B = archetype texture set + tint mask; C = vertex color + accent (current PMC system) **plus** element emissive point-light (already live via `ApplyVisualIdentity`).
- **Rarity ring** exists and stays. Size bands stay (Tiny 0.45 → Huge 1.9).
- The shared texture set `T_Echo_Body_D/N/ORM` + `T_Echo_Emissive_M` (existing, IMPORT_READY) covers A and B; C needs no new textures.

## 8. Animation & locomotion mapping

| Locomotion class | Archetype Move clip | Runtime path (exists) |
|---|---|---|
| Land | trot/walk cycle per plan | `MoveAnimation` swap at velocity ≥60 (`UpdateSkeletalAnimation`) |
| Flying | glide bob + banking (Floating/Avian plans) | `MOVE_Flying` + 3D steering (`EchoAIController::SteerFlyingToward`) |
| Water | serpentine sway + vertical undulation | Water speed multiplier in sea zones (`GetLocomotionSpeedMultiplier`) |
| Amphibious | Land clip + water entry splash hook | zone-conditional multiplier (same path) |

- **Hit clips**: author per archetype; wire as engine-side task (today authored but unwired — the `AM_*_Hit` montages on the 6 heroes) feeding the existing stagger state (`ApplyStagger`).
- Mounts: riding uses the species Move clip (flying mounts already supported; water mounts are a gameplay-depth item — wayfinder ticket 09, not this doc).
- Evolution: body rebuild from the new definition already re-runs the visual path (`EvolveInstance`).

## 9. Import & binding contract (Antigravity side, per HANDOFF §20b)

1. Import order: audio → models → textures (existing §20b contract unchanged).
2. `ArtSource/Meshes/Echoes/*.glb` (incl. `Bosses/`, `Plans/`) → `Content/Characters/Echoes/` via `import_all.py` (Interchange, idempotent `does_asset_exist` guards).
3. Binding: extend the existing `AstrawildArtPack::GetEchoArt()` table — the **only** binding table; definitions keep soft refs with automatic PMC fallback so a missing import never breaks spawn. Repo contract is **assets-first, binding-second** (how the 6 heroes landed): the 8 new rows land on the engine machine AFTER the imported `.uasset`s are committed — the verbatim row patch + sequence is packaged in HANDOFF §20c. `AAstrawildEchoBossCharacter` already carries the opt-in path source-side (cone hides the moment the boss mesh resolves).
4. Boss meshes: replace the Cone placeholder path in `AstrawildEchoBossCharacter` visual setup only after import succeeds (keep cone as fallback — same never-auto-replace rule as weapons, ticket 06).
5. Verify in PIE: per-zone silhouette separation spot-check + the 6 hero meshes + 4 boss meshes at gameplay camera distance. Record results in the Antigravity report-back (§20b reporting split).
6. Quaternius policy: if a direct-URL CC0 Ultimate pack becomes acquirable (batch 3+), its meshes are **reference/dressing only** under the Kenney supporting-content policy (ticket 05) — they do not define Echo identity.

## 10. Priority & sequencing

| Priority | Work | Estimated scope |
|---|---|---|
| **P0** | 8 bespoke Tier-A meshes: 4 bosses + Gloomfang + Lumewisp + Sprigling + Auroraling | **COMPLETE — delivered across V25-C1 (4 bosses) + V25-C2 (4 story): 14 bespoke echo meshes IMPORT_READY**; residual P0 work is engine-side import/binding (Antigravity §20b), not ArtSourceGen |
| **P1** | Archetype rig library: 8 plans × (rig + 3 clips + parameter manifest) + first variant bakes for the 17 zone signatures | **EXECUTED at PCR-4 — aw_archetypes.py + gen_tier_b.py delivered 39 unique variant-baked GLBs** (every zone signature + dungeon pools + monolith/colossus + Huge; convention-path opt-in binding; zero engine-side patch); import/binding rides the §20b baseline pass |
| **P1.5** | Tier-B variant bakes for the remaining ~38 rule members (dungeon pools, monolith line, wildlife rows) | **EXECUTED at PCR-4 — folded into the same bake (39 species cover dungeon pools + monolith line + wildlife rows)** |
| **P2** | Tier-C identity strengthening (element emissive/point-light audit, pattern/accent variation pass), species `Icon` field wiring + `CodexIndex` UI, Hit-clip runtime wiring | engine-side material/task work |
| **P3** | Evolution variant escalation rule application (6 species) if the tint/scale/glow rule alone reads weak in PIE | small |

Everything above is source-side ArtSourceGen/manifest work except where marked engine-side; nothing is committed into `Content/` by these batches, and no license other than CC0/self-generated is ever introduced (generated assets are project-authored, no third-party license needed).

## 11. Acceptance criteria

1. Census intact: 229 species, no test/census drift (validators green at every commit).
2. Tier coverage: 20 bespoke A + ~55 archetype B + ~154 procedural C = 229, membership computable by rule from source.
3. Visual-distinguishability contract (engine-verified at PIE): in any zone, at gameplay camera distance, signature species are separable by silhouette **and** element glow; heroes and bosses are unmistakable; no boss renders as a cone once its mesh import is confirmed.
4. Budget: ArtSource creature additions stay under the acquisition soft limits (well under 2GB/pack; current creature set ≈ 1MB total — bespoke meshes projected < 5MB).
5. Player-facing test: "that Echo is useful because it does X" is backed by gameplay identity (ability kits, work affinities — already live) **and** visual identity (this strategy). Neither alone is sufficient.

## 12. Status ledger

| Item | Status |
|---|---|
| This document | SOURCE-VERIFIED (v1.2) |
| 6 hero meshes + evolution variants | IMPORT_READY (existing) |
| 4 boss meshes (DrownedSovereign / GlassTyrant / Dawnfang / EyeSentinel) | **IMPORT_READY (V25-C1)**; engine binding ENGINE-UNVERIFIED |
| 4 story species meshes (Lumewisp / Sprigling / Gloomfang / Auroraling) | **IMPORT_READY (V25-C2 — this batch)**; engine binding ENGINE-UNVERIFIED |
| Tier-A bespoke set (8 meshes from §10 P0) | **COMPLETE — 10 hero/boss + 4 story = 14 bespoke echo meshes IMPORT_READY** |
| Tier-B archetype library (§6/§10 P1+P1.5) | **EXECUTED at PCR-4 — 39 unique variant-baked GLBs (4.8 MB, validate_glb PASS ×39) + definition-driven opt-in binding; PMC bodies stay until the engine import pass (per-species proportion/feature/palette variation from the bestiary rows + name-hash jitter — never a plain recolor)** |
| Archetype rig library (8 plans) | MISSING → P1 |
| Tier-C identity strengthening | PARTIAL (tints/ring/point-light live; pattern pass pending) |
| Engine import/binding of any creature mesh | ENGINE-UNVERIFIED (Antigravity §20b) |
