# ASTRAWILD — Prompt Pack สำหรับ Antigravity AI

## วิธีใช้

ให้เปิดโปรเจกต์ `ASTRAWILD` ใน Antigravity AI แล้ววาง **Master Prompt** ก่อน จากนั้นส่ง Prompt ระยะที่ 1 ถึง 8 ทีละชุด ไม่ควรส่งทุกระยะพร้อมกัน เพราะแต่ละระยะต้อง Compile, เปิด Editor, ทดสอบ และแก้ปัญหาก่อนเข้าสู่ระยะถัดไป

Repository หลักคือ [https://github.com/banksaisuoy/astrawild-game](https://github.com/banksaisuoy/astrawild-game) เป็น repository แบบ Private โปรเจกต์ใช้ Unreal Engine 5.8 และโมดูล C++ ชื่อ `AstrawildCore` หาก Antigravity AI พบว่าเครื่องใช้ Unreal Engine คนละรุ่น ห้ามเปลี่ยนรุ่นเอง ให้แจ้งปัญหาและเสนอทางเลือกก่อน

> **หลักสำคัญ:** ห้ามรายงานว่า “ทำเสร็จแล้ว” หากยังไม่ได้ตรวจไฟล์ที่เปลี่ยนแปลง, Compile หรือ Playtest ตามที่เครื่องรองรับ หากเครื่องไม่สามารถเปิด Unreal Editor ได้ ให้สร้างไฟล์ที่จำเป็นและรายงานอย่างตรงไปตรงมาว่ายังไม่ได้ Compile/Run

---

# 1. Master Prompt — บทบาทและข้อกำหนดหลัก

```text
คุณคือ Lead Unreal Engine Developer, Gameplay Programmer และ Technical Designer ของโปรเจกต์ ASTRAWILD: Echoes of the First Dawn

เป้าหมายของคุณคือพัฒนา Vertical Slice ที่เปิดใน Unreal Engine แล้วกด Play ได้จริง โดยเริ่มจากระบบเล็กแต่เชื่อมกันครบ ไม่สร้างระบบจำนวนมากแบบแยกส่วนที่ยังเล่นไม่ได้

ข้อมูลโครงการ:
- Engine target: Unreal Engine 5.8
- Project: ASTRAWILD.uproject
- Primary C++ module: AstrawildCore
- Platform แรก: Windows PC
- กล้อง: Third-person
- เป้าหมาย Vertical Slice: แผนที่ทดลองหนึ่งพื้นที่, ตัวละครหนึ่งตัว, Echo 3 ชนิด, เก็บทรัพยากร, ต่อสู้, จับ Echo, คราฟต์, สร้างจุดพัก และ Save/Load
- Single-player เป็นแกนหลัก; ออกแบบข้อมูลให้ขยาย Co-op 1–4 คนได้ภายหลัง
- Performance target: 60 FPS บนเครื่องระดับกลางในฉากทั่วไป และต้องลดคุณภาพได้ด้วย preset
- IP ต้องเป็นของ ASTRAWILD เอง ห้ามคัดลอกชื่อ รูปลักษณ์ เสียง โค้ด แผนที่ หรือ asset เฉพาะของ Palworld หรือเกมอื่น

กฎการทำงาน:
1. ตรวจสอบโครงสร้าง repository และสถานะ Git ก่อนแก้ไขไฟล์
2. อ่าน README.md และเอกสารใน Docs ก่อนตัดสินใจเชิงสถาปัตยกรรม
3. แยก C++ สำหรับกฎระบบที่ต้องเสถียร ออกจาก Blueprint/Data Asset สำหรับคอนเทนต์ที่ต้องปรับบ่อย
4. ใช้ Stable ID และ Data-Driven Design สำหรับ Echo, Item, Recipe, Ability, Quest และ Building
5. ห้ามลบหรือเขียนทับไฟล์สำคัญโดยไม่แสดงแผนและขออนุญาต
6. ห้ามเปลี่ยน Unreal Engine version, plugin หลัก, ชื่อโมดูล หรือโครงสร้าง save โดยพลการ
7. ทุกฟีเจอร์ต้องมี error handling, save/load consideration, debug output และ acceptance criteria
8. ทุกระยะต้องสรุปไฟล์ที่แก้ ปัญหาที่พบ วิธีทดสอบ ผล Compile/Run และสิ่งที่ยังไม่เสร็จ
9. ห้ามใช้ asset จากอินเทอร์เน็ตที่ไม่ทราบ license หากยังไม่มี asset ให้ใช้ placeholder ที่สร้างเองหรือ primitive ของ Unreal
10. อย่าเพิ่ม Multiplayer เต็มรูปแบบก่อน Single-player core loop เล่นได้จริง

วิธีทำงานสำหรับแต่ละงาน:
- เริ่มด้วยสรุปความเข้าใจและรายการไฟล์ที่จะสร้าง/แก้
- ตรวจ dependency และความเข้ากันได้กับ Unreal Engine 5.8
- ลงมือแก้ทีละส่วนเล็ก ๆ
- Compile หรือ run validation เมื่อทำได้
- ทดสอบกรณีปกติและกรณีผิดพลาด
- สรุปผลเป็น Handoff Report

เมื่อจบแต่ละระยะ ให้รายงานในรูปแบบนี้:
STATUS: COMPLETE / PARTIAL / BLOCKED
CHANGED FILES:
- รายการไฟล์
IMPLEMENTED:
- สิ่งที่ทำสำเร็จ
TESTED:
- คำสั่งหรือวิธีทดสอบ
- ผล Compile/Editor/Playtest
KNOWN ISSUES:
- ปัญหาที่ยังเหลือ
NEXT STEP:
- ขั้นตอนถัดไปที่แนะนำสำหรับ Manus AI
GIT:
- branch และ commit hash หากทำ commit ได้
```

---

# 2. Prompt ระยะที่ 1 — ตรวจสอบและทำให้โปรเจกต์เปิดได้

```text
เริ่มจากตรวจสอบโปรเจกต์ ASTRAWILD โดยยังไม่เพิ่มระบบเกมใหม่

งานที่ต้องทำ:
1. ตรวจสอบ ASTRAWILD.uproject, Source, Config, Content และ Docs
2. ตรวจสอบว่าโมดูล AstrawildCore, Build.cs และ Target.cs มีชื่อสอดคล้องกัน
3. ตรวจสอบ JSON ของ .uproject และ syntax ของ C++/Config
4. ตรวจสอบ plugin ที่เปิดใช้ว่ามีอยู่ใน Unreal Engine 5.8
5. สร้าง project files หรือ regenerate project files ตามวิธีที่ถูกต้องของเครื่อง
6. Compile Development Editor หากเครื่องมี Unreal Engine และ compiler พร้อม
7. หาก compile ไม่ผ่าน ให้แก้เฉพาะ error ที่จำเป็นและรายงานสาเหตุ
8. สร้างโฟลเดอร์ Content/ASTRAWILD/Maps/Prototype, Blueprints, Data, UI และ Audio พร้อม README หรือ placeholder ที่จำเป็น
9. ห้ามสร้าง asset ภายนอกหรือดาวน์โหลดไฟล์ที่ไม่ทราบ license

เกณฑ์ผ่าน:
- เปิด ASTRAWILD.uproject ได้
- โมดูล AstrawildCore โหลดได้ หรือมีรายงาน error ที่ชัดเจน
- Editor ไม่ปิดตัวเองจาก configuration error
- โครงโฟลเดอร์สำหรับ Vertical Slice พร้อมใช้งาน
- ส่ง Handoff Report ตามรูปแบบใน Master Prompt
```

---

# 3. Prompt ระยะที่ 2 — ทำตัวละคร กล้อง และอินพุตให้เล่นได้

```text
พัฒนา Player Vertical Slice ของ ASTRAWILD โดยเน้นการควบคุมที่ตอบสนองและกด Play ได้จริง

งานที่ต้องทำ:
1. สร้างหรือปรับ Character/Player Controller สำหรับมุมมอง third-person
2. รองรับเดิน, วิ่ง, กระโดด, หลบ และกล้องหมุนด้วยเมาส์/คอนโทรลเลอร์
3. ใช้ Enhanced Input และสร้าง Input Actions/Mapping Context แบบเป็นระบบ
4. แยกค่าความเร็ว การหมุนกล้อง ความไว และ stamina เป็นค่าที่ปรับได้
5. ทำพื้นทดสอบที่มีพื้นต่างระดับ ขอบ และพื้นที่ชน เพื่อทดสอบกันติด
6. เพิ่ม interaction trace สำหรับเก็บของ เปิดจุดพัก และอ่านวัตถุ
7. ทำ HUD ขั้นต่ำ: health, stamina, interaction prompt และ crosshair/reticle ถ้าจำเป็น
8. เพิ่ม debug commands หรือ debug overlay สำหรับตรวจ movement และ interaction

ข้อจำกัด:
- อย่าใส่ระบบหิว กระหาย หรือ inventory เต็มรูปแบบในระยะนี้
- อย่าเพิ่ม asset ภายนอก
- อย่าผูก logic สำคัญไว้ใน Level Blueprint เพียงอย่างเดียว

เกณฑ์ผ่าน:
- กด Play แล้วเดินและควบคุมกล้องได้
- อินพุตไม่ทำงานซ้ำเมื่อเปิด/ปิดเมนู
- สามารถโต้ตอบกับ placeholder ได้
- มี fallback เมื่อ Input Action หรือ asset หาย
- Compile และ Playtest ผ่าน หรือรายงาน blocker อย่างละเอียด
```

---

# 4. Prompt ระยะที่ 3 — ระบบ Echo แบบ Data-Driven

```text
สร้างระบบ Echo ต้นแบบของ ASTRAWILD โดยใช้ Data Asset และระบบที่ขยายชนิดใหม่ได้ง่าย

ออกแบบและทำระบบต่อไปนี้:
1. EchoDefinition Data Asset: ชื่อ, Stable ID, ธาตุ, บทบาท, base stats, icon, mesh reference และความสามารถ
2. EchoInstance: instance ID, level, health, trust, personality tags, experience และสถานะการเป็นเจ้าของ
3. Echo Actor/Pawn ที่อ่านค่าจาก EchoDefinition แทนการ hard-code แต่ละตัว
4. Echo 3 ชนิดแบบ placeholder ที่บทบาทต่างกันจริง:
   - Echo สายสำรวจ: ช่วยตรวจร่องรอยหรือเคลื่อนที่
   - Echo สายต่อสู้: มี attack และป้องกันผู้เล่น
   - Echo สายฐาน: มี utility สำหรับงานหรือผลิตของ
5. ระบบ spawn/despawn เบื้องต้นในพื้นที่ทดสอบ
6. ระบบ interaction และแสดงชื่อ/สถานะ Echo
7. debug command สำหรับ spawn Echo ตาม Stable ID
8. เตรียม interface สำหรับ AI, combat, capture และ save โดยไม่ทำให้ระบบพึ่งกันแน่นเกินไป

เกณฑ์ผ่าน:
- เพิ่ม Echo ชนิดใหม่ด้วย Data Asset ได้โดยไม่ต้องคัดลอก class ใหม่
- Echo แต่ละชนิดแสดงบทบาทและค่าสถานะต่างกัน
- Actor ที่ถูก spawn ไม่ทำให้เกม crash เมื่อ asset บางตัวเป็น null
- มี log ชัดเจนเมื่อ Data Asset ไม่ครบ
- Compile/Playtest และรายงานไฟล์ที่เปลี่ยน
```

---

# 5. Prompt ระยะที่ 4 — Combat, Damage และ Ability

```text
สร้างระบบต่อสู้ต้นแบบที่อ่านง่ายและตอบสนองดีสำหรับ ASTRAWILD

งานที่ต้องทำ:
1. สร้าง health/attribute system ที่แยกจาก UI
2. ทำ damage interface และ damage event ที่ใช้ได้กับ player, Echo และศัตรู
3. ทำโจมตีระยะประชิดหรือระยะไกลหนึ่งแบบสำหรับผู้เล่น
4. ทำ attack/ability อย่างน้อยหนึ่งแบบต่อ Echo ต้นแบบ
5. เพิ่ม hit reaction, attack telegraph, cooldown และสถานะชั่วคราวหนึ่งชนิด
6. ใช้ Gameplay Tags/Ability System เฉพาะส่วนที่เหมาะสม ไม่ทำทุก interaction เป็น Ability
7. เพิ่ม training target หรือศัตรู placeholder ในพื้นที่ทดสอบ
8. แสดงผล health/damage ด้วย feedback ทางภาพ/เสียง placeholder ที่ไม่รบกวนการอ่าน
9. ตรวจสอบว่า damage ไม่ถูกเรียกซ้ำจาก collision หรือ animation notify โดยไม่ตั้งใจ

เกณฑ์ผ่าน:
- ผู้เล่นโจมตีเป้าหมายและเห็นผลลัพธ์ชัดเจน
- เป้าหมายตอบสนองและถูกกำจัดได้
- cooldown และสถานะไม่ค้างเมื่อ actor ถูกทำลาย
- มี debug log สำหรับ damage source/target/amount
- ระบบต่อยอดไป server authority ได้ในอนาคต
```

---

# 6. Prompt ระยะที่ 5 — Capture และ Relationship

```text
สร้างระบบจับ Echo และความสัมพันธ์แบบต้นแบบ โดยไม่ทำให้เป็นเพียงการกดปุ่มสุ่ม

งานที่ต้องทำ:
1. เพิ่ม CaptureComponent หรือ service แยกจาก Echo Actor
2. สร้าง capture state: พบ, อ่อนแรง, พร้อมจับ, จับสำเร็จ, ล้มเหลว และ cooldown
3. ให้การจับตรวจสอบระยะ, target validity, health/state และอุปกรณ์ที่ใช้
4. สร้าง capture item/recipe แบบ Data-Driven
5. เมื่อจับสำเร็จ ให้สร้าง EchoInstance ที่มี Stable Instance ID
6. เพิ่ม trust/relationship value และเริ่มจากค่าที่แตกต่างตามวิธีจับ
7. เพิ่ม UI feedback ว่าจับได้หรือไม่ พร้อมเหตุผลเมื่อจับไม่ได้
8. แสดง Echo ที่จับได้ในหน้ารายการทีมแบบง่าย
9. ทำ save contract สำหรับ EchoInstance แต่ยังไม่ต้องทำระบบ save เต็มรูปแบบถ้ายังไม่พร้อม

ข้อห้าม:
- ห้ามสร้าง Echo ซ้ำเมื่อ request เดิมถูกประมวลผลซ้ำ
- ห้ามให้ client เป็นผู้ตัดสิน capture success หากมีการเตรียม Multiplayer
- ห้ามใช้ชื่อหรือกลไกเฉพาะจากเกมอื่น

เกณฑ์ผ่าน:
- จับ Echo ได้อย่างน้อย 3 ชนิด
- Echo ที่จับได้ยังอยู่ในทีมหลังออกจากพื้นที่และเปิด session ใหม่ หากระบบ save พร้อม
- สถานะผิดพลาดมีข้อความที่เข้าใจได้
```

---

# 7. Prompt ระยะที่ 6 — Inventory, Crafting และ Base จุดพัก

```text
สร้างระบบ progression ขั้นต้นจากทรัพยากรไปสู่การคราฟต์และการสร้างฐาน

งานที่ต้องทำ:
1. สร้าง ItemDefinition และ ItemStack พร้อม Stable ID
2. ทำ InventoryComponent ที่เพิ่ม ลด ย้าย และตรวจจำนวนได้
3. ทำ resource node placeholder อย่างน้อยไม้ หิน และเศษพลังงาน
4. ทำ RecipeDefinition และ Crafting Service
5. ทำ UI inventory/crafting แบบง่ายที่รองรับเมาส์และคอนโทรลเลอร์ในระดับพื้นฐาน
6. สร้าง building preview แบบ grid/snap สำหรับจุดพักหนึ่งชนิด
7. ตรวจสอบ collision, ระยะ, พื้นที่วาง และวัตถุดิบก่อนยืนยันการสร้าง
8. สร้าง Rest Point ที่ใช้เป็นจุดพักหรือจุดเกิดใหม่
9. แยกข้อมูล Item/Recipe/Building จาก Actor logic
10. เพิ่ม log และ error feedback เมื่อวัตถุดิบไม่พอหรือวางไม่ได้

เกณฑ์ผ่าน:
- เก็บทรัพยากร → เห็นใน inventory → คราฟต์ → วางจุดพักได้
- การกดสร้างซ้ำไม่สร้างของซ้ำหรือหักวัตถุดิบซ้ำ
- ย้าย/รื้อจุดพักทำงานตามกฎที่กำหนด
- ระบบมี contract พร้อมขยายเป็นฐานหลายชิ้นในอนาคต
```

---

# 8. Prompt ระยะที่ 7 — แผนที่ทดลอง AI และ Performance

```text
ประกอบ Vertical Slice map ของ ASTRAWILD ให้เป็นพื้นที่ทดลองที่มีเส้นทางและการทดสอบครบ

งานที่ต้องทำ:
1. สร้างหรือจัดระเบียบ map ทดสอบด้วย primitive/placeholder ที่มี license ปลอดภัย
2. ทำ landmark หนึ่งจุด, resource route หนึ่งเส้น, จุดอันตรายหนึ่งจุด และจุดพักหนึ่งจุด
3. เพิ่ม AI behavior ขั้นต้น: idle, wander, detect player, attack, flee และ return
4. แยก Echo ที่ใกล้ผู้เล่นออกจาก simulation ที่ไม่จำเป็นสำหรับระยะไกล
5. เพิ่ม spawn rules ที่ไม่สร้าง actor ซ้ำหรือเพิ่มจำนวนไม่จำกัด
6. เปิด debug visualization สำหรับ AI perception, target, state และ path
7. ตรวจ Game Thread, Render Thread, GPU และ memory จากฉากทดสอบ
8. ทำ scalability settings ระดับ Low/Medium/High อย่างน้อยสำหรับ shadows, effects, foliage/placeholder และ view distance
9. ห้ามเปิดฟีเจอร์กราฟิกแพงโดยไม่มีผลการวัดบนเครื่องเป้าหมาย

เกณฑ์ผ่าน:
- ผู้เล่นรู้ว่าควรไปทางไหนโดยไม่ต้องมีแผนที่สมบูรณ์
- AI ไม่ค้างเป็นวงหรือทะลุฉากในกรณีพื้นฐาน
- ไม่มี spawn loop หรือ memory growth ชัดเจนระหว่างเล่นนาน
- มี performance report พร้อมข้อมูลที่วัดจริง
```

---

# 9. Prompt ระยะที่ 8 — Save/Load และ QA

```text
ทำระบบ Save/Load สำหรับ Vertical Slice โดยให้ข้อมูลมี version และกู้คืนได้

งานที่ต้องทำ:
1. แยก PlayerProfile, WorldSnapshot และ SettingsProfile
2. ทุกข้อมูลที่บันทึกต้องมี schema version และ Stable ID
3. ทำ SaveGame object หรือ subsystem ที่รวมการ serialize ไว้จุดเดียว
4. รองรับ manual save และ autosave ที่ไม่เขียนทุก frame
5. เขียน temporary save แล้วเปลี่ยนชื่อแบบปลอดภัย พร้อม backup slot อย่างน้อยหนึ่งชุด
6. ทำ migration function สำหรับการเปลี่ยน schema ในอนาคต
7. ตรวจกรณีปิดเกมระหว่าง save, actor ถูกลบ, item count ผิด และ asset reference หาย
8. โหลดกลับมาแล้วต้องคืนค่า inventory, Echo team, trust, rest point และ quest/progression ขั้นต้น
9. เพิ่ม UI สถานะกำลัง save, save สำเร็จ และ save ล้มเหลว
10. เขียน automated/manual test checklist สำหรับ save/load

เกณฑ์ผ่าน:
- ปิดเกมแล้วเปิดใหม่ ข้อมูลหลักยังถูกต้อง
- ไม่เกิด item duplication จากการ save/load ซ้ำ
- หาก save เสีย ระบบแจ้งผู้เล่นและพยายามใช้ backup
- มีรายงาน schema/version และ test result
```

---

# 10. Prompt ระยะที่ 9 — ตรวจรับ, Build และส่งต่องานให้ Manus AI

```text
เตรียม ASTRAWILD Vertical Slice สำหรับส่งต่อให้ Manus AI และทีมพัฒนาขั้นถัดไป

งานที่ต้องทำ:
1. ตรวจ git status และรายการไฟล์ที่เปลี่ยนทั้งหมด
2. ตรวจว่าไม่มี Binaries, Intermediate, Saved หรือ DerivedDataCache ถูก commit
3. ตรวจว่าไฟล์ binary ที่จำเป็นถูกจัดตาม Git LFS policy
4. Compile Development Editor และ Development Game หากเครื่องรองรับ
5. เปิด Editor และกด Play ทดสอบ core loop:
   - เริ่มเกม
   - เดินและควบคุมกล้อง
   - เก็บทรัพยากร
   - ต่อสู้
   - จับ Echo
   - คราฟต์
   - สร้างจุดพัก
   - Save
   - ปิด/เปิดใหม่
   - Load
6. ทดสอบกรณีผิดพลาดที่สำคัญอย่างน้อย 5 กรณี
7. สร้างหรืออัปเดต `Docs/BUILD_STATUS.md`
8. commit เป็นข้อความที่ชัดเจน เช่น `feat: playable astralwild vertical slice prototype`
9. ห้าม push ไป branch main หากมี policy ของผู้ใช้กำหนดไว้ต่างกัน ให้สร้าง branch และรายงานชื่อ branch
10. ส่ง Handoff Report ตามแบบด้านล่าง

Handoff Report:
STATUS: COMPLETE / PARTIAL / BLOCKED
ENGINE:
- Unreal version
- Compiler version
BUILD:
- Compile result
- Play result
- Package result, if available
CORE LOOP:
- รายการที่เล่นได้จริง
CHANGED FILES:
- รายการไฟล์หรือโฟลเดอร์สำคัญ
TEST RESULTS:
- Test case และผล
KNOWN ISSUES:
- ปัญหาและความรุนแรง
GIT:
- branch
- latest commit hash
- push status
NEXT FOR MANUS AI:
- งานถัดไปที่ควรทำตามลำดับความสำคัญ
```

---

# 11. Prompt สำหรับส่งงานกลับให้ Manus AI

ใช้ Prompt นี้หลัง Antigravity AI ทำงานเสร็จ เพื่อให้มีข้อมูลครบก่อนส่งต่อ:

```text
สร้าง Handoff Report ของ ASTRAWILD สำหรับ Manus AI โดยห้ามสรุปกว้าง ๆ ให้ระบุข้อมูลจริงจากโครงการเท่านั้น

ต้องมี:
1. Unreal Engine version และ compiler version
2. สถานะ compile พร้อม error/warning ที่เหลือ
3. วิธีเปิดและวิธีทดสอบ core loop
4. รายการไฟล์ที่สร้าง/แก้/ลบ พร้อมเหตุผล
5. รายการ Blueprint, C++, Data Asset, Map และ Config ที่มีอยู่จริง
6. ระบบใดที่ทำงานแล้ว ระบบใดเป็น placeholder และระบบใดยังไม่ได้ทำ
7. ผล Save/Load test
8. ผล performance test และเครื่องที่ใช้ทดสอบ
9. Git branch, latest commit, push status และไฟล์ที่ยังไม่ได้ track
10. Known issues พร้อมระดับ Blocker/High/Medium/Low
11. ขั้นตอนที่ Manus AI ควรทำต่อ โดยเรียงลำดับจากสำคัญที่สุด

หากยัง Compile หรือ Play ไม่ได้ ให้ใช้ STATUS: BLOCKED และระบุ error เต็ม ๆ พร้อมไฟล์และบรรทัดที่เกี่ยวข้อง ห้ามใช้ STATUS: COMPLETE เพื่อให้ดูเหมือนเสร็จ
```

---

## สิ่งที่ต้องส่งกลับมาให้ Manus AI หลัง Antigravity ทำเสร็จ

ให้คัดลอก Handoff Report ทั้งหมดกลับมาในแชต พร้อมบอกว่าแก้เสร็จบน branch ใด ถ้ามีไฟล์ ZIP หรือ screenshot ของ Error ให้แนบมาด้วย โดยเฉพาะกรณี Compile ไม่ผ่านหรือ Unreal Editor เปิดไม่ได้

ถ้า Antigravity AI ทำเสร็จเพียงบางระยะ ให้ส่งรายงานกลับมาได้เลย ไม่ต้องรอให้ทั้งเกมเสร็จ เพราะ Manus AI จะตรวจสถานะต่อและช่วยวางงานระยะถัดไปจากไฟล์จริงและผลทดสอบจริง
