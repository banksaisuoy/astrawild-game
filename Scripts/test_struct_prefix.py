import unreal
import json

test_names = [
    ("FAstrawildBiomeDefinition", "AstrawildBiomeDefinition"),
    ("FAstrawildBossAttackRow", "AstrawildBossAttackRow"),
    ("FAstrawildEchoDexRow", "AstrawildEchoDexRow"),
    ("FAstrawildFishRow", "AstrawildFishRow"),
    ("FAstrawildCraftingRecipeRow", "AstrawildCraftingRecipeRow")
]

results = []
for f_name, bare_name in test_names:
    has_f_attr = getattr(unreal, f_name, None)
    has_bare_attr = getattr(unreal, bare_name, None)
    
    obj_f = unreal.load_object(None, f"/Script/AstrawildCore.{f_name}")
    obj_bare = unreal.load_object(None, f"/Script/AstrawildCore.{bare_name}")
    
    results.append({
        "original": f_name,
        "bare": bare_name,
        "getattr(f_name)": str(has_f_attr),
        "getattr(bare_name)": str(has_bare_attr),
        "load_object(f_name)": str(obj_f),
        "load_object(bare_name)": str(obj_bare),
    })

print(json.dumps(results, indent=2))
