# ASTRAWILD Next-Gen Expansion Roadmap

**Scope:** Deep-sea, space flight, cosmetics, vocalization, guild warfare, fishing and racing expansion from the attached production brief.

> **Current decision:** Protect the compact 20–30 minute vertical slice first. Pillar 1 has a repository-side depth/pressure/oxygen/buoyancy source contract, and Sprint 2 now adds source contracts for space flight, guild/arena rules, and 16 cosmetic dyes. All new systems remain staged until their native systems have explicit gameplay ownership, save/network design and Windows UE evidence.

## Expansion gates

Every pillar must pass four gates in order: source contract, data/import contract, Unreal Editor asset integration, and runtime/network acceptance. A class skeleton or CSV row alone is not a complete feature.

| Gate | Required result |
|---|---|
| Source contract | Native types, authority rules, save schema impact, failure/rollback behavior and regression validator |
| Data/import contract | CSV/DataAsset schema, row validation, importer mapping and stable Gameplay Tags |
| Editor integration | `.uasset`/`.umap`, materials, meshes, animation, Niagara, audio, UMG, collision and navigation assigned in UE 5.8 |
| Runtime acceptance | PIE first-loop regression, save/load, latency/authority tests where applicable, profiling and package smoke test |

## Pillar 1 — Underwater Abyssal Biome

**Repository status:** Source/data contract added.

`FAstrawildUnderwaterZoneRow` and `DT_UnderwaterZones.csv` define the `Underwater.AbyssalTrench` row at 100–1000 meters, pressure damage, oxygen multiplier, buoyancy and hazard/spawn tags. `UAstrawildUnderwaterSubsystem` provides clamped depth evaluation, pressure-damage calculation, depth-sensitive oxygen drain, gradual surface refill, buoyancy multiplier, emergency oxygen state and optional zone-table lookup.

The next implementation boundary is a CharacterMovement/water-volume integration component that owns per-diver state on the server. It must not store authoritative oxygen in a world subsystem shared by all players. Required Editor work includes water volume, underwater camera/post-process, swimming/free-dive movement, pressure-suit/mecha insulation, oxygen UI, damage routing, aquatic navigation/AI, coral/vent assets and four submerged base actors: pressurized dome, airlock, generator core and aquarium display.

**Acceptance:** Two players can enter/leave the water independently, oxygen drains/refills correctly, pressure damage is authoritative, save/load preserves equipment/state, and the subsystem does not create duplicate actors or mutate client-owned state.

## Pillar 2 — Space Flight and Lunar Zenith

**Repository status:** Source contract added in `UAstrawildSpaceFlightSubsystem`, `FAstrawildSpaceFlightData` and `AAstrawildLaunchPad`. The subsystem owns launch-pad registration, server-side proximity checks, launch progression, low-gravity scale, per-pilot flight state and vacuum pressure emergency state.

**Required integration boundary:** `UAstrawildSpaceFlightSubsystem` should remain a deterministic world service; a `UAstrawildSpaceFlightComponent` or CharacterMovement extension must own per-pawn movement state. `AAstrawildLaunchPad` validates a launch request but does not own fuel, travel persistence or final flight physics. Fuel must be reserved and consumed atomically by the owning inventory/travel system only after a successful launch reservation.

**Data contract to add:** `DT_SpaceZones.csv` with zone tag, gravity scale, atmosphere/pressure, oxygen drain, radiation, launch requirement and return destination. Add `DT_LaunchProfiles.csv` for pad type, fuel item, countdown, passenger capacity and failure refund. The current source contract exposes tags and tuning but does not yet claim these future tables. Add tags under `Space`, `Hazard`, `Launch` and `Gravity` only after the row structs and validator exist.

**Editor work:** Lunar landscape/map, zero-G traversal volumes, grappling sockets, asteroid-field Niagara, helmet/exosuit materials, launchpad Blueprint, travel loading flow and navigation volumes.

**Acceptance:** A server-controlled launch with a failed-launch refund, 0.16G movement profile, vacuum hazard protection, reconnect-safe travel checkpoint and a return path works in PIE and two-player Network PIE.

## Pillar 3 — Echo Cosmetics and Dye Customization

**Repository status:** `FAstrawildDyeRow` and `DT_Dyes.csv` are now present with exactly 16 original colors, primary/secondary linear tints, material parameter names, unlock tags, craft costs and default-unlock flags.

**Required integration boundary:** A future `UAstrawildCosmeticSubsystem` should be a registry/query service, not the owner of replicated appearance. The owning Player/Echo appearance component should replicate compact cosmetic IDs and dye indices, validate ownership/unlock state on the server, and apply material parameters locally through a presentation layer.

**Data contract to add:** `DT_CosmeticSkins.csv` with cosmetic ID, target family, material slot contract, unlock condition and original asset path; extend `DT_Dyes.csv` later with rarity, recipe and allowed target masks if the final equipment contract requires them. Avoid storing arbitrary client-provided colors; use a server-approved dye row.

**Editor work:** 50 original overlay material instances, master material parameters, preview UI, dye crafting recipes, icons, thumbnail capture and per-family material slot verification.

**Acceptance:** A client cannot equip an unowned skin/dye, replicated appearance is deterministic after join/save/load, material parameter changes do not alter gameplay stats, and all 16 dyes render correctly under the selected scalability presets.

## Pillar 4 — Creature Vocalization and Audio

The audio generator may produce original source WAVs, but generated files are not a substitute for authored Sound Waves, Sound Cues, attenuation, concurrency, animation-notify routing or mix. Keep creature voices separated by species/variant/emotion and avoid recognizable melodies or protected sounds.

**Data contract to add:** `DT_VocalizationProfiles.csv` with species tag, vocalization event, source Sound Wave path, Sound Cue path, cooldown, attenuation class, pitch range and combat/ambient usage. The audio registry should resolve a stable event ID and fail safely when an asset is missing.

**Editor work:** 50+ original vocalization source files, Sound Cues, random/weighted variations, distance attenuation, occlusion, underwater filtering, mecha beep layer, combat/capture notifications and zone ambience routing.

**Acceptance:** No duplicate event spam under rapid combat events, voice variation remains within the configured profile, underwater low-pass/reverb is audible, and missing assets produce a logged fallback rather than a crash.

## Pillar 5 — Guild Territory and 4v4 Arena

**Repository status:** Source contract added in `UAstrawildGuildSubsystem`, `FAstrawildGuildData` and `AAstrawildGuildTotem`. The current contract covers authority-gated guild registration, prerequisite-gated buff levels, territory capture radius checks and a two-team/four-member arena roster/match state.

**Required integration boundary:** A persistent guild service must still own guild identity, membership, roles, bank transactions, territory claims, technology unlocks and audit records. `AAstrawildGuildTotem` must be extended with contested state and cooldown rules before live-like warfare. Do not put guild bank authority in a client inventory component.

**Data contract to add:** `DT_GuildTechNodes.csv` for shared buffs and prerequisites; `DT_TerritoryRules.csv` for claim radius, biome restrictions, raid windows and defense modifiers; `DT_ArenaRules.csv` for team size, bracket timeout, Echo restrictions, scoring and reward tags. The current source structs are runtime registration contracts, not replacements for these authored rule tables.

**Editor/runtime work:** Guild UI, bank transaction UI, territory projection/replication, claim-totem actor, raid rules, matchmaking service boundary, 4v4 arena map, bracket state, spectator/respawn rules, podium and trophy assets.

**Acceptance:** Server-side bank transactions are atomic and logged, territory conflicts resolve deterministically, a 4v4 match cannot be started with invalid teams, disconnect/rejoin behavior is defined, and ranked rewards cannot be duplicated.

## Pillar 6 — Deep Fishing and Echo Grand Prix

**Repository status:** Fishing source contract and `DT_FishDex.csv` are complete for the 30-row bait/depth/tension/reward catalogue. Racing source contract is complete for server-owned track/checkpoint/lap/boost state. Both still require their authored interaction actors, movement bridges and UI.

**Required integration boundary:** Fishing must use a server-owned session state machine: cast, bite window, tension, line failure, catch reservation and reward commit. Racing must use server-owned track/checkpoint state, signed lap times and deterministic boost-pad/checkpoint validation. Do not trust client-reported catch species, drift distance or lap time.

**Data contract to add:** `DT_FishDex.csv` is now present with 30 species, habitat/depth, bait tags, tension profile, rarity and reward items. Still add `DT_FishingRods.csv` for cast range, line strength and reel speed, and `DT_RaceTracks.csv` for checkpoint order, mode, boost pads, lap count, allowed mounts and leaderboard rules.

**Editor work:** Fishing volumes, original fish meshes/animation, rod/bait assets, tension UI, underwater catch VFX/audio, ground/aerial track maps, checkpoint actors, boost-pad Niagara, drift presentation, leaderboard UI and trophy assets.

**Acceptance:** A failed fishing session returns uncommitted bait correctly, a catch commits once, invalid checkpoints reject a lap, reconnect state is safe, and leaderboard times cannot be fabricated by a client.

## Delivery order

| Priority | Work package | Why |
|---:|---|---|
| 1 | Finish Pillar 1 CharacterMovement/water-volume integration | Builds on the new source contract and directly extends exploration without destabilizing the first loop |
| 2 | Integrate Pillar 3 dyes | The 16-row source table is present; remaining work is presentation, material slots, save and cosmetic replication |
| 3 | Pillar 4 vocalization/audio | Can be integrated incrementally after event IDs and audio routing are stable |
| 4 | Integrate Pillar 2 space flight/lunar travel | Source contract is present; movement, travel/save/network design and Lunar map evidence remain |
| 5 | Integrate Pillar 6 fishing/racing | Source contracts are present; rods, race rules, actors, movement/UI and significant Editor content remain |
| 6 | Integrate Pillar 5 guild/territory/4v4 | Source contract is present; persistence, exploit prevention, matchmaking and live-service complexity still require co-op evidence |

## Explicit non-claims

This roadmap does not claim that the Sprint 2 source contracts are playable Editor features. It does not claim final visual assets, animation, Niagara graphs, UMG screens, multiplayer services, travel persistence or packaged builds. Windows must still compile the native module, import all 38 DataTables, author the required actors/assets and create PIE/Network PIE evidence before the features are considered complete.
