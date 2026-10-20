# ASTRAWILD COVERAGE REPORT — Phase L1 Truth Reconciliation

> Directive: LONG-RUN DIRECTIVE L1. This report reconciles the repo's claimed
> content census against (a) the new traced extractor
> (`Design/design_data.json`, built in the P0 session) and (b) independent
> grep/AST counts re-run live at HEAD for this report.
>
> HEAD at audit time: `459a561` (branch `final-completion`).

---

## 0. Headline verdict

**The repo documentation is NOT overclaiming. The new extractor is the incomplete side.**

The pre-existing repo validator `Scripts/validate_final_run.py` §11 has been
enforcing a 15-metric content census for the whole final-completion campaign,
and every one of those numbers re-proves true under independent counting at
`459a561` (output pasted in §2). The discrepancy the directive flagged —
"extraction reported 204 species / 49 items / 32 recipes" vs claimed
"229 / 78 / 58" — exists because the new `Design/design_data.json` extractor
was scoped to only two of the three content-authoring files:

| Content file | Role | Covered by new extractor? |
|---|---|---|
| `Source/AstrawildCore/Private/AstrawildBestiaryData.cpp` | 204-row generated bestiary | YES (pass B) |
| `Source/AstrawildCore/Private/AstrawildContentLibrary.cpp` (CL) | legacy base content | YES (passes C1/C2) |
| `Source/AstrawildCore/Private/AstrawildProductionContent.cpp` (PC, 3452 lines) | the production content pack | **NO — entirely missed** |

PC carries 29 items, 26 recipes, 9 hero species, 6 evolution targets, 7 techs,
6 loot tables, 16 world events, 17 POIs, 12 quests, 13 dialogue trees, 8 work
sites, 10 resource nodes, 8 weapons, 3 robots — every one registered through
internal-registering `Make*` helpers whose `Registry->RegisterX(` line appears
only ONCE (inside the helper body), which is why naive call-site counting
misses them.

Brutal-honesty note: this is the second census system in the repo. The new
`Design/` layer duplicated counting work the old validator already did
correctly, without first reading how PC registers content. L2 will fix the
extractor and make `Design/` the single traced superset.

---

## 1. TRUTH TABLE

`extracted` = what `Design/design_data.json` (schema `astrawild-design-data/1`)
contained at the start of this phase. `actual` = independent count re-run live
for this report (commands + output in §2). `claimed` = the repo's authoritative
census (`Scripts/validate_final_run.py` `EXPECTED_CENSUS`, mirrored in
`Docs/ASTRAWILD_MASTER_CONTROL.md` §census and `README.md`).

| # | Category | claimed | extracted | actual in source | verdict |
|---|---|---|---|---|---|
| 1 | species (Echo) | 229 | 204 | **229** | **EXTRACTOR_INCOMPLETE** |
| 2 | items | 78 | 49 | **78** | **EXTRACTOR_INCOMPLETE** |
| 3 | recipes | 58 | 32 | **58** | **EXTRACTOR_INCOMPLETE** |
| 4 | buildings | 26 | 0 | **26** | **EXTRACTOR_INCOMPLETE** (no domain) |
| 5 | quests | 22 | 0 | **22** | **EXTRACTOR_INCOMPLETE** (no domain) |
| 6 | technologies | 17 | 0 | **17** | **EXTRACTOR_INCOMPLETE** (no domain) |
| 7 | NPCs | 13 | 0 | **13** | **EXTRACTOR_INCOMPLETE** (no domain) |
| 8 | dungeons | 3 | 0 | **3** | **EXTRACTOR_INCOMPLETE** (no domain) |
| 9 | bosses | 4 | 0 | **4** | **EXTRACTOR_INCOMPLETE** (no domain) |
| 10 | zones | 12 | 12 (via bestiary aggregates) | **12** | **MATCHED** |
| 11 | weather profiles | no doc claim | 0 | **8** | **EXTRACTOR_INCOMPLETE** (no domain) |
| 12 | world events | 16 | 0 | **16** | **EXTRACTOR_INCOMPLETE** (no domain) |
| 13 | hunt contracts | no doc claim | 0 | **8** | **EXTRACTOR_INCOMPLETE** (no domain) |
| 14 | abilities | no doc claim | 0 | **53** | **EXTRACTOR_INCOMPLETE** (no domain) |
| 15 | mutations | 204 rows (v9.0/v9.1 record) | 0 | **204** | **EXTRACTOR_INCOMPLETE** (no domain) |
| 16 | loot tables | 11 | 0 | **11** | **EXTRACTOR_INCOMPLETE** (no domain) |
| 17 | save fields | no doc claim | 0 | **29 top-level + 20 record structs** | **EXTRACTOR_INCOMPLETE** (no domain) |

Bonus categories the directive did not list but the census gates (audited here
so L2 can domain-ify them too) — all **MATCHED claim, EXTRACTOR absent**:

| # | Category | claimed | extracted | actual | verdict |
|---|---|---|---|---|---|
| B1 | weapons | 8 | 0 | **8** | EXTRACTOR_INCOMPLETE (no domain) |
| B2 | resource nodes | 10 | 0 | **10** | EXTRACTOR_INCOMPLETE (no domain) |
| B3 | work sites | 8 | 0 | **8** | EXTRACTOR_INCOMPLETE (no domain) |
| B4 | POIs | 17 | 0 | **17** | EXTRACTOR_INCOMPLETE (no domain) |
| B5 | dialogue trees | 13 | 0 | **13** | EXTRACTOR_INCOMPLETE (no domain) |
| B6 | robots | 3 | 0 | **3** | EXTRACTOR_INCOMPLETE (no domain) |
| B7 | automation tests | 134 | 134 | **134** | **MATCHED** |
| B8 | biomes | (no claim found) | 0 | 1 (`RegisterBiome` in PC) | no doc claim; EXTRACTOR absent |

**DOC_OVERCLAIM rows: 0 (zero). SOURCE_INCOMPLETE rows: 0 (zero) for the
census categories — every claimed number is backed by real registration code
in Source/.**

The one soft nuance: species "229" is a *composition* claim (19 authored +
6 evolution targets + 204 bestiary) — the composition itself re-proves exactly
(see §2.1), so it is a MATCHED claim, not an overclaim.

---

## 2. Evidence — real commands, real outputs

All commands run at HEAD `459a561` on branch `final-completion`. Outputs are
truncated only where marked `...`; numbers are verbatim.

### 2.1 Species — 229 actual (204 + 19 + 6)

```
$ rg -c 'TEXT\("Echo_' Source/AstrawildCore/Private/AstrawildBestiaryData.cpp
204
$ rg -n 'RegisterEcho\(' Source/AstrawildCore/Private/AstrawildContentLibrary.cpp | wc -l
10            # Lumewisp Stonehide Voltling Duskmoth Gloomfang Sprigling
              # Emberfang Rimefang Voltmaw Auroraling (variable-style authored)
$ rg -n 'MakeProductionEcho\(Registry' Source/AstrawildCore/Private/AstrawildProductionContent.cpp | wc -l
9             # Terraquill Cindermule Voltpylon Bastionbeetle Mistmender
              # Deepdelver GlassTyrant EyeSentinel DrownedSovereign
$ rg -n 'EvolveToDefinitionId|TargetId' Source/AstrawildCore/Private/AstrawildProductionContent.cpp
1419:    const FEvolutionSpec Specs[] = {     # 6 rows:
              # TerraquillVerdant CindermulePyre VoltpylonTempest
              # BastionbeetleBulwark MistmenderRime DeepdelverAbyssal
```
204 + 10 + 9 + 6 = **229**. The composition claim in
`Docs/ASTRAWILD_MASTER_CONTROL.md:129` ("229 Echo species (19 authored + 6
evolution targets + 204 bestiary-generated rows)") is exact: 19 authored =
10 CL + 9 PC.

What the extractor missed: CL variable-style `RegisterEcho(Var)` registrations
(the same pattern its item pass already handles for items!) and the entire PC
file (`MakeProductionEcho` helper registers internally at
`AstrawildProductionContent.cpp:150`; evolution loop registers at `:1484`).

### 2.2 Items — 78 actual (49 CL + 29 PC)

```
$ rg -c 'RegisterItem\(' Source/AstrawildCore/Private/AstrawildContentLibrary.cpp
49
$ python3 (collapsed-statement scan, MakeItem(...) id capture):
ContentLibrary: MakeItem calls=49 distinct=49
ProductionContent: items=29 (distinct 29)
TOTAL distinct items (MakeItem): 78
```
Note `rg -c 'MakeItem\(' ProductionContent.cpp` prints 30 because the 30th
match is the helper *definition* line (`AstrawildProductionContent.cpp:23`).
The helper body calls `Registry->RegisterItem(Item)` once
(`AstrawildProductionContent.cpp:35`) — that single line is the only literal
`RegisterItem(` text in PC, which is why call-site grep alone undercounts.

### 2.3 Recipes — 58 actual (32 CL + 26 PC)

```
$ rg -c 'RegisterRecipe\(' Source/AstrawildCore/Private/AstrawildContentLibrary.cpp
32
$ python3 (collapsed-statement scan):
ContentLibrary: MakeRecipe calls=32 distinct=32
ProductionContent: recipes=26 (distinct 26)
TOTAL distinct recipes (MakeRecipe): 58
```
Same helper-internal-registration pattern (`MakeRecipe` def at
`AstrawildProductionContent.cpp:62`, registers at `:76`).

### 2.4 Buildings / NPCs / quests / techs / loot (CL side, direct counts)

```
$ rg -o 'Registry->RegisterBuilding\(' Source/AstrawildCore/Private/AstrawildContentLibrary.cpp | wc -l
26
$ rg -o 'NPC_[A-Za-z]+' Source/AstrawildCore/Private/AstrawildContentLibrary.cpp | sort -u | wc -l
13            # BlacksmithBorin ElderRowan FarmerJori FisherNima GuardBram
              # GuardSela HerbalistWren Ione OldSaltPerry SkiffWardenKael
              # VendorTam Vess WardenMaren
$ rg -o 'Quest_[A-Za-z0-9]+' CL.cpp PC.cpp (combined, sort -u, minus test ids)
22            # 10 CL RegisterQuest + 12 PC RegisterQuest
$ MakeTech calls in PC: 8 rg lines - 1 definition = 7; CL RegisterTechnology = 10 → 17
$ MakeLoot calls in PC: 7 rg lines - 1 definition = 6; CL RegisterLootTable = 5 → 11
```

### 2.5 The repo's own census gate — re-run live (authoritative claim source)

```
$ python3 Scripts/validate_final_run.py
[PASS] Census items == 78
[PASS] Census recipes == 58
[PASS] Census species == 229
[PASS] Census buildings == 26
[PASS] Census techs == 17
[PASS] Census quests == 22
[PASS] Census loot_tables == 11
[PASS] Census npcs == 13
[PASS] Census weapons == 8
[PASS] Census resource_nodes == 10
[PASS] Census work_sites == 8
[PASS] Census world_events == 16
[PASS] Census pois == 17
[PASS] Census dialogue_trees == 13
[PASS] Census robots == 3
FINAL RUN VALIDATION: ALL CHECKS PASSED (static level — engine build/test
still required on the target machine)
```

### 2.6 Dungeons — 3 actual

```
$ rg -n 'DungeonId = TEXT\(' Source/AstrawildCore/Private/AstrawildWorldBootstrapper.cpp
838:        Dungeon->DungeonId = TEXT("Dungeon_HollowUnderlight");
873:        Vault->DungeonId = TEXT("Dungeon_SunkenVault");
953:        EyeDungeon->DungeonId = TEXT("Dungeon_EyeOfTheMaelstrom");
```

### 2.7 Bosses — 4 actual

```
$ rg -n 'BossDefinitionId = TEXT\(' Source/AstrawildCore/Private/AstrawildWorldBootstrapper.cpp
840:        Dungeon->BossDefinitionId = TEXT("Echo_Gloomfang");        # Hollow Underlight warden
875:        Vault->BossDefinitionId = TEXT("Echo_Dawnfang");           # Sunken Vault colossus
955:        EyeDungeon->BossDefinitionId = TEXT("Echo_DrownedSovereign");
$ rg -n 'BossSpeciesId = TEXT\(' Source/AstrawildCore/Private/AstrawildWorldBootstrapper.cpp
937:            Tyrant->BossSpeciesId = TEXT("Echo_GlassTyrant");      # Act 3 world boss
```
3 dungeon bosses + 1 world boss = **4**, exactly as claimed.

### 2.8 Zones — 12 actual

```
$ rg -c 'Zones\.Add\(MakeZone\(' Source/AstrawildCore/Private/AstrawildZoneSubsystem.cpp
12
```
The new extractor's bestiary aggregates independently derive 12 home zones
(validator check `agg/zones sum` passes), so zones are MATCHED.

### 2.9 Weather profiles — 8 actual (no doc claim found)

```
$ sed -n '89,100p' Source/AstrawildCore/Private/AstrawildWeatherSubsystem.cpp
static const TMap<EAstrawildWeatherState, FAstrawildWeatherProfile> Profiles = {
    Clear, Cloudy, Rain, HeavyRain, Storm, Fog, Heat, Cold   # 8 entries
};
$ rg -n 'EAstrawildWeatherState : uint8' Source/AstrawildCore/Public/AstrawildTypes.h
180:  (8 enum entries, same 8)
```

### 2.10 World events / POIs / nodes / sites / robots / weapons (PC helpers)

```
$ rg -c 'MakeWorldEvent\(' PC.cpp → 17 lines − 1 definition = 16 calls  ✓
$ rg -c 'MakePOI\('        PC.cpp → 18 lines − 1 definition = 17 calls  ✓
$ rg -c 'MakeNode\('       PC.cpp → 11 lines − 1 definition = 10 calls  ✓
$ rg -c 'RegisterWorkSite\(' PC.cpp → 8 (direct variable-style)         ✓
$ rg -c 'RegisterRobot\('    PC.cpp → 3                                 ✓
$ rg -c 'MakeWeapon\('     PC.cpp →  9 lines − 1 definition =  8 calls  ✓
```

### 2.11 Hunt contracts — 8 actual (no doc claim found)

```
$ sed -n '20,33p' Source/AstrawildCore/Private/AstrawildHuntSubsystem.cpp
static const TArray<UAstrawildHuntSubsystem::FHuntContract> Table = {
    Hunt_DuskmothCull, Hunt_StonehideCull, Hunt_EmberfangCull, Hunt_RimefangCull,
    Hunt_BrinefinCull, Hunt_SunhideCull, Hunt_VerdantbloomCull, Hunt_MonolithCull
};
constexpr int32 ContractTableSize = 8;
```

### 2.12 Abilities — 53 actual (no doc claim found)

```
$ rg -c 'Table\.Add\(TEXT\("Ability_' Source/AstrawildCore/Private/AstrawildAbilityLibrary.cpp
53
```

### 2.13 Mutations — 204 actual (matches the v9.0/v9.1 record)

```
$ rg -c 'TEXT\("Echo_' Source/AstrawildCore/Private/AstrawildEchoMutationData.cpp
204
```

### 2.14 Save fields — 29 top-level + 20 record structs (no doc claim found)

```
$ python3 (UPROPERTY field scan of class UAstrawildSaveGame):
UAstrawildSaveGame UPROPERTY fields: 29
  SavedAtUtc PlayerInventory EchoRoster RestPoints ActiveRestPointId WorldState
  PlayerSurvival PlayerTransform EchoRosterV2 Buildings Research Quests Journal
  Dungeons Zones WorkSites Drones Robots PowerGrid WorldEvents DiscoveredPOIIds
  DialogueFlags Attributes NPCAffinities DefeatedCreatureCounts
  EquipmentDurability FoodFreshness CoopPlayers Hunts
$ python3 (USTRUCT scan, save-family): 20 record struct types, all in
  AstrawildTypes.h (FAstrawild*SaveData family + EchoInstanceV2 +
  JournalEntry + HuntSaveRow + ItemStack ...)
```

### 2.15 Automation tests — 134 (MATCHED by both systems)

```
$ rg -c 'IMPLEMENT_SIMPLE_AUTOMATION_TEST' Source/AstrawildCore/Private/AstrawildAutomationTests.cpp
134
```

---

## 3. What the new extractor must learn (input for Phase L2)

1. **File scope**: add `AstrawildProductionContent.cpp` (3452 lines) and the
   CL variable-style `RegisterEcho` pattern; also `AstrawildWorldBootstrapper.cpp`
   (dungeons/bosses), `AstrawildZoneSubsystem.cpp` (zones), `AstrawildWeatherSubsystem.cpp`
   (profiles), `AstrawildHuntSubsystem.cpp` (contracts), `AstrawildAbilityLibrary.cpp`
   (abilities), `AstrawildEchoMutationData.cpp` (mutations), `AstrawildSaveSubsystem.h`
   (save schema).
2. **Helper-internal registration**: PC's `MakeItem`/`MakeRecipe`/`MakeWeapon`/
   `MakeTech`/`MakeLoot`/`MakeWorldEvent`/`MakePOI`/`MakeNode`/`MakeProductionEcho`
   register INSIDE the helper — the call-site `MakeX(Registry, TEXT("Id"), ...)`
   is the registration evidence, not `RegisterX(`.
3. **rg line-count trap**: every `rg -c 'MakeX\('` includes the helper
   definition line itself — subtract 1 (verified per helper above).
4. **Evolution targets** are born from the `FEvolutionSpec Specs[]` table at
   `AstrawildProductionContent.cpp:1419` — 6 ids with level/bond gates, each
   registered via the loop at `:1484`.
5. **Bosses** are identified by `BossDefinitionId`/`BossSpeciesId` assignments
   in the bootstrapper, not by any registry call.
6. New domains to create in `Design/`: buildings, quests, technologies, npcs,
   dungeons, bosses, weather, world_events, hunt_contracts, abilities,
   mutations, loot_tables, save_schema, weapons, resource_nodes, work_sites,
   pois, dialogue_trees, robots (per the L2 directive list).

## 4. Honest bottom line

The repo's documentation census has been *under* attack twice in its history
(v9.2 and v9.5 truth passes) and it holds: **15/15 census metrics + dungeons +
bosses + zones all re-prove at source.** The genuinely weak artifact is the
brand-new `Design/design_data.json` (3 days old, one session of work): it
covered 204/229 species, 49/78 items, 32/58 recipes and 0 of 13 further
domains, while presenting itself as "every authored design value extracted
from C++ source" in `Design/README.md` — an overclaim by THAT file, which L2
must now either make true or soften. The older `validate_final_run.py` census
gates remain the correct reference; L2's job is to bring the traced layer up
to full coverage and then cross-check it against those same gates.
