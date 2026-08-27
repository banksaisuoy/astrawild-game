# ASTRAWILD — Antigravity Start Here

โปรเจกต์นี้เตรียม C++ core และ data contracts ไว้แล้ว แต่ยังต้องใช้ Unreal Editor บนเครื่องจริงเพื่อ Generate project files, Compile, สร้าง Blueprint/Map/Data Assets และ Playtest

## ลำดับที่ต้องทำ

1. Pull repository ล่าสุดจาก `https://github.com/banksaisuoy/astrawild-game`.
2. เปิด `ASTRAWILD.uproject` ด้วย Unreal Engine 5.8.
3. อ่าน `Docs/CODE_SCOPE_AND_HANDOFF.md`.
4. อ่าน `Docs/ANTIGRAVITY_BUILD_CHECKLIST.md`.
5. อ่าน `Docs/ANTIGRAVITY_PROMPT_PACK.md` แล้วเริ่มจาก Prompt ระยะที่ 1.
6. รัน `Scripts/validate_repository.sh` ก่อน Compile หากเครื่องมี Bash และ jq; บน Windows ใช้ Git Bash หรือทำรายการตรวจเทียบเท่า.
7. Compile `ASTRAWILDEditor Win64 Development`.
8. สร้าง Blueprint/Data Asset/Map ตาม checklist.
9. อัปเดต `Docs/BUILD_STATUS.md` หลังทุกการทดสอบ.
10. Commit และ push ผลงานเป็น milestone พร้อมส่ง Handoff Report กลับให้ Manus AI.

## ห้ามทำ

ห้ามรายงานว่าเกมพร้อมเล่นหากยังไม่มี Compile และ Playtest จริง ห้ามลบโค้ดหรือเปลี่ยน Unreal version เพื่อหลบ error โดยไม่รายงาน ห้ามนำ asset ที่ไม่มี license เข้าโปรเจกต์ และห้าม commit โฟลเดอร์ `Binaries`, `Intermediate`, `Saved` หรือ `DerivedDataCache`

## สัญญาระหว่าง Manus กับ Antigravity

Manus พัฒนา C++, Config, Data schema, save contract, validation และเอกสารบน GitHub ส่วน Antigravity ใช้เครื่องจริงสร้างสิ่งที่เป็น Unreal binary, Compile, เปิด Map, ทดสอบ Blueprint และส่ง error/ผลทดสอบกลับมา ทั้งสองฝั่งต้องทำงานผ่าน branch และ commit ที่ตรวจสอบย้อนกลับได้
