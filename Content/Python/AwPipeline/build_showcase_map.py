"""
ASTRAWILD AwPipeline — ASSET OVERHAUL showcase map builder.

Creates (or updates) /Game/Maps/L_Showcase_ArtOverhaul: a playable showcase
level that places every REAL imported mesh on real ground so the asset
overhaul can be verified by eye in PIE:

  * PlayerStart at the plaza center (the survivor pawn spawns with the real
    Tier-3 Singularity Exosuit mesh + its 7 locomotion clips at runtime).
  * Armor-tier podium: SK_Survivor_T1_Scavenger / T2_Astraite / Exosuit.
  * Hero Echo row (6 production heroes, real unique meshes).
  * Tier-B species grid (33 species with unique real CC0 meshes).
  * 16 base-archetype row (the mutation-system geometry layer).
  * Boss arena line (3 production bosses + 14 showcase bosses).
  * Weapon rack (5 distinct blaster meshes on pedestals, Muzzle forward).
  * Vehicle pad (Dawn Skiff hovercraft, ground rover, support skiff, heavy).
  * Resource-node garden (4 ore types + flora/trees/ruins dressing).

Run inside the UE Editor AFTER import_all.py (console):
    py "<ProjectRoot>/Content/Python/AwPipeline/build_showcase_map.py"
Idempotent: re-running clears the showcase actors and places them again.
"""
import json
import os
import sys

import unreal

_HERE = os.path.dirname(os.path.abspath(__file__))
if _HERE not in sys.path:
    sys.path.insert(0, _HERE)

PLAZA_FLOOR = 3000.0        # cm
ROW_SPACING = 700.0
GRID_STEP = 450.0
Y_BASE = 600.0

placed = {"static": 0, "skeletal": 0, "labels": 0, "other": 0}


def log(msg: str) -> None:
    unreal.log("[ShowcaseMap] " + str(msg))


def warn(msg: str) -> None:
    unreal.log_warning("[ShowcaseMap] " + str(msg))


def project_root() -> str:
    return os.path.dirname(os.path.abspath(unreal.Paths.get_project_file_path()))


def load_manifest(root: str) -> dict:
    with open(os.path.join(root, "ArtSource", "manifest.json"), "r", encoding="utf-8") as f:
        return json.load(f)


def get_level_subsystem():
    if hasattr(unreal, "EditorActorSubsystem"):
        return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    return None


def spawn(cls, location, rotation=(0.0, 0.0, 0.0)):
    loc = unreal.Vector(*location)
    rot = unreal.Rotator(*rotation)
    sub = get_level_subsystem()
    if sub is not None:
        actor = sub.spawn_actor_from_class(cls, loc, rot)
    else:
        actor = unreal.EditorLevelLibrary.spawn_actor_from_class(cls, loc, rot)
    return actor


def label(text: str, location, size=60):
    actor = spawn(unreal.TextRenderActor, location)
    if not actor:
        return None
    try:
        rend = actor.get_editor_property("text_render")
        rend.set_editor_property("text", text)
        rend.set_editor_property("text_world_size", float(size))
        actor.set_editor_property("text_render", rend)
        placed["labels"] += 1
    except Exception as e:  # noqa: BLE001
        warn(f"label {text}: {e}")
    return actor


def place_static(asset_id: str, manifest: dict, location, scale=1.0, rotation=(0.0, 0.0, 0.0)):
    mesh = unreal.EditorAssetLibrary.load_asset(manifest["assets"][asset_id]["ue_path"])
    if not mesh:
        warn(f"{asset_id}: mesh not imported — skipped")
        return None
    sub = get_level_subsystem()
    if sub is not None:
        actor = sub.spawn_actor_from_object(mesh, unreal.Vector(*location), unreal.Rotator(*rotation))
    else:
        actor = unreal.EditorLevelLibrary.spawn_actor_from_object(
            mesh, unreal.Vector(*location), unreal.Rotator(*rotation))
    if not actor:
        return None
    try:
        actor.set_actor_scale3d(unreal.Vector(scale, scale, scale))
    except Exception:
        pass
    placed["static"] += 1
    return actor


def place_skeletal(asset_id: str, manifest: dict, location, rotation=(0.0, 0.0, 0.0)):
    info = manifest["assets"][asset_id]
    mesh = unreal.EditorAssetLibrary.load_asset(info["ue_path"])
    if not mesh:
        warn(f"{asset_id}: skeletal mesh not imported — skipped")
        return None
    sub = get_level_subsystem()
    if sub is not None:
        actor = sub.spawn_actor_from_object(mesh, unreal.Vector(*location), unreal.Rotator(*rotation))
    else:
        actor = unreal.EditorLevelLibrary.spawn_actor_from_object(
            mesh, unreal.Vector(*location), unreal.Rotator(*rotation))
    if not actor:
        return None
    try:
        skel = actor.get_editor_property("skeletal_mesh_component")
        skel.set_editor_property("skeletal_mesh", mesh)
        # Play the real idle clip when one exists (clip_map convention).
        folder = os.path.dirname(info["ue_path"])
        idle = f"{folder}/AM_{asset_id}_Idle"
        if unreal.EditorAssetLibrary.does_asset_exist(idle):
            seq = unreal.EditorAssetLibrary.load_asset(idle)
            skel.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_SINGLENODE)
            skel.set_editor_property("animation_asset", seq)
            skel.set_editor_property("play_animation", True)
        actor.set_editor_property("skeletal_mesh_component", skel)
    except Exception as e:  # noqa: BLE001
        warn(f"{asset_id} anim setup: {e}")
    placed["skeletal"] += 1
    return actor


def pedestal(location, height=40.0, size=120.0):
    mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube")
    if not mesh:
        return None
    sub = get_level_subsystem()
    if sub is not None:
        actor = sub.spawn_actor_from_object(mesh, unreal.Vector(*location), unreal.Rotator(0, 0, 0))
    else:
        actor = unreal.EditorLevelLibrary.spawn_actor_from_object(
            mesh, unreal.Vector(*location), unreal.Rotator(0, 0, 0))
    if actor:
        actor.set_actor_scale3d(unreal.Vector(size / 100.0, size / 100.0, height / 100.0))
        actor.set_actor_location(unreal.Vector(location[0], location[1], height / 2.0))
        placed["other"] += 1
    return actor


def ensure_level(map_path: str = "/Game/Maps/L_Showcase_ArtOverhaul"):
    """Create the showcase level if missing, then open it. Returns the world."""
    if unreal.EditorAssetLibrary.does_asset_exist(map_path):
        unreal.EditorLevelLibrary.load_level(map_path)
        clear_showcase_actors()
        return unreal.EditorLevelLibrary.get_editor_world()
    try:
        if hasattr(unreal, "EditorLevelSubsystem"):
            sub = unreal.get_editor_subsystem(unreal.EditorLevelSubsystem)
            if sub is not None:
                sub.new_level(map_path)
                return unreal.EditorLevelLibrary.get_editor_world()
        unreal.EditorLevelLibrary.new_level(map_path)
        return unreal.EditorLevelLibrary.get_editor_world()
    except Exception as e:  # noqa: BLE001
        warn(f"level creation fell back to load-only: {e}")
        return unreal.EditorLevelLibrary.get_editor_world()


def clear_showcase_actors() -> int:
    """Remove previously placed showcase actors (idempotent re-run)."""
    removed = 0
    level_lib = unreal.EditorLevelLibrary if hasattr(unreal, "EditorLevelLibrary") else None
    if level_lib is None:
        return removed
    world = level_lib.get_editor_world()
    if not world:
        return removed
    for actor in world.get_actors():
        name = str(actor.get_name())
        if name.startswith("ShowcaseTag_") or name.startswith("SM_") \
                or name.startswith("SK_") or name.startswith("Cube_") \
                or name.startswith("TextRenderActor_"):
            try:
                actor.destroy_actor()
                removed += 1
            except Exception:
                pass
    return removed


def basic_lighting():
    sun = spawn(unreal.DirectionalLight, (0.0, 0.0, 900.0), (-40.0, 35.0, 0.0))
    if sun:
        try:
            comp = sun.get_editor_property("light_component")
            comp.set_editor_property("intensity", 8.0)
            sun.set_editor_property("light_component", comp)
        except Exception:
            pass
    sky = spawn(unreal.SkyLight, (0.0, 0.0, 900.0))
    if sky:
        try:
            comp = sky.get_editor_property("light_component")
            comp.set_editor_property("intensity", 1.2)
            sky.set_editor_property("light_component", comp)
        except Exception:
            pass
    spawn(unreal.SkyAtmosphere, (0.0, 0.0, 0.0))
    placed["other"] += 3


def ground_floor():
    mesh = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube")
    if not mesh:
        warn("no engine cube for floor")
        return
    sub = get_level_subsystem()
    loc = unreal.Vector(0.0, 0.0, -50.0)
    rot = unreal.Rotator(0.0, 0.0, 0.0)
    if sub is not None:
        actor = sub.spawn_actor_from_object(mesh, loc, rot)
    else:
        actor = unreal.EditorLevelLibrary.spawn_actor_from_object(mesh, loc, rot)
    if actor:
        actor.set_actor_scale3d(unreal.Vector(PLAZA_FLOOR / 100.0 * 2, PLAZA_FLOOR / 100.0, 1.0))
        placed["other"] += 1


def main() -> int:
    root = project_root()
    manifest = load_manifest(root)
    assets = manifest["assets"]
    log(f"project root: {root} — manifest {len(assets)} entries")

    world = ensure_level()
    if not world:
        unreal.log_error("[ShowcaseMap] no world — aborting")
        return 1
    log(f"level ready: {world.get_path_name()}")

    ground_floor()
    basic_lighting()

    # --- PlayerStart: the real survivor pawn spawns here (Tier-3 Exosuit).
    ps = spawn(unreal.PlayerStart, (0.0, -800.0, 120.0), (0.0, 0.0, 0.0))
    if ps:
        label("PLAYER START — Tier-3 Singularity Exosuit (real mesh, PIE to play)", (0.0, -950.0, 260.0), 40)

    # --- Armor tier podium (3 real rigged humanoids).
    label("ARMOR TIERS — T1 Scavenger / T2 Astraite / T3 Singularity Exosuit", (0.0, -200.0, 260.0), 46)
    for i, tier in enumerate(("SK_Survivor_T1_Scavenger", "SK_Survivor_T2_Astraite", "SK_Survivor_Exosuit")):
        place_skeletal(tier, manifest, (-450.0 + i * 450.0, 0.0, 0.0), (0.0, 0.0, 0.0))

    # --- Hero Echo row.
    label("PRODUCTION HERO ECHOES (6 unique real meshes)", (0.0, Y_BASE + 120.0, 260.0), 46)
    heroes = ["SK_Echo_Terraquill", "SK_Echo_Cindermule", "SK_Echo_Voltpylon",
              "SK_Echo_Bastionbeetle", "SK_Echo_Mistmender", "SK_Echo_Deepdelver"]
    for i, h in enumerate(heroes):
        place_skeletal(h, manifest, (-1125.0 + i * 450.0, Y_BASE + 250.0, 0.0))

    # --- Base archetype row (16 — mutation-system geometry layer).
    label("SCI-FANTASY BASE ARCHETYPES x16 (mutation geometry layer)", (0.0, Y_BASE + 900.0, 260.0), 46)
    bases = sorted(k for k in assets if k.startswith("SK_Base_"))
    for i, b in enumerate(bases):
        col, row = i % 8, i // 8
        place_skeletal(b, manifest, (-1575.0 + col * 450.0, Y_BASE + 1050.0 + row * 450.0, 0.0))

    # --- Tier-B species grid (33 unique real meshes).
    label("TIER-B SPECIES x33 (unique real CC0 meshes)", (0.0, Y_BASE + 2100.0, 260.0), 46)
    tierb = sorted(k for k in assets
                   if k.startswith("SK_Echo_") and k not in heroes
                   and not k.endswith(("DrownedSovereign", "EyeSentinel", "GlassTyrant")))
    for i, t in enumerate(tierb):
        col, row = i % 7, i // 7
        place_skeletal(t, manifest, (-1350.0 + col * 450.0, Y_BASE + 2250.0 + row * 450.0, 0.0))

    # --- Boss line (3 production + 14 showcase bosses).
    label("BOSSES — 3 production (Act-3) + 14 showcase (unique real meshes)", (0.0, Y_BASE + 4900.0, 260.0), 46)
    bosses = ["SK_Echo_GlassTyrant", "SK_Echo_EyeSentinel", "SK_Echo_DrownedSovereign"] + \
             sorted(k for k in assets if k.startswith("SK_Boss_"))
    for i, b in enumerate(bosses):
        col, row = i % 6, i // 6
        scale = 1.6 if row == 0 else 1.35
        act = place_skeletal(b, manifest, (-1125.0 + col * 450.0, Y_BASE + 5100.0 + row * 600.0, 0.0))
        if act:
            act.set_actor_scale3d(unreal.Vector(scale, scale, scale))

    # --- Weapon rack (5 distinct blasters on pedestals, muzzles forward).
    label("WEAPONS — Scrap Rifle / Plasma Carbine / Arc Cannon / Railgun / Singularity Cannon",
          (-2200.0, 1200.0, 260.0), 42)
    weapons = ["SM_Weapon_ScrapRifle", "SM_Weapon_PlasmaCarbine", "SM_Weapon_ArcCannon",
               "SM_Weapon_Railgun", "SM_Weapon_SingularityCannon"]
    for i, w in enumerate(weapons):
        loc = (-2200.0, 1600.0 + i * 450.0, 60.0)
        pedestal((loc[0], loc[1], 0.0), height=120.0)
        act = place_static(w, manifest, loc, 4.0, (0.0, 0.0, -90.0))
        if act:
            act.set_actor_scale3d(unreal.Vector(4.0, 4.0, 4.0))

    # --- Vehicle pad.
    label("VEHICLES — Dawn Skiff (hover) / Ground Rover / Support Skiff / Heavy Rover",
          (2200.0, 1200.0, 260.0), 42)
    vehicles = ["SM_Vehicle_DawnSkiff", "SM_Vehicle_GroundRover",
                "SM_Vehicle_SupportSkiff", "SM_Vehicle_RoverHeavy"]
    for i, v in enumerate(vehicles):
        place_static(v, manifest, (2200.0, 1600.0 + i * 700.0, 20.0), 2.0)

    # --- Resource-node garden + flora + ruins.
    label("RESOURCE NODES — Astraite / Pyronite / Voidstone / Ancient Vein (+ flora & ruins)",
          (2200.0, -1200.0, 260.0), 42)
    nodes = ["SM_Node_Astraite", "SM_Node_Pyronite", "SM_Node_Voidstone", "SM_Node_AncientVein"]
    for i, n in enumerate(nodes):
        place_static(n, manifest, (2200.0, -900.0 + i * 450.0, 0.0), 2.0)
    flora = ["SM_Tree_Broadleaf", "SM_Tree_Conifer", "SM_Tree_SporeCanopy", "SM_Fern",
             "SM_Grass_Tuft", "SM_GlowReed", "SM_Ruin_Arch", "SM_Ruin_Pillar",
             "SM_Flower_Red", "SM_Mushroom_Red"]
    for i, f in enumerate(flora):
        col, row = i % 5, i // 5
        place_static(f, manifest, (2900.0 + col * 450.0, -900.0 + row * 550.0, 0.0), 1.5)

    # --- Save the level.
    try:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)
        log(f"level saved: /Game/Maps/L_Showcase_ArtOverhaul")
    except Exception as e:  # noqa: BLE001
        warn(f"save: {e}")

    unreal.log("=" * 60)
    unreal.log(f"[ShowcaseMap] PLACED: static={placed['static']} skeletal={placed['skeletal']} "
               f"labels={placed['labels']} utility={placed['other']} — press Play (PIE) to test.")
    unreal.log("=" * 60)
    return 0


if __name__ == "__main__":
    main()
