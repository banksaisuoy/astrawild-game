# ASTRAWILD — Echoes of the First Dawn

> [!NOTE]
> **Current status (v9.10 CGC COMPLETION PASS + MACHINE-READY PACK, INTEGRATED — 2026)**: the repository is source-complete on branch
> `final-completion` — **141 world-free automation contracts**, full 12-zone world, 229 Echo
> species, MQ-01..17 + 5 post-game side quests + two endings (each now with a pure-C++ staged cinematic) + NEW GAME PLUS,
> save schema V5 (+ additive NGPlusCycle) — and the staged art is now **109 REAL
> unique CC0 meshes** (no cylinders, no recolors): 3 survivor armor tiers (T1 Scavenger /
> T2 Astraite / T3 Singularity Exosuit, rigged 62-bone humanoids with gun anims), 42 Echo
> species + 16 base archetypes + 14 showcase bosses (Quaternius creatures; 9 of them now bound as direct species bodies — direct-mesh coverage 51/229), 5
> geometry-distinct weapons, 4 vehicles (Dawn Skiff hover + ground rover), 4 ore nodes,
> 21 environment props — manifest **189/189 present / 0 pending**, 1:1 source uniqueness
> enforced, per-asset CC0 provenance (`Docs/ASTRAWILD_REAL_ASSET_CREDITS.json`).
> **FIRST DAY ON THE MACHINE: follow `Docs/ASTRAWILD_ON_PC_TASKS.md`** — the
> hour-by-hour runbook (clone → build → orchestrated source-side flow → asset import →
> 141-test gate → PIE feel grading → package), each step with exact commands, expected
> results, and fail routes. **Came back to a broken/unknown state? `Docs/ASTRAWILD_REVIVAL_PLAN.md`** triages from ANY state.
> The single-command source-side flow is `Tools\Python\first_day_orchestrator.py`
> (verify → prototype map → input assets, idempotent).
> **One-click on a prepared machine: run `Setup_And_Play.bat`** — it imports everything, builds the
> showcase map (`/Game/Maps/L_Showcase_ArtOverhaul`) and opens the editor; press Play.
> Design numbers are never hand-typed: every value lives traced in `Design/design_data.json`
> (`Scripts/extract_design_data.py` + `Scripts/validate_design_data.py` round-trip).
> **Everything engine-side is ENGINE-UNVERIFIED at this tip** — the one-time integration
> run (build, 140 tests, PIE, package, V2-29..V2-36 queue) belongs to the Windows UE 5.8
> machine per `Docs/ASTRAWILD_MASTER_CONTROL.md` v9.10 §8 and
> `Docs/ASTRAWILD_FINAL_BUILD_HANDOFF.md`; risks are ranked with a binding fix-forward
> protocol in `Docs/ASTRAWILD_COMPILE_RISK.md`. The canonical control doc is
> **`Docs/ASTRAWILD_MASTER_CONTROL.md`**; live execution truth is
> **`Docs/ASTRAWILD_LIVE_EXECUTION_STATE.md`**; the cinematic charter + traced
> character-feel contract is **`Docs/ASTRAWILD_CINEMATIC_IDENTITY.md`**. The narrative text below this note is the
> Vertical-Slice-era README, retained as history (its "Batch 7 / 54/54 tests" snapshot
> refers to the 2026-09-01 Antigravity V2 session).

**เกมเอาชีวิตรอด/ผจญภัยสำรวจโลก มุมมองบุคคลที่สาม บน Unreal Engine 5 (C++)** — โปรเจ็คนี้เป็น Vertical Slice ที่สมบูรณ์: เริ่มเกมที่หน้า Title → เลือก New Journey / Continue → สำรวจโลกประมวลผลลัพธ์ (procedural) 12 โซน → จับ Echo → สร้างฐาน → วิจัยเทคโนโลยี → ลงดันเจี้ยนเจอบอส → จบเควสต์หลัก 12 ตอน → ฉากชัยชนะ

A complete, playable third-person survival-adventure vertical slice for Unreal Engine 5, written almost entirely in C++ (~41,000 LOC). The runtime world generates procedurally — the game is fully playable even with zero imported art assets, and upgrades itself automatically when the optional art pack is imported.

## Download & Play

**Option 1 — packaged build (no Unreal Engine required on the playing PC)**

1. Open the **Releases** page of this repository.
2. Download `ASTRAWILD-Win64-Shipping.zip` from the latest release.
3. Unzip it anywhere and run `ASTRAWILD\Binaries\Win64\ASTRAWILD.exe`.

Requirements: Windows 10/11 64-bit, a DirectX 12 capable GPU, 8 GB RAM.
The zip is self-contained — no Unreal Engine install, no prerequisites.

**Option 2 — package it yourself (one click, on the dev machine)**

```bat
Tools\package_windows.bat
```

- Runs RunUAT `BuildCookRun` for **Win64 Shipping** (`-cook -allmaps -stage -pak -package -archive`) and writes the build to `Build\Windows\ASTRAWILD\` — the game executable lands at `Build\Windows\ASTRAWILD\Binaries\Win64\ASTRAWILD.exe`.
- Requires Unreal Engine 5.8 at `E:\Epic Games\UnrealEngine` (or set the `UE_ROOT` environment variable to your engine root) and a clone made with **Git LFS**.

**How releases are produced** — `.github/workflows/release.yml` builds, zips and attaches the Shipping build to a GitHub Release whenever a `v*` tag is pushed. It targets a **self-hosted Windows runner** (the dev PC) on purpose: GitHub-hosted runners have no UE 5.8 install, and the engine is far too large to provision per run. The workflow header documents the one-time runner setup (Settings → Actions → Runners → New self-hosted runner → install as a service).

## Current repository status

> [!TIP]
> **LATEST STATUS**: **SOURCE-COMPLETE / REAL-ART-CATALOG / DEFERRED-PACK-COMPLETE /
> FRESH-MACHINE-PLAYBOOK SHIPPED / ENGINE-UNVERIFIED (v9.10 + INTEGRATION + FIX-1)** —
> `final-completion` @ the v9.10 INTEGRATION line + FIX-1 (`b375d01` — see `Docs/ASTRAWILD_LIVE_EXECUTION_STATE.md` §1),
> **141** automation contracts, 229 species, Sci-Fantasy mutation
> system wired (incl. runtime theme-material swap), LFS 588/588 pointers resolved
> (the merge adopted the machine-ready line's two LFS-tracked Docs PNGs).
> Blank Windows machine → `Docs/ASTRAWILD_FRESH_MACHINE_PLAYBOOK.md` +
> `Scripts/fresh_machine_preflight.ps1` + `Docs/ASTRAWILD_FRESH_MACHINE_CHECKLIST.json`.
> The live task-state truth is **`Docs/ASTRAWILD_LIVE_EXECUTION_STATE.md`** (canonical,
> created 2026-09-06); the product rulebook is
> **`Docs/ASTRAWILD_MASTER_CONTROL.md` (v9.10)**;
> engine-run history (the 54/54 + packaged-exe evidence at SHA 8313c61) is
> [**`Docs/PROJECT_STATUS_SUMMARY.md`**](Docs/PROJECT_STATUS_SUMMARY.md) (HISTORICAL)
> and [**`Docs/ENGINE_LOGS/ANTIGRAVITY_EVIDENCE_MANIFEST.md`**](Docs/ENGINE_LOGS/ANTIGRAVITY_EVIDENCE_MANIFEST.md).
> **The old claim below ("compiles with 0 errors across 54/54 tests") is the
> 8313c61-era record, NOT the current tip** — the re-run queue is
> `Docs/ASTRAWILD_ENGINE_VERIFICATION_QUEUE.md` (V2-29..V2-36 — V2-36 is the one-click `Setup_And_Play.bat` real-mesh import + showcase gate).

This repository contains the verified Unreal Engine 5.8.2 C++ core, data contracts, and ArtPack ingestion pipeline. The core playable loop (Spawn -> Move -> Look -> Jump -> Sprint -> Interact -> Inventory -> Build -> Scan -> Attack) is physically verified and compiles with 0 errors across 54/54 automated tests and packaged standalone binaries.
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
