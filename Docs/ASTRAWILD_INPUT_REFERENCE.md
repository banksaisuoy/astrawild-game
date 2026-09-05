# ASTRAWILD — Input Reference

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-09-03** (PCR-3 sync — 31 actions incl. AWD_Journal Field Journal on P + AWD_Roster Echo Roster on L + AWD_Map World Map on M; screens close with their advertised keys; crafting stations open the crafting screen on E)
**Primary sources:** `AstrawildPlayerCharacter.cpp` (BuildRuntimeInputDefaults / BuildGamepadInputDefaults /
SetupPlayerInputComponent / input handlers), `AstrawildCheatManager.h/.cpp`, `AstrawildNPCCharacter.cpp` (vendor transactions)

Defaults below are the **runtime-built Enhanced Input mapping** (used when no editor IMC asset is
assigned). Keyboard + mouse **plus a full gamepad companion context** (final production run — plugs in
and works with zero configuration).

---

## 1. Complete Keybinding Table (31 actions)

`BuildRuntimeInputDefaults` creates **31 runtime actions** (`MakeRuntimeAction` count incl.
Descend). Key count = physical keyboard keys (mouse inputs listed separately).

| Key | Action (runtime name) | Trigger events | Handler | System driven | Notes |
|---|---|---|---|---|---|
| **W / A / S / D** | Move (`AWD_Move`, Axis2D) | Triggered | `Move` | Character movement | WASD feed one 2D action; A/S use Negate modifiers (X/Y) |
| **Mouse delta** | Look (`AWD_Look`, Axis2D) | Triggered | `Look` | Camera | Y axis negated (standard 3rd-person pitch) |
| **Left Shift** | Sprint (`AWD_Sprint`) | Started / Completed / Canceled | `StartSprint` / `StopSprint` | Movement (450→700 speed), stamina | Sprint drains 7 stamina/s while moving (≈14 s from full); blocked below 5 % stamina until recovery; exosuit adds +15 % speed |
| **Space** | Jump (`AWD_Jump`) | Started / Completed | `HandleJump` / `StopJumping` | Movement | JumpZ 600 |
| **E** | Interact (`AWD_Interact`) | Started | `Interact` | Interaction system: harvest / craft station / rest / NPC / **capture Echo** / **research desk opens tree screen** | Camera ray 300 cm; wild Echo in reach → capture attempt |
| **Left Mouse Button** | Light Attack (`AWD_LightAttack`) | Started | `Attack` | Combat (25 dmg, 0.45 s CD) — **or Building confirm** while placing — **or fires the Pulse Lance when a ranged weapon is equipped** (auto-routed) | Placement mode intercepts the input |
| **Right Mouse Button (hold)** | Block (`AWD_Block`) | Started / Completed / Canceled | `StartBlock` / `StopBlock` | Combat (45 % unarmed mitigation / 65 % with Stonehide Shield, ×0.45 move speed) | Shield replaces the unarmed baseline; speed penalty live |
| **F** | Heavy Attack (`AWD_HeavyAttack`) | Started | `HeavyAttack` | Combat (60 dmg, 1.3 s CD, 25 stamina) | |
| **Q** | Dodge (`AWD_Dodge`) | Started | `Dodge` | Combat (0.4 s i-frames, 900 impulse, 0.9 s CD, 22 stamina) | Dodges along movement input (or forward) |
| **C** | Party Command (`AWD_Command`) | Started | `CyclePartyCommand` | Echo commands: Follow → Attack → Defend → Stay → Work → (loop) | Broadcast to all owned captured Echoes; each rolls obedience |
| **R** | Feed (`AWD_Feed`) | Started | `FeedTarget` | Echo trust/bond/capture pipeline | Preferred food first, then any EchoFeedValue item |
| **B** | Build Mode (`AWD_BuildMode`) | Started | `ToggleBuildMode` | Building placement (toggle in/out) | Refuses when nothing unlocked |
| **N** | Rotate Building (`AWD_BuildRotate`) | Started | `RotateBuilding` | Building placement (+15° yaw per press) | |
| **G** | Smart Consume (`AWD_Consume`) | Started | `SmartConsume` | Survival: consumes the best matching owned consumable for the most depleted vital | |
| **X** | Equip Best (`AWD_EquipBest`) | Started | `EquipBest` | Equipment: equips strongest owned weapon + shield | Advanced slots (helmet/exosuit/scanner) equip via the inventory screen |
| **Z** | Delete Building (`AWD_DeleteBuilding`) | Started | `DeleteBuilding` | Building: dismantles the building under the crosshair (5 m reach) + refund | Weight-safe refund |
| **V** (hold) | Scan (`AWD_Scan`) | Started / Completed / Canceled | `StartScan` / `StopScan` | **Scanner framework (final run):** hold to accelerate journal observation ×3 while a Field Scanner is equipped | Requires `Item_FieldScanner` equipped in the scanner slot; HUD shows "SCANNING..." |
| **H** | Deploy Drone (`AWD_DeployDrone`) | Started | `DeployDrone` | **Robotics (final run):** consumes `Item_UtilityDrone` → hovering companion (auto-scan + auto-harvest); press again to recall | One drone per player |
| **J** | Deploy Robot (`AWD_DeployRobot`) | Started | `DeployRobot` | **Robotics (final run):** consumes `Item_UtilityRobot` → mans the nearest unmanned work site (flat rate, power-gated) | |
| **TAB** | Inventory (`AWD_Inventory`) | Started | `ToggleInventoryScreenInput` | **UI (final run):** pack screen — stacks/weight/6-slot loadout, Use/Equip buttons | Closes other screens |
| **K** | Research (`AWD_Research`) | Started | `ToggleResearchScreenInput` | **UI (final run):** research tree screen — costs/prereqs/unlock buttons | Also opens from the Research Desk [E] |
| **P** | Field Journal (`AWD_Journal`) | Started | `ToggleJournalScreenInput` | **UI (PCR-1):** Field Journal bestiary screen — every species' scan/food/habitat/weakness knowledge, observation %, encounter count, collection totals | Undiscovered species read "???" (the collection pull); gamepad reaches it via the pause-menu button |
| **L** | Echo Roster (`AWD_Roster`) | Started | `ToggleRosterScreenInput` | **UI (PCR-2):** captured-Echo roster — identity/level/bond/top-work per Echo + Bench/Deploy ring management (server-authoritative; replicated mirror for LAN clients) | Ring = MaxPartySize (3); gamepad reaches it via the pause-menu button |
| **M** | World Map (`AWD_Map`) | Started | `ToggleMapScreenInput` | **UI (PCR-3):** world map — 12-zone grid (name/threat/hazard tint), discovered-POI dots, villages, dungeons, active world-event pins, player marker, active-objective line + quest-target POI highlight | Read-only snapshot (reopen to refresh); undiscovered POIs stay hidden; gamepad reaches it via the pause-menu button |
| **Escape** | Pause (`AWD_Pause`) | Started | `TogglePauseMenuInput` | **UI (final run):** pause menu — Resume / Save Now / Quit To Desktop (world paused) | Also on gamepad Start |
| **F5** | Quick Save (`AWD_Save`) | Started | `QuickSave` | Save system → `ASTRAWILD_Main` (schema v3) | Server/authority only |
| **F9** | Quick Load (`AWD_Load`) | Started | `QuickLoad` | Save system → restores world (work sites, battery, drones, robots included) | Server/authority only |
| *(LMB while placing)* | — (routes from Attack) | Started | `BuildingComponent::ConfirmPlacement` | Building: consume materials + server RPC | See LMB row |

Death disables input until respawn (5 s GameMode timer). Building-cycling between piece types
(`CycleBuildingDefinition`) is bound to the mouse wheel (`AWD_BuildCycle`, Axis1D).

---

## 1b. Gamepad Companion Mapping (final production run — M9/H-13)

`BuildGamepadInputDefaults` builds a second IMC (`AWD_GamepadIMC`) sharing the SAME action
objects — gamepad and KB/M work simultaneously, zero configuration needed.

| Gamepad input | Action | Notes |
|---|---|---|
| Left stick (`Gamepad_Left2D`) | Move | No modifiers |
| Right stick (`Gamepad_Right2D`) | Look | Stick up = look up (no negation, unlike mouse Y) |
| Face Button Bottom (A/cross) | Jump | |
| Face Button Right (B/circle) | Interact | |
| Face Button Left (X/square) | Dodge | |
| Face Button Top (Y/triangle) | Build Mode | |
| Right Shoulder | Sprint | |
| Left Shoulder | Block (hold) | |
| Right Trigger | Light Attack / Pulse Lance | Same auto-routing as LMB |
| Left Trigger | Heavy Attack | |
| D-pad Up | Party Command | |
| D-pad Right | Feed | |
| D-pad Down | Smart Consume | |
| D-pad Left | Equip Best | |
| Special Left (select) | Rotate Building | |
| Special Right (start) | Pause Menu | The natural pause button |

*(Scan/drone/robot/save/load remain keyboard-only on gamepad — the four face buttons and D-pad are
fully allocated; a future radial menu can host them.)*

---

## 2. Action → System Cross-Reference

| System | Keys involved |
|---|---|
| Movement / camera | W A S D, mouse, Shift, Space |
| Combat | LMB, RMB, F, Q |
| Interaction (harvest/craft/rest/talk) | E |
| Capture | E (on wild Echo) + R (feed, trust path) |
| Echo commands / party | C |
| Building | B, N, mouse wheel, LMB (place), **Z (delete — wave 4)** |
| Survival (smart consume) | G |
| Equipment (equip best) | X |
| Economy (vendor buy/sell) | — console only: `AW.BuyItem` / `AW.SellItem` while within 6 m of Trader Tam (no keybind — shop UMG screen is a future round) |
| Save / Load | F5, F9 |

---

## 3. Console Cheat Reference (`UAstrawildCheatManager`)

Active in non-Shipping builds (engine strips CheatManagers in Shipping). Open the console with `~` and
prefix commands with `AW.`. All IDs are the registry ids from the Asset Manifest. **15 commands.**

| Command | Arguments | Effect |
|---|---|---|
| `AW.SpawnEcho` | `<EchoDefinitionId>` | Spawns that Echo 400 cm in front of the player (e.g. `AW.SpawnEcho Echo_Lumewisp`) |
| `AW.GiveItem` | `<ItemId> [Quantity]` | Adds items to the player inventory (e.g. `AW.GiveItem Item_Wood 50`) |
| `AW.BuyItem` | `<ItemId> [Quantity]` | **Batch 4 (M-11):** buys a ware from the nearest vendor NPC within 6 m in its currency (Dawn Shards at Trader Tam) — e.g. `AW.BuyItem Item_Bandage 2`. Server-authoritative: validates vendor → 450 cm trade range → ware membership + price → funds → weight before transferring anything (no partial transactions); result toast via `PlayerController::Notify` |
| `AW.SellItem` | `<ItemId> [Quantity]` | **Batch 4 (M-11):** sells a priced item to the nearest vendor for half its buy price (floor 1) per unit — e.g. `AW.SellItem Item_Berry 5`. Junk (`VendorPrice 0`) and the currency itself are not sellable (no arbitrage) |
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


## Gameplay Depth Pack additions (v3.4)

| Key | Gamepad | Action | Since |
|-----|---------|--------|-------|
| T | Right stick click | Party ability cast — every owned Echo casts its best ready ability (heal when hurt, offense otherwise) | GDP-1 |
| Y | — (radial menu pass owns gamepad smart-cast) | Player smart-cast — priority ladder picks the best ready unlocked skill (SecondWind > Whirlwind > PowerStrike > HuntersFocus > Dash > Overcharge) | GDP-3 |
