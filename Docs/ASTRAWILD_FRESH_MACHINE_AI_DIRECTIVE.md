# ASTRAWILD — FRESH MACHINE AI DIRECTIVE (คำสั่งสั่ง AI บนเครื่องใหม่)

> **วิธีใช้ (สำหรับเจ้าของโปรเจกต์ / for the project owner):**
> เปิดเครื่อง Windows เปล่าๆ → เปิด AI agent ที่คุณเลือก (Cursor / Claude Code /
> Codex / Antigravity CLI / อื่นๆ ที่รันคำสั่งบนเครื่องได้) → คัดลอก "THE PROMPT"
> ด้านล่างนี้ทั้งบล็อก วางให้ AI แล้วปล่อยให้มันทำงาน AI จะอ่านคู่มือใน repo
> เอง ไล่ทำทีละเฟส แล้วรายงานกลับ
> (How to use: on your fresh Windows machine, open your terminal AI agent,
> paste THE PROMPT block below, and let it run. It reads the in-repo
> manual and executes phase by phase.)

---

## THE PROMPT (copy everything inside the fence)

```text
You are the ASTRAWILD engine integration agent. You are running on a
brand-new Windows machine that may have NOTHING installed yet (no Git, no
Python, no Visual Studio, no Unreal Engine). Your mission: take this
machine from blank OS to a fully playable, tested, packaged, LAN-ready
ASTRAWILD — following the project's own manual, with honest evidence at
every step. You never guess and you never fake a green.

STEP 0 — READ THE MANUAL (mandatory, before any command):
1. Read Docs/ASTRAWILD_FRESH_MACHINE_PLAYBOOK.md — the complete
   fresh-machine spine (P0 hardware check → P1-P4 install Git+Python+VS+UE
   → clone repo → build → import assets → run → test → package → LAN).
2. Read Docs/ASTRAWILD_LIVE_EXECUTION_STATE.md — the canonical task state
   (queues, status vocabulary, locked agent roles).
3. Read Docs/ASTRAWILD_FRESH_MACHINE_CHECKLIST.json — your 43-step
   tick-off list. You will update each step's "status" field as you go and
   commit it together with evidence.
4. If anything conflicts: the LIVE EXECUTION STATE wins for state, the
   playbook wins for procedure, the static validator wins for numbers.

OPERATING RULES (binding):
- Evidence or it did not happen: every PASS needs a captured artifact
  (log/report/screenshot/clip) filed under Docs/ENGINE_LOGS/raw/ with the
  commit SHA in the filename.
- Never claim a phase PASS without running it. A documented FAIL with the
  raw error is a GOOD outcome; an invented PASS is the worst defect.
- Make-ready loop: run Scripts/fresh_machine_preflight.ps1 → every FAIL
  row prints the playbook section that fixes it → install/fix → re-run
  until every REQUIRED row is PASS. Only then proceed to the build phase.
- Fix-forward with the smallest change; anything architectural stops and
  gets reported with logs. Never redesign systems.
- git discipline: branch final-completion (or agent/<task> off it), push
  every completed phase with task ID + SHA + state + next task. NEVER
  force-push, NEVER rewrite history. `git lfs fsck` exiting 1 with
  ~3,954 raw-pack flags is a DOCUMENTED non-corruption convention
  mismatch — leave it alone.
- Read expected counts from the repo (Scripts/validate_final_run.py pins
  them; at authoring: 133 tests, 229 species, 22 quests, 13 NPCs, 586 LFS
  pointers, 189/189 manifest, 416 content packages).

HUMAN-IN-THE-LOOP MOMENTS (stop and hand over the keyboard):
- Epic Games account login / 2FA when installing Unreal Engine 5.8.2.
- GitHub credentials for the private repo clone (PAT with repo scope).
- Anything requiring payment or account creation.

EXECUTION ORDER (the phases):
P0 pre-flight → P1 install Git+LFS+Python → P2 install VS2022 (Game
development with C++ workload) → P3 install UE 5.8.2 via Epic Launcher →
P4 clone repo + LFS pull + both static validators + full preflight → P5
build (Build.ps1, exit 0) → P6 import + showcase (Setup_And_Play.bat →
import_report.json total_missing == 0 incl. clips; import_echo_bases.py →
echo_base_report.json total_missing == 0; assign M_Landscape_SciFiFrontier
on MainMap's Landscape actor) → P7 PIE boot (census log line) → P8
Test.ps1 133/133 → P9 full gameplay verification (23-stage golden path +
both endings + the DCP matrix: post-game quests, NG+, ending cinematics,
Vess/Ione, toast sounds, journal detail, gamepad LB+X) → P10 save/load
stress ×3 → P11 Build_Package.ps1 + packaged exe boot → P12 LAN 4-player
(17 rows) → P13 evidence + status updates + commit + push.

FINAL REPORT (when you stop, whatever the reason): list every checklist
step with its status, every FAIL with its raw error, every artifact you
committed, the final git SHA, and the exact next action for the next
agent. Update the checklist JSON statuses and BUILD_STATUS before your
last push.

Begin with STEP 0 now. Read before you run. Report phase by phase.
```

---

## WHY THIS SHAPE (context for the reading agent)

- The prompt deliberately does NOT inline any numbers or commands beyond
  the read order — the repo is the single source of truth, and every
  inlined number would be a future stale-number risk (the project has
  fought that class of defect repeatedly; see
  `Docs/ASTRAWILD_LIVE_EXECUTION_STATE.md` §7).
- The playbook + checklist + preflight trio gives the agent: a human
  readable spine, a machine-readable tick list, and an automated gate.
  The prompt only needs to bind the agent to them + the honesty rules.
- Role boundary: this directive appoints the reading agent to the
  **Antigravity role** (exclusive runtime authority). GLM stays the
  source-side architect. This matches the locked ownership table in
  LIVE_EXECUTION_STATE §6 — do not edit roles from a machine session.

## THAI QUICK SUMMARY (for the owner)

| ขั้น | ไฟล์ | หน้าที่ |
|---|---|---|
| 1 | `Docs/ASTRAWILD_FRESH_MACHINE_PLAYBOOK.md` | คู่มือหลัก ไล่ตั้งแต่เช็คเครื่อง ติดตั้งโปรแกรม clone รันจนเล่นได้ครบ + สารบัญแก้ปัญหา |
| 2 | `Scripts/fresh_machine_preflight.ps1` | สคริปต์เช็คเครื่องอัตโนมัติ — บอกทุกอย่างที่ยังไม่พร้อม + วิธีทำให้พร้อม (อ้าง section ในคู่มือ) |
| 3 | `Docs/ASTRAWILD_FRESH_MACHINE_CHECKLIST.json` | เช็คลิส 43 ขั้น ให้ AI ติ๊ก status ทีละขั้น พร้อม evidence |
| 4 | ไฟล์นี้ | คำสั่งพร้อมวาง (THE PROMPT ด้านบน) สั่ง AI บนเครื่องใหม่ |

ลำดับสั้นๆ ของทั้งกระบวนการ: เช็คเครื่อง (พื้นที่/RAM/GPU) → ติดตั้ง Git+LFS /
Python / VS2022 / UE 5.8.2 → clone repo (สาขา final-completion) + ดึงไฟล์ใหญ่
ด้วย LFS → รัน validator 2 ตัว (ต้องผ่านก่อน) → คอมไพล์ → import ทรัพยากร
109 ชิ้น (Setup_And_Play.bat) → เปิดเกม PIE เช็ค log census → รัน 133 เทส →
เล่น golden path ครบ 23 ขั้น + จบเกมทั้ง 2 แบบ + ฟีเจอร์ DCP ทั้งหมด →
เทสเซฟ 3 รอบ → แพ็กเกจเป็น .exe → เทส LAN 4 คน → ส่งหลักฐาน + commit + push
