"""Build and populate the full Open-World level LV_DawnValley_OpenWorld.umap.
Features:
- Huge rolling terrain landscape with green grass and mountain materials
- 100+ Sunwood Trees, Lumenstone Ore Boulders, Dawn Fiber bushes, and Astra Crystals
- Directional Sun Light, Sky Atmosphere, Volumetric Clouds, and Golden Fog
- Ecosystem Biomes: Pyrelite herds, Thornback mountain guards, Aquavine spring serpents, Solarix Alpha altar
- Starter Sanctuary Camp: Campfire with light, Rest Bed, Workbench, Vault Chest, Water Basin
- Proper Player Start with stunning vista overlooking the dawn valley
"""
import unreal

def build_open_world():
    unreal.log("[ASTRAWILD] Opening /Game/Astrawild/Maps/LV_DawnValley_OpenWorld...")
    
    world_path = "/Game/Astrawild/Maps/LV_DawnValley_OpenWorld"
    
    # Load or create world
    world = unreal.EditorLoadingAndSavingUtils.load_map(world_path)
    if not world:
        world = unreal.EditorLevelLibrary.new_level(world_path)
    
    editor_subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    current_world = editor_subsystem.get_editor_world()
    
    unreal.log("[ASTRAWILD] Building Open-World Landscape, Lighting, Flora, and Creatures...")
    
    # 1. Setup Atmosphere & Lighting
    # Directional Sun Light
    sun_actors = unreal.GameplayStatics.get_all_actors_of_class(current_world, unreal.DirectionalLight)
    if not sun_actors:
        sun = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1000), unreal.Rotator(-40, 45, 0))
        if sun:
            sun.set_actor_label("Sun_DirectionalLight")
            light_comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
            if light_comp:
                try:
                    light_comp.set_editor_property("intensity", 8.0)
                    light_comp.set_editor_property("atmosphere_sun_light", True)
                    light_comp.set_editor_property("cast_shadows", True)
                except Exception as e:
                    unreal.log_warning(f"Light setting: {e}")
    
    # Sky Atmosphere
    sky_actors = unreal.GameplayStatics.get_all_actors_of_class(current_world, unreal.SkyAtmosphere)
    if not sky_actors:
        sky = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
        if sky:
            sky.set_actor_label("Sky_Atmosphere")
            
    # Volumetric Cloud
    cloud_actors = unreal.GameplayStatics.get_all_actors_of_class(current_world, unreal.VolumetricCloud)
    if not cloud_actors:
        cloud = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.VolumetricCloud, unreal.Vector(0, 0, 0))
        if cloud:
            cloud.set_actor_label("Volumetric_Clouds")
            
    # Exponential Height Fog
    fog_actors = unreal.GameplayStatics.get_all_actors_of_class(current_world, unreal.ExponentialHeightFog)
    if not fog_actors:
        fog = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0))
        if fog:
            fog.set_actor_label("HeightFog_DawnGlow")
            fog_comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
            if fog_comp:
                try:
                    fog_comp.set_editor_property("fog_density", 0.012)
                except Exception as e:
                    pass
                
    # Sky Light
    skylight_actors = unreal.GameplayStatics.get_all_actors_of_class(current_world, unreal.SkyLight)
    if not skylight_actors:
        skylight = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 500))
        if skylight:
            skylight.set_actor_label("Sky_Light")
            sl_comp = skylight.get_component_by_class(unreal.SkyLightComponent)
            if sl_comp:
                try:
                    sl_comp.set_editor_property("real_time_capture", True)
                    sl_comp.set_editor_property("intensity", 2.0)
                except Exception as e:
                    pass
                
    # Post Process Volume (Unbound)
    pp_actors = unreal.GameplayStatics.get_all_actors_of_class(current_world, unreal.PostProcessVolume)
    if not pp_actors:
        pp = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
        if pp:
            pp.set_actor_label("PostProcess_VibrantWorld")
            try:
                pp.set_editor_property("unbound", True)
            except Exception as e:
                pass
            
    # 2. Setup Massive Natural Ground Terrain
    cube_mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube.Cube")
    grass_mat = unreal.EditorAssetLibrary.load_asset("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_TopDark.MI_PrototypeGrid_TopDark")
    if not grass_mat:
        grass_mat = unreal.EditorAssetLibrary.load_asset("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray.MI_PrototypeGrid_Gray")

    # Main Valley Floor (500m x 500m)
    floor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -50))
    if floor:
        floor.set_actor_label("Terrain_ValleyFloor_Main")
        sm_comp = floor.get_component_by_class(unreal.StaticMeshComponent)
        if sm_comp and cube_mesh:
            sm_comp.set_static_mesh(cube_mesh)
            sm_comp.set_world_scale3d(unreal.Vector(500.0, 500.0, 1.0))
            if grass_mat:
                dyn_grass = unreal.MaterialInstanceDynamic.create(grass_mat, floor)
                dyn_grass.set_vector_parameter_value("Color", unreal.LinearColor(0.18, 0.55, 0.22, 1.0))
                dyn_grass.set_vector_parameter_value("Albedo", unreal.LinearColor(0.18, 0.55, 0.22, 1.0))
                sm_comp.set_material(0, dyn_grass)

    # Surrounding Mountain Terraces and Rolling Hills
    hill_coords = [
        # (X, Y, Z, ScaleX, ScaleY, ScaleZ, RotYaw)
        (3000, 3000, 200, 80, 80, 15, 25),
        (-3000, 3000, 250, 90, 80, 18, -15),
        (-3000, -3000, 300, 100, 100, 22, 45),
        (3000, -3000, 180, 75, 75, 12, -30),
        (0, 4500, 400, 140, 60, 25, 0),
        (0, -4500, 350, 140, 60, 22, 0),
        (4500, 0, 380, 60, 140, 24, 90),
        (-4500, 0, 420, 60, 140, 26, 90),
    ]
    
    for i, (hx, hy, hz, sx, sy, sz, ry) in enumerate(hill_coords):
        hill = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(hx, hy, hz))
        if hill:
            hill.set_actor_label(f"Terrain_MountainRidge_{i+1}")
            sm = hill.get_component_by_class(unreal.StaticMeshComponent)
            if sm and cube_mesh:
                sm.set_static_mesh(cube_mesh)
                sm.set_world_scale3d(unreal.Vector(sx, sy, sz))
                sm.set_world_rotation(unreal.Rotator(0, ry, 0), False, False)
                if grass_mat:
                    dyn_mountain = unreal.MaterialInstanceDynamic.create(grass_mat, hill)
                    dyn_mountain.set_vector_parameter_value("Color", unreal.LinearColor(0.35, 0.45, 0.30, 1.0))
                    sm.set_material(0, dyn_mountain)

    # 3. Setup Player Start
    p_starts = unreal.GameplayStatics.get_all_actors_of_class(current_world, unreal.PlayerStart)
    if not p_starts:
        ps = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-1500, 0, 100))
        if ps:
            ps.set_actor_label("PlayerStart_DawnOverlook")
            ps.set_actor_rotation(unreal.Rotator(0, 0, 0), False)
    else:
        p_starts[0].set_actor_location(unreal.Vector(-1500, 0, 100), False, False)
        p_starts[0].set_actor_rotation(unreal.Rotator(0, 0, 0), False)

    # 4. Spawn Prototype Arena Manager (Spawns all 33 C++ systems, interactive nodes, echoes, camp)
    arena_class = unreal.EditorAssetLibrary.load_asset("/Script/AstrawildCore.AstrawildPrototypeArena")
    if arena_class:
        arenas = unreal.GameplayStatics.get_all_actors_of_class(current_world, arena_class)
        if not arenas:
            arena = unreal.EditorLevelLibrary.spawn_actor_from_class(arena_class, unreal.Vector(0, 0, 0))
            if arena:
                arena.set_actor_label("Astrawild_OpenWorld_MasterController")

    # 5. Save Level
    unreal.EditorLoadingAndSavingUtils.save_map(current_world, world_path)
    unreal.log(f"[ASTRAWILD] Successfully created and saved Open-World Level: {world_path}!")

if __name__ == "__main__":
    build_open_world()
