# P0 — Skeletal Mesh and Animation Blueprint Contract

## Scope and truth

The release branch contains the C++ runtime hooks for skeletal meshes and animation classes, but the cloned Content tree currently contains no `.uasset`, Skeletal Mesh, Animation Blueprint, montage, skeleton, or animation sequence. This document is a contract for creating those assets in Unreal Editor; it is not a claim that the assets already exist.

## Runtime hooks now available

`AAstrawildCharacter` exposes `PlayerSkeletalMesh`, `PlayerAnimationBlueprintClass`, and `AnimationProfileId`. It loads the soft references during `BeginPlay` and applies them to the inherited `Character` mesh.

`UAstrawildEchoDataAsset` exposes `SkeletalMesh`, `AnimationBlueprintClass`, and `AnimationProfileId`. `AAstrawildEchoBase::ApplyVisualRepresentation` loads these references when present and hides the static fallback mesh; otherwise it retains the existing fallback primitive path.

`UAstrawildAnimInstance` exposes runtime variables for both Player and Echo animation graphs: `GroundSpeed`, `Direction`, `bIsInAir`, `bIsSprinting`, `bIsDodging`, `PlayerMovementState`, `EchoState`, `ElementalAffinity`, and `HealthNormalized`.

## Required Content assets

| ID | Expected path | Parent/type | Required setup |
|---|---|---|---|
| `SK_Player_Astra` | `Content/Astrawild/Characters/Player/SK_Player_Astra` | Skeletal Mesh | compatible humanoid skeleton, physics asset, sockets `hand_r`, `root`, `weapon_r` |
| `ABP_Player_Astra` | `Content/Astrawild/Characters/Player/ABP_Player_Astra` | Anim Blueprint using `UAstrawildAnimInstance` | locomotion, sprint, fall, dodge, attack montage slots |
| `SK_Pyrelite` | `Content/Astrawild/Characters/Echoes/Pyrelite/SK_Pyrelite` | Skeletal Mesh | `root`, `head`, `resonator_capture`, physics asset |
| `ABP_Pyrelite` | `Content/Astrawild/Characters/Echoes/Pyrelite/ABP_Pyrelite` | Anim Blueprint using `UAstrawildAnimInstance` | passive, hostile, flee, companion, hit, defeat |
| `SK_Thornback` | `Content/Astrawild/Characters/Echoes/Thornback/SK_Thornback` | Skeletal Mesh | same gameplay sockets, species-specific extras allowed |
| `ABP_Thornback` | `Content/Astrawild/Characters/Echoes/Thornback/ABP_Thornback` | Anim Blueprint using `UAstrawildAnimInstance` | passive, hostile, flee, companion, hit, defeat |
| `SK_Aquavine` | `Content/Astrawild/Characters/Echoes/Aquavine/SK_Aquavine` | Skeletal Mesh | same gameplay sockets, tail/water extras allowed |
| `ABP_Aquavine` | `Content/Astrawild/Characters/Echoes/Aquavine/ABP_Aquavine` | Anim Blueprint using `UAstrawildAnimInstance` | passive, hostile, flee, companion, hit, defeat |

## Player state machine

Use a locomotion state machine with the following states:

| State | Entry condition | Exit condition | Notes |
|---|---|---|---|
| Idle/Walk | `GroundSpeed < 10` or `GroundSpeed < WalkThreshold` | speed threshold changes | blend by GroundSpeed |
| Run/Sprint | `bIsSprinting` and `GroundSpeed > WalkThreshold` | sprint false or speed low | do not infer sprint from speed alone |
| InAir | `bIsInAir` | grounded | use jump/fall blend |
| Dodge | `bIsDodging` | dodge false | montage or state priority above locomotion |
| Hit | gameplay hit event | montage notify/end | additive or montage slot |
| Defeat | health normalized <= 0 or Echo death event | terminal | disable locomotion |

Attack montages should use named sections `Attack_1`, `Attack_2`, `Attack_3` and notify events `ComboWindow_Open`, `ComboWindow_Close`, `DamageWindow`, and `Footstep`. The existing `UAstrawildCombatComponent` remains the authority for damage and combo index; animation notifies only request or signal presentation timing.

## Echo state machine

Drive the state machine from `EchoState` and `GroundSpeed`. The minimum states are `WildPassive`, `WildHostile`, `Fleeing`, `Captured`, `SummonedCompanion`, and `Working`. Use species Data Asset element and role only for additive overlays, VFX selection, and optional locomotion variations.

## Import and validation gate

Antigravity must verify skeleton compatibility, retargeting, physics asset, capsule collision, root motion policy, LODs, animation blueprint parent class, preview mesh, and all sockets. After creating assets, assign the Player class references and the three Echo Data Assets. Record exact Content Browser paths in `Docs/BUILD_STATUS.md` and include one PIE screenshot of each species in passive and combat states.

## Asset source rule

Use original or clearly licensed meshes. Do not import models or animations from Pokémon, ARK, Palworld, Nintendo, Pocketpair, Studio Wildcard, or other games. Record every external source in `Docs/ThirdPartyLicenses.md`.
