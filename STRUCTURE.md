# ASTRAWILD Unreal Structure

## Repository layers

```text
ASTRAWILD.uproject
├── Config/                         Unreal defaults and target settings
├── Source/AstrawildCore/
│   ├── Public/                     Stable public C++ contracts
│   └── Private/                    Runtime implementations
├── Content/ASTRAWILD/              Unreal binary assets created in Editor
├── Docs/                           Design, QA, assets, licenses and handoff
├── Scripts/                        Repository-only validation scripts
├── PLAN.md                         Production milestones and risk gates
├── STRUCTURE.md                    This architecture map
├── ASSETS.md                       Visual/asset manifest
└── MEMORY.md                       Decisions and current verified facts
```

## Runtime ownership

| Responsibility | C++ owner | Blueprint/Content responsibility |
|---|---|---|
| Player movement/camera/input contract | `AAstrawildPlayerCharacter` | Assign mesh, Input Actions, Mapping Context, animation and tuning |
| Inventory | `UAstrawildInventoryComponent` | UI widgets, item icons, content references |
| Crafting | `UAstrawildCraftingComponent` | Recipe Data Assets and crafting UI |
| Capture | `UAstrawildCaptureComponent`, `AAstrawildEchoCharacter` | capture input, target feedback, VFX/audio and roster UI |
| Echo data | `UAstrawildEchoDefinition` | three Echo Data Assets, mesh, animation and abilities |
| Resource harvest | `AAstrawildResourceNode` | placement, material, item ID and respawn tuning |
| Rest point | `AAstrawildRestPoint` | placement, visuals, VFX and UI feedback |
| Damage test | `AAstrawildDamageTarget` | placement and combat HUD |
| Save/load | `UAstrawildSaveGame`, `UAstrawildSaveSubsystem` | buttons/autosave calls and restore orchestration |
| World composition | `AAstrawildGameMode` and Map | `L_Prototype`, Player Start, lighting, floor, navigation and actors |

## Data flow

```text
Input Action
  -> Player Character
  -> Interaction Interface / Capture / Crafting
  -> Inventory and Echo state
  -> SaveSubsystem snapshot
  -> SaveGame slot
```

Stable IDs are persisted in SaveGame; asset object paths are not a substitute for stable IDs. Blueprint should compose content and presentation while C++ owns validation and transaction rules.

## Content paths expected from Antigravity

```text
Content/ASTRAWILD/Blueprints/BP_AstrawildPlayer.uasset
Content/ASTRAWILD/Blueprints/BP_Echo_Explorer.uasset
Content/ASTRAWILD/Blueprints/BP_Echo_Combat.uasset
Content/ASTRAWILD/Blueprints/BP_Echo_Base.uasset
Content/ASTRAWILD/Blueprints/BP_ResourceNode.uasset
Content/ASTRAWILD/Blueprints/BP_RestPoint.uasset
Content/ASTRAWILD/Blueprints/BP_DamageTarget.uasset
Content/ASTRAWILD/Data/DA_Item_*.uasset
Content/ASTRAWILD/Data/DA_Recipe_*.uasset
Content/ASTRAWILD/Data/DA_Echo_*.uasset
Content/ASTRAWILD/Input/IA_*.uasset
Content/ASTRAWILD/Input/IMC_*.uasset
Content/ASTRAWILD/Maps/Prototype/L_Prototype.umap
Content/ASTRAWILD/UI/WBP_*.uasset
```

These paths are contracts, not claims that the files already exist. Verify the Content Browser and Git tree before reporting completion.
