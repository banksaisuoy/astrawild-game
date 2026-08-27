# ASTRAWILD Blueprints Directory

All Blueprint classes in this directory MUST inherit directly from C++ base classes in \AstrawildCore\:
- \Characters/\: \BP_AstrawildCharacter\ (inherits from \AAstrawildCharacter\)
- \Echoes/\: \BP_Echo_Pyrelite\, \BP_Echo_Aquavine\, \BP_Echo_Thornback\ (inherit from \AAstrawildEchoBase\)
- \Environment/\: \BP_HarvestNode_Tree\, \BP_HarvestNode_Ore\ (inherit from \AAstrawildHarvestableNode\)
- \Buildings/\: \BP_Building_Campfire\, \BP_Building_Bed\, \BP_Building_Bench\, \BP_Building_Chest\ (inherit from \AAstrawildBuildingPiece\)