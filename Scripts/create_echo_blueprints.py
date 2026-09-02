#!/usr/bin/env python3
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
