# ASTRAWILD Project Master Status & Comprehensive AI Handoff Runbook

> **Target Audience:** GLM 5.3, Antigravity, Qwen, and any incoming AI Game Engineers / Technical Directors.  
> **Last Updated:** 2026-09-01 20:25:00 (+07:00)  
> **Repository:** `banksaisuoy/astrawild-game`  
> **Target Engine:** Unreal Engine 5.5+ (Source / Custom Build at `E:\Epic Games\UnrealEngine`)  
> **Local Workspace:** `E:\AstrawildGame`  

---

## 1. Executive Summary & Project Status

ASTRAWILD is a third-person, sci-fi survival open-world game built with C++ and Unreal Engine 5.
The project has successfully transitioned from an initial **Zero-Asset Graybox Prototype** to a **Production Alpha / Visual Vertical Slice** with imported 3D skinned meshes, PBR landscape materials, and 12-zone dynamic procedural world generation.

### 📊 Codebase & System Metrics
- **C++ Source Files:** 142 files (72 Headers `.h`, 70 Source `.cpp`)
- **Total Lines of C++ Code:** **40,638 LOC**
- **Automation Test Coverage:** **53 / 53 Tests PASS (100%)**
- **C++ Build Time (Incremental):** ~15–60 seconds via UBT / UBA Local Executor on MSVC v143
- **Current Completion Status:** **~73–75% (Playable Production Alpha)**

---

## 2. Local Environment & Hardware Specifications

| Component | Specification |
| :--- | :--- |
| **Operating System** | Windows 11 (64-bit) |
| **GPU** | NVIDIA GeForce GTX 1660 Ti (6 GB VRAM, Driver 610.88) |
| **Target Graphics API** | Direct3D 11 (SM5) (`-d3d11` launch argument for high stability on GTX 1660 Ti) |
| **Unreal Engine Path** | `E:\Epic Games\UnrealEngine` |
| **Project Root** | `E:\AstrawildGame` |
| **DotNet SDK** | Bundled .NET 10.0 (win-x64) |
| **Visual Studio** | VS 2022 Community (Toolchain v14.44.35228, Windows SDK 10.0.22621.0) |

### ⚡ Essential Developer CLI Commands (PowerShell / Windows Terminal):

1. **Compile C++ Editor DLL:**
   ```powershell
   & "E:\Epic Games\UnrealEngine\Engine\Build\BatchFiles\Build.bat" ASTRAWILDEditor Win64 Development "E:\AstrawildGame\Astrawild.uproject" -WaitMutex -FromMsBuild
   ```

2. **Run Full Automation Test Suite:**
   ```powershell
   & "E:\Epic Games\UnrealEngine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "E:\AstrawildGame\Astrawild.uproject" -ExecCmds="Automation RunTests Astrawild; Quit" -nullrhi -unattended -NoSplash -stdout -testexit="Automation Test Queue Empty"
   ```

3. **Launch Standalone Game Window (1080p D3D11):**
   ```powershell
   & "E:\Epic Games\UnrealEngine\Engine\Binaries\Win64\UnrealEditor.exe" "E:\AstrawildGame\Astrawild.uproject" /Game/ThirdPerson/Lvl_ThirdPerson -game -windowed -ResX=1920 -ResY=1080 -d3d11 -log
   ```

4. **Re-run Python Asset Import Pipeline:**
   ```powershell
   & "E:\Epic Games\UnrealEngine\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "E:\AstrawildGame\Astrawild.uproject" -ExecutePythonScript="E:\AstrawildGame\Content\Python\AwPipeline\import_all.py" -NoUI -Log
   ```

---

## 3. Core Architecture & Implemented Systems

### 🏛️ C++ Module Hierarchy (`Source/AstrawildCore/`)
1. **Player & Locomotion (`AstrawildPlayerCharacter.cpp`):**
   - 3rd-person camera boom (360° mouse yaw/pitch, collision probing, zero ground clipping).
   - Soft-bound to `SK_Survivor_Exosuit` (22-bone skinned mesh) via `AstrawildArtPack.h`.
   - Locomotion states: Idle, Walk, Run, Jump, Aim, Fire, Gather.
   - Cylinder fallback placeholder hidden automatically upon successful mesh load.
2. **Combat & Weapons (`AstrawildCombatComponent.cpp`, `AstrawildWeaponActor.cpp`):**
   - 8 Production weapons defined (5 tiers: Scrap, Plasma, Arc, Rail, Singularity).
   - Projectile physics, hit scan raycasting, dynamic recoil, weapon swap (keys 1–5).
3. **AI & Echo Companions (`AstrawildEchoCharacter.cpp`, `AstrawildEchoAIController.cpp`, `AstrawildBestiaryData.cpp`):**
   - Bestiary codex containing **214 species definitions**.
   - 6 Core Production Echoes: *Terraquill*, *Cindermule*, *Voltpylon*, *Bastionbeetle*, *Mistmender*, *Deepdelver*.
   - AI state machine: Roaming, Following, Fleeing, Attacking, Harvesting.
4. **World & Terrain Generation (`AstrawildTerrainTileActor.cpp`, `AstrawildZoneSubsystem.cpp`, `AstrawildWorldBootstrapper.cpp`):**
   - 12 Continuous Biomes across a 3.2 km × 2.4 km world partition:
     1. *Dawn Fields* (Starter lush plains)
     2. *Whispering Woods* (Dense conifer forest)
     3. *Lumina Ridge* (High alpine crystal ridges)
     4. *Cinder Flats* (Volcanic ash plains)
     5. *Frostpeak* (Snow mountains & frozen shelf)
     6. *Searing Chasm* (Lava gorge)
     7. *Shimmering Oasis* (Water pools & glow reeds)
     8. *Ancient Basin* (Ruin archeological site)
     9. *Toxic Mire* (Spore marsh)
     10. *Obsidian Wastes* (Basalt rock fields)
     11. *Starfall Crater* (Meteor impact zone)
     12. *Apex Sanctuary* (Endgame pinnacle)
   - Procedural height evaluation using FBM Multi-fractal & Ridged Noise.
   - Bound to `M_Landscape_SciFiFrontier` (4-layer slope-blended shader).
5. **Inventory & Crafting (`AstrawildInventoryComponent.cpp`, `AstrawildCraftingComponent.cpp`):**
   - Grid-based inventory with weight capacity, stack sizes, and durability.
   - Tech Tree research stations and workbench crafting recipes.
6. **UI & HUD (`AstrawildHudWidget.cpp`, `AstrawildInventoryScreenWidget.cpp`, `AstrawildShopWidget.cpp`):**
   - Glassmorphism slate HUD: Health bar, Stamina bar, Energy gauge, Quickbar, Radar Compass.

---

## 4. Asset Ingestion & Art Pack Status (Batch 4)

- **Source Asset Directory:** `E:\AstrawildGame\ArtSource/`
- **Total Real Assets:** 112 items
  - **Meshes (32 GLB files):** 1 Exosuit survivor, 6 Echoes, 5 Weapons, 1 DawnSkiff hovercraft, 12 Foliage/Rock/Ruin pieces, 7 Resource nodes.
  - **Textures (44 PNG files):** 1K–2K PBR maps (Albedo `_D`, Normal `_N`, Packed ORM `_ORM`, Emissive `_E`).
  - **Audio (36 WAV files):** Footsteps, weapon fires, creature vocalizations, ambient loops, UI cues.
- **Unreal Engine Content Directory (`Content/`):**
  - Textures: `/Game/Textures/` (44 assets imported)
  - Materials: `/Game/Materials/M_Landscape_SciFiFrontier`, `/Game/Materials/M_Master_Surface`, and 30+ `MI_*` instances.
  - Characters: `/Game/Characters/Survivor/` and `/Game/Characters/Echoes/`
  - Audio: `/Game/Audio/`

---

## 5. Recent Fixes Applied (2026-09-01)

1. **Fixed Terrain Whiteout (`AstrawildTerrainTileActor.cpp`):**
   - Swapped hardcoded fallback `DefaultMaterial` to prioritize `M_Landscape_SciFiFrontier`.
2. **Fixed Survivor Mesh Mismatch (`AstrawildPlayerCharacter.cpp`):**
   - Added direct object load fallback for `/Game/Characters/Survivor/SK_Survivor_Exosuit` in `TryActivateSkeletalBody()`.
   - Guaranteed retirement and invisibility of the placeholder cylinder (`PlaceholderMesh->SetHiddenInGame(true)`).
3. **Fixed Foliage PBR Shader (`AstrawildBiomeDressingActor.cpp`):**
   - Swapped debug material to prioritize `M_Master_Surface`.

---

## 6. Disk Hygiene & Maintenance Guide

- **Total Project Size:** ~10.99 GB
  - Essential Source Code & Assets: **~131 MB**
  - Safe-to-purge C++ intermediate cache & `.pdb` files: **~10.4 GB** (`Intermediate/`, `Saved/Logs/`, `Binaries/*.pdb`)
- **Unreal Engine Size:** ~291 GB
  - Safe-to-purge Engine `.pdb` debug symbols & Intermediate: **~196 GB** (`Engine/Binaries/Win64/*.pdb`, `Engine/Intermediate/`)

---

## 7. Actionable Roadmap & Next Steps for GLM 5.3 / Next AI

```
┌─────────────────────────────────────────────────────────────────────────┐
│                      IMMEDIATE NEXT STEPS ROADMAP                       │
├─────────────────────────────────────────────────────────────────────────┤
│ [ ] Step 1: Animation Blueprint State Machine                           │
│     - Connect AM_Survivor_* sequences into a full AnimBP with blend-     │
│       spaces for 8-way directional movement and weapon aiming.          │
│                                                                         │
│ [ ] Step 2: Foliage & Biome Scatter Tuning                              │
│     - Tune InstancedStaticMeshComponent (HISM) density for Conifer and  │
│       Broadleaf trees across DawnFields and Glimmerwood.                │
│                                                                         │
│ [ ] Step 3: Echo Companion World Spawners                               │
│     - Connect FAstrawildZoneDescriptor wildlife tables to spawn the 6   │
│       Echo species naturally around Player camps and wild nests.        │
│                                                                         │
│ [ ] Step 4: Niagara VFX Muzzle & Impact Sockets                         │
│     - Bind NS_AW_MuzzleFlash and NS_AW_Weap_Impact to Weapon_R socket.  │
│                                                                         │
│ [ ] Step 5: Sound Spatialization & Ambient Cue Triggers                 │
│     - Bind 36 WAV sound cues to C++ Footstep and Combat delegates.      │
└─────────────────────────────────────────────────────────────────────────┘
```

---
*Ready for immediate continuation and execution by GLM 5.3 or peer AI agents.*
