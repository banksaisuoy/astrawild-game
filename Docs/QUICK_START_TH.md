# ASTRAWILD — Quick Start Guide (ภาษาไทย)
**วิธีทำให้เกมเล่นได้ภายใน 1 ชั่วโมง**

---

## สิ่งที่ต้องมีก่อนเริ่ม

1. **Unreal Engine 5.3+** ติดตั้งแล้ว
2. **Visual Studio 2022** พร้อม C++ Workload
3. **Git LFS** เปิดใช้งาน
4. **Quixel Bridge** (สำหรับโหลด Megascans ฟรี)

---

## 🚀 ขั้นตอนที่ 1: Compile โปรเจกต์ (10 นาที)

```bash
cd /workspace
# เปิด ASTRAWILD.uproject ด้วย UE Editor
# หรือใช้ Command Line:
"C:\Program Files\Epic Games\UE_5.3\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" ASTRAWILD.uproject -run=ShaderCompiler
```

**ใน UE Editor:**
1. เปิด `ASTRAWILD.uproject`
2. รอให้ Engine Compile C++ Code (เห็น Progress Bar)
3. เมื่อเสร็จ จะเห็น Console ไม่มี Error

---

## 🗺️ ขั้นตอนที่ 2: สร้าง Map แรก (20 นาที)

### 2.1 สร้าง Level ใหม่
1. ใน Content Browser → Right-click `Content/ASTRAWILD/Maps/`
2. เลือก **Level** → **Default**
3. ตั้งชื่อ `LVL_DawnFields`
4. Double-click เพื่อเปิด

### 2.2 วาง Object พื้นฐาน
**Place Actors Panel** (Shift+1):

| Actor | ตำแหน่งแนะนำ | จำนวน |
|---|---|---|
| Player Start | (0, 0, 100) | 1 |
| DirectionalLight | (0, 0, 5000) | 1 |
| SkyAtmosphere | (0, 0, 0) | 1 |
| ExponentialHeightFog | (0, 0, 0) | 1 |
| Skylight | (0, 0, 5000) | 1 |

**วาง Game Objects:**
1. Right-click ใน World Outliner
2. Spawn Actor → ค้นหา:
   - `AstrawildRestPoint` (จุดพัก/เซฟเกม)
   - `AstrawildCraftingStation` (ที่คราฟต์)
   - `AstrawildResourceNode` (ทรัพยากร)

### 2.3 สร้าง Landscape เบื้องต้น
1. Modes Panel → Landscape (Shift+3)
2. ตั้งค่า:
   - Size: 800x800 cm (เล็กสำหรับทดสอบ)
   - Sections: 7x7
   - Resolution: 63x63
3. ใช้ Sculpt Tool สร้างเนินเขาเล็กน้อย
4. กด **Build** ด้านล่าง

### 2.4 บันทึกและทดสอบ
1. File → Save Current
2. กด **Play** (Alt+P)
3. ผู้เล่นควร Spawn และเดินได้

---

## 📦 ขั้นตอนที่ 3: สร้าง Data Assets (30 นาที)

### 3.1 สร้าง Item Definitions
1. ใน Content Browser → Right-click `Content/ASTRAWILD/Data/Items/`
2. เลือก **Data Asset** → ค้นหา `AstrawildItemDefinition`
3. ตั้งชื่อ `DA_Item_Wood`
4. ใน Details Panel:
   - **Item Id:** `Item_Wood` (ต้องตรงกับ Manifest!)
   - **Display Name:** Dawnwood
   - **Category:** Material
   - **Weight:** 0.5
   - **Max Stack Size:** 200
5. Save

**ทำซ้ำสำหรับ Items หลัก (10 ชนิดแรก):**
- `DA_Item_Stone`
- `DA_Item_Fiber`
- `DA_Item_Berry`
- `DA_Item_RawMeat`
- `DA_Item_CrystalShard`
- `DA_Item_Resonator`
- `DA_Item_WaterFlask`
- `DA_Item_Bandage`
- `DA_Item_CookedMeat`
- `DA_Item_WoodPlank`

### 3.2 สร้าง Echo Definitions
1. Right-click `Content/ASTRAWILD/Data/Echoes/`
2. Data Asset → `AstrawildEchoDefinition`
3. ตั้งชื่อ `DA_Echo_Lumewisp`
4. ตั้งค่า:
   - **Echo Id:** `Echo_Lumewisp`
   - **Species Name:** Lumewisp
   - **Base HP:** 60
   - **Body Plan:** Spirit (หรือ Sphere สำหรับ Placeholder)
   - **Element:** Light
   - **Behavior:** Passive
5. Save

**ทำซ้ำสำหรับ 3 Echoes แรก:**
- `DA_Echo_Stonehide` (HP 100, Earth, Passive)
- `DA_Echo_Voltling` (HP 50, Electric, Skittish)
- `DA_Echo_Gloomfang` (HP 120, Dark, Aggressive) ← ศัตรูแรก!

### 3.3 สร้าง Recipe Definitions
1. Right-click `Content/ASTRAWILD/Data/Recipes/`
2. Data Asset → `AstrawildRecipeDefinition`
3. ตั้งชื่อ `DA_Recipe_Resonator`
4. ตั้งค่า:
   - **Recipe Id:** `Recipe_Resonator`
   - **Ingredients:** Stone x2, Fiber x1
   - **Output:** Resonator x1
   - **Craft Time:** 3.0 วินาที
5. Save

**Recipes สำคัญที่ต้องสร้าง:**
- `DA_Recipe_Bandage` (Fiber x2 → Bandage)
- `DA_Recipe_CookedMeat` (Raw Meat → Cooked Meat)
- `DA_Recipe_WoodPlank` (Wood x2 → Plank)

---

## 🎮 ขั้นตอนที่ 4: ทดสอบการเล่น (10 นาที)

### 4.1 ตรวจสอบ Input
1. Edit → Project Settings → Input
2. ตรวจสอบว่ามี Action Mappings:
   - Jump
   - Interact
   - Attack
   - Capture
3. ถ้าไม่มี → เพิ่มใหม่

### 4.2 ทดสอบ Gameplay Loop
**ใน Play Mode:**
1. เดินไปหา Resource Node
2. กด Interact (E) เพื่อเก็บทรัพยากร
3. เปิด Inventory (Tab) → เห็นของที่ได้
4. ไปที่ Crafting Station
5. Craft Resonator
6. หา Lumewisp ในป่า
7. กด Capture เพื่อจับ

### 4.3 แก้ปัญหาทั่วไป

| ปัญหา | สาเหตุ | วิธีแก้ |
|---|---|---|
| Player ตกโลก | ไม่มี Ground | วาง Plane หรือสร้าง Landscape |
| เดินไม่ได้ | Input ไม่ตั้งค่า | ตรวจสอบ Project Settings → Input |
| เก็บของไม่ได้ | Item Registry ว่าง | สร้าง Data Assets ให้ครบ |
| Craft ไม่ได้ | Recipe ไม่มี | สร้าง Recipe Definitions |
| Echo ไม่โผล่ | ไม่มี Spawner | วาง Echo Spawner ใน Level |

---

## 🎨 ขั้นตอนที่ 5: ทำให้ภาพสวยขึ้น (Optional)

### 5.1 ใช้ Quixel Megascans
1. เปิด Quixel Bridge (ฟรีกับ UE5)
2. โหลด Grass/Moss Textures
3. Drag & Drop ลงบน Landscape
4. สร้าง Material Instance

### 5.2 ตั้งค่า Lighting
1. เลือก DirectionalLight
2. ปรับ Rotation ให้เป็น Golden Hour (มุมต่ำ)
3. เพิ่ม Intensity เป็น 3-5
4. เปิด Cast Shadows

### 5.3 Post-Processing
1. Place Actor → PostProcessVolume
2. ตั้งค่า Infinite Extent (Unbound)
3. ปรับ:
   - Exposure: Min 8, Max 14
   - Bloom: 0.8
   - Color Grading: Warm Highlights

---

## ✅ Checklist การ playable

- [ ] Map สร้างแล้ว มี Player Start
- [ ] Lighting พื้นฐาน (DirectionalLight, SkyLight, Fog)
- [ ] Landscape หรือ Ground Plane
- [ ] Rest Point อย่างน้อย 1 จุด
- [ ] Resource Nodes (Wood, Stone)
- [ ] Crafting Station อย่างน้อย 1 อัน
- [ ] Item Data Assets (10 ชนิดแรก)
- [ ] Echo Data Assets (3 ชนิดแรก)
- [ ] Recipe Data Assets (3 สูตรแรก)
- [ ] ทดสอบ Play → เดิน เก็บของ คราฟต์ ได้

---

## 📞 ขั้นตอนต่อไป

เมื่อPlayable แล้ว ทำตาม `GAME_COMPLETION_PLAN.md`:
1. เพิ่ม Echo Species ให้ครบ 10
2. สร้าง Environment Art สวยๆ
3. ใส่ Character Meshes จริง
4. เพิ่ม Weapons และ VFX
5. ทำ Audio และ Music
6. Polish และ Optimize

---

## 🔗 เอกสารอ้างอิง

- `ASTRAWILD_ASSET_MANIFEST.md` — รายการ IDs ทั้งหมด
- `ASTRAWILD_ASSET_PIPELINE.md` — วิธี Override CODE_DEFAULT
- `astra_wild_art_content.md` — Art Direction
- `ASTRAWILD_TEST_PLAN.md` — วิธีทดสอบระบบ

---

## 💡 เคล็ดลับ

1. **เริ่มจาก Greybox ก่อน** — อย่าเพิ่งใส่ Asset สวยๆ จนกว่าจะเล่นได้
2. **Save บ่อยๆ** — UE Editor อาจ Crash ได้
3. **ใช้ Git Commit ทุกชั่วโมง** — กันข้อมูลหาย
4. **Test ใน Play Mode ตลอด** — อย่ารอจนเสร็จค่อยเทส
5. **อ่าน Log เมื่อมี Error** — Message มักบอกวิธีแก้

**เวลาโดยประมาณ:** 1-2 ชั่วโมง สำหรับ First Playable Slice
**ผลลัพธ์:** เกมที่เดิน เก็บของ คราฟต์ จับ Echo ได้ (ยังไม่งาม)
