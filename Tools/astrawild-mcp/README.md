# ASTRAWILD Unreal Engine 5.8 MCP Server

This **Model Context Protocol (MCP)** Server allows **Manus AI**, Antigravity, Claude, or any AI coding agent to directly inspect, query, audit, scaffold, and develop systems for **ASTRAWILD: Echoes of the First Dawn** (Unreal Engine 5.8).

---

## 🚀 Quick Setup for Manus AI / Agent Config

Add this MCP server definition into your Manus / MCP Client configuration file (e.g. `mcp.json` or `claude_desktop_config.json`):

```json
{
  "mcpServers": {
    "astrawild-unreal": {
      "command": "node",
      "args": [
        "c:\\Users\\saisu\\OneDrive - kmutnb.ac.th\\Documents\\game\\Tools\\astrawild-mcp\\server.js"
      ]
    }
  }
}
```

---

## 🛠️ Available MCP Tools for Manus AI

| Tool Name | Description | Example Arguments |
| :--- | :--- | :--- |
| `astrawild_get_build_status` | Returns the current build status, active Git branch, latest commit, and live core loop mechanics. | `{}` |
| `astrawild_code_audit` | Scans all C++ headers & sources in `AstrawildCore` for syntax, brace/paren balance, and `.generated.h` includes. | `{ "verbose": false }` |
| `astrawild_inspect_echo` | Returns complete creature stats, elemental affinities, roles, speeds, and abilities for species (Pyrelite, Thornback, Aquavine, or all). | `{ "speciesTag": "Echo.Pyrelite" }` |
| `astrawild_inspect_items_recipes` | Returns all registered item definitions, crafting recipe costs, and building structures. | `{ "category": "all" }` |
| `astrawild_create_echo_template` | Generates C++ Data Asset initialization code for creating a new Echo species. | `{ "speciesName": "Frostlynx", "element": "Glacial", "role": "Combat", "baseHealth": 380 }` |
| `astrawild_create_recipe_template` | Generates recipe definition struct and ingredient specifications for a new tool or weapon. | `{ "recipeName": "Primal Bow", "outputItemTag": "Item.Weapon.PrimalBow", "ingredients": [{ "tag": "Item.Resource.Sunwood", "qty": 8 }] }` |
| `astrawild_run_performance_check` | Returns frame budget breakdown (Game Thread, Render Thread, GPU time) and memory footprint. | `{}` |
| `astrawild_validate_save_file` | Inspects and validates save game schema v1 profiles (PlayerProfile, WorldSnapshot, Settings). | `{ "slotName": "Slot_01" }` |

---

## 💻 Manual Terminal Run / Testing

To verify the MCP server locally:
```powershell
node "c:\Users\saisu\OneDrive - kmutnb.ac.th\Documents\game\Tools\astrawild-mcp\server.js"
```