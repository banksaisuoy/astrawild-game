"""
build_prototype_map.py — ENGINE-RUN-1 / TASK 2
================================================
Creates (or deterministically rebuilds) the zero-dependency prototype level:

    /Game/ASTRAWILD/Maps/Prototype/L_Proto_01

Contents:
  - 100 m x 100 m floor   (Engine BasicShapes Cube, scaled 100x100x1)
  - DirectionalLight      (rotated, lit)
  - SkyLight              (real-time capture, movable)
  - SkyAtmosphere         (defaults)
  - ExponentialHeightFog  (light haze)
  - one PlayerStart       at (0, 0, 200)
  - 5 cube obstacles      at varied positions/scales
  - the level is SAVED to disk

Idempotent: every actor this script owns is labeled with the "LPROTO_"
prefix. On re-run the script loads the existing level (or creates it),
DESTROYS every LPROTO_* actor, re-adds the full deterministic layout and
saves — so running it twice always converges to the same result.

Run headless (Windows, engine at E:\\Epic Games\\UnrealEngine):
  "E:\\Epic Games\\UnrealEngine\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" ^
    "E:\\AstrawildGame\\ASTRAWILD.uproject" ^
    -run=pythonscript -script="E:\\AstrawildGame\\Tools\\Python\\build_prototype_map.py" ^
    -stdout -unattended -nopause -nosplash

Required editor plugin: Python Editor Script Plugin (PythonScriptPlugin) —
it is listed in ASTRAWILD.uproject so the commandlet works out of the box.
"""

import sys
import unreal

# ---------------------------------------------------------------------------
# Configuration
# ---------------------------------------------------------------------------

LEVEL_PATH = "/Game/ASTRAWILD/Maps/Prototype/L_Proto_01"
CUBE_MESH = unreal.load_asset("/Engine/BasicShapes/Cube")  # 100 x 100 x 100 cm

FLOOR_LABEL = "LPROTO_Floor"
FLOOR_SCALE = unreal.Vector(100.0, 100.0, 1.0)  # 100 m x 100 m x 1 m
FLOOR_LOCATION = unreal.Vector(0.0, 0.0, -50.0)  # top surface exactly at Z = 0

PLAYER_START_LABEL = "LPROTO_PlayerStart"
PLAYER_START_LOCATION = unreal.Vector(0.0, 0.0, 200.0)

OBSTACLE_LABEL_PREFIX = "LPROTO_Obstacle_"
# (location, relative scale) — all sit ON the floor (top at Z = 0).
OBSTACLES = [
    (unreal.Vector(2500.0, 1800.0, 100.0), unreal.Vector(2.0, 2.0, 2.0)),
    (unreal.Vector(-3100.0, -2400.0, 150.0), unreal.Vector(3.0, 2.0, 3.0)),
    (unreal.Vector(3800.0, -2900.0, 75.0), unreal.Vector(1.5, 1.5, 1.5)),
    (unreal.Vector(-1800.0, 3400.0, 200.0), unreal.Vector(4.0, 4.0, 4.0)),
    (unreal.Vector(600.0, -4200.0, 50.0), unreal.Vector(1.0, 1.0, 1.0)),
]

PREFIX = "LPROTO_"

# ---------------------------------------------------------------------------
# Editor-API compatibility layer
#
# UE 5.x moved the EditorLevelLibrary / spawn helpers onto editor
# subsystems. Prefer the subsystems (no deprecation warnings in the Output
# Log); fall back to the legacy library on older installs.
# ---------------------------------------------------------------------------


def _level_api():
    """Returns an object exposing new_level / load_level / save_current_level."""
    try:
        return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    except Exception:  # pragma: no cover — legacy engines only
        return unreal.EditorLevelLibrary


def _actor_api():
    """Returns an object exposing spawn_actor_from_class / get_all_level_actors."""
    try:
        return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    except Exception:  # pragma: no cover — legacy engines only
        return unreal.EditorLevelLibrary


def _destroy_actor(actor):
    api = _actor_api()
    if hasattr(api, "destroy_actor"):
        api.destroy_actor(actor)
    else:  # pragma: no cover
        actor.destroy_actor()


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def log(message):
    unreal.log("[L_Proto_01] {}".format(message))


def purge_owned_actors():
    """Destroy every actor whose label carries our prefix — idempotency."""
    actors = _actor_api().get_all_level_actors()
    destroyed = 0
    for actor in actors:
        label = actor.get_actor_label()
        if label.startswith(PREFIX):
            _destroy_actor(actor)
            destroyed += 1
    return destroyed


def spawn_actor(actor_class, location, rotation=unreal.Rotator(0.0, 0.0, 0.0)):
    return _actor_api().spawn_actor_from_class(actor_class, location, rotation)


def make_static_cube(label, location, scale):
    """Spawns a StaticMeshActor with the BasicShapes cube mesh applied."""
    actor = spawn_actor(unreal.StaticMeshActor, location)
    actor.set_actor_label(label)

    mesh_component = actor.get_editor_property("static_mesh_component")
    # Mesh + scale are set at editor time; static mobility keeps lighting cheap.
    mesh_component.set_static_mesh(CUBE_MESH)
    mesh_component.set_relative_scale3d(scale)
    return actor


def first_component(actor, component_class):
    components = actor.get_components_by_class(component_class)
    return components[0] if components else None


def safe_set_property(obj, property_name, value):
    """Sets a property when it exists on this engine version, else skips.

    Lets the script survive minor property renames across engine releases
    while keeping the core contract (lit floor + pawn start) guaranteed.
    """
    try:
        obj.set_editor_property(property_name, value)
        return True
    except Exception as exc:  # noqa: BLE001 — resilience by design
        log("WARNING: could not set '{}' on {} ({}). Continuing."
            .format(property_name, obj.get_name(), exc))
        return False


# ---------------------------------------------------------------------------
# Builders
# ---------------------------------------------------------------------------


def build_floor():
    make_static_cube(FLOOR_LABEL, FLOOR_LOCATION, FLOOR_SCALE)
    log("Floor: 100 m x 100 m BasicShapes cube (top at Z = 0).")


def build_lighting():
    # --- DirectionalLight -------------------------------------------------
    sun = spawn_actor(
        unreal.DirectionalLight,
        unreal.Vector(0.0, 0.0, 1500.0),
        unreal.Rotator(-50.0, 130.0, 0.0),
    )
    sun.set_actor_label("LPROTO_DirectionalLight")
    sun_component = first_component(sun, unreal.DirectionalLightComponent)
    if sun_component:
        safe_set_property(sun_component, "intensity", 8.0)
        safe_set_property(sun_component, "mobility", unreal.ComponentMobility.MOVABLE)

    # --- SkyLight ---------------------------------------------------------
    sky_light = spawn_actor(unreal.SkyLight, unreal.Vector(0.0, 0.0, 1500.0))
    sky_light.set_actor_label("LPROTO_SkyLight")
    sky_component = first_component(sky_light, unreal.SkyLightComponent)
    if sky_component:
        # Real-time capture needs movable mobility on the component.
        safe_set_property(sky_component, "mobility", unreal.ComponentMobility.MOVABLE)
        safe_set_property(sky_component, "real_time_capture", True)

    # --- SkyAtmosphere ------------------------------------------------------
    atmosphere = spawn_actor(unreal.SkyAtmosphere, unreal.Vector(0.0, 0.0, 0.0))
    atmosphere.set_actor_label("LPROTO_SkyAtmosphere")

    # --- ExponentialHeightFog ----------------------------------------------
    fog = spawn_actor(unreal.ExponentialHeightFog, unreal.Vector(0.0, 0.0, 0.0))
    fog.set_actor_label("LPROTO_ExponentialHeightFog")
    fog_component = first_component(fog, unreal.ExponentialHeightFogComponent)
    if fog_component:
        safe_set_property(fog_component, "fog_density", 0.02)
        safe_set_property(fog_component, "fog_height_falloff", 0.2)

    log("Lighting: DirectionalLight + SkyLight (real-time) + SkyAtmosphere "
        "+ ExponentialHeightFog.")


def build_player_start():
    start = spawn_actor(unreal.PlayerStart, PLAYER_START_LOCATION)
    start.set_actor_label(PLAYER_START_LABEL)
    log("PlayerStart at (0, 0, 200).")


def build_obstacles():
    for index, (location, scale) in enumerate(OBSTACLES):
        make_static_cube(
            "{}{:02d}".format(OBSTACLE_LABEL_PREFIX, index + 1), location, scale
        )
    log("Obstacles: {} cubes placed.".format(len(OBSTACLES)))


# ---------------------------------------------------------------------------
# Level lifecycle
# ---------------------------------------------------------------------------


def open_or_create_level():
    """Loads the level when it exists, otherwise creates it empty."""
    level_api = _level_api()
    if unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):
        if not level_api.load_level(LEVEL_PATH):
            raise RuntimeError("Failed to load existing level: " + LEVEL_PATH)
        log("Loaded existing level (rebuild).")
    else:
        if not level_api.new_level(LEVEL_PATH):
            raise RuntimeError("Failed to create level: " + LEVEL_PATH)
        log("Created new level.")


def save_level():
    if not _level_api().save_current_level():
        raise RuntimeError("Failed to save level: " + LEVEL_PATH)
    log("Level saved: " + LEVEL_PATH)


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------


def main():
    if CUBE_MESH is None:
        raise RuntimeError(
            "Could not load /Engine/BasicShapes/Cube — run inside the "
            "ASTRAWILD project editor (UnrealEditor-Cmd -run=pythonscript)."
        )

    log("Building {} …".format(LEVEL_PATH))
    open_or_create_level()

    purged = purge_owned_actors()
    if purged:
        log("Purged {} stale LPROTO_* actor(s) for idempotency.".format(purged))

    build_floor()
    build_lighting()
    build_player_start()
    build_obstacles()

    save_level()
    log("DONE — open L_Proto_01 and press Play.")


if __name__ == "__main__":
    try:
        main()
    except Exception as error:  # noqa: BLE001 — commandlet must report failure
        unreal.log_error("[L_Proto_01] FAILED: {}".format(error))
        sys.exit(1)
