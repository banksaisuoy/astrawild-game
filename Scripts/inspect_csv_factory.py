import unreal

factory = unreal.CSVImportFactory()
unreal.log(f"[CSV_FACTORY]: {dir(factory)}")

try:
    settings = factory.get_editor_property("import_settings")
    unreal.log(f"[SETTINGS]: {dir(settings)}")
    unreal.log(f"[SETTINGS_PROPS]: row_struct={settings.get_editor_property('import_row_struct')}")
except Exception as e:
    unreal.log(f"[SETTINGS_ERR]: {e}")
