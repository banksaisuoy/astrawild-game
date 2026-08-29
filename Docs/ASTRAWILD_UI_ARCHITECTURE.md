# ASTRAWILD — UI Architecture

**Status: IMPLEMENTED IN C++ (compile validation pending on target machine)**
**Date: 2026-08-29**
**Primary sources:** `AstrawildHudWidget.h/.cpp`, `AstrawildPlayerController.cpp`,
`AstrawildPlayerCharacter.cpp` (BuildRuntimeInputDefaults / SetupPlayerInputComponent)

The V2 UI strategy is **zero UMG assets**: the HUD builds its entire widget tree in C++, and the complete
default Enhanced Input mapping (actions + keys + modifiers) is constructed at runtime when no editor input
assets are assigned. This keeps the project playable straight from compile with an empty `Content/` folder.

---

## 1. Pure-C++ HUD — `UAstrawildHudWidget`

- Created by `AAstrawildPlayerController::BeginPlay` for **local controllers only**
  (`CreateWidget<UAstrawildHudWidget>`) and added to the viewport. `HudWidgetClass` (soft class) allows a
  future UMG subclass to replace it without touching the controller.
- `NativeConstruct` → `BuildWidgetTree()`: a `UCanvasPanel` root with progress bars and text blocks built
  through `WidgetTree->ConstructWidget`. Font: engine `Roboto-Regular` (22/16/15/14 pt variants). Text
  blocks get a 1.5 px black shadow (85 % opacity) for readability.
- `NativeTick`: refresh accumulator — **state refresh every 0.15 s** (≈6.7 Hz); notification timer counts
  down independently.

### 1.1 Widget tree & layout (actual anchors/sizes from code)

| Widget | Anchor (min/max) | Offset | Size | Color | Content source |
|---|---|---|---|---|---|
| `HealthBar` | (0.02, 0.86) | 0,0 | 280×18 | red (0.85, 0.16, 0.12) | `Survival->GetHealthFraction()` |
| `StaminaBar` | (0.02, 0.895) | 0,0 | 240×12 | amber (0.95, 0.75, 0.10) | `Survival->GetStaminaFraction()` |
| `HungerBar` | (0.02, 0.925) | 0,0 | 200×10 | orange (0.90, 0.45, 0.10) | `Stats.Hunger / 100` |
| `ThirstBar` | (0.02, 0.95) | 0,0 | 200×10 | teal (0.10, 0.70, 0.65) | `Stats.Thirst / 100` |
| `TimeText` | (0.5, 0.02) | −110, 0 | 220×26 | white, 22 pt | `"Day %d  HH:MM"` from GameState |
| `WeatherText` | (0.5, 0.055) | −110, 0 | 220×20 | pale blue, 14 pt | weather display name + temperature (hard-coded 20 °C base display — see gap list) |
| `QuestText` | (0.02, 0.02) | 0,0 | 360×110 (auto-wrap) | gold, 14 pt | active objectives `"[ ] text (progress/required)"` |
| `PromptText` | (0.5, 0.82) | −200, 0 | 400×22 | white, 16 pt | interactable prompt or `"Capture Echo [E] — needs Resonator"` |
| `CaptureText` | (0.5, 0.855) | −200, 0 | 400×22 | green, 15 pt | `"Capture chance: NN%"` (`PreviewCaptureChance` × 100) |
| `CommandText` | (0.98, 0.93) | −300, 0 | 300×20 | blue, 14 pt | `"Party command [C]: <command>"` |
| `NotificationText` | (0.5, 0.14) | −320, 0 | 640×24 | cream, 15 pt | `PushNotification(msg)` — 4 s timeout |

### 1.2 Data flow

HUD is a **read-only presentation layer**: it reads replicated state (survival stats, GameState time/
weather), queries public APIs (`FindInteractableActor`, `PreviewCaptureChance`, `GetActiveObjectives`,
`CurrentPartyCommand`) and never mutates gameplay state. Refresh work per cycle is trivial (a handful of
`SetPercent`/`SetText` calls + one camera raycast for the prompt).

### 1.3 Known cosmetic gaps

- Weather temperature label prints a literal `20` (°C) rather than the survival component's felt
  temperature — cosmetic bug, noted for the first polish pass.
- No icons, no crosshair, no damage numbers, no menu screens (main menu comes with the real map — Roadmap M7).

---

## 2. Runtime Enhanced Input Construction

`AAstrawildPlayerCharacter::BuildRuntimeInputDefaults()` runs in `BeginPlay` **only when no editor IMC is
assigned** (zero-asset playability). It creates:

- **15 runtime input actions** (UInputAction objects, named `AWD_*`, kept GC-rooted in `RuntimeActions`):
  Move (Axis2D), Look (Axis2D), Sprint, Jump, Interact, LightAttack, HeavyAttack, Dodge, Block, Command,
  Feed, BuildMode, BuildRotate, Save, Load (all Boolean).
  *(The code's log line says "16 actions" — cosmetic off-by-one in the log string; 15 actions are created.)*
- One `UInputMappingContext` (`AWD_DefaultIMC`) with key mappings:

| Key | Action | Modifiers |
|---|---|---|
| W / S | Move | Y +1 / −1 (Negate-Y) |
| A / D | Move | X −1 / +1 (Negate-X) |
| Mouse2D (delta) | Look | Y negated for standard pitch |
| Left Shift | Sprint | — |
| Space | Jump | — |
| E | Interact | — |
| Left Mouse Button | LightAttack (or confirm placement while building) | — |
| Right Mouse Button | Block | — |
| F | HeavyAttack | — |
| Q | Dodge | — |
| C | Command | — |
| R | Feed | — |
| B | BuildMode | — |
| N | BuildRotate (+15°) | — |
| F5 | Save | — |
| F9 | Load | — |

- `SetupPlayerInputComponent` binds every action to handlers (`ETriggerEvent::Started` for one-shots;
  Started/Completed/Canceled for holds like sprint/block).
- If an editor-authored IMC **is** assigned (`DefaultMappingContext`), the runtime build is skipped — the
  project supports both workflows.
- Binding details per action: see `ASTRAWILD_INPUT_REFERENCE.md`.

---

## 3. Future CommonUI / UMG Migration Path

**PLANNED** (not implemented). The migration contract:

1. **Now (v2 foundation):** C++ HUD + runtime IMC — playable with zero assets; all state access through
   public C++ APIs and delegates, so the UI layer is swappable.
2. **Phase 1 (M7, user machine):** keep the C++ HUD; author a real IMC + IA assets in-editor, assign to
   `DefaultMappingContext` (runtime build auto-disables). Author the first map with a proper main menu.
3. **Phase 2 (content alpha):** introduce CommonUI (`CommonPlayerController` / `CommonUIExtension`) —
   `UAstrawildHudWidget`'s state-reading logic moves into ViewModels; screens (inventory, crafting, tech
   tree, journal) subclass the existing widget or replace it via `PlayerController.HudWidgetClass`.
   The crafting screen contract is already fixed (Crafting doc §6).
4. Localization: all strings currently use `FText::FromString` code literals or NSLOCTEXT — the two v1
   prompts (`HarvestPrompt`, `RestPointPrompt`) are already localized; full LOCTEXT tables come with real UI.

---

## 4. Not Implemented (honest)

| Surface | Status |
|---|---|
| Main menu / pause / settings | NOT IMPLEMENTED (PIE direct-to-game) |
| Inventory screen | NOT IMPLEMENTED (weight/stats visible via delegates only) |
| Crafting screen | NOT IMPLEMENTED (station interact stopgap) |
| Tech tree / journal / map screens | NOT IMPLEMENTED |
| Gamepad support | NOT IMPLEMENTED (KB+M default mapping only) |
| Input remapping UI | NOT IMPLEMENTED (runtime IMC is code-defined) |
