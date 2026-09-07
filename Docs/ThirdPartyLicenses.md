# ASTRAWILD — Third-Party Asset Licenses

> Keep one row for every external asset that enters the Unreal project. A private game still needs a clear record of the rights attached to files shared with collaborators.

| Asset ID | File/pack name | Creator/publisher | Source URL | License | Download/purchase date | Included files | Usage | Notes |
|---|---|---|---|---|---|---|---|---|
| `KENNEY_IMPACT_SOUNDS` | Impact Sounds | Kenney (kenney.nl) | https://kenney.nl/assets/impact-sounds | CC0 1.0 Universal | 2026-09-04 | 130 OGG + 16-bit PCM WAV conversions (`ArtSource/Audio/Kenney_ImpactSounds/`) | Impact/hit feedback library: harvest, melee, structure impacts, footstep surfaces | Per-file SHA256 in `ASSET_MANIFEST.json`; pack zip SHA256 in `ASSETS_CREDITS.md`; publisher `License.txt` committed with the pack |
| `KENNEY_INTERFACE_SOUNDS` | Interface Sounds | Kenney (kenney.nl) | https://kenney.nl/assets/interface-sounds | CC0 1.0 Universal | 2026-09-04 | 100 OGG + WAV conversions (`ArtSource/Audio/Kenney_InterfaceSounds/`) | UI feedback palette (navigation, confirmation, error, screen transitions) | Same provenance records as above |
| `KENNEY_SCIFI_SOUNDS` | Sci-fi Sounds | Kenney (kenney.nl) | https://kenney.nl/assets/sci-fi-sounds | CC0 1.0 Universal | 2026-09-04 | 73 OGG + WAV conversions (`ArtSource/Audio/Kenney_SciFiSounds/`) | Energy-weapon fire, doors, computer noise, force fields, explosions, skiff engines | Same provenance records as above |
| `KENNEY_NATURE_KIT` | Nature Kit 2.1 | Kenney (kenney.nl) | https://kenney.nl/assets/nature-kit | CC0 1.0 Universal | 2026-09-04 | 314 GLB models (`ArtSource/Models/Kenney_NatureKit/GLB/`) | Biome dressing for the 12-zone world + farms/villages/ruins pieces | GLB only; FBX/OBJ/DAE/STL duplicates dropped; ground_grass/ground_river tiles excluded (procedural terrain/water) |
| `KENNEY_SPACE_KIT` | Space Kit | Kenney (kenney.nl) | https://kenney.nl/assets/space-kit | CC0 1.0 Universal | 2026-09-04 | 107 GLB models (`ArtSource/Models/Kenney_SpaceKit/GLB/`) | Dungeon/ancient-tech dressing + Bolt Turret candidates | Character/vehicle/monorail/rocket models excluded (deferred scope, no consuming system) |
| `KENNEY_BLASTER_KIT` | Blaster Kit 2.1 | Kenney (kenney.nl) | https://kenney.nl/assets/blaster-kit | CC0 1.0 Universal | 2026-09-04 | 40 GLB models + shared `Textures/colormap.png` (`ArtSource/Models/Kenney_BlasterKit/GLB/`) | Energy weapon model library — CANDIDATE_REPLACEMENT pool for held-weapon meshes | Colormap dependency kept beside the GLBs so relative URIs resolve |
| `KENNEY_PARTICLE_PACK` | Particle Pack | Kenney (kenney.nl) | https://kenney.nl/assets/particle-pack | CC0 1.0 Universal | 2026-09-04 | 96 transparent-background PNG sprites + pre-rotated frames (`ArtSource/Textures/Kenney_ParticlePack/PNG/`) | Combat VFX sprite library for Niagara (muzzle/impact/spark/smoke/glow) | Baked-black-bg duplicate set, Preview.png and XML metadata excluded at selection; per-file SHA256 in `ASSET_MANIFEST.json` |
| `KENNEY_UI_PACK_SCIFI` | UI Pack: Sci-Fi (Space Expansion) | Kenney (kenney.nl) | https://kenney.nl/assets/ui-pack-sci-fi | CC0 1.0 Universal | 2026-09-04 | 690 PNG panels/buttons/icons (6 color families × Default/Double states) + Kenney Future & Kenney Future Narrow TTF fonts (`ArtSource/Textures/Kenney_UIPackSciFi/`) | Sci-fi UI art for the 7 C++ UMG widget classes + UMG typography | SVG vector sources, previews and samples dropped; 50 identical-content mask (`_m`) files hash-deduped to single copies |
| `KENNEY_SURVIVAL_KIT` | Survival Kit 2.0 | Kenney (kenney.nl) | https://kenney.nl/assets/survival-kit | CC0 1.0 Universal | 2026-09-04 | 80 GLB models (`ArtSource/Models/Kenney_SurvivalKit/GLB/`) | Survival-frontier props: camps, fires, crates, tools, shelters across all 12 zones | GLB only; FBX/OBJ duplicates and previews dropped |
| `KENNEY_CITY_KIT_INDUSTRIAL` | City Kit (Industrial) 2.0 | Kenney (kenney.nl) | https://kenney.nl/assets/city-kit-industrial | CC0 1.0 Universal | 2026-09-04 | 38 GLB models (`ArtSource/Models/Kenney_CityKitIndustrial/GLB/`) | Industrial/research props — containers, cranes, pipes, warehouse shells (Ember Ridge, Stormcrest, research POIs) | GLB only |
| `KENNEY_MODULAR_SPACE_KIT` | Modular Space Kit 1.0 | Kenney (kenney.nl) | https://kenney.nl/assets/modular-space-kit | CC0 1.0 Universal | 2026-09-04 | 41 GLB modular tiles (`ArtSource/Models/Kenney_ModularSpaceKit/GLB/`) | Modular snapping sci-fi interiors — the 3 dungeons + Hollow Approach | Distinct from the classic Space Kit dressing pack (non-modular) |
| `KENNEY_MODULAR_DUNGEON_KIT` | Modular Dungeon Kit 1.0 | Kenney (kenney.nl) | https://kenney.nl/assets/modular-dungeon-kit | CC0 1.0 Universal | 2026-09-04 | 40 GLB modular tiles (`ArtSource/Models/Kenney_ModularDungeonKit/GLB/`) | Stone/ancient dungeon tiles — dungeon ruin segments + Sunscar ruins | GLB only |
| `KENNEY_ANIMATED_CHARACTERS_SURVIVORS` | Animated Characters: Survivors | Kenney (kenney.nl) | https://kenney.nl/assets/animated-characters-survivors | CC0 1.0 Universal | 2026-09-04 | 4 FBX (1 medium humanoid + idle/run/jump animations) (`ArtSource/Models/Kenney_AnimatedCharactersSurvivors/FBX/`) | NPC body candidate + locomotion/retarget reference | Pack ships no GLB (classic FBX + 2D-skin structure — corrected against the actual archive); 2D skins/SVG sources dropped; mesh/rig verified at engine import |
| `KENNEY_SKYBOXES` | Skyboxes | Kenney (kenney.nl) | https://kenney.nl/assets/skyboxes | CC0 1.0 Universal | 2026-09-04 | 5 equirectangular PNG sky textures (`ArtSource/Textures/Kenney_Skyboxes/PNG/`) | Alien sky dome art for 12-zone atmosphere variants | Sample renders and Preview.png excluded; import as long-lat in engine |
| `KENNEY_CROSSHAIR_PACK` | Crosshair Pack | Kenney (kenney.nl) | https://kenney.nl/assets/crosshair-pack | CC0 1.0 Universal | 2026-09-04 | 1,600 reticle PNGs (Dark/Glow/Light/Outline × standard/2×) (`ArtSource/Textures/Kenney_CrosshairPack/PNG/`) | Reticle art for the HudWidget hip-fire/aim states | Tilesheet atlases dropped (duplicates of individual PNGs); SVG variants and Preview excluded |

## Rules

1. Do not import an asset when the source page or license is unknown.
2. Do not commit standalone source asset packs when the license only allows use inside a project.
3. Keep Fab, CC0, CC-BY, and custom licenses clearly separated.
4. For CC-BY assets, record the required attribution text and include it in the project credits/readme if the project is shared.
5. Do not use names, models, sounds, logos, maps, or artwork from Pokémon, ARK, Palworld, Nintendo, Pocketpair, Studio Wildcard, or another game as project assets.
6. Store large source archives in the project Google Drive folder and keep the project-facing imported files under the Git LFS policy.

## Verified candidate sources

| Source | Intended use | Verification |
|---|---|---|
| Fab | Unreal environment/prop/animation packs | Review the specific Fab asset page and Fab Standard License before import. |
| Kenney | UI, icons, simple props and placeholders | Kenney support states game assets are CC0/public domain. |
| Poly Haven | HDRI, PBR textures, rocks and environment materials | Poly Haven states its assets are CC0; website content has separate restrictions. |
