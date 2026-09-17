"""
setup_input_assets.py — ENGINE-RUN-1 / TASK 3
================================================
Generates the editor-side Enhanced Input asset layer for
AAstrawildPlayerCharacter, in exact parity with the C++ runtime fallback
(BuildRuntimeInputDefaults / BuildGamepadInputDefaults):

  /Game/ASTRAWILD/Input/IA_*            32 InputActions (value types match)
  /Game/ASTRAWILD/Input/IMC_Player      full Keyboard+Mouse mapping context
  /Game/ASTRAWILD/Input/IMC_Gamepad     full gamepad companion context
  /Game/ASTRAWILD/Blueprints/BP_AstrawildPlayer   pawn BP, defaults bound to
                                        the generated actions/contexts
  /Game/ASTRAWILD/Blueprints/BP_AstrawildGameMode game-mode BP whose
                                        DefaultPawn is BP_AstrawildPlayer

The generated assets are OPTIONAL: with nothing assigned the C++ character
builds the identical mapping at runtime (zero-asset playability). This
script materializes the same contract as real assets so designers can
retune keys in the editor without touching code.

Idempotent: existing assets are loaded, their mappings/defaults are
rebuilt deterministically, and everything is saved. Run after
build_prototype_map.py (or before — order does not matter; the map script
picks BP_AstrawildGameMode up when present).

Run headless (Windows, engine at E:\\Epic Games\\UnrealEngine):
  "E:\\Epic Games\\UnrealEngine\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" ^
    "E:\\AstrawildGame\\ASTRAWILD.uproject" ^
    -run=pythonscript -script="E:\\AstrawildGame\\Tools\\Python\\setup_input_assets.py" ^
    -stdout -unattended -nopause -nosplash
"""

import sys
import unreal

INPUT_DIR = "/Game/ASTRAWILD/Input"
BP_DIR = "/Game/ASTRAWILD/Blueprints"

BOOLEAN = unreal.InputActionValueType.BOOLEAN
AXIS_1D = unreal.InputActionValueType.AXIS_1D
AXIS_2D = unreal.InputActionValueType.AXIS_2D

# ---------------------------------------------------------------------------
# Action table — (asset name, C++ UPROPERTY on AAstrawildPlayerCharacter,
# value type). Key bindings live in the context tables below.
# ---------------------------------------------------------------------------

ACTIONS = [
    ("IA_Move", "MoveAction", AXIS_2D),
    ("IA_Look", "LookAction", AXIS_2D),
    ("IA_Sprint", "SprintAction", BOOLEAN),
    ("IA_Jump", "JumpAction", BOOLEAN),
    ("IA_Interact", "InteractAction", BOOLEAN),
    ("IA_LightAttack", "AttackAction", BOOLEAN),
    ("IA_HeavyAttack", "HeavyAttackAction", BOOLEAN),
    ("IA_Dodge", "DodgeAction", BOOLEAN),
    ("IA_Block", "BlockAction", BOOLEAN),
    ("IA_Command", "CommandAction", BOOLEAN),
    ("IA_Feed", "FeedAction", BOOLEAN),
    ("IA_BuildMode", "BuildModeAction", BOOLEAN),
    ("IA_BuildRotate", "BuildRotateAction", BOOLEAN),
    ("IA_BuildCycle", "BuildCycleAction", AXIS_1D),
    ("IA_Consume", "ConsumeAction", BOOLEAN),
    ("IA_EquipBest", "EquipBestAction", BOOLEAN),
    ("IA_DeleteBuilding", "DeleteBuildingAction", BOOLEAN),
    ("IA_Save", "SaveAction", BOOLEAN),
    ("IA_Load", "LoadAction", BOOLEAN),
    ("IA_Scan", "ScanAction", BOOLEAN),
    ("IA_DeployDrone", "DeployDroneAction", BOOLEAN),
    ("IA_DeployRobot", "DeployRobotAction", BOOLEAN),
    ("IA_Inventory", "InventoryAction", BOOLEAN),
    ("IA_Research", "ResearchAction", BOOLEAN),
    ("IA_Journal", "JournalAction", BOOLEAN),
    ("IA_Roster", "RosterAction", BOOLEAN),
    ("IA_Map", "MapAction", BOOLEAN),
    ("IA_Hunt", "HuntAction", BOOLEAN),
    ("IA_Pause", "PauseAction", BOOLEAN),
    ("IA_Descend", "DescendAction", BOOLEAN),
    ("IA_PartyAbility", "PartyAbilityAction", BOOLEAN),
    ("IA_PlayerSkill", "PlayerSkillAction", BOOLEAN),
]

# Modifier shorthands used by the binding tables.
SWIZZLE_YXZ = "swizzle_yxz"
NEGATE_Y = "negate_y"
NEGATE_X = "negate_x"
CHORD_LEFT_SHOULDER = "chord_left_shoulder"

# KB/M context — (action asset, key, [modifiers]); 1:1 with
# BuildRuntimeInputDefaults().
KBM_MAPPINGS = [
    ("IA_Move", "W", [SWIZZLE_YXZ]),
    ("IA_Move", "S", [SWIZZLE_YXZ, NEGATE_Y]),
    ("IA_Move", "D", []),
    ("IA_Move", "A", [NEGATE_X]),
    ("IA_Look", "Mouse2D", [NEGATE_Y]),
    ("IA_Sprint", "LeftShift", []),
    ("IA_Jump", "SpaceBar", []),
    ("IA_Interact", "E", []),
    ("IA_LightAttack", "LeftMouseButton", []),
    ("IA_HeavyAttack", "F", []),
    ("IA_Dodge", "Q", []),
    ("IA_Block", "RightMouseButton", []),
    ("IA_Command", "C", []),
    ("IA_Feed", "R", []),
    ("IA_BuildMode", "B", []),
    ("IA_BuildRotate", "N", []),
    ("IA_BuildCycle", "MouseWheelAxis", []),
    ("IA_Consume", "G", []),
    ("IA_EquipBest", "X", []),
    ("IA_DeleteBuilding", "Z", []),
    ("IA_Save", "F5", []),
    ("IA_Load", "F9", []),
    ("IA_Scan", "V", []),
    ("IA_DeployDrone", "H", []),
    ("IA_DeployRobot", "J", []),
    ("IA_Inventory", "Tab", []),
    ("IA_Research", "K", []),
    ("IA_Journal", "P", []),
    ("IA_Roster", "L", []),
    ("IA_Map", "M", []),
    ("IA_Hunt", "U", []),
    ("IA_Pause", "Escape", []),
    ("IA_Descend", "LeftControl", []),
    ("IA_PartyAbility", "T", []),
    ("IA_PlayerSkill", "Y", []),
]

# Gamepad context — 1:1 with BuildGamepadInputDefaults().
GAMEPAD_MAPPINGS = [
    ("IA_Move", "Gamepad_Left2D", []),
    ("IA_Look", "Gamepad_Right2D", []),
    ("IA_Jump", "Gamepad_FaceButton_Bottom", []),
    ("IA_Interact", "Gamepad_FaceButton_Right", []),
    ("IA_Dodge", "Gamepad_FaceButton_Left", []),
    ("IA_BuildMode", "Gamepad_FaceButton_Top", []),
    ("IA_Sprint", "Gamepad_RightShoulder", []),
    ("IA_Block", "Gamepad_LeftShoulder", []),
    ("IA_LightAttack", "Gamepad_RightTrigger", []),
    ("IA_HeavyAttack", "Gamepad_LeftTrigger", []),
    ("IA_Command", "Gamepad_DPad_Up", []),
    ("IA_PlayerSkill", "Gamepad_FaceButton_Left", [CHORD_LEFT_SHOULDER]),
    ("IA_PartyAbility", "Gamepad_RightThumbstick", []),
    ("IA_Descend", "Gamepad_LeftThumbstick", []),
    ("IA_Feed", "Gamepad_DPad_Right", []),
    ("IA_Consume", "Gamepad_DPad_Down", []),
    ("IA_EquipBest", "Gamepad_DPad_Left", []),
    ("IA_BuildRotate", "Gamepad_Special_Left", []),
    ("IA_Pause", "Gamepad_Special_Right", []),
]


def log(message):
    unreal.log("[InputSetup] {}".format(message))


def safe_set(obj, *names, value=None):
    """Sets the first property name that sticks (snake_case / native)."""
    for name in names:
        try:
            obj.set_editor_property(name, value)
            return True
        except Exception:
            continue
    log("WARNING: could not set {} on {}".format("/".join(names), obj.get_name()))
    return False


def asset_path(name, directory):
    return "{}/{}".format(directory, name)


# ---------------------------------------------------------------------------
# Asset creation / loading
# ---------------------------------------------------------------------------


def get_asset_tools():
    return unreal.AssetToolsHelpers.get_asset_tools()


def create_or_load(name, directory, asset_class, factory):
    path = asset_path(name, directory)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.EditorAssetLibrary.load_asset(path)
        log("Loaded existing {}".format(name))
        return asset
    asset = get_asset_tools().create_asset(name, directory, asset_class, factory)
    if asset is None:
        raise RuntimeError("Failed to create asset: " + path)
    log("Created {}".format(name))
    return asset


def make_input_action(name):
    """Creates or loads an InputAction asset with the right value type."""
    factory = None
    try:
        factory = unreal.DataAssetFactory()
    except Exception:
        factory = None  # older engines: create_asset accepts the class directly
    action = create_or_load(name, INPUT_DIR, unreal.InputAction, factory)
    value_type = dict((a[0], a[2]) for a in ACTIONS)[name]
    safe_set(action, "value_type", value=value_type)
    return action


def make_modifier(kind, outer, index):
    """Builds one modifier subobject for a mapping."""
    name = "{}_{:03d}".format(kind, index)
    if kind == SWIZZLE_YXZ:
        modifier = unreal.new_object(unreal.InputModifierSwizzleAxis, outer, name)
        safe_set(modifier, "order", value=unreal.InputAxisSwizzle.YXZ)
        return modifier
    if kind == NEGATE_Y:
        modifier = unreal.new_object(unreal.InputModifierNegate, outer, name)
        safe_set(modifier, "b_y", "bY", value=True)
        safe_set(modifier, "b_x", "bX", value=False)
        safe_set(modifier, "b_z", "bZ", value=False)
        return modifier
    if kind == NEGATE_X:
        modifier = unreal.new_object(unreal.InputModifierNegate, outer, name)
        safe_set(modifier, "b_x", "bX", value=True)
        safe_set(modifier, "b_y", "bY", value=False)
        safe_set(modifier, "b_z", "bZ", value=False)
        return modifier
    if kind == CHORD_LEFT_SHOULDER:
        modifier = unreal.new_object(unreal.InputModifierChordAction, outer, name)
        safe_set(modifier, "key", value=unreal.Key("Gamepad_LeftShoulder"))
        return modifier
    raise RuntimeError("Unknown modifier kind: " + kind)


def make_mapping_context(name, mappings, actions_by_name):
    """Creates or loads an IMC and rebuilds its mapping list deterministically."""
    factory = None
    try:
        factory = unreal.DataAssetFactory()
    except Exception:
        factory = None
    context = create_or_load(name, INPUT_DIR, unreal.InputMappingContext, factory)

    rebuilt = []
    for index, (action_name, key_name, modifier_kinds) in enumerate(mappings):
        mapping = unreal.EnhancedActionKeyMapping()
        safe_set(mapping, "action", value=actions_by_name[action_name])
        safe_set(mapping, "key", value=unreal.Key(key_name))
        modifiers = [make_modifier(kind, context, index) for kind in modifier_kinds]
        safe_set(mapping, "modifiers", value=modifiers)
        rebuilt.append(mapping)

    safe_set(context, "mappings", value=rebuilt)
    log("{}: {} mappings rebuilt.".format(name, len(rebuilt)))
    return context


# ---------------------------------------------------------------------------
# Blueprint wiring
# ---------------------------------------------------------------------------


def blueprint_class(asset_name):
    """Returns the generated class of a saved Blueprint asset."""
    package = asset_path(asset_name, BP_DIR)
    return unreal.load_object(None, "{}.{}_C".format(package, asset_name))


def make_pawn_blueprint(actions_by_name, kbm_context, gamepad_context):
    factory = unreal.BlueprintFactory()
    safe_set(factory, "parent_class", value=unreal.AstrawildPlayerCharacter)
    blueprint = create_or_load("BP_AstrawildPlayer", BP_DIR, None, factory)

    # The CDO carries the defaults a spawned pawn will use.
    pawn_class = unreal.BlueprintEditorLibrary.generated_class(blueprint)
    if not pawn_class:
        pawn_class = blueprint_class("BP_AstrawildPlayer")
    if not pawn_class:
        raise RuntimeError("BP_AstrawildPlayer has no generated class.")
    defaults = unreal.get_default_object(pawn_class)

    safe_set(defaults, "default_mapping_context", value=kbm_context)
    safe_set(defaults, "gamepad_mapping_context", value=gamepad_context)
    for action_name, property_name, _ in ACTIONS:
        safe_set(defaults, _snake(property_name), property_name,
                 value=actions_by_name[action_name])

    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)
    log("BP_AstrawildPlayer defaults bound ({} actions + 2 contexts)."
        .format(len(ACTIONS)))
    return pawn_class


def make_game_mode_blueprint(pawn_class):
    factory = unreal.BlueprintFactory()
    safe_set(factory, "parent_class", value=unreal.AstrawildGameMode)
    blueprint = create_or_load("BP_AstrawildGameMode", BP_DIR, None, factory)

    mode_class = unreal.BlueprintEditorLibrary.generated_class(blueprint)
    if not mode_class:
        mode_class = blueprint_class("BP_AstrawildGameMode")
    if not mode_class:
        raise RuntimeError("BP_AstrawildGameMode has no generated class.")
    defaults = unreal.get_default_object(mode_class)

    safe_set(defaults, "default_pawn_class", value=pawn_class)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)
    log("BP_AstrawildGameMode DefaultPawn = BP_AstrawildPlayer.")
    return mode_class


def _snake(name):
    """CamelCase -> snake_case for set_editor_property name candidates."""
    out = []
    for char in name:
        if char.isupper():
            out.append("_")
        out.append(char.lower())
    return "".join(out).lstrip("_")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------


def main():
    log("Generating Enhanced Input asset layer …")

    actions = {}
    for action_name, _, _ in ACTIONS:
        actions[action_name] = make_input_action(action_name)

    kbm_context = make_mapping_context("IMC_Player", KBM_MAPPINGS, actions)
    gamepad_context = make_mapping_context("IMC_Gamepad", GAMEPAD_MAPPINGS, actions)

    pawn_class = make_pawn_blueprint(actions, kbm_context, gamepad_context)
    make_game_mode_blueprint(pawn_class)

    unreal.EditorAssetLibrary.save_directory(INPUT_DIR, only_if_is_dirty=False)
    unreal.EditorAssetLibrary.save_directory(BP_DIR, only_if_is_dirty=False)

    log("DONE — {} actions, IMC_Player ({} maps), IMC_Gamepad ({} maps), "
        "BP_AstrawildPlayer + BP_AstrawildGameMode saved."
        .format(len(ACTIONS), len(KBM_MAPPINGS), len(GAMEPAD_MAPPINGS)))


if __name__ == "__main__":
    try:
        main()
    except Exception as error:  # noqa: BLE001 — commandlet must report failure
        unreal.log_error("[InputSetup] FAILED: {}".format(error))
        sys.exit(1)
