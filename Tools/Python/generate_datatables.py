"""
generate_datatables.py — LONG-RUN DIRECTIVE L5.1
=================================================
Turns Design/design_data.json into real UE DataTable assets — one per
domain WHERE A REFLECTED ROW STRUCT EXISTS TODAY (R1/R2 discipline: no
invented struct names, no fabricated APIs):

    /Game/ASTRAWILD/DataTables/DT_AstrawildAbilities   53 rows -> FAstrawildAbilityData
    /Game/ASTRAWILD/DataTables/DT_AstrawildWeather      8 rows -> FAstrawildWeatherProfile
    /Game/ASTRAWILD/DataTables/DT_AstrawildZones       12 rows -> FAstrawildZoneDescriptor

Domains with NO reflected row struct today (bestiary/items/recipes/species/
buildings/...) are STAGED as import-ready JSON under Design/DataTable/ with
a TODO_ASK_OWNER marker: authoring new C++ USTRUCT row shapes is an owner
decision (the runtime registry already owns those values — a DataTable
twin without a consumer would be dead data).

Row values are never hand-typed: they are read from Design/design_data.json,
which itself is {file, line}-traced to Source/ (see Design/README.md).

Idempotent: existing DataTable assets are loaded and re-filled
deterministically; the fill empties and rebuilds every row.

UNVERIFIED — NEEDS MACHINE (R2/R3): the exact 5.8 Python spellings
`unreal.DataTable`, `unreal.DataTableFactory.struct`, and
`unreal.DataTableFunctionLibrary.fill_data_table_from_json_string` are
documented UE5 editor-scripting surfaces; the first engine run is the
conformance test (ON_PC hour 0 item).

Run headless (Windows):
  "E:\\Epic Games\\UnrealEngine\\Engine\\Binaries\\Win64\\UnrealEditor-Cmd.exe" ^
    "E:\\AstrawildGame\\ASTRAWILD.uproject" ^
    -run=pythonscript -script="E:\\AstrawildGame\\Tools\\Python\\generate_datatables.py" ^
    -stdout -unattended -nopause -nosplash
"""

import json
import os
import sys

import unreal

DT_DIR = "/Game/ASTRAWILD/DataTables"
DESIGN_JSON = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(
    os.path.abspath(__file__)))), "Design", "design_data.json")

PREFIX = "[DTGen]"


def log(message):
    unreal.log("{} {}".format(PREFIX, message))


def log_warn(message):
    unreal.log_warning("{} {}".format(PREFIX, message))


# ---------------------------------------------------------------------------
# Row-struct mappings — field names BELOW (left) are the C++ UPROPERTY names
# read from the headers (AstrawildTypes.h:1149 FAstrawildAbilityData,
# AstrawildWeatherSubsystem.h FAstrawildWeatherProfile,
# AstrawildZoneSubsystem.h FAstrawildZoneDescriptor); keys (right) are the
# design_data.json domain keys. Nothing invented.
# ---------------------------------------------------------------------------

TABLES = [
    {
        "asset": "DT_AstrawildAbilities",
        "row_struct": "AstrawildAbilityData",
        "domain": "abilities",
        "row_key": "id",
        "fields": {
            "AbilityId": "id",
            "DisplayName": "name",
            "Description": "description",
            "Category": "category",
            "Element": "element",
            "Power": "power",
            "CooldownSeconds": "cooldown",
            "Range": "range",
            "UnlockLevel": "unlock_level",
            "StatusId": "status_id",
            "StatusSeconds": "status_seconds",
            "StatusSpeedMultiplier": "status_speed",
        },
    },
    {
        "asset": "DT_AstrawildWeather",
        "row_struct": "AstrawildWeatherProfile",
        "domain": "weather",
        "row_key": "state",
        "fields": {
            "TemperatureOffset": "temp_offset",
            "SelectionWeight": "weight",
            "VisibilityMultiplier": "visibility",
        },
    },
    {
        "asset": "DT_AstrawildZones",
        "row_struct": "AstrawildZoneDescriptor",
        "domain": "zones",
        "row_key": "id",
        "fields": {
            "Zone": "zone",
            "ZoneId": "id",
            "DisplayName": "name",
            "Subtitle": "subtitle",
            "GroundTint": "tint",
            "AmbientLightColor": "light_color",
            "BaseHeight": "base",
            "HeightAmplitude": "amplitude",
            "RidgeBlend": "ridge",
            "ThreatLevel": "threat",
            "HazardType": "hazard",
            "HazardPressure": "hazard_pressure",
        },
        # Bounds (FBox2D) intentionally left at default: the runtime zone
        # subsystem owns world-rect placement (center + 800m cells).
        "skip_fields_note": ["Bounds"],
    },
]

# Domains staged for a future C++ row-struct decision (owner territory).
STAGED_DOMAINS = [
    "bestiary", "species", "items", "recipes", "buildings", "weapons",
    "technologies", "quests", "npcs", "loot_tables", "world_events", "pois",
    "resource_nodes", "work_sites", "robots", "dialogue_trees",
    "hunt_contracts", "mutations", "dungeons", "bosses",
]


def color(value):
    """[r, g, b(, a)] list -> importer-friendly nested object."""
    if not isinstance(value, (list, tuple)):
        return value
    out = {"R": float(value[0]), "G": float(value[1]), "B": float(value[2])}
    out["A"] = float(value[3]) if len(value) > 3 else 1.0
    return out


def build_rows(spec, entries):
    rows = {}
    for entry in entries:
        key = entry.get(spec["row_key"])
        if not key:
            continue
        row = {}
        for struct_field, design_key in spec["fields"].items():
            value = entry.get(design_key)
            if value is None:
                continue
            if struct_field in ("GroundTint", "AmbientLightColor"):
                value = color(value)
            row[struct_field] = value
        rows[str(key)] = row
    return rows


def get_row_struct_class(name):
    """Resolve the USTRUCT's Python class (F prefix stripped by UE)."""
    cls = getattr(unreal, name, None)
    if cls is None:
        raise RuntimeError(
            "row struct unreal.{} not found — the AstrawildCore module must "
            "be compiled first (ON_PC hour 1)".format(name))
    return cls


def make_data_table(spec, rows):
    path = "{}/{}".format(DT_DIR, spec["asset"])
    factory = unreal.DataTableFactory()
    row_struct = get_row_struct_class(spec["row_struct"])
    try:
        factory.set_editor_property("struct", row_struct)
    except Exception as exc:  # noqa: BLE001 — surface as actionable failure
        raise RuntimeError(
            "could not set DataTableFactory.struct to {} ({}): verify the "
            "5.8 factory property name on the engine run".format(
                spec["row_struct"], exc))

    if unreal.EditorAssetLibrary.does_asset_exist(path):
        table = unreal.EditorAssetLibrary.load_asset(path)
        log("Loaded existing {}".format(spec["asset"]))
    else:
        table = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            spec["asset"], DT_DIR, unreal.DataTable, factory)
        if table is None:
            raise RuntimeError("Failed to create DataTable: " + path)
        log("Created {}".format(spec["asset"]))

    payload = json.dumps(rows)
    ok = unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(
        table, payload)
    if not ok:
        raise RuntimeError(
            "fill_data_table_from_json_string failed for {} ({} rows)".format(
                spec["asset"], len(rows)))
    unreal.EditorAssetLibrary.save_loaded_asset(table, False)
    log("{}: {} rows filled from design_data.json (traced).".format(
        spec["asset"], len(rows)))
    return table


def stage_unstructured_domains(domains):
    """Write import-ready JSON for domains lacking a reflected row struct.
    These are TODO_ASK_OWNER: a C++ USTRUCT row shape + a consumer must be
    authored before a DataTable twin is anything but dead data."""
    out_dir = os.path.join(os.path.dirname(DESIGN_JSON), "DataTable")
    os.makedirs(out_dir, exist_ok=True)
    staged = []
    for domain in STAGED_DOMAINS:
        dom = domains.get(domain)
        if not dom:
            continue
        entries = dom.get("entries") or dom.get("species") or []
        if not entries:
            continue
        out_path = os.path.join(out_dir, domain + ".json")
        with open(out_path, "w", encoding="utf-8") as fh:
            json.dump({"TODO_ASK_OWNER":
                       "no reflected USTRUCT row shape exists for this "
                       "domain yet; author one in Source/ + a consumer, "
                       "then add the mapping to TABLES in "
                       "Tools/Python/generate_datatables.py",
                       "rows": entries}, fh, indent=1, ensure_ascii=False)
        staged.append("{} ({} rows)".format(domain, len(entries)))
    if staged:
        log_warn("STAGED for owner decision (no row struct yet): " +
                 "; ".join(staged))


def main():
    if not os.path.exists(DESIGN_JSON):
        raise RuntimeError(
            DESIGN_JSON + " missing — run Scripts/extract_design_data.py "
            "first (sandbox-safe, no engine needed)")
    with open(DESIGN_JSON, encoding="utf-8") as fh:
        doc = json.load(fh)
    domains = doc["domains"]

    log("Generating DataTable assets from traced design data ...")
    made = []
    for spec in TABLES:
        dom = domains.get(spec["domain"]) or {}
        entries = dom.get("entries") or dom.get("species") or []
        if not entries:
            log_warn("domain '{}' empty in design_data.json — skipping {}"
                     .format(spec["domain"], spec["asset"]))
            continue
        rows = build_rows(spec, entries)
        make_data_table(spec, rows)
        made.append("{}={}".format(spec["asset"], len(rows)))

    stage_unstructured_domains(domains)

    log("DONE — " + ", ".join(made) +
        ". Staged JSON lives in Design/DataTable/ (TODO_ASK_OWNER).")


if __name__ == "__main__":
    try:
        main()
    except Exception as error:  # noqa: BLE001 — commandlet must report failure
        unreal.log_error("[DTGen] FAILED: {}".format(error))
        sys.exit(1)
