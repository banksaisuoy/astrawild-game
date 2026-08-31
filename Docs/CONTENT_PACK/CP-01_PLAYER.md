# CP-01 — PLAYER: Exosuit, Armor Tiers, Materials & Equipment Visuals

**Goal:** the survivor reads as a scavenger-engineer in a graphite/amber sci-fi frontier —
not a gray capsule. The procedural silhouette (Batch 2) already establishes proportions and
palette; this pack replaces it with real meshes, tiered materials and visible progression.

---

## 1. Exosuit specification

**Base body (naked frame) — `SK_AW_Player_Base`**
| Property | Value |
|---|---|
| Skeleton | Root at pelvis, 3-finger IK-ready hands, visor socket bone `b_Visor` |
| Tri budget | ≤ 18,000 (body) + 2,500 (hands) |
| UVs | 2 sets: 2048 albedo/normal/ORM + 1024 emissive/detail |
| Dimensions | 180 cm tall, 58 cm shoulder width (matches current capsule 34/88) |

**Exosuit modular slots** (5 visible layers, all attached to `SK_AW_Player_Base`):
1. **Underlayer** — woven thermal suit (always on). `SM_AW_Exo_Underlayer`, ≤ 4k tris.
2. **Chestplate** — swaps per armor tier. `SM_AW_Exo_Chest_T1..T4`.
3. **Helm/visor** — swaps per tier. `SM_AW_Exo_Helm_T1..T4`. Visor emissive = scanner-tier
   color (teal `#A2F6EF` / amber `#FFC2A6` / violet `#D9A3FF` — CP-05 §palette).
4. **Greaves + boots** — shared by tier pairs (T1/T2 one sculpt, T3/T4 heavy variant).
5. **Thruster pack** — T3+ only; amber emissive vents matching `FAstrawildVfxPalette` Ember.

**Thruster FX (T3+):** idle = faint heat shimmer; boost (Skiff/boost-sprint) = Ember
`#FFB87C` cone, 0.4 s puff — Niagara `NS_AW_Exo_ThrusterBurst` (CP-05 §7).

## 2. Armor tiers

The C++ ladder is `EAstrawildTechTier` (Field, Mk1, Mk2, Mk3, Experimental). Item ids exist:

| Tier | Item (existing) | Visual identity | Materials |
|---|---|---|---|
| Field | Scavenger gear (start) | Patched tarps, strap bundles, no plates | `M_AW_Armor_Field` — cloth + worn leather |
| Mk I | `Item_ScavengerVest` etc. | First plates over cloth — mixed metal scrap | `M_AW_Armor_Mk1` — scratched steel, amber accent straps |
| Mk II | `Item_VanguardHelm/Vest` "Vanguard" | Layered plate, split thermal bands (per item lore) | `M_AW_Armor_Mk2` — two-tone plate + insulation seams |
| Mk III | `Item_BastionHelm/Plate` "Bastion" | Siege bulk, deep winter/forge shading | `M_AW_Armor_Mk3` — heavy plate, cold-blue core glow |
| Experimental | `Item_AstralforgedExosuit` | Ancient-alloy exosuit — light-vein circuitry | `M_AW_Armor_Astral` — pale-gold Light element `#FDF9DD` emissive veins |

**Rules:** each tier must read at 10 m in silhouette (bulk + accent color); insulation stats
already differ per tier in data (`ColdInsulationRating`/`HeatInsulationRating`) — visuals may
telegraph them (Mk II banding = balanced, Mk III = heavy both-sides) but never contradict.

## 3. Materials

| Material | Shading model | Textures | Notes |
|---|---|---|---|
| `M_AW_Armor_Field` | DefaultLit | T_2048 set | Cloth response; rain darkening via CP-04 weather param |
| `M_AW_Armor_Mk1` | DefaultLit | T_2048 set | Scratches in ORM roughness channel |
| `M_AW_Armor_Mk2` | DefaultLit | T_2048 set + emissive 1024 | Amber seam emissive ×0.6 (never bloom-white) |
| `M_AW_Armor_Mk3` | DefaultLit | T_2048 set + emissive 1024 | Frost-blue core emissive `#C4F1FD` |
| `M_AW_Armor_Astral` | DefaultLit + ClearCoat | T_2048 set + emissive 1024 | Vein emissive pulses at 0.8 Hz — “alive” ancient alloy |
| `M_AW_Cloth_Master` | DefaultLit | T_1024 | Shared underlayer/straps |

All armor materials expose: `TintArmor` (linear, default white), `EmissiveIntensity` (0–2),
`Wetness` (0–1, driven by weather CP-04 §6).

## 4. Equipment visuals — binding contract

**NEW C++ (this batch):** `UAstrawildItemDefinition`
- `TSoftObjectPtr<UStaticMesh> EquipMeshOverride`
- `TSoftObjectPtr<UMaterialInterface> EquipMaterialOverride`

**Binding steps (Antigravity):**
1. Create `.uasset` item definitions for the seven armor items (ids must match:
   `Item_VanguardHelm`, `Item_VanguardVest`, `Item_BastionHelm`, `Item_BastionPlate`,
   `Item_AstralforgedExosuit`, plus Mk I pieces) — same-id override of CODE_DEFAULTs.
2. Set `EquipMeshOverride` → `SM_AW_Exo_Chest_Mk2` (etc.), `EquipMaterialOverride` → `M_AW_Armor_Mk2`.
3. Equipment rig: an `ABP_AW_Player` slot component reads the equipped chest/helm items'
   overrides; null/unloaded → procedural silhouette sections stay (today's behavior).
4. Weapons already read tier scale (Field ×1.0 → Experimental ×1.6) and family tint —
   keep those until CP-03 weapon meshes land.

## 5. Acceptance criteria

- [ ] Player at spawn: graphite/amber survivor, visor teal glow, backpack silhouette.
- [ ] Equipping each tier changes chest+helm visuals immediately (no restart).
- [ ] Astralforged veins visibly pulse at 0.8 Hz.
- [ ] Total character texture memory ≤ 24 MB; ≤ 6 draw calls for full body+gear.
- [ ] Fallback contract intact: removing all bindings returns the procedural silhouette.
