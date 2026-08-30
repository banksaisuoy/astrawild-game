# ASTRAWILD — Antigravity Start Here

> ### ⚠️ อัปเดตสถานะ (FINAL PRODUCTION RUN, commit `249eec7`)
> ขั้นตอนที่ 8 "สร้าง Blueprint/Data Asset/Map ตาม checklist" **ไม่จำเป็นอีกต่อไปสำหรับการเล่น** —
> โปรเจกต์เปลี่ยนเป็นสถาปัตยกรรม **zero-asset runtime world**: `AstrawildGameMode` +
> `AstrawildWorldBootstrapper` สร้างโลก 6 โซน/แคมป์/สัตว์/ดันเจี้ยน/UI ทั้งหมดจาก C++ ล้วนตอน runtime
> (เปิด PIE ได้ทันทีหลัง compile) การสร้าง asset เหลือเป็น **ทางเลือก** (art pass ตาม
> `ASTRAWILD_ASSET_MANIFEST.md` และ editor Landscape ทางเลือกจาก `.r16` ใน `Content/Heightmaps/`)
> ลำดับงานจริงของคุณคือ: **pull → compile → รัน verification queue** ตาม
> `Docs/ASTRAWILD_ENGINE_VERIFICATION_QUEUE.md` → บันทึกผลใน `Docs/BUILD_STATUS.md`
> รายงานสรุปสถานะล่าสุด: `Docs/ASTRAWILD_BUILD_READINESS_REPORT.md` + `Docs/ASTRAWILD_MILESTONE_REPORT.md`

โปรเจกต์นี้เตรียม C++ core และ data contracts ไว้แล้ว แต่ยังต้องใช้ Unreal Editor บนเครื่องจริงเพื่อ Generate project files, Compile และ Playtest (asset creation เป็นทางเลือก — ดู banner ด้านบน)

## ลำดับที่ต้องทำ

1. Pull repository ล่าสุดจาก `https://github.com/banksaisuoy/astrawild-game`.
2. เปิด `ASTRAWILD.uproject` ด้วย Unreal Engine 5.8.
3. อ่าน `Docs/ASTRAWILD_BUILD_READINESS_REPORT.md` (สถานะล่าสุด) และ `Docs/ASTRAWILD_ENGINE_VERIFICATION_QUEUE.md` (แผนทดสอบของคุณ).
4. อ่าน `Docs/ANTIGRAVITY_BUILD_CHECKLIST.md` (Phase B fix-forward policy).
5. Compile `ASTRAWILDEditor Win64 Development`.
6. รัน Automation tests (filter `ASTRAWILD`, 25 tests) ผ่าน Session Frontend.
7. เดิน 23-stage golden path ตาม verification queue §2 (รวม save/load round-trip ×3).
8. อัปเดต `Docs/BUILD_STATUS.md` หลังทุกการทดสอบ.
9. Commit และ push ผลงานเป็น milestone พร้อม Handoff Report.
10. (ทางเลือก) Art pass ตาม `ASTRAWILD_ASSET_MANIFEST.md` / editor Landscape จาก `.r16`.

## ห้ามทำ

ห้ามรายงานว่าเกมพร้อมเล่นหากยังไม่มี Compile และ Playtest จริง ห้ามลบโค้ดหรือเปลี่ยน Unreal version เพื่อหลบ error โดยไม่รายงาน ห้ามนำ asset ที่ไม่มี license เข้าโปรเจกต์ และห้าม commit โฟลเดอร์ `Binaries`, `Intermediate`, `Saved` หรือ `DerivedDataCache`

## สัญญาระหว่าง Manus กับ Antigravity

Manus พัฒนา C++, Config, Data schema, save contract, validation และเอกสารบน GitHub ส่วน Antigravity ใช้เครื่องจริงสร้างสิ่งที่เป็น Unreal binary, Compile, เปิด Map, ทดสอบ Blueprint และส่ง error/ผลทดสอบกลับมา ทั้งสองฝั่งต้องทำงานผ่าน branch และ commit ที่ตรวจสอบย้อนกลับได้
