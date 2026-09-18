# ASTRAWILD — Cinematic Identity & Character-Feel Contract

> **Master Directive v1 / roadmap P3 — the CINEMATIC BRANCH charter.**
> This document does two things the machine run cannot do without:
> 1. Reconciles the directive's cinematic identity ("fewer but better",
>    atmosphere-scale-light-wonder over systems breadth) with the 229-species
>    systems-wide codebase that actually exists in Source/.
> 2. Pins the **character-feel targets** the first PIE session judges
>    against — every number traced to `Design/design_data.json`
>    (file:line), so "does it feel right" becomes "did it hit the number".
> Generated 2026-09-18 at HEAD `6157a1e`. Nothing here claims engine
> verification: feel targets become REAL the moment PIE runs on the machine.

---

## 1. The reconciliation, stated honestly

The Master Directive v1 declares the ASTRAWILD Unreal Division a
**cinematic branch**: atmosphere, scale, light and wonder over system
breadth; fewer but more refined creatures; slower, heavier rhythm; when in
doubt, choose the more visually stunning option.

The repository this branch inherited is the opposite shape on paper:
**204 generated species rows** (`Design/design_data.json` → bestiary
aggregates: 10 families, 12 zones, 6 elements), 229 roster identities with
art conventions, and 15+ gameplay subsystems — a systems-wide survival
framework authored before the cinematic charter existed.

The directive itself anticipated this collision: the divergence is
declared intentional and approved, and rule R1 (no invented values, no
rewrites of what exists) governs extraction. Therefore:

1. **Nothing is pruned.** The 204-species table stays; it is compiled,
   tested, and art-conventioned. Pruning would be an invention of a
   smaller game that no one built.
2. **The cinematic identity is expressed as a CAMERA, not a scalpel.**
   The machine prototype's job is to make one zone, one light state, one
   dawn read as a film frame — using the existing world bootstrapper's
   atmosphere layer — not to shrink the menagerie.
3. **"Fewer but better" applies to what the first day foregrounds**: the
   prototype map showcases a handful of species silhouettes with distinct
   procedural bodies; the depth of the roster is background systems
   realism, not the front of frame.
4. **Where a future choice is free** (a new feature, a new scene, a
   trade-off), the cinematic option wins by charter. Where the code is
   already written, R1 wins: existing values are facts, not suggestions.

---

## 2. Character-feel targets (traced)

These are the numbers a PIE session can verify in minutes. Every row cites
`Design/design_data.json`, which itself traces to file:line in Source/.

### 2.1 Movement — "explorer's stride, not arena dash"
| Target | Value | Trace |
|---|---|---|
| Walk speed | **450 uu/s** | `AstrawildPlayerCharacter.h:327` |
| Sprint speed | **700 uu/s** (1.56× walk) | `AstrawildPlayerCharacter.h:330` |
| Sprint budget | 100 stamina / **7.0 per s** ≈ **14 s** of sprint | `AstrawildSurvivalComponent.h:59,68` |
| Stamina regen | **14.0 per s** (~7 s full recovery, only when not draining) | `AstrawildSurvivalComponent.h:59` |
| Feel verdict rule | walk cadence reads "trek"; sprint reads "urgent but mortal" — if sprint feels FREE (never exhausts in a dash across the proto map), FAIL | traced above |

### 2.2 Combat — "deliberate swings, readable windows"
| Target | Value | Trace |
|---|---|---|
| Light attack | 25 dmg / **0.45 s** cooldown | `AstrawildCombatComponent.h:44,47` |
| Heavy attack | 60 dmg / **1.3 s** cooldown | `AstrawildCombatComponent.h:50,53` |
| Attack range | **320 uu** | `AstrawildCombatComponent.h:56` |
| Dodge | **0.9 s** cooldown, 22 stamina, **0.4 s i-frames**, 900 impulse | `AstrawildCombatComponent.h:77-87` |
| Block | unarmed mitigation **0.45**, move at **0.45×** while blocking | `AstrawildCombatComponent.h:92,95` |
| Stagger | hits ≥ **35** post-mitigation stagger for **0.6 s** | `AstrawildCombatComponent.h:113,117` |
| Feel verdict rule | light→light chains at ~2.2 swings/s; heavy must FEEL like a commitment (1.3 s); dodge i-frames must visibly carry through a boss telegraph disc | traced above |

### 2.3 Survival pressure — "20-minute horizon, never menu-spam"
| Target | Value | Trace |
|---|---|---|
| Hunger decay | 0.083/s → full→empty ≈ **20 min** | `AstrawildSurvivalComponent.h:49` |
| Thirst decay | 0.0833/s → ≈ **20 min** | `AstrawildSurvivalComponent.h:56` |
| Base max health | **100** | `AstrawildSurvivalComponent.h:182` |
| Cold / heat bands | below **4 °C** / above **36 °C** ambient | `AstrawildSurvivalComponent.h:75,78` |
| Feel verdict rule | during a 10-minute PIE, vitals bars must move noticeably but never force a menu trip | traced above |

### 2.4 World rhythm — "a day is a session beat"
| Target | Value | Trace |
|---|---|---|
| Time scale | **1.0 in-world min / real second** → 24-min full day | `AstrawildTimeSubsystem.h:33` |
| Prototype wildlife | 8 wild Echoes + **2 hostiles** on the proto map | `AstrawildWorldBootstrapper.h:92,95` |
| Resource nodes | **21** on the proto map | `AstrawildWorldBootstrapper.h:89` |
| Feel verdict rule | a full day/night cycle must be OBSERVABLE in one sitting; hostiles must present as weather-like events, not constant pressure | traced above |

### 2.5 The cinematic layer (atmosphere-first)
| Target | Value | Trace |
|---|---|---|
| Exponential fog density | **0.00012** | `AstrawildWorldBootstrapper.h:27` (atmosphere sample) |
| Sky light intensity | **1.4** | `AstrawildWorldBootstrapper.h:28` |
| Atmosphere enable flag | true (bootstrapper builds sun/sky/fog/PPV) | `AstrawildWorldBootstrapper.h:85` |
| Prototype map light rig | DirectionalLight + SkyLight (real-time capture) + SkyAtmosphere + ExpHeightFog | `Tools/Python/build_prototype_map.py` (L_Proto_01 spec) |
| Feel verdict rule | at DAWN and DUSK in-world times, the proto map must produce at least one composition you would screenshot — the charter's minimum bar for "wonder" | traced above |

### 2.6 Ending sequence — the charter's proof scene
| Target | Value | Trace |
|---|---|---|
| Total runtime | **18.7 s**, skippable | `AstrawildEndingCinematicComponent.h:85` |
| Shot plan | bars 1.2 → low hero 1.2 → title 3.2 → high orbit 6.2 → subtitle 7.7 → return-behind 11.7 → hint 14.7 → fade 17.2 | `AstrawildEndingCinematicComponent.h:77-85` |
| Shot blend | **1.4 s** cross-blends | `AstrawildEndingCinematicComponent.h:86` |
| Feel verdict rule | the 6.2 s high wide orbit over the Shattered Vale IS the identity statement — if it doesn't land emotionally, the cinematic branch isn't done, regardless of systems state | traced above |

---

## 3. How the machine run grades feel (ON_PC_TASKS hour 4-6 hooks)

Each verdict rule above becomes a checklist row in
`Docs/ASTRAWILD_ON_PC_TASKS.md` with three outcomes:
- **HIT** — number observed, feel confirmed;
- **MISS (number)** — traced value didn't manifest (a bug: log it, fix-forward);
- **MISS (feel)** — number hit but the sensation is wrong (a DESIGN
  decision: change the value IN SOURCE, re-run
  `Scripts/extract_design_data.py`, and this document updates with it —
  the trace chain is the anti-drift mechanism).

Feel disputes never bypass the trace: a new number without a source change
is a hallucination by charter.

---

## 4. Aspiration vs implementation (honest ledger)

| Charter aspiration | Implementation state | Verdict |
|---|---|---|
| Lumen / Nanite volumetric spectacle | Engine features available at 5.8 defaults; project does NOT yet hard-configure Lumen panels | ASPIRATION — machine run should screenshot defaults, then decide |
| Fewer but better creatures | 204 species exist; prototype foregrounds a handful | RECONCILED (§1) |
| Slower, heavier rhythm | 0.45/1.3 s attack cadence, 14 s sprint budget, 20-min vitals | IMPLEMENTED (traced §2) |
| Atmosphere-scale-light first | Full bootstrapper rig + proto map light rig + ending sequence | IMPLEMENTED, ENGINE-UNVERIFIED |
| "Screenshot minimum bar" | Dawn/dusk compositions on the proto map | MACHINE-JUDGED (hour 4-6) |
