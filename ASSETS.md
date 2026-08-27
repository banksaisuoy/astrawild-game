# ASTRAWILD Asset Manifest

## Visual target

- Reference: `Docs/visual_target_astrawild.png`
- Purpose: define silhouette, landmark readability, palette and composition for the primitive Vertical Slice.
- Status: concept reference only; not a final 3D asset and not a replacement for an imported Unreal asset.

## Current actual assets

| ID | Asset | Type | Current status |
|---|---|---|---|
| `VISUAL_TARGET_DAWN_VALLEY` | `Docs/visual_target_astrawild.png` | concept image | present |
| `ENGINE_CUBE_PLACEHOLDER` | Unreal Basic Shapes Cube | runtime placeholder | referenced by C++ |
| `ENGINE_SPHERE_PLACEHOLDER` | Unreal Basic Shapes Sphere | runtime placeholder | referenced by C++ |
| `ENGINE_CYLINDER_PLACEHOLDER` | Unreal Basic Shapes Cylinder | runtime placeholder | referenced by C++ |

## Required Content Browser assets

| ID | Expected path | Type | Status | Source/license required |
|---|---|---|---|---|
| `BP_PLAYER` | `Content/ASTRAWILD/Blueprints/BP_AstrawildPlayer` | Blueprint | missing at audit | engine/project |
| `BP_ECHO_EXPLORER` | `Content/ASTRAWILD/Blueprints/BP_Echo_Explorer` | Blueprint | missing at audit | original/licensed |
| `BP_ECHO_COMBAT` | `Content/ASTRAWILD/Blueprints/BP_Echo_Combat` | Blueprint | missing at audit | original/licensed |
| `BP_ECHO_BASE` | `Content/ASTRAWILD/Blueprints/BP_Echo_Base` | Blueprint | missing at audit | original/licensed |
| `DA_ITEM_WOOD` | `Content/ASTRAWILD/Data/DA_Item_Wood` | Item Data Asset | missing at audit | project data |
| `DA_ITEM_STONE` | `Content/ASTRAWILD/Data/DA_Item_Stone` | Item Data Asset | missing at audit | project data |
| `DA_ITEM_RESONATOR` | `Content/ASTRAWILD/Data/DA_Item_Resonator` | Item Data Asset | missing at audit | project data |
| `DA_RECIPE_REST_POINT` | `Content/ASTRAWILD/Data/DA_Recipe_RestPoint` | Recipe Data Asset | missing at audit | project data |
| `DA_ECHO_EXPLORER` | `Content/ASTRAWILD/Data/DA_Echo_Explorer` | Echo Data Asset | missing at audit | original/licensed |
| `DA_ECHO_COMBAT` | `Content/ASTRAWILD/Data/DA_Echo_Combat` | Echo Data Asset | missing at audit | original/licensed |
| `DA_ECHO_BASE` | `Content/ASTRAWILD/Data/DA_Echo_Base` | Echo Data Asset | missing at audit | original/licensed |
| `L_PROTOTYPE` | `Content/ASTRAWILD/Maps/Prototype/L_Prototype` | Map | missing at audit | project composition |
| `UI_HUD` | `Content/ASTRAWILD/UI/WBP_PrototypeHUD` | Widget Blueprint | missing at audit | project UI |
| `INPUT_MOVE` | `Content/ASTRAWILD/Input/IA_Move` | Input Action | missing at audit | project input |
| `INPUT_LOOK` | `Content/ASTRAWILD/Input/IA_Look` | Input Action | missing at audit | project input |
| `INPUT_INTERACT` | `Content/ASTRAWILD/Input/IA_Interact` | Input Action | missing at audit | project input |
| `INPUT_SPRINT` | `Content/ASTRAWILD/Input/IA_Sprint` | Input Action | missing at audit | project input |
| `INPUT_CONTEXT` | `Content/ASTRAWILD/Input/IMC_Player` | Mapping Context | missing at audit | project input |

## Replacement order

1. Keep engine primitive placeholders until the gameplay gate passes.
2. Replace environment primitives with licensed environment assets and record each in `Docs/ThirdPartyLicenses.md`.
3. Create original Echo silhouettes and animations; do not use recognizable franchise creatures.
4. Add VFX, sound and UI polish only after the core-loop screenshot/video is readable.

## License record rule

Every external asset must have creator, URL, exact license, date and usage recorded in `Docs/ThirdPartyLicenses.md`. An asset being free to download is not proof that it is free to redistribute.
