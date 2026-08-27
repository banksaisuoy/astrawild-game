# ASTRAWILD Feature Roadmap

## Product direction

ASTRAWILD should feel like a **real-time cooperative creature survival action game**. The distinctive loop is: read the environment and element, fight with responsive melee and Echo abilities, capture a weakened creature, use its role at the camp, craft the next tool, and return to a more dangerous region. The project should borrow category strengths without copying any franchise's characters, names, maps, art, sound, or proprietary systems.

## Stage 0 — Make the current Vertical Slice fun

The first playable target is one compact map with four readable zones, a player, three Echo species, one Alpha boss, and a 20–30 minute loop. The minimum success path is movement and dodge, harvest, craft one Resonator, fight a wild Echo, exploit a type advantage, capture it, summon it, use one role action at camp, defeat or survive Solarix Alpha, and save/load the result.

The priority is not the number of systems. It is **moment-to-moment clarity**: hit feedback, capture tension, readable telegraphs, fast inventory interaction, and meaningful decisions about which three Echoes to bring.

## Stage 1 — MVP expansion after the slice passes

| System | Scope | Acceptance gate |
|---|---|---|
| Creature progression | levels, stat growth, 3–5 passive abilities, role perks | a captured Echo has a meaningful build choice |
| Survival pressure | hunger, thirst, weight, day/night and one weather hazard | survival changes route/camp decisions without busywork |
| Base utility | one production task per role, station queue, storage | a companion visibly helps produce a needed item |
| Equipment | two weapon types, basic armor, durability and repair | combat and resource choices affect loadout |
| Quest flow | imported Lore/Quest DataTables, objective tracker, rewards, save | one main quest and two side quests complete end-to-end |
| Boss content | Solarix Alpha two phases, rewards, quest progression | the encounter is readable and repeatable in PIE |

## Stage 2 — Co-op foundation

Implement server-authoritative inventory, damage, capture, crafting, building and quest progression. Add session create/join, reconnect policy, party replication, Echo ownership replication and a two-client test. Keep the first co-op world small and avoid dedicated-server optimization until a listen-server loop is stable.

## Stage 3 — Long-term systems

Breeding and inheritance should follow stable species and trait IDs. Evolution should be a controlled transformation with explicit conditions and save migration. Mounts should share a movement/seat contract with Echo animation and replication. A technology tree should unlock recipes and camp capabilities rather than become a separate grind layer. Weather, raids, guilds, larger world streaming and additional biomes should follow only after the core loop has retention evidence.

## Cut line for the next milestone

Do not add breeding, evolution, mounts, raids, guilds, full weather simulation, or a technology tree to the current Vertical Slice. If the current slice cannot pass a clean 20-minute playtest with three Echoes and Solarix Alpha, adding more systems will hide the real problem rather than improve the game.
