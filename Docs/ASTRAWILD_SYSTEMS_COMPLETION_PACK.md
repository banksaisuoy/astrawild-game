# ASTRAWILD — Systems Completion Pack (SCP)

> Branch: `final-completion` · Commits: a7a827f → 394ac81 → 0e9fc2b → edc6b08 → 6cd29e4 → bbe2e3c → 9864cce
> Source: the vULTIMATE 14-phase master directive, plan-vs-repo audited on session start (84 tests baseline).

## 1. Plan-vs-repo audit (before SCP)

| Phase | Directive system | Pre-SCP state | SCP action |
|---|---|---|---|
| 1.2 | `UAstrawildDataValidator` (checksum, dup ids, refs, stat bounds) | MISSING | SCP-1 static tables + registry reference integrity + content checksum |
| 2.1 | AssetFallbackManager (procedural stand-in, never crash) | MISSING (silent misses) | SCP-1 engine-shape fallback library + reported skiff/survivor paths |
| 2.3 | ErrorReporter (Standalone log file) | MISSING | SCP-1 ring buffer + `Saved/Logs/AstrawildErrorReport.log` flush |
| 3.2 | Dynamic Difficulty Adjustment | MISSING | SCP-4 skill bands + hostile/resource scaling |
| 5.3 | Mount/Rider + sockets | MISSING (skiff only) | SCP-3 mount component + 6-socket contract + input routing |
| 6.3 | Dual-Tech Combos (10 recipes) | MISSING | SCP-4 12-reaction table + 3s window + Steam Explosion x2.5 + hitstop |
| 7.2 | NPC schedules (4 professions, rain shelter) | MISSING | SCP-5 schedule component on every NPC |
| 8.1 | Farm plot 7-state crop lifecycle | MISSING (building only) | SCP-5 crop component + water/fertilizer/season math |
| 8.3 | Offline production (max 48h) | MISSING | SCP-5 CreditOfflineProduction (50% rate, honest inputs) |
| 9.1 | Base Terminal (Palbox, radius 3500, garrison 5-20) | MISSING | SCP-2 terminal actor + territory gating + decay |
| 9.2 | Sanity (SAN) + illness + healthcare | MISSING | SCP-2 sanity component + Cure Tonic + Medicine Bench |
| 10.3 | Genetics (4 traits + IVs) | MISSING | SCP-6 genetics library + Breeding Pen + Egg Incubator |
| 11.2 | Defense turret | MISSING (night raid exists) | SCP-5 turret component + Bolt Turret building |
| 12.1 | Item durability + Repair Bench + tool specialization | MISSING | SCP-1 durability component + pick/axe/sickle x3/x3/x4 |
| 12.2 | Spoilage + Ice Box | MISSING | SCP-1 spoilage subsystem + x10 preservation + compost loop |
| 13.1 | Dynamic performance manager | MISSING | SCP-6 tiered scalability ladder (console vars) |
| 13.2 | Object pooling (projectiles/debris) | NOT IN SCP | Deferred — see §4 risks (documented next-phase item) |
| 4.2/4.3 | TeamAgent friendly-fire + RPC rate limiter | NOT IN SCP | Deferred — cooldown gating already exists server-side; full team interface is next-phase |

Everything else in the 14-phase plan (data tables 204 species, save migration
V1→V5, tutorial chain, GAS-style attributes, tri-locomotion, ability engine,
time/seasons, labor automation, raids (Night Raid), tech tree, build scripts)
was already source-complete before SCP and is untouched.

## 2. Test inventory

84 → **99** automation contracts (+15): DataValidator tables, ErrorReporter
ring, AssetFallback paths, Spoilage math, Durability contracts, Sanity math,
BaseTerminal levels, Mount spec, Combo table, DDA bands, Crop math, NPC
schedules, Turret policy, Genetics inheritance, Perf tiers.

All ENGINE-UNVERIFIED until Antigravity runs the suite (AG-2).

## 3. Save schema (additive v5 — no schema bump)

- `EquipmentDurability` (TMap<FName,float>) — wear pools
- `FoodFreshness` (TMap<FName,float>) — spoil aging
- EchoInstanceV2: `Sanity`, `IllnessId`, `Traits` (TArray<FName>)
- BuildingSaveData: `CropSeedId`, `CropState`, `CropGrowth`, `bCropFertilized`

Every import path sanitizes (unknown ids dropped, values clamped, NaN-safe).

## 4. Known risks / next phase

- **Object pooling (13.2)**: projectile lifecycle integration deferred — requires engine-side verification of the destroy path before safe retrofit.
- **Team interface (4.2)**: co-op friendly-fire wants IGenericTeamAgentInterface at perception layer; SP/LS behavior is already party-safe via capture ownership checks.
- **Warm-hatch simplification**: incubation is interact-driven (temperature grants a level bonus, not a timer) — a timed incubator state needs a per-building tick budget decision.
- **Breeding lineage**: the egg follows the dominant parent's species (single- species egg item); multi-species eggs need egg-item payload serialization.
- Riding is transient by design (no save field) — remount after load.
- All SCP numbers (multipliers, bands, rates) are PIE-tuning candidates; contracts pin the *shipped* values.
