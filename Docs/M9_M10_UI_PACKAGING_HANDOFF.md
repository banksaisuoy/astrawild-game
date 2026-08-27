# ASTRAWILD Milestones 9–10 — Master UI and Windows Packaging Handoff

## Current truth boundary

The branch contains native master-widget bases for gameplay HUD, inventory/crafting composition, EchoDex, technology tree, and dungeon status. It also contains an opt-in PowerShell validation/package path. The repository still does **not** contain finished `.uasset`/`.umap` binary assets, authored widget Blueprints, final fonts/icons, a production map, or a verified executable. Those require Unreal Engine 5.8 on Windows.

## UI assembly contract

Create the following derived Widget Blueprints under `Content/Astrawild/UI/`:

| Widget Blueprint | Native parent | Required binding/data |
|---|---|---|
| `WBP_MasterHUD` | `UAstrawildMasterHUDWidget` | Optional child panels and safe margins |
| `WBP_GameplayHUD` | `UAstrawildGameplayHUDWidget` | Health, stamina, hunger, thirst, SAN progress bars |
| `WBP_Inventory` | `UAstrawildInventoryWidget` | `InventoryGrid`, slot widget class |
| `WBP_Crafting` | `UAstrawildCraftingWidget` | Recipe list, selected recipe, craft failure text |
| `WBP_EchoDex` | `UAstrawildEchoDexWidget` | 30-entry list, element badges, selected detail |
| `WBP_Technology` | `UAstrawildTechnologyTreeWidget` | Tier groups, points, prerequisite state |
| `WBP_DungeonStatus` | `UAstrawildDungeonStatusWidget` | Active tower, timer, participant count |

Use `RefreshAllPanels` after opening the master screen and let the native panel methods remain the source of truth. Do not duplicate inventory, survival, SAN, technology, or dungeon state in Blueprint variables. The native widget code is defensive when optional bindings are absent; this is an authoring aid, not a substitute for finished UI art.

## Asset authoring gate

In Unreal Editor, import the source DataTables into `Content/Astrawild/Data/Imported/`, create the player and Echo Blueprint children, assign the real skeletal meshes/AnimBPs, and create the UI widget children above. Add original icons, fonts, Niagara systems, sound cues, tower meshes, and map assets only after recording their source/license in `Docs/ThirdPartyLicenses.md`. Never import or rename protected assets from Pokémon, ARK, Palworld, Nintendo, Pocketpair, or Studio Wildcard.

## Verification sequence on Windows

Run from the repository root in a Developer PowerShell after pulling `release/vertical-slice-v1`:

```powershell
python Scripts/validate_content_contracts.py
.\Tools\Validate_Astrawild.ps1 -ProjectRoot (Get-Location)
.\Tools\Validate_Astrawild.ps1 -ProjectRoot (Get-Location) -TryUnreal
.\Tools\Validate_Astrawild.ps1 -ProjectRoot (Get-Location) -Package -PackageDirectory .\Builds\WindowsDevelopment
```

The first command checks source contracts. The second checks repository paths and binary asset inventory. The third runs the command-line Blueprint compile check if an Unreal executable is discoverable. The package command invokes `RunUAT BuildCookRun` and is the first step that can establish a Development package, but it still requires a real authored map and imported assets.

After compilation, run the Session Frontend automation test `Astrawild.Systems.Elements.Compatibility`, then a PIE smoke test for movement, dodge, capture, summon, inventory, crafting, ranged fire, mount/dismount, breeding save/load, technology unlock, spire travel, and dungeon entry. Run a two-player network PIE test for authority boundaries. Finally launch the archived executable and update `Docs/BUILD_STATUS.md` with exact commit, engine version, warnings/errors, test results, package path, and screenshots.

## Performance and shipping targets

Keep the existing AI distance LOD and World Partition/Data Layer design active. Profile a representative outdoor cell, a base with several workers, and a tower arena with VFX enabled. Record frame time, game thread time, GPU time, memory, and network-role behavior rather than claiming a generic FPS target without measurement. Use Development packaging first; Shipping packaging is a later acceptance step after PIE and network PIE are clean.
