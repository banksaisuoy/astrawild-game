# ASTRAWILD — Gemini Antigravity Run Scripts (Batch 8+)

> คัดลอก "Prompt" ด้านล่างไปวางใน Antigravity ได้เลย — ทุกอย่างที่มันต้องรู้อยู่ใน repo
> แล้ว (เริ่มที่ `ANTIGRAVITY_START_HERE.md`)
>
> โปรโตคอลสำคัญ: **ถ้า Antigravity ติดอะไรก็ให้มันเขียน log ลง `Docs/ENGINE_LOGS/` แล้ว
> commit+push** — ผู้ดูแล (GLM/Z.ai) จะอ่าน log แล้วแก้ source-side ให้ในรอบถัดไป

---

## PROMPT 1 — Build & Smoke Test (ใช้อันนี้ก่อนเสมอ)

```
You are working on ASTRAWILD — a UE5.8 C++ open-world survival + creature-catching
game. Repository: banksiasuoy/astrawild-game (this checkout).

READ FIRST (in this order):
1. ANTIGRAVITY_START_HERE.md
2. Docs/ASTRAWILD_BUILD_STATUS.md (current state + known issues)
3. Docs/ASTRAWILD_ENGINE_VERIFICATION_QUEUE.md (your task list)

YOUR TASK THIS SESSION — "Batch 8 build & smoke test":
1. Right-click ASTRAWILD.uproject -> Generate project files (VS 2022 / latest).
2. Build the AstrawildCore module in Development Editor config.
3. If COMPILE ERRORS occur:
   a. Fix ONLY mechanical/local issues (missing includes, typos, wrong enum names).
   b. Do NOT redesign systems or delete features.
   c. For anything structural you cannot fix in <10 lines, STOP, write the error
      to Docs/ENGINE_LOGS/BATCH8_BUILD_LOG.md (template below), commit + push.
4. If the build SUCCEEDS: launch the editor, PIE (Play In Editor), and run the
   golden path from Docs/ASTRAWILD_ENGINE_VERIFICATION_QUEUE.md Phase A rows
   1-10 (new-game -> explore -> village -> board skiff -> fly to isles).
5. Run the automation suite: Tools -> Test Automation -> Astrawild (31 tests).
6. Write your results to Docs/ENGINE_LOGS/BATCH8_PLAYTEST_LOG.md, commit + push
   with message "test: batch 8 engine verification round <n>".

ENGINE LOG TEMPLATE (Docs/ENGINE_LOGS/<NAME>.md):
---
# <Log name> — <date> — <agent>
## Environment
UE version / compiler / GPU:
## What I did
## What failed (exact error text + file:line)
## What I suspect
## Suggested source-side fix (if known)
---

Batch 8 highlights you will see in the world (all expected, not bugs):
- TWELVE zones (world is now 3.2km x 2.4km): sea (Azure Shallows), islands
  (Tidebreaker Isles), desert (Sunscar), highlands (Stormcrest), jungle
  (Verdant Reach), coral sea (Pearlsea Reef).
- 214 Echo species with procedural bodies (8 body plans, tints) — every creature
  looks different.
- Two villages: Dawnstead (camp, 8 NPCs) + Driftwood Landing (isles, 3 NPCs).
  Villagers walk waypoints, gather at the campfire at night, guards fight
  hostiles.
- Dawn Skiff aircraft: E to board, WASD/SPACE/CTRL/SHIFT to fly, E to dismount.
- Dungeon #2 "The Sunken Vault" in the Tidebreaker Isles (boss: the Dawnfang).
- Quests 9-10 continue the chain (Elder Rowan -> Kael).
```

---

## PROMPT 2 — ถ้าบิวด์ผ่านแล้ว: เล่น Golden Path เต็ม 23 ขั้น

```
Continue the ASTRAWILD engine verification session.

Build is green. Now walk the FULL golden path from
Docs/ASTRAWILD_ENGINE_VERIFICATION_QUEUE.md (all 23 stages), but with the
Batch 8 world:

1. NEW GAME -> spawn at Dawnstead (Dawn Fields, camp center is now at -400m, 0).
2. Talk to Warden Maren -> Quest 1. Gather, craft, capture a Lumewisp.
3. Explore the twelve-zone Vale: walk to at least 6 zone borders and confirm the
   HUD zone banner + discovery toast fire (n/12).
4. Board the Dawn Skiff at the camp pad (E). Fly: W/S thrust, A/D yaw,
   SPACE/CTRL altitude, SHIFT boost, E dismount. Fly to the Tidebreaker Isles
   (far south-west, across the sea). Confirm Quest 9 "Wings over the Vale"
   objectives complete (VisitZone + chart Driftwood Landing marker).
5. At Driftwood Landing: talk to Kael -> Quest 10. Descend into the Sunken Vault
   (4 rooms, gates, Dawnfang boss with 3 phases + telegraphs + weak points).
   Defeat the Dawnfang -> confirm Creature_VaultColossus quest completion.
6. Return to camp, assign Echoes to work sites, SAVE (F5) -> QUIT -> relaunch ->
   LOAD (F9) -> verify cleared rooms stay cleared, roster/party/base/power all
   restore (schema v3).
7. Night test: wait for 21:00 — villagers must gather at the campfire; guards
   must attack any hostile Echo dragged near the village.

Log everything to Docs/ENGINE_LOGS/BATCH8_GOLDENPATH_LOG.md (pass/fail per stage
+ screenshots if possible), commit + push: "test: batch 8 golden path <verdict>".
```

---

## PROMPT 3 — ถ้าเจอปัญหา: ให้ Antigravity แก้ + รายงาน

```
Continue the ASTRAWILD engine verification session.

You reported failures in your last log. Fix them following these rules:
1. Mechanical compile/runtime errors: fix directly (missing include, typo,
   wrong constant, null guard). Keep changes minimal and local.
2. Design/systemic problems (feature doesn't work end-to-end): do NOT redesign.
   Write a precise report in Docs/ENGINE_LOGS/BATCH8_FIX_REPORT.md:
   - exact repro steps
   - expected vs actual
   - the smallest code location you believe is responsible (file:line)
   - your suggested fix
3. Art/UX polish (shapes look rough, lights too dim, etc.): note it in the same
   report under "COSMETIC" — do not spend session time on it.
4. After fixes: rebuild, re-run the failed steps, update the log, commit + push
   "fix: batch 8 engine round <n>".
5. Never: force-push, rewrite history, delete Docs/, or "clean up" C++ you don't
   recognize — that's another agent's work.
```

---

## PROMPT 4 — Content/Art pass (เฉพาะเมื่อบิวด์+เกมเสถียรแล้ว)

```
Continue the ASTRAWILD engine session — content pass.

The C++ systems are stable. Now improve presentation WITHOUT touching gameplay
systems (all 14 REPLACE_BEFORE_RELEASE markers are yours to upgrade):
1. Replace placeholder shapes on the skiff, portals, dungeon gates with simple
   meshes/materials you build in-editor (keep the C++ classes untouched).
2. Give the 12 zones distinct sky/fog/lighting moods (per-zone PostProcess or
   light color tweaks via the existing AmbientLightColor data).
3. Add simple ambient audio (wind per biome, camp fire, skiff hum, boss roar).
4. Journal/Codex UI: surface the 214-species bestiary (data is all in
   UAstrawildItemRegistrySubsystem::FindEcho + CodexIndex + Family/BodyPlan/
   SizeClass/PrimaryTint fields — a UMG list can render it all).
Log + commit + push as "feat: batch 8 content pass <area>".
```

---

## หมายเหตุสำหรับเจ้าของโปรเจกต์ (คุณ)

- ใช้ **PROMPT 1** เปิดเซสชันใหม่ทุกครั้ง (มันจะอ่าน START_HERE เอง)
- ถ้ารอบแรกยังบิวด์ไม่ผ่าน (มันกำลังบิวด์อยู่ตอนนี้) — ปล่อยให้มันจบ แล้วดู
  `Docs/ENGINE_LOGS/` ใน GitHub: ทุกปัญหาที่มันเขียนจะถูกแก้ฝั่ง source โดย GLM
  ในรอบถัดไป (worklog อัปเดตต่อเนื่องทุก 15 นาที)
- ลำดับความสำคัญที่ตกลงกันไว้: **บิวด์ผ่าน > golden path 23 ขั้น > save/load
  3 รอบ > content/art pass**
