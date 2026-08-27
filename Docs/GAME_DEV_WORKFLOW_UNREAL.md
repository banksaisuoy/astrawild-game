# ASTRAWILD — Unreal Game Development Workflow

## Purpose

เอกสารนี้นำหลักการ workflow แบบเกมจริงมาใช้กับ Unreal Engine โดยปรับจากขั้น visual target → risk decomposition → scaffold → architecture → asset planning → implementation → visual verification ให้เหมาะกับโปรเจกต์ที่มี C++/Blueprint/Map binary และให้ Antigravity เป็นผู้รัน Unreal Editor บนเครื่องจริง

> Game Dev skill ที่ใช้อ้างอิงถูกออกแบบหลักสำหรับเกมเว็บ Babylon.js ไม่ใช่ Unreal Engine โดยตรง ดังนั้นเราใช้เฉพาะหลักการผลิตและตรวจรับที่เหมาะสม ไม่ใช้คำสั่ง WebDev หรือ pipeline Babylon.js กับ ASTRAWILD

## Stage 0: Source of truth

Antigravity ต้องอ่านไฟล์ตามลำดับนี้ก่อนแก้โปรเจกต์:

1. `ANTIGRAVITY_START_HERE.md`
2. `README.md`
3. `Docs/CODE_SCOPE_AND_HANDOFF.md`
4. `Docs/ANTIGRAVITY_BUILD_CHECKLIST.md`
5. `Docs/BUILD_STATUS.md`
6. `Docs/ASSET_AND_CONTENT_PLAN.md`
7. `Docs/ASTRAWILD_PROJECT_MASTER_PLAN_v1.md`
8. source headers ใน `Source/AstrawildCore/Public`

หากรายงานภายนอกอ้างว่า branch หรือไฟล์มีอยู่ แต่ GitHub revision ที่ดึงมาไม่มี ให้ถือ repository เป็น source of truth และรายงานความแตกต่างก่อนนำไฟล์จากเครื่องอื่นมารวม

## Stage 1: Visual target

ก่อนทำโมเดลจริง ให้สร้าง visual target สำหรับ Vertical Slice หนึ่งภาพหรือ moodboard ที่แสดงทุ่งอรุณ, Ancient Dawn Spire, Resource Grove, Danger Pit และ Rest Sanctuary โดยใช้รูปทรงที่แยกได้จากกล้อง third-person สีธรรมชาติและพลังงานเรืองแสงเฉพาะจุด

Visual target ต้องตอบได้ว่า:

- ผู้เล่นเห็น landmark และรู้ทิศทางอย่างไร
- Echo เพื่อน ศัตรู และ resource node แยกจากกันอย่างไร
- สีธาตุไม่ใช่ข้อมูลเพียงชนิดเดียวอย่างไร
- คุณภาพ Low/Medium ยังอ่าน gameplay ได้อย่างไร
- จุดพัก/ฐานเป็นจุดกลับมาใช้งาน ไม่ใช่เพียง prop ตกแต่งอย่างไร

ไม่ต้อง commit ภาพตัวอย่างที่มี license ไม่ชัดเจน หากสร้างภาพ concept ให้เก็บ prompt, วันที่ และสถานะว่าเป็น reference ไม่ใช่ final asset

## Stage 2: Risk slices

ทำความเสี่ยงสูงก่อนคอนเทนต์ใหญ่:

| Risk slice | วิธีพิสูจน์ | เกณฑ์ผ่าน |
|---|---|---|
| Unreal C++ API | Compile Development Editor | 0 compile errors |
| Enhanced Input | Player Blueprint + Input Assets | เดิน/มอง/วิ่ง/Interact ได้ |
| C++/Blueprint data | Echo/Item/Recipe assets | ID ไม่ว่างและอ่านค่าจริง |
| Capture transaction | Echo + CaptureComponent | จับซ้ำ/เป้าหมายผิด/นอกระยะไม่เสีย item |
| Inventory/crafting | resource → recipe → output | ไม่หักของซ้ำและไม่ติดลบ |
| Save/load | Player/World snapshot | โหลดแล้วข้อมูลตรงและ fallback เมื่อเสีย |
| Prototype map | `L_Prototype.umap` | เล่น core loop จบได้ |
| Placeholder art | primitive mesh/material | ทุก object มองเห็นและ collision ถูก |
| Performance | Unreal Insights/Stat Unit | อยู่ในงบ target scene |

## Stage 3: Architecture contract

C++ เป็นเจ้าของกฎและข้อมูลที่ต้องเสถียร ส่วน Blueprint เป็นเจ้าของการประกอบคอนเทนต์และ reference asset ใน Unreal Editor การแก้ Blueprint ไม่ควรย้ายกฎสำคัญออกจาก C++ โดยไม่มีเอกสาร เพราะจะทำให้ Save/Multiplayer และการทดสอบแตกต่างกัน

ทุก asset ต้องมี path และ stable ID ที่ระบุใน checklist การใช้ชื่อไฟล์ไม่แทน stable ID และการใช้ Actor reference ฝังใน Level Blueprint ไม่ควรเป็นวิธีเก็บสถานะถาวร

## Stage 4: Asset pipeline

ลำดับ asset ที่เร็วและปลอดภัยคือ primitive placeholder → licensed environment/props → original Echo silhouettes → animation/rig → material/VFX/audio polish ทุก asset ต้องมี license record ใน `Docs/ThirdPartyLicenses.md` ก่อนรวมเข้าเกม

โมเดล Echo ต้องผ่าน mesh, skeleton, physics asset, animation set, sockets, material, LOD/fallback, collision และ Data Asset ไม่ใช่เพียงนำ FBX เข้า Content Browser แล้วถือว่าเสร็จ

## Stage 5: Implementation order

Antigravity ควรทำตามลำดับนี้และหยุดเมื่อ gate ไม่ผ่าน:

1. Compile C++ และแก้ API error
2. สร้าง Player Blueprint/Input assets
3. สร้าง Item/Recipe/Echo Data Assets
4. สร้าง Resource Node/Rest Point/Test Target Blueprint
5. สร้าง Prototype Map และ GameMode
6. ทดสอบ movement/interact/harvest/inventory
7. ทดสอบ damage/capture/crafting/rest point
8. ทดสอบ save/load และ backup
9. เพิ่ม HUD/debug visualization
10. แทน placeholder ด้วย licensed/original assets
11. จึงค่อยเพิ่ม AI polish, Niagara, sound และ multiplayer

## Stage 6: Visual verification

เกมยังไม่ถือว่าเสร็จจากการ Compile เพียงอย่างเดียว ต้องเปิด Play-in-Editor และตรวจจากภาพที่ผู้เล่นเห็นจริง ผู้ตรวจต้องอัด screenshot หรือวิดีโอสั้นของการเดินทางใน `L_Prototype`, การเห็น Echo, การเก็บทรัพยากร, การจับ, การสร้างจุดพัก และการโหลดเกมกลับมา

หากระบบมีอยู่ใน C++ แต่ผู้เล่นไม่สามารถเห็นหรือใช้ผ่าน Map/Blueprint/UI ให้รายงานเป็น `PARTIAL` ไม่ใช่ `COMPLETE` การตรวจ visual ต้องทำทั้ง Low และ Medium preset อย่างน้อยหนึ่งรอบเมื่อเริ่มใส่ asset จริง

## Stage 7: Handoff

หลังแต่ละ milestone Antigravity ต้อง commit branch พร้อม `Docs/BUILD_STATUS.md`, รายการไฟล์เปลี่ยน, compile result, playtest result, known issues และ screenshot/video path หาก report อ้างว่าทำสำเร็จแต่ branch หรือไฟล์ไม่ปรากฏบน GitHub ให้ถือว่ายังไม่ส่งมอบจนกว่าจะ push และตรวจ tree ได้
