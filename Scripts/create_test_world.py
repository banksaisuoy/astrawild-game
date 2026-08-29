"""Create and save the prototype test map LV_DawnValley_Test with lights, sky, and terrain.

Run via UnrealEditor-Cmd:
    UnrealEditor-Cmd.exe "Astrawild.uproject" -ExecutePythonScript="Scripts/create_test_world.py" -NullRHI -NoLoadingScreen
"""
import unreal

def create_dawn_valley_map():
    map_package = "/Game/Astrawild/Maps/Prototype/LV_DawnValley_Test"
    unreal.log(f"[ASTRAWILD] Generating test world map: {map_package}")

    # Ensure directories exist
    unreal.EditorAssetLibrary.make_directory("/Game/Astrawild/Maps/Prototype")

    # Create new blank world/level
    world = unreal.EditorLevelLibrary.new_level(map_package)
    if not world:
        unreal.log_error(f"[ASTRAWILD] Failed to create new level at {map_package}")
        return False

    # 1. Spawn Sun (Directional Light - Movable)
    sun_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 500), unreal.Rotator(-50, -40, 0))
    if sun_actor:
        sun_actor.set_actor_label("Sun_DirectionalLight")
        sun_comp = sun_actor.get_component_by_class(unreal.DirectionalLightComponent)
        if sun_comp:
            try:
                sun_comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
            except Exception:
                pass
            sun_comp.set_editor_property("intensity", 75.0)
            sun_comp.set_editor_property("atmosphere_sun_light", True)
            sun_comp.set_editor_property("cast_shadows", True)
            sun_comp.set_editor_property("dynamic_shadow_distance_movable_light", 30000.0)

    # 2. Spawn Sky Atmosphere
    sky_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if sky_actor:
        sky_actor.set_actor_label("SkyAtmosphere")

    # 3. Spawn Sky Light (Movable)
    skylight_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 200), unreal.Rotator(0, 0, 0))
    if skylight_actor:
        skylight_actor.set_actor_label("SkyLight")
        skylight_comp = skylight_actor.get_component_by_class(unreal.SkyLightComponent)
        if skylight_comp:
            try:
                skylight_comp.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
            except Exception:
                pass
            skylight_comp.set_editor_property("real_time_capture", True)
            skylight_comp.set_editor_property("intensity", 4.0)

    # 4. Spawn Exponential Height Fog (Volumetric Fog)
    fog_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if fog_actor:
        fog_actor.set_actor_label("ExponentialHeightFog")
        fog_comp = fog_actor.get_component_by_class(unreal.ExponentialHeightFogComponent)
        if fog_comp:
            fog_comp.set_editor_property("fog_density", 0.005)
            fog_comp.set_editor_property("volumetric_fog", True)

    # 5. Spawn Post Process Volume (Unbound & Fixed Brightness)
    pp_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
    if pp_actor:
        pp_actor.set_actor_label("GlobalPostProcess")
        pp_actor.set_editor_property("unbound", True)
        settings = pp_actor.get_editor_property("settings")
        if settings:
            try:
                settings.set_editor_property("auto_exposure_bias", 2.0)
            except Exception:
                pass

    # 6. Spawn Large Floor (StaticMeshActor)
    floor_mesh = unreal.load_asset("/Engine/BasicShapes/Cube.Cube")
    floor_actor = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(0, 0, -50), unreal.Rotator(0, 0, 0))
    if floor_actor:
        floor_actor.set_actor_label("Ground_DawnMeadow")
        floor_actor.set_actor_scale3d(unreal.Vector(500.0, 500.0, 1.0)) # 500m x 500m
        sm_comp = floor_actor.get_component_by_class(unreal.StaticMeshComponent)
        if sm_comp and floor_mesh:
            sm_comp.set_static_mesh(floor_mesh)

    # 7. Spawn Player Start
    player_start = unreal.EditorLevelLibrary.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 150), unreal.Rotator(0, 0, 0))
    if player_start:
        player_start.set_actor_label("PlayerStart_Main")

    # 8. Spawn Decorative Pillars & Arena Markers
    pillar_mesh = unreal.load_asset("/Engine/BasicShapes/Cylinder.Cylinder")
    sphere_mesh = unreal.load_asset("/Engine/BasicShapes/Sphere.Sphere")
    
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

    # Save the level
    success = unreal.EditorLevelLibrary.save_current_level()
    unreal.log(f"[ASTRAWILD] Map save result: {success}")
    return success

if __name__ == "__main__":
    create_dawn_valley_map()