"""Build and save a self-contained, standalone 3D level LV_DawnValley_Main without World Partition issues.

Run via UnrealEditor-Cmd:
    UnrealEditor-Cmd.exe "Astrawild.uproject" -ExecutePythonScript="Scripts/build_dawn_valley_map.py" -NullRHI -NoLoadingScreen
"""
import unreal

def build_dawn_valley_map():
    map_path = "/Game/Astrawild/Maps/LV_DawnValley_Main"
    unreal.log(f"[ASTRAWILD] Starting level build at: {map_path}")

    # Ensure directory
    unreal.EditorAssetLibrary.make_directory("/Game/Astrawild/Maps")

    # Create new level
    world = unreal.EditorLevelLibrary.new_level(map_path)
    if not world:
        unreal.log_error(f"[ASTRAWILD] Failed to create level at {map_path}")
        return False

    # 1. Sun (Directional Light - Movable)
    sun = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 800), unreal.Rotator(-45, -45, 0))
    if sun:
        sun.set_actor_label("Sun_DirectionalLight")
        comp = sun.get_component_by_class(unreal.DirectionalLightComponent)
        if comp:
            comp.set_editor_property("intensity", 75000.0) # Physical Lux
            comp.set_editor_property("atmosphere_sun_light", True)
            comp.set_editor_property("cast_shadows", True)
            try:
                comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
            except Exception:
                pass

    # 2. Sky Atmosphere
    sky = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if sky:
        sky.set_actor_label("SkyAtmosphere")

    # 3. Sky Light (Movable)
    skylight = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 400), unreal.Rotator(0, 0, 0))
    if skylight:
        skylight.set_actor_label("SkyLight")
        comp = skylight.get_component_by_class(unreal.SkyLightComponent)
        if comp:
            comp.set_editor_property("real_time_capture", True)
            comp.set_editor_property("intensity", 3.0)
            try:
                comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
            except Exception:
                pass

    # 4. Exponential Height Fog
    fog = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if fog:
        fog.set_actor_label("ExponentialHeightFog")
        comp = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
        if comp:
            comp.set_editor_property("fog_density", 0.005)
            comp.set_editor_property("volumetric_fog", True)

    # 5. Post Process Volume (Unbound with manual exposure baseline)
    pp = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if pp:
        pp.set_actor_label("GlobalPostProcess")
        pp.set_editor_property("unbound", True)

    # 6. Main Ground Floor
    floor_mesh = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    floor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -50), unreal.Rotator(0, 0, 0))
    if floor:
        floor.set_actor_label("Ground_DawnMeadow")
        floor.set_actor_scale3d(unreal.Vector(500.0, 500.0, 1.0))
        comp = floor.get_component_by_class(unreal.StaticMeshComponent)
        if comp and floor_mesh:
            comp.set_static_mesh(floor_mesh)

    # 7. Player Start
    player_start = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 120), unreal.Rotator(0, 0, 0))
    if player_start:
        player_start.set_actor_label("PlayerStart_Main")

    # 8. Decorative Pillars
    pillar_mesh = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
    pillar_coords = [
        (800, 800, 150), (-800, 800, 150), (800, -800, 150), (-800, -800, 150),
        (1500, 0, 150), (-1500, 0, 150), (0, 1500, 150), (0, -1500, 150)
    ]
    if pillar_mesh:
        for i, pos in enumerate(pillar_coords):
            pillar = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*pos), unreal.Rotator(0, 0, 0))
            if pillar:
                pillar.set_actor_label(f"Spire_Pillar_{i+1}")
                pillar.set_actor_scale3d(unreal.Vector(3.0, 3.0, 8.0))
                comp = pillar.get_component_by_class(unreal.StaticMeshComponent)
                if comp:
                    comp.set_static_mesh(pillar_mesh)

    # Save level package
    all_actors = unreal.EditorLevelLibrary.get_all_level_actors()
    unreal.log(f"[ASTRAWILD] Total actors spawned in level: {len(all_actors)}")

    save_result = unreal.EditorLevelLibrary.save_current_level()
    unreal.log(f"[ASTRAWILD] Level save result: {save_result}")
    return save_result

if __name__ == "__main__":
    build_dawn_valley_map()