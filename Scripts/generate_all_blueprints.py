import unreal

def create_blueprints():
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.BlueprintFactory()
    
    # List of Blueprints to generate from C++ Parent Classes
    blueprints = [
        # Characters
        ("/Game/Astrawild/Blueprints/Characters", "BP_Player", unreal.load_object(None, "/Script/AstrawildCore.AstrawildCharacter")),
        # Echoes
        ("/Game/Astrawild/Blueprints/Echoes", "BP_Echo_Pyrelite", unreal.load_object(None, "/Script/AstrawildCore.AstrawildEchoBase")),
        ("/Game/Astrawild/Blueprints/Echoes", "BP_Echo_Thornback", unreal.load_object(None, "/Script/AstrawildCore.AstrawildEchoBase")),
        ("/Game/Astrawild/Blueprints/Echoes", "BP_Echo_Aquavine", unreal.load_object(None, "/Script/AstrawildCore.AstrawildEchoBase")),
        # Bosses
        ("/Game/Astrawild/Blueprints/Bosses", "BP_Alpha_Solarix", unreal.load_object(None, "/Script/AstrawildCore.AstrawildAlphaEcho")),
        # Environment & Harvestables
        ("/Game/Astrawild/Blueprints/Environment", "BP_Harvestable_Tree", unreal.load_object(None, "/Script/AstrawildCore.AstrawildHarvestableNode")),
        ("/Game/Astrawild/Blueprints/Environment", "BP_Harvestable_Ore", unreal.load_object(None, "/Script/AstrawildCore.AstrawildHarvestableNode")),
        # Framework & UI
        ("/Game/Astrawild/Blueprints/Core", "BP_AstrawildGameMode", unreal.load_object(None, "/Script/AstrawildCore.AstrawildGameMode")),
        ("/Game/Astrawild/Blueprints/UI", "BP_AstrawildHUD", unreal.load_object(None, "/Script/AstrawildCore.AstrawildHUD")),
    ]
    
    created_count = 0
    for pkg_path, name, parent_class in blueprints:
        if not parent_class:
            unreal.log_error(f"[BP_GEN] Parent class for {name} is None!")
            continue
            
        unreal.EditorAssetLibrary.make_directory(pkg_path)
        factory.set_editor_property("parent_class", parent_class)
        
        bp_asset = tools.create_asset(
            asset_name=name,
            package_path=pkg_path,
            asset_class=unreal.Blueprint,
            factory=factory
        )
        
        if bp_asset:
            unreal.EditorAssetLibrary.save_loaded_asset(bp_asset)
            unreal.log(f"[BP_GEN] Successfully created Blueprint: {pkg_path}/{name}.uasset (Parent: {parent_class.get_name()})")
            created_count += 1
        else:
            unreal.log_warning(f"[BP_GEN] Failed to create or already exists: {pkg_path}/{name}")

    unreal.log(f"[BP_GEN] Total Blueprints Created: {created_count}/{len(blueprints)}")

if __name__ == "__main__":
    create_blueprints()
