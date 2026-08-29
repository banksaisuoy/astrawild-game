"""Import all OBJ 3D models into Unreal Engine as real .uasset StaticMeshes.
Imports:
- Characters: SM_Alpha_Solarix, SM_Player_AstralSurveyor
- Echoes: Pyrelite, Thornback, Aquavine, and all 100+ monsters
- MapKit: DawnSpire, ResourceGrove, DangerPit, RestSanctuary
- Props: Campfire, SunwoodLog, LumenRock, AstraCrystal, AstraResonator
"""
import os
import unreal

def import_all_models():
    unreal.log("[ASTRAWILD] Starting batch OBJ import into Unreal Engine .uasset format...")
    
    project_content = "C:/Users/saisu/OneDrive - kmutnb.ac.th/Documents/game/Content"
    mesh_dir = os.path.join(project_content, "Astrawild/Meshes")

    tasks = []
    for root, dirs, files in os.walk(mesh_dir):
        for f in files:
            if f.endswith(".obj"):
                full_path = os.path.join(root, f).replace("\\", "/")
                rel_dir = os.path.relpath(root, project_content).replace("\\", "/")
                dest_path = f"/Game/{rel_dir}"
                asset_name = os.path.splitext(f)[0]
                if asset_name.endswith("_Source"):
                    asset_name = asset_name[:-7] # e.g. SM_Echo_Pyrelite_Source -> SM_Echo_Pyrelite
                
                task = unreal.AssetImportTask()
                task.set_editor_property("filename", full_path)
                task.set_editor_property("destination_path", dest_path)
                task.set_editor_property("destination_name", asset_name)
                task.set_editor_property("replace_existing", True)
                task.set_editor_property("automated", True)
                task.set_editor_property("save", True)
                
                # Import options
                options = unreal.FbxImportUI()
                options.set_editor_property("import_mesh", True)
                options.set_editor_property("import_textures", False)
                options.set_editor_property("import_materials", True)
                options.set_editor_property("import_as_skeletal", False)
                task.set_editor_property("options", options)
                
                tasks.append(task)

    unreal.log(f"[ASTRAWILD] Found {len(tasks)} OBJ models to import as .uasset!")
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    unreal.log("[ASTRAWILD] Batch OBJ import completed successfully!")

if __name__ == "__main__":
    import_all_models()
