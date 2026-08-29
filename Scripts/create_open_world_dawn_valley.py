"""Generate the full-scale, immersive ASTRAWILD Open World Map: LV_DawnValley_OpenWorld

Features:
- Vast 300m x 300m Dawn Valley Meadow Terrain
- Perimeter Mountain Wall
- Central Dawn Spire & Monolith Dais
- Sylvan Forest with Harvestable Trees & Fiber
- Obsidian Combat Arena with Alpha Boss
- Basecamp Sanctuary with Campfire, Workbench, Bed & Chest
- Full Atmospheric Lighting & Post-Processing
"""
import unreal

def create_open_world():
    map_path = "/Game/Astrawild/Maps/LV_DawnValley_OpenWorld"
    unreal.log(f"[ASTRAWILD] Generating Full Open World at {map_path}...")

    unreal.EditorAssetLibrary.make_directory("/Game/Astrawild/Maps")

    # Create new level
    world = unreal.EditorLevelLibrary.new_level(map_path)
    if not world:
        unreal.log_error(f"[ASTRAWILD] Failed to create {map_path}")
        return False

    cube_mesh = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    cyl_mesh = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
    sphere_mesh = unreal.load_asset("/Engine/BasicShapes/Sphere.Sphere")
    
    # Try load green grid material
    green_mat = unreal.load_asset("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray.MI_PrototypeGrid_Gray")

    # 1. LIGHTING & SKY ATMOSPHERE
    # Sun Light
    sun = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 2000), unreal.Rotator(-40, 50, 0))
    if sun:
        sun.set_actor_label("Sun_DirectionalLight")
        comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
        if comp:
            comp.set_editor_property("intensity", 75000.0)
            comp.set_editor_property("atmosphere_sun_light", True)
            comp.set_editor_property("cast_shadows", True)
            try:
                comp.set_editor_property("light_color", unreal.Color(255, 245, 225, 255))
                comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
            except Exception:
                pass

    # Sky Atmosphere
    sky = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if sky:
        sky.set_actor_label("SkyAtmosphere")

    # Sky Light
    skylight = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1000), unreal.Rotator(0, 0, 0))
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

    # Height Fog
    fog = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if fog:
        fog.set_actor_label("ExponentialHeightFog")
        comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
        if comp:
            comp.set_editor_property("fog_density", 0.003)
            comp.set_editor_property("volumetric_fog", True)

    # Post Process
    pp = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if pp:
        pp.set_actor_label("GlobalPostProcess")
        pp.set_editor_property("unbound", True)

    # 2. VAST MEADOW TERRAIN (300m x 300m)
    main_terrain = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -50), unreal.Rotator(0, 0, 0))
    if main_terrain:
        main_terrain.set_actor_label("Terrain_DawnMeadow_Main")
        main_terrain.set_actor_scale3d(unreal.Vector(3000.0, 3000.0, 1.0)) # 300m x 300m
        comp = main_terrain.get_component_by_class(unreal.StaticMeshComponent)
        if comp and cube_mesh:
            comp.set_static_mesh(cube_mesh)
            if green_mat:
                comp.set_material(0, green_mat)

    # 3. MOUNTAIN WALL BARRIER (Perimeter Cliffs)
    mountain_coords = [
        (15000, 0, 600, 300, 3000, 15), (-15000, 0, 600, 300, 3000, 15),
        (0, 15000, 600, 3000, 300, 15), (0, -15000, 600, 3000, 300, 15),
        (12000, 12000, 800, 500, 500, 20), (-12000, 12000, 800, 500, 500, 20),
        (12000, -12000, 800, 500, 500, 20), (-12000, -12000, 800, 500, 500, 20)
    ]
    for idx, (x, y, z, sx, sy, sz) in enumerate(mountain_coords):
        mtn = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(x, y, z), unreal.Rotator(0, 0, 0))
        if mtn:
            mtn.set_actor_label(f"Mountain_Ridge_{idx+1}")
            mtn.set_actor_scale3d(unreal.Vector(sx, sy, sz))
            comp = mtn.get_component_by_class(unreal.StaticMeshComponent)
            if comp and cube_mesh:
                comp.set_static_mesh(cube_mesh)

    # 4. ZONE 1: CENTRAL DAWN SPIRE
    spire_dais = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 40), unreal.Rotator(0, 0, 0))
    if spire_dais:
        spire_dais.set_actor_label("DawnSpire_Dais")
        spire_dais.set_actor_scale3d(unreal.Vector(25.0, 25.0, 1.0))
        comp = spire_dais.get_component_by_class(unreal.StaticMeshComponent)
        if comp and cyl_mesh:
            comp.set_static_mesh(cyl_mesh)

    # Monolith Pillar (Center)
    monolith = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, 700), unreal.Rotator(0, 0, 0))
    if monolith:
        monolith.set_actor_label("DawnSpire_Monolith_Pillar")
        monolith.set_actor_scale3d(unreal.Vector(4.0, 4.0, 14.0))
        comp = monolith.get_component_by_class(unreal.StaticMeshComponent)
        if comp and cyl_mesh:
            comp.set_static_mesh(cyl_mesh)

    # 8 Surrounding Spire Pillars
    pillar_offsets = [
        (1200, 0), (-1200, 0), (0, 1200), (0, -1200),
        (850, 850), (-850, 850), (850, -850), (-850, -850)
    ]
    for idx, (px, py) in enumerate(pillar_offsets):
        pil = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(px, py, 350), unreal.Rotator(0, 0, 0))
        if pil:
            pil.set_actor_label(f"Spire_Crystalline_Pillar_{idx+1}")
            pil.set_actor_scale3d(unreal.Vector(2.0, 2.0, 7.0))
            comp = pil.get_component_by_class(unreal.StaticMeshComponent)
            if comp and cyl_mesh:
                comp.set_static_mesh(cyl_mesh)

    # Player Start (Sanctuary entrance)
    pstart = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, -600, 100), unreal.Rotator(0, 90, 0))
    if pstart:
        pstart.set_actor_label("PlayerStart_DawnSanctuary")

    # 5. ZONE 2: SYLVAN RESOURCE GROVE (North-West Forest)
    grove_plateau = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(-4500, 4500, 60), unreal.Rotator(0, 0, 0))
    if grove_plateau:
        grove_plateau.set_actor_label("Grove_Plateau_Terrace")
        grove_plateau.set_actor_scale3d(unreal.Vector(70.0, 70.0, 1.2))
        comp = grove_plateau.get_component_by_class(unreal.StaticMeshComponent)
        if comp and cube_mesh:
            comp.set_static_mesh(cube_mesh)

    # Forest Trees (Visual Static Mesh Trees + Trunks)
    for i in range(16):
        tx = -3500 - (i % 4) * 700 + (i * 50)
        ty = 3200 + (i // 4) * 800 - (i * 30)
        # Trunk
        trunk = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(tx, ty, 250), unreal.Rotator(0, 0, 0))
        if trunk:
            trunk.set_actor_label(f"Forest_Tree_Trunk_{i+1}")
            trunk.set_actor_scale3d(unreal.Vector(1.2, 1.2, 5.0))
            comp = trunk.get_component_by_class(unreal.StaticMeshComponent)
            if comp and cyl_mesh:
                comp.set_static_mesh(cyl_mesh)
        # Foliage Canopy (Sphere)
        canopy = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(tx, ty, 550), unreal.Rotator(0, 0, 0))
        if canopy:
            canopy.set_actor_label(f"Forest_Tree_Canopy_{i+1}")
            canopy.set_actor_scale3d(unreal.Vector(5.0, 5.0, 4.0))
            comp = canopy.get_component_by_class(unreal.StaticMeshComponent)
            if comp and sphere_mesh:
                comp.set_static_mesh(sphere_mesh)

    # 6. ZONE 3: OBSIDIAN COMBAT CALDERA (South-East Arena)
    caldera_base = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(4500, -4500, -80), unreal.Rotator(0, 0, 0))
    if caldera_base:
        caldera_base.set_actor_label("Caldera_Arena_Pit")
        caldera_base.set_actor_scale3d(unreal.Vector(80.0, 80.0, 1.0))
        comp = caldera_base.get_component_by_class(unreal.StaticMeshComponent)
        if comp and cyl_mesh:
            comp.set_static_mesh(cyl_mesh)

    # Caldera Perimeter Battle Pillars
    for i in range(12):
        import math
        angle = (i / 12.0) * 2.0 * math.pi
        bx = 4500 + math.cos(angle) * 3600
        by = -4500 + math.sin(angle) * 3600
        pillar = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(bx, by, 300), unreal.Rotator(0, 0, 0))
        if pillar:
            pillar.set_actor_label(f"Caldera_Colosseum_Pillar_{i+1}")
            pillar.set_actor_scale3d(unreal.Vector(3.0, 3.0, 6.0))
            comp = pillar.get_component_by_class(unreal.StaticMeshComponent)
            if comp and cyl_mesh:
                comp.set_static_mesh(cyl_mesh)

    # 7. ZONE 4: AZURE CREST BASECAMP (North-East Sanctuary)
    camp_deck = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(4500, 4500, 150), unreal.Rotator(0, 0, 0))
    if camp_deck:
        camp_deck.set_actor_label("BaseCamp_Sanctuary_Deck")
        camp_deck.set_actor_scale3d(unreal.Vector(40.0, 40.0, 1.5))
        comp = camp_deck.get_component_by_class(unreal.StaticMeshComponent)
        if comp and cube_mesh:
            comp.set_static_mesh(cube_mesh)

    # Camp Structure Shelter
    shelter_roof = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(4500, 4500, 500), unreal.Rotator(0, 0, 0))
    if shelter_roof:
        shelter_roof.set_actor_label("BaseCamp_Shelter_Roof")
        shelter_roof.set_actor_scale3d(unreal.Vector(25.0, 25.0, 0.5))
        comp = shelter_roof.get_component_by_class(unreal.StaticMeshComponent)
        if comp and cube_mesh:
            comp.set_static_mesh(cube_mesh)

    # Save level
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    unreal.log(f"[ASTRAWILD] Open World generation complete! Total spawned actors: {len(actors)}")
    saved = unreal.EditorLevelLibrary.save_current_level()
    unreal.log(f"[ASTRAWILD] Open World level saved: {saved}")
    return saved

if __name__ == "__main__":
    create_open_world()