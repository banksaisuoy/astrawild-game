# P3 — Alpha Echo: Solarix Encounter

## Encounter identity

`Echo.SolarixAlpha` is a Tier-2 Solar Alpha Echo that lives in the South-East Danger Pit. It is an original ASTRAWILD creature encounter and must not use a recognizable franchise design. The encounter tests reading telegraphs, elemental status management, capture preparation, and cooperative positioning.

## Arena contract

Place the boss spawn at `Location.DangerPit.AlphaSpawn`, with a 26 m leash radius around its home marker. Keep three readable approach lanes, two line-of-sight rocks, one safe reset edge, and enough open floor for telegraphs. Do not hide the attack area under dense foliage or VFX.

## Two-phase design

| Phase | Health | Behavior | Required patterns |
|---|---:|---|---|
| Phase One | 100–50% | patrol, short Solar melee, teaches timing | `SolarClaw`, `EmberLine`, `DawnRoar` |
| Phase Two | below 50% | faster cadence, area denial, summons pressure | `SolarNova`, `FlareRing`, `AshenRush`, `DawnRoar` |

`AAstrawildAlphaEcho` transitions once at `PhaseTwoHealthThreshold`, resets attack cooldowns, broadcasts `OnPhaseChanged`, and exposes `ExecuteAttackPattern` for a Behavior Tree, State Tree, or Blueprint timer. C++ remains responsible for cooldown gating and target ability execution; Blueprint owns telegraph presentation and audio/VFX assignment.

## Pattern tuning

| Pattern | Telegraph | Damage intent | Counterplay |
|---|---:|---|---|
| `SolarClaw` | 0.45 s | close cone, 1.0x | dodge backward or flank |
| `EmberLine` | 0.80 s | narrow ground line, Ignited | move across the lane |
| `DawnRoar` | 1.10 s | radial stagger, low damage | leave radius or dodge |
| `SolarNova` | 1.25 s | large radial burst, 1.5x | use cover or perfect dodge |
| `FlareRing` | 0.90 s | expanding ring, 1.2x | jump/dodge through safe gap |
| `AshenRush` | 0.65 s | targeted charge, 1.35x | bait into line-of-sight rock |

## Blueprint/content requirements

Create `BP_Echo_SolarixAlpha` derived from `AAstrawildAlphaEcho`, `DA_Echo_SolarixAlpha` with `Echo.SolarixAlpha`, and an attack pattern array matching this document. Bind `OnAttackTelegraph` to Niagara telegraphs and `OnPhaseChanged` to an arena lighting/audio cue. Add a short intro state, phase transition invulnerability only during the transition montage, and a defeat reward event that writes a quest progression tag.

## Multiplayer readiness

The server owns phase, cooldown, damage, and reward results. Clients may predict only telegraph presentation. Do not grant rewards or advance the quest from a client-only event.
