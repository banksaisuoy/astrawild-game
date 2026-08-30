# ASTRAWILD — Antigravity Build & Test Checklist

## Phase A: Pull and generate

- [ ] Clone or pull `https://github.com/banksaisuoy/astrawild-game.git` on the `main` branch or a feature branch.
- [ ] Confirm Unreal Engine version is 5.8 or report the installed version before changing project files.
- [ ] Open `ASTRAWILD.uproject` and regenerate project files.
- [ ] Confirm `AstrawildCore` is detected and no plugin is missing.

## Phase B: Compile

- [ ] Build `ASTRAWILDEditor Win64 Development`.
- [ ] Fix include/API errors without changing public contracts unless documented.
- [ ] Confirm generated headers are produced for every `UCLASS`, `USTRUCT`, `UENUM`, `UINTERFACE`, and delegate owner.
- [ ] Confirm no warnings are promoted to errors by the local toolchain.
- [ ] Record engine, compiler, platform, error count, warning count, and build time in `Docs/BUILD_STATUS.md`.

## Phase C: Create required Blueprint assets

Create these assets under `Content/ASTRAWILD/` using the C++ classes in the repository:

| Asset | Parent class / contract | Required setup |
|---|---|---|
| `BP_AstrawildPlayer` | `AAstrawildPlayerCharacter` | Assign mesh, animations, Input Actions, Mapping Context, camera tuning |
| `BP_AstrawildGameMode` | `AAstrawildGameMode` | Optional override; keep default pawn valid |
| `BP_Echo_Explorer` | `AAstrawildEchoCharacter` | Assign `DA_Echo_Explorer` |
| `BP_Echo_Combat` | `AAstrawildEchoCharacter` | Assign `DA_Echo_Combat` |
| `BP_Echo_Base` | `AAstrawildEchoCharacter` | Assign `DA_Echo_Base` |
| `BP_ResourceNode` | `AAstrawildResourceNode` | Assign resource item ID and mesh |
| `BP_RestPoint` | `AAstrawildRestPoint` | Add mesh/VFX and interaction collision |
| `BP_TestTarget` | Any damage receiver | Expose health and hit feedback |

## Phase D: Create Data Assets

- [ ] Create three `UAstrawildEchoDefinition` assets with IDs `Echo_Explorer`, `Echo_Combat`, and `Echo_Base`.
- [ ] Create item assets `Item_Wood`, `Item_Stone`, `Item_PulseShard`, and `Item_RestPointKit`.
- [ ] Create recipe `Recipe_RestPoint` using the item IDs above.
- [ ] Confirm every Definition ID and Item ID is unique and non-empty.
- [ ] Assign icons only after the gameplay data is validated.

## Phase E: Create test map

- [ ] Create `Content/ASTRAWILD/Maps/Prototype/L_Prototype.umap`.
- [ ] Add `BP_AstrawildGameMode` or set `AAstrawildGameMode` as the map GameMode override.
- [ ] Add a player start, three Echo actors, resource nodes, a rest point, a test target, directional light, sky light, floor, and navigation volume if AI is enabled.
- [ ] Assign `L_Prototype` to `EditorStartupMap` only after the map exists.
- [ ] Add a simple HUD or debug text so the test state is visible.

## Phase F: Playtest

Test the following in order:

1. Start Play-in-Editor.
2. Move, look, jump, sprint, and interact.
3. Harvest `Item_Wood` and `Item_Stone` from resource nodes.
4. Confirm inventory quantities increase once per interaction.
5. Apply damage to an Echo or test target.
6. Capture an Echo and confirm `InstanceId`, `Trust`, and `bCaptured`.
7. Craft `Recipe_RestPoint` and confirm ingredients are consumed once.
8. Activate a Rest Point and confirm `WorldObjectId` and active state.
9. Save through `UAstrawildSaveSubsystem`.
10. Stop Play-in-Editor, start again, load, and verify inventory/Echo/rest point data.

## Phase F2: Shattered Vale zone tour (Batch 7 — after Phase F)

1. **First frame**: rolling meadow terrain (not a flat plane), camp on the ground, six distinct
   horizon silhouettes; HUD banner reads `Dawn Fields · Threat 1` under the clock.
2. **Zone transition**: walk ~1.2 km east → banner flips to `Hollow Approach · Threat 4`, a
   `Region discovered: Hollow Approach (2/6)` notification fires once; ash spires + dim red light.
3. **Ember Ridge**: orange lava-mound lights visibly flicker; obsidian spires on ridgelines.
4. **Frostveil Expanse**: terrain climbs ~23 m to the zone center; ice pillars + cool blue light;
   crests read snow-pale (if the vertex-color material loaded — plain gray otherwise, see ZONE_WORLD §8).
5. **Glimmerwood**: pulsing violet crystal light; the single Ancient-rare Auroraling should be
   findable (deep zone, spawn ≥ 200 m from center).
6. **Dusk Marsh**: dips below Z 0 render as dark muck pools; teal wisps flicker over them.
7. **Zone persistence**: save → stop PIE → play → load — the discovered-zone count is restored.
8. **Perf pass**: `Stat Unit` across at least three zones; flag if the ~196k-tri terrain or the
   ~20 point lights push frame time over budget on the target GPU.
9. **Optional (editor-only)**: import `Content/Heightmaps/Zone_DawnFields_505.r16` per
   `Content/Heightmaps/README.md` to compare the editor Landscape against the runtime tiles.

## Phase G: Handoff report

Create or update `Docs/BUILD_STATUS.md` with:

- Unreal and compiler versions.
- Build target and result.
- Map and Blueprint assets created.
- Playtest result for each step.
- Errors, warnings, crashes, or missing assets.
- Git branch and commit hash.
- Exact next action for Manus AI.

Do not mark the project complete when the result is only a successful C++ compile. Mark it `PARTIAL` until the map, Blueprint references, and save/load Playtest have passed.
