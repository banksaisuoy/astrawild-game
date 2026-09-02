# ASTRAWILD VISUAL ENGINEERING MASTER PLAN
**Version:** 1.0 (Final Strategic Plan)
**Status:** PLANNING COMPLETE - READY FOR EXECUTION
**Owner:** Qwen (Visual Engineering Agent)
**Verifier:** Antigravity (UE5 Runtime Verification)

---

## 🛑 STOP & THINK: STRATEGIC OVERVIEW

ก่อนเขียนโค้ดเพิ่ม 1 บรรทัด เราต้องตอบคำถามให้ชัด:
1.  **เรามีอะไรแล้ว?** 
    *   C++ Core ระบบ Echo 204 ชนิด (Data Only)
    *   ระบบ Procedural Generation โลก 12 โซน
    *   Base Material `M_Master_Surface` (ที่มีอยู่จริง)
2.  **เราขาดอะไร?**
    *   **การจับคู่ (Mapping):** ตัวไหนอยู่โซนไหน? ใช้อาวุธอะไร?
    *   **Visual Specifics:** สีเฉพาะตัว, สกิล, รูปร่างที่ชัดเจน
    *   **Assets จริง:** .uasset files (Materials, Meshes, Blueprints)
3.  **เราจะทำยังไง?**
    *   **ไม่ทำทีเดียว 204 ตัว** (เสี่ยงผิดพลาดและตรวจสอบยาก)
    *   **ทำเป็น Phase:** เริ่มจาก 3 ตัวต้นแบบ -> ขยายเป็น 12 ตัว (1 โซน) -> ครบ 12 โซน
    *   **Git Strategy:** แยก Branch ต่อ 1 Phase หรือต่อ 1 กลุ่มใหญ่ เพื่อให้ง่ายต่อการ Review และ Merge

---

## 🗺️ PHASE 0: ECOSYSTEM MAPPING (The "Brain" Work)

กำหนดการจัดสรร Echo 204 สายพันธุ์ ลงใน 12 โซน อย่างมีเหตุผลทางนิเวศวิทยา

### 12 Zones & Biome Identity
| Zone ID | Zone Name | Biome Type | Difficulty | Key Elements |
|:---:|:---|:---|:---:|:---|
| **Z01** | Verdant Weald | ป่าดิบชื้น/ทุ่งหญ้า | Low | Grass, Trees, Water, Earth |
| **Z02** | Cinder Basin | ภูเขาไฟ/ลาวา | Med-High | Fire, Rock, Ash, Heat |
| **Z03** | Frostveil Peaks | ภูเขาหิมะ/น้ำแข็ง | Med-High | Ice, Snow, Wind, Cold |
| **Z04** | Stormcrest Archipelago | เกาะพายุ/สายฟ้า | High | Lightning, Water, Wind, Metal |
| **Z05** | Silversand Desert | ทะเลทราย/คริสตัล | Med | Sand, Sun, Light, Crystal |
| **Z06** | Umbra Mangroves | ป่าชายเลนมืด/พิษ | Med-High | Poison, Darkness, Water, Organic |
| **Z07** | Aetherial Tundra | ทุ่งน้ำแข็งลอยฟ้า | High | Gravity, Ice, Magic, Air |
| **Z08** | Ironroot Jungle | ป่าดงดิบโลหะ | High | Metal, Vine, Earth, Acid |
| **Z09** | Whispering Dunes | ทะเลทรายต้องคำสาป | Med-High | Sand, Wind, Spirit, Magic |
| **Z10** | Crystalline Caverns | ถ้ำคริสตัลลึก | High | Crystal, Dark, Light, Earth |
| **Z11** | Void-Touched Wastes | พื้นที่ว่างเปล่าวิปริต | Extreme | Void, Chaos, Mutation, Dark |
| **Z12** | Primeval Core | แก่นโลกโบราณ | Boss | All Elements, Ancient, Fire, Magic |

### 📋 Echo Distribution Matrix (Sample for Planning)
*เราจะไม่แจกแจงทั้ง 204 ตัวในนี้ แต่จะกำหนด "Quota" ต่อโซน*

*   **Common (50%):** 10-12 ตัว/โซน (ไม่มีสกิลพิเศษ เน้นจำนวน)
*   **Rare (30%):** 6-8 ตัว/โซน (มี Elemental Attack 1 อย่าง)
*   **Epic (15%):** 3-4 ตัว/โซน (มี Unique Mechanic/Weapon)
*   **Legendary (5%):** 1 ตัว/โซน (Boss Mini / Elite)

---

## 🚀 EXECUTION PHASES (Step-by-Step)

### 🔹 PHASE 1: THE PROTOTYPES (Pilot Run)
**เป้าหมาย:** สร้าง Echo 3 ตัวแรก ให้ "สมบูรณ์ที่สุด" เพื่อใช้เป็น Template
**Scope:** Bastionbeetle, Terraquill, Cindermule
**Location:** Verdant Weald (Z01) & Cinder Basin (Z02)

#### 1.1 Bastionbeetle (The Tank)
*   **Zone:** Z01 (Verdant Weald)
*   **Body Plan:** Quadruped (Beetle-like)
*   **Role:** Defender / Tank
*   **Element:** Earth / Plant
*   **Weapon/Attack:** Headbutt (Charge), Shell Slam
*   **Visual Specs:**
    *   *Color:* Deep Green (#2E5A1C) + Brown Shell (#5C4033)
    *   *Material:* High Roughness (Shell), Low Metallic (Organic)
    *   *Emissive:* None (Non-elemental)
    *   *Unique:* Shell texture details, Moss growth on back

#### 1.2 Terraquill (The Ranged)
*   **Zone:** Z01 (Verdant Weald)
*   **Body Plan:** Quadruped (Porcupine-like)
*   **Role:** Ranged DPS
*   **Element:** Earth / Wind
*   **Weapon/Attack:** Quill Shoot (Projectile)
*   **Visual Specs:**
    *   *Color:* Grey Fur (#808080) + Golden Quills (#DAA520)
    *   *Material:* Medium Roughness (Fur), High Metallic (Quills)
    *   *Emissive:* Slight Gold Glow on Quill tips when charging
    *   *Unique:* Quill geometry, Fur shader params

#### 1.3 Cindermule (The Carrier/Blaster)
*   **Zone:** Z02 (Cinder Basin)
*   **Body Plan:** Biped/Quadruped Hybrid (Mule-like)
*   **Role:** Carrier / Fire AoE
*   **Element:** Fire / Earth
*   **Weapon/Attack:** Magma Spit, Hoof Stomp (Fire Trail)
*   **Visual Specs:**
    *   *Color:* Black Charcoal (#1A1A1A) + Glowing Orange Cracks (#FF4500)
    *   *Material:* Low Roughness (Charcoal), Zero Metallic
    *   *Emissive:* **HIGH** Orange Pulse on cracks (Dynamic based on aggro)
    *   *Unique:* Particle smoke emitter from nostrils, Heat distortion aura

**✅ Deliverables Phase 1:**
1.  C++ Class `A_Bastionbeetle`, `A_Terraquill`, `A_Cindermule` (Subclass of Echo)
2.  Data Assets (`DA_Bastionbeetle`, etc.) กำหนดค่า Stats, Color, Material Params
3.  Material Instances (`MI_Echo_Bastionbeetle`, etc.) สร้างจาก `M_Master_Surface`
4.  Blueprint Logic สำหรับ Attack เฉพาะตัว (QuillShoot, MagmaSpit)
5.  **Verification:** Spawn ใน Map ได้, ยิงได้, สีตรงตามสเปค, Performance OK

---

### 🔹 PHASE 2: ZONE 01 COMPLETION (Verdant Weald)
**เป้าหมาย:** เติม Echo ให้ครบโซน Z01 (รวม 12-15 ตัว)
**Strategy:** ใช้ Template จาก Phase 1 แล้วเปลี่ยน Parameter เป็นหลัก (ไม่ต้องเขียน Code ใหม่เยอะ)

*   **Add 10 Common:** Grasshopper, LeafBug, MudCrab, etc. (เปลี่ยนแค่ Color/Mesh Variant)
*   **Add 2 Rare:** VineSnare (Poison), WindRaptor (Fast)
*   **Add 1 Epic:** ElderTreant (Mini-boss, Large scale)

**📝 Git Plan:** `qwen/phase2-zone01-fill`
*   Focus: Data Assets & Material Instances generation script.
*   Minimal C++ changes.

---

### 🔹 PHASE 3: ELEMENTAL EXPANSION (Fire & Ice)
**เป้าหมาย:** ขยายไป Z02 (Cinder Basin) และ Z03 (Frostveil)
**Focus:** ระบบ Material Emissive และ Particle Effects

*   **Z02 (Fire):** Add 12 Fire-based Echoes (Magma Golem, AshBat, etc.)
    *   *Tech:* Dynamic EmissiveIntensity, Heat Haze Material Function.
*   **Z03 (Ice):** Add 12 Ice-based Echoes (FrostWolf, IceGolem, etc.)
    *   *Tech:* Subsurface Scattering (Ice), Breath Fog Particles.

**📝 Git Plan:** `qwen/phase3-elemental-vfx`

---

### 🔹 PHASE 4: ADVANCED MECHANICS (Lightning, Poison, Void)
**เป้าหมาย:** Z04, Z06, Z11
**Focus:** Complex AI & VFX Binding

*   **Z04 (Lightning):** Chain lightning attacks, Metal conductivity visuals.
*   **Z06 (Poison):** Gas clouds, melting terrain effects.
*   **Z11 (Void):** Teleportation, Distortion shaders, Color shifting.

---

### 🔹 PHASE 5: WEAPONS & SURVITOR GEAR
**เป้าหมาย:** ติดตั้งอาวุธให้ Survivor และ Echo บางชนิดที่ใช้เครื่องมือ
*   **Survivor Exosuit:** MI_Exosuit_Standard, MI_Exosuit_FireResist, etc.
*   **Echo Weapons:**绑定 Niagara Effects ให้กับ Socket (Muzzle, Blade Tip).

---

### 🔹 PHASE 6: OPTIMIZATION & LODS
**เป้าหมาย:** ลด Draw Calls, ตั้งค่า LOD ระยะไกล, Bake Lighting
*   Auto-generate LODs สำหรับ Echo ทุกตัว
*   Merge Actors ในพื้นที่หนาแน่น

---

## 🛠️ TECHNICAL IMPLEMENTATION STRATEGY

### 1. Data-Driven Approach (สำคัญที่สุด)
เราจะไม่ Hardcode สีหรือค่าต่างๆ ใน C++ อีกต่อไป
*   **สร้าง Struct `FEchoVisualSpec`:**
    ```cpp
    USTRUCT()
    struct FEchoVisualSpec {
        GENERATED_BODY()
        UPROPERTY() FLinearColor PrimaryColor;
        UPROPERTY() FLinearColor SecondaryColor;
        UPROPERTY() float Roughness;
        UPROPERTY() float Metallic;
        UPROPERTY() float EmissiveIntensity;
        UPROPERTY() FLinearColor EmissiveColor;
        UPROPERTY() UMaterialInterface* OverrideMaterial;
    };
    ```
*   **ใส่ใน Data Asset:** เมื่อสร้าง DA_EchoXXX ก็แค่กรอกค่าในตารางนี้ -> เกมจะสร้าง Material Instance แบบ Dynamic เองตอนรันไทม์ (หรือ Pre-generate ตอน Build)

### 2. Git Branching Strategy
เพื่อป้องกัน Conflict และง่ายต่อการตรวจสอบ:
*   `main`: โค้ดที่เสถียรที่สุด (Verified by Antigravity)
*   `qwen/core-visual-system`: ระบบกลาง (Material, Data Struct) -> **ทำตอนนี้**
*   `qwen/echo-phase1-prototypes`: ตัวต้นแบบ 3 ตัว -> **ทำถัดไป**
*   `qwen/zone-01-content`: เนื้อหาโซน 1
*   `qwen/zone-XX-content`: แยกโซนกันไป (สามารถทำพร้อมกันได้ถ้ามีทีม)

### 3. Automation Script Update
แก้ไข Python Script ที่มีอยู่ ให้สร้าง:
1.  **Data Assets** (.uasset) พร้อมค่า Visual Spec ที่ถูกต้อง
2.  **Material Instances** (.uasset) โดยอ้างอิง `M_Master_Surface` และตั้งค่า Parameters ให้ตรงกับ Data
3.  **Blueprints** (ถ้าจำเป็น) สำหรับ Logic พิเศษ

---

## ✅ CHECKLIST BEFORE STARTING PHASE 1

- [ ] **ยืนยันแผน:** คุณ (User) เห็นชอบกับแผน Phase 1-6 นี้หรือไม่?
- [ ] **ยืนยัน 3 ตัวแรก:** Bastionbeetle, Terraquill, Cindermule โอเคไหม? หรือจะเปลี่ยน?
- [ ] **ยืนยันระบบ Data:** จะใช้วิธีกรอกค่าใน Data Asset (แนะนำ) หรือ Hardcode ใน C++?
- [ ] **เตรียม Environment:** Antigravity พร้อมรัน Script ใน UE5 เมื่อไหร่?

---

## 📣 คำสั่งถัดไป (Next Action)

หากแผนนี้ผ่าน:
1.  ผมจะสร้าง **`FEchoVisualSpec` Struct** ใน C++
2.  เพิ่มตัวแปรนี้เข้าไปใน **`UAstrawildEchoData`**
3.  เขียนโค้ด **`A_Bastionbeetle`**, **`A_Terraquill`**, **`A_Cindermule`** โดยดึงค่าจาก Data
4.  อัปเดต **Python Script** ให้ generate DA และ MI สำหรับ 3 ตัวนี้พร้อมค่าสี/วัสดุที่ถูกต้อง
5.  ส่งให้ Antigravity Verify ใน UE5

**รอคำสั่งยืนยันแผนก่อนดำเนินการขั้นต่อไปครับ!**
