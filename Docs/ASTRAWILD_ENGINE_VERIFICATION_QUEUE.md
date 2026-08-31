# ASTRAWILD — ENGINE VERIFICATION QUEUE

> Everything that **requires a real Unreal Engine 5.8 run** (Antigravity / Windows) to clear.
> The sandbox that produced this source has no UE — compile status is honestly **NOT_RUN**.
> Verifying agent: open `ASTRAWILD.uproject` (UE 5.8) → build `ASTRAWILDEditor Win64 Development`
> → run the queue top-to-bottom. Mark each row PASS / FAIL + evidence in `BUILD_STATUS.md`
> (playtest table). On FAIL: fix-forward (small source fixes allowed), never silently defer.

---

## §1 Compile gate (BLOCKING — everything else waits on this)

| # | Check | Expected | Evidence to record |
|---|-------|----------|--------------------|
| V-1 | UHT pass on `AstrawildCore` (45 headers) | 0 errors; ~45 `.gen.cpp` produced | compiler output tail |
| V-2 | Full `Development Editor` build | 0 errors, 0 warnings-as-errors | build log |
| V-3 | Link `ASTRAWILD.exe` / editor target | links clean (watch for the historical C-2 class of LNK2001) | build log |
| V-4 | PIE boot, zero-asset world | terrain tiles + camp + spawners visible in <30 s | screenshot + log tail |
| V-5 | Automation tests (`ast.``*`, 51 tests) | `&` filter in Session Frontend → all pass, 0 fail | screenshot of test list |

Known "may surface on first compile" items (static review already cleaned these once):
native gameplay tag registration order, `TObjectPtr` BP exposure policy, IMC runtime
construction warnings. If UHT rejects a pattern, prefer the minimal mechanical fix
(e.g. drop `BlueprintReadWrite` from an internal struct field) over restructuring.

---

## §2 Gameplay-loop playtest (the 23-stage golden path)

One continuous PIE session, ~30–45 min. Every stage must work in sequence.

| # | Stage | Steps | PASS criterion |
|---|-------|-------|----------------|
| V-6 | NEW GAME | PIE start | HUD up, zone banner "Dawn Fields", quest 1 active |
| V-7 | EXPLORE | walk 200 m any direction | terrain continuous (no seams/holes), zone sweep fires banner on crossing |
| V-8 | SURVIVE | observe vitals 5 min; eat berry (G) when hungry | hunger/thirst decay; eat restores; temp label changes near water/night |
| V-9 | FIND ECHO | approach wild Lumewisp | it flees/wanders per activity pattern |
| V-10 | SCAN | equip Scanner (if crafted) or just observe; watch HUD journal % | observation accrues; 25 % → "Scanned" research points; active scanner hold-V accelerates ×3 |
| V-11 | COMBAT | attack hostile with LMB/heavy RMB; try dodge (Space double?) & block | damage numbers/log, stagger, elemental status text (Ember→Burn) |
| V-12 | CAPTURE | weaken Emberling <50 %, use Resonator (F) | capture chance shown on HUD, success joins roster + party |
| V-13 | INVENTORY | press TAB | inventory screen lists stacks + weight + equipment; consume works |
| V-14 | GATHER | harvest 3 nodes (E) | items added; node despawns → respawns 30 s |
| V-15 | BUILD BASE | B → place Foundation/Wall/Generator/Battery/Lamp; N rotate; Z dismantle+refund | ghost preview, invalid placement blocked, power icons update |
| V-16 | GENERATE POWER | add Echo Dynamo + Fuel? (Dynamo needs assigned Echo) | grid resolve ≤2 s; lamp lights; battery fills |
| V-17 | ASSIGN ECHO | E on Gathering Hub | nearest idle captured Echo walks to site, begins work anim/log |
| V-18 | AUTOMATION | wait 1–2 outputs; E to collect | output accrues while powered (×1.5), collect adds items, `ItemCollected` quest event |
| V-19 | RESEARCH | interact Research Desk | research screen opens; pick and unlock a tech with points; gated recipes appear |
| V-20 | ADVANCED TECH | craft helmet + exosuit + laser; deploy drone; build Utility Robot | slots reflect on HUD; laser fires projectile consuming energy cells; drone follows + auto-scans; robot works site |
| V-21 | DUNGEON | enter Hollow Approach portal; clear 5 rooms | gates seal/open per room clear; portals teleport; boss room last |
| V-22 | BOSS | fight Underlight Warden | boss HP bar top-center; phases at 66 %/33 %; telegraph ring before AoE (dodge out); weak-point core glows periodically (×2 dmg); hazards spawn phase 2+; enrage 180 s |
| V-23 | REWARD | defeat boss | Loot_DungeonBoss drops + Ancient tech force-unlock + 10 RP |
| V-24 | RETURN BASE | exit portal | teleports to Dawn Camp |
| V-25 | SAVE | F5 | save file written (`AstrawildSaveSlot*`), log line |
| V-26 | QUIT | ESC → Pause → Quit To Desktop (or End PIE) | clean shutdown, no crash/hang |
| V-27 | LOAD | restart PIE, F9 (or auto-load) | world restores: buildings, party, equipment, research, quest, journal, zone discovery, **work assignments + site output + battery charge + rest state** |
| V-28 | VERIFY | diff state vs pre-save | automation resumes without re-assign; battery retains charge; saved drone re-deploys |

**Round-trip stress:** repeat V-25→V-28 at least 3× (create → save → quit → load → verify),
per the production directive PHASE 16.

---

## §3 System-specific checks

| # | System | Check |
|---|--------|-------|
| V-29 | Save schema | old v1/v2 saves (if any exist) migrate without refusal; corrupt file (flip a byte) is refused with checksum error, game continues |
| V-30 | Replication (listen server) | 2-PIE: capture/equip/build/work-site state visible on client; dungeon gate opens on client (C-4 regression) |
| V-31 | Gamepad | plug controller: left stick move, right stick look, A jump, X interact, RB attack, LB capture, DPad commands work |
| V-32 | Echo AI | 10+ Echoes: no stuck actors on terrain seams; perception aggro/flee correct; follow command keeps pace |
| V-33 | Navmesh | runtime generation around invokers — enemies path across zone borders |
| V-34 | Weather/time | `AW.SetWeather cold` (cheat) → felt temp < 4 °C → cold damage ticks (T-4 fix) |
| V-35 | Vendor | buy/sell updates both inventories + currency (listen-server client too) |
| V-36 | Performance | `stat unit` 60 fps target @ 1080p mid GPU; ~30 Echoes + 6 terrain tiles; `stat anim` dominated by nothing pathological |
| V-37 | Console/cheats | `AW.*` cheat manager functions all respond (help listing) |
| V-38 | Terrain determinism | same seed → identical heights (tests cover math; verify visual) + `.r16` import path optional |
| V-39 | Death/respawn | die to hostile → 5 s respawn at PlayerStart with input restored (no stuck movement) |
| V-40 | Boss edge cases | kill during telegraph; die to hazard; leave arena mid-fight (leash/hazard cleanup); adds (Gloomfang ×2) spawn phase 2 |

### Batch 8 — The Grand Expanse + Grand Menagerie (NEW)

| # | Area | Check |
|---|------|-------|
| B8-1 | Bestiary bodies | walk the Vale: wild Echoes render 8 distinct body plans (quadruped/biped/serpent/floating/insectoid/avian/crystalline/amorphous), 5 size scales, per-species tints — not gray spheres |
| B8-2 | 12 zones | visit all 12 zones → HUD banner + discovery n/12; sea zones show the blue water plane; islands poke above water with beach tint |
| B8-3 | Villages | Dawnstead (7 huts + 8 NPCs) around the camp; villagers walk the waypoint circuit; talk → they stop and face you |
| B8-4 | Village nights | wait for 21:00 → all villagers gather at the campfire; 06:00 → resume patrol |
| B8-5 | Village guards | drag a hostile Echo near Dawnstead (cheat or aggro) → Sela/Bram sprint over, attack through the damage pipeline, return to patrol |
| B8-6 | Skiff flight | board [E] at the camp pad → WASD/SPACE/CTRL/SHIFT fly; ceiling ~120 m above ground; hull stops on cliffs; [E] dismount restores walking |
| B8-7 | Sea crossing | fly Dawnstead → Tidebreaker Isles across Azure Shallows → zone banner fires; land at Driftwood Landing (3 huts + dock + 3 NPCs) |
| B8-8 | Quest 9 | Elder Rowan offers "Wings over the Vale" → VisitZone + chart marker both complete → rewards |
| B8-9 | Dungeon #2 | Driftwood vault portal → Sunken Vault: 4 rooms + gates + Dawnfang boss (3 phases, telegraphs, weak point) → defeat publishes Creature_VaultColossus → quest 10 completes |
| B8-10 | New economy | buy from Borin (armory)/Wren (herbalist)/Nima (isles); sell Sea Pearls/Coral; new materials drop from bestiary species |
| B8-11 | Save after Batch 8 | NEW GAME (old saves are stale) → play → save → load round-trip ×3 (schema v3, zone discovery n/12 persists) |
| B8-12 | Perf sanity | 12 terrain tiles + ~70 wildlife + 11 NPCs + 2 villages: `stat unit` target 60 fps mid GPU (tiles ~393k tris total — if below target, lower `TerrainResolution` knob to 96 and re-test) |

| V2-1 | P0 node determinism | harvest several node types → loot ALWAYS matches the definition (wood/stone/fiber/crystal/alloy); rarity shapes differ (cube/sphere/cylinder/cone) | inventory + screenshot |
| V2-2 | Weapon fire modes | Scrapshot (fast bolt), Arc Caster into a pack (chains 4), Lumen Beam (pierce), Magrail (slow line-wipe), Skysinger (homing track) | short clips / log |
| V2-3 | Ammo economy | each energy weapon consumes its ammo per shot; out-of-ammo = dry fire | inventory before/after |
| V2-4 | HUD readouts | weapon line (DMG/interval/AMMO), GRID (+/-/CELL) line, world-event banner appear | screenshot |
| V2-5 | Split insulation | wear Bastion set in Frostveil (no cold damage) then Ember Ridge (no heat damage); legacy pieces still cover both sides | healthbar timelapse |
| V2-6 | Scanner tiers | Array Scanner → observation range visibly longer + hidden alloy veins harvestable; Oracle → POIs discover at double radius + signal sources appear | journal + POI toasts |
| V2-7 | Consume→produce | stage raw meat at Camp Kitchen [E] → output only flows while inputs last; Ridge rig produces only when grid-powered | site output over 2 min |
| V2-8 | World events | first roll ~09:00 day 1; toasts + HUD banner; night raid spawns 3 hostiles near camp after day 3 | log + screenshot |
| V2-9 | POI discovery | 12 beacon pillars; walk within radius → discovery toast + rewards; POI quest objectives tick | quest tracker |
| V2-10 | Robot chassis | deploy Borebot [J] at the Ridge rig → orange light, visibly faster stone output than a general robot | site output compare |
| V2-11 | Echo auras | capture Mistmender → player + party HP slowly rises; Cindermule near → carry limit +20kg | stat watch |
| V2-12 | Save v4 | save mid-event with a drone out + kitchen buffer → quit → load → event/battery/buffer all restored | save/load compare |
| V2-13 | **Biome dressing boot** | log line "Biome dressing placed for 12/12 zones"; Dawn Fields shows trees/rocks/grass within 60m of (but NOT inside) the camp | screenshot from camp + from 100m out |
| V2-14 | Zone dressing identity | Frostveil snow-dusted conifers / Sunscar cacti + sandstone / Ember glass spires / Isles palms — each zone reads differently at first glance | 4 screenshots |
| V2-15 | Atmosphere day cycle | 07:00 warm dawn fog → 12:00 neutral noon → 18:30 ember dusk → 23:00 cool dark night; sun color shifts with it | timelapse clip / 4 screenshots |
| V2-16 | Weather coupling | force Storm → sun visibly dims + fog thickens within ~0.25s; clear → lifts | clip + `stat unit` frame |
| V2-17 | Beam/Arc VFX | Lumen Beam = bright piercing beam to furthest target; Arc Caster into a pack = jagged lightning chains + muzzle flashes on every shot | short clips |
| V2-18 | Scanner pulse + Echo identity | hold V per scanner tier → teal/amber/violet expanding rings (Oracle largest); Rare+ wild Echo wears a rarity ring; captured elemental glows | 3 screenshots |
| V2-19 | VVS perf sanity | dressing (~22k verts/36 sections) + ≤8 active element lights: `stat unit` holds 60 fps mid-GPU at the camp | stat screenshot |
| V2-20 | Player silhouette | third-person look: survivor body (amber chest/teal visor); equip different weapon families → held gun tint/size changes within 0.5s | 2 screenshots |
| V2-21 | **Dialogue screen (Batch 3)** | Talk [E] to Warden Maren → conversation panel (speaker amber, replies teal-filtered); Accept starts First Light; ESC closes; input returns to game-only | short clip |
| V2-22 | Dialogue conditions + flags | after First Light completes, Maren's "Report" choice appears ONCE (grants 15 research) then never again — also after save/load | 2 clips + log |
| V2-23 | Vendor hand-off through dialogue | Tam: "Browse wares" closes conversation → same shop screen opens | clip |
| V2-24 | Chained flags (Perry) | hear Perry's tide story → "Accept the sea pearl" appears → grants Item_SeaPearl once, never again | inventory + clip |
| V2-25 | Evolution end-to-end (CP-02) | level/bond a Voltpylon to gates (cheat: `ASTRAWILD.` cheat manager is live) → `EvolveInstance` swaps to Voltpylon Tempest: roster entry + spawned actor rebuild (bigger, Epic) with level/bond/trust intact | roster + 2 screenshots |
| V2-26 | Weapon Niagara binding dispatch | bind any NS_ on a weapon profile's `MuzzleFlashVfx` (or leave empty) → bound: Niagara plays, procedural octahedron does NOT; empty: octahedron plays as today | 2 clips |
| V2-27 | Dialogue save round-trip | mid-conversation flag set → save → quit → load → flags still gate choices (Maren report hidden if already reported) | log + clip |

---

## §4 Packaging (stretch — after §1–§3 green)

| # | Check |
|---|-------|
| V-41 | `Development Win64` package boots (Entry map policy — may need a minimal project map with the GameMode; engine Entry map behavior in packaged builds is the open question) |
| V-42 | Packaged build runs the golden path §2 end-to-end |

---

## Status legend

- `NOT_RUN` — default for everything (be honest)
- `PASS` / `FAIL` + evidence appended to `BUILD_STATUS.md` playtest table by the verifying agent
- A FAIL on any §1 row blocks all §2–§4 rows (fix-forward first)
