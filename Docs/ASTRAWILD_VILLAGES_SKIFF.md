# ASTRAWILD — Living Villages, NPC AI & the Dawn Skiff (Batch 8)

> Status: **SOURCE COMPLETE — compile/playtest pending on the UE 5.8 target machine**
> (sandbox has no engine; see `ASTRAWILD_ENGINE_VERIFICATION_QUEUE.md` Batch 8 rows).
> Companions: `ASTRAWILD_BESTIARY_CODEX.md` (the 214-species roster) and
> `ASTRAWILD_ZONE_WORLD.md` (the 12-zone Grand Expanse).

## 1. หมู่บ้าน (Living Villages)

Two settlements, both procedural (engine basic shapes, zero assets):

| Village | Zone | Layout | Roster |
|---|---|---|---|
| **Dawnstead** | Dawn Fields (camp + 280m) | 7 huts, palisade ring, campfire, lamp posts, 6 waypoints | 8 NPCs — Warden Maren (quest), Trader Tam (shop), Herbalist Wren (shop), Blacksmith Borin (armory), Elder Rowan (quest 9), Guard Captain Sela + Guard Bram (guards), Farmer Jori |
| **Driftwood Landing** | Tidebreaker Isles (dry-spot search) | 3 huts, dock planks, campfire, 6 waypoints | 3 NPCs — Skiff Warden Kael (quest 10), Fisher Nima (shop), Old Salt Perry |

`AAstrawildVillageActor` builds the hamlet **and owns the waypoint circuit** its
NPCs patrol. `AAstrawildWorldBootstrapper::SpawnVillages()` spawns one village
actor + its NPC roster in a ring and links every NPC through `SetHomeVillage`.

## 2. NPC AI (AAstrawildNPCAIController)

Zero-asset C++ brain (same think-timer pattern as `AAstrawildEchoAIController`),
possessed automatically via `AIControllerClass` on the NPC character. Every NPC
carries a `UNavigationInvokerComponent` so navmesh tiles generate around villagers
anywhere (audit C-3 pattern).

| Behaviour | Detail |
|---|---|
| **เดิน-วิ่ง** | Waypoint circuit at walk speed (190 cm/s), idle pause 5–9 s per post; guards chase at run speed (430 cm/s) |
| **ตารางกลางวัน-กลางคืน** | 21:00–06:00 (world clock) every villager walks to the campfire and stays — the village visibly *lives* |
| **การ์ดสู้** | Guards aggro hostile wild Echoes within 35 m (iterates live Echoes, checks `bHostileToPlayers && !bCaptured && !IsDefeated`), chase, strike every 1.4 s for 14 damage through the standard `ApplyElementalDamage` pipeline; break off if the target flees >1.5× aggro range |
| **คุยแล้วหยุด** | Interacting pauses the AI 5 s and turns the NPC to face the player |
| **หน้าตาตาม role** | Procedural look — body capsule proportions per role (guards bulky, elders tall), head + hat cone, role-colored lantern point light; vendor tint blends from the definition |

NPC definitions (12) live in `UAstrawildContentLibrary::BuildNPCs` with the
Batch 8 additive fields: `Role`, `VillageId`, `PrimaryTint`, `Greeting`.

## 3. เครื่องบิน — Dawn Skiff (AAstrawildSkiffActor)

The Vale's first aircraft. Two are parked per world: **Skiff_Dawnstead** (camp
pad) and **Skiff_Driftwood** (isles dock).

| Input | Action |
|---|---|
| **E** ใกล้ลำตัว | ขึ้นเครื่อง (board) |
| **W / S** | บินหน้า-ถอยหลัง (thrust) |
| **A / D** | เลี้ยวซ้าย-ขวา (yaw) |
| **SPACE** | ร่อนขึ้น (ค้างไว้) |
| **CTRL** | ร่อนลง (ค้างไว้) — gamepad: LS click |
| **SHIFT** | resonance boost (cruise 14 m/s → boost 26 m/s) |
| **E** ระหว่างบิน | ลงจากเครื่อง (dismount ข้างลำตัว) |

Technical notes:

- **Server-authoritative movement** (listen-server inline policy — same as the
  dungeon portals; dedicated-client RPCs arrive with the H-9 multiplayer batch).
- Pilot attaches to the hull (movement disabled, capsule collision off) — mouse
  look keeps orbiting freely (third-person free look while flying).
- Altitude clamps: terrain floor +220 cm hover minimum, ground +120 m ceiling;
  sweeps stop the hull on contact and kill the vertical axis so it slides.
- Pure flight math is `ComputeSkiffVelocity` (static, automation-tested).
- Silhouette: hull + nose cone + tail fin + twin pontoons + bow light.

## 4. ทะเล (The Sea)

Three zones sit below the global sea level (`GetSeaLevelZ() = −450 cm`): Azure
Shallows, Tidebreaker Isles, Pearlsea Reef. `AAstrawildWaterPlaneActor` renders a
walkable, vertex-colored blue surface per sea zone (+8 km blend padding) — a
stylized shallow sea. Terrain tint adds a beach band above the waterline and a
deep-blue shelf below it. **Real swimming** (buoyancy/volumes) is queued for a
later batch — the skiff is the fast lane, the sea floor is walkable in the
meantime.

## 5. Quest chain (Batch 8 tail)

```
Quest 8  The Vale Beyond
   └── Quest 9  Wings over the Vale   (Elder Rowan, Dawnstead)
         • VisitZone  Zone_TidebreakerIsles
         • ReachLocation Location_DriftwoodLanding  (survey marker, publish-only pad)
         └── Quest 10 The Sunken Vault  (Skiff Warden Kael, Driftwood Landing)
               • ReachLocation Location_SunkenVault
               • DefeatCreature Creature_VaultColossus  (the Dawnfang sea-dragon)
```

The Sunken Vault (dungeon #2) generates deep in the Isles with the bestiary's
**Dawnfang** (Dragon/Serpent/Large) as boss; its defeat publishes the distinct
`Creature_VaultColossus` id via the new per-dungeon `BossDefeatEventId`.
Rewards: 25 RP + pearls/coral/shards. The Ancient tech era stays exclusive to the
Hollow Underlight (vault grants research points only).

## 6. Verification checklist (target machine)

1. Walk Dawnstead at dawn → villagers stroll the circuit; talk to one → they stop
   and face you; wait for 21:00 → everyone gathers at the campfire.
2. Drag a hostile Echo near the village (cheat/aggro) → guards sprint over and
   fight it through the standard damage pipeline.
3. Board the Skiff at the camp pad → WASD/SPACE/CTRL/SHIFT fly; E dismounts;
   ceiling/floor clamps hold; hull stops on cliffs (sweep).
4. Fly west-south across the Azure Shallows to the Tidebreaker Isles → zone
   banner fires, quest 9 VisitZone completes; land at Driftwood Landing → chart
   the survey marker (E) → ReachLocation completes; quest 10 offers from Kael.
5. Descend into the Sunken Vault → 4 rooms + Dawnfang boss (stats from the
   bestiary row: HP 550-scale via BossHealthScale) → defeat → quest 10 completes.
6. Every Echo in the world renders a distinct procedural silhouette (200+
   species across 8 body plans × 5 size classes × per-species tints).
