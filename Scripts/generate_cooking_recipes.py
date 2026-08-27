"""Generate the ASTRAWILD 30-recipe culinary database."""
from __future__ import annotations

import csv
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "Content/Astrawild/Data/Source/DT_CookingRecipes.csv"
HEADERS = [
    "Name", "RecipeTag", "DisplayName", "Description", "IngredientTags", "IngredientQuantities", "OutputItemTag", "OutputQuantity", "HungerRestored", "ThirstRestored", "NutritionValue", "SpoilageDurationSeconds", "RefrigeratedSpoilageRate", "BuffTag", "BuffMagnitude", "BuffDurationSeconds", "RequiredStation"
]

RECIPES = [
    ("SolarChili", "Solar Chili", "A warm pepper stew that preserves body heat.", "Item.Berry,Item.SolarOrgan", "2,1", "Item.Food.SolarChili", 1, 36, 4, 42, 1200, "Buff.ColdResistance", 0.20, 1200, "HeatForge"),
    ("GlacialSorbet", "Glacial Sorbet", "A crystal-cold dessert that cools an overheated traveler.", "Item.FrostFruit,Item.GlacialOrgan", "2,1", "Item.Food.GlacialSorbet", 1, 24, 18, 38, 1200, "Buff.HeatResistance", 0.20, 1200, "Campfire"),
    ("AstraDumpling", "Astra Dumpling", "A compact high-energy meal for dangerous expeditions.", "Item.Fiber,Item.AstraOrgan,Item.Grain", "1,1,2", "Item.Food.AstraDumpling", 1, 48, 8, 56, 900, "Buff.AttackPowerAndStamina", 0.30, 900, "HeatForge"),
    ("SanityCake", "Sanity Cake", "A calming cake required by advanced breeding farms.", "Item.Berry,Item.Grain,Item.LumenShard", "2,2,1", "Item.Food.SanityCake", 1, 32, 10, 50, 1800, "Buff.SANRecovery", 50, 900, "HeatForge"),
    ("HerbalBroth", "Herbal Broth", "Steaming broth made from fragrant medicinal leaves.", "Item.Herb,Item.Water", "3,1", "Item.Food.HerbalBroth", 1, 28, 18, 34, 1800, "Buff.SANRecovery", 18, 600, "Campfire"),
    ("EmberSkewer", "Ember Skewer", "Charred meat infused with a Solar ember.", "Item.Meat,Item.SolarOrgan", "2,1", "Item.Food.EmberSkewer", 1, 44, 2, 45, 900, "Buff.AttackPower", 0.12, 900, "Campfire"),
    ("RainleafSalad", "Rainleaf Salad", "Crisp leaves and dew fruit harvested after rain.", "Item.Leaf,Item.Berry,Item.Water", "3,2,1", "Item.Food.RainleafSalad", 1, 26, 22, 40, 900, "Buff.WorkSpeed", 0.15, 900, "Campfire"),
    ("StonegrainPorridge", "Stonegrain Porridge", "A dense porridge that steadies a miner's hands.", "Item.Grain,Item.GeoOrgan", "3,1", "Item.Food.StonegrainPorridge", 1, 40, 6, 44, 1800, "Buff.MiningEfficiency", 0.20, 1200, "HeatForge"),
    ("VoltPickle", "Volt Pickle", "Charged vegetables with a bright, sharp bite.", "Item.Vegetable,Item.VoltOrgan,Item.Vinegar", "2,1,1", "Item.Food.VoltPickle", 2, 18, 12, 26, 2400, "Buff.SprintRecovery", 0.18, 600, "Campfire"),
    ("AbyssalStew", "Abyssal Stew", "A dark stew whose aroma keeps nocturnal predators away.", "Item.Meat,Item.AbyssalOrgan,Item.Mushroom", "2,1,2", "Item.Food.AbyssalStew", 1, 52, 3, 48, 1200, "Buff.FearResistance", 0.25, 900, "HeatForge"),
    ("DawnFruitTart", "Dawn Fruit Tart", "A sweet tart baked with dawn fruit.", "Item.Berry,Item.Grain,Item.Honey", "3,2,1", "Item.Food.DawnFruitTart", 1, 38, 10, 44, 2400, "Buff.CaptureOdds", 0.10, 900, "HeatForge"),
    ("MireMushroomRice", "Mire Mushroom Rice", "Rice and mushrooms with a slow-release meal buff.", "Item.Grain,Item.Mushroom,Item.Water", "3,2,1", "Item.Food.MireMushroomRice", 1, 42, 14, 50, 1800, "Buff.HungerDrain", -0.20, 1200, "Campfire"),
    ("CrystalTea", "Crystal Tea", "A clear tea steeped beside an Astra crystal.", "Item.Herb,Item.AstraOrgan,Item.Water", "2,1,1", "Item.Food.CrystalTea", 2, 12, 28, 24, 2400, "Buff.SANRecovery", 35, 900, "Campfire"),
    ("FrostrootCasserole", "Frostroot Casserole", "Root vegetables baked with frost spice.", "Item.Root,Item.FrostFruit,Item.Grain", "2,1,2", "Item.Food.FrostrootCasserole", 1, 46, 9, 47, 1800, "Buff.HeatResistance", 0.12, 900, "HeatForge"),
    ("SunwoodSmokedMeat", "Sunwood Smoked Meat", "Meat smoked over aromatic Sunwood.", "Item.Meat,Item.SunwoodResin", "3,2", "Item.Food.SunwoodSmokedMeat", 1, 58, 0, 52, 3600, "Buff.MaxHealth", 0.10, 1200, "Campfire"),
    ("TidehornChowder", "Tidehorn Chowder", "A coastal chowder that restores thirst quickly.", "Item.Fish,Item.TidehornOrgan,Item.Water", "2,1,2", "Item.Food.TidehornChowder", 1, 34, 34, 48, 1200, "Buff.SwimSpeed", 0.25, 900, "HeatForge"),
    ("Mosswrap", "Mosswrap", "A portable wrap with balanced nutrition.", "Item.Leaf,Item.Meat,Item.Moss", "2,1,2", "Item.Food.Mosswrap", 2, 30, 8, 34, 900, "Buff.HarvestYield", 0.10, 900, "Campfire"),
    ("ThunderNoodle", "Thunder Noodle", "A spicy noodle dish that crackles on the tongue.", "Item.Grain,Item.VoltOrgan,Item.Vegetable", "2,1,2", "Item.Food.ThunderNoodle", 1, 40, 12, 45, 1200, "Buff.MoveSpeed", 0.12, 900, "HeatForge"),
    ("RainwaterJelly", "Rainwater Jelly", "A cool jelly made from clean rainwater.", "Item.Water,Item.Berry,Item.Gel", "2,2,1", "Item.Food.RainwaterJelly", 2, 20, 30, 30, 1800, "Buff.ThirstDrain", -0.25, 900, "Campfire"),
    ("GeoRootHash", "Geo-root Hash", "Crisped root vegetables with mineral salt.", "Item.Root,Item.GeoOrgan,Item.Oil", "3,1,1", "Item.Food.GeoRootHash", 1, 50, 4, 48, 1800, "Buff.KnockbackResistance", 0.20, 900, "HeatForge"),
    ("NightbloomPudding", "Nightbloom Pudding", "A moonlit pudding that quiets the mind.", "Item.Nightbloom,Item.Milk,Item.Grain", "2,1,2", "Item.Food.NightbloomPudding", 1, 34, 16, 42, 1800, "Buff.NightVision", 1.0, 900, "Campfire"),
    ("AbyssalPepperCurry", "Abyssal Pepper Curry", "A potent curry for abyssal expeditions.", "Item.Meat,Item.AbyssalOrgan,Item.Pepper", "2,1,2", "Item.Food.AbyssalPepperCurry", 1, 54, 2, 50, 900, "Buff.AbyssalResistance", 0.20, 900, "HeatForge"),
    ("AstraNectar", "Astra Nectar", "A rare nectar that sharpens partner resonance.", "Item.AstraOrgan,Item.Honey,Item.Water", "1,2,1", "Item.Food.AstraNectar", 1, 18, 24, 32, 1200, "Buff.PartnerSkillCooldown", -0.20, 900, "HeatForge"),
    ("CinderBread", "Cinder Bread", "A loaf baked on a volcanic stone.", "Item.Grain,Item.SolarOrgan,Item.Oil", "3,1,1", "Item.Food.CinderBread", 2, 36, 3, 38, 1800, "Buff.HeatResistance", 0.10, 900, "HeatForge"),
    ("GlacierMilkshake", "Glacier Milkshake", "A chilled shake that braces against heat.", "Item.Milk,Item.FrostFruit,Item.Ice", "2,2,1", "Item.Food.GlacierMilkshake", 1, 30, 26, 40, 1200, "Buff.HeatResistance", 0.18, 900, "Campfire"),
    ("LumenBisque", "Lumen Bisque", "A luminous soup served in sanctuary kitchens.", "Item.LumenShard,Item.Mushroom,Item.Water", "1,2,2", "Item.Food.LumenBisque", 1, 38, 20, 46, 1800, "Buff.CaptureOdds", 0.08, 900, "HeatForge"),
    ("DewberryCompote", "Dewberry Compote", "Sweet fruit reduced with fresh dew.", "Item.Berry,Item.Water", "4,2", "Item.Food.DewberryCompote", 2, 28, 24, 35, 900, "Buff.FarmingYield", 0.15, 900, "Campfire"),
    ("VolcanicRibs", "Volcanic Ribs", "Slow-cooked ribs glazed with ember resin.", "Item.Meat,Item.SolarOrgan,Item.Resin", "3,1,1", "Item.Food.VolcanicRibs", 1, 64, 0, 58, 1200, "Buff.AttackPower", 0.18, 900, "HeatForge"),
    ("GlacialHerbCongee", "Glacial Herb Congee", "Gentle congee for recovery after a cold night.", "Item.Grain,Item.Herb,Item.GlacialOrgan", "3,2,1", "Item.Food.GlacialHerbCongee", 1, 44, 14, 46, 1800, "Buff.HealthRecovery", 0.20, 900, "Campfire"),
    ("FirstDawnFeast", "First Dawn Feast", "A ceremonial feast reserved for the final sanctuary.", "Item.Meat,Item.AstraOrgan,Item.AncientAstraSchematic", "4,2,1", "Item.Food.FirstDawnFeast", 1, 100, 35, 100, 3600, "Buff.AllCoreStats", 0.15, 1200, "HeatForge"),
]


def generate() -> None:
    rows = []
    for index, (slug, display, description, ingredients, quantities, output, output_quantity, hunger, thirst, nutrition, spoilage, buff, magnitude, duration, station) in enumerate(RECIPES, start=1):
        rows.append({
            "Name": f"Cooking_{index:02d}_{slug}",
            "RecipeTag": f"Recipe.Cooking.{slug}",
            "DisplayName": display,
            "Description": description,
            "IngredientTags": f"({ingredients})",
            "IngredientQuantities": f"({quantities})",
            "OutputItemTag": output,
            "OutputQuantity": output_quantity,
            "HungerRestored": hunger,
            "ThirstRestored": thirst,
            "NutritionValue": nutrition,
            "SpoilageDurationSeconds": spoilage,
            "RefrigeratedSpoilageRate": 0.10,
            "BuffTag": f"{buff}",
            "BuffMagnitude": magnitude,
            "BuffDurationSeconds": duration,
            "RequiredStation": station,
        })
    if len(rows) != 30:
        raise RuntimeError(f"Expected 30 cooking recipes, generated {len(rows)}")
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    with OUTPUT.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=HEADERS)
        writer.writeheader()
        writer.writerows(rows)
    print(f"Generated {len(rows)} cooking recipes: {OUTPUT}")


if __name__ == "__main__":
    generate()
