import unreal

def fix_exposure_clean():
    map_pkg = "/Game/Astrawild/Maps/LV_DawnValley_OpenWorld"
    unreal.log("[ASTRAWILD] Fix Manual Exposure - Clean Version")
    
    subsystem = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    subsystem.load_level(map_pkg)
    
    actors = unreal.EditorLevelLibrary.get_all_level_actors()
    
    for act in actors:
        if isinstance(act, unreal.PostProcessVolume):
            act.set_editor_property("unbound", True)
            settings = act.get_editor_property("settings")
            
            # Override auto exposure → manual mode
            settings.set_editor_property("override_auto_exposure_method", True)
            settings.set_editor_property("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
            
            settings.set_editor_property("override_auto_exposure_bias", True)
            settings.set_editor_property("auto_exposure_bias", 0.0)  # EV=0 normal exposure
            
            settings.set_editor_property("override_auto_exposure_min_brightness", True)
            settings.set_editor_property("auto_exposure_min_brightness", 1.0)
            
            settings.set_editor_property("override_auto_exposure_max_brightness", True)
            settings.set_editor_property("auto_exposure_max_brightness", 10.0)
            
            # Write back settings struct to the volume
            act.set_editor_property("settings", settings)
            unreal.log(f"[ASTRAWILD] Manual Exposure applied to: {act.get_actor_label()}")
            break
    
    # Save
    saved = subsystem.save_current_level()
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
    unreal.log(f"[ASTRAWILD] Level saved: {saved}")

if __name__ == "__main__":
    fix_exposure_clean()
