"""
_mock_unreal — a recording stub of the Unreal Editor Python API subset
=======================================================================
LONG-RUN DIRECTIVE L3: this package mimics ONLY the API surface the
ASTRAWILD Tools/Python scripts actually call, records every call, and
simulates enough editor state (assets, levels, actors, input objects)
for the tools' LOGIC to execute end-to-end inside Scripts/dryrun_unreal_tools.py.

It is NEVER imported by the real engine. The dry-run harness injects it as
`sys.modules["unreal"]` before importing the tools.

Every public symbol carries a class-level annotation:

    _UE_STATUS = ("VERIFIED", "<evidence>")     # confirmed in UE 5.x Python docs
    _UE_STATUS = ("UNVERIFIED", "<why / doc link to check>")

The harness prints this table and flags UNVERIFIED symbols for the
ON_PC conformance pass (first real engine run).
"""

import types

# ---------------------------------------------------------------------------
# Recording + simulated editor state
# ---------------------------------------------------------------------------

CALL_LOG = []          # entries: {"symbol": str, "args": tuple, "kwargs": dict}
MISSING_LOOKUPS = []   # attribute names tools touched that the mock lacks


def record(symbol, *args, **kwargs):
    CALL_LOG.append({
        "symbol": symbol,
        "args": tuple(_freeze(a) for a in args),
        "kwargs": {k: _freeze(v) for k, v in kwargs.items()},
    })
    return None


def _freeze(value):
    """Make a call argument comparable across runs. Value types freeze by
    VALUE; containers freeze RECURSIVELY; other mock objects freeze by
    class name (construction indices would break cross-run comparison by
    design — the stateful re-run keeps counting)."""
    if isinstance(value, Vector):
        return ("Vector", value.x, value.y, value.z)
    if isinstance(value, Rotator):
        return ("Rotator", value.pitch, value.yaw, value.roll)
    if isinstance(value, Key):
        return ("Key", value.key_name)
    if isinstance(value, _MockEnum):
        return (value.__class__.__name__, value.name)
    if isinstance(value, _MockObject):
        return ("<{}>".format(value.__class__.__name__),)
    if isinstance(value, type) and issubclass(value, _MockObject):
        return ("class", value.__name__)
    if isinstance(value, bool) or value is None or isinstance(value, (int, float, str)):
        return value
    if isinstance(value, (list, tuple)):
        return tuple(_freeze(x) for x in value)
    if isinstance(value, dict):
        return tuple((k, _freeze(v)) for k, v in sorted(value.items(),
                                                        key=repr))
    return repr(value)


class _MockState(object):
    """The simulated editor: assets on disk, loaded levels, live actors."""

    def __init__(self):
        self.assets = {}          # path -> asset object
        self.actor_seq = 0        # deterministic actor numbering
        self.actors = []          # live actors in the current level
        self.levels = {}          # path -> {"exists": True}
        self.engine_assets = {
            "/Engine/BasicShapes/Cube": "StaticMesh",
            "/Engine/BasicShapes/Sphere": "StaticMesh",
        }

    def reset(self):
        self.assets = {}
        self.actor_seq = 0
        self.actors = []
        self.levels = {}
        # engine assets survive reset (they ship with the engine)

    def snapshot(self):
        """Comparable end-state: asset paths + actor labels + frozen props."""
        return {
            "assets": sorted(self.assets.keys()),
            "levels": sorted(self.levels.keys()),
            "actors": sorted(
                (a.get_actor_label(),
                 tuple(sorted((k, _freeze(v))
                              for k, v in a._properties.items())))
                for a in self.actors
            ),
        }


STATE = _MockState()


class _MockMeta(type):
    """Metaclass marker (kept for symmetry; get_name is a hybrid descriptor
    on _MockObject so BOTH class objects and instances answer it — real UE
    class objects (UClass instances) and objects both expose get_name)."""

    pass


class _HybridGetName(object):
    """Descriptor returning a bound callable for BOTH instance access and
    class access (real UE exposes get_name on UClass objects too)."""

    def __get__(self, obj, objtype=None):
        if obj is None:
            def _class_name():
                record(objtype.__name__ + ".get_name")
                return objtype.__name__
            return _class_name

        def _inst_name():
            record(obj.__class__.__name__ + ".get_name")
            return "{}_{}".format(obj.__class__.__name__, obj._mock_index)
        return _inst_name


class _MockObject(object, metaclass=_MockMeta):
    """Base for every mock unreal object; owns a construction index and a
    property bag so set_editor_property/get_editor_property round-trip."""

    _mock_seq = 0
    _UE_STATUS = ("UNVERIFIED", "base mock class — not an engine symbol")
    get_name = _HybridGetName()

    def __init__(self, *args, **kwargs):
        _MockObject._mock_seq += 1
        self._mock_index = _MockObject._mock_seq
        self._properties = {}
        self._ctor_args = tuple(_freeze(a) for a in args)
        record("new:" + self.__class__.__name__, *self._ctor_args)

    # -- property protocol used by every tool --------------------------------
    def set_editor_property(self, name, value):
        record(self.__class__.__name__ + ".set_editor_property", name,
               _freeze(value))
        self._properties[name] = _freeze(value)
        return None

    def get_editor_property(self, name):
        record(self.__class__.__name__ + ".get_editor_property", name)
        return self._properties.get(name)

    def __repr__(self):
        return "<mock {} #{}>".format(self.__class__.__name__, self._mock_index)


# ---------------------------------------------------------------------------
# Value types
# ---------------------------------------------------------------------------

class Vector(_MockObject):
    _UE_STATUS = ("VERIFIED",
                  "unreal.Vector(x, y, z) — documented struct type, stable "
                  "across UE4/5 Python API")

    def __init__(self, x=0.0, y=0.0, z=0.0):
        super(Vector, self).__init__(x, y, z)
        self.x, self.y, self.z = float(x), float(y), float(z)

    def __eq__(self, other):
        return isinstance(other, Vector) and (self.x, self.y, self.z) == \
            (other.x, other.y, other.z)

    def __hash__(self):
        return hash((self.x, self.y, self.z))


class Rotator(_MockObject):
    _UE_STATUS = ("VERIFIED",
                  "unreal.Rotator(pitch, yaw, roll) — documented struct type")

    def __init__(self, pitch=0.0, yaw=0.0, roll=0.0):
        super(Rotator, self).__init__(pitch, yaw, roll)
        self.pitch, self.yaw, self.roll = float(pitch), float(yaw), float(roll)


class Key(_MockObject):
    _UE_STATUS = ("VERIFIED",
                  "unreal.Key('W') — documented struct (input key FNames)")

    def __init__(self, name="None"):
        super(Key, self).__init__(name)
        self.key_name = name


# ---------------------------------------------------------------------------
# Enums (UE enums expose class attributes)
# ---------------------------------------------------------------------------

class _MockEnum(object):
    _UE_STATUS = ("VERIFIED", "documented enum")

    def __init__(self, name):
        self.name = name

    def __repr__(self):
        return "{}.{}".format(self.__class__.__name__, self.name)

    def __eq__(self, other):
        return isinstance(other, self.__class__) and other.name == self.name

    def __hash__(self):
        return hash((self.__class__.__name__, self.name))


def _enum(name, members):
    cls = type(name, (_MockEnum,), {
        "_UE_STATUS": ("VERIFIED", "documented enum " + name),
    })
    for member in members:
        setattr(cls, member, cls(member))
    return cls


InputActionValueType = _enum("InputActionValueType",
                             ["BOOLEAN", "AXIS_1D", "AXIS_2D", "AXIS_3D"])
ComponentMobility = _enum("ComponentMobility", ["STATIC", "STATIONARY",
                                                "MOVABLE"])
InputAxisSwizzle = _enum("InputAxisSwizzle",
                         ["YXZ", "YZX", "ZXY", "ZYX", "XZY", "XYZ"])


# ---------------------------------------------------------------------------
# Module-level functions
# ---------------------------------------------------------------------------

def log(message):
    record("log", message)
    print("[mock-log] {}".format(message))


def log_warning(message):
    record("log_warning", message)
    print("[mock-warn] {}".format(message))


def log_error(message):
    record("log_error", message)
    print("[mock-error] {}".format(message))


def load_asset(path):
    record("load_asset", path)
    if path in STATE.engine_assets:
        return _MockObject("engine-asset")
    return STATE.assets.get(path)


def load_object(outer, path):
    record("load_object", outer, path)
    if path in STATE.assets:
        return STATE.assets[path]
    # Blueprint generated classes simulate as present after their BP exists
    base = path.rsplit(".", 1)[0]
    if base in STATE.assets:
        cls = type(path.rsplit(".", 1)[1], (_MockObject,), {})
        cls._UE_STATUS = ("UNVERIFIED",
                          "generated BP class resolution — engine behavior")
        return cls
    return None


def load_class(outer, path):
    record("load_class", outer, path)
    known = (
        "/Script/AstrawildCore.AstrawildGameMode",
        "/Script/AstrawildCore.AstrawildPlayerCharacter",
        "/Script/AstrawildCore.AstrawildEchoCharacter",
        "/Script/AstrawildCore.AstrawildWorldBootstrapper",
        "/Script/EnhancedInput.EnhancedInputComponent",
        "/Script/ProceduralMeshComponent.ProceduralMeshComponent",
    )
    if path in known:
        return type("Class_" + path.rsplit(".", 1)[1], (_MockObject,), {
            "_UE_STATUS": ("VERIFIED",
                           "project/engine class path present in Source")},
        )
    return None


def get_engine_version():
    record("get_engine_version")
    return "5.8.2-0+++UE5+Release-5.8"


def new_object(cls, outer=None, name=""):
    record("new_object", "class:" + cls.__name__, _freeze(outer), name)
    obj = cls()
    return obj


def get_default_object(cls):
    record("get_default_object", "class:" + cls.__name__)
    if not hasattr(cls, "_cdo"):
        cls._cdo = cls()
    return cls._cdo


def get_editor_subsystem(subsystem_class):
    record("get_editor_subsystem", "class:" + subsystem_class.__name__)
    if not hasattr(subsystem_class, "_subsystem"):
        subsystem_class._subsystem = subsystem_class()
    return subsystem_class._subsystem


# ---------------------------------------------------------------------------
# Actors & components
# ---------------------------------------------------------------------------

class _MockActor(_MockObject):
    def set_actor_label(self, label):
        record(self.__class__.__name__ + ".set_actor_label", label)
        self._label = label

    def get_actor_label(self):
        record(self.__class__.__name__ + ".get_actor_label")
        return getattr(self, "_label",
                       "{}_{}".format(self.__class__.__name__,
                                      self._mock_index))

    def get_components_by_class(self, component_class):
        record(self.__class__.__name__ + ".get_components_by_class",
               "class:" + component_class.__name__)
        return list(getattr(self, "_components", []))

    def destroy_actor(self):
        record(self.__class__.__name__ + ".destroy_actor")
        if self in STATE.actors:
            STATE.actors.remove(self)


class StaticMeshActor(_MockActor):
    _UE_STATUS = ("VERIFIED", "documented engine actor class")

    def __init__(self):
        super(StaticMeshActor, self).__init__()
        comp = StaticMeshComponent()
        self._components = [comp]
        self._properties["static_mesh_component"] = comp


class StaticMeshComponent(_MockObject):
    _UE_STATUS = ("VERIFIED", "documented component class")

    def set_static_mesh(self, mesh):
        record("StaticMeshComponent.set_static_mesh", _freeze(mesh))
        self._properties["static_mesh"] = _freeze(mesh)

    def set_relative_scale3d(self, scale):
        record("StaticMeshComponent.set_relative_scale3d", _freeze(scale))


class DirectionalLight(_MockActor):
    _UE_STATUS = ("VERIFIED", "documented engine actor class")

    def __init__(self):
        super(DirectionalLight, self).__init__()
        self._components = [DirectionalLightComponent()]


class DirectionalLightComponent(_MockObject):
    _UE_STATUS = ("VERIFIED", "documented component class")


class SkyLight(_MockActor):
    _UE_STATUS = ("VERIFIED", "documented engine actor class")

    def __init__(self):
        super(SkyLight, self).__init__()
        self._components = [SkyLightComponent()]


class SkyLightComponent(_MockObject):
    _UE_STATUS = ("VERIFIED", "documented component class")


class SkyAtmosphere(_MockActor):
    _UE_STATUS = ("VERIFIED", "documented engine actor class")


class ExponentialHeightFog(_MockActor):
    _UE_STATUS = ("VERIFIED", "documented engine actor class")

    def __init__(self):
        super(ExponentialHeightFog, self).__init__()
        self._components = [ExponentialHeightFogComponent()]


class ExponentialHeightFogComponent(_MockObject):
    _UE_STATUS = ("VERIFIED", "documented component class")


class PlayerStart(_MockActor):
    _UE_STATUS = ("VERIFIED", "documented engine actor class")


class WorldSettings(_MockActor):
    _UE_STATUS = ("VERIFIED", "documented engine actor class")

    def get_world_settings(self):
        record("WorldSettings.get_world_settings")
        return self


# ---------------------------------------------------------------------------
# Project classes (exist in Source/AstrawildCore)
# ---------------------------------------------------------------------------

class AstrawildGameMode(_MockObject):
    _UE_STATUS = ("VERIFIED",
                  "AAstrawildGameMode defined at "
                  "Source/AstrawildCore/Public/AstrawildGameMode.h — class "
                  "resolves once the module compiles (machine check)")


class AstrawildPlayerCharacter(_MockObject):
    _UE_STATUS = ("VERIFIED",
                  "AAstrawildPlayerCharacter defined at "
                  "Source/AstrawildCore/Public/AstrawildPlayerCharacter.h")


# ---------------------------------------------------------------------------
# Enhanced Input types
# ---------------------------------------------------------------------------

class InputAction(_MockObject):
    _UE_STATUS = ("VERIFIED", "documented EnhancedInput data asset class")


class InputMappingContext(_MockObject):
    _UE_STATUS = ("VERIFIED", "documented EnhancedInput data asset class")


class EnhancedActionKeyMapping(_MockObject):
    _UE_STATUS = ("VERIFIED",
                  "documented struct (mapping rows inside an IMC)")


class InputModifierNegate(_MockObject):
    _UE_STATUS = ("VERIFIED", "documented InputModifier class")


class InputModifierSwizzleAxis(_MockObject):
    _UE_STATUS = ("VERIFIED", "documented InputModifier class")


class InputModifierChordAction(_MockObject):
    _UE_STATUS = ("VERIFIED",
                  "documented InputModifier class (used by DCP-6 gamepad "
                  "smart-cast chord)")


class DataAssetFactory(_MockObject):
    _UE_STATUS = ("VERIFIED", "documented factory class for data assets")


class BlueprintFactory(_MockObject):
    _UE_STATUS = ("VERIFIED", "documented factory class for Blueprints")


class DataTableFactory(_MockObject):
    _UE_STATUS = ("VERIFIED",
                  "documented factory (L5 generate_datatables.py will use it)")


class RowStruct(_MockObject):
    _UE_STATUS = ("UNVERIFIED",
                  "unreal.RowStruct helper — check exact name in 5.8 docs "
                  "before the engine run")


# ---------------------------------------------------------------------------
# Editor libraries & subsystems (static-method classes)
# ---------------------------------------------------------------------------

class EditorAssetLibrary(object):
    _UE_STATUS = ("VERIFIED",
                  "unreal.EditorAssetLibrary — documented editor scripting "
                  "library (UE 5.x Editor Scripting)")

    @staticmethod
    def does_asset_exist(path):
        record("EditorAssetLibrary.does_asset_exist", path)
        return path in STATE.assets or path in STATE.engine_assets

    @staticmethod
    def does_directory_exist(path):
        record("EditorAssetLibrary.does_directory_exist", path)
        return True

    @staticmethod
    def load_asset(path):
        record("EditorAssetLibrary.load_asset", path)
        return load_asset(path)

    @staticmethod
    def save_loaded_asset(asset, only_if_is_dirty):
        record("EditorAssetLibrary.save_loaded_asset", _freeze(asset),
               only_if_is_dirty)
        return True

    @staticmethod
    def save_directory(path, only_if_is_dirty=True):
        record("EditorAssetLibrary.save_directory", path, only_if_is_dirty)
        return True


class AssetTools(object):
    _UE_STATUS = ("VERIFIED", "IAssetTools exposed via AssetToolsHelpers")

    @staticmethod
    def create_asset(name, package_path, asset_class, factory):
        record("AssetTools.create_asset", name, package_path,
               "class:" + asset_class.__name__ if asset_class else None,
               _freeze(factory))
        path = "{}/{}".format(package_path, name)
        asset = (asset_class or _MockObject)()
        STATE.assets[path] = asset
        return asset


class AssetToolsHelpers(object):
    _UE_STATUS = ("VERIFIED", "unreal.AssetToolsHelpers.get_asset_tools()")

    @staticmethod
    def get_asset_tools():
        record("AssetToolsHelpers.get_asset_tools")
        return AssetTools


class BlueprintEditorLibrary(object):
    _UE_STATUS = ("VERIFIED", "unreal.BlueprintEditorLibrary")

    @staticmethod
    def generated_class(blueprint):
        record("BlueprintEditorLibrary.generated_class", _freeze(blueprint))
        cls = type("BPGeneratedClass", (_MockObject,), {})
        return cls


class SystemLibrary(object):
    _UE_STATUS = ("VERIFIED", "unreal.SystemLibrary")

    @staticmethod
    def get_engine_version():
        record("SystemLibrary.get_engine_version")
        return get_engine_version()


class GameplayStatics(object):
    _UE_STATUS = ("VERIFIED", "unreal.GameplayStatics")

    @staticmethod
    def get_actor_of_class(world, actor_class):
        record("GameplayStatics.get_actor_of_class", _freeze(world),
               "class:" + actor_class.__name__)
        for actor in STATE.actors:
            if isinstance(actor, actor_class):
                return actor
        return None


class UnrealEditorSubsystem(_MockObject):
    _UE_STATUS = ("VERIFIED",
                  "unreal.UnrealEditorSubsystem — level API owner in UE5 "
                  "(EditorLevelLibrary deprecated path exists as fallback)")

    def new_level(self, path):
        record("UnrealEditorSubsystem.new_level", path)
        STATE.levels[path] = {"exists": True}
        STATE.actors = []
        return True

    def load_level(self, path):
        record("UnrealEditorSubsystem.load_level", path)
        if path not in STATE.levels:
            return False
        STATE.actors = []
        return True

    def save_current_level(self):
        record("UnrealEditorSubsystem.save_current_level")
        return True

    def get_editor_world(self):
        record("UnrealEditorSubsystem.get_editor_world")
        world = World()
        return world


class World(_MockObject):
    _UE_STATUS = ("VERIFIED", "unreal.World")

    def get_world_settings(self):
        record("World.get_world_settings")
        return WorldSettings()


class EditorActorSubsystem(_MockObject):
    _UE_STATUS = ("VERIFIED",
                  "unreal.EditorActorSubsystem — actor spawn/destroy API in UE5")

    def spawn_actor_from_class(self, actor_class, location, rotation=None):
        record("EditorActorSubsystem.spawn_actor_from_class",
               "class:" + actor_class.__name__, _freeze(location),
               _freeze(rotation))
        STATE.actor_seq += 1
        actor = actor_class()
        actor._properties["location"] = _freeze(location)
        STATE.actors.append(actor)
        return actor

    def get_all_level_actors(self):
        record("EditorActorSubsystem.get_all_level_actors")
        return list(STATE.actors)

    def destroy_actor(self, actor):
        record("EditorActorSubsystem.destroy_actor", _freeze(actor))
        if actor in STATE.actors:
            STATE.actors.remove(actor)
        return True


class EditorLevelLibrary(object):
    _UE_STATUS = ("VERIFIED",
                  "legacy library kept as engine-version fallback in the tools")

    @staticmethod
    def spawn_actor_from_class(actor_class, location, rotation=None):
        return EditorActorSubsystem().spawn_actor_from_class(
            actor_class, location, rotation)

    @staticmethod
    def get_all_level_actors():
        return EditorActorSubsystem().get_all_level_actors()

    @staticmethod
    def destroy_actor(actor):
        return EditorActorSubsystem().destroy_actor(actor)

    @staticmethod
    def new_level(path):
        return UnrealEditorSubsystem().new_level(path)

    @staticmethod
    def load_level(path):
        return UnrealEditorSubsystem().load_level(path)

    @staticmethod
    def save_current_level():
        return UnrealEditorSubsystem().save_current_level()

    @staticmethod
    def get_editor_world():
        return UnrealEditorSubsystem().get_editor_world()


class LevelEditorSubsystem(_MockObject):
    _UE_STATUS = ("VERIFIED",
                  "unreal.LevelEditorSubsystem — present for L5 tooling "
                  "(build_showcase_map.py camera/pie hooks)")


# ---------------------------------------------------------------------------
# Module export table + missing-lookup tracking
# ---------------------------------------------------------------------------

# module-level function annotations (function objects carry attributes set
# AFTER their def, not inside the body)
_FUNC_STATUS = {
    "log": ("VERIFIED", "unreal.log — documented module function"),
    "log_warning": ("VERIFIED", "unreal.log_warning — documented module function"),
    "log_error": ("VERIFIED", "unreal.log_error — documented module function"),
    "load_asset": ("VERIFIED", "unreal.load_asset — documented module function"),
    "load_object": ("VERIFIED", "unreal.load_object — documented module function"),
    "load_class": ("VERIFIED", "unreal.load_class — documented module function"),
    "get_engine_version": ("VERIFIED", "unreal.get_engine_version — documented module function"),
    "new_object": ("VERIFIED", "unreal.new_object — documented module function"),
    "get_default_object": ("VERIFIED", "unreal.get_default_object — documented module function"),
    "get_editor_subsystem": ("VERIFIED", "unreal.get_editor_subsystem — documented module function"),
}
for _fname, _st in _FUNC_STATUS.items():
    globals()[_fname]._UE_STATUS = _st

__all__ = [
    "Vector", "Rotator", "Key", "World",
    "InputActionValueType", "ComponentMobility", "InputAxisSwizzle",
    "log", "log_warning", "log_error",
    "load_asset", "load_object", "load_class", "get_engine_version",
    "new_object", "get_default_object", "get_editor_subsystem",
    "EditorAssetLibrary", "AssetToolsHelpers", "AssetTools",
    "BlueprintEditorLibrary", "SystemLibrary", "GameplayStatics",
    "UnrealEditorSubsystem", "EditorActorSubsystem", "EditorLevelLibrary",
    "LevelEditorSubsystem",
    "StaticMeshActor", "StaticMeshComponent", "DirectionalLight",
    "DirectionalLightComponent", "SkyLight", "SkyLightComponent",
    "SkyAtmosphere", "ExponentialHeightFog", "ExponentialHeightFogComponent",
    "PlayerStart", "WorldSettings",
    "AstrawildGameMode", "AstrawildPlayerCharacter",
    "InputAction", "InputMappingContext", "EnhancedActionKeyMapping",
    "InputModifierNegate", "InputModifierSwizzleAxis",
    "InputModifierChordAction", "DataAssetFactory", "BlueprintFactory",
    "DataTableFactory", "RowStruct",
]

_EXPORTS = {name: globals()[name] for name in __all__}


class _UnrealModule(types.ModuleType):
    """Module wrapper that records lookups of symbols the mock LACKS —
    tools' try/except blocks would otherwise silently swallow them."""

    def __getattr__(self, name):
        MISSING_LOOKUPS.append(name)
        raise AttributeError(
            "mock unreal has no symbol {!r} (recorded as MISSING)".format(
                name))


def _reset_all():
    """Full harness reset: call log, state, actor sequence, singletons."""
    global CALL_LOG, MISSING_LOOKUPS
    CALL_LOG = []
    MISSING_LOOKUPS = []
    STATE.reset()
    _MockObject._mock_seq = 0
    for cls in (UnrealEditorSubsystem, EditorActorSubsystem):
        if hasattr(cls, "_subsystem"):
            del cls._subsystem
    for obj in list(globals().values()):
        if isinstance(obj, type) and hasattr(obj, "_cdo"):
            del obj._cdo


def install():
    """Return the recording module object to inject as sys.modules['unreal']."""
    mod = _UnrealModule("unreal")
    for name, value in _EXPORTS.items():
        setattr(mod, name, value)
    mod._reset_all = _reset_all
    mod._STATE = STATE
    mod._CALL_LOG = lambda: CALL_LOG
    mod._MISSING_LOOKUPS = lambda: list(MISSING_LOOKUPS)
    mod._freeze = _freeze
    return mod
