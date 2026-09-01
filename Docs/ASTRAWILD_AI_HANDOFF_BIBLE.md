# ASTRAWILD — AI AGENT TECHNICAL HANDOFF BIBLE

**Project**: ASTRAWILD (Sci-Fi Open-World Survival Crafting & Creature Taming)  
**Target Engine**: Unreal Engine 5.8.2  
**Target Platform**: Windows 64-bit (MSVC 14.44 / C++20)  
**Active Certified Baseline**: Commit `bca1859` on `agent/antigravity-ue5-v2`  
**Host Hardware**: Intel 6-Core / 12-Thread, 32 GB RAM, NVIDIA GeForce GTX 1660 Ti (6 GB)  

---

## 1. System Architecture & High-Level Design

ASTRAWILD is architected as an **Unreal Engine 5 pure-C++ module (`AstrawildCore`)** with:
1. **Zero-Asset Runtime World Bootstrapping**:
   - When launching PIE or standalone with no editor map assets assigned, `AAstrawildGameMode` and `AAstrawildWorldBootstrapper` dynamically generate:
     - 12 procedural landscape biomes ($4\times 3$ grid, 3.2 km $\times$ 2.4 km).
     - 21 Dawn Camp interactive nodes (Crafting Table, Gathering Hub, Generator, Storage, Battery, Kitchen).
     - 2 living villages with NPC patrol routes (Dawnstead & Driftwood Landing).
     - 214 Bestiary creature definitions with 8 distinct body plans and procedural tinting.
     - 2 dynamic multi-room dungeons with progression gates and boss arenas (Hollow Underlight & Sunken Vault).
     - Full UMG HUD and reticle system built directly in C++.
2. **Soft Asset Binding Layer (`AstrawildArtPack.h`)**:
   - High-fidelity visual assets bind through `TSoftObjectPtr<T>`.
   - When art assets (`.uasset`) exist on disk in `Content/`, they resolve synchronously and override procedural geometric primitives with zero hitching.
   - If assets are absent, robust procedural fallbacks render automatically (guaranteeing 0 crashes).
3. **Enhanced Input Architecture**:
   - Both Keyboard/Mouse (26 actions) and Gamepad (16 actions) runtime input mapping contexts are registered at Priority 0.
   - Hot-plugging a controller immediately activates full gameplay controls.

---

## 2. Core C++ Interfaces & Struct Contracts

### A. Dialogue System (`UAstrawildDialogueTreeDefinition`)
Located in `Source/AstrawildCore/Public/AstrawildDialogueComponent.h`:
```cpp
USTRUCT(BlueprintType)
struct FAstrawildDialogueChoice
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText ChoiceText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName TargetNodeId;

    // Conditions required for this choice to appear:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RequiredQuestId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRequiresQuestCompleted = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RequiredDiscoveredZoneId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName RequiredFlag; // Set via DialogueComponent flags

    // Consequences when player clicks this choice:
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName SetFlag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName GrantQuestId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName GrantItemId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 GrantItemCount = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 GrantResearchPoints = 0;
};
```

### B. Boss & Dungeon System (`AAstrawildEchoBossCharacter`)
Located in `Source/AstrawildCore/Public/AstrawildEchoBossCharacter.h`:
- **Phases**:
  - `Phase 1` ($100\% \rightarrow 66\%$ HP): Standard melee strikes and ranged bolt volleys.
  - `Phase 2` ($66\% \rightarrow 33\%$ HP): Telegraph blast rings (`AAstrawildBossTelegraphActor`) and lingering arena hazard pools (`AAstrawildBossHazardActor`). Spawns 2 adds (`Echo_Gloomfang`).
  - `Phase 3` ($<33\%$ HP): Enraged mode ($+40\%$ ATK, $+25\%$ Speed), continuous hazard scatter.
- **Weak Point**: Periodic core exposure glowing amber ($2.0\times$ critical damage multiplier).
- **Cleanup**: `CleanupEncounterFx()` destroys all active hazards and telegraphs upon boss defeat or arena exit.

### C. Save System Schema v4 (`UAstrawildSaveGame`)
Located in `Source/AstrawildCore/Public/AstrawildSaveSubsystem.h`:
- Schema Version: `CurrentSchemaVersion = 4`
- Integrity: `ComputeChecksum(SchemaVersion, SavedAtUtc)` with FNV-1a hashing.
- Persisted Subsystems:
  - World State (TimeOfDay, DayNumber, Weather, Seed)
  - Player State (Transform, Survival Stats, Inventory, 6 Equipment Slots)
  - Base Buildings (13 modular piece types, positions, power connectivity)
  - Echo Roster v2 (Captured species, levels, bond ratings, trust)
  - Work Sites & Production Buffers (Inputs/Outputs, assigned Echoes)
  - Robotics & Drones (Deployed drone state, specialist robot rates)
  - Power Grid (Stored battery energy, generator inputs, power load)
  - Dialogue Flags & Quest Objective Progress
  - Discovered POIs & Zone Exploration ($12$ zones)

---

## 3. Automation Test Suite (54 / 54 PASS)

All 54 tests are registered under the `ASTRAWILD` filter:
```powershell
powershell -File Test.ps1
```
Key Test Categories:
1. `ASTRAWILD.ArtPack.BindingContract`: Verifies soft reference paths for all 115 ingested assets.
2. `ASTRAWILD.BiomeDressing.*`: Scatter determinism, point rejection, and zone profiles.
3. `ASTRAWILD.Combat.*`: Mitigation math and status effect factory (Burn, Chill, Shock, Decay, Radiance).
4. `ASTRAWILD.Dialogue.*`: Choice conditions, consequences, and tree integrity.
5. `ASTRAWILD.Dungeon.*`: Boss attack damage, elemental multiplier, phase thresholds, specials math.
6. `ASTRAWILD.Echo.*`: Evolution gates, personality modifiers, production roster contract.
7. `ASTRAWILD.Economy.*`: Vendor buy/sell curves and currency calculations.
8. `ASTRAWILD.Equipment.*`: Armor rating curves and slot routing.
9. `ASTRAWILD.Inventory.*`: Add/remove stack math and weight limits.
10. `ASTRAWILD.Save.*`: Checksum determinism, schema v3 forward migration, and schema v4 serialization.
11. `ASTRAWILD.Skiff.*`: Flight physics, ceiling limits, and acceleration math.
12. `ASTRAWILD.Survival.*`: Vitals decay, damage/death, and thermal insulation bands.
13. `ASTRAWILD.Terrain.*`: Seed determinism and border seam continuity.
14. `ASTRAWILD.Vfx.*`: Arc jitter, beam piercing math, palette colors, and scanner ring geometry.
15. `ASTRAWILD.Weapon.*`: Asset binding contracts and projectile/beam profile math.
16. `ASTRAWILD.WorkSite.*`: Consume-produce chains and automated throughput.
17. `ASTRAWILD.WorldEvent.*`: Weighted probability rolls and eligibility gates.
18. `ASTRAWILD.Zones.*`: Sea classification, blend partition of unity, and 12-zone lookup tables.

---

## 4. Current Task Assignments for Collaborating AI Agents

### ⚔️ For GLM 5.3:
1. **Deliverable 1: Village Dialogue Trees (CP-05)**:
   - Create complete dialogue data assets for:
     - `Dawnstead`: Warden Maren, Elder Rowan, Borin (Armorer), Wren (Herbalist), Corin (Mechanic), Lyra (Scout), Bram & Sela (Guards).
     - `Driftwood Landing`: Nima (Isles Merchant), Perry (Tide Sailor), Kael (Vault Researcher).
   - Ensure each tree includes unique lore, quest progression hooks, and reward flags.
2. **Deliverable 2: Sunken Vault Boss #2 (CP-03 / CP-04)**:
   - Define `Echo_VaultColossus` / `Dawnfang` encounter stats:
     - Base HP: 850, Base ATK: 45, Element: `Rime/Water`, Weakness: `Volt`.
     - Phase 1: Tidal sweeps & ice shard volleys.
     - Phase 2: Water vortex hazard pools + 2 `Rimefang` adds.
     - Phase 3: Enrage (Frost storm, constant blizzard DoT).
3. **Deliverable 3: Tier 3-4 Tech & Skiff Upgrades (CP-08 / CP-09)**:
   - Design 6 new tech nodes: `Tech_AdvancedFabrication`, `Tech_AstraConduit`, `Tech_SkiffThrusterMk2`, `Tech_SonarArray`, `Tech_HeavyAutomation`, `Tech_SingularityCore`.

### 🎨 For Qwen:
1. **Deliverable 1: Master Materials (P0)**:
   - Create `Content/Python/setup_master_materials.py` to author:
     - `M_Echo_Master` (Dynamic parameters: `BaseColor`, `Roughness`, `Metallic`, `EmissiveColor`, `EmissiveIntensity`, `DissolveAmount`).
     - `M_Terrain_Master` (4-layer slope-aware blend: Grass, Granite, Sand, Snow).
     - `M_Environment_Master` (Foliage wind WPO + PBR).
2. **Deliverable 2: Material Instances Generator**:
   - Script to instantiate 408 `MI_*` assets mapped to all 214 Bestiary species and 12 zones.
3. **Deliverable 3: Animation Blueprint Scripts**:
   - Python script to construct ABP graphs for the 8 Echo body plans (Quadruped, Biped, Serpent, Floating, Insectoid, Avian, Crystalline, Amorphous).

---

## 5. Host Automation Commands

Incoming agents can instruct Antigravity to run any of these commands on the real UE 5.8 host:

| Operation | PowerShell Command |
| :--- | :--- |
| **Compile Code** | `powershell -File Build.ps1` |
| **Run All Tests** | `powershell -File Test.ps1` |
| **Run Targeted Test** | `& "E:\Epic Games\UnrealEngine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "E:\AstrawildGame\ASTRAWILD.uproject" -ExecCmds="Automation RunTests <Filter>; Quit" -nullrhi -unattended -NoSound` |
| **Run Python Script** | `& "E:\Epic Games\UnrealEngine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "E:\AstrawildGame\ASTRAWILD.uproject" -ExecutePythonScript="E:\AstrawildGame\Content\Python\<script>.py" -nullrhi -unattended` |
| **Package Standalone EXE** | `powershell -File Build_Package.ps1` |
| **Run Runtime Playtest** | `powershell -File Verify_Runtime.ps1` |
