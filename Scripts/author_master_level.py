import unreal

def build_master_level():
    map_pkg = "/Game/Astrawild/Maps/LV_DawnValley_OpenWorld"
    unreal.log(f"[ASTRAWILD] Authoring Master Open World Map: {map_pkg}")
    
    subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    subsystem.load_level(map_pkg)
    
    # 1. Clean all existing actors
    existing = unreal.EditorLevelLibrary.get_all_level_actors()
    unreal.log(f"[ASTRAWILD] Removing {len(existing)} old actors...")
    for act in existing:
        try:
            unreal.EditorLevelLibrary.destroy_actor(act)
        except Exception:
            pass
            
    # Load basic shapes
    cube_mesh = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    cyl_mesh = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
    sphere_mesh = unreal.load_asset("/Engine/BasicShapes/Sphere.Sphere")
    default_mat = unreal.load_asset("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")
    
    # 2. Hero DirectionalLight (Physical Sunlight: 75,000 Lux)
    sun = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.DirectionalLight, 
        unreal.Vector(0, 0, 1000), 
        unreal.Rotator(-38.0, 52.0, 0.0) # Golden hour elevation
    )
    if sun:
        sun.set_actor_label("Sun_Main")
        comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
        if comp:
            comp.set_editor_property("intensity", 75000.0) # Physical 75,000 Lux
            comp.set_editor_property("atmosphere_sun_light", True)
            comp.set_editor_property("atmosphere_sun_light_index", 0)
            comp.set_editor_property("forward_shading_priority", 1)
            comp.set_editor_property("cast_shadows", True)
            comp.set_editor_property("cast_volumetric_shadow", True)
            comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
            comp.set_editor_property("light_color", unreal.Color(255, 245, 230, 255))
        unreal.log("[ASTRAWILD] Sun_Main created with 75,000 Lux physical sunlight.")

    # 3. SkyAtmosphere (Physically-based blue sky)
    sky = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyAtmosphere, 
        unreal.Vector(0, 0, 0), 
        unreal.Rotator(0, 0, 0)
    )
    if sky:
        sky.set_actor_label("SkyAtmosphere_Main")
        unreal.log("[ASTRAWILD] SkyAtmosphere_Main created.")

    # 4. SkyLight with Real-Time Dynamic Reflection Capture
    skylight = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.SkyLight, 
        unreal.Vector(0, 0, 500), 
        unreal.Rotator(0, 0, 0)
    )
    if skylight:
        skylight.set_actor_label("SkyLight_Main")
        comp = skylight.get_component_by_class(unreal.SkyLightComponent)
        if comp:
            comp.set_editor_property("real_time_capture", True)
            comp.set_editor_property("intensity", 1.5)
            comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
        unreal.log("[ASTRAWILD] SkyLight_Main created with RealTimeCapture.")

    # 5. ExponentialHeightFog (Atmospheric haze & depth)
    fog = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.ExponentialHeightFog, 
        unreal.Vector(0, 0, 0), 
        unreal.Rotator(0, 0, 0)
    )
    if fog:
        fog.set_actor_label("ExponentialHeightFog_Main")
        comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
        if comp:
            comp.set_editor_property("fog_density", 0.002)
            comp.set_editor_property("fog_height_falloff", 0.2)
        unreal.log("[ASTRAWILD] ExponentialHeightFog_Main created.")

    # 6. Global PostProcessVolume (Histogram Auto-Exposure + Bloom)
    pp = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.PostProcessVolume, 
        unreal.Vector(0, 0, 0), 
        unreal.Rotator(0, 0, 0)
    )
    if pp:
        pp.set_actor_label("PostProcess_Main")
        pp.set_editor_property("unbound", True)
        settings = pp.get_editor_property("settings")
        settings.set_editor_property("override_auto_exposure_method", True)
        settings.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_HISTOGRAM)
        settings.set_editor_property("override_auto_exposure_min_brightness", True)
        settings.set_editor_property("auto_exposure_min_brightness", 0.03)
        settings.set_editor_property("override_auto_exposure_max_brightness", True)
        settings.set_editor_property("auto_exposure_max_brightness", 10.0)
        settings.set_editor_property("override_auto_exposure_speed_up", True)
        settings.set_editor_property("auto_exposure_speed_up", 4.0)
        settings.set_editor_property("override_auto_exposure_speed_down", True)
        settings.set_editor_property("auto_exposure_speed_down", 2.0)
        settings.set_editor_property("override_bloom_intensity", True)
        settings.set_editor_property("bloom_intensity", 0.675)
        pp.set_editor_property("settings", settings)
        unreal.log("[ASTRAWILD] PostProcess_Main created with full histogram auto-exposure.")

    # 7. Ground Meadow Floor (300m x 300m flat surface at Z = 0)
    ground = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor, 
        unreal.Vector(0, 0, -25), 
        unreal.Rotator(0, 0, 0)
    )
    if ground:
        ground.set_actor_label("Ground_DawnMeadow")
        ground.set_actor_scale3d(unreal.Vector(3000.0, 3000.0, 0.5))
        comp = ground.get_component_by_class(unreal.StaticMeshComponent)
        if comp and cube_mesh:
            comp.set_static_mesh(cube_mesh)
            if default_mat:
                comp.set_material(0, default_mat)
        unreal.log("[ASTRAWILD] Ground_DawnMeadow created (300m x 300m).")

    # 8. Dawn Spire Sanctuary Landmark (North of Spawn at Y = 1500)
    dais = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor, 
        unreal.Vector(0, 1500, 15), 
        unreal.Rotator(0, 0, 0)
    )
    if dais:
        dais.set_actor_label("DawnSpire_Dais")
        dais.set_actor_scale3d(unreal.Vector(25.0, 25.0, 0.3))
        comp = dais.get_component_by_class(unreal.StaticMeshComponent)
        if comp and cyl_mesh:
            comp.set_static_mesh(cyl_mesh)

    monolith = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.StaticMeshActor, 
        unreal.Vector(0, 1500, 350), 
        unreal.Rotator(0, 0, 0)
    )
    if monolith:
        monolith.set_actor_label("DawnSpire_Monolith")
        monolith.set_actor_scale3d(unreal.Vector(4.0, 4.0, 7.0))
        comp = monolith.get_component_by_class(unreal.StaticMeshComponent)
        if comp and cyl_mesh:
            comp.set_static_mesh(cyl_mesh)

    # 9. Sylvan Forest Groves (East and West)
    for i in range(16):
        tx = -1800 - (i % 4) * 600
        ty = 400 + (i // 4) * 600
        trunk = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.StaticMeshActor, 
            unreal.Vector(tx, ty, 150), 
            unreal.Rotator(0, 0, 0)
        )
        if trunk:
            trunk.set_actor_label(f"Tree_Trunk_{i+1}")
            trunk.set_actor_scale3d(unreal.Vector(1.2, 1.2, 3.0))
            comp = trunk.get_component_by_class(unreal.StaticMeshComponent)
            if comp and cyl_mesh:
                comp.set_static_mesh(cyl_mesh)

        canopy = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.StaticMeshActor, 
            unreal.Vector(tx, ty, 380), 
            unreal.Rotator(0, 0, 0)
        )
        if canopy:
            canopy.set_actor_label(f"Tree_Canopy_{i+1}")
            canopy.set_actor_scale3d(unreal.Vector(4.0, 4.0, 3.5))
            comp = canopy.get_component_by_class(unreal.StaticMeshComponent)
            if comp and sphere_mesh:
                comp.set_static_mesh(sphere_mesh)

    # 10. PlayerStart at (0, 0, 100) facing North (0, 90, 0)
    pstart = unreal.EditorLevelLibrary.spawn_actor_from_class(
        unreal.PlayerStart, 
        unreal.Vector(0, 0, 100), 
        unreal.Rotator(0, 90, 0)
    )
    if pstart:
        pstart.set_actor_label("PlayerStart_Main")
        unreal.log("[ASTRAWILD] PlayerStart_Main created at (0, 0, 100).")

    # 11. Save Pristine Master Level
    total_actors = len(unreal.EditorLevelLibrary.get_all_level_actors())
    unreal.log(f"[ASTRAWILD] Master Level Build Complete! Total Clean Actors: {total_actors}")
    
    saved = subsystem.save_current_level()
    unreal.log(f"[ASTRAWILD] Level save result: {saved}")
    saved_dirty = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"[ASTRAWILD] save_dirty_packages result: {saved_dirty}")

if __name__ == "__main__":
    build_master_level()
