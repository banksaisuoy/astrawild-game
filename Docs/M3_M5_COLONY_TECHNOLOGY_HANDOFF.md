# ASTRAWILD Milestones 3–5 — SAN, Colony Work, and Technology Handoff

## Scope and verification boundary

This slice adds native contracts for Echo SAN, colony work orders, building-hosted work queues, and player technology progression. It also adds source DataTables for 20 technology nodes. It does not claim that a finished colony building set, worker navigation graph, production UI, or binary DataTables already exist.

## Runtime contracts

| System | Native owner | Data source | Expected Editor work |
|---|---|---|---|
| SAN | `UAstrawildSanComponent` on `AAstrawildEchoBase` | Echo instance fields and trait rows | Bind SAN bar, critical warning, rest/care interactions |
| Colony work | `UAstrawildColonyWorkComponent` on `AAstrawildBuildingPiece` | Work suitability tags on EchoDex | Place stations, register workers, route output inventory |
| Technology | `UAstrawildTechnologyComponent` on player | `DT_TechnologyNodes.csv` | Import DataTable, connect research UI, award research points |
| Save | `FAstrawildPlayerProfile` and `FAstrawildWorldSnapshot` | Additive fields | Verify legacy v1 save loads with empty new arrays |

## Design rules

The colony scheduler is server-authoritative. A worker must be registered, have a matching `WorkSuitabilityTags` entry, and not be SAN-critical. Work progress uses the worker’s `WorkEfficiencyMultiplier`; SAN decreases under work stress and recovers outside work. A completed order emits an output tag/quantity event; a Blueprint station or inventory router owns the final item grant so native contracts do not hard-code a specific building layout.

Technology unlocks are server-side gameplay decisions. The component checks the DataTable row, duplicate unlock state, research cost, and every prerequisite tag before spending points. UI should call `CanUnlockTechnology` to show reasons and call `TryUnlockTechnology` only on the authoritative player. Do not infer unlocks from hidden UI state.

## Editor setup

1. Import `DT_TechnologyNodes.csv` with `FAstrawildTechnologyNodeRow` into `Content/Astrawild/Data/Imported/DT_TechnologyNodes`.
2. Add a technology DataTable reference to the player Blueprint’s native `Technology` component. Create a UMG research screen that groups rows by `Tier` and disables locked nodes with the native failure reason.
3. Create a Blueprint child for a work-capable building such as `BP_CraftingBench_Colony`. Use the inherited `ColonyWork` component and connect completion output to a validated inventory grant. Do not add another ColonyWork component in the child Blueprint.
4. On capture/worker assignment, copy the Echo instance’s WorkSuitabilityTags and WorkEfficiencyMultiplier into the worker’s native state. Register/unregister workers on station occupancy or colony assignment; unregister before destroying an Echo actor.
5. Create a rest/care interaction that calls `ModifySAN` or otherwise restores SAN without bypassing the component’s clamping and events. The visual state should use `OnSANChanged` and `OnSANStateChanged`, not a second ad-hoc SAN variable.
6. Create station navigation/interaction volumes and test that workers cannot be assigned across a blocked or unloaded World Partition cell without an explicit production decision.

## Acceptance tests

| Test | Expected result |
|---|---|
| SAN clamp | SAN stays in `0..MaxSAN`; critical state changes only at threshold crossing |
| Work suitability | A mismatched worker cannot claim an order |
| Critical worker | A SAN-critical worker cannot claim or progress an order |
| Work completion | Output event fires once and order is removed |
| Technology prerequisites | Missing prerequisite or insufficient points does not mutate unlock state |
| Legacy save | v1 save with missing new arrays loads without crash and defaults safely |
| Authority | Client cannot advance colony work or spend technology points without server authority |

Source validation is not Unreal compile. Windows must compile the module, import DataTables, run Automation tests, play a listen-server PIE session, and package a Development build before any production readiness claim.
