# ASTRAWILD — PRODUCTION V2 MASTER PLAN
**Target:** Transform Engineering Prototype into a Commercial-Grade Sci-Fi Survival Open-World RPG (UE5)  
**Art Direction:** *Sci-Fi Survival Frontier* (ARK + Palworld + Subnautica + The Planet Crafter)  
**Pipeline Division:**  
- **GLM 5.3 (GitHub / Cloud):** Game Architecture, DataTables, Bestiary Codex (214 Species), Itemization, Quests, Combat Balance.  
- **Antigravity (Local UE5 Host):** Engine Production, Landscape Master Materials, Foliage & Biomes, Lighting & Post-Processing, Niagara VFX, Sci-Fi HUD, Packaging & QA.

---

## 🎯 Production Phase Matrix

```
┌─────────────────────────┐     ┌─────────────────────────┐
│ 1. ENGINEERING ENGINE   │ ──> │ 2. VISUAL PRODUCTION    │
│ [x] C++ Core Subsystems │     │ [/] Landscape Shaders   │
│ [x] UHT / Compilation   │     │ [/] Biome Lighting/Atmo │
│ [x] Standalone Packaging│     │ [ ] Foliage & Rocks     │
│ [x] 25 Unit Tests       │     │ [ ] Echo Body Visuals   │
└─────────────────────────┘     └─────────────────────────┘
             │                               │
             ▼                               ▼
┌─────────────────────────┐     ┌─────────────────────────┐
│ 3. CONTENT SCALING      │     │ 4. GAME FEEL & POLISH   │
│ [ ] 5 Distinct Biomes   │ ──> │ [ ] Sci-Fi Diegetic UI  │
│ [ ] 214 Bestiary Echoes │     │ [ ] Combat VFX (Niagara)│
│ [ ] 8-Tier Tech Tree    │     │ [ ] Audio & Ambience    │
│ [ ] Base Automation Bay │     │ [ ] 60 FPS Optimization │
└─────────────────────────┘     └─────────────────────────┘
```

---

## 🌲 Phase 1: World & Environment Visual Pass (Antigravity Local)

- [ ] **M-01: Procedural Landscape Master Material (`M_Landscape_SciFiFrontier`)**
  - Layer 1: Lush Bioluminescent Grass (Top normal, slope < 25°)
  - Layer 2: Weathered Cliff Granite (Slope > 35°, tri-planar projection)
  - Layer 3: Fertile Loam / Soil (Valley paths)
  - Layer 4: Beach Sand / Shallow Reef (Z < SeaLevel)
  - Distance-based tiling blend to eliminate repetitive macro textures.
- [ ] **M-02: Lighting & Atmospheric Scenery (`BP_SciFiSkyAtmosphere`)**
  - Directional Sun at 75,000 Lux with warm golden-hour scatter.
  - Exponential Height Fog with volumetric fog scattering & colored horizon tint.
  - PostProcess Volume: ACES tonemapper, subtle anamorphic bloom, exposure lock `[10.0, 14.0]`.
- [ ] **M-03: Procedural Foliage & Biome Dressing**
  - Scatter 5 tiers of procedural vegetation (Ferns, Sci-Fi Broadleaf Trees, Spore Canopies).
  - Procedural rock clusters, harvestable crystals (Astraite, Pyronite).
  - Water planes with depth fade, refraction, and shoreline foam.

---

## 🐾 Phase 2: Echo Creature Bestiary & Visual Silhouettes

- [ ] **E-01: 8 Body-Plan Vertex Shader Silhouettes (`AAstrawildEchoCharacter`)**
  - Beast, Dragon, Construct, Spirit, Elemental, Aquatic, Insectoid, Avian.
  - Emissive core energy veins colored by elemental affinity (Fire, Frost, Electric, Astra).
- [ ] **E-02: Living Ecosystem Behaviors (`AAstrawildEchoAIController`)**
  - Sleep / Roam / Grazing / Herd migration cycles.
  - Territorial defense vs hostile predators.
  - Worker Echo assignment at Player Base.

---

## 🛠️ Phase 3: Advanced Sci-Fi Exosuit, Weapons & Combat VFX

- [ ] **W-01: 5-Tier Weapon Roster & Niagara VFX**
  - Tier 1: Pulse Scrap Rifle (Kinetic projectile)
  - Tier 2: Plasma Carbine (Slow burning heat projectile)
  - Tier 3: Arc Lightning Cannon (Chain-lightning beam)
  - Tier 4: Heavy Railgun (High-velocity penetration beam)
  - Tier 5: Singularity / Gravity Cannon (Vortex distortion)
- [ ] **A-01: Exosuit Armor Modular System**
  - Modular slots: Helmet, Chestplate, Greaves, Thruster Boots, Resonator Core.

---

## 🖥️ Phase 4: Sci-Fi Diegetic HUD & Polish

- [ ] **UI-01: Glassmorphism Sci-Fi HUD Canvas**
  - Hexagonal HP / SP / Energy / Temperature gauges.
  - Crosshair with dynamic hit-markers, critical flashes, and lock-on brackets.
  - Mini-Radar showing compass bearing, nearby resource nodes, and Echo signatures.
- [ ] **UI-02: Full Inventory & Tech Research Tree UI**
  - Sleek grid inventory with item tooltip cards, rarity borders, and drag-and-drop.
  - Holographic Crafting Workbench menu.

---

## 📋 Task Execution Log
- `2026-08-30`: Initialized Production V2 Master Plan. Shifting from Engineering Prototype to Visual & Content Production.
