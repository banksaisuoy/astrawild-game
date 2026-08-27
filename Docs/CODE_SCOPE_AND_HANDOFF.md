# ASTRAWILD — Code Scope and Antigravity Handoff

## เป้าหมายของรุ่นนี้

รุ่นนี้จะพัฒนาโค้ดและสัญญาข้อมูลให้มากที่สุดโดยไม่พึ่งการเปิด Unreal Editor ในสภาพแวดล้อมของ Manus เป้าหมายคือให้ repository มีแกน C++ ที่ชัดเจนสำหรับ Data Asset, Inventory, Resource Node, Echo, Interaction และ Save/Load พร้อมเอกสารให้ Antigravity สร้าง Blueprint, Map, Input Assets, UI และ asset binary บนเครื่องจริง

## สิ่งที่ Manus พัฒนาใน repository

| ระบบ | สิ่งที่จะอยู่ใน GitHub |
|---|---|
| Core types | Enum, struct, Stable ID, Item Stack, Echo stats และ save schema |
| Data-driven content | Primary Data Asset สำหรับ Item, Recipe และ Echo |
| Inventory | เพิ่ม/ลบ/ตรวจไอเทมและ event เปลี่ยนแปลง |
| Interaction | Interface กลางและ Resource Node/Rest Point contract |
| Echo | Actor/Pawn ที่อ่าน Echo Definition และมีสถานะพื้นฐาน |
| Save | SaveGame schema, version, player inventory, Echo roster และ rest point |
| Debug | Console commands/log categories และ validation messages |
| Handoff | ขั้นตอน Generate Project, Compile, สร้าง Blueprint/Map และ Playtest |

## สิ่งที่ Antigravity ต้องทำบนเครื่องจริง

Antigravity ต้องเปิด Unreal Editor เพื่อสร้างไฟล์ binary ที่ไม่เหมาะกับการเขียนผ่าน GitHub ได้แก่ `.uasset`, `.umap`, Blueprint Graph, Animation Blueprint, Input Mapping Context, Widget Blueprint, NavMesh data และ asset import นอกจากนี้ต้อง Compile C++, แก้ include/API error ที่ขึ้นกับ Unreal build จริง และกด Playtest บนเครื่องของผู้ใช้

## เกณฑ์ไม่หลอกว่าทำเสร็จ

โค้ดจะถือว่า “พร้อมให้ Antigravity ตรวจ” เมื่อไฟล์มี dependency และ API ที่ออกแบบชัดเจน แต่จะถือว่า “พร้อมเล่น” ได้ก็ต่อเมื่อ Antigravity Compile ผ่าน เปิด Map ได้ กด Play ผ่าน core loop และทดสอบ Save/Load บนเครื่องจริงแล้วเท่านั้น

## ลำดับการรวมงาน

1. Antigravity ดึง branch ล่าสุดจาก GitHub และ regenerate project files
2. Compile Development Editor และรายงาน error เต็ม ๆ หากมี
3. สร้าง Blueprint/Map ตาม `Docs/ANTIGRAVITY_BUILD_CHECKLIST.md`
4. ทดสอบ C++ API ผ่าน Blueprint หรือ test actor
5. รายงานผลกลับมาให้ Manus แก้ error และปรับสถาปัตยกรรม
6. Commit เป็น milestone และ push กลับไปยัง branch ที่กำหนด
