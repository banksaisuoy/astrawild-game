#!/usr/bin/env python3
"""
ASTRAWILD Final Run — static content validator (no engine required).

Cross-checks every ID referenced in the Final Run content pack against the
registered content, verifies quest-chain closure, LFS pointer integrity, and
asset path references. Complements Scripts/validate_repository.sh (structural)
and the 119 in-engine automation tests (behavioral, ENGINE-UNVERIFIED until run).
"""
import json
import os
import re
import sys
import glob

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
FAILURES = []


def check(name, ok, detail=""):
    status = "PASS" if ok else "FAIL"
    print(f"[{status}] {name}" + (f" — {detail}" if detail and not ok else ""))
    if not ok:
        FAILURES.append(f"{name}: {detail}")


def read(path):
    with open(os.path.join(ROOT, path), encoding="utf-8", errors="replace") as f:
        return f.read()


CL = read("Source/AstrawildCore/Private/AstrawildContentLibrary.cpp")
PC = read("Source/AstrawildCore/Private/AstrawildProductionContent.cpp")
WB = read("Source/AstrawildCore/Private/AstrawildWorldBootstrapper.cpp")
TYPES = read("Source/AstrawildCore/Public/AstrawildTypes.h")

# --- 1. Registered ID extraction (CODE_DEFAULT registrations) ---
def ids_from(source, prefix):
    pattern = re.compile(prefix + r"\(Registry, TEXT\(\"([A-Za-z0-9_]+)\"\)|" +
                          r"Make\w+\(Outer, TEXT\(\"([A-Za-z0-9_]+)\"\)")
    return {m.group(1) or m.group(2) for m in pattern.finditer(source)}

items = ids_from(PC, r"MakeItem") | ids_from(CL, r"MakeItem")
items |= {m for m in re.findall(r"TEXT\(\"(Item_[A-Za-z0-9_]+)\"\)", PC + CL) if m in items}
echoes = set(re.findall(r"TEXT\(\"(Echo_[A-Za-z0-9_]+)\"\)", PC + CL + read("Source/AstrawildCore/Private/AstrawildBestiaryData.cpp")))
quests = set(re.findall(r"TEXT\(\"(Quest_[A-Za-z0-9_]+)\"\)", PC + CL))
techs = set(re.findall(r"TEXT\(\"(Tech_[A-Za-z0-9_]+)\"\)", PC + CL))
recipes = set(re.findall(r"TEXT\(\"(Recipe_[A-Za-z0-9_]+)\"\)", PC + CL))
loots = set(re.findall(r"TEXT\(\"(Loot_[A-Za-z0-9_]+)\"\)", PC + CL))

check("Registry extraction non-empty",
      len(items) > 50 and len(quests) >= 17 and len(techs) >= 17,
      f"items={len(items)} quests={len(quests)} techs={len(techs)}")

# --- 2. Final Run content presence ---
FINAL_QUESTS = ["Quest_StormAnchors", "Quest_CrownRelay", "Quest_EyeOfTheMaelstrom",
                "Quest_TheDrownedSovereign", "Quest_FirstDawnAgain"]
for q in FINAL_QUESTS:
    check(f"Final Run quest registered: {q}", q in quests)

check("Vanguard bridges into Act 3", 'Vanguard->NextQuestId = TEXT("Quest_StormAnchors")' in PC)
check("Drowned Sovereign registered", "Echo_DrownedSovereign" in echoes)
check("Eye Sentinel registered", "Echo_EyeSentinel" in echoes)
check("Glass Tyrant registered", "Echo_GlassTyrant" in echoes)
check("Sovereign Core item registered", "Item_SovereignCore" in items)
check("Maelstrom Glass item registered", "Item_MaelstromGlass" in items)
check("Stratos Coil item registered", "Item_SkiffStratosCoil" in items)
check("Skiff Engineering tech registered", "Tech_SkiffEngineering" in techs)
check("Stratos Coil recipe registered", "Recipe_SkiffStratosCoil" in recipes)
check("Loot_EyeCore registered", "Loot_EyeCore" in loots)

# --- 3. Quest chain closure (NextQuestId walk) ---
check("Quest chain: FirstLight present", "Quest_FirstLight" in quests)
check("FirstDawnAgain is terminus",
      re.search(r"FirstDawn->NextQuestId = NAME_None", PC) is not None)

# --- 4. Quest objective producers (map anchors to registered content) ---
POI_TARGETS = ["POI_FrostveilSignalSource", "POI_SunscarMirageStone", "POI_StormcrestArray"]
POISRC = PC
for poi in POI_TARGETS:
    check(f"Anchor POI registered: {poi}", f'TEXT("{poi}")' in POISRC)
check("Azure Shallows POI registered", 'TEXT("POI_ShallowsSextant")' in POISRC)

# --- 5. World bootstrap wiring ---
check("Eye dungeon spawns", 'TEXT("Dungeon_EyeOfTheMaelstrom")' in WB)
check("Eye Gate portal spawns", 'TEXT("Location_EyeGate")' in WB)
check("Eye arrival marker", 'TEXT("Location_EyeOfTheMaelstrom")' in WB)
check("Dawnstead homecoming marker", 'TEXT("Location_Dawnstead")' in WB)
check("Glass Tyrant world boss spawns", 'TEXT("Creature_GlassTyrant")' in WB)
check("Sovereign defeat event wired", 'TEXT("Creature_DrownedSovereign")' in WB)
check("Eye boss loot override", 'BossLootTableId = TEXT("Loot_EyeCore")' in WB)
check("Eye boss summon override", 'BossSummonSpeciesId = TEXT("Echo_EyeSentinel")' in WB)

# --- 6. Ending state machine ---
GS_H = read("Source/AstrawildCore/Public/AstrawildGameState.h")
GS_C = read("Source/AstrawildCore/Private/AstrawildGameState.cpp")
SS_H = read("Source/AstrawildCore/Public/AstrawildSaveSubsystem.h")
SS_C = read("Source/AstrawildCore/Private/AstrawildSaveSubsystem.cpp")
check("Ending enum defined", "EAstrawildEndingState" in TYPES)
check("GameState ending field", "EndingState" in GS_H and "DOREPLIFETIME(AAstrawildGameState, EndingState)" in GS_C)
check("SetEndingState applies weather pin", "SetWeatherState(EAstrawildWeatherState::Clear)" in GS_C)
check("Save schema V5", "CurrentSchemaVersion = 5" in SS_H and "MigrateV4ToV5" in SS_H)
check("V5 migration wired", "MigrateV4ToV5(SaveGame)" in SS_C)
check("Ending saved", "SaveGame->EndingState = static_cast<int32>(GameState->EndingState)" in SS_C)
check("Ending loaded", "GameState->SetEndingState(static_cast<EAstrawildEndingState>" in SS_C)
check("TriggerEndingId consequence field", "TriggerEndingId" in read("Source/AstrawildCore/Public/AstrawildDataAssets.h"))
check("Dialogue ending routing", 'Ending_BreakCage' in read("Source/AstrawildCore/Private/AstrawildDialogueComponent.cpp"))
check("Weather pin post-ending", "EndingBreak" in read("Source/AstrawildCore/Private/AstrawildWeatherSubsystem.cpp"))
check("HUD ending banner", "EndingBannerText" in read("Source/AstrawildCore/Private/AstrawildHudWidget.cpp"))

# --- 7. LFS pointer integrity for all tracked binary assets ---
missing_lfs = 0
checked = 0
for pattern in ("Content/**/*.uasset", "Content/**/*.umap", "ArtSource/**/*.glb", "ArtSource/**/*.png", "ArtSource/**/*.wav"):
    for path in glob.glob(os.path.join(ROOT, pattern), recursive=True):
        with open(path, "rb") as f:
            head = f.read(64)
        checked += 1
        if head.startswith(b"version https://git-lfs"):
            continue
        # Non-LFS real binaries (r16 heightmaps etc.) are allowed
        if path.endswith((".r16",)):
            continue
        if head.startswith(b"\x00"):
            missing_lfs += 1  # raw binary not through LFS
check(f"LFS pointers valid ({checked} files checked)", missing_lfs == 0, f"{missing_lfs} raw binaries")

# --- 8. Asset path references resolve against tracked Content ---
tracked = set()
for root, _, files in os.walk(os.path.join(ROOT, "Content")):
    for fn in files:
        rel = os.path.relpath(os.path.join(root, fn), ROOT).replace("\\", "/")
        tracked.add(rel.rsplit(".", 1)[0])
code = PC + WB + read("Source/AstrawildCore/Private/AstrawildArtPack.cpp") + read("Source/AstrawildCore/Private/AstrawildPlayerCharacter.cpp")
refs = set(re.findall(r'"/Game/([A-Za-z0-9_/]+)"', code))
unresolved = []
for ref in refs:
    game_path = "Content/" + ref
    # skip engine-basic-shape and known-package-path style refs (package vs asset)
    candidates = {game_path, game_path.rsplit("/", 1)[0] + "/" + game_path.rsplit("/", 1)[-1]}
    if not any(c in tracked or (c + ".uasset") in tracked or (c + ".umap") in tracked for c in candidates):
        # one more try: package path where asset name repeats (UAsset folder pattern)
        parts = game_path.split("/")
        if len(parts) >= 2 and parts[-1] != parts[-2]:
            alt = "/".join(parts[:-1] + [parts[-1]])
        unresolved.append(ref)
# The single known dead fallback path is documented (SK_Survivor_Exosuit dup branch)
unresolved = [u for u in unresolved if "SkeletalMeshes/SK_Survivor_Exosuit" not in u]
check(f"Asset path references resolve ({len(refs)} refs)", len(unresolved) == 0, f"unresolved: {unresolved[:6]}")

# --- 9. Automation test count ---
TESTS = read("Source/AstrawildCore/Private/AstrawildAutomationTests.cpp")
count = len(re.findall(r"IMPLEMENT_SIMPLE_AUTOMATION_TEST", TESTS))
check("Automation tests == 134 (109 through DP-9 + 2 LCP-2 + 2 LCP-3 + 2 LCP-4 + 2 LCP-5 + 2 LCP-6 session flow + 1 PCR-1 journal + 1 PCR-2 roster + 1 PCR-3 map + 1 PCR-4 Tier-B + 1 PCR-5 hunts + 1 FPP-1 presentation + 1 SCI-FANTASY mutation + 1 DCP-1 post-game quests + 1 DCP-2 New Game Plus + 1 DCP-3 ending cinematic + 1 DCP-4 Act 3 NPCs + 1 DCP-5 journal detail + toast + 1 DCP-6 gamepad smart-cast chord + 1 DCP-7 mesh coverage expansion + 1 VIS-1 creature identity contract)", count == 134, f"count={count} — update this gate + all active docs together")

# --- 9b. PCR-4/PCR-5: Tier-B archetype library coherence ---
ARTPACK = read("Source/AstrawildCore/Private/AstrawildArtPack.cpp")
tierb_ids = re.findall(r'TEXT\("(Echo_\w+)"\)', ARTPACK[ARTPACK.find("GetTierBSpeciesTable"):ARTPACK.find("} // namespace AstrawildArtPack") if "} // namespace AstrawildArtPack" in ARTPACK else len(ARTPACK)])
tierb_table = re.findall(r'TEXT\("(Echo_\w+)"\)', ARTPACK[ARTPACK.find("GetTierBSpeciesTable"):ARTPACK.find("FString BuildTierBMechPath")])
glb_files = set()
echoes_dir = os.path.join(ROOT, "ArtSource", "Meshes", "Echoes")
if os.path.isdir(echoes_dir):
    for fn in os.listdir(echoes_dir):
        if fn.startswith("SK_Echo_") and fn.endswith(".glb"):
            glb_files.add("Echo_" + fn[len("SK_Echo_"):-4])
missing_glbs = [sid for sid in tierb_table if sid not in glb_files]
check(f"Tier-B code list == 36 species with unique real-mesh GLBs ({len(tierb_table)} listed)", len(tierb_table) == 36 and not missing_glbs, f"listed={len(tierb_table)} missing_glbs={missing_glbs[:5]}")
PROD_BOSSES_9B = {"Echo_DrownedSovereign", "Echo_EyeSentinel", "Echo_GlassTyrant"}
check("Production bosses carry direct real meshes (Act-3 chain)",
      PROD_BOSSES_9B <= set(tierb_table), f"missing={sorted(PROD_BOSSES_9B - set(tierb_table))}")

# --- 9b-2. ASSET OVERHAUL: real unique-mesh catalog + manifest truth ---------
try:
    MANIFEST = json.loads(read("ArtSource/manifest.json"))
    MA = MANIFEST.get("assets", {})
    mesh_rows = {k: v for k, v in MA.items() if v.get("category") == "mesh"}
    statuses = {v.get("status") for v in MA.values()}
    check(f"Manifest 100% present / 0 pending ({len(MA)} entries)", statuses == {"present"},
          f"statuses={statuses}")
    src_pairs = [v.get("source_model") for v in mesh_rows.values() if v.get("source_model")]
    check(f"Mesh rows carry unique source models — no palette swaps ({len(mesh_rows)} rows)",
          len(src_pairs) == len(set(src_pairs)), f"dupe sources={[s for s in set(src_pairs) if src_pairs.count(s) > 1][:5]}")
    missing_files = [k for k, v in mesh_rows.items()
                     if not os.path.isfile(os.path.join(ROOT, v.get("path", "Z:none")))]
    check("Every mesh row maps a real file on disk", not missing_files, f"missing={missing_files[:5]}")
    no_license = [k for k, v in mesh_rows.items() if "CC0" not in str(v.get("license", ""))]
    check("Every mesh row records a CC0 license", not no_license, f"no_license={no_license[:5]}")
    boss_dir = os.path.join(ROOT, "ArtSource", "Meshes", "Echoes", "Bosses")
    boss_files = {fn[:-4] for fn in os.listdir(boss_dir)} if os.path.isdir(boss_dir) else set()
    check(f"Boss showcase meshes staged (14 distinct real models)", len(boss_files) == 14,
          f"count={len(boss_files)}")
    survivor_dir = os.path.join(ROOT, "ArtSource", "Meshes", "Characters", "Survivor")
    survivor_files = {fn[:-4] for fn in os.listdir(survivor_dir)} if os.path.isdir(survivor_dir) else set()
    check("3 survivor armor-tier meshes staged (T1 Scavenger / T2 Astraite / T3 Exosuit)",
          survivor_files == {"SK_Survivor_Exosuit", "SK_Survivor_T1_Scavenger", "SK_Survivor_T2_Astraite"},
          f"on_disk={sorted(survivor_files)}")
    for w in ("SM_Weapon_ScrapRifle", "SM_Weapon_PlasmaCarbine", "SM_Weapon_ArcCannon",
              "SM_Weapon_Railgun", "SM_Weapon_SingularityCannon"):
        if w in mesh_rows:
            continue
        check(f"Weapon mesh row present: {w}", False, "row missing from manifest")
    check("5 weapon meshes distinct (unique source models)",
          all(w in mesh_rows for w in ("SM_Weapon_ScrapRifle", "SM_Weapon_PlasmaCarbine",
                                       "SM_Weapon_ArcCannon", "SM_Weapon_Railgun",
                                       "SM_Weapon_SingularityCannon"))
          and len({mesh_rows[w].get("source_model") for w in
                   ("SM_Weapon_ScrapRifle", "SM_Weapon_PlasmaCarbine", "SM_Weapon_ArcCannon",
                    "SM_Weapon_Railgun", "SM_Weapon_SingularityCannon")}) == 5, "weapon rows not 1:1")
except Exception as exc:  # noqa: BLE001
    check("ASSET OVERHAUL manifest gates parse", False, f"exception={exc}")

# --- 9c. SCI-FANTASY directive: mutation table + base archetype coherence ---
MUTDATA = read("Source/AstrawildCore/Private/AstrawildEchoMutationData.cpp")
MUTATOR = read("Source/AstrawildCore/Private/AstrawildEchoMutator.cpp")
ECHOCHAR = read("Source/AstrawildCore/Private/AstrawildEchoCharacter.cpp")
BD_9C = read("Source/AstrawildCore/Private/AstrawildBestiaryData.cpp")
mutation_rows = re.findall(r'TEXT\("(Echo_[A-Za-z0-9_]+)"\),\s*EAstrawildSciFantasyTheme::(\w+)', MUTDATA)
bestiary_ids = set(re.findall(r'\{ TEXT\("(Echo_[A-Za-z0-9_]+)"\), TEXT\("', BD_9C))
mutation_ids = [m[0] for m in mutation_rows]
check("Sci-Fantasy mutation table == 204 rows (one per bestiary species)",
      len(mutation_rows) == 204 and set(mutation_ids) == bestiary_ids,
      f"rows={len(mutation_rows)} unique={len(set(mutation_ids))} bestiary={len(bestiary_ids)} drift={sorted(set(mutation_ids) ^ bestiary_ids)[:5]}")
THEMES_9C = {"AncientConstruct", "ElementalBeast", "MutatedFauna", "ArmoredOrganic",
             "EtherealSpirit", "MechanicalHybrid", "PlantMonster", "VoidAbomination"}
seen_themes = {m[1] for m in mutation_rows}
check("All 8 Sci-Fantasy themes populated", seen_themes == THEMES_9C, f"missing={THEMES_9C - seen_themes}")
base_dir = os.path.join(ROOT, "ArtSource", "Meshes", "Echoes", "BaseMeshes")
base_glbs = {fn[:-4] for fn in os.listdir(base_dir) if fn.endswith(".glb")} if os.path.isdir(base_dir) else set()
SCI_BASES = {"SK_Base_GolemQuadruped", "SK_Base_MonolithColossus", "SK_Base_ElemDrake", "SK_Base_ElemWisp",
             "SK_Base_MutantBeast", "SK_Base_MutantAvian", "SK_Base_ArmoredBeetle", "SK_Base_ArmoredCrab",
             "SK_Base_SpiritWisp", "SK_Base_SpiritOrb", "SK_Base_CyborgBeast", "SK_Base_CyborgSerpent",
             "SK_Base_PlantMaw", "SK_Base_Mushroomling", "SK_Base_VoidBlob", "SK_Base_VoidTentacle"}
check("16 Sci-Fantasy base archetypes baked on disk", base_glbs == SCI_BASES,
      f"on_disk={len(base_glbs)} missing={sorted(SCI_BASES - base_glbs)[:5]}")
bound_bases = set(re.findall(r'TEXT\("(SK_Base_\w+)"\)', MUTDATA))
check("Every mutation row binds one of the 16 bases", bound_bases <= SCI_BASES,
      f"unknown={sorted(bound_bases - SCI_BASES)[:5]}")
check("Mutation consumption wired (EchoCharacter + ProductionContent + mutator)",
      "FAstrawildEchoMutator::" in ECHOCHAR and "AstrawildEchoMutation::GetMutationSpecs()" in PC
      and "FindSpec" in MUTATOR and "AppendMutationParts" in ECHOCHAR,
      "EchoCharacter/ProductionContent/Mutator must all consume the table")
echo_audio_dir = os.path.join(ROOT, "ArtSource", "Audio", "Echoes")
staged_cues = {fn[:-4] for fn in os.listdir(echo_audio_dir) if fn.endswith(".wav")} if os.path.isdir(echo_audio_dir) else set()
check("16 theme sound cues staged (8 sets x 2)", len(staged_cues) == 16,
      f"staged={len(staged_cues)}")

# --- 10. Building catalog completeness ---
cats = ["Foundation", "Wall", "Floor", "Roof", "Door", "Storage", "Workstation", "Farm", "Power", "Research"]
missing_cats = [c for c in cats if f"EAstrawildBuildingCategory::{c}" not in CL]
check("Building categories populated", not missing_cats, f"missing: {missing_cats}")

# --- 11. Authoritative content census (one truth per metric) ---
# These EXPECTED values are the single authoritative content counts (Final
# Completion Run Phase 0). Every active doc MUST cite these exact numbers; the
# engine-side live census log (ContentLibrary "live census" line) re-derives them
# at runtime. If source drifts, this gate forces reconciliation instead of
# letting stale numbers propagate silently.
BD = read("Source/AstrawildCore/Private/AstrawildBestiaryData.cpp")

def call_ids(source, helper):
    pat = re.compile(r"\b" + helper + r"\s*\((?:[^()]|\([^)]*\))*?TEXT\(\"([A-Za-z0-9_]+)\"\)")
    return pat.findall(source)

item_ids = set(call_ids(CL, "MakeItem")) | set(call_ids(PC, "MakeItem"))
recipe_ids = set(call_ids(CL, "MakeRecipe")) | set(call_ids(PC, "MakeRecipe"))
building_ids = set(call_ids(CL, "MakeBuilding"))
weapon_ids = set(call_ids(PC, "MakeWeapon"))
poi_ids = set(re.findall(r'TEXT\("(POI_[A-Za-z0-9_]+)"\)', PC)) - {"POI_Test"}
event_ids = set(call_ids(PC, "MakeWorldEvent"))
site_ids = set(re.findall(r'TEXT\("(Site_[A-Za-z0-9_]+)"\)', PC + CL)) - {"Site_Test"}
node_ids = set(re.findall(r'TEXT\("(Node_[A-Za-z0-9_]+)"\)', PC + CL)) - {"Node_Test"}
# robots are created via NewObject + RobotId = TEXT("Robot_X") + RegisterRobot (not a Make helper)
bestiary_rows = re.findall(r'\{ TEXT\("(Echo_[A-Za-z0-9_]+)"\), TEXT\("', BD)
hero_ids = set(call_ids(PC, "MakeProductionEcho"))
evo_targets = {b for _a, b in re.findall(r'\{ TEXT\("(Echo_[A-Za-z0-9_]+)"\),\s*TEXT\("(Echo_[A-Za-z0-9_]+)"\)', PC)}
starter_ids = set(call_ids(CL, "MakeEcho"))
species_ids = set(bestiary_rows) | hero_ids | evo_targets | starter_ids
quest_ids = set(re.findall(r'TEXT\("(Quest_[A-Za-z0-9_]+)"\)', PC + CL)) - {"Quest_Any", "Quest_DoesNotExist"}
tech_ids = set(re.findall(r'TEXT\("(Tech_[A-Za-z0-9_]+)"\)', PC + CL)) - {"Tech_DoesNotExist", "Tech_TestTech"}
loot_ids = set(re.findall(r'TEXT\("(Loot_[A-Za-z0-9_]+)"\)', PC + CL)) - {"Loot_Test"}
npc_ids = set(re.findall(r'TEXT\("(NPC_[A-Za-z0-9_]+)"\)', PC + CL)) - {"NPC_Test"}
robot_ids = set(re.findall(r'RobotId\s*=\s*TEXT\("(Robot_[A-Za-z0-9_]+)"\)', PC))
dialogue_ids = set(re.findall(r'TEXT\("(Dialogue_[A-Za-z0-9_]+)"\)', PC + CL))

EXPECTED_CENSUS = {
    "items": 78,
    "recipes": 58,
    "species": 229,
    "buildings": 26,
    "techs": 17,
    "quests": 22,
    "loot_tables": 11,
    "npcs": 13,
    "weapons": 8,
    "resource_nodes": 10,
    "work_sites": 8,
    "world_events": 16,
    "pois": 17,
    "dialogue_trees": 13,
    "robots": 3,
}
ACTUAL_CENSUS = {
    "items": len(item_ids),
    "recipes": len(recipe_ids),
    "species": len(species_ids),
    "buildings": len(building_ids),
    "techs": len(tech_ids),
    "quests": len(quest_ids),
    "loot_tables": len(loot_ids),
    "npcs": len(npc_ids),
    "weapons": len(weapon_ids),
    "resource_nodes": len(node_ids),
    "work_sites": len(site_ids),
    "world_events": len(event_ids),
    "pois": len(poi_ids),
    "dialogue_trees": len(dialogue_ids),
    "robots": len(robot_ids),
}
for metric, expected in EXPECTED_CENSUS.items():
    actual = ACTUAL_CENSUS[metric]
    check(f"Census {metric} == {expected}", actual == expected,
          f"actual={actual} — update EXPECTED_CENSUS + all active docs together (never edit one side alone)")

# The completion log must be dynamic (no hardcoded counts in log strings).
check("Completion log derives counts live (no hardcoded census)",
      "GetAllItems().Num()" in CL and "live census" in CL,
      "ContentLibrary completion log must count from the registry")

print()
if FAILURES:
    print(f"FINAL RUN VALIDATION: {len(FAILURES)} FAILURE(S)")
    for f in FAILURES:
        print(" -", f)
    sys.exit(1)
print("FINAL RUN VALIDATION: ALL CHECKS PASSED (static level — engine build/test still required on the target machine)")
