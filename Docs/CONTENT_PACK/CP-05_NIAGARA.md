# CP-05 — NIAGARA: Weapon, Capture, Scanner, Shield & Elemental Effects

**Goal:** one effects language for the whole game — every system reads from
`FAstrawildVfxPalette` (element/rarity/family/scanner tints, already live in C++).
All systems live as content; **binding fields for weapons are NEW this batch**
(`MuzzleFlashVfx/ImpactVfx/ProjectileTrailVfx` on weapon profiles; consumed
Niagara-first with procedural fallback).

**Shared conventions:** GPU sprites where possible; all systems accept a
`User.ElementColor` linear-color parameter defaulting to the palette value;
lifetime ≤ 0.4 s for gameplay feedback; no lights (the C++ procedural actors own
light hand-off until the art pass proves the light budget).

---

## 1. Weapon effects (CP-03 details apply)

| System | Palette input | Procedural fallback it replaces |
|---|---|---|
| `NS_AW_Weap_Muzzle_*` ×5 | family tint | octahedron flash + 12k light |
| `NS_AW_Weap_Impact_*` ×3 | element tint | (none — new capability) |
| `NS_AW_Weap_Trail_*` ×3 | element tint | PMC element core |
| `NS_AW_Weap_Beam_Laser` | red `#FF8C8C` | beam prism actor |
| `NS_AW_Weap_ArcChain` | electric `#BEE5FF` | jagged PMC lightning |
| `NS_AW_Weap_LockOnBracket` | family tint | (new) target bracket — see CP-10 |

Beam/Arc upgrades bind on the combat component's existing spawn sites
(`SpawnBeam`/`SpawnArcChain` remain the procedural path).

## 2. Capture effect — `NS_AW_Echo_Capture` (the signature moment)

Stages (mirror the C++ capture window state machine):
1. **Beam tether** (cast → window): teal energy lasso, 12 cm ribbon, slight sag.
2. **Struggle pulses** (window ticks): beam stretches/whips with the target's anim.
3. **Collapse** (success): beam snaps taut → target shrinks into a **rarity-colored
   energy sphere** → beams home to the player's wrist device → chime (CP-06 §5).
4. **Break** (failure): beam shatters into sparks, grey-out 0.3 s.

Success sphere uses the rarity tint (Rare `#9EE5F8`, Epic `#E2ADF9`, Legendary
`#FFDD81`, Mythic `#FA90AD`); Mythic adds 2 slow orbit rings. Budget ≤ 64 particles.

## 3. Scanner — `NS_AW_Scanner_Pulse` (replaces the PMC annulus ring)

- Expanding ring mesh-ribbon, scanner-tier color (Field teal `#A2F6EF`, Array amber
  `#FFC2A6`, Oracle violet `#D9A3FF`), radius = live C++ multiplier × 2400 cm.
- **Node reveal sub-burst:** hidden veins flare once through terrain (depth-tested off,
  0.6 s) — visual only, reveal authority stays C++.
- **Echo signatures:** soft amber motes over creatures inside scan radius (≤ 24 motes).
- **Ancient signal tracking (Oracle):** violet compass ribbon pointing at the 2 signal
  POIs while the tracker HUD is active.

## 4. Evolution burst — `NS_AW_Echo_Evolve` (CP-02)

Rarity-ring collapse inward → element column 3 m → white flash → burst of
element-colored petals/shards. 1.6 s, ≤ 80 particles, one point light ≤ 4k lumens
(acceptable: single rare event).

## 5. Shield / damage feedback

| System | Trigger | Look |
|---|---|---|
| `NS_AW_Player_ShieldHit` | mitigated hit ≥ 15 damage | hex-cell flash across torso, Frost glacial `#C4F1FD` |
| `NS_AW_Player_Heal` | Mending Aura tick (Rare threshold) | Light `#FDF9DD` upward motes ×6 |
| `NS_AW_Player_Overheat` | heat/Ember status applied | Ember `#FFB87C` rim shimmer |

Exosuit shield: T3+ armors get a faint hex-cell body shell (0.15 opacity) that
brightens on ShieldHit — mesh-attached system `NS_AW_Exo_ShieldShell` (CP-01 link).

## 6. Elemental status effects — `NS_AW_Elem_*` ×6

Live on the status system (C++ applies the gameplay; FX attach to target):
- **Ember:** ember trail (drips), 6 particles/s, 3 s.
- **Frost:** frost rime builds on limbs (opacity ramp 0→1 over 2 s) + shard pop on end.
- **Pulse:** periodic crackle arcs (2 arcs every 0.7 s, stun beats).
- **Flora:** root grip ring at feet + petal burst per damage tick.
- **Light:** golden spark drift + 1 glow pulse.
- **Ash:** grey veil fall + dissolve edge on defeated targets.

All ≤ 24 particles; attach to `b_Spine_02`; network: server-confirmed only (status is
replicated — FX reads the replicated state, never predicts).

## 7. Exosuit thruster — `NS_AW_Exo_ThrusterBurst` (CP-01 §1)

## 8. Rarity ring — `NS_AW_Echo_RarityRing` (CP-02 §4)

## 9. Stretch (post-launch)

- Surface-switching impact FX via physical materials.
- Weather-mated FX: storm static on metal ruins, reef bubbles underwater.
- Skiff contrail + water spray on hull (`NS_AW_Skiff_Wake`).

## Budgets & acceptance

- [ ] Gameplay feedback systems (muzzle/impact/trail) ≤ 0.4 s lifetime each.
- [ ] 30 Echoes with active statuses: ≤ 1,200 GPU sprites total, 60 FPS held.
- [ ] Capture sequence reads unmistakably as capture (tether→sphere→home-in).
- [ ] Every system responds to `User.ElementColor` (palette discipline).
- [ ] Clearing bindings returns the PMC/procedural fallbacks (zero-asset rule).
