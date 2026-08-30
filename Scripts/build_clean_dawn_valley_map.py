import unreal

def build_pristine_level():
    map_pkg = "/Game/Astrawild/Maps/LV_DawnValley_OpenWorld"
    unreal.log(f"[ASTRAWILD] Building pristine clean Open World map: {map_pkg}")
    
    subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    loaded = subsystem.load_level(map_pkg)
    unreal.log(f"[ASTRAWILD] Level loaded: {loaded}")
    
    # 1. Clear ALL existing actors in the level to avoid duplicates or competing lights
    existing_actors = unreal.EditorLevelLibrary.get_all_level_actors()
    unreal.log(f"[ASTRAWILD] Cleaning {len(existing_actors)} old actors...")
    for act in existing_actors:
        try:
            unreal.EditorLevelLibrary.destroy_actor(act)
        except Exception:
            pass
            
    # Load basic shapes
    cube_mesh = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    cyl_mesh = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
    sphere_mesh = unreal.load_asset("/Engine/BasicShapes/Sphere.Sphere")
    gray_grid_mat = unreal.load_asset("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial")
    
    # 2. EXACTLY ONE Sun (Directional Light)
    sun = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1000), unreal.Rotator(-45, 45, 0))
    if sun:
        sun.set_actor_label("Sun_Main")
        comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
        if comp:
            comp.set_editor_property("intensity", 100000.0) # Standard physical lux
            comp.set_editor_property("atmosphere_sun_light", True)
            comp.set_editor_property("atmosphere_sun_light_index", 0)
            comp.set_editor_property("forward_shading_priority", 1)
            comp.set_editor_property("cast_shadows", True)
            comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
        unreal.log("[ASTRAWILD] Created single Sun_Main (Priority=1, Index=0)")

    # 3. EXACTLY ONE SkyAtmosphere
    sky = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if sky:
        sky.set_actor_label("SkyAtmosphere_Main")
        unreal.log("[ASTRAWILD] Created SkyAtmosphere_Main")

    # 4. EXACTLY ONE SkyLight
    skylight = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500), unreal.Rotator(0, 0, 0))
    if skylight:
        skylight.set_actor_label("SkyLight_Main")
        comp = skylight.get_component_by_class(unreal.SkyLightComponent)
        if comp:
            comp.set_editor_property("real_time_capture", True)
            comp.set_editor_property("intensity", 2.5)
            comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
        unreal.log("[ASTRAWILD] Created SkyLight_Main")

    # 5. EXACTLY ONE ExponentialHeightFog
    fog = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if fog:
        fog.set_actor_label("ExponentialHeightFog_Main")
        comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
        if comp:
            try:
                comp.set_editor_property("fog_density", 0.001)
            except Exception:
                pass
        unreal.log("[ASTRAWILD] Created ExponentialHeightFog_Main")

    # 6. EXACTLY ONE Global PostProcessVolume
    pp = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if pp:
        pp.set_actor_label("PostProcess_Main")
        pp.set_editor_property("unbound", True)
        unreal.log("[ASTRAWILD] Created PostProcess_Main (Unbound)")

    # 7. Main Ground Meadow Floor (Flat surface at Z = 0)
    ground = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -25), unreal.Rotator(0, 0, 0))
    if ground:
        ground.set_actor_label("Ground_DawnMeadow")
        ground.set_actor_scale3d(unreal.Vector(2500.0, 2500.0, 0.5)) # 250m x 250m
        comp = ground.get_component_by_class(unreal.StaticMeshComponent)
        if comp and cube_mesh:
            comp.set_static_mesh(cube_mesh)
            if gray_grid_mat:
                comp.set_material(0, gray_grid_mat)
        unreal.log("[ASTRAWILD] Created Ground_DawnMeadow")

    # 8. Central Dawn Spire Dais & Landmark (North of Spawn)
    dais = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 1500, 15), unreal.Rotator(0, 0, 0))
    if dais:
        dais.set_actor_label("DawnSpire_Dais")
        dais.set_actor_scale3d(unreal.Vector(20.0, 20.0, 0.3))
        comp = dais.get_component_by_class(unreal.StaticMeshComponent)
        if comp and cyl_mesh:
            comp.set_static_mesh(cyl_mesh)

    monolith = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 1500, 300), unreal.Rotator(0, 0, 0))
    if monolith:
        monolith.set_actor_label("DawnSpire_Monolith")
        monolith.set_actor_scale3d(unreal.Vector(3.0, 3.0, 6.0))
        comp = monolith.get_component_by_class(unreal.StaticMeshComponent)
        if comp and cyl_mesh:
            comp.set_static_mesh(cyl_mesh)

    # 9. Sylvan Forest Trees (East and West)
    for i in range(12):
        tx = -1500 - (i % 3) * 600
        ty = 500 + (i // 3) * 600
        trunk = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(tx, ty, 150), unreal.Rotator(0, 0, 0))
        if trunk:
            trunk.set_actor_label(f"Tree_Trunk_{i+1}")
            trunk.set_actor_scale3d(unreal.Vector(1.0, 1.0, 3.0))
            comp = trunk.get_component_by_class(unreal.StaticMeshComponent)
            if comp and cyl_mesh:
                comp.set_static_mesh(cyl_mesh)

        canopy = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(tx, ty, 380), unreal.Rotator(0, 0, 0))
        if canopy:
            canopy.set_actor_label(f"Tree_Canopy_{i+1}")
            canopy.set_actor_scale3d(unreal.Vector(3.5, 3.5, 3.0))
            comp = canopy.get_component_by_class(unreal.StaticMeshComponent)
            if comp and sphere_mesh:
                comp.set_static_mesh(sphere_mesh)

    # 10. EXACTLY ONE PlayerStart at (0, 0, 80) facing North (0, 90, 0)
    pstart = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 80), unreal.Rotator(0, 90, 0))
    if pstart:
        pstart.set_actor_label("PlayerStart_Main")
        unreal.log("[ASTRAWILD] Created PlayerStart_Main at (0, 0, 80)")

    # 11. Save Pristine Level
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    unreal.log(f"[ASTRAWILD] Clean Level Build Complete! Total Clean Actors: {len(actors)}")
    
    saved = subsystem.save_current_level()
    unreal.log(f"[ASTRAWILD] Level save result: {saved}")
    
    saved_dirty = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"[ASTRAWILD] save_dirty_packages result: {saved_dirty}")

if __name__ == "__main__":
    build_pristine_level()
