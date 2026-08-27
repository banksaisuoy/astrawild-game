# ASTRAWILD Evolution Contract

The repository now contains an additive evolution runtime and `DT_Evolutions.csv` with 12 original species paths. The native component preserves Echo GUID, nickname, trust, ownership, parents, generation, mutation count, level, and current health while swapping to the target species DataAsset. Evolution is server-authoritative and refunds the catalyst if the target DataAsset cannot load.

## Editor import

Import `DT_Evolutions.csv` as a DataTable using `FAstrawildEvolutionRow`. Create the target Echo DataAssets under the paths declared in the `TargetSpeciesData` column, assign each target asset’s `SpeciesTag` to the matching `TargetSpeciesTag`, and set the table on the native `Evolution` component of the Echo Blueprint. The CSV path is a contract; it is not proof that the binary DataAssets exist.

## Acceptance gates

| Gate | Expected result |
|---|---|
| Source species | An Echo with no matching row reports no evolution path and does not mutate. |
| Level | An Echo below `RequiredLevel` is rejected without consuming a catalyst. |
| Catalyst | A required catalyst is checked and consumed exactly once on the server. |
| Target asset | A missing target DataAsset rejects safely and refunds the catalyst. |
| Preservation | GUID, nickname, trust, parents, ownership, generation, and mutation count survive the swap. |
| Presentation | Target mesh, AnimBP, icon, partner skill, work tags, and mount profile become visible after target DataAsset assignment. |
| Save/load | The evolved `SpeciesTag` and additive lineage fields survive save/load. |
| Multiplayer | A client cannot evolve an Echo directly; the server owns the mutation and broadcasts the event. |

Run the source validators before opening Unreal. Then perform the level/catalyst/target-asset/PIE gates and update `Docs/BUILD_STATUS.md` with actual Windows evidence.
