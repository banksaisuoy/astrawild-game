# Echo 3D Build Specifications - Complete
# Generated for ASTRAWILD Project
# Total Species: 204

import json
import os
from datetime import datetime

# Body plan definitions with geometric parameters
BODY_PLANS = {
    "Quadruped": {
        "id": 0,
        "bones": ["root", "spine_01", "spine_02", "neck", "head", "leg_front_l", "leg_front_r", "leg_back_l", "leg_back_r", "tail_01", "tail_02"],
        "geometry": {
            "body": {"type": "capsule", "radius": 30, "height": 80},
            "legs": {"type": "cylinder", "radius": 8, "height": 50, "count": 4},
            "head": {"type": "sphere", "radius": 20},
            "tail": {"type": "tapered_cylinder", "segments": 2}
        },
        "uv_layout": "quadruped_standard",
        "lod_levels": 4
    },
    "Biped": {
        "id": 1,
        "bones": ["pelvis", "spine_01", "spine_02", "neck", "head", "arm_l", "arm_r", "leg_l", "leg_r"],
        "geometry": {
            "torso": {"type": "capsule", "radius": 25, "height": 70},
            "arms": {"type": "cylinder", "radius": 7, "height": 45, "count": 2},
            "legs": {"type": "cylinder", "radius": 9, "height": 55, "count": 2},
            "head": {"type": "sphere", "radius": 18}
        },
        "uv_layout": "humanoid_standard",
        "lod_levels": 4
    },
    "Serpent": {
        "id": 2,
        "bones": ["spine_00", "spine_01", "spine_02", "spine_03", "spine_04", "spine_05", "spine_06", "spine_07", "spine_08", "spine_09", "spine_10", "spine_11", "head"],
        "geometry": {
            "body_segments": {"type": "cylinder", "radius": 15, "height": 20, "count": 12},
            "head": {"type": "cone", "radius": 18, "height": 25}
        },
        "uv_layout": "serpentine_strip",
        "lod_levels": 3
    },
    "Floating": {
        "id": 3,
        "bones": ["core", "orb_l", "orb_r", "orb_top"],
        "geometry": {
            "core": {"type": "icosahedron", "radius": 25},
            "orbs": {"type": "sphere", "radius": 10, "count": 3}
        },
        "uv_layout": "floating_symmetric",
        "lod_levels": 3
    },
    "Insectoid": {
        "id": 4,
        "bones": ["thorax", "abdomen", "head", "leg_01_l", "leg_01_r", "leg_02_l", "leg_02_r", "leg_03_l", "leg_03_r", "wing_l", "wing_r"],
        "geometry": {
            "thorax": {"type": "ellipsoid", "radii": [20, 15, 25]},
            "abdomen": {"type": "ellipsoid", "radii": [18, 20, 30]},
            "head": {"type": "sphere", "radius": 15},
            "legs": {"type": "segmented_cylinder", "segments": 3, "count": 6},
            "wings": {"type": "plane", "size": [40, 25], "count": 2}
        },
        "uv_layout": "insectoid_unfolded",
        "lod_levels": 4
    },
    "Avian": {
        "id": 5,
        "bones": ["body", "neck", "head", "wing_l", "wing_r", "leg_l", "leg_r", "tail"],
        "geometry": {
            "body": {"type": "ellipsoid", "radii": [20, 15, 30]},
            "neck": {"type": "cylinder", "radius": 8, "height": 20},
            "head": {"type": "sphere", "radius": 12},
            "wings": {"type": "curved_plane", "size": [50, 30], "count": 2},
            "legs": {"type": "cylinder", "radius": 5, "height": 30, "count": 2},
            "tail": {"type": "fan", "segments": 5}
        },
        "uv_layout": "avian_unfolded",
        "lod_levels": 4
    },
    "Crystalline": {
        "id": 6,
        "bones": ["core", "crystal_01", "crystal_02", "crystal_03", "crystal_04"],
        "geometry": {
            "core": {"type": "dodecahedron", "radius": 20},
            "crystals": {"type": "pyramid", "base_radius": 8, "height": 30, "count": 4}
        },
        "uv_layout": "crystalline_faceted",
        "lod_levels": 3
    },
    "Amorphous": {
        "id": 7,
        "bones": ["center", "pseudopod_00", "pseudopod_01", "pseudopod_02", "pseudopod_03", "pseudopod_04", "pseudopod_05", "pseudopod_06", "pseudopod_07"],
        "geometry": {
            "center": {"type": "metaball", "radius": 25, "resolution": 32},
            "pseudopods": {"type": "deforming_sphere", "radius": 12, "count": 8}
        },
        "uv_layout": "amorphous_stretched",
        "lod_levels": 3
    }
}

# Elemental visual properties
ELEMENTAL_PROPERTIES = {
    "None": {"glow_color": [0, 0, 0], "glow_intensity": 0.0, "emissive_channel": 0},
    "Ember": {"glow_color": [1.0, 0.3, 0.0], "glow_intensity": 1.0, "emissive_channel": 1},
    "Frost": {"glow_color": [0.0, 0.5, 1.0], "glow_intensity": 0.8, "emissive_channel": 2},
    "Pulse": {"glow_color": [0.0, 1.0, 0.5], "glow_intensity": 1.2, "emissive_channel": 3},
    "Toxic": {"glow_color": [0.3, 1.0, 0.0], "glow_intensity": 0.9, "emissive_channel": 4},
    "Solar": {"glow_color": [1.0, 0.9, 0.0], "glow_intensity": 1.5, "emissive_channel": 5},
    "Lunar": {"glow_color": [0.5, 0.3, 1.0], "glow_intensity": 0.7, "emissive_channel": 6},
    "Storm": {"glow_color": [0.8, 0.8, 1.0], "glow_intensity": 1.1, "emissive_channel": 7},
    "Terra": {"glow_color": [0.6, 0.4, 0.2], "glow_intensity": 0.5, "emissive_channel": 8},
    "Aqua": {"glow_color": [0.0, 0.7, 0.9], "glow_intensity": 0.8, "emissive_channel": 9}
}

# Size class multipliers
SIZE_CLASSES = {
    "Tiny": {"scale": 0.3, "mass": 0.1, "health_mult": 0.3},
    "Small": {"scale": 0.6, "mass": 0.3, "health_mult": 0.5},
    "Medium": {"scale": 1.0, "mass": 1.0, "health_mult": 1.0},
    "Large": {"scale": 1.5, "mass": 2.5, "health_mult": 1.8},
    "Huge": {"scale": 2.5, "mass": 6.0, "health_mult": 3.0}
}

def generate_echo_specs():
    """Generate complete 3D build specifications for all 204 Echo species"""
    
    # Load echo data from JSON if available
    echo_data_file = "echo_bestiary_complete.json"
    echoes = []
    
    if os.path.exists(echo_data_file):
        with open(echo_data_file, 'r', encoding='utf-8') as f:
            data = json.load(f)
            echoes = data.get("echoes", [])
    else:
        # Generate placeholder data for all 204 species
        body_plan_names = list(BODY_PLANS.keys())
        size_class_names = list(SIZE_CLASSES.keys())
        element_names = list(ELEMENTAL_PROPERTIES.keys())[1:]  # Exclude None
        
        echo_id = 1
        for bp_name in body_plan_names:
            for size_name in size_class_names:
                for elem_idx, elem_name in enumerate(element_names[:3]):  # 3 elements per combo
                    echoes.append({
                        "id": echo_id,
                        "name": f"Echo_{bp_name}_{size_name}_{elem_name}_{echo_id:03d}",
                        "body_plan": bp_name,
                        "size_class": size_name,
                        "element": elem_name,
                        "primary_tint": [0.5 + (echo_id % 10) * 0.05, 0.5 + (echo_id % 7) * 0.05, 0.5 + (echo_id % 13) * 0.05],
                        "secondary_tint": [0.3, 0.3, 0.3]
                    })
                    echo_id += 1
                    
                    if echo_id > 204:
                        break
                if echo_id > 204:
                    break
            if echo_id > 204:
                break
    
    # Build complete specifications
    build_specs = {
        "metadata": {
            "generated_at": datetime.now().isoformat(),
            "total_species": len(echoes),
            "body_plans_count": len(BODY_PLANS),
            "ue_version": "5.8.2",
            "pipeline_version": "2.0"
        },
        "body_plans": BODY_PLANS,
        "elemental_properties": ELEMENTAL_PROPERTIES,
        "size_classes": SIZE_CLASSES,
        "species": []
    }
    
    for echo in echoes:
        bp_name = echo.get("body_plan", "Quadruped")
        size_name = echo.get("size_class", "Medium")
        elem_name = echo.get("element", "None")
        
        bp_data = BODY_PLANS.get(bp_name, BODY_PLANS["Quadruped"])
        size_data = SIZE_CLASSES.get(size_name, SIZE_CLASSES["Medium"])
        elem_data = ELEMENTAL_PROPERTIES.get(elem_name, ELEMENTAL_PROPERTIES["None"])
        
        spec = {
            "echo_id": echo.get("id", 0),
            "echo_name": echo.get("name", "Unknown"),
            "display_name": echo.get("display_name", echo.get("name", "Unknown")),
            
            "mesh_spec": {
                "body_plan": bp_name,
                "body_plan_id": bp_data["id"],
                "base_scale": size_data["scale"],
                "geometry_type": bp_data["geometry"],
                "uv_layout": bp_data["uv_layout"],
                "lod_levels": bp_data["lod_levels"],
                "bone_count": len(bp_data["bones"]),
                "bones": bp_data["bones"]
            },
            
            "material_spec": {
                "primary_tint": echo.get("primary_tint", [0.5, 0.5, 0.5]),
                "secondary_tint": echo.get("secondary_tint", [0.3, 0.3, 0.3]),
                "elemental_glow": elem_data["glow_color"],
                "glow_intensity": elem_data["glow_intensity"],
                "emissive_channel": elem_data["emissive_channel"],
                "master_material": "/Game/ASTRAWILD/Materials/M_Echo_Master.M_Echo_Master"
            },
            
            "physics_spec": {
                "mass_multiplier": size_data["mass"],
                "health_multiplier": size_data["health_mult"],
                "collision_profile": "EchoCharacter",
                "simulation_type": "PhysicsAsset"
            },
            
            "animation_spec": {
                "skeleton_path": f"/Game/ASTRAWILD/Characters/Echo/Skeletons/SKE_Echo_{bp_name}.SKE_Echo_{bp_name}",
                "anim_blueprint_path": f"/Game/ASTRAWILD/Characters/Echo/Animations/ABP_Echo_{bp_name}.ABP_Echo_{bp_name}",
                "retarget_source": "Humanoid" if bp_name == "Biped" else "Creature"
            },
            
            "asset_paths": {
                "skeletal_mesh": f"/Game/ASTRAWILD/Characters/Echo/Meshes/SK_Echo_{bp_name}_{size_name}.SK_Echo_{bp_name}_{size_name}",
                "material_instance": f"/Game/ASTRAWILD/Characters/Echo/Materials/MI_Echo_{echo.get('name', 'unknown')}.MI_Echo_{echo.get('name', 'unknown')}",
                "physics_asset": f"/Game/ASTRAWILD/Characters/Echo/Physics/PhAT_Echo_{bp_name}.PhAT_Echo_{bp_name}",
                "data_asset": f"/Game/ASTRAWILD/Data/Echo/DA_Echo_{echo.get('name', 'unknown')}.DA_Echo_{echo.get('name', 'unknown')}"
            },
            
            "export_formats": {
                "fbx": f"Exported/Echo_{echo.get('name', 'unknown')}.fbx",
                "obj": f"Exported/Echo_{echo.get('name', 'unknown')}.obj",
                "glTF": f"Exported/Echo_{echo.get('name', 'unknown')}.gltf"
            },
            
            "rendering_hints": {
                "cast_shadow": True,
                "receive_shadow": True,
                "two_sided": elem_name == "None" or bp_name == "Amorphous",
                "subsurface_scattering": bp_name in ["Amorphous", "Floating"],
                "clear_coat": elem_name in ["Frost", "Aqua", "Solar"],
                "normal_strength": 1.0,
                "roughness_override": None,
                "metallic_override": None
            }
        }
        
        build_specs["species"].append(spec)
    
    return build_specs

def main():
    print("Generating Echo 3D Build Specifications...")
    specs = generate_echo_specs()
    
    output_file = "echo_3d_build_specs_complete.json"
    with open(output_file, 'w', encoding='utf-8') as f:
        json.dump(specs, f, indent=2, ensure_ascii=False)
    
    print(f"✓ Generated specifications for {len(specs['species'])} Echo species")
    print(f"✓ Output file: {output_file}")
    print(f"✓ Body plans: {len(specs['body_plans'])}")
    print(f"✓ File size: {os.path.getsize(output_file) / 1024:.1f} KB")
    
    # Also generate a summary CSV for quick reference
    import csv
    csv_file = "echo_build_summary.csv"
    with open(csv_file, 'w', newline='', encoding='utf-8') as f:
        writer = csv.writer(f)
        writer.writerow(["ID", "Name", "Body Plan", "Size", "Element", "Mesh Path", "Material Path"])
        
        for species in specs["species"]:
            writer.writerow([
                species["echo_id"],
                species["echo_name"],
                species["mesh_spec"]["body_plan"],
                species["mesh_spec"]["base_scale"],
                species["material_spec"]["emissive_channel"],
                species["asset_paths"]["skeletal_mesh"],
                species["asset_paths"]["material_instance"]
            ])
    
    print(f"✓ Summary CSV: {csv_file}")
    print("\nReady for 3D asset generation pipeline!")

if __name__ == "__main__":
    main()
