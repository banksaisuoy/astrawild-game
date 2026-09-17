# ASTRAWILD — Echoes of the First Dawn

**เกมเอาชีวิตรอด/ผจญภัยสำรวจโลก มุมมองบุคคลที่สาม บน Unreal Engine 5 (C++)** — โปรเจ็คนี้เป็น Vertical Slice ที่สมบูรณ์: เริ่มเกมที่หน้า Title → เลือก New Journey / Continue → สำรวจโลกประมวลผลลัพธ์ (procedural) 12 โซน → จับ Echo → สร้างฐาน → วิจัยเทคโนโลยี → ลงดันเจี้ยนเจอบอส → จบเควสต์หลัก 12 ตอน → ฉากชัยชนะ

A complete, playable third-person survival-adventure vertical slice for Unreal Engine 5, written almost entirely in C++ (~41,000 LOC). The runtime world generates procedurally — the game is fully playable even with zero imported art assets, and upgrades itself automatically when the optional art pack is imported.

---

## 🚀 เริ่มเล่นเร็วที่สุด / Quickstart (Windows)

### สิ่งที่ต้องมี / Requirements
| อย่าง | เวอร์ชัน |
|---|---|
| Unreal Engine | **5.8** (เปิด `.uproject` ได้เลย — สคริปต์หา engine ให้อัตโนมัติ ทั้งจาก registry และ path ที่ติดตั้งทั่วไป) |
| Visual Studio 2022 | workload "Game development with C++" |
| Git + **Git LFS** | ต้องมี LFS — ไม่งั้น map/texture/mesh จะเป็นไฟล์ pointer 130 ไบต์และเกมบูตไม่ได้ |

### 3 ขั้นตอน / 3 Steps
```bat
:: 1) Clone (ต้องติดตั้ง git-lfs ก่อน!) แล้วเข้าโฟลเดอร์
git lfs install
git clone https://github.com/banksaisuoy/astrawild-game.git
cd astrawild-game

:: 2) ตรวจสอบ + ดึงไฟล์ binary จริง (map/texture/mesh/audio) — รันครั้งเดียว
Setup_FirstRun.bat

:: 3) เปิด Editor ให้ compile C++ ครั้งแรก (กี่นาทีแล้วแต่เครื่อง)
Launch_ASTRAWILD_Editor.bat
```

จากนั้นใน Editor กด **Play (PIE)** เพื่อทดสอบ หรือรันเกมจริง:
```bat
Launch_ASTRAWILD.bat        :: เล่นแบบ standalone (มีหน้า Title)
Package_Game.bat            :: สร้าง .exe shipping ไว้ที่ Packaged\Windows\
Import_Art.bat              :: (ถ้าอยากได้) นำเข้า art pack 112 ชิ้น — ต้นไม้/หิน/Echo 3D จริง + ตัวละคร exosuit
Build_Project.bat           :: คอมไพล์ C++ อย่างเดียว
```

> **หมายเหตุ:** ถ้าหา engine ไม่เจอ ให้ตั้งค่า path เอง: `setx UE_ROOT "E:\Epic Games\UnrealEngine"`

---

## ⌨️ ปุ่มควบคุม / Controls

| การกระทำ | ปุ่ม |
|---|---|
| เดิน / มอง | WASD / เมาส์ |
| กระโดด / วิ่ง | Space / Shift |
| โต้ตอบ (เก็บของ/คุย/ใช้งาน) | E |
| โจมตี / โจมตีหนัก | คลิกซ้าย / F |
| หลบหลีก / บล็อก | Q / คลิกขวา |
| สั่ง Echo / ให้อาหาร Echo | C / R |
| โหมดก่อสร้าง / หมุน / เปลี่ยนชิ้น | B / N / ลูกกลิ้งเมาส์ |
| รื้อสิ่งปลูกสร้าง (คืนวัสดุบางส่วน) | Z |
| กินของ / สวมอุปกรณ์ที่ดีที่สุด | G / X |
| สแกน (กดค้าง) / ปล่อยโดรน / หุ่นยนต์ | V / H / J |
| กระเป๋า / งานวิจัย | TAB / K |
| เมนูพัก | ESC |
| เซฟ / โหลดด่วน | F5 / F9 |

Gamepad ใช้ได้พร้อมกับคีย์บอร์ด-เมาส์ (context แยก, รองรับตั้งแต่เสียบปลั๊ก)

---

## 🎮 ระบบที่มีในเกม / Feature List

- **โลก:** 12 โซน 2.4×1.6 กม. (ที่ราบ/ป่า/หนองน้ำ/ภูเขาไฟ/น้ำแข็ง/ทะเล 3 โซน) — terrain, แสง, หมอก, น้ำ, หมู่บ้าน 2 แห่ง, เรือบิน skiff, POI, สภาพอากาศ, กลางวัน-กลางคืน — สร้างใน C++ ทั้งหมดตอน runtime
- **เส้นเรื่อง:** เควสต์หลัก 12 ตอน จบด้วย **ฉากชัยชนะ** (The First Dawn Awakens) + สถิติ session; NPC 6 ตัวมี dialogue tree + ธงเรื่องราวบันทึกในเซฟ
- **Combat:** อาวุธ 8 แบบ (kinetic/energy/element), บล็อก-หลบ-โจมตีหนัก, stagger, status effects (ไฟไหม้/หนาว/ช็อต), บอสดันเจี้ยน 3 เฟส + ธาตุอ่อน-แข็งแรง
- **Echo (สัตว์ประหลาด):** bestiary 214 สปีชีส์, จับได้ 6 สายพันธุ์หลัก + วิวัฒนาการ, AI state (สำรวจ/หนี/ต่อสู้/ตาม), สั่งงาน/เลี้ยง/ส่งทำงานที่ work site
- **ฐาน+เศรษฐกิจ:** ก่อสร้าง (foundation/ผนัง/workbench/ไฟฟ้า/แบตเตอรี่), คราฟต์ 13+ สูตร, งานวิจัย 5 ยุค, ร้านค้า NPC, โดรน/หุ่นยนต์
- **Survival:** หิว/กระหาย/เลือด/สแตมิน่า, เกราะ 3 ชั้น, เซฟ Schema v4 (checksum + migration จาก v1) + autosave 5 นาที + Continue จาก Title
- **เกมสมบูรณ์ loop ปิดจบ:** Title → New Game → เควสต์ → ชัยชนะ → เกมต่อฟรี / ตายมี death screen + respawn

## 🔍 ยังขาดอะไร / Honest Missing List (2026-09-18)

ดูรายละเอียดใน [`Docs/WHATS_MISSING.md`](Docs/WHATS_MISSING.md) — สรุปสั้น:
1. **Co-op 1-4 คน** — โค้ดฝั่ง server มีแล้วหมด (RPC/replication บางส่วน) แต่ client-side RPC ยังไม่ครบ (H-12) — ตอนนี้เป็น single-player สมบูรณ์
2. **อาร์ตไฟนอล** — ใช้ art pack 112 ชิ้น (GLB→/Game) + ตัวละคร Manny/exosuit; ไม่มี Animation Blueprint มืออาชีพ (ใช้ single-node code-driven locomotion)
3. **เสียงดนตรี BGM** — มี SFX 36 ไฟล์ + ambient แต่ไม่มีดนตรีประกอบ
4. **แผนที่/minimap UI** — มี HUD และ zone banner แต่ไม่มีแผนที่โลก

## 📁 โครงสร้างสำคัญ

```
ASTRAWILD.uproject            เอนจิ้น 5.8 / โมดูล AstrawildCore
Source/AstrawildCore/         C++ ทั้งหมด (145 ไฟล์, ~41k LOC)
Content/                       แผนที่บูต + material + pipeline python (ผ่าน LFS)
ArtSource/                     ต้นฉบับ art 112 ชิ้น (GLB/PNG/WAV — ผ่าน LFS)
Config/                        Engine/Game/Input defaults
Tools/ArtSourceGen             ตัวสร้าง art ต้นฉบับ (python)
Docs/                          เอกสารออกแบบ+สถานะ build ทั้งหมด (91 ไฟล์)
*.bat                          สคริปต์รัน/ตั้งค่า/แพ็คเกจ (หา engine อัตโนมัติ)
```

## 📚 เอกสารหลัก
| เอกสาร | เนื้อหา |
|---|---|
| `Docs/WHATS_MISSING.md` | ผลตรวจ 2026-09-18: อะไรพัง อะไรถูกแก้ ยังเหลืออะไร |
| `Docs/BUILD_STATUS.md` | สถานะ build/verify ล่าสุด |
| `Docs/astra_wild_game_design.md` | วิสัยทัศน์เกม |
| `Docs/ASTRAWILD_ULTIMATE_GAP_ANALYSIS.md` | gap analysis ทุกยุค |
| `Docs/ASTRAWILD_INPUT_REFERENCE.md` | รายละเอียด input |

## ⚖️ ข้อตกลงในทีม
- กติกาเกมอยู่ใน C++ / data-driven; Blueprint ใช้แต่งการนำเสนอ
- ห้าม commit โฟลเดอร์ที่ engine generate (`Binaries/Intermediate/Saved/DerivedDataCache`)
- ไฟล์ binary ทุกชนิดเข้า Git LFS (`.gitattributes`)
- เซฟใหม่ทุกครั้งต้องมี schema version + migration path
- ทรัพย์สินบุคคลที่สามทุกชิ้นต้องจด license (`Docs/ThirdPartyLicenses.md`)
