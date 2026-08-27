# ASTRAWILD: Echoes of the First Dawn

> **Unreal Engine 5.8 Vertical Slice**  
> Primary C++ Module: `AstrawildCore`  
> Target Platform: Windows PC (Third-Person Creature-Taming Survival Action RPG)

---

## 🌟 Overview & Core Loop
ASTRAWILD is an original IP action RPG blending primal survival, dynamic creature taming ("Echoes"), tactical combat, crafting, and base shelter construction.

In this Vertical Slice:
1. **Explore**: Traverse the *Dawn Valley* test environment.
2. **Harvest**: Gather raw resources (Sunwood, Lumen Stone, Astra Flora) from procedural/interactive nodes.
3. **Combat**: Engage wild Echoes using responsive melee strikes, dodges, and elemental abilities.
4. **Capture**: Weaken wild Echoes and use the **Astra Resonator** to capture and register them to your active party/storage.
5. **Craft & Build**: Construct campfires, resting shelters, crafting benches, and storage chests with real-time hologram placement.
6. **Persist**: Comprehensive asynchronous Save & Load subsystem storing player stats, inventory, active/stored Echoes, and placed world structures.

---

## 🏗️ Architecture & Conventions
- **C++ Core (`AstrawildCore`)**: Contains authoritative game rules, math formulas, serialization structs, components, and base classes.
- **Data-Driven Layer**: DataAssets (`UAstrawildEchoDataAsset`, `UAstrawildItemDataAsset`, `UAstrawildRecipeDataAsset`) and GameplayTags for extensible, non-hardcoded tuning.
- **Stable Identifiers**: All Echo species, items, recipes, abilities, and buildings use stable tags (`Echo.Pyrelite`, `Item.Resource.Sunwood`, etc.).
- **Co-op Ready Design**: Clean actor-component isolation with authoritative state replication preparation.

---

## 🐾 Initial Vertical Slice Echoes
1. **Pyrelite (The Ember Fawn)** - Solar/Flare Affinity. High speed skirmisher, camp ignition assistant.
2. **Aquavine (The Dew Serpent)** - Torrent/Hydration Affinity. Ranged slowing attacks, crop/flora hydration.
3. **Thornback (The Terra Bramble)** - Geo/Verdurous Affinity. High defense tank, heavy mining/logging specialist.

---

## 📁 Repository Directory Structure
```
ASTRAWILD/
├── ASTRAWILD.uproject
├── Config/                  # Engine, Game, Input configurations
├── Docs/                    # Technical, Design, and QA specifications
├── Content/Astrawild/       # Blueprints, DataAssets, Maps, UI Widgets, CSV source data
├── Scripts/                 # Text/content contract validation
└── Source/
    ├── ASTRAWILD.Target.cs
    ├── ASTRAWILDEditor.Target.cs
    └── AstrawildCore/       # Core C++ Module (Public / Private)
```

---

## 📖 Technical Documentation
- [Architecture & C++ Guidelines](Docs/Architecture_Design.md)
- [Echoes Species Database](Docs/Echoes_Database.md)
- [Combat & Capture Mechanics](Docs/Combat_And_Capture_Design.md)
- [Crafting & Building Specification](Docs/Crafting_And_Building_Spec.md)
- [Testing & Verification Guide](Docs/Testing_And_Verification_Guide.md)
- [P0 Animation Contract](Docs/P0_Animation_Contract.md)
- [P1 Niagara and Sound Contract](Docs/P1_Feedback_Contract.md)
- [P2 UMG Contract](Docs/P2_UMG_Contract.md)
- [P3 Alpha Echo Design](Docs/P3_Alpha_Echo_Design.md)
- [P4 Lore and Quest Data Contract](Docs/P4_Lore_Quest_Data_Contract.md)
- [Remote MCP Guide](Docs/REMOTE_MCP_GUIDE.md)
- [Feature Benchmark Matrix](Docs/FEATURE_BENCHMARK_MATRIX.md)
- [Feature Roadmap](Docs/FEATURE_ROADMAP.md)
- [Progression, Survival and Save Contract](Docs/PROGRESSION_SURVIVAL_SAVE_CONTRACT.md)

## Validation without Unreal Editor

Run `python Scripts/validate_content_contracts.py` to validate the P0-P4 source contracts and CSV headers. This does not replace an Unreal C++ compile, DataTable import, Blueprint validation, or PIE test.