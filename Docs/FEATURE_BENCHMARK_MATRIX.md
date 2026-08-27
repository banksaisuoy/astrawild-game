# ASTRAWILD Feature Benchmark Matrix

## Purpose

เอกสารนี้แปลงรายการ benchmark ของเกมสะสมสิ่งมีชีวิต เอาชีวิตรอด ต่อสู้ และสร้างฐานให้เป็นเกณฑ์วางแผน ASTRAWILD โดยยึดสถานะจาก source/docs ใน branch `release/vertical-slice-v1` ไม่ถือคำกล่าวว่าเล่นได้เป็นหลักฐานจนกว่าจะมี Unreal Compile/PIE evidence

## Feature matrix

| ระบบ benchmark | สถานะใน release branch | ช่องว่างที่ตรวจพบ | Priority |
|---|---|---|---|
| จับและสะสม Echo | มี `UAstrawildCaptureComponent`, capture odds/trust, party และ storage | ต้องสร้าง Blueprint/Data Assets/UI binary และทดสอบ capture loop จริง | MVP |
| สถิติ/Level/Ability | มี Data Asset, level, HP/Attack/Defense/Speed, role และ innate abilities | passive ที่เป็นระบบยังไม่ลึก และยังไม่มี progression UI ที่สมบูรณ์ | MVP |
| ธาตุและ matchup | มี Solar, Torrent, Geo, Aether, Neutral และ matrix แบบวงจร | ต้องแสดง feedback ใน UI/VFX และทดสอบ status interaction ให้ชัด | MVP |
| Real-time combat | มี 3-hit combo, cooldown, damage event และ status effects | ต้องผูก animation montage, hit windows, VFX/SFX และ telegraph จริง | MVP |
| Turn-based combat | ไม่มี และไม่ควรเพิ่มใน Vertical Slice | จะขัดกับ identity real-time action ของ ASTRAWILD | ไม่ทำตอนนี้ |
| Breeding/Inheritance | ยังไม่มีระบบ | ต้องมี genetics schema, parent compatibility, egg/incubation, inherited stats/abilities | หลัง MVP |
| Evolution/Transformation | ยังไม่มีระบบ | ต้องมี evolution conditions, form data, animation/mesh swap และ save migration | หลัง MVP |
| Base building | มี grid snap, rest point, building pieces, crafting และ refund | ต้องเพิ่ม production stations, placement UI, persistence test และ raid protection | MVP+ |
| Echo work suitability | มี role และ `WorkEfficiencyMultiplier` ใน Data Asset | ยังไม่มี work task scheduler, station assignment, queue และ output simulation | หลัง MVP |
| Survival meters | มี stamina และ restore flow | hunger, thirst, temperature, weight pressure, day/night, weather ยังไม่ครบ | MVP+ |
| Crafting/equipment | มี resource recipes, tools, resonators, buildings | weapon/armor tiers, durability, upgrade loop และ boss materials ยังต้องเพิ่ม | MVP+ |
| Mount/riding | ยังไม่มี | ต้องมี mountability contract, seat/socket, movement mode, dismount safety และ replication | หลัง MVP |
| Co-op/multiplayer | architecture เตรียม authority แต่ยังไม่ใช่ online feature ที่ตรวจผ่าน | ต้องมี session flow, replication, server validation, join/leave และ 2-client test | หลัง Vertical Slice |
| Exploration/world | มี prototype arena design 4 zones ใน C++ | Content tree ไม่มี final `.umap`/environment assets; ต้อง compose map ใน Editor | MVP |
| Quest/mission | มี Lore/Quest row schemas และ CSV source ใน branch | ต้อง import DataTables, progression subsystem, quest UI และ save integration | MVP+ |
| Technology/research tree | ยังไม่มี | ต้องมี unlock nodes, costs, prerequisites, save data และ UI | หลัง MVP |
| AI ecosystem | มี Echo AI, spawner, distance LOD, leash และ combat hooks | behavior variety, group behavior, work AI และ boss BT/StateTree ยังต้อง polish | MVP+ |
| Boss encounter | มี `AAstrawildAlphaEcho` contract และ Solarix design | ต้องสร้าง Blueprint/Data Asset/arena telegraphs/VFX/reward quest hook | MVP+ |
| Social/guild/raid | ยังไม่มี | ต้องรอ multiplayer foundation และ social scope decision | หลัง MVP |

## Current design decision

ASTRAWILD ควรเริ่มจาก **real-time cooperative creature survival action** ไม่ใช่การรวมทุกระบบของ Pokémon, ARK, Palworld และ Monster Hunter ในรุ่นเดียวกัน แกนที่ต้องทำให้สนุกก่อนคือการสำรวจพื้นที่เล็ก การอ่านธาตุและ telegraph การลดเลือดแล้วจับ Echo การจัดทีม 3 ตัว การคราฟต์ของที่จำเป็น และการสร้างจุดพักที่มีประโยชน์จริง

Breeding, evolution, mounts, weather, technology tree และ full multiplayer เป็นระบบขยายที่ต้องออกแบบให้ต่อกับ stable IDs และ Save Schema ตั้งแต่ต้น แต่ไม่ควรบังคับให้เสร็จใน Vertical Slice เพราะจะเพิ่ม content, UI, persistence และ test matrix แบบทวีคูณ
