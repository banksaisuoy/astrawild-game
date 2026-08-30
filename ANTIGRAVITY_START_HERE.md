# ASTRAWILD — Antigravity Start Here

> ### 🏭 อัปเดตล่าสุด (PRODUCTION V2 BATCH 1 — DATA FOUNDATION)
> **ตาม Master Plan ใหม่ `Docs/ASTRAWILD_PRODUCTION_V2_MASTER_PLAN.md`** — batch นี้วางรากฐาน
> data-driven ทั้งหมด: **อาวุธ 8 ตระกูล** (Kinetic/Pulse/Plasma/Laser/Arc/Rail/Missile/Experimental —
> 4 รูปแบบการยิงจริง: กระสุน/homing lock-on/beam เจาะทะลุ/arc ลูกโซ่) · **ชุดเกราะ Mk II/III/
> Experimental** + ฉนวนแยกฝั่งร้อน/เย็น · **เครื่องสแกน 3 เทียร** (Mk II มองเห็น vein ลับ, Oracle
> แทร็กสัญญาณโบราณ ×2.5 ระยะ) · **โดรนมีแบต+โมดูล 3 ชิ้น + หุ่นยนต์เฉพาะทาง 3 รุ่น**
> (Borebot ขุด/Cultivator เกษตร/Sentinel เฝ้า) · **ไซต์ผลิตแบบ consume→produce** (Camp Kitchen:
> เนื้อดิบ→เนื้อสุก) · **World events 9 แบบ** (พายุ/raid กลางคืน/อุกกาบาต/ของตก...) + **POI 12
> จุด** พร้อมระบบค้นพบ · **Echo โปรดักชัน 6 ตัว** พร้อม passive aura + **เควส 11-12** ·
> **save schema v4** (โหลด save v3 เก่าได้) · รวม 39 automation tests
> **→ อ่าน `Docs/ASTRAWILD_PRODUCTION_V2_BATCH_1.md` ก่อนเริ่มงานทุกครั้ง** (แผนบิวด์/ทดสอบ/
> ข้อจำกัด อยู่ในนั้นครบ)
>
> ### 🌊 อัปเดตก่อนหน้า (BATCH 8 "THE GRAND EXPANSE + GRAND MENAGERIE")
> **โลกขยายเป็น 12 โซน (3.2×2.4 กม.)** — ทะเล Azure Shallows, เกาะ Tidebreaker Isles,
> ทะเลทราย Sunscar, ภูเขา Stormcrest, ป่า Verdant Reach, แนวปะการัง Pearlsea Reef +
> น้ำทะเลเดินได้ (water planes) · **Echo 214 สายพันธุ์** (มังกร/หุ่นยนต์/วิญญาณ/อสูรน้ำ...)
> ทุกตัวมีร่าง procedural ต่างกัน (8 body plans × 5 ขนาด × สีเฉพาะตัว — ดู
> `Docs/ASTRAWILD_BESTIARY_CODEX.md`) · **หมู่บ้านมีชีวิต 2 แห่ง** — Dawnstead (NPC 8 ตัว)
> + Driftwood Landing (NPC 3 ตัว): เดินเวียน patrol, กลางคืนมานั่งล้อมกองไฟ, การ์ดสู้กับ
> สัตว์ร้าย (`Docs/ASTRAWILD_VILLAGES_SKIFF.md`) · **เครื่องบิน Dawn Skiff** — [E] ขึ้น,
> WASD/SPACE/CTRL/SHIFT บิน, [E] ลง · **ดันเจียนที่ 2 "Sunken Vault"** ที่เกาะ (บอสมังกรน้ำ
> Dawnfang) + เควส 9-10 ปิดท้ายเกม · ⚠️ **เริ่ม NEW GAME ใหม่** (โลกเปลี่ยน layout — save เก่า
> ไม่ใช้ได้)
>
> ### 🤖 คำสั่งพร้อมวางแปะ (ให้กับคุณ/Antigravity โดยตรง)
> ดู **`Docs/ASTRAWILD_ANTIGRAVITY_PROMPTS.md`** — Prompt 1 บิวด์+smoke test, Prompt 2 golden
> path เต็ม, Prompt 3 แก้ปัญหา+รายงาน, Prompt 4 content/art pass
>
> ### ⚠️ อัปเดตสถานะ (FINAL PRODUCTION RUN, commit `249eec7`)
> ขั้นตอนที่ 8 "สร้าง Blueprint/Data Asset/Map ตาม checklist" **ไม่จำเป็นอีกต่อไปสำหรับการเล่น** —
> โปรเจกต์เปลี่ยนเป็นสถาปัตยกรรม **zero-asset runtime world**: `AstrawildGameMode` +
> `AstrawildWorldBootstrapper` สร้างโลก/แคมป์/หมู่บ้าน/สัตว์ 214 สายพันธุ์/ดันเจียน 2 แห่ง/UI ทั้งหมด
> จาก C++ ล้วนตอน runtime (เปิด PIE ได้ทันทีหลัง compile) การสร้าง asset เหลือเป็น **ทางเลือก**
> (art pass ตาม `ASTRAWILD_ASSET_MANIFEST.md` และ editor Landscape ทางเลือกจาก `.r16` ใน
> `Content/Heightmaps/`) ลำดับงานจริงของคุณคือ: **pull → compile → รัน verification queue**
> ตาม `Docs/ASTRAWILD_ENGINE_VERIFICATION_QUEUE.md` → บันทึกผลใน `Docs/ENGINE_LOGS/` +
> `Docs/BUILD_STATUS.md` รายงานสรุปสถานะล่าสุด: `Docs/ASTRAWILD_BUILD_READINESS_REPORT.md`
> + `Docs/ASTRAWILD_MILESTONE_REPORT.md`

โปรเจกต์นี้เตรียม C++ core และ data contracts ไว้แล้ว แต่ยังต้องใช้ Unreal Editor บนเครื่องจริงเพื่อ Generate project files, Compile และ Playtest (asset creation เป็นทางเลือก — ดู banner ด้านบน)

## ลำดับที่ต้องทำ

1. Pull repository ล่าสุดจาก `https://github.com/banksiasuoy/astrawild-game`.
2. เปิด `ASTRAWILD.uproject` ด้วย Unreal Engine 5.8.
3. อ่าน `Docs/ASTRAWILD_ANTIGRAVITY_PROMPTS.md` (คำสั่งรอบนี้) และ `Docs/ASTRAWILD_ENGINE_VERIFICATION_QUEUE.md` (แผนทดสอบของคุณ).
4. อ่าน `Docs/ANTIGRAVITY_BUILD_CHECKLIST.md` (Phase B fix-forward policy).
5. Compile `ASTRAWILDEditor Win64 Development`.
6. รัน Automation tests (filter `ASTRAWILD`, 28 tests) ผ่าน Session Frontend.
7. เดิน 23-stage golden path ตาม verification queue §2 (รวม save/load round-trip ×3) + Batch 8 ขั้นเพิ่มเติม (บิน skiff ข้ามทะเล → เกาะ → Sunken Vault).
8. อัปเดต `Docs/ENGINE_LOGS/` + `Docs/BUILD_STATUS.md` หลังทุกการทดสอบ.
9. Commit และ push ผลงานเป็น milestone พร้อม Handoff Report.
10. (ทางเลือก) Art pass ตาม `ASTRAWILD_ASSET_MANIFEST.md` / editor Landscape จาก `.r16`.

## ห้ามทำ

ห้ามรายงานว่าเกมพร้อมเล่นหากยังไม่มี Compile และ Playtest จริง ห้ามลบโค้ดหรือเปลี่ยน Unreal version เพื่อหลบ error โดยไม่รายงาน ห้ามนำ asset ที่ไม่มี license เข้าโปรเจกต์ และห้าม commit โฟลเดอร์ `Binaries`, `Intermediate`, `Saved` หรือ `DerivedDataCache`

## สัญญาระหว่าง Manus กับ Antigravity

Manus พัฒนา C++, Config, Data schema, save contract, validation และเอกสารบน GitHub ส่วน Antigravity ใช้เครื่องจริงสร้างสิ่งที่เป็น Unreal binary, Compile, เปิด Map, ทดสอบ Blueprint และส่ง error/ผลทดสอบกลับมา ทั้งสองฝั่งต้องทำงานผ่าน branch และ commit ที่ตรวจสอบย้อนกลับได้
