# ASTRAWILD — Detailed Repository Audit

**Audit date:** 2026-08-27  
**Audited revision:** `227c8a1` plus local uncommitted placeholder-visual changes pending commit  
**Repository:** `https://github.com/banksaisuoy/astrawild-game`  
**Audit scope:** source tree, Unreal project descriptor, Config, Content, documentation, validation script, and asset plan

## Executive conclusion

### Verification of the attached Antigravity report

ไฟล์แนบ `pasted_content.txt` ระบุว่า Antigravity มี Vertical Slice ที่ Compile/Play ผ่านและอยู่บน branch `release/vertical-slice-v1` commit `f8cf5f1` แต่จากการตรวจ GitHub จริงพบว่ามีเพียง branch `main` และไม่พบ branch `release/vertical-slice-v1` หรือ commit ดังกล่าวบน repository ที่ส่งมอบ ดังนั้นรายงานนั้นถือเป็น **ผลจากเครื่องหรือ workspace อื่นที่ยังไม่ได้ส่งขึ้น GitHub** ไม่ควรนับเป็นสถานะของ repository จนกว่าจะมีการ push branch/commit และตรวจไฟล์ได้จริง

รายงานที่แนบยังอ้างถึง source paths จำนวนมาก เช่น `Characters/`, `Echoes/`, `Components/`, `Environment/`, `SaveSystem/` และ `UI/` ซึ่งไม่มีอยู่ใน GitHub `main` ที่ตรวจสอบ ส่วน `Content` บน GitHub ยังมีเพียง `.gitkeep` ดังนั้นต้องให้ Antigravity push ผลงานจากเครื่องจริง หรือส่ง patch/archive ที่ตรวจสอบได้ก่อนจึงจะรวมเป็นสถานะโครงการ

> **สรุปตรง ๆ:** โค้ดแกนกลางมีแล้วในระดับ prototype contract แต่เกมยังไม่ใช่โปรเจกต์ที่เปิด Unreal แล้วกด Play ได้ทันที เพราะยังไม่มี Unreal binary assets ใด ๆ ใน `Content` นอกจาก `.gitkeep` ไม่มี `.umap`, `.uasset`, input assets, Blueprint, animation, UI, material, sound หรือโมเดล 3D จริง

สิ่งที่ Antigravity ต้องทำต่อจึงไม่ใช่การเขียนระบบ C++ ใหม่ทั้งหมด แต่ต้องเปิด Unreal Editor, Compile โมดูล, สร้าง Blueprint/Data Assets/Map และทดสอบ API ที่มีอยู่ หาก Compile พบ API mismatch ตาม Unreal 5.8 ให้แก้เฉพาะ error แล้วส่งผลกลับมา การทำให้เกม “เหมือน ARK + Palworld + Pokémon” จะต้องเพิ่มคอนเทนต์จำนวนมากหลัง Vertical Slice ไม่ควรพยายามสร้างโลกใหญ่และโมเดลจำนวนมากก่อน core loop ผ่าน

## 1. Inventory ที่ตรวจพบจริง

| หมวด | จำนวน/สถานะที่ตรวจพบ | ข้อสรุป |
|---|---|---|
| Unreal project descriptor | `ASTRAWILD.uproject` 1 ไฟล์, target 5.8 | มีโครงเปิดโปรเจกต์ แต่ต้องตรวจใน Editor จริง |
| Runtime C++ module | `AstrawildCore` | มี module, Build.cs, target และ source แกนกลาง |
| C++ source | 24 ไฟล์ใน `Source` รวม headers, cpp และ target rules | มีโค้ดประมาณ 1,400 บรรทัดตาม static inventory |
| Unreal Config | `DefaultEngine.ini`, `DefaultGame.ini` | มี GameMode, render baseline, max players และ save/game settings |
| Content binary | มีเพียง `Content/ASTRAWILD/.gitkeep` | **ไม่มีโมเดล แผนที่ Blueprint หรือ Data Asset จริง** |
| Documentation | game design, gameplay, architecture, art, performance, roadmap, build checklist, prompt pack, asset plan และ audit | เอกสารค่อนข้างครบสำหรับเริ่ม Vertical Slice |
| Automated validation | `Scripts/validate_repository.sh` | ตรวจ JSON, required files, generated folders และ typo markers; ไม่แทน Unreal Compile |
| GitHub | private repo, branch `main` | มีโค้ดและ docs ล่าสุดบน GitHub |
| Google Drive | มี pre-production package และ code snapshots | ใช้สำรอง archive และไฟล์ใหญ่ |

## 2. สิ่งที่โค้ดทำแล้ว

### 2.1 Project and module foundation

โปรเจกต์มี `AstrawildCore` runtime module, Windows targets, Unreal 5.8 association, Enhanced Input, Gameplay Ability/Tags/Tasks และ StateTree plugin entries โครง C++ ใช้ `FAstrawildCoreModule` และ log category `LogAstrawild` สำหรับ debugging

### 2.2 Core data contracts

`AstrawildTypes.h` มี element types, Echo roles, stable IDs, item stacks, Echo stats, Echo instance save data และ rest point save data ส่วน `AstrawildDataAssets.h` มี `UAstrawildItemDefinition`, `UAstrawildRecipeDefinition` และ `UAstrawildEchoDefinition` ที่ใช้ Primary Asset IDs, display data, stats, role, element, icons, soft mesh references และ ability IDs

### 2.3 Player and interaction

`AAstrawildPlayerCharacter` มี camera boom, follow camera, Enhanced Input contract, movement, look, sprint, interaction trace และ components สำหรับ inventory, crafting และ capture มี primitive cylinder placeholder mesh เพื่อให้ตัวละครมองเห็นได้เมื่อ Unreal สร้าง class สำเร็จ แต่ยังไม่มี Input Action, Mapping Context, animation หรือ UI asset

`UAstrawildInteractable` เป็น interface กลางสำหรับ Resource Node, Rest Point และวัตถุอื่นที่จะเพิ่มภายหลัง

### 2.4 Inventory and crafting

`UAstrawildInventoryComponent` รองรับ Add, Remove, GetQuantity, HasItem, ConsumeItems, SetItemStacks และ event `OnInventoryChanged` ส่วน `UAstrawildCraftingComponent` ตรวจสูตรและหัก/เพิ่ม item แบบ transaction พื้นฐาน โดยอ้างอิง Recipe Data Asset

### 2.5 Echo and capture

`AAstrawildEchoCharacter` อ่าน `UAstrawildEchoDefinition`, สร้าง Instance ID, เก็บ level/trust/health, รับ damage, capture และแปลงเป็น `FAstrawildEchoInstanceSaveData` มี sphere placeholder mesh เพื่อทดสอบระบบได้โดยไม่ต้องมี creature model จริง

`UAstrawildCaptureComponent` มี cooldown, target cast และ event ผลลัพธ์การ capture แต่ยังไม่มี capture input asset, capture UI, animation, VFX หรือระบบ roster/party UI

### 2.6 World interaction

`AAstrawildResourceNode` มี primitive cube placeholder, collision, item ID, quantity, harvesting และ respawn ส่วน `AAstrawildRestPoint` มี primitive cylinder placeholder, world object ID, active state, interaction และ save data

### 2.7 Save and game mode

`UAstrawildSaveGame` และ `UAstrawildSaveSubsystem` มี schema version, timestamp, inventory, Echo roster, rest points และ active rest point พร้อม save/load/delete/exists API `AAstrawildGameMode` กำหนด `AAstrawildPlayerCharacter` เป็น default pawn

## 3. สิ่งที่ยังขาดและระดับความสำคัญ

| ระดับ | สิ่งที่ขาด | ผลกระทบ | ผู้รับผิดชอบที่เหมาะสม |
|---|---|---|---|
| Blocker | Unreal Compile จริง | ไม่รู้ว่า API/Generated Header/Engine version ผ่านหรือไม่ | Antigravity |
| Blocker | Prototype map `.umap` | ยังไม่มีฉากให้กด Play | Antigravity |
| Blocker | Player/Input Blueprint/Data Assets | C++ ยังไม่มี input และ asset references | Antigravity |
| Blocker | GameMode/Player Start/World setup | เปิดเกมแล้วไม่เกิด core loop | Antigravity |
| High | Echo/Item/Recipe Data Assets | ระบบไม่มีข้อมูลจริงให้เล่น | Antigravity |
| High | UI/HUD/Inventory feedback | ผู้เล่นไม่รู้ health, item, capture และ save state | Antigravity |
| High | Damage receiver/combat test target | Combat API ยังไม่มี loop ให้ทดสอบ | Antigravity + Manus แก้ API เมื่อพบ error |
| High | AI behavior/StateTree/Behavior Tree | Echo ยังไม่เดิน ตรวจจับ ต่อสู้ หรือหนีแบบเกมจริง | Antigravity |
| High | Save integration กับ Player/World | Save API มีแล้ว แต่ยังไม่มี actor ที่เรียกและ restore จริง | Antigravity |
| High | 3D models/materials/animation | ภาพยังเป็น primitive และไม่มีการเคลื่อนไหว | Asset pipeline/Antigravity |
| Medium | Quest/progression | ยังไม่มีเรื่องราวและเป้าหมายระยะกลาง | Manus/Antigravity |
| Medium | Building placement หลายชิ้น | มี Rest Point contract แต่ยังไม่มี grid building system | Manus + Antigravity |
| Medium | VFX/audio | feedback ยังไม่ถึงระดับเกม | Antigravity/asset pipeline |
| Medium | Multiplayer replication/server authority | วาง contract บางส่วนใน docs แต่ C++ ยังเป็น single-player prototype | ระยะหลัง Vertical Slice |
| Low | Full World Partition/HLOD/open world | ไม่ควรทำก่อน map ทดลองสนุกและ performance ผ่าน | ระยะขยายเกม |
| Low | Dedicated server, matchmaking, voice chat | ไม่จำเป็นสำหรับการเล่นกับเพื่อนใน prototype แรก | ระยะ Co-op จริง |

## 4. สถานะโมเดล 3D และแผนที่

### คำตอบแบบตรงไปตรงมา

**ยังไม่ได้ทำโมเดล 3D จริงและยังไม่ได้ทำแผนที่ 3D จริงครับ** ใน repository ไม่มี `.uasset`, `.umap`, `.fbx`, `.glb`, texture, animation หรือ sound asset ใด ๆ ที่เป็นเกม สิ่งที่เพิ่มล่าสุดคือ **primitive placeholder mesh ใน C++** สำหรับ Player, Echo, Resource Node และ Rest Point เพื่อให้ Antigravity สามารถ Compile แล้วประกอบฉากทดลองได้เร็วขึ้น แต่ placeholder ไม่ใช่ final art และไม่ได้ทำให้เกมมีโลกหรือคอนเทนต์พร้อมเล่นเอง

### สิ่งที่ต้องมีเพื่อให้ Vertical Slice เล่นได้

Antigravity ต้องสร้าง `L_Prototype.umap`, `BP_AstrawildPlayer`, `BP_Echo_Explorer`, `BP_Echo_Combat`, `BP_Echo_Base`, `BP_ResourceNode`, `BP_RestPoint`, Input Actions, Mapping Context, HUD, Item Data Assets, Recipe Data Assets, Echo Data Assets, Player Start และพื้น/แสงในฉาก รายการเต็มอยู่ใน `Docs/ANTIGRAVITY_BUILD_CHECKLIST.md`

## 5. แนวทางให้เหมือน ARK + Palworld + Pokémon โดยไม่ลอก

เกมควรยืม “ประเภทประสบการณ์” ไม่ใช่คัดลอกทรัพย์สินหรือระบบเฉพาะแบบหนึ่งต่อหนึ่ง

| แรงบันดาลใจ | สิ่งที่นำมาเป็นแนวคิดได้ | สิ่งที่ไม่ควรคัดลอก |
|---|---|---|
| ARK | การเอาชีวิตรอด คราฟต์ ทรัพยากร ฐาน และการออกเดินทาง | ชื่อ สิ่งมีชีวิต โมเดล UI แผนที่ และสูตรเฉพาะ |
| Palworld | อีโคที่ช่วยต่อสู้/ทำงาน ฐาน และ Co-op ที่เข้าถึงง่าย | รูปลักษณ์ ชื่อ ระบบจับ และดีไซน์ที่ระบุแหล่งที่มา |
| Pokémon | การสะสม ความสัมพันธ์ ธาตุ บทบาททีม และความก้าวหน้า | Pokémon design, names, cries, Pokédex style และ artwork |
| ASTRAWILD | โลกฟื้นตัว อีโคมีระบบนิเวศ ความไว้ใจ และการตัดสินใจของทีม | ต้องสร้างตัวตนและดีไซน์ใหม่ของเราเอง |

สำหรับการเล่นกับเพื่อน ให้ตั้งเป้า Co-op 1–4 คนก่อน ไม่ต้องทำ MMO ระบบที่ควรทำให้สนุกก่อนคือออกสำรวจ เก็บของ เจออีโค จับ/ช่วยเหลือ ต่อสู้ กลับฐาน คราฟต์ และพัฒนาอีโค การทำระบบนี้ครบหนึ่งรอบมีคุณค่ามากกว่าการมีแผนที่ใหญ่ที่ยังไม่มีเหตุผลให้เดินทาง

## 6. แหล่งโมเดลและ asset ที่แนะนำ

สำหรับ prototype ให้ใช้ primitive และ placeholder ที่สร้างเองก่อน แล้วค่อยแทนด้วย asset ที่มีสิทธิ์ชัดเจน

| แหล่ง | เหมาะกับ | สถานะ license ที่ตรวจพบ |
|---|---|---|
| Fab | environment kit, rocks, foliage, materials, animation และ packs | Fab Standard License ระบุว่าใช้แบบส่วนตัว/เชิงพาณิชย์ ปรับรวมใน Project และแชร์กับผู้ร่วมโครงการได้ แต่ห้ามแจก asset แบบแยกเดี่ยว [1] |
| Kenney | props, UI, icon และ placeholder ทั่วไป | Kenney ระบุว่า asset บนหน้าสินค้าเป็น CC0/public domain และไม่บังคับ attribution [2] |
| Poly Haven | HDRI, PBR texture, rocks, ground และบาง model | Poly Haven ระบุว่า asset เป็น CC0 ใช้ได้ทุกวัตถุประสงค์ แต่เนื้อหาเว็บไซต์ที่ไม่ใช่ asset อาจมีสิทธิ์แยกต่างหาก [3] [4] |
| สร้างเองใน Blender | Echo และ asset ที่ต้องการเอกลักษณ์ | ควบคุมสิทธิ์และสไตล์ได้ดีที่สุด แต่ต้องทำ rig, animation, collision และ optimization |

แม้จะเล่นกันเองและไม่ขายเกม ก็ยังควรเก็บ license record เพราะ asset บางชนิดมีข้อจำกัดการแชร์กับคนอื่นหรือการอัปโหลดใน repository การแชร์ private repository กับผู้ร่วมพัฒนาอาจทำได้ตาม license ของ Fab แต่ห้ามนำ source asset ไปแจกเป็นไฟล์เดี่ยว

## 7. รายการที่ควรเพิ่มต่อในโค้ด

โค้ด core ปัจจุบันเพียงพอสำหรับเริ่มให้ Antigravity Compile และประกอบ Vertical Slice แต่ถ้าต้องการให้เขาเขียนเพิ่มน้อยที่สุด ควรเพิ่มใน milestone ถัดไปดังนี้:

1. `UPrimaryDataAsset` validation และ content registry สำหรับตรวจ Item/Recipe/Echo ID ซ้ำหรือว่าง
2. `AAstrawildDamageTarget` หรือ damage interface สำหรับ target ฝึกต่อสู้ที่ใช้กับ Player/Echo/Enemy ได้
3. `AAstrawildEnemyCharacter` แบบ primitive พร้อม health/damage/aggro state เพื่อให้ combat ทดสอบได้ทันที
4. `UAstrawildQuestSubsystem` แบบ minimal สำหรับ objective เก็บของ/จับ Echo/เปิด Rest Point
5. `UAstrawildBuildingComponent` แบบ grid/snap สำหรับ Rest Point และอาคารหนึ่งชนิด
6. `UAstrawildPrototypeWorldSubsystem` สำหรับ spawn resource/Echo แบบจำกัดจำนวนและ reset ได้
7. ระบบเริ่มต้นของผู้เล่น เช่น starter items และ starter Echo เพื่อทำให้กด Play แล้วทดลองได้ทันที
8. Automated test หรือ functional test สำหรับ inventory transaction, capture cooldown และ save schema
9. C++ fallback input หรือ input asset generation instructions ที่ชัดขึ้น เพราะ Enhanced Input assets ยังเป็น binary ที่ต้องสร้างใน Editor
10. Network-safe command boundary สำหรับ capture, inventory และ build เมื่อเริ่ม Co-op

## 8. เกณฑ์ “Antigravity ทำแปปเดียวเสร็จ” ที่เป็นจริง

Antigravity จะทำต่อได้เร็วที่สุดถ้าไม่ต้องออกแบบใหม่ โดยให้มันทำตามลำดับนี้:

1. Pull commit ล่าสุดและ Compile ก่อน
2. สร้าง Data Assets และ Blueprint ตามชื่อที่ระบุใน checklist
3. สร้าง `L_Prototype.umap` ด้วย primitive และไฟล์แสงพื้นฐาน
4. ตั้ง Input Actions และ Player Blueprint
5. วาง Echo 3 ตัว Resource Nodes Rest Point และ Test Target
6. สร้าง HUD/debug text ขั้นต่ำ
7. ทดสอบ harvest → inventory → capture → craft → rest point → save/load
8. บันทึกผลใน `Docs/BUILD_STATUS.md`
9. ส่ง error เต็ม ๆ กลับให้ Manus หาก Compile หรือ Play ไม่ผ่าน

ถ้าต้องการให้ Antigravity “ไม่ต้องเขียนเพิ่มเลย” จริง ๆ จะต้องมี `.uasset`, `.umap`, Input Assets, Blueprint, UI และ asset 3D อยู่ใน repository หรือ Google Drive ตั้งแต่ต้น ซึ่งตอนนี้ยังไม่มี ดังนั้นคำสั่งที่ถูกต้องคือให้ Antigravity **ประกอบ binary assets ตาม contract ที่มีอยู่** ไม่ใช่คาดหวังว่า GitHub จะสร้าง Unreal Editor assets ได้เอง

## References

[1]: https://www.fab.com/eula?lang "Fab — Fab Standard License"

[2]: https://kenney.nl/support "Kenney — Common questions and game asset license"

[3]: https://polyhaven.com/license "Poly Haven — Asset License"

[4]: https://docs.polyhaven.com/en/faq "Poly Haven Wiki — FAQ"
