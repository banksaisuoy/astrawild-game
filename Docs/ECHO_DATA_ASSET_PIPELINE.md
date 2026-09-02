# ASTRAWILD — Echo Data Asset Pipeline (Batch 9)

## สรุปสิ่งที่สร้างแล้ว

### 1. C++ Core Files
- **`Source/AstrawildCore/Public/AstrawildEchoDataAsset.h`** - Header สำหรับ UAstrawildEchoDataAsset class
- **`Source/AstrawildCore/Private/AstrawildEchoDataAsset.cpp`** - Implementation

### 2. Python Scripts
- **`Scripts/generate_echo_data_assets.py`** - Generate JSON และ Blueprint script จาก AstrawildBestiaryData.cpp
- **`Scripts/create_echo_blueprints.py`** - (Generated) สคริปต์สำหรับรันใน Unreal Editor เพื่อสร้าง Data Assets อัตโนมัติ

### 3. Data Files
- **`Content/ASTRAWILD/EchoDataAssets.json`** - ข้อมูล Echo ทั้ง 204 สายพันธุ์ พร้อม:
  - ข้อมูลพื้นฐาน (ID, ชื่อ, ตระกูล, โครงร่าง, ขนาด)
  - ลักษณะทางกายภาพ (สีหลัก/รอง, silhouette description)
  - ธาตุและจุดอ่อน
  - สถิติ (HP, ATK, DEF, Speed)
  - ระบบจับ (capture difficulty, hostility)
  - อุปนิสัย (personality, activity pattern)
  - อาหารและ Loot
  - งานที่ถนัด (work affinities)
  - ถิ่นที่อยู่ (home zone)
  - **Asset paths** สำหรับทุกสิ่งที่ต้องสร้าง

## วิธีใช้งานสำหรับ antigravity/k

### ขั้นตอนที่ 1: อ่านข้อมูล JSON
```python
import json

with open('Content/ASTRAWILD/EchoDataAssets.json', 'r') as f:
    data = json.load(f)

species_list = data['species']  # 204 species
first_species = species_list[0]

print(first_species['DisplayName'])  # "Mosspaw"
print(first_species['SilhouetteDescription'])  # คำอธิบายรูปร่าง
print(first_species['PrimaryTintR'], first_species['PrimaryTintG'], first_species['PrimaryTintB'])  # สี RGB
```

### ขั้นตอนที่ 2: สร้างโมเดล 3D
สำหรับแต่ละ species ให้สร้าง:

1. **Skeletal Mesh** ตาม `SkeletalMeshPath`
   - ใช้ `BodyPlan` เป็น guideline สำหรับโครงสร้างกระดูก
   - ใช้ `SizeClass` สำหรับสเกล
   - อ่าน `SilhouetteDescription` สำหรับรายละเอียดรูปร่าง

2. **Material Instances** 2 อัน:
   - `PrimaryMaterialPath` - ใช้ `PrimaryTintR/G/B` เป็นสีหลัก
   - `SecondaryMaterialPath` - ใช้ `SecondaryTintR/G/B` เป็นสีรอง

3. **Blueprint Class** ตาม `BlueprintPath`
   - Parent class: `BP_Echo_Character_Base` (มีอยู่แล้ว)
   - Assign Data Asset ที่สร้าง

4. **Animation Blueprint** ตาม `AnimBlueprintPath`
   - ใช้ Animation BP กลางตาม `BodyPlan`
   - Adjust blend spaces ตาม `SizeClass`

### ขั้นตอนที่ 3: รัน Automation Script ใน Unreal Editor
```python
# ใน Unreal Editor Output Log:
execfile("Scripts/create_echo_blueprints.py")
```

สคริปต์นี้จะ:
- โหลด JSON data
- สร้าง `UAstrawildEchoDataAsset` สำหรับทุก species
- ตั้งค่า properties ทั้งหมดอัตโนมัติ
- บันทึกเป็น `.uasset` files

### ขั้นตอนที่ 4: ตรวจสอบ
หลังจากรัน script แล้ว ตรวจสอบใน Content Browser:
```
/Game/Characters/Echo/
├── Mosspaw/
│   ├── DA_Echo_Mosspaw (Data Asset)
│   ├── SK_Mosspaw (Skeletal Mesh) ← ต้องสร้าง
│   ├── MI_Mosspaw_Primary (Material Instance) ← ต้องสร้าง
│   ├── MI_Mosspaw_Secondary (Material Instance) ← ต้องสร้าง
│   ├── BP_Echo_Mosspaw (Blueprint Class) ← ต้องสร้าง
│   └── ABP_Mosspaw (Animation Blueprint) ← ต้องสร้าง
├── Dawnhorn/
│   └── ...
└── ... (204 species)
```

## ตัวอย่างข้อมูล 1 Species

```json
{
  "EchoId": "Echo_Mosspaw",
  "DisplayName": "Mosspaw",
  "Family": "Beast",
  "BodyPlan": "Quadruped",
  "SizeClass": "Small",
  "Element": "Flora",
  "Weakness": "Ember",
  "Role": "Base",
  "HomeZone": "DawnFields",
  
  "BaseHP": 70.0,
  "BaseATK": 10.0,
  "BaseDEF": 6.0,
  "MovementSpeed": 480.0,
  "CaptureDifficulty": 0.2,
  "bHostileByDefault": false,
  
  "PrimaryTintR": 0.58,
  "PrimaryTintG": 0.54,
  "PrimaryTintB": 0.36,
  "SecondaryTintR": 0.419,
  "SecondaryTintG": 0.347,
  "SecondaryTintB": 0.398,
  
  "SilhouetteDescription": "knee-high, lightweight; four-legged body, low head position, tail extending from rear",
  
  "PreferredFoodA": "Item_Berry",
  "LootItemA": "Item_RawMeat",
  "LootQuantityA": 2,
  
  "PrimaryWorkType": "Gathering",
  "SecondaryWorkType": "Transport",
  "WorkAffinityA": 1.2,
  
  "SkeletalMeshPath": "/Game/Characters/Echo/Mosspaw/SK_Mosspaw",
  "PrimaryMaterialPath": "/Game/Characters/Echo/Mosspaw/MI_Mosspaw_Primary",
  "BlueprintPath": "/Game/Characters/Echo/Mosspaw/BP_Echo_Mosspaw"
}
```

## Body Plans ที่มี (8 แบบ)

| BodyPlan | คำอธิบาย | Species Count |
|----------|----------|---------------|
| Quadruped | 4 ขา, หัวต่ำ, มีหาง | ~60 |
| Biped | 2 ขา, ยืนตรง, มีแขน | ~25 |
| Serpent | ลำตัวยาว, คดเคี้ยว, มีครีบ | ~20 |
| Floating | ลอยได้, ไม่มีขา, มีแสงล้อมรอบ | ~25 |
| Insectoid | เปลือกแข็ง, หนวดคู่, 6 ขา | ~20 |
| Avian | มีปีก, จงอยปาก, грудอกนูน | ~25 |
| Crystalline | รูปทรงเรขาคณิต, แสงส่องผ่าน | ~15 |
| Amorphous | รูปร่างไม่แน่นอน, ไหลได้ | ~14 |

## Size Classes ที่มี (5 ระดับ)

| SizeClass | ความสูง | HP Base | Speed Base |
|-----------|---------|---------|------------|
| Tiny | เท่าฝ่ามือ | 40 | 520 |
| Small | เท่าเข่า | 70 | 480 |
| Medium | เท่าเอว | 110 | 420 |
| Large | สูงกว่าคน | 170 | 360 |
| Huge | Massive | 260 | 300 |

## Element System (6 ธาตุ)

3 คู่ตรงข้าม (weakness คือคู่ตรงข้าม ×1.5 damage):
- **Light ↔ Ash** (แสง ↔ เถ้า)
- **Flora ↔ Ember** (พฤกษชาติ ↔ เพลิง)
- **Frost ↔ Pulse** (น้ำแข็ง ↔ สายฟ้า)

## Families ที่มี (10 ตระกูล)

1. **Beast** - สัตว์ร้างทั่วไป (26 species)
2. **Dragon** - มังกร (22 species)
3. **Construct** - หุ่นยนต์โบราณ (18 species)
4. **Spirit** - วิญญาณ (22 species)
5. **Elemental** - ธาตุบริสุทธิ์ (24 species)
6. **Aquatic** - อสูรน้ำ (21 species)
7. **Insectoid** - แมลงยักษ์ (18 species)
8. **Flora** - พฤกษอสูร (24 species)
9. **Avian** - นกอสูร (20 species)
10. **Ancient** - เทวะโบราณ (9 species)

## Next Steps

### สำหรับ antigravity/k:
1. ✅ อ่าน JSON file (`Content/ASTRAWILD/EchoDataAssets.json`)
2. 🔄 สร้างโมเดล 3D ตาม Silhouette Description
3. 🔄 สร้าง Material Instances ด้วยสีจาก JSON
4. 🔄 สร้าง Blueprint Classes
5. 🔄 สร้าง Animation Blueprints
6. 🔄 รัน `create_echo_blueprints.py` ใน Editor เพื่อสร้าง Data Assets

### สำหรับ GLM53:
- Compile C++ project หลังเพิ่ม `AstrawildEchoDataAsset.h/cpp`
- Verify ว่า `UAstrawildEchoDataAsset` class register ถูกต้อง
- รัน automation tests

## Statistics

- **Total Species**: 204 (generated) + 10 (hand-authored) = **214 Echo species**
- **Zones Covered**: 12/12 (ทุกโซนมี species เฉพาะตัว)
- **Body Plans**: 8 แบบ
- **Size Classes**: 5 ระดับ
- **Elements**: 6 ธาตุ (3 คู่ตรงข้าม)
- **Families**: 10 ตระกูล
- **JSON File Size**: ~500KB (รวมข้อมูลทั้งหมด)

---

*Generated by `Scripts/generate_echo_data_assets.py` (Batch 9)*
