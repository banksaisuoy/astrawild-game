import unreal

def build_open_world_level():
    map_pkg = "/Game/Astrawild/Maps/LV_DawnValley_OpenWorld"
    unreal.log(f"[ASTRAWILD] Initializing level authoring for: {map_pkg}")
    
    subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    
    # 1. Load the existing level into editor
    loaded = subsystem.load_level(map_pkg)
    unreal.log(f"[ASTRAWILD] Level loaded status: {loaded}")
    
    world = unreal.EditorLevelLibrary.get_editor_world()
    
    cube_mesh = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    cyl_mesh = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
    sphere_mesh = unreal.load_asset("/Engine/BasicShapes/Sphere.Sphere")
    
    unreal.log(f"[ASTRAWILD] Basic meshes loaded: Cube={bool(cube_mesh)}, Cyl={bool(cyl_mesh)}, Sphere={bool(sphere_mesh)}")
    
    # 2. Spawn Directional Light (Sun)
    sun = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1500), unreal.Rotator(-45, 50, 0))
    if sun:
        sun.set_actor_label("Sun_DirectionalLight")
        comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
        if comp:
            comp.set_editor_property("intensity", 75000.0)
            comp.set_editor_property("atmosphere_sun_light", True)
            comp.set_editor_property("cast_shadows", True)
            try:
                comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
            except Exception:
                pass
        unreal.log("[ASTRAWILD] Spawned Sun_DirectionalLight")

    # 3. Sky Atmosphere
    sky = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if sky:
        sky.set_actor_label("SkyAtmosphere")
        unreal.log("[ASTRAWILD] Spawned SkyAtmosphere")

    # 4. Sky Light
    skylight = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 800), unreal.Rotator(0, 0, 0))
    if skylight:
        skylight.set_actor_label("SkyLight")
        comp = skylight.get_component_by_class(unreal.SkyLightComponent)
        if comp:
            comp.set_editor_property("real_time_capture", True)
            comp.set_editor_property("intensity", 3.5)
            try:
                comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
            except Exception:
                pass
        unreal.log("[ASTRAWILD] Spawned SkyLight")

    # 5. Exponential Height Fog
    fog = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if fog:
        fog.set_actor_label("ExponentialHeightFog")
        comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
        if comp:
            try:
                comp.set_editor_property("fog_density", 0.003)
            except Exception:
                pass
        unreal.log("[ASTRAWILD] Spawned ExponentialHeightFog")

    # 6. Global Post Process Volume
    pp = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if pp:
        pp.set_actor_label("GlobalPostProcess")
        pp.set_editor_property("unbound", True)
        unreal.log("[ASTRAWILD] Spawned GlobalPostProcess")

    # 7. Main Meadow Floor (300m x 300m at Z = 0)
    floor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if floor:
        floor.set_actor_label("Terrain_DawnMeadow_Floor")
        floor.set_actor_scale3d(unreal.Vector(3000.0, 3000.0, 1.0)) # 300m x 300m
        comp = floor.get_component_by_class(unreal.StaticMeshComponent)
        if comp and cube_mesh:
            comp.set_static_mesh(cube_mesh)
        unreal.log("[ASTRAWILD] Spawned Terrain_DawnMeadow_Floor")

    # 8. Central Dawn Spire & Dais
    dais = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 30), unreal.Rotator(0, 0, 0))
    if dais:
        dais.set_actor_label("DawnSpire_Dais")
        dais.set_actor_scale3d(unreal.Vector(30.0, 30.0, 1.5))
        comp = dais.get_component_by_class(unreal.StaticMeshComponent)
        if comp and cyl_mesh:
            comp.set_static_mesh(cyl_mesh)

    monolith = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 700), unreal.Rotator(0, 0, 0))
    if monolith:
        monolith.set_actor_label("DawnSpire_Monolith")
        monolith.set_actor_scale3d(unreal.Vector(5.0, 5.0, 14.0))
        comp = monolith.get_component_by_class(unreal.StaticMeshComponent)
        if comp and cyl_mesh:
            comp.set_static_mesh(cyl_mesh)

    # 9. Perimeter Mountain Wall
    mtn_positions = [
        (14000, 0, 800, 300, 2800, 20), (-14000, 0, 800, 300, 2800, 20),
        (0, 14000, 800, 2800, 300, 20), (0, -14000, 800, 2800, 300, 20)
    ]
    for idx, (x, y, z, sx, sy, sz) in enumerate(mtn_positions):
        mtn = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z), unreal.Rotator(0, 0, 0))
        if mtn:
            mtn.set_actor_label(f"Perimeter_Mountain_{idx+1}")
            mtn.set_actor_scale3d(unreal.Vector(sx, sy, sz))
            comp = mtn.get_component_by_class(unreal.StaticMeshComponent)
            if comp and cube_mesh:
                comp.set_static_mesh(cube_mesh)

    # 10. Forest Trees (20 Trees in Sylvan Grove)
    for i in range(20):
        tx = -2500 - (i % 5) * 700
        ty = 2500 + (i // 5) * 700
        trunk = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(tx, ty, 200), unreal.Rotator(0, 0, 0))
        if trunk:
            trunk.set_actor_label(f"Forest_Tree_{i+1}")
            trunk.set_actor_scale3d(unreal.Vector(1.2, 1.2, 4.5))
            comp = trunk.get_component_by_class(unreal.StaticMeshComponent)
            if comp and cyl_mesh:
                comp.set_static_mesh(cyl_mesh)

        canopy = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(tx, ty, 520), unreal.Rotator(0, 0, 0))
        if canopy:
            canopy.set_actor_label(f"Forest_Canopy_{i+1}")
            canopy.set_actor_scale3d(unreal.Vector(4.5, 4.5, 4.0))
            comp = canopy.get_component_by_class(unreal.StaticMeshComponent)
            if comp and sphere_mesh:
                comp.set_static_mesh(sphere_mesh)

    # 11. Player Start Actor (Standing near central dais)
    pstart = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, -800, 150), unreal.Rotator(0, 90, 0))
    if pstart:
        pstart.set_actor_label("PlayerStart_DawnValley")
        unreal.log("[ASTRAWILD] Spawned PlayerStart_DawnValley at (0, -800, 150)")

    # 12. Save Current Level
    all_actors = unreal.EditorLevelLibrary.get_all_level_actors()
    unreal.log(f"[ASTRAWILD] Level Population Complete. Total Actors: {len(all_actors)}")
    
    saved = subsystem.save_current_level()
    unreal.log(f"[ASTRAWILD] LevelEditorSubsystem.save_current_level returned: {saved}")
    
    saved_dirty = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"[ASTRAWILD] save_dirty_packages returned: {saved_dirty}")

if __name__ == "__main__":
    build_open_world_level()
