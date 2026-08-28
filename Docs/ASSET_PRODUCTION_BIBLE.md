# ASTRAWILD Asset Production Bible

**Project:** ASTRAWILD: Echoes of the First Dawn
**Target:** Unreal Engine 5.8 / Windows PC
**Asset policy:** Original ASTRAWILD IP only. No copied characters, models, maps, textures, UI, effects, music, sounds, terminology, or visual signatures from external games or franchises.

## Production boundary

This pass generates reproducible source assets that can be reviewed, imported and replaced in Unreal Editor. OBJ files are static source meshes, not skeletal meshes with skin weights. WAV files are generated source audio, not final mastered music or complete Sound Cue graphs. `.uasset`, `.umap`, AnimBP, Niagara graphs, materials, retargeted animation, Control Rig and final Widget Blueprints remain Editor deliverables and must not be claimed as complete until Windows UE 5.8 evidence exists.

## Visual direction

ASTRAWILD uses a dawn-ecology visual language: readable silhouettes, asymmetrical natural forms, mineral and plant growth, soft bioluminescent accents, and restrained technology motifs derived from the fictional Astra resonance. Solar forms use warm amber and pale gold; Torrent forms use cyan and deep teal; Geo forms use ochre, moss and slate; Aether/Astra forms use violet-white and glassy blue. Technology is integrated as original field equipment and resonance lattice, not as a recognizable real-world or franchise mecha silhouette.

The asset pipeline uses deterministic variation keyed by each row's stable tag. This preserves identity between regeneration runs and avoids palette-only duplication. Each Echo receives a distinct body plan, crest or sensory feature, locomotion hint, element accent and material grouping derived from its DataTable row.

## Asset tiers

| Tier | Contents | Acceptance purpose |
|---|---|---|
| P0 playable slice | Player placeholder body, Pyrelite, Thornback, Aquavine, Solarix Alpha, 9 map props, 4 zone kits, camp/station pieces, capture/projectile presentation and core SFX | Prove the first 20–30 minute loop in a compact map |
| P1 content breadth | All 30 legacy Echo rows, boss/tower visual variants, 21 mount profiles, weapons, resource families, building kit and environmental dressing | Prove DataTable coverage and recognizable collection breadth |
| P2 catalogue coverage | All 200 Master Echo rows as deterministic low-poly source silhouettes, with per-row manifest records and generation seed | Ensure every catalogue row has a stable asset placeholder before bespoke art production |
| P3 final art | Original high-quality skeletal meshes, UVs, materials, textures, LODs, rigs, animation sets, VFX graphs and audio mix | Replace source placeholders in Windows Editor and validate runtime performance |

## Character and Echo asset naming

| Asset family | Naming rule | Example |
|---|---|---|
| Player body | `SK_Player_AstralSurveyor` | `SK_Player_AstralSurveyor.fbx` |
| Echo skeletal mesh | `SK_Echo_<SpeciesTagWithoutPrefix>` | `SK_Echo_Pyrelite.fbx` |
| Echo source placeholder | `SM_Echo_<SpeciesTagWithoutPrefix>_Source` | `SM_Echo_Terradon_Source.obj` |
| Echo AnimBP | `ABP_Echo_<SpeciesTagWithoutPrefix>` | `ABP_Echo_Aquavine` |
| Echo animation profile | `DA_AnimProfile_<SpeciesTagWithoutPrefix>` | `DA_AnimProfile_Pyrelite` |
| Alpha boss | `SK_Alpha_Solarix` / `ABP_Alpha_Solarix` | `SK_Alpha_Solarix.fbx` |
| Map kit | `SM_<Zone>_<Purpose>` | `SM_DawnSpire_Platform` |
| Audio SFX | `SFX_<Domain>_<Action>` | `SFX_Mecha_Overboost` |
| Ambient | `AMB_<Zone>_<State>` | `AMB_DangerPit_Combat` |
| Music | `MUS_<Context>_<Intensity>` | `MUS_DangerPit_Encounter` |

## Required P0 map kit

The compact map must contain four authored visual zones: Dawn Spire start landmark, Resource Grove, Rest Sanctuary and South-East Danger Pit. It needs a navigable ground surface, blocking volumes, a small climb/cover route, spawn volumes, a combat arena boundary, a camp interaction cluster, a water spring, resource nodes, an Echo encounter route, a boss arena and a return/reward path. The source bootstrap already supplies stable actor IDs and placeholder actor intent; Editor work must assign meshes, materials, navigation, collision, lighting and DataTable references.

## Audio coverage

The source pass covers core interaction SFX first. The full audio pack must additionally cover player movement and dodge, harvest for wood/ore/fiber, crafting, inventory, capture throw/struggle/success/failure, Echo vocal stingers by element, camp/building placement, cooking/spoilage, level-up and technology unlock, weather ambience, four zone ambiences, danger telegraphs, boss phase transitions, mount movement, exosuit flight/overboost/beam/plasma/shutdown and UI feedback. Music must be instrumental and original, with separate exploration, sanctuary, danger and boss contexts plus non-looping victory/defeat stingers.

## Unreal acceptance rule

Every imported asset must have a stable path, a source row or manifest record, assigned collision/material or attenuation settings, and at least one Editor screenshot or Output Log record. A generated OBJ/WAV manifest proves source coverage only. It does not prove that a DataTable row references a valid `.uasset`, that an AnimBP compiles, that Niagara renders, that a Sound Cue mixes correctly, or that a packaged build contains the asset.
