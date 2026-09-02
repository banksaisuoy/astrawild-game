#!/usr/bin/env python3
"""
ASTRAWILD — Echo Data Asset Generator (Batch 9)

สร้าง:
  1. Data Asset (.uasset paths) สำหรับ Echo ทั้ง 204 สายพันธุ์
  2. JSON export สำหรับ antigravity/k อ่านแล้วสร้างโมเดล 3D, Materials, Blueprints
  3. Python script สำหรับ generate Blueprint templates ใน Unreal Editor

Design Contract:
  - ทุกสายพันธุ์มี Data Asset path ชัดเจน
  - JSON มีข้อมูลครบสำหรับสร้าง: Skeletal Mesh, Material Instances, BP Class
  - รองรับ procedural generation ของ silhouette จาก body plan + size class
"""

import json
import os
from datetime import datetime

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BESTIARY_CPP = os.path.join(ROOT, "Source", "AstrawildCore", "Private", "AstrawildBestiaryData.cpp")
JSON_OUT = os.path.join(ROOT, "Content", "ASTRAWILD", "EchoDataAssets.json")
BP_SCRIPT_OUT = os.path.join(ROOT, "Scripts", "create_echo_blueprints.py")

# ============================================================================
# Parsing functions (อ่านจาก AstrawildBestiaryData.cpp ที่ generate แล้ว)
# ============================================================================

def parse_bestiary_cpp():
    """Parse generated C++ file เพื่อดึงข้อมูล Echo ทั้งหมด"""
    species_list = []
    
    with open(BESTIARY_CPP, 'r', encoding='utf-8') as f:
        content = f.read()
    
    # หาส่วน Rows[] array
    start_marker = "static const FBestiaryRow Rows[] = {"
    end_marker = "};"
    
    start_idx = content.find(start_marker)
    end_idx = content.rfind(end_marker)  # ใช้ rfind เพื่อหาตัวสุดท้าย
    
    if start_idx == -1 or end_idx == -1:
        print(f"ERROR: Cannot find Rows[] array in {BESTIARY_CPP}")
        return species_list
    
    rows_content = content[start_idx + len(start_marker):end_idx].strip()
    
    import re
    
    # Regex pattern สำหรับจับแต่ละ row - จับคู่ TEXT("...", TEXT("...")
    row_pattern = r'\{\s*TEXT\s*\(\s*"([^"]+)"\s*\)\s*,\s*TEXT\s*\(\s*"([^"]+)"\s*\)'
    
    matches = re.findall(row_pattern, rows_content)
    
    print(f"Found {len(matches)} species markers")
    
    # แยกแต่ละ row โดยดูจาก },{ หรือ }, ตามด้วย newline และ {
    # ใช้วิธี split ด้วย pattern ที่ซับซ้อนน้อยลง
    row_blocks = []
    current_block = ""
    brace_depth = 0
    in_string = False
    
    for char in rows_content:
        if char == '"' and (len(current_block) == 0 or current_block[-1] != '\\'):
            in_string = not in_string
        
        if char == '{' and not in_string:
            brace_depth += 1
            current_block += char
        elif char == '}' and not in_string:
            brace_depth -= 1
            current_block += char
            if brace_depth == 0:
                row_blocks.append(current_block.strip())
                current_block = ""
        elif in_string or brace_depth > 0:
            current_block += char
    
    print(f"Parsed {len(row_blocks)} row blocks")
    
    for block in row_blocks:
        # ลบ { } ออก
        block = block.strip().lstrip('{').rstrip('}').strip()
        
        # แยกค่าด้วย comma แต่ระวังใน string
        values = []
        current_val = ""
        in_string = False
        paren_depth = 0
        
        for char in block:
            if char == '"' and (len(current_val) == 0 or current_val[-1] != '\\'):
                in_string = not in_string
                current_val += char
            elif char == '(' and in_string:
                paren_depth += 1
                current_val += char
            elif char == ')' and in_string:
                paren_depth -= 1
                current_val += char
            elif char == ',' and not in_string and paren_depth == 0:
                val = current_val.strip()
                if val:
                    values.append(val)
                current_val = ""
            else:
                current_val += char
        
        if current_val.strip():
            values.append(current_val.strip())
        
        if len(values) < 30:  # ต้องมีอย่างน้อย 30 ค่า
            continue
        
        try:
            def extract_text(val):
                m = re.search(r'TEXT\s*\(\s*"([^"]*)"\s*\)', val)
                return m.group(1) if m else ""
            
            def extract_float(val):
                clean = val.replace('f', '').replace('F', '').strip()
                try:
                    return float(clean)
                except:
                    return 0.0
            
            def extract_int(val):
                try:
                    return int(val.strip())
                except:
                    return 0
            
            def extract_bool(val):
                return val.strip().lower() == 'true'
            
            def clean_enum(val):
                # แปลง EAstrawildElementType::Flora -> Flora
                m = re.search(r'EAstrawild\w+::(\w+)', val)
                return m.group(1) if m else val.strip()
            
            species = {
                "EchoId": extract_text(values[0]),
                "DisplayName": extract_text(values[1]),
                "Family": clean_enum(values[2]),
                "BodyPlan": clean_enum(values[3]),
                "SizeClass": clean_enum(values[4]),
                "Element": clean_enum(values[5]),
                "Weakness": clean_enum(values[6]),
                "Role": clean_enum(values[7]),
                "HomeZone": clean_enum(values[8]),
                "HomeZoneId": extract_text(values[9]),
                "Personality": clean_enum(values[10]),
                "ActivityPattern": clean_enum(values[11]),
                "BaseHP": extract_float(values[12]),
                "BaseATK": extract_float(values[13]),
                "BaseDEF": extract_float(values[14]),
                "MovementSpeed": extract_float(values[15]),
                "CaptureDifficulty": extract_float(values[16]),
                "bHostileByDefault": extract_bool(values[17]),
                "PrimaryTintR": extract_float(values[18]),
                "PrimaryTintG": extract_float(values[19]),
                "PrimaryTintB": extract_float(values[20]),
                "SecondaryTintR": extract_float(values[21]),
                "SecondaryTintG": extract_float(values[22]),
                "SecondaryTintB": extract_float(values[23]),
                "PreferredFoodA": extract_text(values[24]),
                "PreferredFoodB": extract_text(values[25]) if values[25].strip() != 'nullptr' else "",
                "LootItemA": extract_text(values[26]),
                "LootQuantityA": extract_int(values[27]),
                "LootItemB": extract_text(values[28]),
                "LootQuantityB": extract_int(values[29]),
                "PrimaryWorkType": clean_enum(values[30]),
                "SecondaryWorkType": clean_enum(values[31]),
                "SightRadius": extract_float(values[32]),
                "WorkAffinityA": extract_float(values[33]),
            }
            
            # Generate asset paths
            clean_name = species["EchoId"].replace("Echo_", "")
            base_path = f"/Game/Characters/Echo/{clean_name}"
            
            species["SkeletalMeshPath"] = f"{base_path}/SK_{clean_name}"
            species["PrimaryMaterialPath"] = f"{base_path}/MI_{clean_name}_Primary"
            species["SecondaryMaterialPath"] = f"{base_path}/MI_{clean_name}_Secondary"
            species["BlueprintPath"] = f"{base_path}/BP_Echo_{clean_name}"
            species["AnimBlueprintPath"] = f"{base_path}/ABP_{clean_name}"
            
            # Generate silhouette description
            species["SilhouetteDescription"] = get_silhouette_desc(
                species["BodyPlan"], 
                species["SizeClass"]
            )
            
            species_list.append(species)
            
        except Exception as e:
            print(f"Error parsing row: {e}")
            import traceback
            traceback.print_exc()
            continue
    
    return species_list


def get_silhouette_desc(body_plan, size_class):
    """Generate คำอธิบาย silhouette จาก body plan และ size"""
    descs = {
        "Quadruped": "four-legged body, low head position, tail extending from rear",
        "Biped": "upright stance, forward-facing arms, crest or head ornament",
        "Serpent": "elongated coiling spine, wedge-shaped head, fin-like tail",
        "Floating": "hovering core body, drifting motes or appendages, no legs",
        "Insectoid": "segmented shell carapace, twin antennae, four spindly legs",
        "Avian": "keeled chest structure, folded wings along back, beaked head",
        "Crystalline": "faceted geometric shards, luminous energy seams, crown formation",
        "Amorphous": "flowing mass without fixed shape, embedded light organs",
    }
    
    size_mods = {
        "Tiny": "very small, fits in hand",
        "Small": "knee-high, lightweight",
        "Medium": "waist-high, balanced proportions",
        "Large": "taller than human, imposing presence",
        "Huge": "massive, multi-ton bulk",
    }
    
    base_desc = descs.get(body_plan, "unique creature form")
    size_mod = size_mods.get(size_class, "medium sized")
    
    return f"{size_mod}; {base_desc}"


def export_json(species_list):
    """Export เป็น JSON สำหรับ antigravity/k"""
    output = {
        "metadata": {
            "generated_at": datetime.now().isoformat(),
            "total_species": len(species_list),
            "version": "Batch 9 - Data Assets",
            "purpose": "For antigravity/k to create 3D models, materials, and blueprints"
        },
        "species": species_list
    }
    
    os.makedirs(os.path.dirname(JSON_OUT), exist_ok=True)
    
    with open(JSON_OUT, 'w', encoding='utf-8') as f:
        json.dump(output, f, indent=2, ensure_ascii=False)
    
    print(f"Wrote {JSON_OUT} ({len(species_list)} species)")


def create_blueprint_script(species_list):
    """สร้าง Python script สำหรับ generate Blueprints ใน Unreal Editor"""
    
    script_content = '''#!/usr/bin/env python3
"""
ASTRAWILD — Echo Blueprint Generator

รันใน Unreal Editor เพื่อสร้าง:
  - Data Assets (.uasset) สำหรับทุก species
  - Blueprint Classes
  - Material Instances

วิธีใช้:
  1. เปิด Unreal Editor
  2. Window -> Developer Tools -> Output Log
  3. พิมพ์: execfile("Scripts/create_echo_blueprints.py")
"""

import unreal
import json
import os

def load_json():
    """โหลด JSON data"""
    json_path = os.path.join(os.path.dirname(__file__), "..", "Content", "ASTRAWILD", "EchoDataAssets.json")
    with open(json_path, 'r', encoding='utf-8') as f:
        return json.load(f)

def create_data_asset(species):
    """สร้าง UAstrawildEchoDataAsset สำหรับหนึ่ง species"""
    echo_id = species["EchoId"]
    display_name = species["DisplayName"]
    
    # สร้าง path
    asset_path = f"/Game/Characters/Echo/{echo_id.replace('Echo_', '')}/DA_{echo_id}"
    
    # เช็คว่ามีอยู่แล้วหรือไม่
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        print(f"SKIP: {asset_path} already exists")
        return None
    
    # สร้าง Data Asset
    factory = unreal.DataAssetFactory()
    asset = unreal.EditorAssetLibrary.create_asset(
        factory=factory,
        package_name=asset_path,
        asset_class=unreal.load_object(None, "/Script/AstrawildCore.AstrawildEchoDataAsset")
    )
    
    if asset is None:
        print(f"ERROR: Failed to create {asset_path}")
        return None
    
    # ตั้งค่า properties
    asset.echo_id = echo_id
    asset.display_name = unreal.Text.from_string(display_name)
    asset.description = unreal.Text.from_string(f"A {species['SizeClass'].lower()} {species['Family']} of the {species['HomeZone']}")
    
    # Appearance
    asset.family = getattr(unreal.EAstrawildEchoFamily, species["Family"], unreal.EAstrawildEchoFamily.BEAST)
    asset.body_plan = getattr(unreal.EAstrawildBodyPlan, species["BodyPlan"], unreal.EAstrawildBodyPlan.QUADRUPED)
    asset.size_class = getattr(unreal.EAstrawildSizeClass, species["SizeClass"], unreal.EAstrawildSizeClass.MEDIUM)
    
    # Colors
    asset.primary_tint = unreal.LinearColor(
        species["PrimaryTintR"], 
        species["PrimaryTintG"], 
        species["PrimaryTintB"], 
        1.0
    )
    asset.secondary_tint = unreal.LinearColor(
        species["SecondaryTintR"], 
        species["SecondaryTintG"], 
        species["SecondaryTintB"], 
        1.0
    )
    
    asset.silhouette_description = species["SilhouetteDescription"]
    
    # Element & Role
    asset.element = getattr(unreal.EAstrawildElementType, species["Element"], unreal.EAstrawildElementType.FLORA)
    asset.weakness = getattr(unreal.EAstrawildElementType, species["Weakness"], unreal.EAstrawildElementType.EMBER)
    asset.role = getattr(unreal.EAstrawildEchoRole, species["Role"], unreal.EAstrawildEchoRole.BASE)
    
    # Stats
    asset.base_hp = species["BaseHP"]
    asset.base_atk = species["BaseATK"]
    asset.base_def = species["BaseDEF"]
    asset.movement_speed = species["MovementSpeed"]
    asset.sight_radius = species["SightRadius"]
    
    # Capture & Personality
    asset.capture_difficulty = species["CaptureDifficulty"]
    asset.b_hostile_by_default = species["bHostileByDefault"]
    asset.personality = getattr(unreal.EAstrawildPersonality, species["Personality"], unreal.EAstrawildPersonality.CURIOUS)
    asset.activity_pattern = getattr(unreal.EAstrawildActivityPattern, species["ActivityPattern"], unreal.EAstrawildActivityPattern.DIURNAL)
    
    # Work
    asset.primary_work_type = getattr(unreal.EAstrawildWorkType, species["PrimaryWorkType"], unreal.EAstrawildWorkType.GATHERING)
    asset.secondary_work_type = getattr(unreal.EAstrawildWorkType, species["SecondaryWorkType"], unreal.EAstrawildWorkType.TRANSPORT)
    asset.work_affinity_a = species["WorkAffinityA"]
    
    # Zone
    asset.home_zone = getattr(unreal.EAstrawildZone, species["HomeZone"], unreal.EAstrawildZone.DAWN_FIELDS)
    asset.home_zone_id = unreal.FName(species["HomeZoneId"])
    
    # Asset paths
    asset.skeletal_mesh_path = species["SkeletalMeshPath"]
    asset.primary_material_path = species["PrimaryMaterialPath"]
    asset.secondary_material_path = species["SecondaryMaterialPath"]
    asset.blueprint_path = species["BlueprintPath"]
    asset.anim_blueprint_path = species["AnimBlueprintPath"]
    
    # บันทึก
    unreal.EditorAssetLibrary.save_loaded_asset(asset)
    print(f"CREATED: {asset_path}")
    
    return asset

def main():
    print("=" * 60)
    print("ASTRAWILD Echo Data Asset Generator")
    print("=" * 60)
    
    data = load_json()
    species_list = data["species"]
    
    print(f"Loaded {len(species_list)} species")
    print()
    
    created = 0
    skipped = 0
    errors = 0
    
    for i, species in enumerate(species_list):
        try:
            result = create_data_asset(species)
            if result:
                created += 1
            else:
                skipped += 1
        except Exception as e:
            print(f"ERROR creating {species['EchoId']}: {e}")
            errors += 1
        
        # Progress update ทุก 20 ตัว
        if (i + 1) % 20 == 0:
            print(f"Progress: {i+1}/{len(species_list)}...")
    
    print()
    print("=" * 60)
    print(f"Complete: {created} created, {skipped} skipped, {errors} errors")
    print("=" * 60)

if __name__ == "__main__":
    main()
'''
    
    with open(BP_SCRIPT_OUT, 'w', encoding='utf-8') as f:
        f.write(script_content)
    
    print(f"Wrote {BP_SCRIPT_OUT}")


def main():
    print("=" * 60)
    print("ASTRAWILD Echo Data Asset Generator (Batch 9)")
    print("=" * 60)
    print()
    
    # Parse bestiary
    species_list = parse_bestiary_cpp()
    
    if not species_list:
        print("ERROR: No species parsed. Check AstrawildBestiaryData.cpp")
        return
    
    print()
    
    # Export JSON
    export_json(species_list)
    
    # Create blueprint script
    create_blueprint_script(species_list)
    
    print()
    print("=" * 60)
    print("Next Steps for antigravity/k:")
    print("=" * 60)
    print("""
1. อ่านไฟล์ JSON: Content/ASTRAWILD/EchoDataAssets.json
   - มีข้อมูลครบ 204 species พร้อม paths สำหรับ assets

2. สร้างโมเดล 3D ตาม Silhouette Description:
   - ใช้ BodyPlan + SizeClass เป็น guideline
   - Apply PrimaryTint และ SecondaryTint เป็นสีหลัก/รอง

3. สร้าง assets ใน Unreal Editor:
   - Skeletal Mesh: ตาม SkeletalMeshPath
   - Material Instances: ตาม Primary/SecondaryMaterialPath
   - Blueprint Class: ตาม BlueprintPath
   - Animation Blueprint: ตาม AnimBlueprintPath

4. รันสคริปต์ใน Editor (ถ้าต้องการ automate):
   execfile("Scripts/create_echo_blueprints.py")

5. ตรวจสอบ Data Assets ที่สร้าง:
   - ทุกตัวต้องมี UAstrawildEchoDataAsset
   - Properties ต้องตรงกับ JSON
""")
    print("=" * 60)


if __name__ == "__main__":
    main()
