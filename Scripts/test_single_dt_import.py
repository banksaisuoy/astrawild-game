import unreal
from pathlib import Path

def test_import_dt():
    csv_path = "c:/Users/saisu/OneDrive - kmutnb.ac.th/Documents/game/Content/Astrawild/Data/Source/DT_Biomes.csv"
    dest_path = "/Game/Astrawild/Data/Imported"
    dest_name = "DT_Biomes"
    
    # Load row struct
    row_struct = unreal.load_object(None, "/Script/AstrawildCore.AstrawildBiomeDefinition")
    unreal.log(f"[TEST] Row Struct: {row_struct}")
    
    # Create factory
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", row_struct)
    
    # Create task
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", csv_path)
    task.set_editor_property("destination_path", dest_path)
    task.set_editor_property("destination_name", dest_name)
    task.set_editor_property("factory", factory)
    task.set_editor_property("automated", True)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("save", True)
    
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    tools.import_asset_tasks([task])
    
    # Check if created
    asset_path = f"{dest_path}/{dest_name}"
    exists = unreal.EditorAssetLibrary.does_asset_exist(asset_path)
    unreal.log(f"[TEST] Result: {asset_path} exists = {exists}")

if __name__ == "__main__":
    test_import_dt()
