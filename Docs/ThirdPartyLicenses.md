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
