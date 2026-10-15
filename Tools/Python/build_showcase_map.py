"""
build_showcase_map.py — LONG-RUN DIRECTIVE L5.3
================================================
Builds (or deterministically rebuilds) the day-one finisher level:

    /Game/ASTRAWILD/Maps/L_Showcase_ArtOverhaul

CINEMATIC IDENTITY (Master Directive Q2 — presentation only, zero content
reduction): lighting, volumetric fog, exposure, and a FRAMED HERO VISTA.
The full 229-species canon stays untouched — this level presents it.

Contents (every actor owned by this script carries the "LSHOW_" prefix and
is purged on re-run — idempotent convergence):

  - 140 m x 140 m staging floor (top at Z = 0)
  - HERO VISTA: a raised 24 m vista deck at the far end, reached by a ramp,
    framing the whole stage from above (the "day-one finisher" camera beat)
  - hero row: 6 pedestal markers across the stage midline (the first-impression
    lineup surface — content meshes bind at the engine pass via
    GetEchoArt/direct-mesh coverage; BasicShapes stand in fail-closed)
  - Tier-B grid: 8 low markers for the broad roster surface
  - boss arena ring: 4 large markers in the far corner
  - CINEMATIC lighting rig:
      * DirectionalLight at golden-hour angle (-35 deg pitch), warm intensity
      * SkyLight (real-time capture, movable)
      * SkyAtmosphere
      * ExponentialHeightFog — dense volumetric haze with a far start
        distance (depth-cue framing for the vista)
      * PostProcessVolume (unbound) — auto-exposure bias + slight lift:
        the readable-cinematic exposure baseline
  - PlayerStart ON the vista deck, facing the stage (the framed hero shot
    is the first thing the player sees)

UNVERIFIED — NEEDS MACHINE (R2/R3): PostProcessVolume property names
(auto_exposure_bias etc.) follow the documented 5.x naming; the first
engine run is the conformance test. Fail-closed: every property set goes
through safe_set_property, so a rename degrades the look, never the run.

Run headless (Windows):
  "E:\\Epic Games\\UnrealEngine\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" ^
    "E:\\AstrawildGame\\ASTRAWILD.uproject" ^
    -run=pythonscript -script="E:\\AstrawildGame\\Tools\\Python\\build_showcase_map.py" ^
    -stdout -unattended -nopause -nosplash
"""

import sys

import unreal

LEVEL_PATH = "/Game/ASTRAWILD/Maps/L_Showcase_ArtOverhaul"
CUBE_MESH = unreal.load_asset("/Engine/BasicShapes/Cube")

PREFIX = "LSHOW_"

FLOOR_LABEL = PREFIX + "Floor"
FLOOR_SCALE = unreal.Vector(140.0, 140.0, 1.0)
FLOOR_LOCATION = unreal.Vector(0.0, 0.0, -50.0)

# Hero vista deck: raised, far end of the stage (+X), facing the stage.
VISTA_LABEL = PREFIX + "VistaDeck"
VISTA_SCALE = unreal.Vector(4.0, 12.0, 12.0)
VISTA_LOCATION = unreal.Vector(5200.0, 0.0, 1100.0)
RAMP_LABEL = PREFIX + "VistaRamp"
RAMP_SCALE = unreal.Vector(8.0, 4.0, 3.0)
RAMP_LOCATION = unreal.Vector(4200.0, 0.0, 300.0)

PLAYER_START_LABEL = PREFIX + "PlayerStart"
PLAYER_START_LOCATION = unreal.Vector(5400.0, 0.0, 1400.0)
PLAYER_START_ROTATION = unreal.Rotator(0.0, 180.0, 0.0)  # face the stage

# Hero row — 6 pedestals across the midline (first-impression lineup).
HERO_ROW_LABEL = PREFIX + "Hero_"
HERO_ROW = [
    (unreal.Vector(-1500.0, -2000.0, 100.0), unreal.Vector(2.0, 2.0, 1.0)),
    (unreal.Vector(-900.0, -1200.0, 100.0), unreal.Vector(2.0, 2.0, 1.0)),
    (unreal.Vector(-300.0, -400.0, 100.0), unreal.Vector(2.0, 2.0, 1.0)),
    (unreal.Vector(300.0, 400.0, 100.0), unreal.Vector(2.0, 2.0, 1.0)),
    (unreal.Vector(900.0, 1200.0, 100.0), unreal.Vector(2.0, 2.0, 1.0)),
    (unreal.Vector(1500.0, 2000.0, 100.0), unreal.Vector(2.0, 2.0, 1.0)),
]

# Tier-B grid — 8 low markers (the broad-roster surface).
TIERB_LABEL = PREFIX + "TierB_"
TIERB_BASE = unreal.Vector(-4200.0, -5200.0, 50.0)
TIERB_STEP = unreal.Vector(900.0, 900.0, 0.0)

# Boss arena ring — 4 large markers in the far corner.
BOSS_LABEL = PREFIX + "Boss_"
BOSS_RING = [
    unreal.Vector(-4800.0, 4400.0, 200.0),
    unreal.Vector(-3600.0, 5200.0, 200.0),
    unreal.Vector(-2400.0, 4400.0, 200.0),
    unreal.Vector(-3600.0, 3600.0, 200.0),
]
BOSS_SCALE = unreal.Vector(4.0, 4.0, 4.0)

# Cinematic rig values (presentation-only — the canon census is untouched).
SUN_LOCATION = unreal.Vector(0.0, 0.0, 4000.0)
SUN_ROTATION = unreal.Rotator(-35.0, 155.0, 0.0)  # golden-hour rake
SUN_INTENSITY = 10.0
FOG_DENSITY = 0.06          # dense cinematic haze
FOG_HEIGHT_FALLOFF = 0.25
EXPOSURE_BIAS = 0.8         # lifted, readable-cinematic baseline


def log(message):
    unreal.log("[L_Showcase] {}".format(message))


def _level_api():
    try:
        return unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    except Exception:  # pragma: no cover — legacy engines only
        return unreal.EditorLevelLibrary


def _actor_api():
    try:
        return unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    except Exception:  # pragma: no cover
        return unreal.EditorLevelLibrary


def spawn_actor(actor_class, location, rotation=unreal.Rotator(0.0, 0.0, 0.0)):
    return _actor_api().spawn_actor_from_class(actor_class, location, rotation)


def make_static_cube(label, location, scale):
    actor = spawn_actor(unreal.StaticMeshActor, location)
    actor.set_actor_label(label)
    mesh_component = actor.get_editor_property("static_mesh_component")
    mesh_component.set_static_mesh(CUBE_MESH)
    mesh_component.set_relative_scale3d(scale)
    return actor


def first_component(actor, component_class):
    components = actor.get_components_by_class(component_class)
    return components[0] if components else None


def safe_set_property(obj, property_name, value):
    try:
        obj.set_editor_property(property_name, value)
        return True
    except Exception as exc:  # noqa: BLE001 — resilience by design
        log("WARNING: could not set '{}' on {} ({}). Continuing."
            .format(property_name, obj.get_name(), exc))
        return False


def purge_owned_actors():
    actors = _actor_api().get_all_level_actors()
    destroyed = 0
    for actor in actors:
        if actor.get_actor_label().startswith(PREFIX):
            if hasattr(_actor_api(), "destroy_actor"):
                _actor_api().destroy_actor(actor)
            else:  # pragma: no cover
                actor.destroy_actor()
            destroyed += 1
    return destroyed


def open_or_create_level():
    level_api = _level_api()
    if unreal.EditorAssetLibrary.does_asset_exist(LEVEL_PATH):
        if not level_api.load_level(LEVEL_PATH):
            raise RuntimeError("Failed to load existing level: " + LEVEL_PATH)
        log("Loaded existing level (rebuild).")
    else:
        if not level_api.new_level(LEVEL_PATH):
            raise RuntimeError("Failed to create level: " + LEVEL_PATH)
        log("Created new level.")


def build_floor():
    make_static_cube(FLOOR_LABEL, FLOOR_LOCATION, FLOOR_SCALE)
    log("Staging floor: 140 m x 140 m (top at Z = 0).")


def build_vista():
    make_static_cube(VISTA_LABEL, VISTA_LOCATION, VISTA_SCALE)
    make_static_cube(RAMP_LABEL, RAMP_LOCATION, RAMP_SCALE)
    log("Hero vista: raised deck + ramp (the day-one finisher framing).")


def build_player_start():
    start = spawn_actor(unreal.PlayerStart, PLAYER_START_LOCATION,
                        PLAYER_START_ROTATION)
    start.set_actor_label(PLAYER_START_LABEL)
    log("PlayerStart on the vista deck, facing the stage (framed hero shot).")


def build_showcase_content():
    for index, (location, scale) in enumerate(HERO_ROW):
        make_static_cube("{}{:02d}".format(HERO_ROW_LABEL, index + 1),
                         location, scale)
    for index in range(8):
        location = unreal.Vector(
            TIERB_BASE.x + (index % 4) * TIERB_STEP.x,
            TIERB_BASE.y + (index // 4) * TIERB_STEP.y,
            TIERB_BASE.z)
        make_static_cube("{}{:02d}".format(TIERB_LABEL, index + 1),
                         location, unreal.Vector(1.5, 1.5, 0.75))
    for index, location in enumerate(BOSS_RING):
        make_static_cube("{}{:02d}".format(BOSS_LABEL, index + 1),
                         location, BOSS_SCALE)
    log("Showcase surfaces: {} hero pedestals + 8 Tier-B markers + "
        "{} boss ring markers (content meshes bind at the engine pass)."
        .format(len(HERO_ROW), len(BOSS_RING)))


def build_cinematic_rig():
    # --- warm golden-hour key light -----------------------------------------
    sun = spawn_actor(unreal.DirectionalLight, SUN_LOCATION, SUN_ROTATION)
    sun.set_actor_label(PREFIX + "Sun_GoldenHour")
    sun_component = first_component(sun, unreal.DirectionalLightComponent)
    if sun_component:
        safe_set_property(sun_component, "intensity", SUN_INTENSITY)
        safe_set_property(sun_component, "mobility",
                          unreal.ComponentMobility.MOVABLE)

    # --- ambient sky (real-time capture) ------------------------------------
    sky_light = spawn_actor(unreal.SkyLight, unreal.Vector(0.0, 0.0, 3000.0))
    sky_light.set_actor_label(PREFIX + "SkyLight")
    sky_component = first_component(sky_light, unreal.SkyLightComponent)
    if sky_component:
        safe_set_property(sky_component, "mobility",
                          unreal.ComponentMobility.MOVABLE)
        safe_set_property(sky_component, "real_time_capture", True)

    # --- atmosphere -----------------------------------------------------------
    atmosphere = spawn_actor(unreal.SkyAtmosphere,
                             unreal.Vector(0.0, 0.0, 0.0))
    atmosphere.set_actor_label(PREFIX + "SkyAtmosphere")

    # --- volumetric haze (depth cue for the vista) ---------------------------
    fog = spawn_actor(unreal.ExponentialHeightFog,
                      unreal.Vector(0.0, 0.0, 0.0))
    fog.set_actor_label(PREFIX + "CinematicFog")
    fog_component = first_component(fog, unreal.ExponentialHeightFogComponent)
    if fog_component:
        safe_set_property(fog_component, "fog_density", FOG_DENSITY)
        safe_set_property(fog_component, "fog_height_falloff",
                          FOG_HEIGHT_FALLOFF)
        safe_set_property(fog_component, "start_distance", 2200.0)

    # --- exposure baseline (unbound post-process) -----------------------------
    ppv = spawn_actor(unreal.PostProcessVolume,
                      unreal.Vector(0.0, 0.0, 0.0))
    ppv.set_actor_label(PREFIX + "ExposurePPV")
    safe_set_property(ppv, "unbound", True)
    safe_set_property(ppv, "priority", 10.0)
    safe_set_property(ppv, "auto_exposure_bias", EXPOSURE_BIAS)

    log("Cinematic rig: golden-hour sun + real-time sky + atmosphere + "
        "volumetric haze (density {}, start 2200 cm) + exposure bias {}."
        .format(FOG_DENSITY, EXPOSURE_BIAS))


def main():
    if CUBE_MESH is None:
        raise RuntimeError(
            "Could not load /Engine/BasicShapes/Cube — run inside the "
            "ASTRAWILD project editor (UnrealEditor-Cmd -run=pythonscript).")
    log("Building {} — the day-one finisher ...".format(LEVEL_PATH))
    open_or_create_level()

    purged = purge_owned_actors()
    if purged:
        log("Purged {} stale {} actor(s) for idempotency."
            .format(purged, PREFIX))

    build_floor()
    build_vista()
    build_player_start()
    build_showcase_content()
    build_cinematic_rig()

    if not _level_api().save_current_level():
        raise RuntimeError("Failed to save level: " + LEVEL_PATH)
    log("Level saved: " + LEVEL_PATH)
    log("DONE — open L_Showcase_ArtOverhaul and press Play: the framed hero "
        "vista is the first frame.")


if __name__ == "__main__":
    try:
        main()
    except Exception as error:  # noqa: BLE001 — commandlet must report failure
        unreal.log_error("[L_Showcase] FAILED: {}".format(error))
        sys.exit(1)
