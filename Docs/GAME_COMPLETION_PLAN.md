# ASTRAWILD — GAME COMPLETION MASTER PLAN
**เป้าหมาย:** เปลี่ยน C++ Prototype เป็นเกมที่เล่นได้จริง ภาพสวยงาม Commercial-grade
**วันที่:** 2026-08-31
**สถานะ:** Content/ASTRAWILD/ ว่างเปล่า — ต้องเริ่ม Asset Production

---

## 📋 ระยะที่ 1: ทำให้ "เล่นได้" ก่อน (Playable Slice)
*ลำดับความสำคัญสูงสุด — ต้องทำให้มีอะไรให้เล่นก่อน แล้วค่อยแต่งภาพ*

### 1.1 สร้าง Map แรกสำหรับทดสอบ (Week 1)
```
Content/ASTRAWILD/Maps/
├── LVL_DawnFields.umap          ← แผนที่เริ่มต้นสำหรับเล่น
├── LVL_TestArena.umap           ← สนามทดสอบ combat/capture
└── LVL_DungeonTest.umap         ← ทดสอบดันเจียน
```

**สิ่งที่ต้องทำ:**
1. เปิด UE Editor → สร้าง Level ใหม่
2. วาง Player Start, Rest Point, Workbench, Campfire
3. วาง Resource Nodes (Wood, Stone, Crystal)
4. วาง Echo Spawner สำหรับ Lumewisp, Stonehide, Voltling
5. Build Lighting → Save → Test Play

### 1.2 ตั้งค่า Game Mode & Input (Day 1)
```
Content/ASTRAWILD/Data/
├── DA_GameSettings.uasset
├── DA_InputMappingContext.uasset
└── DA_PlayerConfig.uasset
```

**สิ่งที่ต้องทำ:**
1. สร้าง GameMode Blueprint อ้างอิง `AstrawildGameState`
2. สร้าง Input Mapping Context (WASD, Jump, Interact, Attack, Capture)
3. ตั้งค่า Default GameMode ใน Project Settings

### 1.3 สร้าง Data Assets สำหรับเนื้อหา (Week 1-2)
ตาม `ASTRAWILD_ASSET_MANIFEST.md`:

| ประเภท | จำนวน | ไฟล์ที่ต้องสร้าง |
|---|---|---|
| Items | 23 | `DA_Item_Wood`, `DA_Item_Stone`, ... |
| Recipes | 18 | `DA_Recipe_Resonator`, `DA_Recipe_Bandage`, ... |
| Echo Species | 10 | `DA_Echo_Lumewisp`, `DA_Echo_Stonehide`, ... |
| Buildings | 13 | `DA_Building_Foundation`, `DA_Building_Wall`, ... |
| Technologies | 10 | `DA_Tech_BasicCrafting`, `DA_Tech_Cooking`, ... |
| Quests | 7 | `DA_Quest_FirstLight`, `DA_Quest_FirstEcho`, ... |

**วิธีสร้าง:**
1. ใน UE Editor → Right-click → Data Asset
2. เลือก Class: `AstrawildItemDefinition`, `AstrawildEchoDefinition`, ฯลฯ
3. ตั้งชื่อให้ตรงกับ ID ใน Manifest (สำคัญมาก!)
4. กรอกค่า Stats ตามเอกสาร
5. Save ไปที่ `Content/ASTRAWILD/Data/`

---

## 🎨 ระยะที่ 2: ทำให้ "ภาพสวย" (Visual Production)

### 2.1 Environment Art (Week 2-4)

#### Master Landscape Material
```
Content/ASTRAWILD/Materials/
├── M_Landscape_SciFiFrontier.uasset
├── MI_Landscape_Instance.uasset
├── T_Grass_Bioluminescent_*.uasset    (BaseColor, Normal, Roughness)
├── T_Cliff_Granite_*.uasset
├── T_Soil_Fertile_*.uasset
└── T_Sand_Beach_*.uasset
```

**วิธีทำ:**
1. สร้าง Material Function สำหรับ Layer Blend
2. ใช้ Landscape Layer Blend Node (4 layers)
3. Import Texture Pack (แนะนำ: Quixel Megascans — ฟรีกับ UE5)
4. สร้าง Material Instance สำหรับปรับสีแต่ละ Biome

#### Foliage & Vegetation
```
Content/ASTRAWILD/Environment/
├── Foliage/
│   ├── SM_Tree_SciFiBroadleaf.uasset
│   ├── SM_Tree_Conifer.uasset
│   ├── SM_Bush_Glowing.uasset
│   ├── SM_Fern_01.uasset
│   └── SM_Grass_Clump.uasset
├── Rocks/
│   ├── SM_Rock_Cluster_Large.uasset
│   └── SM_Rock_Cluster_Small.uasset
└── Crystals/
    ├── SM_Crystal_Astraite.uasset
    ├── SM_Crystal_Pyronite.uasset
    └── SM_Crystal_Voidstone.uasset
```

**แหล่ง Asset ฟรีแนะนำ:**
- **Quixel Megascans** (ฟรีกับ UE5) — หิน, ต้นไม้, พื้นผิว
- **Unreal Engine Marketplace** — Free monthly assets
- **Sketchfab** — Sci-fi crystals, low-poly trees
- **Kenney.nl** — Low-poly nature pack (ฟรี)

### 2.2 Character & Echo Meshes (Week 3-6)

#### Player Survivor
```
Content/ASTRAWILD/Characters/Survivor/
├── SK_Survivor_Exosuit.uasset      ← Skeletal Mesh
├── SA_Survivor_AnimSet.uasset      ← Animation Set
├── MAT_Survivor_Exosuit.uasset     ← Material
└── T_Survivor_*.uasset             ← Textures (2K)
```

**ข้อกำหนด:**
- ใช้ Skeleton เดียวกับ UE5 Mannequin (เพื่อใช้ Animation ที่มี)
- มี Sockets: `Weapon_R`, `Backpack_Socket`, `Scanner_L`
- PBR Textures: BaseColor, Normal, Roughness, Metallic, Emissive (Visor)

#### Echo Creatures (10 Species)
```
Content/ASTRAWILD/Echoes/
├── Quadruped/
│   ├── SK_Echo_Lumewisp.uasset
│   └── SK_Echo_Gloomfang.uasset
├── Biped/
│   └── SK_Echo_Stonehide.uasset
├── Avian/
│   └── SK_Echo_Voltling.uasset
└── ... (8 Body Plans)
```

**ทางเลือกสำหรับการผลิต:**
1. **จ้าง Artist** (แนะนำสำหรับ Commercial Game)
   - Sketchfab Freelance
   - Fiverr / Upwork
   - ราคาประมาณ $200-500 ต่อ character rig + texture

2. **ใช้ Marketplace Assets** (ประหยัด)
   - ค้นหา "Sci-Fi Creature Pack"
   - ปรับ Material ให้เข้ากับเกม

3. **ใช้ Procedural Tools** (ทางเทคนิค)
   - **Auto-Rig Pro** (Blender)
   - **Monster Mash** (Google)
   - **Masterpiece Studio** (VR sculpting)

### 2.3 Weapons & Equipment (Week 4-5)
```
Content/ASTRAWILD/Weapons/
├── SM_Weapon_ScrapRifle.uasset
├── SM_Weapon_PlasmaCarbine.uasset
├── SM_Weapon_ArcCannon.uasset
├── SM_Weapon_Railgun.uasset
└── SM_Weapon_SingularityCannon.uasset

Content/ASTRAWILD/Vehicles/
└── SM_Vehicle_DawnSkiff.uasset
```

**แนะนำ:** เริ่มจาก Low-poly ก่อน (5k-10k tris) แล้วค่อยเพิ่มรายละเอียด

### 2.4 VFX & Audio (Week 5-6)

#### Niagara Systems
```
Content/ASTRAWILD/VFX/
├── NS_MuzzleFlash_Plasma.uasset
├── NS_Beam_Arc_Lightning.uasset
├── NS_Capture_Orbital_Rings.uasset
├── NS_Hit_Impact_Spark.uasset
└── NS_Elemental_Trail_*.uasset
```

#### Audio
```
Content/ASTRAWILD/Audio/
├── SFX/
│   ├── A_Weapon_Plasma_Fire.wav
│   ├── A_Weapon_Impact_Hit.wav
│   ├── A_Echo_Lumewisp_Vocal.wav
│   └── A_UI_Click_Confirm.wav
├── Ambient/
│   ├── A_Ambient_DawnFields_Loop.wav
│   └── A_Ambient_Cave_Drip.wav
└── Music/
    └── M_Exploration_Theme_01.wav
```

**แหล่ง Audio ฟรี:**
- **Freesound.org** — SFX, Ambient
- **Sonniss GDC Bundles** — ฟรีทุกปี
- **Unreal Engine Sound Libraries**

---

## 🔧 ระยะที่ 3: Polish & Optimization (Week 6-8)

### 3.1 UI/UX Polish
- สร้าง Widget Blueprint สำหรับ Inventory, Crafting, Quest Log
- เพิ่ม Animation ให้ UI (Fade, Slide, Scale)
- ทำ Tutorial Popups สำหรับผู้เล่นใหม่

### 3.2 Performance Optimization
```
Target: 60 FPS @ 1080p (Medium Settings)
- LODs สำหรับทุก Mesh (LOD 0, 1, 2)
- HLOD สำหรับ Environment Clusters
- Texture Streaming Pool: 512MB
- Material Instructions < 100 per material
```

### 3.3 Lighting & Post-Processing
```
Content/ASTRAWILD/Maps/PostProcess/
├── PP_DawnFields.uasset
├── PP_DuskMarsh.uasset
└── PP_Dungeon.uasset
```

**ตั้งค่าแนะนำ:**
- Exposure: Min 8.0, Max 14.0
- Bloom: Intensity 0.8, Threshold 1.2
- Color Grading: Warm highlights, Cool shadows
- Volumetric Fog: Density 0.03

---

## 📦 โครงสร้างโฟลเดอร์สุดท้าย

```
Content/ASTRAWILD/
├── Maps/
│   ├── LVL_DawnFields.umap
│   ├── LVL_TestArena.umap
│   └── LVL_DungeonTest.umap
├── Data/
│   ├── Items/
│   │   ├── DA_Item_Wood.uasset
│   │   └── ...
│   ├── Recipes/
│   ├── Echoes/
│   ├── Buildings/
│   ├── Technologies/
│   └── Quests/
├── Characters/
│   ├── Survivor/
│   │   ├── SK_Survivor_Exosuit.uasset
│   │   ├── SA_Survivor_AnimSet.uasset
│   │   └── Materials/
│   └── Echoes/
│       ├── Quadruped/
│       ├── Biped/
│       └── ...
├── Environment/
│   ├── Foliage/
│   ├── Rocks/
│   ├── Crystals/
│   └── Materials/
├── Weapons/
├── Vehicles/
├── VFX/
├── Audio/
└── UI/
```

---

## 🎮 ขั้นตอนการเล่นครั้งแรก (First Playable Experience)

1. **Launch Game** → โหลด LVL_DawnFields
2. **Player Spawn** → เห็นภูมิทัศน์ Sci-Fi สวยงาม
3. **Tutorial Prompt** → สอน Movement, Camera
4. **First Quest: "First Light"** → เก็บ Wood 10, Stone 5
5. **Craft Resonator** → สอน Crafting System
6. **Find Lumewisp** → สอน Capture System
7. **Build Foundation** → สอน Building System
8. **Cook Meat** → สอน Survival System
9. **Save & Exit** → ทดสอบ Save/Load

---

## ⚠️ คำแนะนำสำคัญ

### อย่าทำสิ่งเหล่านี้:
❌ พยายามทำ Asset ทุกอย่างเองคนเดียว (ใช้เวลานานเกินไป)
❌ เริ่มจาก High-poly ก่อน (เริ่มจาก Low-poly แล้วค่อยอัพเกรด)
❌ ลืมเรื่อง LODs และ Optimization (เกมจะตกเฟรม)
❌ ใช้ Texture 4K ทุกอย่าง (ใช้ตามความเหมาะสม)

### ควรทำสิ่งเหล่านี้:
✅ ใช้ Quixel Megascans สำหรับ Environment (ฟรี คุณภาพสูง)
✅ จ้าง Freelance สำหรับ Character/Echo meshes (ประหยัดเวลา)
✅ ทำ Placeholder → Playable → Polish (ลำดับถูกต้อง)
✅ ทดสอบบ่อยๆ ทุกครั้งที่เพิ่ม Asset ใหม่

---

## 📅 Timeline สรุป

| สัปดาห์ | เป้าหมาย | ผลลัพธ์ |
|---|---|---|
| 1 | Map + Data Assets | เล่นได้ (Greybox) |
| 2-3 | Environment Art | ภาพสวยระดับหนึ่ง |
| 4-5 | Characters + Weapons | ตัวละครและอาวุธจริง |
| 6 | VFX + Audio | ความรู้สึกสมบูรณ์ |
| 7-8 | Polish + Optimization | พร้อม Demo |

---

## 🚀 เริ่มต้นทันที

**Step 1:** เปิด UE Editor → สร้าง `Content/ASTRAWILD/Maps/LVL_DawnFields.umap`
**Step 2:** สร้าง Data Assets ตาม Manifest (Items, Recipes, Echoes)
**Step 3:** ดาวน์โหลด Quixel Megascans → สร้าง Landscape Material
**Step 4:** หา Freelance สำหรับ Character/Echo meshes
**Step 5:** ทดสอบเล่น → แก้ไข → ซ้ำ

**ไฟล์ที่ต้องอ่านต่อ:**
- `ASTRAWILD_ASSET_MANIFEST.md` — รายการ Asset ทั้งหมด
- `ASTRAWILD_ASSET_PIPELINE.md` — วิธีแทนที่ CODE_DEFAULT
- `astra_wild_art_content.md` — Art Direction
- `ASTRAWILD_TEST_PLAN.md` — วิธีทดสอบ
