"""Test: try importing DT_Biomes.csv without specifying struct (let UE guess)."""
import unreal
import json

results = {"tests": []}

# Test 1: try creating a DataTable without struct first
try:
    factory = unreal.DataTableFactory()
    task = unreal.AssetImportTask()
    task.set_editor_property("filename", "C:/Users/saisu/OneDrive - kmutnb.ac.th/Documents/game/Content/Astrawild/Data/Source/DT_Biomes.csv")
    task.set_editor_property("destination_path", "/Game/Astrawild/Data/Imported")
    task.set_editor_property("destination_name", "DT_Biomes_AutoDetect")
    task.set_editor_property("factory", factory)
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("automated", True)
    task.set_editor_property("save", True)

    tools = unreal.AssetToolsHelpers.get_asset_tools()
    tools.import_asset_tasks([task])
    results["tests"].append({"name": "auto_detect", "status": "completed"})
except Exception as e:
    results["tests"].append({"name": "auto_detect", "error": str(e)})

# Test 2: check if any DataTable in /Game/Astrawild/Data/Imported was created
try:
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    assets = reg.get_assets_by_path("/Game/Astrawild/Data/Imported", True)
    for a in assets:
        results["tests"].append({"name": "asset_found", "asset": str(a.asset_name), "class": str(a.asset_class_path.asset_name)})
except Exception as e:
    results["tests"].append({"name": "asset_query", "error": str(e)})

# Test 3: try with explicit struct from getattr
try:
    biome_struct = getattr(unreal, "AstrawildBiomeDefinition", None)
    results["tests"].append({"name": "getattr_AstrawildBiomeDefinition", "result": str(type(biome_struct)) if biome_struct else "None"})
except Exception as e:
    results["tests"].append({"name": "getattr", "error": str(e)})

# Test 4: check what Astrawild structs ARE available
try:
    available = [a for a in dir(unreal) if a.startswith("Astrawild") and not a[0].islower()]
    results["tests"].append({"name": "astrawild_attrs_count", "count": len(available)})
    # Show ones that end in Definition/Row/Profile/etc
    structs = [a for a in available if a.endswith(("Definition", "Row", "Profile", "Skill", "Data"))]
    results["tests"].append({"name": "astrawild_structs", "items": structs[:30]})
except Exception as e:
    results["tests"].append({"name": "structs", "error": str(e)})

unreal.log("===== CSV IMPORT TEST =====")
unreal.log(json.dumps(results, indent=2))
unreal.log("===== END =====")
