# CP-02 — ECHO: Body Plans, Evolution, Elemental Variants, Rarity & Animation

**Goal:** 226 species (214 bestiary + 6 production + 6 evolution targets) that read
individually at gameplay distance. The procedural PMC bodies (Batch 8: 8 body plans ×
5 size classes × 2 tints) define the silhouettes; this pack replaces geometry per
**body plan family** with reusable skeletal meshes — species differentiation comes from
scale, tint and element glow, not per-species meshes.

---

## 1. The 8 body plans (C++ `EAstrawildBodyPlan`)

| Plan | Mesh | Tris | Locomotion set | Signature reads |
|---|---|---|---|---|
| Quadruped | `SK_AW_Echo_Quadruped` | ≤ 12k | walk/trot/gallop + graze | Beast workhorse (Terraquill, Cindermule) |
| Biped | `SK_AW_Echo_Biped` | ≤ 14k | walk/run/idle-craft | Constructs, Voltpylon line |
| Serpent | `SK_AW_Echo_Serpent` | ≤ 10k | slither/swim-tunnel | Tidewyrm, dungeon bosses |
| Floating | `SK_AW_Echo_Floating` | ≤ 8k | hover-bob/drift/bob-dive | Spirits, Mistmender line |
| Insectoid | `SK_AW_Echo_Insectoid` | ≤ 16k | skitter/burrow/hover-sting | Bastionbeetle line, 6-limb rigs |
| Avian | `SK_AW_Echo_Avian` | ≤ 10k | perch/hop/flap/glide | Galewing/Stormwing families |
| Crystalline | `SK_AW_Echo_Crystalline` | ≤ 12k | levitate-spin/resonate | Astraite nodes, signal creatures |
| Amorphous | `SK_AW_Echo_Amorphous` | ≤ 12k | ooze/reform/disperse | Deepdelver line, elementals |

**Per-species variance (data-driven, zero extra meshes):**
- Size class scale × tints — already live (`PrimaryTint`/`SecondaryTint` vertex colors).
- Element glow light + emissive band color from `FAstrawildVfxPalette::GetElementTint` — live.
- Rarity ring (Rare+) at feet — live; becomes `NS_AW_Echo_RarityRing` (CP-05 §8) later.

## 2. Evolution / progression (SOURCE_TESTED this batch)

**C++ (NEW):** `UAstrawildEchoDefinition` evolution fields +
`UAstrawildEchoRosterSubsystem::CanEvolve/CanEvolveInstance/EvolveInstance`.

**Shipped chains (CODE_DEFAULT, live now):**

| Base | Evolved form | Level gate | Bond gate | Rarity |
|---|---|---|---|---|
| Terraquill (Flora gatherer) | **Terraquill Verdant** | 20 | 35 | Uncommon→Rare |
| Cindermule (Ember transport) | **Cindermule Pyre** | 22 | 40 | Uncommon→Rare |
| Voltpylon (Pulse battery) | **Voltpylon Tempest** | 25 | 45 | Rare→Epic |
| Bastionbeetle (Ash defender) | **Bastionbeetle Bulwark** | 28 | 50 | Rare→Epic |
| Mistmender (Light healer) | **Mistmender Rime** | 24 | 45 | Rare→Epic |
| Deepdelver (Ash miner) | **Deepdelver Abyssal** | 26 | 40 | Rare→Epic |

**Design law:** dual gate (level AND bond) — evolution is a relationship milestone.
Evolved form = +38% HP / +30% ATK / +32% DEF, +0.1 work affinities, size class +1,
deeper tints; **level/bond/trust/personality carry over** (identity survives).
**Evolution FX (CP-05 §4):** `NS_AW_Echo_Evolve` — capture-style ring collapse + element
burst + white flash; 1.6 s; sound A_EchoEvolve (CP-06 §4).

**UI obligation:** roster screen must show "READY TO EVOLVE" chip when `CanEvolve` returns
true and an Evolve button calling `EvolveInstance` (roster screen is CP-10 scope).

## 3. Elemental variants

Elements are C++ `EAstrawildElementType` — visual identity locked to the palette:

| Element | Glow hex | Body band | Status FX |
|---|---|---|---|
| Light | `#FDF9DD` pale gold | circuitry | `NS_AW_Elem_Light` sparkle drift |
| Ash | `#CEC8C4` grey drift | dust motes | `NS_AW_Elem_Ash` falling veil |
| Flora | `#A2F1AD` chlorophyll | leaf growth | `NS_AW_Elem_Flora` petal burst |
| Frost | `#C4F1FD` glacial | ice crystals | `NS_AW_Elem_Frost` shard pop |
| Pulse | `#A2F6EF` resonant teal | arcs | `NS_AW_Elem_Pulse` crackle |
| Ember | `#FFB87C` forge | heat veins | `NS_AW_Elem_Ember` ember trail |
| None | — | none | — |

Glow budget: captured Echoes glow always (2.4 intensity), wild within 32 m (1.9) —
the C++ cadence already enforces this; meshes expose an `EmissiveBand` scalar to receive it.

## 4. Rarity

`EAstrawildRarity` ring colors already live (Common→Mythic). Art pass upgrade:
- Rings → `NS_AW_Echo_RarityRing` (annulus sprite ribbon, 1 loop, rarity color).
- Legendary/Mythic add slow orbiting sparks (≤ 24 particles).
- Spawn-table weighting already data-driven; no art change.

## 5. Animation required (per body plan)

Per-plan anim sets (shared across all species of that plan; scale/tint sells the species):

| State | Length | Notes |
|---|---|---|
| Idle ×2 | 4–6 s loop | Plan-specific secondary motion (floating bobs, insectoid twitch) |
| Locomotion 3 speeds | walk/trot/run blended | Root-motion OFF — C++ AI drives speed (190/430 walk/run) |
| Combat: attack A/B | 0.6–1.2 s | Hit frame at 55%; notify `ANIM_OnHit` → existing ApplyElementalDamage pipeline |
| Defeat | 1.2 s + fade | Process captured → fade to capture beam (CP-05 §4) |
| Capture-react | 1.0 s | Struggle loop during capture window |
| Graze/sleep/roam idle | 3–8 s | Grazing herd beats (AI states already exist) |
| Evolve burst | 1.6 s | CP-02 §2 trigger |

**Rig rules:** all plans share bone naming `b_Root, b_Spine_01..03, b_Head, b_Limb_XX`;
≤ 42 bones/plan; no cloth sim (vertex-color secondary on floating/insectoid instead).
Root-motion must stay OFF — replication-safe movement is C++ owned.

## 6. Acceptance criteria

- [ ] 8 body-plan meshes in-engine, each under tri budget, shared materials.
- [ ] Any bestiary species spawns with plan mesh + species tint + element band.
- [ ] Evolving Voltpylon (level/bond gates met) rebuilds the actor as Voltpylon Tempest
      with level/bond/trust/personality intact — roster + world actor both update.
- [ ] Evolution FX + sound play once per evolution.
- [ ] 50 Echoes on screen ≤ 60 FPS (LOD: full mesh ≤ 30 m, simplified 12–40%, glow
      cadence already bounded by C++).
