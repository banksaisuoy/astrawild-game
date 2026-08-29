# ASTRAWILD — Asset Pipeline (CODE_DEFAULT → .uasset)

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine) — the registry override
mechanism is in code; **no .uasset content exists yet** (`Content/ASTRAWILD/` is empty).**
**Date: 2026-08-29**
**Primary sources:** `AstrawildItemRegistrySubsystem.h/.cpp`, `AstrawildContentLibrary.cpp`,
`.gitattributes` (LFS), `ASTRAWILD_ASSET_MANIFEST.md`

---

## 1. The Replacement Strategy

All gameplay content resolves through `UAstrawildItemRegistrySubsystem` maps keyed by stable `FName` ids.
The ContentLibrary seeds **CODE_DEFAULT definitions at world begin (server)**. Real content replaces them
**by registering a data asset with the same id** — no gameplay code changes.

```
World begin (server, non-client netmode)
  └─ ItemRegistrySubsystem::OnWorldBeginPlay
       └─ UAstrawildContentLibrary::BuildDefaults(registry)     ← CODE_DEFAULT seeds
            Items / Recipes / Echoes / Buildings / Technologies / Quests

Later (asset pass — M8+):
  your bootstrapping code / GameInstance init:
       Registry->RegisterItem(LoadedUAssetDefinition)           ← same ItemId ⇒ overrides the map entry
       Registry->RegisterEcho(...), RegisterBuilding(...), ...
```

Key facts:

- **Id is the contract.** `TMap.Add` with an existing key replaces the value — that *is* the override
  mechanism. Keep ids identical (`Item_Wood`, `Echo_Lumewisp`, `Building_Generator`, …).
- The full id list lives in `ASTRAWILD_ASSET_MANIFEST.md` — treat it as the replacement checklist.
- Definitions are `UPrimaryDataAsset` subclasses with correct `GetPrimaryAssetId` overrides
  (`Item`, `Recipe`, `Echo`, `Building`, `Tech`, `Quest`, `LootTable`, `NPC` primary asset types), so they
  also participate in asset manager scanning/async loading when you adopt that flow.
- Gameplay code resolves ids through `FindItem/FindRecipe/FindEcho/FindBuilding/FindTechnology/FindQuest`
  **only** — there are no hard object references to break.
- Balance values migrate field-by-field: every CODE_DEFAULT value in the ContentLibrary maps 1:1 onto a
  definition property (e.g. Lumewisp HP 60 → `BaseStats.MaxHealth`). The manifest records the current
  values as the starting-point balance.

---

## 2. Placeholder Visual Policy

**Every visible object in the game is an engine basic shape** — deliberately — so the project plays with
zero binary content. All placeholders are tagged `PLACEHOLDER` / `REPLACE_BEFORE_RELEASE`:

| Object | Placeholder | File |
|---|---|---|
| Player | Engine cylinder (scale 0.45, 0.45, 0.95) | PlayerCharacter.cpp |
| Echo (all species) | Engine sphere (scale 0.8) | EchoCharacter.cpp |
| NPC | Engine capsule (scale 0.4, 0.4, 0.9) | NPCCharacter.cpp |
| Resource node | Engine cube (0.65, 0.65, 0.8) | ResourceNode.cpp |
| Rest point | Engine cylinder (0.75, 0.75, 1.4) | RestPoint.cpp |
| Crafting station | Engine cylinder (0.8, 0.8, 0.6) | CraftingStationActor.cpp |
| Work site | Engine cube (1.2, 1.2, 0.4) | WorkSiteActor.cpp |
| Building (all categories) | Engine cube with per-category scale (Foundation 2/2/0.2, Wall 2/0.2/1.5, Power 0.9/0.9/1.4, other 1.2/1.2/1.0) | BuildingActor.cpp |
| Ground | Engine plane ×80 | WorldBootstrapper.cpp |
| Lighting | Spawned engine DirectionalLight/SkyLight/SkyAtmosphere/HeightFog | WorldBootstrapper.cpp |
| HUD | Engine Roboto font | HudWidget.cpp |

Policy: a placeholder may be replaced at any time without touching gameplay code (meshes are soft
references or spawned engine meshes); **no placeholder ships** — the release gate includes a
zero-placeholder check against the manifest.

---

## 3. Content Folder Conventions

```
Content/
└── ASTRAWILD/
    ├── Data/            ← data assets (DA_Item_*, DA_Echo_*, DA_Building_*,
    │                        DA_Tech_*, DA_Quest_*, DA_Recipe_*, DA_LootTable_*, DA_NPC_*)
    ├── Characters/      ← skeletal meshes, anim BP, physics assets
    ├── Environment/     ← meshes, materials, landscape layers, foliage
    ├── VFX/  Audio/  UI/← systems content
    ├── Maps/            ← .umap files (Dawn Fields real map, future biomes)
    └── Input/           ← IMC/IA assets (optional — runtime defaults already work)
```

Rules:
- Every content file lives under `Content/ASTRAWILD/` (project namespace — required for PrimaryAssetId
  hygiene and future cooking rules).
- Data asset naming: `DA_<Kind>_<DisplayName>` matching the id (e.g. `DA_Echo_Lumewisp` for
  `Echo_Lumewisp`).
- One definition per file; ids never change once content depends on them (save compatibility).

---

## 4. LFS / Source Control Config

- `.gitattributes` already configures **Git LFS** for binary types (`*.uasset`, `*.umap`, `*.png`, textures,
  meshes, audio, etc. — see repo root).
- Text (code/config/docs) stays outside LFS. Never LFS the `Source/` tree.
- Workflow: pull **before** opening the editor (uasset merge conflicts are unrecoverable — one artist per
  asset at a time; communicate ownership).
- Validate pushes with `Scripts/validate_repository.sh` (static checks) — CI compile remains the
  target-machine responsibility this round.

---

## 5. Asset Creation Order (recommended first pass on the user machine)

1. **Echo species** (5) — highest gameplay visibility; wire `SkeletalMesh` + `Icon` on the data assets,
   register, and the sphere placeholders disappear for those species.
2. **Items** (10 icons + definitions) — feeds the future inventory UI.
3. **Recipes/Buildings/Techs/Quests** (data assets mirroring the manifest values).
4. **Real Dawn Fields map** — replace the bootstrapper arena (keep the bootstrapper as a
   PIE-no-map fallback).
5. **Input assets** (IMC/IA) — optional; assign to `DefaultMappingContext`.
6. Only after the above: VFX/audio pass.

Each step is independently shippable to the team because the registry contract stays stable.

---

## 6. Not Implemented (honest)

| Item | Status |
|---|---|
| Asset Manager / streamable asset scan (`GetPrimaryAssetId` groundwork exists) | PLANNED |
| Any actual .uasset/.umap | NOT CREATED (empty Content/ASTRAWILD/) |
| Material/texture pipeline docs | PLANNED with the art content pass (see `astra_wild_art_content.md`) |
