# CP-03 — WEAPONS: 5 Launch Families, Muzzle/Impact/Projectile FX & Sound

**Goal:** every trigger pull reads instantly by family — shape, flash, sound, impact.
The C++ foundation is complete: 8 weapon profiles, 4 firing archetypes
(Projectile/Homing/Beam/ArcChain), Niagara-first dispatch with procedural fallback
(**NEW this batch**: `MuzzleFlashVfx`, `ImpactVfx`, `ProjectileTrailVfx`, `FireSound`,
`ImpactSound` soft refs on `UAstrawildWeaponDefinition`, consumed by the combat component
and projectile actor — see CP-00 §bindings).

**Launch families (5 of 8; the other 3 land post-launch):**

| # | Family | C++ profile | Fire mode | Identity hex | Held-mesh |
|---|---|---|---|---|---|
| 1 | Kinetic | `Weapon_Scrapshot` | Projectile | `#D9D6CF` gunmetal | `SK_AW_Weap_Scrapshot` |
| 2 | Plasma | `Weapon_PlasmaCharger` | Projectile | `#F7A9D6` magenta | `SK_AW_Weap_PlasmaCharger` |
| 3 | Laser (Pulse) | `Weapon_LumenBeam` | Beam (pierce) | `#FF8C8C` red | `SK_AW_Weap_LumenBeam` |
| 4 | Arc | `Weapon_ArcCaster` | ArcChain | `#BEE5FF` electric | `SK_AW_Weap_ArcCaster` |
| 5 | Rail | `Weapon_MagrailDriver` | Beam (velocity) | `#FFE4CC` charged white | `SK_AW_Weap_MagrailDriver` |

Post-launch: Missile `Weapon_SkysingerLauncher` (homing+lock-on), Experimental
`Weapon_StarlancePrototype`, Kinetic sidearms. C++ already runs all 8.

**Mesh budgets:** first-person+world shared skeletal mesh ≤ 6k tris, 1024 texture set,
`b_Muzzle` socket at barrel tip (all muzzle FX/sounds anchor there). Held scale already
tints/scales by family/tier in C++ — meshes replace, not re-implement.

---

## Muzzle FX specification

| Family | System | Look | Budget |
|---|---|---|---|
| Kinetic | `NS_AW_Weap_Muzzle_Kinetic` | 2-frame smoke puff + debris sparks, warm white core | ≤ 8 sprites, 0.15 s |
| Plasma | `NS_AW_Weap_Muzzle_Plasma` | Magenta bloom ring + inward suck-back (charge feel) | ≤ 16, 0.25 s |
| Laser | `NS_AW_Weap_Muzzle_Laser` | Thin red slit + lens flare streak along beam axis | ≤ 6, 0.12 s |
| Arc | `NS_AW_Weap_Muzzle_Arc` | Electric fork burst, 3 jagged spokes | ≤ 12, 0.2 s |
| Rail | `NS_AW_Weap_Muzzle_Rail` | Charged white disc + rail-sheen streak | ≤ 10, 0.3 s |

Rule: muzzle lights (12k lumens in the procedural fallback) drop to ≤ 2k lumens with
real systems; every system must finish ≤ 0.35 s so burst fire never stacks lights.

## Impact FX

| Surface | System | Look |
|---|---|---|
| Creature (any family) | `NS_AW_Weap_Impact_Creature` | Element-tinted burst + 1 hit-flash sprite |
| Rock/ground | `NS_AW_Weap_Impact_Solid` | Dust billow + family-colored sparks |
| Water | `NS_AW_Weap_Impact_Water` | Splash column + ring |

Binding: single `ImpactVfx` per profile today (creature-dominant); surface-switching
via physical materials is a CP-05 §9 stretch. Impact points: projectile OnHit, beam
terminal, arc first contact (all already spawn sites in C++).

## Projectile FX

| Family | Trail | Core |
|---|---|---|
| Kinetic | tracer streak ≤ 60 cm | no core (bullet) |
| Plasma | magenta ribbon, 0.4 s decay + heat shimmer | glowing capsule |
| Laser | n/a — beam is hitscan | beam prism (already live, tint red) |
| Arc | n/a — chain hops | jagged bolt (already live) |
| Rail | charged-white line streak 2 m | small bright slug |

The projectile actor accepts `ProjectileTrailVfx` (attach-spawned, NEW) — the PMC
element core stays as fallback.

## Sound specification (Cue naming `SC_AW_Weap_...`)

| Family | Fire | Impact | Character |
|---|---|---|---|
| Kinetic | `SC_AW_Weap_Kinetic_Fire` | `SC_AW_Weap_Kinetic_Impact` | dry scrap-metal clap, 180 ms tail |
| Plasma | `SC_AW_Weap_Plasma_Fire` | `..._Impact` | airy charge "whoomp", 300 ms |
| Laser | `SC_AW_Weap_Laser_Fire` | `..._Impact` | synth zap, tight 90 ms |
| Arc | `SC_AW_Weap_Arc_Fire` | `..._Impact` | crackling discharge, 2 crack layers |
| Rail | `SC_AW_Weap_Rail_Fire` | `..._Impact` | deep thunk + metallic ring-off |

**Loudness map (LUFS, game mix):** fire shots −14 ±1, impacts −20, blank shots −22,
all cues peak-normalized ≤ −6 dBTP. Fire cues accept ±8% random pitch per shot.
Binding: `FireSound`/`ImpactSound` on the weapon profile (NEW) — plays at muzzle/impact
already; empty refs = silence (current behavior preserved).

---

## Binding steps (Antigravity)

1. Author the 5 weapon meshes + sockets; bind on `.uasset` weapon profiles
   (same ids: `Weapon_Scrapshot`, `Weapon_PlasmaCharger`, `Weapon_LumenBeam`,
   `Weapon_ArcCaster`, `Weapon_MagrailDriver`).
2. Set the five soft refs per profile (muzzle/impact/trail/fire/impact-sound).
3. Validate in-engine: each family fires with its own flash+sound; fallback returns
   if refs are cleared (drop weapon profile → CODE_DEFAULT path intact).

## Acceptance criteria

- [ ] Five families distinguishable **blindfolded** (sound) and at 20 m (visual).
- [ ] No Niagara system exceeds its particle budget; no muzzle light > 2k lumens.
- [ ] 30 sustained shots: zero hitch (async spawns, no sync loads).
- [ ] Removing all bindings returns the procedural octahedron/beam/arc fallbacks.
- [ ] Beam/Arc chains still hit the same targets (combat logic untouched).
