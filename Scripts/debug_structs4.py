"""Test: how to convert UScriptStruct class to ScriptStruct instance for DataTableFactory."""
import unreal
import json

results = {"tests": []}

# Test 1: get the class
try:
    cls = getattr(unreal, "AstrawildBiomeDefinition", None)
    results["tests"].append({"name": "get_class", "type": str(type(cls))})
except Exception as e:
    results["tests"].append({"name": "get_class", "error": str(e)})

# Test 2: try constructor
try:
    instance = cls()
    results["tests"].append({"name": "construct", "type": str(type(instance))})
except Exception as e:
    results["tests"].append({"name": "construct", "error": str(e)})

# Test 3: try factory.set_editor_property with the class
try:
    factory = unreal.DataTableFactory()
    factory.set_editor_property("struct", cls)
    results["tests"].append({"name": "set_class_directly", "result": "OK"})
except Exception as e:
    results["tests"].append({"name": "set_class_directly", "error": str(e)})

# Test 4: try factory.set_editor_property with constructed instance
try:
    factory2 = unreal.DataTableFactory()
    instance = cls()
    factory2.set_editor_property("struct", instance)
    results["tests"].append({"name": "set_instance", "result": "OK"})
except Exception as e:
    results["tests"].append({"name": "set_instance", "error": str(e)})

# Test 5: try setting via setattr
try:
    factory3 = unreal.DataTableFactory()
    instance = cls()
    setattr(factory3, "struct", instance)
    results["tests"].append({"name": "setattr_instance", "result": "OK"})
except Exception as e:
    results["tests"].append({"name": "setattr_instance", "error": str(e)})

# Test 6: try via unreal.ScriptStruct.cast
try:
    factory4 = unreal.DataTableFactory()
    cast = unreal.ScriptStruct.cast(cls) if hasattr(unreal, "ScriptStruct") else None
    results["tests"].append({"name": "cast", "result": str(type(cast)) if cast else "None"})
except Exception as e:
    results["tests"].append({"name": "cast", "error": str(e)})

# Test 7: check struct_class.static_struct
try:
    if hasattr(cls, "static_struct"):
        ss = cls.static_struct()
        results["tests"].append({"name": "static_struct", "type": str(type(ss))})
    else:
        results["tests"].append({"name": "static_struct", "info": "no method"})
except Exception as e:
    results["tests"].append({"name": "static_struct", "error": str(e)})

unreal.log("===== CONVERSION TEST =====")
unreal.log(json.dumps(results, indent=2))
unreal.log("===== END =====")
