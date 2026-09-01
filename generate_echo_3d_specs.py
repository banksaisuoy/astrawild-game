"""
Astrawild Echo 3D Model Specification Generator
------------------------------------------------
สคริปต์นี้ใช้สำหรับแปลงข้อมูล JSON ของ Echo ทั้ง 204 สายพันธุ์ 
ให้กลายเป็น Technical Specification (JSON) ที่พร้อมสำหรับการสร้างโมเดล 3D อัตโนมัติ
โดยเครื่องมือเช่น Blender Python API, Houdini, หรือ AI Model Generators

ผลลัพธ์ที่ได้จะเป็นไฟล์ 'echo_3d_build_specs.json' ที่มีข้อมูล:
- Geometry Primitives (รูปทรงเรขาคณิตพื้นฐาน)
- Scale Factors (อัตราส่วนขนาด)
- Material Shader Params (ค่าพารามิเตอร์วัสดุ)
- Rigging Bone Maps (แผนผังกระดูก)
- LOD Distances (ระยะระดับรายละเอียด)
"""

import json
import math
import os

# --- Configuration Constants ---
BODY_PLAN_SHAPES = {
    "Insectoid": {"base_primitive": "capsule", "segments": 3, "limb_count": 6, "tail": True},
    "Mammalian": {"base_primitive": "cylinder", "segments": 4, "limb_count": 4, "tail": True},
    "Avian": {"base_primitive": "cone", "segments": 3, "limb_count": 2, "wings": True, "tail": True},
    "Reptilian": {"base_primitive": "box", "segments": 5, "limb_count": 4, "tail": True, "scales": True},
    "Aquatic": {"base_primitive": "sphere", "segments": 2, "fins": True, "tail": True, "gills": True},
    "Plantoid": {"base_primitive": "cylinder", "segments": 2, "limb_count": 0, "leaves": True, "roots": True},
    "Elemental": {"base_primitive": "icosphere", "segments": 1, "floating": True, "particles": True},
    "Mechanical": {"base_primitive": "box", "segments": 4, "limb_count": 4, "joints": "rigid", "panels": True}
}

SIZE_CLASS_SCALE = {
    "Tiny": {"height": 0.3, "width": 0.2, "mass": 1.0},
    "Small": {"height": 0.8, "width": 0.5, "mass": 5.0},
    "Medium": {"height": 1.6, "width": 0.8, "mass": 60.0},
    "Large": {"height": 2.5, "width": 1.2, "mass": 200.0},
    "Huge": {"height": 4.0, "width": 2.0, "mass": 800.0}
}

ELEMENT_MATERIAL_PARAMS = {
    "Normal": {"emissive_strength": 0.0, "roughness": 0.6, "metallic": 0.1, "subsurface": 0.0},
    "Fire": {"emissive_strength": 5.0, "roughness": 0.2, "metallic": 0.0, "subsurface": 0.0, "color_shift": [1.0, 0.3, 0.0]},
    "Water": {"emissive_strength": 0.5, "roughness": 0.1, "metallic": 0.8, "subsurface": 0.9, "opacity": 0.7},
    "Earth": {"emissive_strength": 0.0, "roughness": 0.9, "metallic": 0.0, "subsurface": 0.1, "bump_scale": 1.5},
    "Wind": {"emissive_strength": 0.2, "roughness": 0.4, "metallic": 0.0, "subsurface": 0.0, "opacity": 0.8},
    "Lightning": {"emissive_strength": 8.0, "roughness": 0.1, "metallic": 0.9, "subsurface": 0.0, "flicker": True},
    "Ice": {"emissive_strength": 0.3, "roughness": 0.05, "metallic": 0.2, "subsurface": 0.8, "opacity": 0.6},
    "Dark": {"emissive_strength": 0.0, "roughness": 0.8, "metallic": 0.3, "subsurface": 0.0, "absorption": 0.5},
    "Light": {"emissive_strength": 6.0, "roughness": 0.3, "metallic": 0.1, "subsurface": 0.2, "halo": True},
    "Poison": {"emissive_strength": 1.0, "roughness": 0.4, "metallic": 0.0, "subsurface": 0.6, "color_shift": [0.2, 0.8, 0.2]},
    "Psychic": {"emissive_strength": 3.0, "roughness": 0.2, "metallic": 0.0, "subsurface": 0.4, "distortion": True}
}

def load_echo_data(file_path='astrawild_echoes_full.json'):
    """โหลดข้อมูล Echo จากไฟล์ JSON"""
    if not os.path.exists(file_path):
        print(f"Error: ไม่พบไฟล์ {file_path}")
        # สร้างข้อมูลจำลองหากไม่มีไฟล์ (เพื่อทดสอบระบบ)
        return generate_mock_data()
    
    with open(file_path, 'r', encoding='utf-8') as f:
        return json.load(f)

def generate_mock_data():
    """สร้างข้อมูลจำลองสำหรับการทดสอบ (ถ้าไม่มีไฟล์จริง)"""
    return [
        {
            "id": "ECH_001",
            "name": "Ember Fox",
            "family": "Vulpine",
            "body_plan": "Mammalian",
            "size_class": "Small",
            "element": "Fire",
            "base_color_rgb": [255, 100, 50],
            "secondary_color_rgb": [50, 20, 10]
        },
        {
            "id": "ECH_002",
            "name": "Aqua Beetle",
            "family": "Scarab",
            "body_plan": "Insectoid",
            "size_class": "Tiny",
            "element": "Water",
            "base_color_rgb": [50, 150, 255],
            "secondary_color_rgb": [200, 220, 255]
        }
    ]

def calculate_geometry_specs(echo):
    """คำนวณข้อมูลทางเรขาคณิตสำหรับสร้างโมเดล"""
    body_plan = echo.get('body_plan', 'Mammalian')
    size_class = echo.get('size_class', 'Medium')
    
    base_shape = BODY_PLAN_SHAPES.get(body_plan, BODY_PLAN_SHAPES['Mammalian'])
    scale = SIZE_CLASS_SCALE.get(size_class, SIZE_CLASS_SCALE['Medium'])
    
    # คำนวณสัดส่วนเพิ่มเติม
    proportions = {
        "head_scale": 1.0 + (0.1 if body_plan in ['Mammalian', 'Insectoid'] else 0),
        "limb_thickness": 0.8 if size_class in ['Tiny', 'Small'] else 1.2,
        "tail_length": 0.5 if not base_shape.get('tail') else (1.0 * scale['height'])
    }
    
    return {
        "primitive_type": base_shape['base_primitive'],
        "target_dimensions": {
            "height": scale['height'],
            "width": scale['width'],
            "depth": scale['width'] * 0.6
        },
        "topology": {
            "segment_count": base_shape['segments'],
            "limb_count": base_shape.get('limb_count', 4),
            "has_tail": base_shape.get('tail', False),
            "has_wings": base_shape.get('wings', False),
            "has_fins": base_shape.get('fins', False),
            "special_features": [k for k, v in base_shape.items() if v is True and k not in ['base_primitive', 'segments', 'limb_count']]
        },
        "proportions": proportions
    }

def calculate_material_specs(echo):
    """คำนวณค่าวัสดุและแชเดอร์"""
    element = echo.get('element', 'Normal')
    base_rgb = echo.get('base_color_rgb', [128, 128, 128])
    sec_rgb = echo.get('secondary_color_rgb', [50, 50, 50])
    
    elem_params = ELEMENT_MATERIAL_PARAMS.get(element, ELEMENT_MATERIAL_PARAMS['Normal'])
    
    # Normalize RGB to 0-1
    base_norm = [x/255.0 for x in base_rgb]
    sec_norm = [x/255.0 for x in sec_rgb]
    
    # Apply element color shift if exists
    if 'color_shift' in elem_params:
        shift = elem_params['color_shift']
        base_norm = [min(1.0, b * s) for b, s in zip(base_norm, shift)]
    
    return {
        "base_albedo": base_norm,
        "secondary_albedo": sec_norm,
        "shader_parameters": {
            "emissive_strength": elem_params.get('emissive_strength', 0.0),
            "roughness": elem_params.get('roughness', 0.5),
            "metallic": elem_params.get('metallic', 0.0),
            "subsurface_scattering": elem_params.get('subsurface', 0.0),
            "opacity": elem_params.get('opacity', 1.0),
            "normal_map_intensity": 1.0 if 'scales' in str(elem_params) else 0.5
        },
        "effects": {
            "flicker": elem_params.get('flicker', False),
            "halo": elem_params.get('halo', False),
            "distortion": elem_params.get('distortion', False),
            "particle_emission": elem_params.get('particles', False)
        }
    }

def generate_rigging_map(echo):
    """สร้างแผนผังกระดูก (Rigging Map) เบื้องต้น"""
    body_plan = echo.get('body_plan', 'Mammalian')
    geo = calculate_geometry_specs(echo)
    
    bones = ["Root", "Spine_01", "Spine_02", "Head"]
    
    # เพิ่มขา
    limb_count = geo['topology']['limb_count']
    for i in range(limb_count):
        side = "L" if i % 2 == 0 else "R"
        type_name = "Leg" if i >= 2 else "Arm"
        bones.extend([f"{type_name}_{side}_01", f"{type_name}_{side}_02", f"{type_name}_{side}_IK"])
        
    # เพิ่มหาง
    if geo['topology']['has_tail']:
        bones.extend(["Tail_01", "Tail_02", "Tail_End"])
        
    # เพิ่มปีก
    if geo['topology'].get('has_wings'):
        bones.extend(["Wing_L_01", "Wing_L_02", "Wing_R_01", "Wing_R_02"])
        
    return {
        "bone_hierarchy": bones,
        "control_rigs": ["Body_Ctrl", "Head_Ctrl", "Root_Ctrl"],
        "ik_chains": [b for b in bones if "IK" in b]
    }

def generate_lod_specs(echo):
    """กำหนดระยะการแสดงผล LOD (Level of Detail)"""
    size_class = echo.get('size_class', 'Medium')
    scale_factor = SIZE_CLASS_SCALE.get(size_class, SIZE_CLASS_SCALE['Medium'])['height']
    
    # ยิ่งตัวใหญ่ ระยะ LOD ยิ่งไกล
    base_dist = 1000.0 * scale_factor
    
    return {
        "LOD0": {"max_tris": 15000, "distance": 0},
        "LOD1": {"max_tris": 5000, "distance": base_dist * 0.3},
        "LOD2": {"max_tris": 1500, "distance": base_dist * 0.6},
        "LOD3": {"max_tris": 500, "distance": base_dist * 1.0},
        "collision_complexity": "UCX_Simple" if size_class in ['Tiny', 'Small'] else "UCX_Complex"
    }

def process_all_echoes():
    """ฟังก์ชันหลักในการประมวลผลทั้งหมด"""
    print("🚀 เริ่มต้นการสร้าง 3D Build Specifications...")
    
    # โหลดข้อมูล (สมมติว่ามีไฟล์ astrawild_echoes_full.json อยู่แล้ว)
    # หากต้องการทดสอบกับข้อมูลจริง ให้สร้างไฟล์ JSON นั้นก่อน
    echoes = load_echo_data('astrawild_echoes_full.json')
    
    build_specs = []
    
    for i, echo in enumerate(echoes):
        try:
            spec = {
                "meta": {
                    "echo_id": echo.get('id'),
                    "name": echo.get('name'),
                    "family": echo.get('family'),
                    "version": "1.0",
                    "generated_by": "AstrawildCore_Pipeline"
                },
                "geometry": calculate_geometry_specs(echo),
                "materials": calculate_material_specs(echo),
                "rigging": generate_rigging_map(echo),
                "lod": generate_lod_specs(echo),
                "asset_paths": {
                    "skeletal_mesh": f"/Game/Astrawild/Echoes/Meshes/{echo.get('family')}/SK_{echo.get('id')}.uasset",
                    "material_instance": f"/Game/Astrawild/Echoes/Materials/MI_{echo.get('id')}.uasset",
                    "texture_set": f"/Game/Astrawild/Echoes/Textures/{echo.get('id')}/T_{echo.get('id')}_*",
                    "animation_blueprint": f"/Game/Astrawild/Echoes/Blueprints/ABP_{echo.get('body_plan')}.uasset"
                }
            }
            build_specs.append(spec)
            
            if (i + 1) % 50 == 0:
                print(f"   processed {i+1}/{len(echoes)} echoes...")
                
        except Exception as e:
            print(f"⚠️ Error processing {echo.get('name')}: {e}")
            continue
            
    # บันทึกผลลัพธ์
    output_file = 'echo_3d_build_specs.json'
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(build_specs, f, indent=2, ensure_ascii=False)
        
    print(f"✅ สำเร็จ! สร้างไฟล์ specifications แล้ว: {output_file}")
    print(f"   จำนวนโมเดลที่พร้อมสร้าง: {len(build_specs)} ตัว")
    print(f"   ไฟล์นี้สามารถส่งให้ Blender/Houdini Script หรือ AI Model Generator ได้ทันที")

if __name__ == "__main__":
    process_all_echoes()
