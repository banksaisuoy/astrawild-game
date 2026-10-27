# ASTRAWILD Audit Report — 2026-10-27

> ฉบับเต็ม (ฟอร์แมตเอกสาร Word): `ASTRAWILD_Audit_Report_2026-10-27.docx` (ส่งถึงเจ้าของแยกต่างหาก)
> ผู้ตรวจ: Super Z (GLM) — หน่วยตรวจสอบขนาน 4 หน่วย (โค้ด v1 / v2+integration / branch+LFS / เอกสาร+gap)
> ขอบเขต: static audit ทั้ง repo (ไม่มี UE engine ใน sandbox) ที่ tip 27 ตุลาคม 2026
> สายที่ตรวจ: `final-completion` @ c84d889 · `agent/visual-overhaul-v1` @ bcf7c81 · `agent/v1v2-integration` @ adc24ce (+ echodex / antigravity / game-shape / playable-delivery / โลกเก่า)

---

## 1. บทสรุปผู้บริหาร

- **ผลรวม: 38 รายการ** — Critical **0** / High **5** / Medium **15** / Low **18**
- **ไม่พบ compile blocker ใหม่แม้แต่รายการเดียว** — validators ของโครงการ 4/4 ALL PASS ที่ tip ทุกสายหลัก (มีเงื่อนไข git-lfs ใน PATH สำหรับ validate_repository.sh)
- คำตัดสิน **SOURCE-COMPLETE + ENGINE-UNVERIFIED ยืนยันอยู่** — โค้ดคุณภาพสูง (null-guard ครบ, RPC fail-closed, save schema v5 + migration v1→v5 + checksum, 141 automation contracts บน v1 / 145 บน integration)
- เกมยังเล่นไม่ได้จริงเพราะ: (1) โค้ด 100+ commits หลัง last-green-compile `8313c61` ไม่เคยโดน compiler (2) ไม่เคยมี release (0 tag, 0 published release)
- 3 ความเสี่ยงหลัก: **ENGINE-RUN-1 ยังไม่เคยเกิด** · **canonical branch ยังไม่ตัดสิน** (integration ขาด Night Raid fix) · **ArtSource ~254MB ข้าม LFS**
- Quick wins วันนี้ (one-liner): M-1 title screen gate, M-2 ลบ dead declaration, H-2 git-lfs preflight, M-14 เติม AWD_ToggleHUD

## 2. วิธีการตรวจ

หน่วยขนาน 4 หน่วย (2-a โค้ด/คอนฟิก v1, 2-b v2+integration+regression, 2-c topology+LFS+infra, 2-d เอกสาร+design-gap+playability) หลักฐาน: รัน validator ซ้ำเองทั้ง 4 ตัวที่ 3 tips, นับ `IMPLEMENT_SIMPLE_AUTOMATION_TEST` จริง, อ่าน raw logs ยุค 8313c61/520C78E, `git merge-tree` dry-run, `git hash-object` สุ่ม 137 ไฟล์พิสูจน์ phantom dirt, `merge-base --is-ancestor` ทุกสาย
ข้อจำกัด: ไม่มี UE engine (static เท่านั้น), LFS ท้องถิ่นมี 91/604 oid → ตรวจ asset ได้เฉพาะ pointer + สุ่ม magic bytes

## 3. สถานะ Branch (13 refs + HEAD→main)

| สาย | tip | หมายเหตุ |
|---|---|---|
| final-completion (v1) | c84d889 | tip โลกใหม่: Night Raid fix (b375d01) + AUDIT-B (9a285f7) + MERGE-1 (รวมบ้านกับ astrawild-game-v2 develop — diff สุทธิ 6 ไฟล์ bookkeeping, ไม่มีโค้ดใหม่) |
| agent/visual-overhaul-v1 (v2) | bcf7c81 | VO-1R zero-asset visual overhaul, 8 commits/20 ไฟล์ บน base 5f86a47 |
| agent/v1v2-integration | adc24ce | merge v1@4c706fc + v2 (079c1e1) + count-sync 145 + re-stamp — **ตามหลัง v1 อยู่ 3 commits** |
| agent/echodex-v1 | 4695e2c | **เนื้อหา orphan เดียวที่ยังมีชีวิต** (EchoDex 18 ไฟล์ +1 test, เขียนบนยุค 135) |
| agent/antigravity-ue5-v2 | f31f5e1 | ancestor เต็มของทุกสายปัจจุบัน — ไม่มีอะไรเหลือให้ merge |
| playable-delivery / agent/game-shape-v1 | 8c6515c / 2d39f66 | ยุคกลางต.ค. เก็บ archive |
| main | 94a398c | **default branch ของ GitHub ตามหลัง 140 commits — ควรเปลี่ยน** |
| master / release/vertical-slice-v1 | e600889 / 92a02f8 | โลกเก่า root 792ada2 (คนละ root กับเกมปัจจุบัน) — archive |
| fix/sveo-* / review-workspaces-* | — | ขยะ bot (gitlink 1 บรรทัด / empty commits) — **ลบได้** |
| understanding-the-structure-* | 0c54b90 | visual master plan ยุคแรกของ qwen bot — เก็บ archive |

สอง root ที่ไม่เกี่ยวกัน: โลกใหม่ `de92673` / โลกเก่า `792ada2` · **0 tag ทั้ง repo** · ไม่พบหลักฐาน force-push

## 4. ผล v1 (final-completion @ c84d889)

- UE 5.8, module AstrawildCore, **211 ไฟล์ (106h+105cpp) ~73k LOC**; GENERATED_BODY 100%; 0 TODO/FIXME; h/cpp pairing สะอาด; Build.cs deps ครบไม่มีตุ๊ปัด
- Validators: final_run ALL PASS (gate 141, census 15/15, species 229/items 78/recipes 58) · design_data 78/78 · lint 0 err/1 warn · repository PASS (ต้องมี git-lfs ใน PATH)
- Historical claims: **6/6 compile blockers รอบก่อนแก้จริง** · AUDIT-B ปิดจริง (สุ่ม 5/32) · "145 contracts" จริงเฉพาะ integration (v1 = 141, เอกสารสายนี้ pin ถูกแล้ว)
- ขาดฝั่งโค้ด: key rebinding (33 action hardcode), quality presets, กลับ title หลัง victory, multi-slot save browser, localization, แผนที่ editor-generated (by design รอ machine day)

## 5. ผล v2 (visual-overhaul-v1 @ bcf7c81)

- คอนเซ็ปต์ VO-1R: toon PP grade (priority-10) · anime light bias (single-writer หลัง day-night ramp) · flower fields 12 โซน deterministic · kawaii Echo (cute band: head ×1.12 ซ้อน VIS-001 ×1.18, eyes ×1.35, blush) · ambient motes 48 (cap 96) เกสรทองกลางวัน/หิ่งห้อยเขียวกลางคืน
- โค้ดสะอาด: null-check ครบ, ไม่มี div0, ค่าตรง spec ทุกตัว, วงโคจร mote ถูก test pin
- ข้อบกพร่องหลัก = perf: **RebuildMotes รันทุกเฟรม + CreateMeshSection ใหม่ทั้งชุด + LoadObject material ทุกเฟรม** (VisualOverhaul.cpp:274-343, 339)
- VOQA-1 claims ทั้ง 5 ข้อ — พิสูจน์ย้อนได้จริง (รันชุด static ซ้ำใน wt-v2)

## 6. Integration merge quality (agent/v1v2-integration @ adc24ce)

- **คำตัดสิน: SEMANTICALLY SOUND** — merge-tree จำลองได้ conflict 5 ไฟล์พอดีตามบันทึก; ไฟล์ resolve เป็น union สะอาด; **composition จริง**: Radiant ×1.10 คูณซ้อน kawaii (EchoCharacter.cpp:322/341/503); director+wildlife coexist; 145 tests นับได้จริง; validator ทั้งชุดเขียวที่ tip
- รอยเสียหายเชิงสวยงาม 3 จุด: LIVE_STATE แถว 28/96 ถูกบีบเป็นบรรทัดเดียว + validate_final_run.py:8 docstring ยังเขียน 140
- **REGRESSION จริง: ขาด b375d01 Night Raid fix** — WorldEventSubsystem.cpp:363 ยังเป็น filter ว่างเสมอ → Event_NightRaid false alarm ถาวรในสายนี้ (toast ดัง แต่ไม่มี raider เกิด) + ขาด AUDIT-B + MERGE-1 bookkeeping
- แผน UNIFY-1: merge final-completion → integration; conflict ทำนาย 15 ไฟล์/22 hunks กระจุกที่ตัวเลขนับ; **เป้า 146 contracts** (141+5); design_data regenerate ไม่ hand-merge; ห้าม `git add -A` (phantom dirt 3,952 ไฟล์)

## 7. Stale claims ที่เหลือ (14 จุด หลัง AUDIT-B กวาด 32)

| # | ตำแหน่ง | เนื้อหา | ระดับ |
|---|---|---|---|
| S1 | BESTIARY_CODEX.md:8 | 214 species + NPC 12 (จริง 229/13 — เลขที่ MASTER_CONTROL ห้ามใช้) | สูง |
| S2 | BESTIARY_CODEX (Roster) | สอนระบบธาตุสมมาตรเก่า ไม่มีป้าย HISTORICAL | สูง |
| S3 | README (Download&Play) | สัญญา zip บน Releases ที่มี 0 tag/0 release | สูง |
| S4 | README:25 | 140 tests ทับ 141 contracts ใน NOTE เดียวกัน | กลาง |
| S5 | ON_PC_TASKS.md:202 | expected x140 ทั้งที่ gate 141 — จะหลอกวันเครื่องจริง | กลาง |
| S6 | LIVE_STATE:75 | §3 บอก 140 ขัด §1 ของไฟล์เดียวกัน | กลาง |
| S7 | FINAL_READINESS:184 | §14 บอก 133 ขัด header 141 | กลาง |
| S8 | TEST_INVENTORY | ย่อหน้าเก่าอ้าง 119 เป็น authoritative | กลาง |
| S9 | VILLAGES_SKIFF:5 | อ้าง 214-species roster | กลาง |
| S10 | MASTER_CONTROL §12 | อ้าง Docs/MASTER_PLAN/ (8 ไฟล์) ที่ไม่เคยมีใน history | กลาง |
| S11 | DEFINITION_OF_DONE | พรรณนายุค 2026-08-29 ทั้งไฟล์ ไม่มีป้าย | กลาง |
| S12 | FINAL_READINESS:12 | CURRENT_HEAD 2637c13 เก่า | ต่ำ |
| S13 | COMPILE_RISK + EVQ V-1 | 197 files/45 headers (จริง 211/106) | ต่ำ |
| S14 | BUILD_STATUS + PLAYBOOK:40 | 135 (มีวันที่) + ประโยค 140 ที่ยังไม่เช็ด | ต่ำ |

## 8. Design vs Implementation gap

~30 กลุ่ม IMPLEMENTED (มีน้ำหนักจริง: ProductionContent 3,466 บรรทัด, AutomationTests 7,907 บรรทัด) / 3 PARTIAL / ~8 MISSING-DEFERRED (ส่วนใหญ่ defer แบบมีเหตุผลบันทึก)
MISSING: target lock · structural integrity/snapping (by design) · per-player building perms · recipe→container · multi-slot craft queue (by design) · save atomic/rotation/multi-profile · SCP-7 pooling (deferred) · dedicated server (out of scope §1b)
PARTIAL: weather→AI sight (API มี ใช้เฉพาะแสง), editor input 32 IA vs C++ 33 action (**F6 ToggleHUD หาย** — ไม่มีเอกสารเอ่ยถึง), art staged 109/109 แต่ imported 0

## 9. Playability (15 ขั้น)

ขั้น 1-14 มีโค้ด+เครื่องมือครบทุกขั้น (Title→Bootstrap→Loop→Save→Death→Ending→Settings→Quit) — ทั้งหมด ENGINE-UNVERIFIED
**ขั้น 15 = จุดแตก**: package ล่าสุดล้ม FZ-A1 (ExitCode 6 ที่ 8313c61) + ไม่มี release
หลักฐานยุคเครื่องจริง: 03c2fe6 (compile 0 err, 54/54, 90-130 FPS, exe ผ่าน) · 520C78E (PIE 11/11) · 8313c61 (compile ผ่าน, **package ล้ม**)
ENGINE-RUN-1 ที่เหลือ: hour 0-1 compile แรกของยุคใหม่ → 1-2 orchestrator → 3 รัน 141→146 tests → 4-6 PIE golden 23 ขั้น → 7-8 เอาชนะ FZ-A1

## 10. Bug Registry (แกนกลาง)

### High (5)
| ID | ตำแหน่ง | อาการ | แนวทางแก้ |
|---|---|---|---|
| H-1 | ArtSource/** (4,156 ไฟล์ ~254MB) | binary ดิบใน plain git ข้าม LFS → phantom dirt 3,952 รายการ + `git add -A` เป็นกับระเบิด | renormalize LFS แยกหน้า (owner-approved) + preflight ทุกสคริปต์ |
| H-2 | Scripts/validate_repository.sh:85 | ไม่มี git-lfs → exit 128 เงียบ อ่านผิดเป็น network | `command -v git-lfs` check ต้นสคริปต์ |
| H-3 | integration: WorldEventSubsystem.cpp:363 | Night Raid filter ว่างเสมอ → false alarm ถาวร | merge b375d01 เข้า (merge ได้สะอาดอัตโนมัติ) |
| H-4 | README (Download&Play) | สัญญา release ที่ไม่มี | แก้ README หรือสร้าง release แรกหลัง machine day |
| H-5 | ทั้ง repo | 0 tag = ไม่มี rollback anchor; default branch ตามหลัง 140 | tag ก่อน merge เสมอ + flip default branch |

### Medium (15)
| ID | ตำแหน่ง | อาการ | แนวทางแก้ |
|---|---|---|---|
| M-1 | PlayerController.cpp:126-131 | `NM_Client` ผ่าน gate → LAN client เจอ title screen ทับเกม (ขัด comment ตัวเอง h:344) | gate เฉพาะ NM_Standalone/NM_ListenServer (one-liner) |
| M-2 | ItemRegistrySubsystem.h:229 | BuildContentDefaults ประกาศไม่มี def (LNK2019 landmine) | ลบบรรทัดทิ้ง |
| M-3 | WorldBootstrapper.cpp:402 | SpawnActor SkyAtmosphere ไม่ dedup | cache + guard เหมือนตัวอื่น |
| M-4 | HudWidget.cpp:222-231/485-495/642-649 | ทุก 0.15s วน iterator ทั้งโลก + วิ่งต่อเมื่อซ่อน F6 | cadence 0.3-0.5s + roster mirror + early-out bHudHidden |
| M-5 | ข้ามสาย | 141 vs 145 ตาม branch ทำสับสน | ตัดสิน canonical + sync ทุก surface ใน commit เดียว (เป้า 146) |
| M-6 | VisualOverhaul.cpp:274-343,339 | RebuildMotes+CreateMeshSection+LoadObject **ทุกเฟรม** | throttle ~15Hz / UpdateMeshSection + cache material |
| M-7 | BESTIARY_CODEX | 214 + ระบบธาตุเก่า (S1+S2) | regenerate (229 + canon ใหม่) หรือป้าย HISTORICAL |
| M-8 | README:25 | 140 ทับ 141 (S4) | sync เป็น 141 |
| M-9 | ON_PC_TASKS:202 | expected x140 จะหลอกวันเครื่องจริง (S5) | แก้เป็น 141 ก่อน machine day |
| M-10 | LIVE_STATE:75 | 140 ขัด §1 (S6) | sync |
| M-11 | FINAL_READINESS:184 | 133 ขัด header (S7) | sync |
| M-12 | TEST_INVENTORY | ย่อหน้า 119 (S8) | ลบ/mark historical |
| M-13 | VILLAGES_SKIFF:5 | 214 roster (S9) | แก้ 229 |
| M-14 | setup_input_assets.py:8 | 32 IA vs 33 action (F6 หาย) | เติม AWD_ToggleHUD |
| M-15 | DEFINITION_OF_DONE | ยุค 2026-08-29 ทั้งไฟล์ (S11) | ป้าย HISTORICAL/regenerate |

### Low (18)
L-1 WorldBootstrapper.cpp:364 intensity ค่าตายถูกเขียนทับ · L-2 GameMode.cpp:196 GetGameInstance ไม่เช็ค · L-3 Toggle*Screen copy-paste cascade (12 จุด/จอใหม่) · L-4 ไม่มี RemoveDynamic (85 จุด — ปลอดภัยด้วย weak ref) · L-5 Content/ThirdPerson เศษ template · L-6 AndroidFileServer config ตาย · L-7 VisualOverhaul.cpp:245 float drift · L-8 director+PPVolume spawn บน dedicated server · L-9 toggle กลางเกมถอด grade ไม่ได้ (boot-time) · L-10 bEnableVisualOverhaul ไม่ replicate (จงใจ?) · L-11 LIVE_STATE แถว 28/96 บีดเป็นบรรทัดเดียว · L-12 validate_final_run.py:8 docstring 140 · L-13 MASTER_PLAN phantom ref · L-14 CURRENT_HEAD เก่า · L-15 197/45 ไฟล์เก่า · L-16 135/140 เศษเอกสาร · L-17 orphan LFS 3,939 อัน (prune ต้อง owner) · L-18 branch ขยะ bot 2 สาย (ลบได้)

## 11. แผน 5 ระยะ

| ระยะ | งาน | ผล |
|---|---|---|
| 0 วันนี้ | Quick wins: M-1, M-2, H-2, M-14 (ล้วน one-liner) | ลดความเสี่ยง LAN/HUD/วันเครื่องจริงทันที |
| 1 UNIFY-1 | tag ทั้งสอง tip → merge final-completion เข้า integration (resolve 15 ไฟล์: AutomationTests=union, design_data=regen, เลข=146) → validators 4 ตัว → **stage เฉพาะไฟล์ที่ resolve** → push → ff final-completion → tag v3.0-unified | เกมเดียวรวม gameplay ล่าสุด + visual โดยไม่ force-push |
| 2 เช็ดเอกสาร | S1-S14 + L-11/L-12 | เอกสาร current-facing ตรง validator |
| 3 ENGINE-RUN-1 | hour 0-8 ตาม ON_PC_TASKS บน Windows UE 5.8.2 | RUN-VERIFIED ครั้งแรกของยุค |
| 4 ประกาศผล | tag/release แรก/default branch flip/ตัดสิน EchoDex | ผู้เล่นโหลดได้จริง |

**วินัย git ตลอดไป**: ห้าม `git add -A` (stage เฉพาะรายชื่อไฟล์) · ทุกคำสั่งต้องมี git-lfs ใน PATH + `GIT_LFS_SKIP_SMUDGE=1` เมื่อไม่ต้องการไบนารี

## 12. ภาคผนวก (สรุปย่อ)

- **LFS**: pointer 588 ที่ tip / oid 604 ใน history 220 commits (อธิบายเลข 604/604) · ท้องถิ่นมีจริง 91/604 → build offline ไม่ได้ · สุ่ม 10/10 sha256 ผ่าน · orphan 3,939 อันรอ prune · phantom dirt ทั้ง 3 checkout = byte-identical 100% (สุ่ม 137 ไฟล์)
- **CI**: .github/workflows/release.yml (self-hosted Windows runner) ยังไม่เคยรันเพราะไม่มี release
- **ทวนซ้ำ**: `python3 Scripts/validate_final_run.py` ที่ tip ใดๆ · นับ test ด้วย `git show <branch>:...AutomationTests.cpp` · merge-tree 4c706fc adc24ce c84d889 = 15 ไฟล์/22 hunks · hash-object เทียบ HEAD พิสูจน์ phantom
- การตรวจทั้งหมดเป็น read-only ต่อทุก branch — ไม่มีการแก้โค้ด/commit/push ใดๆ กับ branch ของโครงการ (ยกเว้นการเพิ่มไฟล์รายงานนี้ใน branch `agent/audit-v1v2`)
- บันทึกการทำงานของ 4 หน่วยตรวจ: ดู worklog กลางของระบบ (รายการ 2-a, 2-b, 2-c, 2-d)
