# ASTRAWILD: Save/Load System Test Checklist & Verification Protocol

**Subsystem**: `UAstrawildSaveSubsystem`  
**Save Class**: `UAstrawildSaveGame`  
**Current Schema Version**: `v1`  
**Target Engine**: Unreal Engine 5.8  

---

## 1. Schema Architecture & Profile Separation

| Profile | Serialized Fields | Key Invariants & Safeguards |
| :--- | :--- | :--- |
| **`FAstrawildPlayerProfile`** | Transform, HP, MaxHP, SP, MaxSP, Level, EXP, 30 Inventory Slots, Active Party Echoes, Storage Echoes, Respawn Anchor Transform. | Quantity clamped between 0 and 999. Health clamped $\ge 1.0\text{f}$. Duplicate GUIDs deduplicated. |
| **`FAstrawildWorldSnapshot`** | Timestamp, Placed Building Pieces (GUID, Tag, Transform, HP, Container Inventory), Harvest Nodes (GUID, depleted state, respawn timer). | Dead buildings ($\text{HP} \le 0$) purged automatically. |
| **`FAstrawildSettingsProfile`** | Scalability Tier (0-3), Master/Music/SFX Volume, Mouse Sensitivity, Invert Y. | Validated against engine config ranges. |

---

## 2. Test Cases & Verification Matrix

### ✅ Test 1: Full Inventory & State Persistence
- **Action**: Chop Sunwood (15), Mine LumenStone (8), Obtain Astra Resonators (3). Rest at Campfire. Execute `Astrawild_SaveGame Slot_01`. Close game session. Reload PIE. Execute `Astrawild_LoadGame Slot_01`.
- **Expected Result**: Player spawns at saved coordinates with identical 30-slot inventory, full health, and active companion party. Zero item duplication.
- **Status**: **PASS**

### ✅ Test 2: Atomic Safe Write & Backup Rollback
- **Action**: Trigger save on existing slot. Corrupt or delete primary slot file. Execute `Astrawild_LoadGame Slot_01`.
- **Expected Result**: Subsystem detects primary slot failure, falls back to `Slot_01_Backup`, restores complete world snapshot, and renders red/orange alert banner: *"Primary Save Corrupt: Restored from Backup!"*.
- **Status**: **PASS**

### ✅ Test 3: Anti-Duplication & Item Quantity Sanitization
- **Action**: Inject negative quantity slot or stacked item $> 999$.
- **Expected Result**: `ValidateAndSanitize()` cleans negative items to empty slot, and clamps over-stack items to 999.
- **Status**: **PASS**

### ✅ Test 4: Schema Migration
- **Action**: Load a `v0` legacy save file.
- **Expected Result**: `MigrateSchema(1)` runs seamlessly, upgrading profile structures to `v1` without data loss.
- **Status**: **PASS**

### ✅ Test 5: Rest Point Respawn Anchor Restoration
- **Action**: Place Rest Bed in Sanctuary, interact to set respawn point. Save and reload.
- **Expected Result**: Character respawn point is retained in `PlayerProfile.ActiveRespawnTransform`.
- **Status**: **PASS**

---

## 3. In-Game Debug Console Commands

- `Astrawild_SaveGame <SlotName>`: Manually saves the complete game snapshot.
- `Astrawild_LoadGame <SlotName>`: Loads and restores game state with backup fallback.
- `Astrawild_GiveItem <ItemTag> <Quantity>`: Debug inject resources to test inventory limits.