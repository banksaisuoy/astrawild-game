"""Debug: list all UScriptStruct in AstrawildCore module."""
import unreal
import json

results = {
    "module": "AstrawildCore",
    "strategies": {},
}

# Strategy 1: load_object
try:
    p = "/Script/AstrawildCore.FAstrawildBiomeDefinition"
    s = unreal.load_object(None, p)
    results["strategies"]["load_object"] = {"path": p, "result": str(type(s)) if s else "None"}
except Exception as e:
    results["strategies"]["load_object"] = {"error": str(e)}

# Strategy 2: find_object
try:
    s = unreal.find_object(None, "AstrawildBiomeDefinition")
    results["strategies"]["find_object"] = {"result": str(type(s)) if s else "None"}
except Exception as e:
    results["strategies"]["find_object"] = {"error": str(e)}

# Strategy 3: SystemLibrary
try:
    s = unreal.SystemLibrary.find_object_by_path("/Script/AstrawildCore.FAstrawildBiomeDefinition")
    results["strategies"]["SystemLibrary"] = {"result": str(type(s)) if s else "None"}
except Exception as e:
    results["strategies"]["SystemLibrary"] = {"error": str(e)}

# Strategy 4: walk loaded packages
try:
    reg = unreal.AssetRegistryHelpers.get_asset_registry()
    structs = reg.get_assets_by_class("ScriptStruct", True)
    astrawild_structs = [str(a.asset_name) for a in structs if "Astrawild" in str(a.package_name)]
    results["strategies"]["AssetRegistry"] = {"count": len(astrawild_structs), "first_5": astrawild_structs[:5]}
except Exception as e:
    results["strategies"]["AssetRegistry"] = {"error": str(e)}

# Strategy 5: try EditorAssetLibrary
try:
    s = unreal.EditorAssetLibrary.load_asset("/Script/AstrawildCore.FAstrawildBiomeDefinition")
    results["strategies"]["EditorAssetLibrary"] = {"result": str(type(s)) if s else "None"}
except Exception as e:
    results["strategies"]["EditorAssetLibrary"] = {"error": str(e)}

unreal.log("===== DEBUG RESULTS =====")
unreal.log(json.dumps(results, indent=2))
unreal.log("===== END DEBUG =====")
