# ASTRAWILD — Input Reference

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-30** (wave 3 sync — X = equip-best, `AW.EquipItem` cheat)
**Primary sources:** `AstrawildPlayerCharacter.cpp` (BuildRuntimeInputDefaults / SetupPlayerInputComponent /
input handlers), `AstrawildCheatManager.h/.cpp`

Defaults below are the **runtime-built Enhanced Input mapping** (used when no editor IMC asset is
assigned). Keyboard + mouse only; gamepad support is NOT IMPLEMENTED.

---

## 1. Complete Keybinding Table (17 keys → 17 actions)

`BuildRuntimeInputDefaults` creates exactly **17 runtime actions** (`MakeRuntimeAction` count) and the
log line now matches ("17 actions, WASD+mouse"). Key count = physical keyboard keys (mouse inputs
listed separately).

| Key | Action (runtime name) | Trigger events | Handler | System driven | Notes |
|---|---|---|---|---|---|
| **W / A / S / D** | Move (`AWD_Move`, Axis2D) | Triggered | `Move` | Character movement | WASD feed one 2D action; A/S use Negate modifiers (X/Y) |
| **Mouse delta** | Look (`AWD_Look`, Axis2D) | Triggered | `Look` | Camera | Y axis negated (standard 3rd-person pitch) |
| **Left Shift** | Sprint (`AWD_Sprint`) | Started / Completed / Canceled | `StartSprint` / `StopSprint` | Movement (450→700 speed), stamina | Sprint blocked below 5 % stamina |
| **Space** | Jump (`AWD_Jump`) | Started / Completed | `HandleJump` / `StopJumping` | Movement | JumpZ 600 |
| **E** | Interact (`AWD_Interact`) | Started | `Interact` | Interaction system: harvest / craft station / rest / NPC / **capture Echo** | Camera ray 300 cm; wild Echo in reach → capture attempt |
| **Left Mouse Button** | Light Attack (`AWD_LightAttack`) | Started | `Attack` | Combat (25 dmg, 0.45 s CD) — **or Building confirm** while placing | Placement mode intercepts the input |
| **Right Mouse Button (hold)** | Block (`AWD_Block`) | Started / Completed / Canceled | `StartBlock` / `StopBlock` | Combat (45 % unarmed mitigation / 65 % with Stonehide Shield, ×0.45 move speed) | Wave 3: shield replaces the unarmed baseline |
| **F** | Heavy Attack (`AWD_HeavyAttack`) | Started | `HeavyAttack` | Combat (60 dmg, 1.3 s CD, 25 stamina) | |
| **Q** | Dodge (`AWD_Dodge`) | Started | `Dodge` | Combat (0.4 s i-frames, 900 impulse, 0.9 s CD, 22 stamina) | Dodges along movement input (or forward) |
| **C** | Party Command (`AWD_Command`) | Started | `CyclePartyCommand` | Echo commands: Follow → Attack → Defend → Stay → Work → (loop) | Broadcast to all owned captured Echoes; each rolls obedience |
| **R** | Feed (`AWD_Feed`) | Started | `FeedTarget` | Echo trust/bond/capture pipeline | Preferred food first, then any EchoFeedValue item |
| **B** | Build Mode (`AWD_BuildMode`) | Started | `ToggleBuildMode` | Building placement (toggle in/out) | Refuses when nothing unlocked |
| **N** | Rotate Building (`AWD_BuildRotate`) | Started | `RotateBuilding` | Building placement (+15° yaw per press) | |
| **G** | Smart Consume (`AWD_Consume`) | Started | `SmartConsume` | Survival: addresses the most depleted vital (thirst vs hunger) by consuming the best matching owned consumable (score = needed vital value + heal × 0.5) | Added with the T-5 fix (verify at playtest) |
| **X** | Equip Best (`AWD_EquipBest`) | Started | `EquipBest` | Equipment (wave 3): equips strongest owned weapon (max AttackPower) + strongest shield (max BlockMitigation) | Authority only; logs the chosen ids |
| **F5** | Quick Save (`AWD_Save`) | Started | `QuickSave` | Save system → `ASTRAWILD_Main` | Server/authority only |
| **F9** | Quick Load (`AWD_Load`) | Started | `QuickLoad` | Save system → restores world | Server/authority only |
| *(LMB while placing)* | — (routes from Attack) | Started | `BuildingComponent::ConfirmPlacement` | Building: consume materials + server RPC | See LMB row |

Death disables input until respawn (5 s GameMode timer). Building-cycling between piece types
(`CycleBuildingDefinition`) is a component API **without a keybind** yet.

---

## 2. Action → System Cross-Reference

| System | Keys involved |
|---|---|
| Movement / camera | W A S D, mouse, Shift, Space |
| Combat | LMB, RMB, F, Q |
| Interaction (harvest/craft/rest/talk) | E |
| Capture | E (on wild Echo) + R (feed, trust path) |
| Echo commands / party | C |
| Building | B, N, LMB |
| Survival (smart consume) | G |
| Equipment (equip best) | X |
| Save / Load | F5, F9 |

---

## 3. Console Cheat Reference (`UAstrawildCheatManager`)

Active in non-Shipping builds (engine strips CheatManagers in Shipping). Open the console with `~` and
prefix commands with `AW.`. All IDs are the registry ids from the Asset Manifest. **13 commands.**

| Command | Arguments | Effect |
|---|---|---|
| `AW.SpawnEcho` | `<EchoDefinitionId>` | Spawns that Echo 400 cm in front of the player (e.g. `AW.SpawnEcho Echo_Lumewisp`) |
| `AW.GiveItem` | `<ItemId> [Quantity]` | Adds items to the player inventory (e.g. `AW.GiveItem Item_Wood 50`) |
| `AW.EquipItem` | `<ItemId>` | Equips an owned equipment item by id — routes to the weapon slot (`AttackPower > 0`) or shield slot (`BlockMitigation > 0`); warns when the item is missing or not equipment (wave 3) |
| `AW.SetTime` | `<Hour> <Minute>` | Jumps the world clock (0–23 / 0–59); e.g. `AW.SetTime 22 0` for night |
| `AW.SetWeather` | `<clear\|cloudy\|rain\|heavyrain\|storm\|fog\|heat\|cold>` | Forces a weather state (server) |
| `AW.God` | — | Toggles player god mode (damage immune) |
| `AW.HealAll` | — | Full vitals restore (HP/stamina/hunger/thirst, clears status effects) |
| `AW.ResearchPoints` | `<Amount>` | Adds research points to the shared pool |
| `AW.UnlockTech` | `<TechId>` | Attempts a tech unlock (respects prereqs/points) — e.g. `AW.UnlockTech Tech_Electrical` |
| `AW.SaveNow` | — | Saves the world to `ASTRAWILD_Main` immediately |
| `AW.LoadNow` | — | Loads the world from `ASTRAWILD_Main` immediately |
| `AW.CaptureAll` | — | Captures every wild, alive Echo in the world at trust 50 (roster/party testing) |
| `AW.TeleportForward` | `<Distance>` | Teleports forward along facing (+100 cm Z safety) |

Useful engine companions: `stat unit`, `stat fps`, `showdebug abilitysystem` (n/a), `slomo <scale>` for
time-scale testing of decay rates.

---

## 4. Remapping Notes

- **Today:** bindings are code-defined (`BuildRuntimeInputDefaults`); no in-game remap UI. Values can be
  changed by editing the mapping code or by authoring an editor IMC asset and assigning
  `DefaultMappingContext` on the player Blueprint (the runtime build then skips itself).
- **Planned:** real IMC/IA assets (M9) + settings screen with full remap, sensitivity, invert, toggle/hold,
  aim assist per master plan §5/§12.
