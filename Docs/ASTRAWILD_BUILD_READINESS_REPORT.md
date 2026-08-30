# ASTRAWILD — BUILD READINESS REPORT

> Final production run · Commit `249eec7` · Prepared by the GLM 5.3 repo agent.
> **Honest status: SOURCE-COMPLETE, NEVER COMPILED.** The sandbox that produced this
> source has no Unreal Engine — the receiving agent (Antigravity on Windows + UE 5.8)
> performs the first real build. Everything below describes what WILL be built and run.

---

## 1. Project identity

| Item | Value |
|---|---|
| Engine | **Unreal Engine 5.8** (`EngineAssociation`, `IncludeOrderVersion.Unreal5_8`, `BuildSettingsVersion.V5`) |
| Module | `AstrawildCore` (Runtime, Default loading phase) |
| Targets | `ASTRAWILD.Target.cs` (Game) + `ASTRAWILDEditor.Target.cs` (Editor) |
| Plugins | `EnhancedInput`, `ProceduralMeshComponent`, `GameplayTags`, `GameplayAbilities` (unused, kept), `GameplayTasks` (unused), `StateTree` (unused) |
| Build.cs deps | Core, CoreUObject, Engine, InputCore, EnhancedInput, GameplayAbilities, GameplayTags, GameplayTasks, AIModule, NavigationSystem, UMG, ProceduralMeshComponent, Slate, SlateCore |
| Scale | 114 C++ files · ~23,700 LOC · 25 automation tests · 56 docs |

## 2. Build steps (first boot)

1. `git clone https://github.com/banksiasuoy/astrawild-game.git` (private — credentials required)
2. Right-click `ASTRAWILD.uproject` → **Generate Visual Studio project files** (or run
   `UnrealBuildTool -projectfiles`).
3. Open the solution → build **`ASTRAWILDEditor` Win64 Development** (or let the editor
   compile on first open).
4. Launch the editor → **Play In Editor**. No maps or assets are required:
   `AstrawildGameMode::BeginPlay` spawns `AAstrawildWorldBootstrapper`, which generates
   the six-zone world (2.4×1.6 km terrain, camp, wildlife, dungeon, portals) at runtime
   from C++ + engine basic shapes.
5. Optional editor-landscape alternative: import `Content/Heightmaps/*.r16` (see
   `Content/Heightmaps/README.md`) — NOT required for play.

Expected first-build risk (from static review): UHT nitpicks on 5.8 policy. Fix-forward
per `Docs/ANTIGRAVITY_BUILD_CHECKLIST.md` Phase B (mechanical fixes allowed, no contract
changes). The two historical hard compile blockers (C-1 raw-pointer `.Get()`, C-2 RPC
`_Implementation` naming) are already fixed at `750f87a`.

## 3. Project settings (Config)

- `GlobalDefaultGameMode = /Script/AstrawildCore.AstrawildGameMode`
- `GameDefaultMap = /Engine/Maps/Entry` (engine built-in; **packaged-build startup is
  verification-queue item V-41** — PIE is unaffected)
- Renderer: Nanite on, TSR, VSM; `bGenerateNavigationOnlyAroundNavigationInvokers=True`
  (pairs with runtime navmesh invokers on the player)
- Save system: code-driven `USaveGame` (schema v3, FNV-1a checksum) — no ini config
- Networking: IpNetDriver declared; single-player/listen-server first (remote-client
  shop/dismantle RPCs documented as the H-9/H-12 MP batch)

## 4. Runtime architecture (what boots, in order)

1. GameMode BeginPlay → spawns WorldBootstrapper → grants starting techs → (optional)
   auto-load latest save → autosave timer (300 s)
2. Bootstrapper: 6 terrain tiles (ProceduralMeshComponent, seam-continuous height field)
   → camp (workbench, campfire, research desk, rest point, 2 work sites with stable
   SiteIds) → wildlife + hostiles per-zone → dungeon + gates + portals → lights
3. PlayerController: builds the C++ HUD; quest component starts `Quest_FirstLight`
4. Systems online: time-of-day, weather, ecosystem (food chain, population tiers),
   zone sweep (Event.ZoneEntered/Left), journal observation, hostile spawner, power grid

## 5. Gameplay loop coverage (23/23 stages — SOURCE_IMPLEMENTED)

NEW GAME (PIE boot) → EXPLORE (6 zones, discovery save) → SURVIVE (vitals, insulation) →
FIND ECHO (10 species + activity patterns) → SCAN (passive observe + hold-V scanner +
drone pulses) → COMBAT (melee + Pulse Lance projectile path) → CAPTURE (resonator,
chance = weaken + trust + journal bonus) → INVENTORY (TAB screen, 6 equipment slots) →
GATHER (nodes + drone auto-harvest) → BUILD BASE (13 pieces, tech-gated) → GENERATE
POWER (grid + battery, persisted) → ASSIGN ECHO (E on site, persisted) → AUTOMATION
(Echo work + utility robots, persisted) → RESEARCH (tree screen at desk, player choice) →
ADVANCED TECHNOLOGY (scanner, helm, exosuit, laser, drone, robot — all craftable, all
wired into real systems) → DUNGEON (5 rooms, gated progression, saved) → BOSS (3 phases,
telegraphed AoE, energy bolts, weak-point windows, arena hazards, HP bar) → REWARD
(loot + Ancient tech force-unlock) → RETURN BASE (exit portal) → SAVE (F5/autosave,
schema v3) → QUIT (ESC pause menu) → LOAD (F9/auto, full re-link incl. work assignments,
battery charge, drones, robots) → VERIFY (25 automation tests — engine-run required)

## 6. Content inventory (CODE_DEFAULT — all generated in memory)

| Kind | Count | Notes |
|---|---|---|
| Items | 35 | incl. 7 advanced-tech items (Pulse Cell, Field Scanner, Resonance Helm, Dawnstrider Exosuit, Pulse Lance, Utility Drone, Utility Robot) |
| Recipes | 26 | 7 gated behind Tech_AdvancedEnergy |
| Technologies | 10 | every era used; Ancient era = dungeon-only |
| Echo species | 10 | 3 hostile; personality/activity/habitat data-driven |
| Buildings | 13 | power roles + work types |
| Quests | 8 | ends with "The Vale Beyond" (VisitZone + SurviveTime + craft payoff) |
| Loot tables / NPCs | 2 / 2 | vendor economy live |

## 7. Known risks (all tracked)

| Risk | Severity | Mitigation |
|---|---|---|
| Never compiled | **Blocker** | Antigravity build = verification queue §1; fix-forward policy |
| Packaged-build Entry-map startup | Medium | V-41 (may need a minimal project map) |
| Runtime navmesh at scale | Medium | Invoker-based generation; V-33 |
| UHT 5.8 policy surprises | Medium | Static review cleaned twice; conservative API surface |
| Remote-client RPCs (shop/dismantle) | Low | SP/listen-server first; H-9/H-12 documented |
| Placeholder visuals (14 `REPLACE_BEFORE_RELEASE`) | Accepted | Vertical-slice doctrine: engine primitives, zero assets |

## 8. Verification hand-off

The complete in-engine test queue lives in **`Docs/ASTRAWILD_ENGINE_VERIFICATION_QUEUE.md`**
(§1 compile gate → §2 23-stage golden path incl. 3× save/load round-trips → §3 system
checks → §4 packaging stretch). Evidence protocol: mark PASS/FAIL per row in
`BUILD_STATUS.md` after every test.

**Verdict: the repository is at PLAYABLE BUILD CANDIDATE status on the source side.
The remaining gate is exclusively the in-engine compile + playtest run.**
