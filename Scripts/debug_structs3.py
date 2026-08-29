"""Test: try loading struct from AstrawildCore module."""
import unreal
import json

results = {"tests": []}

# Test 1: just check what's available
try:
    available = [a for a in dir(unreal) if a.startswith("Astrawild")]
    results["tests"].append({"name": "all_astrawild_attrs", "count": len(available)})
    structs = [a for a in available if a.endswith(("Definition", "Row", "Profile", "Data"))]
    results["tests"].append({"name": "astrawild_struct_candidates", "items": structs[:30]})
except Exception as e:
    results["tests"].append({"name": "dir", "error": str(e)})

# Test 2: try getattr for each candidate
candidates = ["AstrawildBiomeDefinition", "AstrawildQuestRow", "AstrawildRecipeRow", "AstrawildEchoDexRow", "AstrawildCraftingRecipeRow"]
for name in candidates:
    try:
        v = getattr(unreal, name, None)
        results["tests"].append({"name": f"getattr_{name}", "result": str(type(v)) if v else "None"})
    except Exception as e:
        results["tests"].append({"name": f"getattr_{name}", "error": str(e)})

# Test 3: try alternative paths
paths = [
    "/Script/AstrawildCore.AstrawildBiomeDefinition",
    "AstrawildCore.AstrawildBiomeDefinition",
    "/Script/AstrawildCore.FAstrawildBiomeDefinition",
]
for p in paths:
    try:
        v = unreal.load_object(None, p)
        results["tests"].append({"name": f"load_{p}", "result": str(type(v)) if v else "None"})
    except Exception as e:
        results["tests"].append({"name": f"load_{p}", "error": str(e)})

unreal.log("===== STRUCT LOADING TEST =====")
unreal.log(json.dumps(results, indent=2))
unreal.log("===== END =====")
