# CP-08 — ANIMATION: Player Locomotion, Combat, Dodge, Weapon Handling, Echo Locomotion & Capture

**Goal:** responsive movement with readable intent. The C++ character controllers own
all movement math (speeds, dodges, flight, replication); animation is a presentation
layer over states the code already exposes. Root motion stays OFF everywhere
(net-safe); anim notifies call into existing gameplay hooks.

---

## 1. Player locomotion — `ABP_AW_Player` over `SK_AW_Player_Base`

| State (C++ source) | Anim | Blend |
|---|---|---|
| Idle | 2-pose blend + breathing 8 s | — |
| Walk 190 / Run 430 | walk/run cycles | speed-driven blendspace |
| Sprint/boost | run cycle 1.15 + lean | — |
| Jump/fall/land | 3 states + hard-land 0.4 s | apex blend |
| Swim (post-launch) | surface stroke cycle | waterline blend |
| Skiff seated/pilot | seated + stick idle | attach states (C++ attach exists) |
| Crouch/harvest | 2 poses | interact channel |

Locomotion notes: stop-frames within 0.15 s (front/back), 2-directional strafe
blendspace (3×3), foot-lock off (stylized), camera-driven torso twist ≤ 15°.

## 2. Combat

| Move | Length | Notify → existing hook |
|---|---|---|
| Melee swing A/B | 0.5/0.7 s | `ANIM_OnHit` at 55% → melee sweep (damage pipeline unchanged) |
| Ranged hip-fire pose | additive | muzzle socket recoil (family-specific kick) |
| Beam/Arc sustained | 0.2 s loop | brace + visor flare (CP-01 emissive) |
| Reload (per family) | 1.2–2.0 s | `ANIM_OnReloaded` → ammo pipeline |
| Hit-react ×4 dirs | 0.3 s | upper-body additive layer |

## 3. Dodge

`Anim_AW_Player_Dodge_F/B/L/R` — 0.55 s each, roll scale by distance (the C++ dodge
math already moves the character; anim lerps from start to end pose). Rules:
no root motion (translation is code-owned), 0.1 s anticip-8°, recover 0.2 s cancelable
into sprint, i-frames driven by C++ window — anim must not gate them.

## 4. Weapon handling

- Weapon sockets: right hand `b_WeaponGrip`, left-hand IK to `b_Barrel` foregrip.
- Per-family idle holds (Kinetic low-ready, Plasma cradle, Laser/Arc upright, Rail
  shoulder-braced).
- Equip/unequip 0.6 s (matches the C++ 0.5 s visual-rebuild poll — bind rebuild to
  the equip notify instead for frame-perfect swap).
- Aim offsets: 9-directional, pitch ±60°.
- Family recoil curves (additive): Kinetic sharp 8°, Plasma heavy 5° + settle, Laser
  light 2°, Arc jitter, Rail 12° two-stage.

## 5. Echo locomotion (per body plan — CP-02 §5 lists the sets)

Per-plan rigs share the bone naming standard. Additional contracts:
- Speed blendspace 3-point (graze 0.3 / walk 0.6 / run 1.0) × plan; grazing/sleep
  idles as additive micro-motions on the walk layer.
- Attack hit notify `ANIM_OnHit` at 55% routes the existing per-species attack cadence
  (C++ timer still owns cooldowns).
- Defeat + fade matches the capture window (see §6) — defeat is the fade-out state
  when capture fails.
- Herd behaviors (grazing heads down, migration walk) = additive head-down pose over
  locomotion; no new states.

## 6. Capture performance

On the Echo side (CP-05 §2 FX pairs with this):
| Stage | Anim |
|---|---|
| Tether attach | capture-react struggle loop (1.0 s loop) |
| Window strain | struggle intensity scales with capture window progress (morph target 0→1) |
| Success | shrink-into-sphere pose (limbs tuck 0.4 s) → mesh scale-out |
| Fail | defeat knock-down → grey dissolve |

Player side: cast-hold pose (arm extended, wrist device glow = scanner-tier tint),
release/finish quick pose 0.3 s.

## 7. NPC animation (village life)

The 12 NPCs run the biped plan + role idles: vendor counter-lean, guard patrol-ready,
elder seated-talk (binds the NEW dialogue screen open state — NPC faces the player
already via C++), fisher cast-loop, skiff warden chalk-poses. 8 role idles ≤ 40 frames.

## Acceptance

- [ ] Walk→run→sprint blends seamless at the C++ speeds (190/430/boost).
- [ ] Dodge i-frames exactly match C++ windows (anim never delays or extends).
- [ ] Melee hit notify fires the existing damage pipeline (same DPS timing as today).
- [ ] Equip rebuild frame-perfect via notify (no 0.5 s lag fallback once bound).
- [ ] Capture anim+FX sequence passes the "signature moment" eye test end-to-end.
- [ ] All anims net-safe: root motion off, movement authority stays in C++.
