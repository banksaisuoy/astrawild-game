import unreal

def setup_scifi_world_art():
    unreal.log("==================================================")
    unreal.log("🚀 ASTRAWILD: SETTING UP SCI-FI FRONTIER ART PASS")
    unreal.log("==================================================")
    
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    editor_asset_subsystem = unreal.get_editor_subsystem(unreal.EditorAssetSubsystem)
    level_editor_subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    
    map_pkg = "/Game/ThirdPerson/Lvl_ThirdPerson"
    level_editor_subsystem.load_level(map_pkg)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    
    # 1. Create Landscape Material: M_Landscape_SciFiFrontier
    mat_path = "/Game/Astrawild/Materials"
    if not editor_asset_subsystem.does_directory_exist(mat_path):
        editor_asset_subsystem.make_directory(mat_path)
        
    mat_factory = unreal.MaterialFactoryNew()
    mat_asset_name = "M_Landscape_SciFiFrontier"
    
    mat = editor_asset_subsystem.load_asset(f"{mat_path}/{mat_asset_name}")
    if not mat:
        mat = asset_tools.create_asset(mat_asset_name, mat_path, unreal.Material, mat_factory)
        unreal.log(f"Created Master Material: {mat_asset_name}")
        
    if mat:
        # Create Material Expressions
        mat_editor_library = unreal.MaterialEditingLibrary
        mat_editor_library.delete_all_material_expressions(mat)
        
        # Color 1: Lush Alien Grass
        grass_node = mat_editor_library.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -600, -200)
        grass_node.set_editor_property("parameter_name", "GrassColor")
        grass_node.set_editor_property("default_value", unreal.LinearColor(0.12, 0.45, 0.16, 1.0))
        
        # Color 2: Cliff Granite Rock
        rock_node = mat_editor_library.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -600, 100)
        rock_node.set_editor_property("parameter_name", "RockColor")
        rock_node.set_editor_property("default_value", unreal.LinearColor(0.22, 0.24, 0.28, 1.0))
        
        # World Normal & Dot Product for Slope Blending
        normal_node = mat_editor_library.create_material_expression(mat, unreal.MaterialExpressionWorldNormal, -800, 300)
        vec_up = mat_editor_library.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -800, 450)
        vec_up.set_editor_property("constant", unreal.LinearColor(0, 0, 1, 0))
        
        dot_node = mat_editor_library.create_material_expression(mat, unreal.MaterialExpressionDotProduct, -550, 350)
        mat_editor_library.connect_material_expressions(normal_node, "", dot_node, "A")
        mat_editor_library.connect_material_expressions(vec_up, "", dot_node, "B")
        
        # Clamp & Contrast Slope Blend
        power_node = mat_editor_library.create_material_expression(mat, unreal.MaterialExpressionPower, -400, 350)
        mat_editor_library.connect_material_expressions(dot_node, "", power_node, "Base")
        exp_const = mat_editor_library.create_material_expression(mat, unreal.MaterialExpressionConstant, -550, 480)
        exp_const.set_editor_property("r", 4.0)
        mat_editor_library.connect_material_expressions(exp_const, "", power_node, "Exponent")
        
        # Lerp Grass vs Rock based on slope
        lerp_node = mat_editor_library.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, -200, 0)
        mat_editor_library.connect_material_expressions(rock_node, "", lerp_node, "A")
        mat_editor_library.connect_material_expressions(grass_node, "", lerp_node, "B")
        mat_editor_library.connect_material_expressions(power_node, "", lerp_node, "Alpha")
        
        # Connect to BaseColor
        mat_editor_library.connect_material_property(lerp_node, "", unreal.MaterialProperty.MP_BASE_COLOR)
        
        # Roughness: 0.82
        rough_const = mat_editor_library.create_material_expression(mat, unreal.MaterialExpressionConstant, -200, 200)
        rough_const.set_editor_property("r", 0.82)
        mat_editor_library.connect_material_property(rough_const, "", unreal.MaterialProperty.MP_ROUGHNESS)
        
        mat_editor_library.update_material_after_render_data_change(mat)
        mat_editor_library.recompile_material(mat)
        editor_asset_subsystem.save_asset(f"{mat_path}/{mat_asset_name}")
        unreal.log("SUCCESS: M_Landscape_SciFiFrontier authored with procedural slope blending!")

    # 2. Apply Material to Landscape and Terrain Meshes in World
    actors = unreal.EditorActorSubsystem().get_all_level_actors()
    applied_count = 0
    for a in actors:
        actor_name = a.get_name()
        # If Landscape or StaticMesh floor
        if "Landscape" in actor_name or "Floor" in actor_name or "Plane" in actor_name or "Ground" in actor_name or "Ramp" in actor_name or "Cube" in actor_name:
            components = a.get_components_by_class(unreal.StaticMeshComponent)
            for smc in components:
                smc.set_material(0, mat)
                applied_count += 1
            if isinstance(a, unreal.LandscapeProxy):
                a.set_editor_property("landscape_material", mat)
                applied_count += 1
                
    unreal.log(f"Applied Sci-Fi Landscape Material to {applied_count} terrain elements!")
    
    # 3. Setup Directional Light & Sun Atmosphere
    sun_actors = [a for a in actors if isinstance(a, unreal.DirectionalLight)]
    if sun_actors:
        sun = sun_actors[0]
        sun_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
        if sun_comp:
            sun_comp.set_editor_property("intensity", 75000.0) # 75k Lux Daylight
            sun_comp.set_editor_property("light_color", unreal.Color(255, 244, 230, 255)) # Warm sun
            sun_comp.set_editor_property("atmosphere_sun_light", True)
            sun_comp.set_editor_property("cast_shadows", True)
            unreal.log("Updated Directional Sun: 75,000 Lux Golden Warm Lighting")

    # 4. Save Level
    unreal.EditorLoadingAndSavingUtils.save_current_level()
    unreal.log("🎉 World Art Pass Applied and Saved Successfully!")

if __name__ == "__main__":
    setup_scifi_world_art()
