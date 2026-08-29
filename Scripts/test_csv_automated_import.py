import unreal

factory = unreal.CSVImportFactory()
settings = factory.get_editor_property("automated_import_settings")
unreal.log(f"[SETTINGS_OBJECT]: {settings}")
unreal.log(f"[SETTINGS_PROPS]: {dir(settings)}")

row_struct = unreal.load_object(None, "/Script/AstrawildCore.AstrawildBiomeDefinition")
settings.set_editor_property("import_row_struct", row_struct)
settings.set_editor_property("import_type", unreal.CSVImportType.ECSV_DATA_TABLE)

# Test importing DT_Biomes.csv
task = unreal.AssetImportTask()
task.set_editor_property("filename", "c:/Users/saisu/OneDrive - kmutnb.ac.th/Documents/game/Content/Astrawild/Data/Source/DT_Biomes.csv")
task.set_editor_property("destination_path", "/Game/Astrawild/Data/Imported")
task.set_editor_property("destination_name", "DT_Biomes")
task.set_editor_property("factory", factory)
task.set_editor_property("automated", True)
task.set_editor_property("replace_existing", True)
task.set_editor_property("save", True)

tools = unreal.AssetToolsHelpers.get_asset_tools()
tools.import_asset_tasks([task])

exists = unreal.EditorAssetLibrary.does_asset_exist("/Game/Astrawild/Data/Imported/DT_Biomes")
unreal.log(f"[FINAL_IMPORT_RESULT]: DT_Biomes exists = {exists}")
