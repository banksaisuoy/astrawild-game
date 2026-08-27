# ASTRAWILD — Production Roadmap & Project Operating Plan

## 1. หลักการจัดการโครงการ

ASTRAWILD เป็นโครงการใหญ่ จึงต้องสร้างแบบ **vertical slice ก่อน แล้วค่อยขยายแนวนอน** แทนการสร้างแผนที่และคอนเทนต์จำนวนมากก่อนรู้ว่าเกมสนุกหรือไม่ ทุกระยะต้องมี build ที่เล่นได้จริง มี performance capture และมีการทดสอบกับผู้เล่น เป้าหมายไม่ใช่ให้ทุกระบบมีอยู่ในเอกสาร แต่ให้ระบบที่อยู่ในเกมเชื่อมกันจนเกิดประสบการณ์ที่ดี

กำหนด PC เป็นแพลตฟอร์มแรก รองรับ Single-player เป็นแกน และวางโครงสร้างให้ Co-op 1–4 คนเพิ่มได้ การทำ Dedicated Server, Console และ Mobile ไม่ควรเป็นข้อผูกมัดของระยะต้น เพราะจะเพิ่มค่าใช้จ่ายและข้อจำกัดด้านประสิทธิภาพก่อนที่ core loop จะผ่านการพิสูจน์

## 2. ระยะการพัฒนา

| ระยะ | เวลาประมาณการ | ผลส่งมอบหลัก | เกณฑ์ไปต่อ |
|---|---:|---|---|
| Pre-production | 4–8 สัปดาห์ | game vision, risk register, target hardware, prototype plan และ art direction | ทีมเห็นภาพเดียวกันและรู้ว่าความเสี่ยงสูงสุดคืออะไร |
| Prototype | 8–12 สัปดาห์ | ตัวละคร กล้อง การเคลื่อนที่ ต่อสู้ จับอีโค และทรัพยากรแบบหยาบ | ผู้เล่นใหม่ทำ core loop ได้และอยากเล่นซ้ำ |
| Vertical Slice | 4–6 เดือน | เขตทุ่งอรุณขนาดย่อม อีโค 3–5 ชนิด ฐานเล็ก เควสต์ บอส และ preset กราฟิก | เล่นจบ 45–90 นาที มี save/load และ performance ผ่าน |
| Co-op Foundation | 2–4 เดือน | session, replication, inventory/quest ownership, reconnect และ server validation | ผู้เล่น 4 คนเล่น core loop โดยไม่ dupes/desync |
| Content Alpha | 6–10 เดือน | 2–3 เขต ระบบหลักครบ อีโคและคอนเทนต์ส่วนใหญ่เข้าระบบ | โครงสร้างระบบไม่เปลี่ยนใหญ่และคอนเทนต์เติมผ่าน data ได้ |
| Beta/Polish | 4–8 เดือน | เนื้อหาครบ แก้บั๊ก ปรับสมดุล performance accessibility และ localization | ไม่มี blocker, crash สำคัญ หรือ save migration ที่ล้มเหลว |
| Release & Support | ต่อเนื่อง | build release, patch process, telemetry และคอนเทนต์หลังเปิดตัว | มีทีมดูแลและ rollback/backup ที่เชื่อถือได้ |

ระยะเวลาเป็นกรอบวางแผน ไม่ใช่สัญญาส่งมอบ เพราะจำนวนทีมและการทำ asset มีผลสูง หากเป็นทีมเล็กมากควรลดขอบเขต Launch เหลือ 1–2 เขตและ Co-op ที่จำกัดก่อน เพื่อรักษาคุณภาพและให้เกมเสร็จจริง

## 3. ทีมขั้นต่ำที่เหมาะกับ Vertical Slice

ทีมขั้นต่ำไม่จำเป็นต้องมีคนครบทุกตำแหน่งแบบสตูดิโอใหญ่ แต่ต้องมีเจ้าของความรับผิดชอบชัดเจน หากใช้คนหนึ่งทำหลายบทบาท ต้องไม่ปล่อยให้ระบบสำคัญไม่มีผู้ตรวจรับ

| บทบาท | ความรับผิดชอบ |
|---|---|
| Creative/Producer | วิสัยทัศน์ ขอบเขต ลำดับความสำคัญ และการตัดสินใจตัดฟีเจอร์ |
| Lead Programmer | สถาปัตยกรรม C++/Blueprint, code review, build และ technical risk |
| Gameplay Programmer | ตัวละคร ต่อสู้ ability, AI, inventory และ interaction |
| Technical/Network Programmer | save, replication, server authority, profiling และ tools |
| Environment/World Artist | blockout, landscape, modular kit, lighting และ world composition |
| Character/Creature Artist | อีโค ตัวละคร rig, material และ LOD/fallback |
| Animator/VFX/Audio | animation, feedback, ability VFX, sound cue และ music integration |
| Designer | core loop, encounter, quest, progression, balance และ tutorial |
| QA/Playtest | test plan, bug triage, compatibility, regression และ user test |

สำหรับทีมที่เล็กกว่านี้ สามารถจ้างเสียง ดนตรี concept art หรือโมเดลบางส่วนจากภายนอกได้ แต่ต้องมี art bible และ technical validation ของทีมเองก่อนนำเข้าเกม ห้ามนำ asset ที่ไม่ทราบสิทธิ์หรือไม่ทราบ license มาใส่ใน build เชิงพาณิชย์

## 4. รายการงานสำคัญตามลำดับ

**ช่วงแรก** ต้องล็อกชื่อโครงการ วิสัยทัศน์ ตัวตนของเกม เป้าหมายแพลตฟอร์ม target hardware และคำจำกัดความของ core loop จากนั้นสร้าง prototype ที่ใช้ placeholder ได้ แต่ต้องมี input, camera, movement, combat, capture และ return-to-base ครบ

**ช่วง Vertical Slice** ต้องสร้างหนึ่งเขตที่มี landmark เส้นทางหลัก เส้นทางเสี่ยง จุดลับ ฐานเล็ก อีโคหลายบทบาท ศัตรูหนึ่งกลุ่ม บอสหนึ่งตัว เควสต์หลักหนึ่งสาย และระบบ save/load ผู้เล่นควรเข้าใจเกมจากการเล่น ไม่ต้องอ่านเอกสารยาว

**ช่วงขยายเกม** ต้องย้ายค่าคอนเทนต์เข้าสู่ Data Assets เพิ่มระบบเหตุการณ์ โลกหลายชั้น สายความสัมพันธ์ ฐานที่มีงานผลิตและอีโคทำงาน จากนั้นเพิ่ม Co-op ด้วย server authority ก่อนทำระบบกราฟิกขั้นสูงและเพิ่มจำนวน asset

**ช่วงก่อนเปิดตัว** ต้องหยุดการเปลี่ยนโครงสร้างใหญ่ ทำ content lock, bug triage, performance pass, localization pass, accessibility pass, save migration test, network soak test และ release candidate ทุก build ต้องสามารถสร้างซ้ำและย้อนกลับได้

## 5. ตารางตัดสินใจเรื่องขอบเขต

เมื่อทีมพบฟีเจอร์ใหม่ ให้ประเมินจากประโยชน์ต่อ core loop ความเสี่ยงต่อ performance ความยากด้าน Multiplayer ต้นทุน asset และความสามารถในการทดสอบ ฟีเจอร์ที่ได้คะแนนต่ำหรือเพิ่มเพียงขนาดแต่ไม่เพิ่มทางเลือกให้ผู้เล่นควรถูกเลื่อนออกไป

| ฟีเจอร์ | ตัดสินใจรุ่นแรก | เหตุผล |
|---|---|---|
| Single-player | ต้องมี | เป็นฐานทดสอบที่เร็วและไม่พึ่งเซิร์ฟเวอร์ |
| Co-op 1–4 คน | วางสัญญาข้อมูลตั้งแต่ต้น เปิดหลัง Slice | เพิ่มความสนุกแต่เพิ่มความเสี่ยงสูง |
| MMO/ผู้เล่นหลายร้อยคน | ไม่ทำในรุ่นแรก | โครงสร้าง server, economy และ moderation ใหญ่เกินความจำเป็น |
| Mobile | ไม่ทำในรุ่นแรก | ต้องออกแบบ asset, control และ memory แยก |
| รถ/พาหนะจำนวนมาก | ทำทีละชนิดหลัง core loop ผ่าน | physics และ streaming มีความเสี่ยง |
| ระบบสร้างฐานละเอียดมาก | เริ่มจาก modular snap | คุม collision, save และ network ได้ง่ายกว่า |
| สายพันธุ์อีโคจำนวนมาก | เน้น 3–5 ตัวที่ต่างจริงก่อน | ลดความเสี่ยงผลิต asset ที่ไม่ทำให้เกมดีขึ้น |
| Live service หนัก | เลื่อนหลังเกมหลักเสถียร | ต้องมีทีม support, content และ economy ระยะยาว |

## 6. ความเสี่ยงหลักและแผนรับมือ

ความเสี่ยงสูงสุดคือการทำเกมกว้างเกินทีม การสร้าง Multiplayer ก่อนพิสูจน์ความสนุก การพึ่ง Nanite/Lumen โดยไม่วัด target hardware และการมีระบบ data/save ที่ผูกแน่นจนแก้ไม่ได้ วิธีรับมือคือกำหนด risk spike แยก ทำ Vertical Slice สั้น วัด performance บนเครื่องจริง และบังคับ schema version ตั้งแต่ save แรก

| ความเสี่ยง | สัญญาณเตือน | แผนรับมือ |
|---|---|---|
| ขอบเขตบาน | เพิ่มเขต/อีโคก่อน Slice สนุก | freeze feature และใช้ cut list |
| Core loop ไม่สนุก | ผู้เล่นทำตามทางเดียวและเลิกเร็ว | playtest ทุกสองสัปดาห์ ปรับต้นเหตุ |
| Performance ตก | capture แย่ลงทุก build | budget gate และห้าม merge regression |
| Multiplayer ซับซ้อน | desync, dupes, ownership ไม่ชัด | ทำ network contract และ server test ตั้งแต่ต้น |
| Asset ผลิตช้า | อีโคหนึ่งตัวใช้เวลานานเกินแผน | reusable rig, material, animation และ outsource ที่ตรวจได้ |
| Save เสีย | migration ไม่ผ่านเมื่อเปลี่ยน schema | version, backup, checksum และ automated load test |
| ทีมแก้ asset ชนกัน | merge conflict ใน map/binary | One File Per Actor แบ่งพื้นที่และใช้ source control |
| เกมเหมือนงานอื่นเกินไป | ใช้ trope และ visual ที่ไม่เป็นเอกลักษณ์ | design pillars, original creature bible และ review ด้านสิทธิ์ |

## 7. Backlog สำหรับ Vertical Slice

Backlog ต้องเรียงจากความเสี่ยงต่อประสบการณ์ ไม่ใช่เรียงตามความสวยของ asset งานที่ต้องทำก่อนคือ movement, camera, combat, capture, save และ performance instrumentation ส่วนงานที่เพิ่มความสวยแต่ไม่พิสูจน์เกม เช่น foliage ชนิดที่ห้า หรือ particle ระยะไกล ควรเลื่อนหลัง core loop

| ลำดับ | งาน | ผลตรวจรับ |
|---:|---|---|
| 1 | Project setup, source control, coding rules, build | ทุกคนเปิดและสร้าง build ได้ |
| 2 | Player movement/camera/input | เดิน วิ่ง กระโดด หลบ และปรับค่าได้ |
| 3 | Interaction/collection | เก็บของ โต้ตอบ และ feedback ครบ |
| 4 | Echo prototype | อีโค 3 ชนิดมี behavior และ role ต่างกัน |
| 5 | Combat/ability/status | ต่อสู้ อ่าน telegraph และชนะได้หลายวิธี |
| 6 | Capture/relationship | จับได้และข้อมูลถูกบันทึก |
| 7 | Craft/base prototype | สร้างจุดพักและผลิตของได้ |
| 8 | Quest/tutorial | ผู้เล่นใหม่ทำ core loop จบได้ |
| 9 | Save/load/migration | ปิดเกม เปิดใหม่ และโหลดข้อมูลถูกต้อง |
| 10 | World/streaming/perf | เดินในเขตโดยไม่มี hitch สำคัญ |
| 11 | Co-op spike | ตรวจ ownership และ server authority |
| 12 | Polish/playtest | ข้อมูลทดสอบนำไปปรับ build ถัดไป |

## 8. สิ่งที่ต้องเตรียมก่อนเริ่มสร้างจริง

ทีมต้องเลือกเวอร์ชัน Unreal Engine ที่จะใช้และล็อกไว้ในช่วงหนึ่งของ milestone สร้าง repository ที่มี `.uproject`, source, config, content policy, contribution guide และ ignore rules ทำ project template ที่เปิด plugin เท่าที่จำเป็น พร้อมกำหนด target hardware, rendering preset, input mapping, naming convention, Data Asset schema และ save version

ต้องสร้างโฟลเดอร์โครงการที่ผู้ใช้หรือทีมสามารถผูกไว้กับพื้นที่ทำงานจริงก่อนเริ่มใส่ asset จำนวนมาก หากทำในสภาพแวดล้อมของผู้ใช้ ให้ผู้ใช้ติดตั้ง Unreal Engine, Visual Studio/Rider และ Git/LFS ตามความเหมาะสม รวมถึงระบุว่าต้องการให้โครงการอยู่บนไดรฟ์ใด หากยังไม่มีโฟลเดอร์ที่ผูกกับงาน ควรผูกโฟลเดอร์ก่อนเริ่มการสร้างไฟล์จริงเพื่อป้องกันไฟล์กระจายหลายที่

## 9. ผลลัพธ์ที่ควรส่งมอบจากเอกสารชุดนี้

ชุดเอกสารนี้ทำหน้าที่เป็น pre-production package ได้แก่ game vision, core gameplay specification, technical architecture, art/content bible, performance/security/QA plan และ production roadmap แผนภาพสถาปัตยกรรมช่วยให้ทีมเห็นการไหลของข้อมูลระหว่าง client, server, gameplay, world, AI, save และ replication

เอกสารยังไม่ใช่เกม Unreal ที่คอมไพล์พร้อมเล่น เพราะการสร้างและทดสอบ Editor, C++ module, Blueprint, animation และ asset จริงต้องทำในเครื่องที่ติดตั้ง Unreal Engine และมีโฟลเดอร์โครงการที่ทีมเข้าถึงได้ ขั้นถัดไปที่เหมาะสมคือสร้าง Vertical Slice โดยไม่เพิ่มขอบเขตจนกว่าจะผ่านเกณฑ์ความสนุกและ performance

## 10. คำสั่งเริ่มโครงการจากศูนย์

1. สร้าง Unreal Games project แบบ Third Person สำหรับ PC โดยเลือก C++ และเปิดเฉพาะ plugin ที่จำเป็น
2. สร้าง `AstrawildCore`, `AstrawildGameplay`, `AstrawildSave` และ `AstrawildEditor` เป็นโมดูลแรก
3. ทำ player movement, interaction และ camera ให้เล่นได้ในแผนที่ทดสอบเล็ก
4. สร้างอีโคต้นแบบ 3 ตัวจาก Data Asset เดียวกันแต่ต่างกันที่ behavior, ability และ utility
5. ทำวงจรเก็บทรัพยากร → คราฟต์ → สร้างจุดพัก → ออกสำรวจ → จับอีโค → กลับฐาน
6. เพิ่ม save/load และ performance capture ก่อนทำ asset ความละเอียดสูง
7. ทดสอบกับผู้เล่นจริงกลุ่มเล็ก ตัดสิ่งที่ไม่สนุก และค่อยเพิ่ม Co-op spike
8. หลัง Vertical Slice ผ่าน จึงขยาย World Partition, คอนเทนต์ และระบบออนไลน์เต็มรูปแบบ

## 11. Definition of Success

ASTRAWILD ประสบความสำเร็จในรุ่นต้นแบบเมื่อผู้เล่นใหม่สามารถเล่นตามวงจรหลักได้โดยไม่ต้องมีคนอธิบาย รู้สึกอยากลองอีโคตัวอื่น เห็นประโยชน์ของการกลับฐาน และรู้สึกว่าการเดินทางมีความหมาย เกมต้องรักษาการควบคุมที่ตอบสนองได้บน target hardware มี save/load ที่เชื่อถือได้ และไม่พึ่ง Multiplayer เพื่อกลบปัญหาของ core loop

เกมประสบความสำเร็จในรุ่นเปิดตัวเมื่อโลกมีเอกลักษณ์ อีโคมีบทบาทจริง คอนเทนต์แตกต่างจากกัน ระบบฐานช่วยการสำรวจ Co-op ไม่ทำให้ข้อมูลเสีย และทีมสามารถเพิ่มเนื้อหาใหม่ด้วย data-driven pipeline โดยไม่รื้อระบบแกนกลางทุกครั้ง
