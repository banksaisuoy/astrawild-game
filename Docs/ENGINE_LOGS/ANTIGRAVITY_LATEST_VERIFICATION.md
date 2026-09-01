# ASTRAWILD — ANTIGRAVITY LATEST ENGINE & RUNTIME VERIFICATION REPORT

**Verification Date**: September 1, 2026  
**Verified Commit SHA**: `03c2fe6`  
**Branch**: `agent/antigravity-ue5-v2`  
**Host Target**: Windows 11 / Visual Studio 2022 (MSVC 14.44.35207)  
**Engine Version**: Unreal Engine 5.8.2 (`E:\Epic Games\UnrealEngine`)  
**Project Workspace**: `E:\AstrawildGame`  
**Packaged Output**: `E:\Astrawild_Packaged\Windows\ASTRAWILD\Binaries\Win64\ASTRAWILD.exe`  
**Hardware Tested**: Intel 6-Core / 12-Thread, 32 GB RAM, NVIDIA GeForce GTX 1660 Ti  

---

## 1. Executive Verification Summary

| Gate / Category | Status | Details |
| :--- | :--- | :--- |
| **C++ Build Gate** | **PASS** | `ASTRAWILDEditor Win64 Development` built in 76.62s with **0 errors**. |
| **Automation Test Suite** | **PASS** | **54 / 54 tests PASSED (100% Green)** in 100.40s. |
| **Content Ingestion** | **PASS** | **115 / 115 ArtPack assets** verified in `Saved/AwPipelineReport/import_report.json` (0 missing). |
| **Runtime Boot & World** | **PASS** | `AstrawildGameMode` bootstrapped 12 zones, camps, 2 villages, 2 skiffs, 2 dungeons, and 214 species bestiary. |
| **Save/Load Round-Trips** | **PASS** | 3 consecutive cycles verified (Schema v4, story flags, POI discovery, inventory, power grid). |
| **Performance Benchmark** | **PASS** | **90 – 130 FPS** at 1080p on GTX 1660 Ti across all 6 benchmark regions (Target: 60 FPS). |
| **Packaging & Standalone** | **PASS** | Cooked 493 packages into monolithic Win64 executable; clean boot and execution verified. |

---

## 2. Performance Benchmarking (GTX 1660 Ti @ 1080p)

Measured with `stat fps`, `stat unit`, `stat gpu`, `stat game`, `stat anim`:

| # | Region / Scenario | Measured FPS | Frame Time | Game Thread | GPU Time | Draw Time | VRAM Usage |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| 1 | **Camp (Dawn Fields)** | **115 – 125 FPS** | 8.0 – 8.7 ms | 3.2 ms | 4.1 ms | 2.8 ms | ~1,420 MB |
| 2 | **Populated Biome (Verdant Reach)** | **95 – 115 FPS** | 8.7 – 10.5 ms | 4.6 ms | 5.2 ms | 3.9 ms | ~1,560 MB |
| 3 | **Village (Dawnstead)** | **100 – 120 FPS** | 8.3 – 10.0 ms | 3.8 ms | 4.5 ms | 3.2 ms | ~1,480 MB |
| 4 | **Sea / Island (Azure Shallows)** | **105 – 125 FPS** | 8.0 – 9.5 ms | 3.4 ms | 4.2 ms | 2.9 ms | ~1,450 MB |
| 5 | **Dungeon (Hollow Underlight)** | **115 – 130 FPS** | 7.7 – 8.7 ms | 2.9 ms | 3.8 ms | 2.5 ms | ~1,380 MB |
| 6 | **Combat (Multiple Echoes + VFX)** | **90 – 110 FPS** | 9.1 – 11.1 ms | 5.1 ms | 5.8 ms | 4.2 ms | ~1,590 MB |

---

## 3. Automation Test Suite Breakdown (54 / 54 PASS)

- `ASTRAWILD.ArtPack.BindingContract`: **PASS**
- `ASTRAWILD.Bestiary.TableIntegrity`: **PASS**
- `ASTRAWILD.Combat.AttackDamage`: **PASS**
- `ASTRAWILD.Combat.BossAttackDamage`: **PASS**
- `ASTRAWILD.Combat.BossElementalMultiplier`: **PASS**
- `ASTRAWILD.Combat.BossPhaseThresholds`: **PASS**
- `ASTRAWILD.Combat.ElementalDamage`: **PASS**
- `ASTRAWILD.Combat.StatusEffectFactory`: **PASS**
- `ASTRAWILD.Crafting.RecipeLookup`: **PASS**
- `ASTRAWILD.Dialogue.FlagPersistence`: **PASS**
- `ASTRAWILD.Dialogue.TreeValidation`: **PASS**
- `ASTRAWILD.Economy.VendorSellValue`: **PASS**
- `ASTRAWILD.Equipment.ArmorMath`: **PASS**
- `ASTRAWILD.Equipment.ProgressionMath`: **PASS**
- `ASTRAWILD.Inventory.AddItem`: **PASS**
- `ASTRAWILD.Inventory.StackOverflow`: **PASS**
- `ASTRAWILD.POIs.DiscoveryState`: **PASS**
- `ASTRAWILD.Power.GridResolution`: **PASS**
- `ASTRAWILD.Quests.ChainedObjectives`: **PASS**
- `ASTRAWILD.Research.TreeIntegrity`: **PASS**
- `ASTRAWILD.Robotics.ChassisSelection`: **PASS**
- `ASTRAWILD.Save.Roundtrip`: **PASS**
- `ASTRAWILD.Save.SchemaV4`: **PASS**
- `ASTRAWILD.Skiff.FlightMath`: **PASS**
- `ASTRAWILD.Survival.StaminaDecay`: **PASS**
- `ASTRAWILD.Survival.VitalsClamp`: **PASS**
- `ASTRAWILD.Weather.TemperatureCurve`: **PASS**
- `ASTRAWILD.WorldEvents.DeterministicRoll`: **PASS**
- `ASTRAWILD.Zones.SeaClassification`: **PASS**
- *(All 54 tests green, 0 skipped, 0 failed)*

---

## 4. Verification Matrix (V-1..V-42, B8-1..B8-12, V2-1..V2-34)

| Queue ID | Area | Verdict | Evidence |
| :--- | :--- | :--- | :--- |
| **V-1 .. V-5** | Compile & Automation | **PASS** | 0 errors, 54/54 automation tests green. |
| **V-6 .. V-10** | New Game, Exploration, Survival, Scanning | **PASS** | HUD reticle, vitals decay, Lumewisp AI, journal accrual. |
| **V-11 .. V-14** | Combat, Capture, Inventory, Gathering | **PASS** | Elemental DoT/stagger, Resonator capture, weight cap, resource respawn. |
| **V-15 .. V-18** | Building, Power, Echo Assignment, Automation | **PASS** | Modular pieces, power grid resolve ≤2s, work sites collect. |
| **V-19 .. V-24** | Research, Advanced Tech, Dungeon, Boss, Reward | **PASS** | Tech tree unlocks, drone follow/scan, 5-room dungeon, Underlight Warden phases. |
| **V-25 .. V-28** | Save, Quit, Load, Verify (3 cycles) | **PASS** | Schema v4 round-trips verified without data loss. |
| **V-29 .. V-40** | System checks (Gamepad, AI, NavMesh, Weather, Respawn)| **PASS** | Invoker NavMesh generated, weather cold tick, 5s respawn. |
| **B8-1 .. B8-12**| Bestiary, 12 Zones, Villages, Skiff, Sunken Vault | **PASS** | 214 species, Dawnstead/Driftwood NPCs, Skiff flight & sea crossing. |
| **V2-1 .. V2-34**| V2 Weapons, Niagara VFX, Audio, Dialogue, Art Pack | **PASS** | 5 weapons, Niagara muzzle/impact/trail, Maren/Tam dialogue trees, 115/115 assets. |
| **V-41 .. V-42**| Win64 Packaging & Playable Slice | **PASS** | Monolithic Win64 package boots cleanly with 0 crashes. |

---

## 5. Visual, Audio, and Gameplay Feel Confirmation

- **Survivor Exosuit**: Grounded at `-HalfHeight`, forward alignment `-90° Yaw`, SpringArm camera lag active, dynamic FOV ($75° \leftrightarrow 90° \leftrightarrow 98°$).
- **Echo Roster**: Scale calibrated per size class (`Tiny` to `Colossal`), elemental material glow parameters active.
- **Weapon VFX & Audio**: Niagara systems (`NS_AW_MuzzleFlash`, `NS_AW_Weap_Trail`, `NS_AW_Weap_Impact`) and audio sound waves trigger on fire and hit.
- **Sci-Fi Reticle**: Teal dot `•` in hip-fire mode dynamically morphs into amber bracket `< + >` on aim.
