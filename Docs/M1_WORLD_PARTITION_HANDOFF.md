# ASTRAWILD Milestone 1 — World Partition and Hazard Handoff

## Scope

Milestone 1 defines the production contract for a **4.096 km × 4.096 km** playable world represented by an **8 × 8 logical cell grid (64 cells)**. The grid is a gameplay/data contract; it is not a claim that a binary Unreal map has already been authored. The branch currently provides C++ data types, a world subsystem, hazard runtime logic, CSV sources, and validation rules. Unreal Editor must still create the map, World Partition actor descriptors, Data Layers, landscape, navmesh, and visual assets on Windows.

## Authoritative source files

| Area | Source | Purpose |
|---|---|---|
| World schema | `Source/AstrawildCore/Public/World/AstrawildWorldData.h` | Four biomes, spawn rows, spire rows, 64-cell value type |
| World subsystem | `Source/AstrawildCore/Public/World/AstrawildWorldPartitionSubsystem.h` | Deterministic cell lookup, biome fallback, spire discovery/travel contract |
| Hazard runtime | `Source/AstrawildCore/Public/World/AstrawildEnvironmentHazardComponent.h` | Temperature, insulation, camp protection, authority-only health damage |
| CSV source | `Content/Astrawild/Data/Source/DT_Biomes.csv` | Exactly four biome definitions |
| CSV source | `Content/Astrawild/Data/Source/DT_SpawnRules.csv` | Data-driven spawn rules |
| CSV source | `Content/Astrawild/Data/Source/DT_FastTravelSpires.csv` | Exactly sixteen spires |

## Unreal Editor assembly procedure

1. Create or open an **Open World** map in Unreal Engine 5.8. Enable World Partition for the map and keep the editor-generated map package under `Content/Astrawild/Maps/World/`. Do not replace the existing prototype map until the new map can boot into PIE.
2. Set the world bounds to cover a 4,096 m square. Unreal units are centimeters, so the gameplay bounds are approximately `(-204800, -204800)` to `(204800, 204800)` in X/Y. Use a clearly named World Partition bounds actor or authored volume and record the final coordinates in `Docs/BUILD_STATUS.md`.
3. Import the three CSV files as DataTables using `FAstrawildBiomeDefinition`, `FAstrawildWorldSpawnRule`, and `FAstrawildFastTravelSpire`. Preserve row names and IDs; do not rename rows in the Editor without updating the CSV and validator.
4. Create four Data Layers: `DL_DawnMeadows`, `DL_SylvanRainforest`, `DL_ScorchedObsidianCaldera`, and `DL_GlacialZenith`. Place biome-specific lighting, landscape decoration, resource clusters, and spawn volumes in their corresponding layer.
5. Create logical cell markers or volumes for all coordinates `(0,0)` through `(7,7)`. The C++ subsystem maps world-space positions to this grid; cell markers are for authoring/debugging and must not become a second conflicting coordinate system.
6. Author sixteen spire actors using the row IDs in `DT_FastTravelSpires`. Each actor must expose its `SpireId`, `BiomeId`, discovery interaction, server-authoritative travel, destination collision validation, and a readable world-space marker. The first row is the only default-unlocked entry in the provided source.
7. Add a NavMeshBoundsVolume that covers the authored playable region in manageable streaming/runtime-generation sections. Confirm that navigation generation does not attempt to cover empty space outside the intended bounds.
8. Add biome hazard volumes or biome-level controllers that call `SetAmbientTemperature` on the player `EnvironmentHazard` component. Clothing/armor and camp/rest-point systems should modify insulation/protection rather than directly applying duplicate temperature damage.
9. Build and run PIE in a listen-server or standalone session. Verify: cell lookup at all four corners, spire discovery persistence, travel only on authority, hazard damage only on authority, and no damage while effective temperature stress is below the danger threshold.
10. Commit binary map, DataTable, Data Layer, Landscape, NavMesh, actor, material, and VFX assets through Git LFS where appropriate. Keep raw source/archive files in the project’s Google Drive archive and update `Docs/ThirdPartyLicenses.md` for every external asset.

## Acceptance gates

| Gate | Evidence required | Status before Windows run |
|---|---|---|
| Source contract | Static validator and `git diff --check` pass | Not yet rerun after this slice |
| Unreal compile | `ASTRAWILDEditor Win64 Development` succeeds | Not verified in this environment |
| DataTable import | Three tables load with expected row counts | Not verified |
| World assembly | 4 km bounds, World Partition, four Data Layers, 64 cell markers | Not authored in Git |
| Fast travel | 16 spires, server authority, blocked-destination handling | Not verified |
| Hazard | Stress threshold, insulation/camp mitigation, server-only health damage | Source prepared; PIE not verified |
| Packaging | Development packaged build launches and enters world | Not verified |

## Windows handoff commands

```powershell
git fetch origin
git checkout release/vertical-slice-v1
git pull --ff-only origin release/vertical-slice-v1
python Scripts/validate_content_contracts.py
& .\Tools\Validate_Astrawild.ps1
```

After the Editor assembly, update `Docs/BUILD_STATUS.md` with engine version, map path, compile output, PIE result, packaged build path, and screenshots. A static pass must not be reported as a successful compile, PIE session, or shipping package.
