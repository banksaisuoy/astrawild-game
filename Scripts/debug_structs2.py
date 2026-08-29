"""Debug: list all UScriptStruct in AstrawildCore module - v2"""
import unreal
import json

results = {
    "module": "AstrawildCore",
    "strategies": {},
}

# Strategy A: try find_class (treats as UClass)
try:
    s = unreal.find_class("BiomeDefinition")
    results["strategies"]["find_class_Biome"] = {"result": str(type(s)) if s else "None"}
except Exception as e:
    results["strategies"]["find_class_Biome"] = {"error": str(e)}

# Strategy B: load class
try:
    s = unreal.load_class(None, "/Script/AstrawildCore.FAstrawildBiomeDefinition")
    results["strategies"]["load_class"] = {"result": str(type(s)) if s else "None"}
except Exception as e:
    results["strategies"]["load_class"] = {"error": str(e)}

# Strategy C: walk all loaded UClasses/UEnum/UStruct
try:
    all_structs = []
    for i in range(unreal.SystemLibrary.get_num_objects() if hasattr(unreal.SystemLibrary, 'get_num_objects') else 0):
        pass
    results["strategies"]["walk_objects"] = {"info": "no method available"}
except Exception as e:
    results["strategies"]["walk_objects"] = {"error": str(e)}

# Strategy D: use PythonReflection to get module classes
try:
    import unreal
    # Check if any class starts with Astrawild
    found_classes = []
    for attr in dir(unreal):
        if "Astrawild" in attr or "Biome" in attr or "QuestRow" in attr:
            found_classes.append(attr)
    results["strategies"]["dir_unreal"] = {"count": len(found_classes), "first_20": found_classes[:20]}
except Exception as e:
    results["strategies"]["dir_unreal"] = {"error": str(e)}

# Strategy E: check if QuestRow struct can be found
try:
    s = unreal.load_object(None, "/Script/AstrawildCore.FAstrawildQuestRow")
    results["strategies"]["load_QuestRow"] = {"result": str(type(s)) if s else "None"}
except Exception as e:
    results["strategies"]["load_QuestRow"] = {"error": str(e)}

# Strategy F: check if AstrawildCharacter class works (it's a UClass)
try:
    s = unreal.load_class(None, "/Script/AstrawildCore.AstrawildCharacter")
    results["strategies"]["load_AstrawildCharacter"] = {"result": str(type(s)) if s else "None"}
except Exception as e:
    results["strategies"]["load_AstrawildCharacter"] = {"error": str(e)}

# Strategy G: check if AstrawildEchoDataAsset class works (UDataAsset)
try:
    s = unreal.load_class(None, "/Script/AstrawildCore.AstrawildEchoDataAsset")
    results["strategies"]["load_AstrawildEchoDataAsset"] = {"result": str(type(s)) if s else "None"}
except Exception as e:
    results["strategies"]["load_AstrawildEchoDataAsset"] = {"error": str(e)}

unreal.log("===== DEBUG v2 RESULTS =====")
unreal.log(json.dumps(results, indent=2))
unreal.log("===== END DEBUG v2 =====")
