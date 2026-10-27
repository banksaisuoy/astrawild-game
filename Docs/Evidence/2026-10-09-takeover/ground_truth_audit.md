# Ground-Truth Audit (Level A: real reads) — generated in-session

HEAD: `d326eee81e95447432436a4333a79800be673af0`

Level A = read files directly this session (grep -rn real hits).
Engine runtime = NOT verified here (no Unreal on this sandbox).

| System | Evidence rows | Weakest link |
|---|---|---|
| startup/boot | 3 rows, 3 hits | - |
| GameMode/flow | 3 rows, 54 hits | - |
| pawn/movement | 3 rows, 11 hits | - |
| input | 5 rows, 85 hits | - |
| camera | 2 rows, 7 hits | - |
| interaction | 2 rows, 40 hits | - |
| inventory | 3 rows, 123 hits | - |
| crafting | 3 rows, 28 hits | - |
| creature spawning | 3 rows, 6 hits | - |
| capture/roster | 2 rows, 60 hits | - |
| quests | 2 rows, 5 hits | - |
| building | 3 rows, 14 hits | - |
| save/load | 3 rows, 12 hits | - |
| UI | 3 rows, 41 hits | - |
| audio | 3 rows, 20 hits | - |
| world generation | 3 rows, 37 hits | - |
| packaging | 6 rows, 6 hits | - |

Automation tests (read from AutomationTests.cpp): 134 macro defines, 134 unique ASTRAWILD.* names, families: Ability, Armor, ArtPack, Asset, Atmosphere, Attributes, Bestiary, BiomeDressing, Capture, Combat, Craft, DCP1, DCP2, DCP3, DCP4, DCP5, DCP6, DCP7, DP3, DP4, DP5, DP6, DP7, DP8, DP9, Dialogue, Dungeon, Echo, Economy, Equipment, FCR, FPP1, Input, Inventory, LCP2, LCP3, LCP4, LCP5, LCP6, Locomotion, NPC, PCR1, PCR2, PCR3, PCR4, PCR5, POI, Power, Quest, Research, ResourceNode, Robot, SCI_FANTASY, SCP, Save, Skiff, Survival, Tech, Terrain, VIS1, Vfx, Weapon, WorkSite, WorldEvent, Zones

Config wiring: {"GameDefaultMap": ["/Game/ASTRAWILD/Maps/MainMap"], "EditorStartupMap": ["/Game/ASTRAWILD/Maps/MainMap"], "GlobalDefaultGameMode": ["/Script/AstrawildCore.AstrawildGameMode"], "GameInstanceClass": [], "EngineAssociation": "5.8", "Modules": ["AstrawildCore"], "Plugins": ["EnhancedInput", "ProceduralMeshComponent", "PythonScriptPlugin"], "DefaultGame_ini_gamemode_refs": 0}

Content packages (magic-walked): 414 .uasset + 2 .umap; umaps: ['Content/ASTRAWILD/Maps/MainMap.umap', 'Content/ThirdPerson/Lvl_ThirdPerson.umap']
