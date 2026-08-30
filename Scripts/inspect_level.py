import unreal

map_pkg = "/Game/Astrawild/Maps/LV_DawnValley_OpenWorld"
subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
subsystem.load_level(map_pkg)

actors = unreal.EditorLevelLibrary.get_all_level_actors()
unreal.log(f"=== LEVEL ACTORS REPORT ({len(actors)} total) ===")
for act in actors:
    cls_name = act.get_class().get_name()
    loc = act.get_actor_location()
    lbl = act.get_actor_label()
    unreal.log(f"ACTOR: {lbl} [{cls_name}] at ({loc.x:.1f}, {loc.y:.1f}, {loc.z:.1f})")
    if isinstance(act, unreal.DirectionalLight):
        comp = act.get_component_by_class(unreal.DirectionalLightComponent)
        if comp:
            unreal.log(f"   -> Sun Intensity: {comp.get_editor_property('intensity')}, SunAtmosphere: {comp.get_editor_property('atmosphere_sun_light')}")
    elif isinstance(act, unreal.SkyLight):
        comp = act.get_component_by_class(unreal.SkyLightComponent)
        if comp:
            unreal.log(f"   -> SkyLight Intensity: {comp.get_editor_property('intensity')}, RealtimeCapture: {comp.get_editor_property('real_time_capture')}")
    elif isinstance(act, unreal.PostProcessVolume):
        unreal.log(f"   -> PostProcess unbound: {act.get_editor_property('unbound')}")
