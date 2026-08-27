# ASTRAWILD Crafting & Base Building Specification

## 1. Item Catalog (Vertical Slice)
| Item Tag | Display Name | Category | Max Stack | Description |
| :--- | :--- | :--- | :--- | :--- |
| `Item.Resource.Sunwood` | Sunwood Timber | Material | 99 | Dense, fibrous wood imbued with ambient sunlight. |
| `Item.Resource.LumenStone` | Lumen Stone | Material | 99 | Crystalline stone radiating soft primal light. |
| `Item.Resource.AstraShard` | Astra Shard | Material | 99 | A fragment of pure Astra harmonic energy. |
| `Item.Resource.DawnFiber` | Dawn Fiber | Material | 99 | Resilient plant fiber harvested from Dawn Flowers. |
| `Item.Tool.StoneAxe` | Primal Stone Axe | Tool | 1 | Basic axe for felling Sunwood trees efficiently. |
| `Item.Tool.StonePick` | Primal Stone Pick | Tool | 1 | Basic pick for quarrying Lumen Stone and Astra Shards. |
| `Item.Tool.AstraResonatorBasic` | Astra Resonator T1 | Consumable | 20 | Harmonic containment sphere for capturing wild Echoes. |
| `Item.Weapon.WoodClub` | Sunwood Bat | Weapon | 1 | Simple blunt weapon dealing 25 physical damage. |

---

## 2. Recipe Matrix (Crafting Bench & Handcraft)
| Recipe Tag | Station | Ingredients | Output | Craft Time |
| :--- | :--- | :--- | :--- | :--- |
| `Recipe.Tool.StoneAxe` | Hand / Bench | Sunwood x5, LumenStone x3 | Stone Axe x1 | 2.0s |
| `Recipe.Tool.StonePick` | Hand / Bench | Sunwood x5, LumenStone x3 | Stone Pick x1 | 2.0s |
| `Recipe.Weapon.WoodClub` | Hand / Bench | Sunwood x8 | Wood Club x1 | 2.5s |
| `Recipe.Tool.ResonatorT1` | Crafting Bench | AstraShard x1, LumenStone x2, Sunwood x3 | Astra Resonator T1 x1 | 3.0s |

---

## 3. Base Building Pieces
1. **Campfire (`Building.Campfire`)**:
   - Cost: Sunwood x10, LumenStone x5
   - Function: Provides light, warm radius, cooked food spot, and resting aura.
2. **Resting Shelter / Bed (`Building.RestBed`)**:
   - Cost: Sunwood x20, DawnFiber x10
   - Function: Fast forwards night to dawn, fully replenishes player Health and Stamina.
3. **Crafting Bench (`Building.CraftingBench`)**:
   - Cost: Sunwood x15, LumenStone x5
   - Function: Unlocks advanced tools and Astra Resonators.
4. **Storage Chest (`Building.StorageChest`)**:
   - Cost: Sunwood x15
   - Function: 24-slot persistent world container for surplus resources.