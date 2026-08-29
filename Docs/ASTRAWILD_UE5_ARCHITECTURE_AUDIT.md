# ASTRAWILD — UE5 Production Architecture Audit (2026-08-29)

> ผลการตรวจสอบตาม directive §0.4 / §58 (STEP 1: Repository Audit)
> Auditor: Z.ai Code — Lead Game Architect round
> Repo: `banksaisuoy/astrawild-game` @ `main` (9621f63)

---

## A. Repository Health

| Item | Value | Assessment |
|---|---|---|
| Project | `ASTRAWILD.uproject` — EngineAssociation **5.8**, category Games | OK |
| UE Version | 5.8 (Nanite + Lumen HW-RT off + VSM enabled in DefaultEngine.ini) | OK |
| Build | **NEVER COMPILED** — sandbox ไม่มี Unreal Editor/MSVC; ต้อง compile บนเครื่อง Windows ของผู้ใช้ (Antigravity) | RISK |
| C++ | 1 module `AstrawildCore`, 25 ไฟล์, ~1,753 LOC, สไตล์ conservative compile-safe | KEEP |
| Blueprint | 0 — ไม่มี .uasset/.umap ใด ๆ ใน Content | GAP |
| Content | `Content/ASTRAWILD/.gitkeep` ว่างเปล่า | GAP |
| Config | DefaultEngine/DefaultGame มีพื้นฐาน (GameMode, tags import, renderer) | KEEP |
| Docs | 21 ไฟล์ ครอบคลุม design/architecture/roadmap/workflow | KEEP (บางส่วนต้อง sync กับ v2) |
| Save | Schema v1 (inventory/echo roster/rest points) มี version แต่ยังไม่มี migration | REFACTOR |
| Architecture | Single-module, component-based, data-asset-driven ทิศทางถูกต้องแต่บางมาก | EXPAND |
| Git hygiene | .gitattributes (LFS) + .gitignore ถูกต้อง, validate script มี | KEEP |
| Multiplayer | **ไม่มี replication เลย** — ไม่มี `bReplicates`, `DOREPLIFETIME`, RPC สักตัว | CRITICAL GAP |

**Environment constraint (บันทึกตาม §53):** sandbox เป็น Linux ไม่มี UE5/MSVC/GPU — งานรอบนี้ส่งมอบ **source-complete C++ ที่ compile-conservative** + กลยุทธ์ "playable โดยไม่มี binary assets" (procedural world + in-memory content library) การ compile/แพ็กเกจต้องยืนยันบนเครื่องผู้ใช้ และจะไม่มีการประกาศ COMPLETE ก่อนได้ผล compile จริง (§51)

## B. Existing Systems

| System | Current State | Quality | Action |
|---|---|---|---|
| Types & enums | Element/Role/StableId/ItemStack/EchoStats/save structs | Good | **KEEP + EXPAND** (เพิ่ม personality, needs, weather, work, quest, power, save v2) |
| Data assets | Item/Recipe/Echo definition (UPrimaryDataAsset) | Good | **KEEP + EXPAND** (Building/Tech/Quest/Loot/NPC/Weather/Ability definitions) |
| Player character | Enhanced Input (move/look/sprint/interact/attack), spring-arm camera, distance-melee | OK | **KEEP + EXPAND** (survival, dodge/block, jump, commands, replication) |
| Echo character | Definition-driven, damage, chance-based capture, trust, delegates | Good | **KEEP + EXPAND** (needs, personality, growth, commands, work) |
| Inventory | TMap<FName,int32> flat, no weight/equipment | Minimal | **REFACTOR** (weight, capacity, equipment, registry) |
| Crafting | CanCraft/CraftRecipe instant | Minimal | **REFACTOR** (stations, time, tech gates, queue) |
| Capture | Chance roll + cooldown | OK | **EXPAND** (multi-step pipeline: observe/track/feed/weaken, factors) |
| Save | Schema v1, no migration, no checksum | OK | **REFACTOR** (schema v2 + migration + checksum + full world state) |
| Resource node | Harvest + respawn timer | Good | **KEEP** |
| Rest point | Activate + save | Good | **KEEP** |
| Damage target | Simple HP actor | OK | **KEEP** (ย้ายไปเป็น test dummy) |
| GameMode | Constructor ว่าง | Empty | **REFACTOR** (GameState, cheats, bootstrapper, respawn) |
| Interaction | UInterface trace | Good | **KEEP + EXPAND** (prompt data, multiple targets) |
| Logging | LogAstrawild เดียว | Minimal | **EXPAND** (7 categories) |

## C. Architecture Problems

**Critical**
1. ไม่มี replication/RPC ใด ๆ — ทุกระบบเป็น client-only โดยพฤตินัย → ต้องใส่ server-authoritative ตั้งแต่ตอนนี้ (§28)
2. ไม่มี AI จริง (ไม่มี AIController/Perception/State) — Echo ยืนเฉย ๆ

**High**
3. ไม่มี world simulation (time/weather/ecosystem) เลย
4. Save ไม่มี migration path — เพิ่ม field ทีหลังจะ corrupt ของเก่า
5. Inventory ไม่มี weight/capacity → ไม่มี decision space เรื่องการแบกของ
6. ไม่มี UI ใด ๆ (HUD) — ผู้เล่นมองไม่เห็น stat ตัวเอง

**Medium**
7. Echo ไม่มี needs/personality → ไม่มี "สิ่งมีชีวิต" ตาม pillar 2
8. ไม่มี debug tools → ทดสอบยาก
9. Config ไม่มี input defaults (ต้องสร้าง IMC asset ใน editor — ต้องมี C++ fallback)

**Low**
10. Docs บางฉบับอ้าง web prototype ซึ่งถูกยกเลิกแล้ว (จะ mark เป็น legacy)

## D. Missing Systems (ต้อง ADD ทั้งหมด)

Logging categories • Native gameplay tags • Replicated GameState • Day/Night (deterministic) • Weather • Event bus • Survival (HP/stamina/hunger/thirst/temp + status effects) • Combat v2 (dodge/block/heavy/target-lock/elements) • Echo AIController (perception + C++ state machine + personality hooks + BT-ready blackboard constants) • Echo needs/personality/growth/relationship/commands • Echo roster & party (max 3) • Echo work system + work sites • Capture pipeline v2 + field journal • Inventory v2 (weight/equipment/registry) • Content library (code-defined default data — แทน .uasset ชั่วคราว) • Crafting stations + timed queue • Building placement (preview/snap/rotate/validate) • Building actor + ownership • Power grid (generator/battery/consumer/priority) • Research/tech tree • Quest system (event-driven) • NPC base + schedule (architecture-ready) • Save v2 (migration + checksum + autosave) • Procedural Dawn Fields bootstrapper (zero-asset playable arena) • Debug/Cheat manager • C++ HUD widget • Automation tests

## E. Technical Debt

1. `WEB_PLAYABLE_SLICE.md` — อ้างถึง web prototype ที่ยกเลิก → mark LEGACY
2. ไม่มี automation tests ที่ directive §39 สั่ง
3. Placeholder meshes ทั้งหมด — ต้องมี asset manifest ติด tag PLACEHOLDER
4. ค่า balance ทั้งหมด hard-code ใน C++ constructor — ยอมรับได้ชั่วคราว (ย้ายเข้า data asset ภายหลัง)
5. ไม่เคยมีการ profile จริง (ไม่มีเครื่องมือใน sandbox)

## F. Recommended Architecture V2

หลักการ: **single runtime module (`AstrawildCore`) + subsystem/folder architecture** (§37 อนุญาตเมื่อ module split เกินระยะ) เหตุผล: sandbox ไม่มี compiler — ลด cross-module dependency risk ให้น้อยที่สุด, split เป็นหลาย module ทำหลัง compile ผ่านจริง โครงสร้างภายใน:

```
Source/AstrawildCore/
├── AstrawildCore.Build.cs        (+AIModule, NavigationSystem, UMG, GameplayTags)
├── Public/  Private/             (flat naming: Astrawild*.h/.cpp)
│   # Foundation:  AstrawildCore, AstrawildLog, AstrawildGameplayTags, AstrawildTypes, AstrawildDataAssets
│   # World:       AstrawildGameState, AstrawildTimeSubsystem, AstrawildWeatherSubsystem,
│   #              AstrawildEcosystemSubsystem, AstrawildEventBusSubsystem, AstrawildWorldBootstrapper
│   # Player:      AstrawildPlayerCharacter, AstrawildSurvivalComponent, AstrawildCombatComponent,
│   #              AstrawildQuestComponent, AstrawildHudWidget, AstrawildPlayerController
│   # Echo:        AstrawildEchoCharacter, AstrawildEchoAIController, AstrawildEchoRosterSubsystem,
│   #              AstrawildEchoWorkComponent, AstrawildWorkSiteActor
│   # Items:       AstrawildInventoryComponent, AstrawildItemRegistrySubsystem, AstrawildContentLibrary,
│   #              AstrawildCraftingComponent, AstrawildCraftingStationActor
│   # Capture:     AstrawildCaptureComponent, AstrawildJournalSubsystem
│   # Base:        AstrawildBuildingActor, AstrawildBuildingComponent, AstrawildPowerSubsystem
│   # Meta:        AstrawildResearchSubsystem, AstrawildSaveSubsystem(v2), AstrawildCheatManager,
│   #              AstrawildGameMode, AstrawildNPCCharacter, AstrawildAutomationTests
```

สถาปัตยกรรมสำคัญ:
- **Server-authoritative**: gameplay state ทั้งหมด (survival, inventory, capture, building, power, quests) คำนวณฝั่ง server; client ส่งเฉพาะ intent ผ่าน `Server_` RPC; ผลลัพธ์ replicate ผ่าน `ReplicatedUsing`/`DOREPLIFETIME`
- **Event-driven**: `UAstrawildEventBusSubsystem` — gameplay events (ItemCollected, EchoCaptured, EchoDefeated, BuildingPlaced, TechUnlocked, LocationReached) decouple quests/journal/ecosystem จาก gameplay code
- **Data-driven**: definition data assets + **ContentLibrary** สร้าง default definitions ใน memory (สำรองจนกว่าจะมี .uasset จริง — ทุกอัน tag CODE_DEFAULT replaceable)
- **Zero-asset playability**: `AstrawildWorldBootstrapper` สร้าง Dawn Fields arena (แสง/พื้น/รีซอร์ส/เอคโค่/hostile/รีสต์/สถานีคราฟต์) จาก engine basic shapes — PIE ได้ทันทีหลัง compile โดยไม่ต้องมี content
- **Simulation LOD**: Echoes ลงทะเบียนกับ EcosystemSubsystem; Tier0 (ใกล้ผู้เล่น) full tick, Tier1 ลดความถี่, Tier2 statistical เท่านั้น (§34)
- **Save schema v2 + migration v1→v2 + checksum (FNV-1a) + autosave**

## G. Production Roadmap (round นี้ = M0→M5 ให้ครบ foundation)

| Milestone | เนื้อหา | Dependency |
|---|---|---|
| M0 | Audit (เอกสารนี้) | — |
| M1 | Foundation: log tags, gameplay tags, types v2, module deps | M0 |
| M2 | World: GameState, time, weather, event bus, bootstrapper | M1 |
| M3 | Player: survival, combat v2, HUD, death/respawn, PC + quest component | M2 |
| M4 | Echo: needs/personality/growth/AI/commands/roster/work | M2 |
| M5 | Systems: capture pipeline, journal, inventory v2, content lib, crafting stations, building, power, research, quests, save v2, cheats | M3,M4 |
| M6 | Tests + docs suite + validate script v2 | M5 |
| M7 | (เครื่องผู้ใช้) Compile + map/IMC asset จริง + playtest → ปลดล็อก M8+ | M6 |

## H. First Implementation Batch (ไฟล์ที่จะสร้าง/แก้รอบนี้)

**แก้ (7):** `AstrawildCore.Build.cs`, `AstrawildTypes.h`, `AstrawildDataAssets.h`, `AstrawildEchoCharacter.h/.cpp`, `AstrawildPlayerCharacter.h/.cpp`, `AstrawildGameMode.h/.cpp`, `AstrawildSaveSubsystem.h/.cpp`, `AstrawildCaptureComponent.h/.cpp`, `AstrawildCraftingComponent.h/.cpp`, `AstrawildInventoryComponent.h/.cpp`, `DefaultEngine.ini`, `DefaultGame.ini`, `validate_repository.sh`

**สร้างใหม่ (28):** `AstrawildLog.h/.cpp`, `AstrawildGameplayTags.h/.cpp`, `AstrawildGameState.h/.cpp`, `AstrawildTimeSubsystem.h/.cpp`, `AstrawildWeatherSubsystem.h/.cpp`, `AstrawildEventBusSubsystem.h/.cpp`, `AstrawildEcosystemSubsystem.h/.cpp`, `AstrawildWorldBootstrapper.h/.cpp`, `AstrawildSurvivalComponent.h/.cpp`, `AstrawildCombatComponent.h/.cpp`, `AstrawildQuestComponent.h/.cpp`, `AstrawildHudWidget.h/.cpp`, `AstrawildPlayerController.h/.cpp`, `AstrawildEchoAIController.h/.cpp`, `AstrawildEchoRosterSubsystem.h/.cpp`, `AstrawildEchoWorkComponent.h/.cpp`, `AstrawildWorkSiteActor.h/.cpp`, `AstrawildJournalSubsystem.h/.cpp`, `AstrawildItemRegistrySubsystem.h/.cpp`, `AstrawildContentLibrary.h/.cpp`, `AstrawildCraftingStationActor.h/.cpp`, `AstrawildBuildingActor.h/.cpp`, `AstrawildBuildingComponent.h/.cpp`, `AstrawildPowerSubsystem.h/.cpp`, `AstrawildResearchSubsystem.h/.cpp`, `AstrawildCheatManager.h/.cpp`, `AstrawildNPCCharacter.h/.cpp`, `AstrawildAutomationTests.cpp`

---
*รอบนี้ตั้งใจส่งมอบ M0–M6 ให้ครบ: Full UE5 Game Foundation ที่เปิด PIE ได้ทันทีหลัง compile โดยไม่ต้องมี binary content ใด ๆ*
