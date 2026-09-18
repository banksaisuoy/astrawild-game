# ASTRAWILD — PLAYER RULES

> **The concise authoritative player-facing rulebook.** Every number below is
> derived from the live source (file:line references in the repo at freeze).
> This is the ONE document a new player (or the final UE5 playtester) reads to
> know how the game actually plays. Verified rules only — no aspirational
> features. Source state: `final-completion` @ freeze SHA (see
> `ASTRAWILD_FINAL_READINESS_REPORT.md`).

---

## START

**Who you are:** a frontier survivor on the Shattered Vale — a crashed
resonance world of twelve zones, wild creatures ("Echoes"), and the storm cage
called the Maelstrom.

**Starting equipment** (granted on first spawn):
- Wood ×20, Stone ×20
- Resonator ×3 (the capture tool — every capture attempt consumes one)
- Dawn Shards ×10 (vendor currency)

**Starting capabilities:**
- 100 HP · 100 Stamina · 100 Hunger · 100 Thirst · felt temperature 20°C
- Five attributes at level 1: **Might / Vigor / Agility / Instinct / Craft**
  (cap 10; XP needed per level = 100 × current level)
- Skill loadout: 3 slots, all empty at start — skills unlock automatically at
  attribute milestones (see COMBAT → PLAYER SKILLS)
- Walk 450 cm/s, sprint 700 (needs stamina > 5%); interact reach 300 cm

**First objective (MQ-01 "First Light"):** speak to **Warden Maren** in
Dawnstead (follow the quest tracker [HUD]) and gather 10 Dawnwood + 5
Fieldstone. Reward: 2 Resonators + 5 Research Points.

---

## SURVIVAL

**Hunger & thirst** drain to empty in ~20 real minutes each. Each EMPTY meter
drains 1.5 HP/s (both empty: 3 HP/s). There is no passive regen — eat, drink,
or use healing items. Rest points and respawn fully restore you.

**Temperature:** ambient is 20°C + weather + zone hazard. Below 4°C you take
cold damage; at/above 36°C heat damage (1 HP/s). Armor insulation shifts the
thresholds separately for cold and heat (the **Bastion armor set** covers
both sides). Weather swings hard: Heat wave +20°C, Cold snap −17°C, Storm −9°C.

**Status effects (applied by elemental hits):**
| Element | Effect |
|---|---|
| Ember | Burning: DoT (2 + 5% of the hit) for 4s |
| Frost | Chilled: ×0.5 speed, 3s |
| Flora | Poisoned: 2 DPS for 6s |
| Pulse | Shocked: ×0.3 speed, 0.8s |
| Light / Ash / None | no status |

Speed penalties stack multiplicatively (Chill+Shock = ×0.15). Stagger: any
post-mitigation hit of 35+ zeroes your speed for 0.6s.

**Death & respawn:** input locks, auto-dismount, respawn at the camp after
**5.0s** with full vitals and cleared statuses. **No item or currency loss.**
Falling out of the world is lethal (9999 damage).

**Consumables (craftable):** Field Ration (food 25, heal 10, +4 stamina/s for
90s), Pulse Tonic (water 12, heal 15, capture focus 30s).

---

## COMBAT

**Melee:** light 25 dmg (0.45s), heavy 60 dmg (1.3s, 25 stamina), reach 320cm.
**Ranged:** 8 weapon families, e.g. Scrapshot 14 dmg/0.5s, Pulse Lance 22,
Arc Caster 26 (chains 3), Magrail 85 (pierce 5), Starlance 140 (pierce 6).
Ammo per family (Stone ammo, Energy Cells, …). Dodge Q: 22 stamina, **0.4s
invulnerability**, 0.9s cooldown. Block RMB: 45% (unarmed) to shield-value
mitigation, ×0.45 speed while holding.

**Damage math:** (base + weapon ATK) × Might bonus (×1.04 per Might level) ×
durability (broken weapon ×0.4). Armor reduces by Rating/(Rating+100), cap 60%.

**Elements:** None/Light/Ash/Flora/Frost/Pulse/Ember. Hit a creature's
**weakness element** → **×1.5 damage + a "WEAKNESS HIT" toast**. Hit it with
its OWN element → ×0.80. Everything else ×1.0.
The counter cycle (weak to): **Flora ← Ember ← Frost ← Pulse ← Light**; Light
and Ash have no canonical predator. Discover each species' weakness by
observing it (Field Journal) — or by testing it in combat.

**Weak points (Large/Huge wild creatures):** open every 20s for 4s (element
light pulses). Direct hits during the window deal **×1.5** (stacks with
weakness ×1.5).

**Capture:** weaken the Echo first. Each attempt consumes 1 Resonator.
Chance rises with: missing target HP, low target Resilience, your **Instinct**
(+1.5%/level), journal observation (up to +15%), tracking proximity (+5%),
feeding (trust), weather match, Hunter's Focus skill (+25%). Instinct XP:
+25 success / +4 failure.

**PLAYER SKILLS (Y = smart-cast one key; loadout in pause menu):**
| Skill | Unlocks | Effect | Cooldown |
|---|---|---|---|
| Power Strike | Might 3 | next melee hit ×2.2 (6s) | 8s |
| Whirlwind | Might 6 | sweep all hostiles ≤350cm, heavy dmg + stagger | 14s |
| Dash | Agility 3 | burst-dash on movement input | 6s |
| Second Wind | Vigor 4 | heal 40 HP + 60 stamina | 30s |
| Hunter's Focus | Instinct 4 | +25% capture chance 12s | 25s |
| Overcharge | Instinct 7 | +30% ranged damage 10s | 18s |
| Masterwork | Craft 5 | PASSIVE: 15% chance crafting refunds ALL inputs | — |

Bind up to 3 (a non-empty loadout narrows the smart-cast to your build);
leave all slots empty for the full auto-ladder. **T** orders every party Echo
to cast its best ability.

**XP sources (per attribute):** Might — landing melee hits; Vigor — surviving
big hits; Agility — dodge rolls; Instinct — captures (and failures);
Craft — completing crafts.

---

## ECHO (creatures)

**Capture requirements:** alive + uncaptured + 1 Resonator per attempt; weaken
for better odds. Max roster **100** captured Echoes; max **3** in the active
party ring (pick them on the Echo Roster screen [L]; the rest stay benched).

**Bond & trust:** feeding (preferred food = double) raises trust, mood, and
bond (+travel time: +0.2/in-world hour). **Bond 25** unlocks riding (rideable
species only). **Bond 40 + level 25** unlocks **evolution** (6 authored chains,
e.g. Voltpylon → Voltpylon Tempest; level/bond/personality preserved).

**Abilities:** every species has a 6–7 ability kit (element/role/family
signature + locomotion signatures for water/flying species). Echoes learn
abilities by LEVEL (the journal/roster shows each ability's unlock level).
Casting is free, cooldown-gated; your party casts on **T**.

**Party passive auras (while in the ring):** Mending Aura (party heal), Rhythm
Aura (player stamina regen), Pack Instinct (+20kg carry), Calm Presence (less
wild aggro). **Element resonance:** party pairs of matching elements reduce
damage taken.

**Work & automation:** press **E at a work site** to collect output, stage
input cycles, or assign the nearest idle captured Echo. Garrison cap by Base
Terminal level: 5 / 10 / 20. Work rate scales with the species' work affinity,
mood, energy, sanity, and grid power (powered sites ×1.5). Utility robots can
man sites alone.

**Breeding & genetics:** Breeding Pen (2 party Echoes + 1 Breeding Cake) →
Egg; Egg Incubator (warm = level-2 hatch) → offspring of the highest-bond
parent's species. Traits: 70% inherited / 30% mutated; IVs 0–31 (+1% per point
to HP/ATK/DEF/speed). Traits include Swift +30% speed, Artisan +50% work,
Ferocious +20% ATK, Sturdy +20% HP, Lucky +10% capture.

**Locomotion:** rideable species = Medium+ Beast/Dragon/Avian/Insectoid
quadrupeds or avians, plus Aquatic sea-riders. Mount = ×1.25 speed (min 200),
Bond 25. Flying mounts: SPACE climb / CTRL descend. Water mounts swim in the
three sea zones (×1.4 speed there, ×0.85 ashore).

---

## CRAFTING

- **Recipe unlock:** research-gated recipes need their tech unlocked (free
  recipes always visible). **Station rule:** recipes with a station (Workbench
  / Campfire) require standing within 500cm of it.
- Open the crafting screen with **E at a station** (or Tab → crafting). One
  craft at a time; instant recipes complete immediately; timed crafts can be
  **cancelled for a full ingredient refund**.
- Timed craft speed improves with **Craft** level (×1.04/level, floor ×0.25).
- If the pack is too full for outputs, the craft holds them and retries —
  nothing is silently lost.
- Craft XP: +8 Craft per completion (Masterwork can refund all ingredients).

**Resource nodes (10):** Dawnwood Stand (Wood), Fieldstone (Stone), Sunfiber
(Fiber), Dawn Crystal (Crystal Shard), Ember Ash Vent (Ember Ash), Sea Pearl
Bed, Coral Shard Reef, Dune Glass Seam (Rare), Storm Silver Vein (Rare),
Hidden Alloy Vein (Epic — scanner-only). Nodes respawn (30s–480s by rarity).

---

## BUILDING

- Build mode **B**: ghost preview (green = valid, red = invalid), 600cm reach,
  200cm grid snap, wheel cycles pieces, N rotates, LMB places, Z on a built
  piece **dismantles for a 100% refund** (if the pack can carry it).
- Costs are per building definition (Foundation Wood×4 … Base Terminal Crystal
  Shard×4 … Defense Turret Stone×6).
- **Power:** one auto-connected grid (1200cm links), resolved every 2s.
  Echo Dynamo generates 8; consumers draw 1–3 each; Charge Cell stores 600.
  On **brownout**, lower-priority consumers go dark (priority: Research >
  Workstation > Farm > Defense > Decoration) and a toast warns you.
- Buildings don't decay except weathering outside Base Terminal territory.
- Equipment durability: weapons −1/hit, tools −1/harvest, armor −1/hit taken;
  broken weapon = ×0.4 damage; repair with a Field Repair Kit anywhere, or at
  a Repair Bench for 40% of the craft inputs.

---

## RESEARCH

- **RP sources:** quest rewards, first-discovery POIs, world events, journal
  observation milestones (each +RP), dungeon completions (+10, the Eye +30),
  certain dialogue choices.
- 17 techs in a prerequisite DAG; root techs are free. Unlock to expose
  recipes/buildings/tools. **Ancient Resonance** only unlocks by completing
  the Hollow Underlight dungeon.
- Every unlock toasts on every player's screen (co-op included).

---

## WORLD

**Twelve zones** (4×3 grid, 800×800m each), threat 1–4:
- North: Frostveil Expanse (3, cold −12°C) · Glimmerwood (2) · Ember Ridge (3,
  heat +10°C) · Sunscar Desert (3, heat +12°C)
- Middle: Dusk Marsh (2, −4°C) · **Dawn Fields (1 — the safe starter zone)** ·
  Hollow Approach (4, Ash Lung −6 stamina-regen/s) · Azure Shallows (2, sea)
- South: Tidebreaker Isles (3, −6°C, sea) · Stormcrest Highlands (3, −9°C) ·
  Verdant Reach (2, +4°C) · Pearlsea Reef (4, +5°C, sea)

**Traversal:** the **Dawn Skiff** (board at Dawnstead / Driftwood Landing,
E within 420cm) flies at 1400 (boost 2600) cm/s to 120m ceiling — 160m with
the Stratos Coil, which you need to reach the Eye Gate platform at 150m.
Water mounts swim the seas; no zone strictly requires one.

**POIs (17):** 12 open-world beacons/ruins + 5 scanner-only signal sources
(Scanners also double discovery radius). First discovery = RP + toast.

**Dungeons (3):** Hollow Underlight (Warden) · Sunken Vault (Vault
Colossus/"Dawnfang") · Eye of the Maelstrom (the Drowned Sovereign). Enter by
[E] at the portal; gates open room by room as each room clears; room types
include puzzle rooms (resonance pillars) and hazard rooms.

---

## QUEST

**Main chain MQ-01 → MQ-17 (linear, one active):** First Light → A Friend in
the Fields (capture a Lumewisp) → Homeground (build) → The Spark (power) →
Dawn Guard (cull Gloomfangs) → Shepherd's Dawn (husbandry) → The Hollow
Underlight (first boss) → The Vale Beyond (Ember Ridge + Pulse Lance) → Wings
over the Vale (skiff) → The Sunken Vault (second boss) → Signals in the
Static (scanner) → The Vanguard Protocol (defense) → The Storm Crown Stirs →
The Crown Relay (Glass Tyrant + Stratos Coil) → The Eye of the Maelstrom →
The Drowned Sovereign (final boss) → **First Dawn Again** (homecoming, +50 RP).

Rewards: items + Research Points, auto-chained. The HUD tracker always shows
the active objective's progress.

---

## BOSS

Four bosses, one shared encounter grammar (3 phases, enrage, adds, telegraphed
blasts, hazards, weak point) with per-boss tuning:

| Boss | Where | Identity |
|---|---|---|
| **Underlight Warden** | Hollow Underlight | default set — the tutorial boss |
| **Vault Colossus (Dawnfang)** | Sunken Vault | tidal pressure: faster cadence, paired bolts, Wavecrest adds |
| **Glass Tyrant** | Stormcrest (open world) | shard volleys: triple bolts, twin blasts, sharp hazards; **Light is its bane** |
| **The Drowned Sovereign** | Eye of the Maelstrom | the finale: fastest cadence, triple blasts, triple hazards, Eye Sentinel adds; **Dawn Light is its bane** |

**Encounter rules:** Phase 1 (100–66%) measured pressure → Phase 2 (66–33%)
summons adds + faster → Phase 3 (≤33% **or** the 180s enrage timer) ×1.4
damage. Watch the ground: the amber/element disc = a blast detonating in
~1.5s — sprint out. Lingering pools = standing damage. The **weak-point core**
opens every 12s for 5s (**×2 damage**) and pulses when live; the boss bar
shows its phase + ENRAGED + weak-point state. Bosses shed status effects at
half duration. Defeat: bar clears, "DEFEATED — the way opens" toast, loot to
the killer, gates open (dungeon rooms may also drop their own clear loot).

---

## ENDGAME

Chain: **MQ-15 The Eye of the Maelstrom** (reach the 150m Eye Gate — needs the
Stratos Coil) → **MQ-16 The Drowned Sovereign** (the final fight; use the
Light counter) → **MQ-17 First Dawn Again** (return to Maren).

**Endings (choice, one-way, saved):**
- **Ending A — "The Dawn That Stays":** tell Maren *"Break the cage — end the
  storms forever."* The weather pins to Clear permanently.
- **Ending B — "The Storm That Sleeps":** *"Let it sleep — the crown keeps its
  vigil."* The living sky continues.

---

## POST-GAME (after either ending — free roam)

- **Hunts [U]:** 8 repeatable cull contracts (Duskmoth ×5, Stonehide ×4,
  Emberfang ×3, Rimefang ×3, Brinefin ×5, Sunhide ×5, Verdantbloom ×4,
  Vesper Monolith ×1) with item rewards — the repeatable economy loop.
- **World events (16):** Storm Surge, Great Migration, Resource Surge, Supply
  Drop, Ancient Signal, Night Raid, Meteor Fall, Rare Echo Bloom, Boss
  Stirring, Mist Tide, Cinder Fall, Dune-Buried Cache, Reef Bloom, Wreck
  Surge, Storm Front, Pearlsong — toasts + HUD banner + RP on resolution.
- Dungeons re-runnable; the ending state persists (save v5) and the banner
  stays as your verdict.
- Economy: hunt/loot → Dawn Shards → vendors (Trader Tam, Herbalist Wren,
  Blacksmith Borin, Fisher Nima) → craft → upgrade.

---

## CONTROLS (default keyboard — gamepad mapping coexists)

WASD move · mouse look · Shift sprint · SPACE jump/ascend · **E interact**
(stations, nodes, NPCs, ride, evolve, dismount) · LMB light attack · F heavy ·
Q dodge · RMB block · **C** party command · **R** feed · **B** build mode ·
N rotate build · wheel cycle build · **G** consume · X equip best · Z dismantle ·
F5 quicksave · F9 quickload · **V** (hold) scan · H deploy drone · J deploy
robot · Tab inventory · K research · **P** Field Journal · **L** Echo Roster ·
**M** World Map · **U** Hunt Board · **T** party abilities · **Y** player
smart-cast · Esc pause · CTRL descend (skiff/flying mount)

*Gamepad note: full navigation/combat is mapped; the Y-key player smart-cast
is keyboard-first (bind your loadout in the pause menu — the radial gamepad
cast is a known engine-pass backlog item).*

---

## PERSISTENCE

Save v5: world state, co-op per-player blocks, roster (owner-partitioned),
quests, research, dialogue flags, hunts, ending state, weather pin. F5/F9
quick save-load; hosts save the shared world in LAN co-op.

*Everything above is STATIC-VERIFIED source behavior — runtime confirmation
happens in the one-time final UE5 build (see
`ASTRAWILD_FINAL_BUILD_HANDOFF.md` §20).*
