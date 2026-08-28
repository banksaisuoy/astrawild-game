# ASTRAWILD Compact Vertical Slice Map — 20–30 Minute Test Specification

**Target:** Unreal Engine 5.8
**Map purpose:** ทดสอบ core gameplay loop แบบเล่นคนเดียวให้จบหนึ่งรอบ ก่อนขยายไปสู่โลกใหญ่และ multiplayer
**Recommended map package:** `/Game/Astrawild/Maps/VerticalSlice/L_Astrawild_VS_20Min`
**Recommended gameplay area:** ประมาณ 80 m × 80 m หรือ 8,000 cm × 8,000 cm
**Source reference:** `AAstrawildPrototypeArena`, `FEATURE_ROADMAP.md`, `M1_WORLD_PARTITION_HANDOFF.md`, `P3_Alpha_Echo_Design.md`, `P4_Lore_Quest_Data_Contract.md`, `Crafting_And_Building_Spec.md`

## 1. Definition of success

แผนที่นี้ไม่ใช่แผนที่ Open World เต็มขนาด 4.096 km แต่เป็นสนามทดสอบที่บีบเส้นทางหลักให้ผู้เล่นสามารถเข้าใจระบบและจบลูปแรกได้ภายใน 20–30 นาที เป้าหมายคือให้ผู้เล่นทำเส้นทางต่อไปนี้ได้จริงใน PIE:

> **เกิดที่ Dawn Spire → รับสัญญาณและภารกิจ → เก็บทรัพยากร → เตรียมเครื่องมือ/คราฟต์ Astra Resonator → ต่อสู้กับ Echo → ทำให้ Echo อ่อนแรงและจับ → เรียก Echo ออกมาใช้งาน → กลับ camp เพื่อคราฟต์/พัก/จัดการ survival → เข้าสู่ Danger Pit → อ่าน telegraph และเอาชนะ Alpha Echo → รับ quest reward → save/load แล้วรักษาสถานะเดิม**

ความสำเร็จของแผนที่วัดจากการจบลูป ไม่ใช่จำนวนอาคารหรือพื้นที่ที่สร้างได้ หากระบบใดมี source contract แต่ยังไม่มี actor, DataTable asset, Widget Blueprint, animation, Niagara, audio หรือ runtime evidence ระบบนั้นยังไม่ถือว่าเสร็จสำหรับ vertical slice

## 2. Existing prototype versus required slice

`AAstrawildPrototypeArena` มี source bootstrap สำหรับสนามทดสอบ ได้แก่ foundation 80 m × 80 m, central Dawn Spire, resource grove, South-East combat pit, North-East rest sanctuary, ต้นไม้ 3 จุด, ore 2 จุด, Dawn Fiber 3 จุด, training dummy, Pyrelite, Thornback, Solarix Alpha, campfire, bed, crafting bench, Storage Chest, Aquavine และ spring interactable อย่างไรก็ตาม implementation ปัจจุบันยังเป็น prototype runtime generation ไม่ใช่ final `.umap`; final Blueprint/DataAsset bindings, authored quest triggers, visual assets และ PIE evidence ยังต้องทำใน Editor

| รายการ | สถานะจาก source ปัจจุบัน | สิ่งที่ต้องทำสำหรับ 20–30 นาที loop |
|---|---|---|
| พื้นที่ 80 m × 80 m และ 4 โซน | มีใน `AAstrawildPrototypeArena` | ย้ายหรือประกอบเป็น map package จริงใน Editor และตรวจ collision/navmesh |
| Dawn Spire และ signal interactable | มี monolith interactable และ reward Resonator ใน prototype | ผูก `Quest.Awakening`, `Location.DawnSpire` และ `Interact.DawnSignal` กับ imported quest tables |
| Sunwood trees | มี 3 harvest nodes พร้อม stable `NodeUniqueId`, yield และ respawn settings | ตรวจ mesh, collision, save/load และ depletion/respawn behavior ใน Editor |
| Lumen Stone/Astra Shard rocks | มี 2 mining nodes พร้อม stable `NodeUniqueId`, yield และ rare-drop settings | ตรวจ mesh, collision, tool requirement และ yields ให้พอสำหรับสูตรที่ใช้ใน slice |
| Dawn Fiber | มี 3 foraging nodes ใน source bootstrap | ตรวจ mesh/collision/IDs ใน Editor และยืนยัน yield เพียงพอสำหรับ Rest Bed |
| Player tools | Character เริ่มด้วย Sunwood 10 และ Resonator 5 แต่ยังไม่เห็น starter Axe/Pick ใน `BeginPlay`; harvest interaction ใช้ค่า default | ตัดสินใจ acceptance profile ให้ชัด: grant Axe/Pick/Club หรือคง default gather; ห้ามให้ Resonator ฟรีหากต้องทดสอบ craft objective |
| Astra Resonator craft | Crafting spec ต้องใช้ Astra Shard 1, Lumen Stone 2, Sunwood 3 ที่ Crafting Bench; prototype ยังแจกจาก Player/Spire | ใช้ acceptance profile ที่ไม่แจก Resonator ฟรี และผูก Craft objective กับ imported DataTable; แยก quick-test profile ออกจาก acceptance profile |
| Pyrelite/Thornback | มี wild actors ใน combat pit | สร้าง Blueprint/DataAsset/mesh/AnimBP จริง, ตั้ง capturable flags และ capture feedback |
| Aquavine | มี actor และ spring interactable ใน source bootstrap | ตรวจ `Location.AquavineSpring`, `Item.Water`, Discover/Collect progression และ hydration restore ใน PIE |
| Campfire/Bed/Bench/Chest | มีทั้ง 4 building actors ใน source bootstrap พร้อม stable IDs | ตรวจ interaction, container, collision, save/load และเปลี่ยนเป็น authored assets ใน Editor |
| Solarix Alpha | มี `AAstrawildAlphaEcho` source bootstrap พร้อม 6 abilities และ 2 phase pattern arrays | สร้าง `BP_Echo_SolarixAlpha` + `DA_Echo_SolarixAlpha` ที่ `Location.DangerPit.AlphaSpawn`, ผูก telegraph/VFX/audio และทดสอบ reset/reward |
| Quest progression | มี CSV/source component contract และ generic interactable รองรับ objective type/reward collect | สร้าง imported DataTables, bind Reach/Interact/Discover/Craft/Capture/Defeat triggers และตรวจ Awakening → FirstResonator → DangerPit |
| Food/water survival | Spring source เรียก `DrinkWater` และแจก `Item.Water`; cooking/food source ยังไม่ครบ | เพิ่ม deterministic food source หรือกำหนด test profile ให้ชัด; ตรวจ survival drain/restore ใน PIE และไม่ถือว่า component compile เป็น gameplay proof |

## 3. Physical layout

ใช้ actor location ของ `AAstrawildPrototypeArena` เป็น `Origin` และเก็บ coordinate เป็นเซนติเมตรตาม Unreal units จุดด้านล่างเป็น authoring guide; หากใช้ World Partition ให้สร้าง map marker/volume ที่สะท้อนตำแหน่งเดียวกัน ไม่สร้าง coordinate system ซ้ำอีกชุด

| Zone | Approx. coordinate from Origin | Function | Required landmarks |
|---|---:|---|---|
| Central Dawn Spire | `(0, 0, 0)` | onboarding, quest start, readable navigation landmark | player start, elevated dais, signal monolith, first interaction prompt, HUD/tutorial trigger |
| North-West Resource Grove | `(-1,400, 800, 0)` | resource acquisition, tool test, safe first exploration | 3 Sunwood nodes, 2 Lumen/Astra nodes, at least 3 Dawn Fiber nodes, ramps/ledges, one readable return route |
| South-East Combat Approach | `(1,400, -1,200, -100)` | training, elemental combat and first capture | training dummy, Pyrelite, Thornback, cover rock, capture-safe edge, entry trigger to Danger Pit |
| South-East Danger Pit | `(1,400, -1,200, -100)` | Alpha encounter and quest climax | boss spawn marker, 26 m leash, 3 approach lanes, 2 line-of-sight rocks, safe reset edge, open telegraph floor, phase lighting/audio anchors |
| North-East Rest Sanctuary | `(1,400, 1,200, 200)` platform top around `Z=420` | camp utility, survival reset, crafting, summon/role test, save/load | campfire, Rest Bed, Crafting Bench, Storage Chest, Aquavine, spring/water interaction, save checkpoint |

### 3.1 Central Dawn Spire

วาง `PlayerStart` ประมาณ `(0, -650, 100)` หรือจุดที่มองเห็น monolith ได้ทันที โดยต้องไม่ spawn บน collision ของ dais ให้มีทางแยกที่มองเห็นได้ไปยัง Resource Grove, Combat Approach และ Rest Sanctuary ใช้ monolith เป็นทั้ง landmark และ interaction actor ไม่ใช่เพียง static mesh

เมื่อผู้เล่น interact กับ signal ให้เริ่ม `Quest.Awakening` หรือ complete objectives `ReachSpire` และ `ObserveSignal` ตามลำดับที่ quest component กำหนด อย่าให้ reward item เป็นวิธีเดียวในการยืนยันว่า quest ทำงาน เพราะ reward ที่เกิดจาก prototype อาจทำให้ผู้เล่นข้าม progression ได้

### 3.2 North-West Resource Grove

วางต้นไม้ตามแนวประมาณ `(-1,000, 500)`, `(-1,300, 750)` และ `(-1,600, 1,000)` ซึ่งสอดคล้องกับ prototype เดิม แต่ต้องเปลี่ยนจาก anonymous runtime nodes เป็น actors ที่มี `NodeUniqueId` คงที่ เช่น `VS_Grove_Sunwood_01` ถึง `VS_Grove_Sunwood_03` วาง rocks ประมาณ `(-1,300, 1,100)` และ `(-1,700, 700)` พร้อม IDs `VS_Grove_Lumen_01` และ `VS_Grove_Lumen_02`

เพิ่ม Dawn Flower/Fiber nodes ระหว่างทางอย่างน้อย 3 จุด เช่น `VS_Grove_Fiber_01` ถึง `VS_Grove_Fiber_03` เพื่อไม่ให้ Rest Bed recipe มี dependency ที่ผู้เล่นหาไม่ได้ ควรมีป้าย/สี/รูปทรงแยกชัดเจนระหว่าง Lumber, Mining และ Fiber และจัด resource cluster ให้ผู้เล่นเก็บได้ใน 2–4 นาทีโดยไม่ต้องวิ่งค้นทั่ว map

### 3.3 Combat Approach

วาง training dummy ที่ผู้เล่นเห็นก่อนเข้าพื้นที่ boss และวาง Pyrelite กับ Thornback คนละมุมเพื่อให้เกิดการเลือกเป้าหมายและอ่านธาตุ ไม่ควร spawn ทั้งสองตัวซ้อนกันหรือบังคับให้ผู้เล่นสู้พร้อมกันตั้งแต่ครั้งแรก เป้าหมายของช่วงนี้คือให้ผู้เล่นทดลอง melee, dodge, status/element feedback, ลด HP ของ Echo และจับ Echo อย่างน้อยหนึ่งตัว

ต้องมีพื้นที่ว่างรอบเป้าหมายสำหรับ capture projectile, failure enraged state และ recall/summon companion อย่าวาง foliage หรือ collision volume บัง interaction trace ระยะประมาณ 350 cm ของผู้เล่น

### 3.4 Danger Pit และ Alpha Arena

ใช้ arena ตาม `P3_Alpha_Echo_Design.md`: boss spawn ที่ `Location.DangerPit.AlphaSpawn`, leash 26 m, approach lanes 3 ช่อง, line-of-sight rocks 2 ก้อน, safe reset edge 1 ด้าน และ open floor ที่เห็น telegraph ได้ครบ ให้ `BP_Echo_SolarixAlpha` เป็น actor หลัก ไม่ใช้เพียง wild Echo ที่เปลี่ยนชื่อใน Level

วาง trigger `BV_DangerPit_Entry` ที่ขอบทางลาด เพื่อ complete `Objective_DangerPit_Enter` และเริ่ม intro state เฉพาะเมื่อ `Quest.FirstResonator` complete แล้ว หากเข้า pit ก่อนเวลา ให้แสดง objective/lock state ที่ชัดเจนโดยไม่ spawn boss ซ้ำ

Phase One ต้องสอน timing ด้วย `SolarClaw`, `EmberLine` และ `DawnRoar` ส่วน Phase Two ต้องเพิ่ม `SolarNova`, `FlareRing` และ `AshenRush` ตาม data/design contract บริเวณ cover ต้องช่วยให้ผู้เล่นเข้าใจ counterplay ไม่ใช่ทำให้ AI หายจาก navigation หรือทำให้ camera มองไม่เห็น telegraph

### 3.5 Rest Sanctuary

วาง campfire, Rest Bed และ Crafting Bench บนพื้นที่ plateau เดียวกันแต่มี interaction spacing เพียงพอสำหรับ camera และ prompt ไม่ให้ trigger ซ้อนกัน เพิ่ม Storage Chest เพื่อทดสอบ container persistence และวาง Aquavine ใกล้ spring/water interaction ที่มี `Location.AquavineSpring` เป็น stable target

หากต้องการให้ลูปแรกมี survival pressure ให้กำหนด food/water test data ที่ชัดเจน เช่น starter consumable หรือ recipe ที่ถูก import แล้ว มิฉะนั้นให้ตั้งค่าทดสอบแบบไม่ลด hunger/thirst จนกว่าจะมี source ที่ผู้เล่นหาได้จริง อย่าปล่อยให้ผู้เล่นเสียเลือดจากระบบที่ไม่มีทางฟื้นใน map

## 4. Required actors and content assets

### 4.1 Level actors and systems

| Category | Required objects |
|---|---|
| Map/runtime | `L_Astrawild_VS_20Min`, `WorldSettings`, `GameMode`, `GameState`, `PlayerStart`, `AAstrawildPrototypeArena` only as temporary fallback or authored replacement |
| Navigation | `NavMeshBoundsVolume` ครอบคลุมพื้นที่ใช้งาน, navigation generation สำหรับ ramps/stairs/pit, blocking volumes กันตกนอก arena |
| Quest triggers | Dawn Spire reach trigger, Dawn Signal interactable, Resource Grove region trigger, Danger Pit entry trigger, Aquavine Spring discover/collect trigger |
| Player | Player Blueprint derived from `AAstrawildCharacter`, input mapping, camera, final skeletal mesh/AnimBP, HUD and interaction prompt |
| Echoes | `BP_Echo_Pyrelite`, `BP_Echo_Thornback`, `BP_Echo_Aquavine`, `BP_Echo_SolarixAlpha`, corresponding DataAssets, collision, AI/controller and animation assets |
| Resources | 3 Sunwood nodes, 2 Lumen/Astra nodes, 3 Dawn Fiber nodes, optional water/food source, stable IDs and save settings |
| Camp | Campfire, Rest Bed, Crafting Bench, Storage Chest, optional cooking/water station, stable `BuildingUniqueId` values |
| Feedback | melee hit VFX/SFX, elemental advantage feedback, capture throw/success/failure, dodge, crafting success/failure, boss telegraphs, phase transition and defeat reward |
| UI | Main HUD, inventory grid, crafting menu, interaction prompt, quest/objective tracker, survival meters, companion strip and boss/target state |
| Save | at least one explicit save checkpoint in sanctuary, one manual save path, one reload path, stable IDs for nodes/buildings/quests and no duplicate runtime-spawned actors after load |

### 4.2 Mandatory imported data

The 32-table importer may import the complete source package, but the compact map needs a smaller mandatory subset to be functionally testable.

| Table | Why the slice needs it |
|---|---|
| `DT_Biomes.csv` | map region/temperature/resource metadata |
| `DT_SpawnRules.csv` | controlled Echo spawn rules if actors are not entirely hand-placed |
| `DT_EchoDex.csv` and/or the selected Echo DataAssets | Pyrelite, Thornback, Aquavine and Alpha identity/stats/roles |
| `DT_Lore.csv` | Dawn Signal and first narrative context |
| `DT_Quests.csv` | `Quest.Awakening`, `Quest.FirstResonator`, `Quest.DangerPit`, optional `Quest.Campwater` |
| `DT_QuestObjectives.csv` | Reach/Interact/Craft/Capture/Defeat/Discover/Collect objectives |
| `DT_Recipes.csv` | Axe, Pick, Wood Club and other base recipes |
| `DT_CookingRecipes.csv` | food/water survival path if enabled in this test |
| `DT_BossEncounters.csv` and `DT_BossAttacks.csv` | Alpha encounter identity, phases, telegraph and reward hooks |
| `DT_FastTravelSpires.csv` | optional; keep disabled in the first 20-minute route unless spire discovery is explicitly tested |
| Mecha tables | optional for the first loop; import and test separately unless exosuit is a required slice feature |

All imported assets must preserve row names and Gameplay Tags. The map should use the exact stable target tags `Location.DawnSpire`, `Interact.DawnSignal`, `Location.ResourceGrove`, `Location.DangerPit`, `Location.AquavineSpring`, `Echo.SolarixAlpha`, `Quest.Awakening`, `Quest.FirstResonator` and `Quest.DangerPit` where the corresponding contracts require them.

## 5. Deterministic 20–30 minute route

Timing is an acceptance target, not a hard timer. Distances in an 80 m map are short at the current movement speeds, so the duration must come from meaningful interactions, combat readability and decision-making rather than artificial walking. Use a development profile with deterministic spawn/loot values for QA.

| Time | Player beat | Systems under test | Required map support |
|---:|---|---|---|
| 0:00–2:30 | Spawn, orient to Dawn Spire, reach and attune to signal | PlayerStart, interaction trace, HUD, `Quest.Awakening` | visible monolith, prompt, non-blocking path, initial quest state |
| 2:30–6:30 | Move to Resource Grove and harvest wood/ore/fiber | movement, interaction, tool power/type, inventory, node depletion | 3 wood, 2 ore, 3 fiber nodes, readable return landmarks |
| 6:30–10:00 | Prepare tools/club and craft one Astra Resonator | recipe lookup, ingredient validation, crafting UI, rollback/failure message | Crafting Bench or explicitly allowed handcraft path, exact recipes, no pre-granted Resonator in acceptance profile |
| 10:00–14:30 | Fight Pyrelite or Thornback, test dodge and elemental/status feedback, weaken and capture one | melee combo, damage, dodge i-frame, capture odds/trust, failure enraged state | one staged Echo first, second Echo as optional pressure, open combat space |
| 14:30–18:00 | Summon captured Echo and reach sanctuary | party selection, summon/recall, companion presentation, navigation | safe route, companion spawn point, no collision trap |
| 18:00–22:00 | Use camp, craft/organize, interact with Aquavine, rest or restore survival, save checkpoint | camp interaction, crafting/storage, work-role hook, survival restore, save export | campfire, bed, bench, chest, spring/food source, explicit save point |
| 22:00–28:00 | Enter Danger Pit and defeat Solarix Alpha through two phases | boss trigger, telegraphs, cover, phase transition, damage authority, VFX/SFX, quest reward | entry volume, 26 m leash, 3 lanes, 2 cover rocks, open telegraph floor, boss actor |
| 28:00–30:00 | Return or use checkpoint, save/load, confirm inventory/Echo/quest/building state | SaveSubsystem restore, stable IDs, no duplicate actors, final HUD state | sanctuary save checkpoint or safe post-boss return route |

A quick combat-only profile may start with weapons and Resonators for iteration, but it must be labeled **non-acceptance shortcut**. The full acceptance profile must prove that the player can obtain the needed resources, craft the required item, capture an Echo, and advance the quest from the actual imported DataTables.

## 6. Starting profile and economy gate

The current character source adds `Item.Resource.Sunwood` quantity 10 and `Item.Tool.AstraResonatorBasic` quantity 5 at `BeginPlay`, while the prototype spire also grants three basic Resonators. This is useful for rapid debugging but it bypasses the intended `Quest.FirstResonator` crafting test and does not guarantee that Axe/Pick/Dawn Fiber prerequisites are available.

Before acceptance testing, choose one explicit policy and record it in `BUILD_STATUS.md`:

| Profile | Purpose | Recommended contents |
|---|---|---|
| Acceptance profile | Test the intended first loop | starter tools or a tutorial tool grant, one basic club, enough non-Resonator starter material to avoid a dead start, no free Resonator if the craft objective is required |
| Combat iteration profile | Tune combat/capture quickly | three Resonators, a working weapon/tool set and debug healing; does not count for full quest/economy acceptance |
| Boss iteration profile | Tune Alpha telegraphs/phases | direct spawn at Danger Pit, fixed Echo level/HP, debug reset; does not count for progression or save acceptance |

Do not silently use the combat iteration profile and report it as a completed 20–30 minute loop.

## 7. Acceptance checklist

### 7.1 Boot and map integrity

- [ ] `L_Astrawild_VS_20Min` opens in UE 5.8 without missing asset, redirector or load errors.
- [ ] Player spawns on valid ground, camera and input work, and all four landmarks are readable from the central area.
- [ ] NavMesh covers paths, ramps, sanctuary and pit without trying to generate outside the intended area.
- [ ] No foliage, collision or VFX obscures boss telegraphs, capture traces or interaction prompts.
- [ ] Runtime generation is disabled or guarded against duplicate actors for the final authored map test.

### 7.2 Quest and economy

- [ ] `Quest.Awakening` starts or is available according to the intended new-game policy.
- [ ] Dawn Spire reach and signal interaction advance the correct objectives.
- [ ] Harvesting writes correct Sunwood/Lumen/Astra/Dawn Fiber quantities to inventory.
- [ ] The acceptance profile can craft the required tool/club and one Astra Resonator without free-item shortcuts.
- [ ] Crafting failure explains the missing requirement and does not consume ingredients incorrectly.
- [ ] `Quest.FirstResonator` advances on the authoritative craft/capture events, not merely item possession.

### 7.3 Combat, capture and camp

- [ ] At least one staged wild Echo can be damaged, weakened, captured, failed, enraged and captured on a later attempt.
- [ ] Elemental/status feedback is readable from animation, VFX, SFX and UI; the player can understand the counterplay.
- [ ] Dodge i-frame, melee hit windows and health updates work in PIE.
- [ ] Captured Echo enters party/storage, can be summoned, recalled and shown in the companion strip.
- [ ] Campfire/bed/bench/chest interaction prompts do not overlap and each actor has a stable ID.
- [ ] Food/water/survival behavior is either genuinely testable from map sources or explicitly disabled for this acceptance run.

### 7.4 Alpha encounter and persistence

- [ ] `Quest.DangerPit` requires `Quest.FirstResonator` and the entry trigger does not soft-lock early entry.
- [ ] Solarix Alpha has intro, Phase One, Phase Two, defeat and reset states.
- [ ] All six required attack patterns have readable telegraphs and usable counterplay.
- [ ] Defeat advances the authoritative `DefeatAlpha` objective and grants the intended reward once.
- [ ] Save/load preserves player position or checkpoint, inventory, captured Echo, quest state, survival state and placed building/container state.
- [ ] Reloading does not duplicate harvest nodes, buildings, rewards, Echoes or boss state.

## 8. Evidence to record in BUILD_STATUS.md

For this map, append separate evidence rows rather than one general “passed” statement:

| Evidence | Minimum record |
|---|---|
| Map asset | exact `.umap` path, map open screenshot and engine version |
| Compile | target `ASTRAWILDEditor Win64 Development`, compiler/UHT result, warnings/errors |
| DataTables | imported asset paths, row counts and `DataTableImportReport.json` |
| Generated assets | `GeneratedAssetImportReport.json`, registry path and screenshots of representative OBJ/WAV imports |
| Scaffold | `AssetScaffoldReport.json` with created/existing/skipped/failed counts |
| PIE core loop | map name, test profile, start/end time, screenshots/video, Output Log and known issues |
| Boss | phase/telegraph/defeat evidence and quest reward result |
| Save/load | slot name, before/after state comparison and reload log |
| Performance | game/render/GPU/memory/network measurements for the slice; do not infer them from static source |

## 9. Decision after the first run

If the player can complete the route but the loop feels flat, tune feedback, encounter spacing, resource yield, capture odds and boss telegraphs before adding breeding, mounts, raids, guilds or more biomes. If the route cannot complete because of missing assets, tags, save IDs, collision or compile errors, fix that integration blocker first. The compact map is successful when it exposes the real fun and the real bugs in one reproducible session.

> **Boundary:** This document specifies the map and test contract. It does not claim that the `.umap`, final assets, DataTables or PIE evidence already exist in the repository.
