import unreal

def fix_level_exposure():
    map_pkg = "/Game/Astrawild/Maps/LV_DawnValley_OpenWorld"
    unreal.log(f"[ASTRAWILD] Fixing PostProcess exposure in level: {map_pkg}")
    
    subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    loaded = subsystem.load_level(map_pkg)
    unreal.log(f"[ASTRAWILD] Level loaded: {loaded}")
    
    # Find existing PostProcessVolume
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    pp_vol = None
    for act in actors:
        if isinstance(act, unreal.PostProcessVolume):
            pp_vol = act
            unreal.log(f"[ASTRAWILD] Found PostProcessVolume: {act.get_actor_label()}")
            break
    
    if not pp_vol:
        unreal.log("[ASTRAWILD] No PostProcessVolume found! Creating one...")
        pp_vol = unreal.EditorLevelLibrary.spawn_actor_from_class(
            unreal.PostProcessVolume, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0)
        )
        pp_vol.set_actor_label("PostProcess_Main")
        pp_vol.set_editor_property("unbound", True)
    
    # Apply MANUAL EXPOSURE settings - force a fixed EV so scene is always visible
    try:
        settings = pp_vol.get_editor_property("settings")
        
        # CRITICAL: Override Auto Exposure with Manual mode
        settings.set_editor_property("override_auto_exposure_method", True)
        settings.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
        
        # Set fixed EV100 = 10 (bright outdoor daylight equivalent)
        settings.set_editor_property("override_auto_exposure_bias", True)
        settings.set_editor_property("auto_exposure_bias", 0.0)
        
        # Also set min/max luminance as safety net
        settings.set_editor_property("override_auto_exposure_min_brightness", True)
        settings.set_editor_property("auto_exposure_min_brightness", 1.0)
        settings.set_editor_property("override_auto_exposure_max_brightness", True)
        settings.set_editor_property("auto_exposure_max_brightness", 10.0)
        
        # Set Exposure Compensation for overall brightness
        settings.set_editor_property("override_exposure_compensation_curve", False)
        
        pp_vol.set_editor_property("settings", settings)
        unreal.log("[ASTRAWILD] Manual exposure settings applied successfully!")
    except Exception as e:
        unreal.log_warning(f"[ASTRAWILD] Could not set all exposure props: {e}")
        # Fallback: try direct property approach
        try:
            pp_vol.set_editor_property("unbound", True)
            unreal.log("[ASTRAWILD] PostProcess unbound confirmed")
        except Exception as e2:
            unreal.log_warning(f"[ASTRAWILD] Fallback also failed: {e2}")
    
    # Verify DirectionalLight is set correctly
    for act in actors:
        if isinstance(act, unreal.DirectionalLight):
            comp = act.get_component_by_class(unreal.DirectionalLightComponent)
            if comp:
                try:
                    comp.set_editor_property("intensity", 3.14159)  # 1 π = correct for non-physical sun
                    comp.set_editor_property("atmosphere_sun_light", True)
                    comp.set_editor_property("forward_shading_priority", 1)
                    unreal.log(f"[ASTRAWILD] DirectionalLight '{act.get_actor_label()}' corrected intensity to π lux (non-physical mode)")
                except Exception as e:
                    unreal.log_warning(f"[ASTRAWILD] Could not set DirectionalLight props: {e}")
            break
    
    # Save the level
    saved = subsystem.save_current_level()
    unreal.log(f"[ASTRAWILD] Level save result: {saved}")
    saved_dirty = unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"[ASTRAWILD] save_dirty_packages result: {saved_dirty}")

if __name__ == "__main__":
    fix_level_exposure()
