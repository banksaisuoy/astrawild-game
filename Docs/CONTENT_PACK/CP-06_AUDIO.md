# CP-06 — AUDIO: Weapons, Creatures, Ambience, UI, Footsteps & Environment

**Goal:** a mix that makes the world believable at idle and combat legible at volume.
All weapon fire/impact cues bind via the NEW `FireSound`/`ImpactSound` fields on weapon
profiles; biome ambience binds via `UAstrawildBiomeDefinition::AmbientAudio` (live).
Sound classes/mixing ride the engine's audio mixer; no middleware.

---

## 1. Weapon sounds (details in CP-03 §sound)

5 families × (fire + impact) + 3 post-launch families. Random pitch ±8%,
peak-normalized ≤ −6 dBTP, fire −14 LUFS ±1 / impact −20 LUFS.

## 2. Creature sounds — by body plan (not per species; scale/pitch differentiates)

| Body plan | Vocal set | Voice count | Pitch rule |
|---|---|---|---|
| Quadruped | grunt/chuff/alarm/defeat | 4 | small +25%, large −20% |
| Biped | servo whirr + chirp | 4 | construct texture |
| Serpent | hiss + sub growl | 3 | size −15% |
| Floating | chime breath | 3 | element-tinted resonance |
| Insectoid | chitter ×2 speeds + swarm hum | 4 | colony layering |
| Avian | call ×3 (near/perch/alarm) | 3 | dawn chorus ducked in combat |
| Crystalline | resonant ping ×2 | 2 | resonates with element palette |
| Amorphous | bubbling slosh | 3 | — |

Sets: `A_Echo_<Plan>_<Voice>`; behaviors: idle ambient (rare, ≤ 6 active voices),
aggro alarm, defeat cry (routes the existing AI state broadcasts).

## 3. Ambience — per-zone beds (bind `AmbientAudio` per biome)

| Zone | Bed | One-shot accents |
|---|---|---|
| Dawn Fields | open meadow air, distant birds | skylark call, leaf gust |
| Dusk Marsh | low drone, reed hiss, frog clusters | splash plop, reed bang |
| Ember Ridge | deep rumble bed | obsidian crack, distant flare |
| Frostveil | wind whistle wide | ice settle crack |
| Glimmerwood | hushed shimmer bed | crystal ping (matches Voltine) |
| Hollow Approach | near-silence + sub dread | single rock fall |
| Azure Shallows | waves lap + gull | skiff engine far |
| Tidebreaker | heavier surf | rigging creak |
| Sunscar | dry wind + heat buzz | sand slide |
| Stormcrest | wind gust layers | thunder distant |
| Verdant Reach | dense insect+bird wall | canopy monkey-call |
| Pearlsea Reef | underwater body (low-pass) | coral crackle, whale far |

Beds loop seamlessly ≥ 90 s; day/night crossfade via the time subsystem's phase
(night = −6 dB + swap to night accents — C++ time already broadcast).

## 4. Event stingers

- `A_EchoEvolve` — CP-02 evolution: rising shimmer + resolve chime (−12 LUFS).
- `A_Capture_Success/Fail` — CP-05 §2 sequencing.
- `A_Quest_Start/Complete` — soft two-note motif (UI-adjacent, −20 LUFS).
- `A_Boss_Stirring` — sub boom + metallic groan (world-event banner sync).

## 5. UI

| Sound | Character |
|---|---|
| `A_UI_Hover` | soft tick −26 LUFS |
| `A_UI_Click/Confirm` | positive chirp |
| `A_UI_Deny` | low buzz |
| `A_UI_ScanPing` | per scanner tier (teal/amber/violet pitch) |
| `A_UI_EvolveReady` | sparkle triad (roster chip) |
| Dialogue advance | page brush (subtle, −24 LUFS) — NEW dialogue screen |

## 6. Footsteps

`SC_AW_Player_Footsteps`: 4 surface switches (grass / stone / sand / shallow water),
walk/run cadence (C++ movement speed drives rate), sprint adds landing weight.
Surface detection via physical materials (CP-04 terrain layers). Skiff deck = metal thud.

## 7. Environment

- Weather: storm bed (wind + rain layers) ducking ambience −8 dB (weather state
  already C++ broadcast — subscribe).
- Water planes: shore foam loop near beach Z.
- Base machinery: generator hum loop (power grid live), work-site rhythmic clanks.
- Night raid horn (world event) + village guard response shouts.

## Mix & acceptance

**Mix targets:** ambience bed −30 LUFS, accents −24, creature idle −22, combat fire
−14, UI −26..−20; music (post-launch) ducks −6 under combat.
- [ ] All cues peak-normalized, naming per convention, metadata tags set.
- [ ] Zone change crossfades ≤ 2 s — no hard cuts audible at zone borders.
- [ ] Storm Surge: ambience ducks, rain+wind audible, weapon fire stays legible.
- [ ] Footstep surface switching verified on grass/stone/sand/water in Dawn Fields→Sunscar.
- [ ] No weapon binding → silence (fallback contract, matches C++ dispatch).
