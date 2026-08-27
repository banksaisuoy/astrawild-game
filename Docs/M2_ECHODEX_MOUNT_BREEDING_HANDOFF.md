# ASTRAWILD Milestone 2 — EchoDex, Mount, Traits, and Breeding Handoff

## Scope and truth boundary

The branch now contains an original **30-entry EchoDex source table**, elemental compatibility code with legacy enum preservation, passive-trait and breeding-group schemas, mount profiles, egg state, and C++ components. It does **not** contain 3D meshes, icons, animation Blueprints, binary DataTables, saddle assets, or a finished breeding pen. Those must be authored and tested in Unreal Engine 5.8 on Windows.

## Import order

| Order | CSV source | Row struct | Expected rows |
|---:|---|---|---:|
| 1 | `DT_EchoDex.csv` | `FAstrawildEchoDexRow` | 30 |
| 2 | `DT_EchoTraits.csv` | `FAstrawildEchoTraitRow` | 24 |
| 3 | `DT_BreedingGroups.csv` | `FAstrawildBreedingGroupRow` | 7 |
| 4 | `DT_MountProfiles.csv` | `FAstrawildMountProfile` | 21 |

Create DataTables under `Content/Astrawild/Data/Imported/` and preserve the source row names. The CSV files under `Data/Source/` remain the reviewable source of truth; Editor-generated DataTables are derived binary artifacts.

## Editor assembly

1. Import `DT_EchoDex.csv` with `FAstrawildEchoDexRow`. Confirm `DexOrder` is 1–30 and that primary/secondary elements match the compatibility plan. Create one DataAsset per species only when a real visual/pawn class is available; leave soft visual references empty instead of pointing at placeholder or third-party assets.
2. Import `DT_EchoTraits.csv` and `DT_BreedingGroups.csv`. For each species, assign only tags that exist in those tables. Keep `BreedingGroupId` values stable because they are serialized into captured Echo instances and egg state.
3. Import `DT_MountProfiles.csv`. For every mounted Echo, create or verify a `SaddleSocket` (or the exact profile socket) on the authored skeletal mesh. Test dismount clearance and collision restoration; the component deliberately disables the Echo collision while attached and restores it on dismount.
4. Add `UAstrawildMountComponent` and `UAstrawildBreedingComponent` to the player Blueprint only if the Blueprint does not already inherit the native components. Do not create duplicate components in Blueprint.
5. Bind the mount delegates to UI feedback and a stamina implementation when the final movement design is approved. The current native contract exposes mount speed multiplier and eligibility; it does not pretend to provide finished saddle meshes or animation montages.
6. Bind `OnEggCreated` and `OnEggHatched` to a breeding-pen/egg actor Blueprint. The native component owns deterministic validation, inheritance, incubation progress, save/load state, and server-only progression. The Blueprint owns visual egg presentation, interact prompts, and spawning the authored offspring pawn.
7. Add a DataTable-backed species lookup before calling `TryBreed`; do not accept arbitrary user-provided offspring tags. The server must validate that the offspring species belongs to the parents’ breeding group and that both parents are owned, alive, distinct, and compatible.
8. Run the elemental automation test `Astrawild.Systems.Elements.Compatibility` in the Session Frontend. Confirm old `Neutral/Solar/Torrent/Geo/Aether` values remain stable and new values do not alter legacy save data.
9. Run PIE smoke tests for: capture → party → summon → mount → dismount; breed two compatible captured Echoes → save → reload → egg continues incubating → hatch delegate fires; incompatible parents fail without consuming data; client cannot advance eggs or apply mount state authority-side.

## Original-content and license gate

All species names, descriptions, traits, partner skills, icons, meshes, sounds, and animations must be original or documented in `Docs/ThirdPartyLicenses.md`. Do not import models, textures, names, UI, or audio from Pokémon, ARK, Palworld, Nintendo, Pocketpair, Studio Wildcard, or other protected works. If an external asset is used, record its URL, creator, license, modification status, and redistribution terms before committing the binary asset.

## Windows verification evidence

Update `Docs/BUILD_STATUS.md` with the UE 5.8 compile result, DataTable import screenshots or logs, automation-test result, PIE steps and result, and the packaged build path. Source validation is not equivalent to a compiled Unreal module, and a PIE test is not equivalent to a shipping package.
