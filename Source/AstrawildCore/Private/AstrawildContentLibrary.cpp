#include "AstrawildContentLibrary.h"

#include "AstrawildDataAssets.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "Engine/World.h"

// ---------------------------------------------------------------------------
// Helpers — terse definition builders (data-driven in spirit; values migrate
// to .uasset data assets through the asset pipeline later).
// ---------------------------------------------------------------------------

namespace
{
    UAstrawildItemDefinition* MakeItem(UObject* Outer, FName Id, const FString& Name, EAstrawildItemCategory Category, float Weight, int32 MaxStack)
    {
        UAstrawildItemDefinition* Item = NewObject<UAstrawildItemDefinition>(Outer);
        Item->ItemId = Id;
        Item->DisplayName = FText::FromString(Name);
        Item->Description = FText::FromString(FString::Printf(TEXT("CODE_DEFAULT — %s"), *Name));
        Item->Category = Category;
        Item->Weight = Weight;
        Item->MaxStackSize = MaxStack;
        return Item;
    }

    UAstrawildRecipeDefinition* MakeRecipe(UObject* Outer, FName Id, const FString& Name, const TArray<FAstrawildItemStack>& Inputs, const TArray<FAstrawildItemStack>& Outputs, float Duration, FName Tech, FName Station)
    {
        UAstrawildRecipeDefinition* Recipe = NewObject<UAstrawildRecipeDefinition>(Outer);
        Recipe->RecipeId = Id;
        Recipe->DisplayName = FText::FromString(Name);
        Recipe->Ingredients = Inputs;
        Recipe->Outputs = Outputs;
        Recipe->CraftDurationSeconds = Duration;
        Recipe->RequiredTechId = Tech;
        Recipe->RequiredStationId = Station;
        return Recipe;
    }

    FAstrawildItemStack Stack(FName Id, int32 Qty)
    {
        FAstrawildItemStack S;
        S.ItemId = Id;
        S.Quantity = Qty;
        return S;
    }

    UAstrawildEchoDefinition* MakeEcho(UObject* Outer, FName Id, const FString& Name, EAstrawildElementType Element, EAstrawildEchoRole Role,
        float HP, float ATK, float DEF, float Speed, EAstrawildPersonality Personality, EAstrawildActivityPattern Pattern,
        const TArray<FName>& Food, float CaptureDifficulty, EAstrawildElementType Weakness, bool bHostile)
    {
        UAstrawildEchoDefinition* Echo = NewObject<UAstrawildEchoDefinition>(Outer);
        Echo->DefinitionId = Id;
        Echo->DisplayName = FText::FromString(Name);
        Echo->Description = FText::FromString(FString::Printf(TEXT("CODE_DEFAULT Echo — %s"), *Name));
        Echo->Element = Element;
        Echo->Role = Role;
        Echo->BaseStats.MaxHealth = HP;
        Echo->BaseStats.AttackPower = ATK;
        Echo->BaseStats.Defense = DEF;
        Echo->BaseStats.MoveSpeed = Speed;
        Echo->BaseStats.CaptureResilience = FMath::Clamp(CaptureDifficulty * 0.8f, 0.05f, 0.95f);
        Echo->DominantPersonality = Personality;
        Echo->ActivityPattern = Pattern;
        Echo->PreferredFoodIds = Food;
        Echo->CaptureDifficulty = CaptureDifficulty;
        Echo->WeaknessElement = Weakness;
        Echo->bHostileToPlayers = bHostile;
        Echo->TrustGainOnCapture = 10.0f;
        return Echo;
    }

    UAstrawildBuildingDefinition* MakeBuilding(UObject* Outer, FName Id, const FString& Name, EAstrawildBuildingCategory Category,
        FName RequiredItem, int32 ItemCount, FName Tech, float HP, EAstrawildPowerRole PowerRole, float Generation, float Draw, float Battery, EAstrawildWorkType WorkType)
    {
        UAstrawildBuildingDefinition* Building = NewObject<UAstrawildBuildingDefinition>(Outer);
        Building->DefinitionId = Id;
        Building->DisplayName = FText::FromString(Name);
        Building->Category = Category;
        Building->RequiredItemId = RequiredItem;
        Building->RequiredItemCount = ItemCount;
        Building->RequiredTechId = Tech;
        Building->MaxHealth = HP;
        Building->PowerRole = PowerRole;
        Building->PowerGeneration = Generation;
        Building->PowerDraw = Draw;
        Building->BatteryCapacity = Battery;
        Building->EnabledWorkType = WorkType;
        return Building;
    }

    UAstrawildTechnologyDefinition* MakeTech(UObject* Outer, FName Id, const FString& Name, EAstrawildTechEra Era, int32 Cost, const TArray<FName>& Prereqs, const TArray<FName>& Recipes, const TArray<FName>& Buildings)
    {
        UAstrawildTechnologyDefinition* Tech = NewObject<UAstrawildTechnologyDefinition>(Outer);
        Tech->TechId = Id;
        Tech->DisplayName = FText::FromString(Name);
        Tech->Description = FText::FromString(FString::Printf(TEXT("CODE_DEFAULT tech — %s"), *Name));
        Tech->Era = Era;
        Tech->ResearchCost = Cost;
        Tech->PrerequisiteTechIds = Prereqs;
        Tech->UnlockedRecipeIds = Recipes;
        Tech->UnlockedBuildingIds = Buildings;
        return Tech;
    }
}

// ---------------------------------------------------------------------------
// Items
// ---------------------------------------------------------------------------
void UAstrawildContentLibrary::BuildItems(UAstrawildItemRegistrySubsystem* Registry)
{
    UObject* Outer = Registry;

    Registry->RegisterItem(MakeItem(Outer, TEXT("Item_Wood"), TEXT("Dawnwood"), EAstrawildItemCategory::Material, 0.5f, 200));
    Registry->RegisterItem(MakeItem(Outer, TEXT("Item_Stone"), TEXT("Fieldstone"), EAstrawildItemCategory::Material, 0.8f, 200));
    Registry->RegisterItem(MakeItem(Outer, TEXT("Item_Fiber"), TEXT("Sunfiber"), EAstrawildItemCategory::Material, 0.1f, 200));
    Registry->RegisterItem(MakeItem(Outer, TEXT("Item_WoodPlank"), TEXT("Dawnwood Plank"), EAstrawildItemCategory::Material, 1.0f, 100));
    Registry->RegisterItem(MakeItem(Outer, TEXT("Item_CrystalShard"), TEXT("Dawn Crystal Shard"), EAstrawildItemCategory::Material, 0.3f, 100));

    UAstrawildItemDefinition* Berry = MakeItem(Outer, TEXT("Item_Berry"), TEXT("Glimmer Berry"), EAstrawildItemCategory::Consumable, 0.2f, 50);
    Berry->FoodValue = 15.0f;
    Berry->WaterValue = 5.0f;
    Berry->EchoFeedValue = 6.0f;
    Berry->VendorPrice = 2; // Batch 4 — M-11: Trader Tam ware.
    Registry->RegisterItem(Berry);

    UAstrawildItemDefinition* RawMeat = MakeItem(Outer, TEXT("Item_RawMeat"), TEXT("Raw Echo Meat"), EAstrawildItemCategory::Consumable, 0.7f, 30);
    RawMeat->FoodValue = 8.0f;
    RawMeat->EchoFeedValue = 5.0f;
    Registry->RegisterItem(RawMeat);

    UAstrawildItemDefinition* CookedMeat = MakeItem(Outer, TEXT("Item_CookedMeat"), TEXT("Seared Meat"), EAstrawildItemCategory::Consumable, 0.6f, 30);
    CookedMeat->FoodValue = 30.0f;
    Registry->RegisterItem(CookedMeat);

    UAstrawildItemDefinition* WaterFlask = MakeItem(Outer, TEXT("Item_WaterFlask"), TEXT("Dew Flask"), EAstrawildItemCategory::Consumable, 0.9f, 20);
    WaterFlask->WaterValue = 40.0f;
    WaterFlask->VendorPrice = 2; // Batch 4 — M-11: Trader Tam ware.
    Registry->RegisterItem(WaterFlask);

    UAstrawildItemDefinition* Bandage = MakeItem(Outer, TEXT("Item_Bandage"), TEXT("Sunfiber Bandage"), EAstrawildItemCategory::Consumable, 0.2f, 30);
    Bandage->HealValue = 40.0f;
    Bandage->VendorPrice = 3; // Batch 4 — M-11: Trader Tam ware.
    Registry->RegisterItem(Bandage);

    UAstrawildItemDefinition* Resonator = MakeItem(Outer, TEXT("Item_Resonator"), TEXT("Echo Resonator"), EAstrawildItemCategory::CreatureItem, 0.4f, 20);
    Resonator->VendorPrice = 6; // Batch 4 — M-11: Trader Tam ware (capture resupply).
    Registry->RegisterItem(Resonator);
    Registry->RegisterItem(MakeItem(Outer, TEXT("Item_AncientCore"), TEXT("Ancient Core"), EAstrawildItemCategory::QuestItem, 1.0f, 10));

    // --- Content expansion (CODE_DEFAULT wave 2) ---

    UAstrawildItemDefinition* Dawnbloom = MakeItem(Outer, TEXT("Item_Dawnbloom"), TEXT("Dawnbloom Petal"), EAstrawildItemCategory::Material, 0.1f, 100);
    Dawnbloom->Description = FText::FromString(TEXT("A luminous petal shed by Spriglings. Basis of dawn-field remedies."));
    Registry->RegisterItem(Dawnbloom);

    UAstrawildItemDefinition* EmberAsh = MakeItem(Outer, TEXT("Item_EmberAsh"), TEXT("Ember Ash"), EAstrawildItemCategory::Material, 0.2f, 50);
    EmberAsh->Description = FText::FromString(TEXT("Still-warm residue left behind by Emberfangs."));
    Registry->RegisterItem(EmberAsh);

    UAstrawildItemDefinition* FeedMix = MakeItem(Outer, TEXT("Item_FeedMix"), TEXT("Echo Feed Mix"), EAstrawildItemCategory::Consumable, 0.3f, 40);
    FeedMix->FoodValue = 5.0f;
    FeedMix->EchoFeedValue = 14.0f;
    Registry->RegisterItem(FeedMix);

    UAstrawildItemDefinition* HerbalSalve = MakeItem(Outer, TEXT("Item_HerbalSalve"), TEXT("Dawnbloom Salve"), EAstrawildItemCategory::Consumable, 0.25f, 20);
    HerbalSalve->HealValue = 70.0f;
    HerbalSalve->VendorPrice = 4; // Batch 4 — M-11: Trader Tam ware.
    Registry->RegisterItem(HerbalSalve);

    // --- Vendor economy currency (CODE_DEFAULT wave 6, Batch 4 — M-11) ---

    // Trade currency for NPC vendors. VendorPrice stays 0: the currency can
    // neither be bought with itself nor sold back — shards enter the world only
    // from the dungeon boss loot table, the Dawn Guard quest reward and the
    // prototype starter kit.
    UAstrawildItemDefinition* DawnShard = MakeItem(Outer, TEXT("Item_DawnShard"), TEXT("Dawn Shard"), EAstrawildItemCategory::Material, 0.1f, 200);
    DawnShard->Description = FText::FromString(TEXT("A humming sliver of dawnlight. Vendors in the fields accept it as currency."));
    Registry->RegisterItem(DawnShard);

    // --- Equipment (CODE_DEFAULT wave 3): weapon + shield progression. ---

    UAstrawildItemDefinition* DawnwoodClub = MakeItem(Outer, TEXT("Item_DawnwoodClub"), TEXT("Dawnwood Club"), EAstrawildItemCategory::Equipment, 2.5f, 1);
    DawnwoodClub->AttackPower = 6.0f;
    Registry->RegisterItem(DawnwoodClub);

    UAstrawildItemDefinition* StonehideShield = MakeItem(Outer, TEXT("Item_StonehideShield"), TEXT("Stonehide Shield"), EAstrawildItemCategory::Equipment, 4.0f, 1);
    StonehideShield->BlockMitigation = 0.65f;
    Registry->RegisterItem(StonehideShield);

    UAstrawildItemDefinition* CrystalBlade = MakeItem(Outer, TEXT("Item_CrystalBlade"), TEXT("Dawn Crystal Blade"), EAstrawildItemCategory::Equipment, 3.0f, 1);
    CrystalBlade->AttackPower = 14.0f;
    // Batch 3 — Item A: tier-3 weapon carries the Pulse element → attacks apply Shock.
    CrystalBlade->Element = EAstrawildElementType::Pulse;
    Registry->RegisterItem(CrystalBlade);

    // --- Armor (CODE_DEFAULT wave 5, Batch 3 — Item C): torso progression. ---
    // Rating feeds ComputeArmorFraction(Rating, K=100) → 17% / 31% / 44% reduction.

    UAstrawildItemDefinition* FiberWeaveVest = MakeItem(Outer, TEXT("Item_FiberWeaveVest"), TEXT("Fiberweave Vest"), EAstrawildItemCategory::Equipment, 3.0f, 1);
    FiberWeaveVest->ArmorRating = 20.0f;
    Registry->RegisterItem(FiberWeaveVest);

    UAstrawildItemDefinition* EmberhideJacket = MakeItem(Outer, TEXT("Item_EmberhideJacket"), TEXT("Emberhide Jacket"), EAstrawildItemCategory::Equipment, 5.0f, 1);
    EmberhideJacket->ArmorRating = 45.0f;
    Registry->RegisterItem(EmberhideJacket);

    UAstrawildItemDefinition* CrystalplateCuirass = MakeItem(Outer, TEXT("Item_CrystalplateCuirass"), TEXT("Crystalplate Cuirass"), EAstrawildItemCategory::Equipment, 8.0f, 1);
    CrystalplateCuirass->ArmorRating = 80.0f;
    Registry->RegisterItem(CrystalplateCuirass);

    // --- Ecosystem expansion (CODE_DEFAULT wave 7, Batch 5): Frost/Pulse loot
    //     materials + thermal/agriculture goods for the three new tech nodes. ---

    UAstrawildItemDefinition* Frostbloom = MakeItem(Outer, TEXT("Item_Frostbloom"), TEXT("Frostbloom"), EAstrawildItemCategory::Material, 0.2f, 50);
    Frostbloom->Description = FText::FromString(TEXT("A cold-blooming petal harvested from Rimefang territory."));
    Frostbloom->VendorPrice = 3; // Batch 5: hostile-harvest income + Warm Broth ingredient.
    Registry->RegisterItem(Frostbloom);

    UAstrawildItemDefinition* VoltCore = MakeItem(Outer, TEXT("Item_VoltCore"), TEXT("Volt Core"), EAstrawildItemCategory::Material, 0.4f, 30);
    VoltCore->Description = FText::FromString(TEXT("A still-humming core looted from Voltmaws. Powers resonator batches."));
    VoltCore->VendorPrice = 4; // Batch 5: hostile-harvest income + Mechanics ingredient.
    Registry->RegisterItem(VoltCore);

    UAstrawildItemDefinition* WarmBroth = MakeItem(Outer, TEXT("Item_WarmBroth"), TEXT("Hearth Broth"), EAstrawildItemCategory::Consumable, 0.6f, 20);
    WarmBroth->FoodValue = 18.0f;
    WarmBroth->WaterValue = 12.0f;
    WarmBroth->HealValue = 10.0f;
    WarmBroth->Description = FText::FromString(TEXT("A steaming bowl that warms cold-night expeditions."));
    WarmBroth->VendorPrice = 3; // Batch 5: Tech_Thermal consumable.
    Registry->RegisterItem(WarmBroth);

    UAstrawildItemDefinition* Fertilizer = MakeItem(Outer, TEXT("Item_Fertilizer"), TEXT("Dawnfield Fertilizer"), EAstrawildItemCategory::Material, 0.3f, 50);
    Fertilizer->Description = FText::FromString(TEXT("Composted dawn-field matter. Farm plots thrive on it."));
    Fertilizer->VendorPrice = 2; // Batch 5: Tech_Agriculture crafted good.
    Registry->RegisterItem(Fertilizer);

    // --- Dungeon reward (CODE_DEFAULT wave 8, Batch 6): the Ancient-era weapon
    //     forged from the Hollow Underlight warden's Ancient Core. Light element
    //     — it counters the Ash-element warden (weakness ×1.5).

    UAstrawildItemDefinition* AncientResonator = MakeItem(Outer, TEXT("Item_AncientResonator"), TEXT("Ancient Resonator"), EAstrawildItemCategory::Equipment, 3.0f, 1);
    AncientResonator->AttackPower = 18.0f;
    AncientResonator->Element = EAstrawildElementType::Light;
    AncientResonator->Description = FText::FromString(TEXT("A resonance blade humming with First Dawn light. The Underlight warden's bane."));
    AncientResonator->VendorPrice = 8; // Batch 6: dungeon-economy sellable.
    Registry->RegisterItem(AncientResonator);

    // --- Advanced technology framework (final production run — PHASE 12): scanner,
    //     energy cells, laser weapon, helmet, exosuit, robotics. Every piece feeds
    //     a real system slot — no cosmetic content. ---

    UAstrawildItemDefinition* EnergyCell = MakeItem(Outer, TEXT("Item_EnergyCell"), TEXT("Pulse Cell"), EAstrawildItemCategory::Material, 0.15f, 60);
    EnergyCell->Description = FText::FromString(TEXT("Compressed pulse energy. Ammunition for resonance weaponry."));
    EnergyCell->VendorPrice = 1;
    Registry->RegisterItem(EnergyCell);

    UAstrawildItemDefinition* FieldScanner = MakeItem(Outer, TEXT("Item_FieldScanner"), TEXT("Field Scanner"), EAstrawildItemCategory::Equipment, 1.5f, 1);
    FieldScanner->EquipmentSlot = EAstrawildEquipmentSlot::Scanner;
    FieldScanner->ScannerSpeedMultiplier = 3.0f;
    FieldScanner->Description = FText::FromString(TEXT("Hold [V] to scan — observation knowledge accrues three times faster."));
    FieldScanner->VendorPrice = 10;
    Registry->RegisterItem(FieldScanner);

    UAstrawildItemDefinition* ResonanceHelm = MakeItem(Outer, TEXT("Item_ResonanceHelm"), TEXT("Resonance Helm"), EAstrawildItemCategory::Equipment, 2.5f, 1);
    ResonanceHelm->EquipmentSlot = EAstrawildEquipmentSlot::Helmet;
    ResonanceHelm->ArmorRating = 35.0f;
    ResonanceHelm->InsulationRating = 6.0f;
    ResonanceHelm->Description = FText::FromString(TEXT("Crystal-weave helm: armor plus six degrees of thermal tolerance."));
    ResonanceHelm->VendorPrice = 12;
    Registry->RegisterItem(ResonanceHelm);

    UAstrawildItemDefinition* Exosuit = MakeItem(Outer, TEXT("Item_DawnstriderExosuit"), TEXT("Dawnstrider Exosuit"), EAstrawildItemCategory::Equipment, 9.0f, 1);
    Exosuit->EquipmentSlot = EAstrawildEquipmentSlot::Exosuit;
    Exosuit->InsulationRating = 8.0f;
    Exosuit->StaminaRegenBonus = 6.0f;
    Exosuit->CarryWeightBonus = 40.0f;
    Exosuit->MoveSpeedBonus = 0.15f;
    Exosuit->Description = FText::FromString(TEXT("A resonance-frame exosuit: +40 kg carry, +15% speed, faster stamina, thermal lining."));
    Exosuit->VendorPrice = 18;
    Registry->RegisterItem(Exosuit);

    UAstrawildItemDefinition* PulseLance = MakeItem(Outer, TEXT("Item_PulseLance"), TEXT("Pulse Lance"), EAstrawildItemCategory::Equipment, 4.0f, 1);
    PulseLance->AttackPower = 16.0f;
    PulseLance->Element = EAstrawildElementType::Pulse;
    PulseLance->EquipmentSlot = EAstrawildEquipmentSlot::Weapon;
    PulseLance->bIsRangedWeapon = true;
    PulseLance->AmmoItemId = TEXT("Item_EnergyCell");
    PulseLance->Description = FText::FromString(TEXT("A resonance lance firing pulse bolts [LMB]. Shock status on every hit."));
    PulseLance->VendorPrice = 20;
    Registry->RegisterItem(PulseLance);

    UAstrawildItemDefinition* UtilityDrone = MakeItem(Outer, TEXT("Item_UtilityDrone"), TEXT("Utility Drone"), EAstrawildItemCategory::Equipment, 6.0f, 1);
    UtilityDrone->EquipmentSlot = EAstrawildEquipmentSlot::Auto; // Deployable — not a worn slot.
    UtilityDrone->bDeploysDrone = true;
    UtilityDrone->Description = FText::FromString(TEXT("Deploy [H]: a hovering companion that scans creatures and auto-harvests nearby nodes."));
    UtilityDrone->VendorPrice = 22;
    Registry->RegisterItem(UtilityDrone);

    UAstrawildItemDefinition* UtilityRobot = MakeItem(Outer, TEXT("Item_UtilityRobot"), TEXT("Utility Robot"), EAstrawildItemCategory::Equipment, 10.0f, 1);
    UtilityRobot->EquipmentSlot = EAstrawildEquipmentSlot::Auto; // Deployable — not a worn slot.
    UtilityRobot->bDeploysRobot = true;
    UtilityRobot->Description = FText::FromString(TEXT("Deploy [J]: mans the nearest unmanned work site — steady output, no feeding, power-gated."));
    UtilityRobot->VendorPrice = 26;
    Registry->RegisterItem(UtilityRobot);
}

// ---------------------------------------------------------------------------
// Recipes
// ---------------------------------------------------------------------------
void UAstrawildContentLibrary::BuildRecipes(UAstrawildItemRegistrySubsystem* Registry)
{
    UObject* Outer = Registry;

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_Resonator"), TEXT("Echo Resonator"),
        { Stack(TEXT("Item_Stone"), 2), Stack(TEXT("Item_Fiber"), 1) },
        { Stack(TEXT("Item_Resonator"), 1) }, 3.0f, NAME_None, NAME_None));

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_Bandage"), TEXT("Sunfiber Bandage"),
        { Stack(TEXT("Item_Fiber"), 2) },
        { Stack(TEXT("Item_Bandage"), 1) }, 2.0f, NAME_None, NAME_None));

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_WoodPlank"), TEXT("Dawnwood Plank"),
        { Stack(TEXT("Item_Wood"), 2) },
        { Stack(TEXT("Item_WoodPlank"), 1) }, 2.0f, NAME_None, TEXT("Station_Workbench")));

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_CookedMeat"), TEXT("Seared Meat"),
        { Stack(TEXT("Item_RawMeat"), 1) },
        { Stack(TEXT("Item_CookedMeat"), 1) }, 5.0f, TEXT("Tech_Cooking"), TEXT("Station_Campfire")));

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_WaterFlask"), TEXT("Dew Flask"),
        { Stack(TEXT("Item_Fiber"), 2), Stack(TEXT("Item_CrystalShard"), 1) },
        { Stack(TEXT("Item_WaterFlask"), 1) }, 4.0f, NAME_None, TEXT("Station_Workbench")));

    // --- Content expansion (CODE_DEFAULT wave 2: husbandry economy) ---

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_FeedMix"), TEXT("Echo Feed Mix"),
        { Stack(TEXT("Item_Berry"), 2), Stack(TEXT("Item_Fiber"), 1) },
        { Stack(TEXT("Item_FeedMix"), 1) }, 4.0f, TEXT("Tech_Husbandry"), TEXT("Station_Campfire")));

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_HerbalSalve"), TEXT("Dawnbloom Salve"),
        { Stack(TEXT("Item_Dawnbloom"), 2), Stack(TEXT("Item_Fiber"), 1) },
        { Stack(TEXT("Item_HerbalSalve"), 1) }, 4.0f, TEXT("Tech_Husbandry"), TEXT("Station_Workbench")));

    // --- Equipment (CODE_DEFAULT wave 3): armory progression ---

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_DawnwoodClub"), TEXT("Dawnwood Club"),
        { Stack(TEXT("Item_Wood"), 3), Stack(TEXT("Item_Fiber"), 1) },
        { Stack(TEXT("Item_DawnwoodClub"), 1) }, 3.0f, NAME_None, TEXT("Station_Workbench")));

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_StonehideShield"), TEXT("Stonehide Shield"),
        { Stack(TEXT("Item_Stone"), 3), Stack(TEXT("Item_Wood"), 2), Stack(TEXT("Item_Fiber"), 1) },
        { Stack(TEXT("Item_StonehideShield"), 1) }, 5.0f, TEXT("Tech_Armory"), TEXT("Station_Workbench")));

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_CrystalBlade"), TEXT("Dawn Crystal Blade"),
        { Stack(TEXT("Item_CrystalShard"), 2), Stack(TEXT("Item_WoodPlank"), 2), Stack(TEXT("Item_EmberAsh"), 1) },
        { Stack(TEXT("Item_CrystalBlade"), 1) }, 8.0f, TEXT("Tech_Armory"), TEXT("Station_Workbench")));

    // --- Armor (CODE_DEFAULT wave 5, Batch 3 — Item C): armory progression. ---

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_FiberWeaveVest"), TEXT("Fiberweave Vest"),
        { Stack(TEXT("Item_Fiber"), 4), Stack(TEXT("Item_Wood"), 1) },
        { Stack(TEXT("Item_FiberWeaveVest"), 1) }, 4.0f, TEXT("Tech_Armory"), TEXT("Station_Workbench")));

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_EmberhideJacket"), TEXT("Emberhide Jacket"),
        { Stack(TEXT("Item_EmberAsh"), 2), Stack(TEXT("Item_WoodPlank"), 2), Stack(TEXT("Item_Fiber"), 2) },
        { Stack(TEXT("Item_EmberhideJacket"), 1) }, 6.0f, TEXT("Tech_Armory"), TEXT("Station_Workbench")));

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_CrystalplateCuirass"), TEXT("Crystalplate Cuirass"),
        { Stack(TEXT("Item_CrystalShard"), 3), Stack(TEXT("Item_WoodPlank"), 3), Stack(TEXT("Item_EmberAsh"), 2) },
        { Stack(TEXT("Item_CrystalplateCuirass"), 1) }, 9.0f, TEXT("Tech_Armory"), TEXT("Station_Workbench")));

    // --- Ecosystem expansion (CODE_DEFAULT wave 7, Batch 5): recipes for the
    //     Mechanics / Thermal / Agriculture tech nodes. ---

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_PlankBatch"), TEXT("Bulk Dawnwood Planks"),
        { Stack(TEXT("Item_Wood"), 10) },
        { Stack(TEXT("Item_WoodPlank"), 6) }, 6.0f, TEXT("Tech_Mechanics"), TEXT("Station_Workbench")));

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_ResonatorBatch"), TEXT("Resonator Batch"),
        { Stack(TEXT("Item_CrystalShard"), 2), Stack(TEXT("Item_VoltCore"), 1), Stack(TEXT("Item_Fiber"), 2) },
        { Stack(TEXT("Item_Resonator"), 3) }, 8.0f, TEXT("Tech_Mechanics"), TEXT("Station_Workbench")));

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_WarmBroth"), TEXT("Hearth Broth"),
        { Stack(TEXT("Item_RawMeat"), 1), Stack(TEXT("Item_Frostbloom"), 1), Stack(TEXT("Item_Berry"), 1) },
        { Stack(TEXT("Item_WarmBroth"), 2) }, 5.0f, TEXT("Tech_Thermal"), TEXT("Station_Campfire")));

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_Fertilizer"), TEXT("Dawnfield Fertilizer"),
        { Stack(TEXT("Item_Dawnbloom"), 2), Stack(TEXT("Item_Fiber"), 1), Stack(TEXT("Item_RawMeat"), 1) },
        { Stack(TEXT("Item_Fertilizer"), 3) }, 4.0f, TEXT("Tech_Agriculture"), TEXT("Station_Campfire")));

    // --- Dungeon reward (CODE_DEFAULT wave 8, Batch 6): the warden's Ancient
    //     Core sinks into the strongest weapon in the vertical slice. ---

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_AncientResonator"), TEXT("Ancient Resonator"),
        { Stack(TEXT("Item_AncientCore"), 1), Stack(TEXT("Item_CrystalShard"), 2), Stack(TEXT("Item_Resonator"), 1) },
        { Stack(TEXT("Item_AncientResonator"), 1) }, 10.0f, TEXT("Tech_AncientResonance"), TEXT("Station_Workbench")));

    // --- Advanced technology recipes (final production run — PHASE 12): every
    //     recipe gates behind Tech_AdvancedEnergy, which previously unlocked
    //     NOTHING — the tech tree now pays out across four new systems
    //     (scanner, laser, armor progression, robotics). ---

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_EnergyCell"), TEXT("Pulse Cells"),
        { Stack(TEXT("Item_VoltCore"), 2), Stack(TEXT("Item_CrystalShard"), 1) },
        { Stack(TEXT("Item_EnergyCell"), 4) }, 4.0f, TEXT("Tech_AdvancedEnergy"), TEXT("Station_Workbench")));

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_FieldScanner"), TEXT("Field Scanner"),
        { Stack(TEXT("Item_CrystalShard"), 2), Stack(TEXT("Item_VoltCore"), 1), Stack(TEXT("Item_Fiber"), 2) },
        { Stack(TEXT("Item_FieldScanner"), 1) }, 6.0f, TEXT("Tech_AdvancedEnergy"), TEXT("Station_Workbench")));

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_ResonanceHelm"), TEXT("Resonance Helm"),
        { Stack(TEXT("Item_CrystalShard"), 3), Stack(TEXT("Item_EmberAsh"), 2), Stack(TEXT("Item_Fiber"), 2) },
        { Stack(TEXT("Item_ResonanceHelm"), 1) }, 7.0f, TEXT("Tech_AdvancedEnergy"), TEXT("Station_Workbench")));

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_DawnstriderExosuit"), TEXT("Dawnstrider Exosuit"),
        { Stack(TEXT("Item_VoltCore"), 4), Stack(TEXT("Item_CrystalShard"), 3), Stack(TEXT("Item_EmberAsh"), 4) },
        { Stack(TEXT("Item_DawnstriderExosuit"), 1) }, 12.0f, TEXT("Tech_AdvancedEnergy"), TEXT("Station_Workbench")));

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_PulseLance"), TEXT("Pulse Lance"),
        { Stack(TEXT("Item_VoltCore"), 3), Stack(TEXT("Item_CrystalShard"), 3), Stack(TEXT("Item_DawnShard"), 3) },
        { Stack(TEXT("Item_PulseLance"), 1) }, 10.0f, TEXT("Tech_AdvancedEnergy"), TEXT("Station_Workbench")));

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_UtilityDrone"), TEXT("Utility Drone"),
        { Stack(TEXT("Item_VoltCore"), 4), Stack(TEXT("Item_CrystalShard"), 2), Stack(TEXT("Item_DawnShard"), 3) },
        { Stack(TEXT("Item_UtilityDrone"), 1) }, 12.0f, TEXT("Tech_AdvancedEnergy"), TEXT("Station_Workbench")));

    Registry->RegisterRecipe(MakeRecipe(Outer, TEXT("Recipe_UtilityRobot"), TEXT("Utility Robot"),
        { Stack(TEXT("Item_VoltCore"), 6), Stack(TEXT("Item_CrystalShard"), 4), Stack(TEXT("Item_DawnShard"), 5) },
        { Stack(TEXT("Item_UtilityRobot"), 1) }, 15.0f, TEXT("Tech_AdvancedEnergy"), TEXT("Station_Workbench")));
}

// ---------------------------------------------------------------------------
// Echo species (vertical slice roster — directive §44: 3-5 Echo)
// ---------------------------------------------------------------------------
void UAstrawildContentLibrary::BuildEchoes(UAstrawildItemRegistrySubsystem* Registry)
{
    UObject* Outer = Registry;
    const TArray<FName> BerryFood = { TEXT("Item_Berry") };

    // The first companion: docile, curious, light-element (directive §21 first Echo).
    UAstrawildEchoDefinition* Lumewisp = MakeEcho(Outer, TEXT("Echo_Lumewisp"), TEXT("Lumewisp"), EAstrawildElementType::Light,
        EAstrawildEchoRole::Support, 60.0f, 8.0f, 2.0f, 320.0f, EAstrawildPersonality::Curious,
        EAstrawildActivityPattern::Diurnal, BerryFood, 0.25f, EAstrawildElementType::Ash, false);
    Lumewisp->PreferredWeather = { EAstrawildWeatherState::Clear, EAstrawildWeatherState::Cloudy };
    Lumewisp->HabitatBiomeIds = { TEXT("Biome_DawnFields") };
    FAstrawildWorkAffinity LightWork;
    LightWork.WorkType = EAstrawildWorkType::Gathering;
    LightWork.Affinity = 1.2f;
    Lumewisp->WorkAffinities.Add(LightWork);
    Lumewisp->DefeatLoot.Add(Stack(TEXT("Item_Fiber"), 1));
    Registry->RegisterEcho(Lumewisp);

    // Sturdy stone companion: brave tank.
    UAstrawildEchoDefinition* Stonehide = MakeEcho(Outer, TEXT("Echo_Stonehide"), TEXT("Stonehide"), EAstrawildElementType::Ash,
        EAstrawildEchoRole::Combat, 140.0f, 16.0f, 8.0f, 260.0f, EAstrawildPersonality::Brave,
        EAstrawildActivityPattern::Diurnal, BerryFood, 0.55f, EAstrawildElementType::Light, false);
    FAstrawildWorkAffinity MiningWork;
    MiningWork.WorkType = EAstrawildWorkType::Mining;
    MiningWork.Affinity = 1.8f;
    Stonehide->WorkAffinities.Add(MiningWork);
    Stonehide->DefeatLoot.Add(Stack(TEXT("Item_Stone"), 2));
    Registry->RegisterEcho(Stonehide);

    // Nocturnal energy creature — the power synergy of directive §55.
    UAstrawildEchoDefinition* Voltling = MakeEcho(Outer, TEXT("Echo_Voltling"), TEXT("Voltling"), EAstrawildElementType::Pulse,
        EAstrawildEchoRole::Base, 55.0f, 10.0f, 3.0f, 380.0f, EAstrawildPersonality::Energetic,
        EAstrawildActivityPattern::Nocturnal, BerryFood, 0.45f, EAstrawildElementType::Frost, false);
    FAstrawildWorkAffinity PowerWork;
    PowerWork.WorkType = EAstrawildWorkType::PowerGeneration;
    PowerWork.Affinity = 2.0f;
    Voltling->WorkAffinities.Add(PowerWork);
    Registry->RegisterEcho(Voltling);

    // Dusk moth: shy support with research affinity.
    UAstrawildEchoDefinition* Duskmoth = MakeEcho(Outer, TEXT("Echo_Duskmoth"), TEXT("Duskmoth"), EAstrawildElementType::Flora,
        EAstrawildEchoRole::Support, 45.0f, 6.0f, 2.0f, 300.0f, EAstrawildPersonality::Timid,
        EAstrawildActivityPattern::Crepuscular, BerryFood, 0.35f, EAstrawildElementType::Frost, false);
    FAstrawildWorkAffinity ResearchWork;
    ResearchWork.WorkType = EAstrawildWorkType::ResearchAssist;
    ResearchWork.Affinity = 1.6f;
    Duskmoth->WorkAffinities.Add(ResearchWork);
    Registry->RegisterEcho(Duskmoth);

    // First hostile creature (directive §21): night stalker.
    UAstrawildEchoDefinition* Gloomfang = MakeEcho(Outer, TEXT("Echo_Gloomfang"), TEXT("Gloomfang"), EAstrawildElementType::Ash,
        EAstrawildEchoRole::Combat, 110.0f, 18.0f, 4.0f, 420.0f, EAstrawildPersonality::Aggressive,
        EAstrawildActivityPattern::Nocturnal, TArray<FName>(), 0.85f, EAstrawildElementType::Light, true);
    Gloomfang->DefeatLoot.Add(Stack(TEXT("Item_RawMeat"), 2));
    Gloomfang->DefeatLoot.Add(Stack(TEXT("Item_CrystalShard"), 1));
    Registry->RegisterEcho(Gloomfang);

    // --- Content expansion (CODE_DEFAULT wave 2) ---

    // Herding flora companion: the husbandry anchor species (directive §7 herds).
    UAstrawildEchoDefinition* Sprigling = MakeEcho(Outer, TEXT("Echo_Sprigling"), TEXT("Sprigling"), EAstrawildElementType::Flora,
        EAstrawildEchoRole::Support, 50.0f, 5.0f, 3.0f, 290.0f, EAstrawildPersonality::Social,
        EAstrawildActivityPattern::Diurnal, BerryFood, 0.30f, EAstrawildElementType::Frost, false);
    Sprigling->PreferredWeather = { EAstrawildWeatherState::Clear, EAstrawildWeatherState::Rain };
    Sprigling->HabitatBiomeIds = { TEXT("Biome_DawnFields") };
    FAstrawildWorkAffinity FarmWork;
    FarmWork.WorkType = EAstrawildWorkType::Farming;
    FarmWork.Affinity = 1.7f;
    Sprigling->WorkAffinities.Add(FarmWork);
    Sprigling->DefeatLoot.Add(Stack(TEXT("Item_Dawnbloom"), 2));
    Sprigling->DefeatLoot.Add(Stack(TEXT("Item_Fiber"), 1));
    Registry->RegisterEcho(Sprigling);

    // Ember predator: crepuscular stalker of the meadow edges (directive §7 food chain).
    UAstrawildEchoDefinition* Emberfang = MakeEcho(Outer, TEXT("Echo_Emberfang"), TEXT("Emberfang"), EAstrawildElementType::Ember,
        EAstrawildEchoRole::Combat, 130.0f, 20.0f, 5.0f, 400.0f, EAstrawildPersonality::Aggressive,
        EAstrawildActivityPattern::Crepuscular, TArray<FName>(), 0.90f, EAstrawildElementType::Frost, true);
    Emberfang->DefeatLoot.Add(Stack(TEXT("Item_RawMeat"), 2));
    Emberfang->DefeatLoot.Add(Stack(TEXT("Item_EmberAsh"), 2));
    Registry->RegisterEcho(Emberfang);

    // --- Ecosystem expansion (CODE_DEFAULT wave 7, Batch 5): completes the
    //     element coverage — every element now has at least one species, and
    //     the hostile roster spans Ash/Ember/Frost/Pulse. ---

    // Frost predator: the night's cold answer to the Emberfang rivalry. Weak to
    // Ember (fire melts frost) while Emberfang is weak to Frost — a true rivalry pair.
    UAstrawildEchoDefinition* Rimefang = MakeEcho(Outer, TEXT("Echo_Rimefang"), TEXT("Rimefang"), EAstrawildElementType::Frost,
        EAstrawildEchoRole::Combat, 120.0f, 17.0f, 6.0f, 380.0f, EAstrawildPersonality::Aggressive,
        EAstrawildActivityPattern::Nocturnal, TArray<FName>(), 0.88f, EAstrawildElementType::Ember, true);
    Rimefang->PreferredWeather = { EAstrawildWeatherState::Rain, EAstrawildWeatherState::Storm };
    Rimefang->DefeatLoot.Add(Stack(TEXT("Item_RawMeat"), 2));
    Rimefang->DefeatLoot.Add(Stack(TEXT("Item_Frostbloom"), 2));
    Registry->RegisterEcho(Rimefang);

    // Pulse predator: glass-cannon stalker — highest ATK (22) and speed (440) in
    // the roster, paper-thin defense. Weak to Ash (stone grounds the arc).
    UAstrawildEchoDefinition* Voltmaw = MakeEcho(Outer, TEXT("Echo_Voltmaw"), TEXT("Voltmaw"), EAstrawildElementType::Pulse,
        EAstrawildEchoRole::Combat, 95.0f, 22.0f, 3.0f, 440.0f, EAstrawildPersonality::Aggressive,
        EAstrawildActivityPattern::Crepuscular, TArray<FName>(), 0.92f, EAstrawildElementType::Ash, true);
    Voltmaw->DefeatLoot.Add(Stack(TEXT("Item_CrystalShard"), 1));
    Voltmaw->DefeatLoot.Add(Stack(TEXT("Item_VoltCore"), 1));
    Registry->RegisterEcho(Voltmaw);

    // Ancient-rare companion: the crown jewel of the dawn fields — one spawns per
    // world, hardest capture in the roster (0.95), research affinity par excellence.
    UAstrawildEchoDefinition* Auroraling = MakeEcho(Outer, TEXT("Echo_Auroraling"), TEXT("Auroraling"), EAstrawildElementType::Light,
        EAstrawildEchoRole::Support, 90.0f, 12.0f, 6.0f, 350.0f, EAstrawildPersonality::Curious,
        EAstrawildActivityPattern::Diurnal, BerryFood, 0.95f, EAstrawildElementType::Ash, false);
    Auroraling->PreferredWeather = { EAstrawildWeatherState::Clear };
    Auroraling->HabitatBiomeIds = { TEXT("Biome_DawnFields") };
    FAstrawildWorkAffinity AuroraResearch;
    AuroraResearch.WorkType = EAstrawildWorkType::ResearchAssist;
    AuroraResearch.Affinity = 2.2f;
    Auroraling->WorkAffinities.Add(AuroraResearch);
    Auroraling->DefeatLoot.Add(Stack(TEXT("Item_DawnShard"), 2));
    Auroraling->DefeatLoot.Add(Stack(TEXT("Item_Dawnbloom"), 1));
    Registry->RegisterEcho(Auroraling);
}

// ---------------------------------------------------------------------------
// Buildings
// ---------------------------------------------------------------------------
void UAstrawildContentLibrary::BuildBuildings(UAstrawildItemRegistrySubsystem* Registry)
{
    UObject* Outer = Registry;

    Registry->RegisterBuilding(MakeBuilding(Outer, TEXT("Building_Foundation"), TEXT("Foundation"), EAstrawildBuildingCategory::Foundation,
        TEXT("Item_Wood"), 4, NAME_None, 800.0f, EAstrawildPowerRole::Consumer, 0.0f, 0.0f, 0.0f, EAstrawildWorkType::None));

    Registry->RegisterBuilding(MakeBuilding(Outer, TEXT("Building_Wall"), TEXT("Wall"), EAstrawildBuildingCategory::Wall,
        TEXT("Item_Wood"), 2, NAME_None, 500.0f, EAstrawildPowerRole::Consumer, 0.0f, 0.0f, 0.0f, EAstrawildWorkType::None));

    Registry->RegisterBuilding(MakeBuilding(Outer, TEXT("Building_Workbench"), TEXT("Workbench"), EAstrawildBuildingCategory::Workstation,
        TEXT("Item_Wood"), 8, NAME_None, 400.0f, EAstrawildPowerRole::Consumer, 0.0f, 0.0f, 0.0f, EAstrawildWorkType::Crafting));

    Registry->RegisterBuilding(MakeBuilding(Outer, TEXT("Building_Campfire"), TEXT("Campfire"), EAstrawildBuildingCategory::Workstation,
        TEXT("Item_Wood"), 5, NAME_None, 300.0f, EAstrawildPowerRole::Consumer, 0.0f, 0.0f, 0.0f, EAstrawildWorkType::Cooking));

    Registry->RegisterBuilding(MakeBuilding(Outer, TEXT("Building_Generator"), TEXT("Echo Dynamo"), EAstrawildBuildingCategory::Power,
        TEXT("Item_Stone"), 10, TEXT("Tech_Electrical"), 600.0f, EAstrawildPowerRole::Generator, 8.0f, 0.0f, 0.0f, EAstrawildWorkType::PowerGeneration));

    Registry->RegisterBuilding(MakeBuilding(Outer, TEXT("Building_Battery"), TEXT("Charge Cell"), EAstrawildBuildingCategory::Power,
        TEXT("Item_Stone"), 8, TEXT("Tech_Electrical"), 400.0f, EAstrawildPowerRole::Battery, 0.0f, 0.0f, 600.0f, EAstrawildWorkType::None));

    Registry->RegisterBuilding(MakeBuilding(Outer, TEXT("Building_LampPost"), TEXT("Dawn Lamp"), EAstrawildBuildingCategory::Decoration,
        TEXT("Item_Wood"), 3, TEXT("Tech_Electrical"), 200.0f, EAstrawildPowerRole::Consumer, 0.0f, 2.0f, 0.0f, EAstrawildWorkType::None));

    Registry->RegisterBuilding(MakeBuilding(Outer, TEXT("Building_FarmPlot"), TEXT("Farm Plot"), EAstrawildBuildingCategory::Farm,
        TEXT("Item_Wood"), 6, NAME_None, 250.0f, EAstrawildPowerRole::Consumer, 0.0f, 0.0f, 0.0f, EAstrawildWorkType::Farming));

    Registry->RegisterBuilding(MakeBuilding(Outer, TEXT("Building_ResearchDesk"), TEXT("Research Desk"), EAstrawildBuildingCategory::Research,
        TEXT("Item_Wood"), 6, NAME_None, 350.0f, EAstrawildPowerRole::Consumer, 0.0f, 1.0f, 0.0f, EAstrawildWorkType::ResearchAssist));

    // --- Content expansion (CODE_DEFAULT wave 2) ---

    Registry->RegisterBuilding(MakeBuilding(Outer, TEXT("Building_FeedTrough"), TEXT("Echo Feed Trough"), EAstrawildBuildingCategory::Farm,
        TEXT("Item_Wood"), 5, TEXT("Tech_Husbandry"), 260.0f, EAstrawildPowerRole::Consumer, 0.0f, 0.0f, 0.0f, EAstrawildWorkType::Farming));

    // --- Ecosystem expansion (CODE_DEFAULT wave 7, Batch 5): the three new tech
    //     nodes each unlock a building that anchors its era. ---

    Registry->RegisterBuilding(MakeBuilding(Outer, TEXT("Building_Sawmill"), TEXT("Dawnwood Sawmill"), EAstrawildBuildingCategory::Workstation,
        TEXT("Item_Wood"), 10, TEXT("Tech_Mechanics"), 500.0f, EAstrawildPowerRole::Consumer, 2.0f, 0.0f, 0.0f, EAstrawildWorkType::Crafting));

    Registry->RegisterBuilding(MakeBuilding(Outer, TEXT("Building_Heater"), TEXT("Hearth Coil"), EAstrawildBuildingCategory::Workstation,
        TEXT("Item_Stone"), 6, TEXT("Tech_Thermal"), 300.0f, EAstrawildPowerRole::Consumer, 3.0f, 0.0f, 0.0f, EAstrawildWorkType::None));

    Registry->RegisterBuilding(MakeBuilding(Outer, TEXT("Building_Composter"), TEXT("Dawn Composter"), EAstrawildBuildingCategory::Farm,
        TEXT("Item_Wood"), 4, TEXT("Tech_Agriculture"), 220.0f, EAstrawildPowerRole::Consumer, 0.0f, 0.0f, 0.0f, EAstrawildWorkType::Farming));
}

// ---------------------------------------------------------------------------
// Technology tree (directive §19)
// ---------------------------------------------------------------------------
void UAstrawildContentLibrary::BuildTechnologies(UAstrawildItemRegistrySubsystem* Registry)
{
    UObject* Outer = Registry;

    Registry->RegisterTechnology(MakeTech(Outer, TEXT("Tech_BasicCrafting"), TEXT("Basic Crafting"), EAstrawildTechEra::Primitive, 0,
        {}, {}, {}));

    Registry->RegisterTechnology(MakeTech(Outer, TEXT("Tech_Cooking"), TEXT("Cooking"), EAstrawildTechEra::Primitive, 5,
        { TEXT("Tech_BasicCrafting") }, { TEXT("Recipe_CookedMeat") }, {}));

    Registry->RegisterTechnology(MakeTech(Outer, TEXT("Tech_Electrical"), TEXT("Electrical Foundations"), EAstrawildTechEra::Electrical, 15,
        { TEXT("Tech_BasicCrafting") }, {}, { TEXT("Building_Generator"), TEXT("Building_Battery"), TEXT("Building_LampPost") }));

    Registry->RegisterTechnology(MakeTech(Outer, TEXT("Tech_AdvancedEnergy"), TEXT("Advanced Energy"), EAstrawildTechEra::AdvancedEnergy, 30,
        { TEXT("Tech_Electrical") }, { TEXT("Recipe_EnergyCell"), TEXT("Recipe_FieldScanner"), TEXT("Recipe_ResonanceHelm"),
        TEXT("Recipe_DawnstriderExosuit"), TEXT("Recipe_PulseLance"), TEXT("Recipe_UtilityDrone"), TEXT("Recipe_UtilityRobot") }, {}));

    // --- Content expansion (CODE_DEFAULT wave 2): the husbandry branch. ---

    Registry->RegisterTechnology(MakeTech(Outer, TEXT("Tech_Husbandry"), TEXT("Echo Husbandry"), EAstrawildTechEra::Primitive, 10,
        { TEXT("Tech_Cooking") }, { TEXT("Recipe_FeedMix"), TEXT("Recipe_HerbalSalve") }, { TEXT("Building_FeedTrough") }));

    // --- Content expansion (CODE_DEFAULT wave 3): the armory branch. ---

    Registry->RegisterTechnology(MakeTech(Outer, TEXT("Tech_Armory"), TEXT("Armory"), EAstrawildTechEra::Primitive, 8,
        { TEXT("Tech_BasicCrafting") }, { TEXT("Recipe_StonehideShield"), TEXT("Recipe_CrystalBlade"),
        TEXT("Recipe_FiberWeaveVest"), TEXT("Recipe_EmberhideJacket"), TEXT("Recipe_CrystalplateCuirass") }, {}));

    // --- Ecosystem expansion (CODE_DEFAULT wave 7, Batch 5): the tree grows
    //     from 6 to 9 nodes and now uses every era enum except Ancient (reserved
    //     for the dungeon/boss milestone). Voltmaw loot (Volt Core) sinks into
    //     Mechanics; Rimefang loot (Frostbloom) sinks into Thermal. ---

    Registry->RegisterTechnology(MakeTech(Outer, TEXT("Tech_Mechanics"), TEXT("Mechanical Workshop"), EAstrawildTechEra::Mechanical, 12,
        { TEXT("Tech_BasicCrafting") }, { TEXT("Recipe_PlankBatch"), TEXT("Recipe_ResonatorBatch") }, { TEXT("Building_Sawmill") }));

    Registry->RegisterTechnology(MakeTech(Outer, TEXT("Tech_Thermal"), TEXT("Thermal Engineering"), EAstrawildTechEra::Electrical, 18,
        { TEXT("Tech_Electrical") }, { TEXT("Recipe_WarmBroth") }, { TEXT("Building_Heater") }));

    Registry->RegisterTechnology(MakeTech(Outer, TEXT("Tech_Agriculture"), TEXT("Agriculture"), EAstrawildTechEra::Eco, 20,
        { TEXT("Tech_Husbandry") }, { TEXT("Recipe_Fertilizer") }, { TEXT("Building_Composter") }));

    // --- Dungeon reward (CODE_DEFAULT wave 8, Batch 6): the Ancient era opens
    //     ONLY through the Hollow Underlight (roadmap V3 §21 unique technology
    //     reward) — the generator force-unlocks this node on completion. The
    //     era enum is now fully used. ---

    Registry->RegisterTechnology(MakeTech(Outer, TEXT("Tech_AncientResonance"), TEXT("Ancient Resonance"), EAstrawildTechEra::Ancient, 25,
        { TEXT("Tech_AdvancedEnergy") }, { TEXT("Recipe_AncientResonator") }, {}));
}

// ---------------------------------------------------------------------------
// First quest chain (directive §21/§12)
// ---------------------------------------------------------------------------
void UAstrawildContentLibrary::BuildQuests(UAstrawildItemRegistrySubsystem* Registry)
{
    UObject* Outer = Registry;

    UAstrawildQuestDefinition* Quest1 = NewObject<UAstrawildQuestDefinition>(Outer);
    Quest1->QuestId = TEXT("Quest_FirstLight");
    Quest1->Title = FText::FromString(TEXT("First Light"));
    Quest1->Summary = FText::FromString(TEXT("Gather materials from Dawn Fields to prepare for the journey ahead."));
    FAstrawildQuestObjective ObjWood;
    ObjWood.Type = EAstrawildQuestObjectiveType::CollectItem;
    ObjWood.TargetId = TEXT("Item_Wood");
    ObjWood.RequiredCount = 10;
    ObjWood.ObjectiveText = FText::FromString(TEXT("Collect 10 Dawnwood"));
    Quest1->Objectives.Add(ObjWood);
    FAstrawildQuestObjective ObjStone;
    ObjStone.Type = EAstrawildQuestObjectiveType::CollectItem;
    ObjStone.TargetId = TEXT("Item_Stone");
    ObjStone.RequiredCount = 5;
    ObjStone.ObjectiveText = FText::FromString(TEXT("Collect 5 Fieldstone"));
    Quest1->Objectives.Add(ObjStone);
    Quest1->RewardItems.Add(Stack(TEXT("Item_Resonator"), 2));
    Quest1->RewardResearchPoints = 5;
    Quest1->NextQuestId = TEXT("Quest_FirstEcho");
    Registry->RegisterQuest(Quest1);

    UAstrawildQuestDefinition* Quest2 = NewObject<UAstrawildQuestDefinition>(Outer);
    Quest2->QuestId = TEXT("Quest_FirstEcho");
    Quest2->Title = FText::FromString(TEXT("A Friend in the Fields"));
    Quest2->Summary = FText::FromString(TEXT("Observe a wild Lumewisp, gain its trust, and welcome your first Echo."));
    FAstrawildQuestObjective ObjObserve;
    ObjObserve.Type = EAstrawildQuestObjectiveType::ObserveEcho;
    ObjObserve.TargetId = TEXT("Echo_Lumewisp");
    ObjObserve.RequiredCount = 1;
    ObjObserve.ObjectiveText = FText::FromString(TEXT("Observe a Lumewisp"));
    Quest2->Objectives.Add(ObjObserve);
    FAstrawildQuestObjective ObjCapture;
    ObjCapture.Type = EAstrawildQuestObjectiveType::CaptureEcho;
    ObjCapture.TargetId = TEXT("Echo_Lumewisp");
    ObjCapture.RequiredCount = 1;
    ObjCapture.ObjectiveText = FText::FromString(TEXT("Capture a Lumewisp"));
    Quest2->Objectives.Add(ObjCapture);
    Quest2->RewardItems.Add(Stack(TEXT("Item_Berry"), 10));
    Quest2->RewardResearchPoints = 10;
    Quest2->NextQuestId = TEXT("Quest_Homeground");
    Registry->RegisterQuest(Quest2);

    UAstrawildQuestDefinition* Quest3 = NewObject<UAstrawildQuestDefinition>(Outer);
    Quest3->QuestId = TEXT("Quest_Homeground");
    Quest3->Title = FText::FromString(TEXT("Homeground"));
    Quest3->Summary = FText::FromString(TEXT("Raise the first foundations of your camp."));
    FAstrawildQuestObjective ObjFoundation;
    ObjFoundation.Type = EAstrawildQuestObjectiveType::PlaceBuilding;
    ObjFoundation.TargetId = TEXT("Building_Foundation");
    ObjFoundation.RequiredCount = 1;
    ObjFoundation.ObjectiveText = FText::FromString(TEXT("Place a Foundation"));
    Quest3->Objectives.Add(ObjFoundation);
    FAstrawildQuestObjective ObjWorkbench;
    ObjWorkbench.Type = EAstrawildQuestObjectiveType::PlaceBuilding;
    ObjWorkbench.TargetId = TEXT("Building_Workbench");
    ObjWorkbench.RequiredCount = 1;
    ObjWorkbench.ObjectiveText = FText::FromString(TEXT("Place a Workbench"));
    Quest3->Objectives.Add(ObjWorkbench);
    Quest3->RewardResearchPoints = 10;
    Quest3->NextQuestId = TEXT("Quest_Spark");
    Registry->RegisterQuest(Quest3);

    UAstrawildQuestDefinition* Quest4 = NewObject<UAstrawildQuestDefinition>(Outer);
    Quest4->QuestId = TEXT("Quest_Spark");
    Quest4->Title = FText::FromString(TEXT("The Spark"));
    Quest4->Summary = FText::FromString(TEXT("Research Electrical Foundations and bring light to your camp."));
    FAstrawildQuestObjective ObjTech;
    ObjTech.Type = EAstrawildQuestObjectiveType::UnlockTechnology;
    ObjTech.TargetId = TEXT("Tech_Electrical");
    ObjTech.RequiredCount = 1;
    ObjTech.ObjectiveText = FText::FromString(TEXT("Unlock Electrical Foundations"));
    Quest4->Objectives.Add(ObjTech);
    FAstrawildQuestObjective ObjGenerator;
    ObjGenerator.Type = EAstrawildQuestObjectiveType::PlaceBuilding;
    ObjGenerator.TargetId = TEXT("Building_Generator");
    ObjGenerator.RequiredCount = 1;
    ObjGenerator.ObjectiveText = FText::FromString(TEXT("Build an Echo Dynamo"));
    Quest4->Objectives.Add(ObjGenerator);
    Quest4->RewardResearchPoints = 15;
    Quest4->NextQuestId = TEXT("Quest_DawnGuard");
    Registry->RegisterQuest(Quest4);

    UAstrawildQuestDefinition* Quest5 = NewObject<UAstrawildQuestDefinition>(Outer);
    Quest5->QuestId = TEXT("Quest_DawnGuard");
    Quest5->Title = FText::FromString(TEXT("Dawn Guard"));
    Quest5->Summary = FText::FromString(TEXT("Gloomfangs stalk the fields at night. Protect the dawn."));
    FAstrawildQuestObjective ObjDefeat;
    ObjDefeat.Type = EAstrawildQuestObjectiveType::DefeatCreature;
    ObjDefeat.TargetId = TEXT("Echo_Gloomfang");
    ObjDefeat.RequiredCount = 3;
    ObjDefeat.ObjectiveText = FText::FromString(TEXT("Defeat 3 Gloomfangs"));
    Quest5->Objectives.Add(ObjDefeat);
    Quest5->RewardItems.Add(Stack(TEXT("Item_AncientCore"), 1));
    Quest5->RewardItems.Add(Stack(TEXT("Item_DawnShard"), 5)); // Batch 4 — M-11: vendor currency.
    Quest5->RewardResearchPoints = 20;
    Quest5->NextQuestId = TEXT("Quest_ShepherdsDawn");
    Registry->RegisterQuest(Quest5);

    // --- Content expansion (CODE_DEFAULT wave 2): husbandry chain finale. ---

    UAstrawildQuestDefinition* Quest6 = NewObject<UAstrawildQuestDefinition>(Outer);
    Quest6->QuestId = TEXT("Quest_ShepherdsDawn");
    Quest6->Title = FText::FromString(TEXT("Shepherd's Dawn"));
    Quest6->Summary = FText::FromString(TEXT("Sprigling herds graze the meadows. Learn the ways of Echo husbandry."));
    FAstrawildQuestObjective ObjHusbandry;
    ObjHusbandry.Type = EAstrawildQuestObjectiveType::UnlockTechnology;
    ObjHusbandry.TargetId = TEXT("Tech_Husbandry");
    ObjHusbandry.RequiredCount = 1;
    ObjHusbandry.ObjectiveText = FText::FromString(TEXT("Unlock Echo Husbandry"));
    Quest6->Objectives.Add(ObjHusbandry);
    FAstrawildQuestObjective ObjCaptureSprigling;
    ObjCaptureSprigling.Type = EAstrawildQuestObjectiveType::CaptureEcho;
    ObjCaptureSprigling.TargetId = TEXT("Echo_Sprigling");
    ObjCaptureSprigling.RequiredCount = 1;
    ObjCaptureSprigling.ObjectiveText = FText::FromString(TEXT("Capture a Sprigling"));
    Quest6->Objectives.Add(ObjCaptureSprigling);
    FAstrawildQuestObjective ObjTrough;
    ObjTrough.Type = EAstrawildQuestObjectiveType::PlaceBuilding;
    ObjTrough.TargetId = TEXT("Building_FeedTrough");
    ObjTrough.RequiredCount = 1;
    ObjTrough.ObjectiveText = FText::FromString(TEXT("Place an Echo Feed Trough"));
    Quest6->Objectives.Add(ObjTrough);
    FAstrawildQuestObjective ObjFeed;
    ObjFeed.Type = EAstrawildQuestObjectiveType::CollectItem;
    ObjFeed.TargetId = TEXT("Item_FeedMix");
    ObjFeed.RequiredCount = 3;
    ObjFeed.ObjectiveText = FText::FromString(TEXT("Craft 3 Echo Feed Mix"));
    Quest6->Objectives.Add(ObjFeed);
    Quest6->RewardItems.Add(Stack(TEXT("Item_FeedMix"), 5));
    Quest6->RewardItems.Add(Stack(TEXT("Item_HerbalSalve"), 2));
    Quest6->RewardResearchPoints = 20;
    Quest6->NextQuestId = TEXT("Quest_HollowUnderlight"); // Batch 6: the chain now descends.
    Registry->RegisterQuest(Quest6);

    // --- Dungeon quest (CODE_DEFAULT wave 8, Batch 6 — directive §23/§25): the
    //     Hollow Underlight finale. ReachLocation finally has a publisher (the
    //     dungeon portals); DefeatCreature targets the warden's distinct event id
    //     so wild Gloomfang kills don't complete the objective. ---

    UAstrawildQuestDefinition* Quest7 = NewObject<UAstrawildQuestDefinition>(Outer);
    Quest7->QuestId = TEXT("Quest_HollowUnderlight");
    Quest7->Title = FText::FromString(TEXT("The Hollow Underlight"));
    Quest7->Summary = FText::FromString(TEXT("A sealed resonance gate hums beyond the eastern wilds. Whatever wards it has been waiting since the First Dawn."));
    FAstrawildQuestObjective ObjEnter;
    ObjEnter.Type = EAstrawildQuestObjectiveType::ReachLocation;
    ObjEnter.TargetId = TEXT("Location_HollowUnderlight");
    ObjEnter.RequiredCount = 1;
    ObjEnter.ObjectiveText = FText::FromString(TEXT("Enter the Hollow Underlight"));
    Quest7->Objectives.Add(ObjEnter);
    FAstrawildQuestObjective ObjWarden;
    ObjWarden.Type = EAstrawildQuestObjectiveType::DefeatCreature;
    ObjWarden.TargetId = TEXT("Creature_UnderlightWarden");
    ObjWarden.RequiredCount = 1;
    ObjWarden.ObjectiveText = FText::FromString(TEXT("Defeat the Underlight Warden"));
    Quest7->Objectives.Add(ObjWarden);
    Quest7->RewardItems.Add(Stack(TEXT("Item_HerbalSalve"), 2));
    Quest7->RewardItems.Add(Stack(TEXT("Item_DawnShard"), 5));
    Quest7->RewardResearchPoints = 15;
    Quest7->NextQuestId = TEXT("Quest_ValeBeyond");
    Registry->RegisterQuest(Quest7);

    // --- Post-dungeon epilogue (final production run — PHASE 15): exercises the
    //     two previously-dead objective types — VisitZone (Event.ZoneEntered) and
    //     SurviveTime (per-second accrual) — plus the laser weapon payoff. ---

    UAstrawildQuestDefinition* Quest8 = NewObject<UAstrawildQuestDefinition>(Outer);
    Quest8->QuestId = TEXT("Quest_ValeBeyond");
    Quest8->Title = FText::FromString(TEXT("The Vale Beyond"));
    Quest8->Summary = FText::FromString(TEXT("The warden's fall resonates across the Vale. Prove the resonance technology in Ember Ridge's heat — and live there for three minutes."));
    FAstrawildQuestObjective ObjEmber;
    ObjEmber.Type = EAstrawildQuestObjectiveType::VisitZone;
    ObjEmber.TargetId = TEXT("Zone_EmberRidge");
    ObjEmber.RequiredCount = 1;
    ObjEmber.ObjectiveText = FText::FromString(TEXT("Travel to Ember Ridge"));
    Quest8->Objectives.Add(ObjEmber);
    FAstrawildQuestObjective ObjSurvive;
    ObjSurvive.Type = EAstrawildQuestObjectiveType::SurviveTime;
    ObjSurvive.TargetId = NAME_None;
    ObjSurvive.RequiredCount = 180; // Seconds.
    ObjSurvive.ObjectiveText = FText::FromString(TEXT("Survive for 3 minutes"));
    Quest8->Objectives.Add(ObjSurvive);
    FAstrawildQuestObjective ObjLance;
    ObjLance.Type = EAstrawildQuestObjectiveType::CraftRecipe;
    ObjLance.TargetId = TEXT("Recipe_PulseLance");
    ObjLance.RequiredCount = 1;
    ObjLance.ObjectiveText = FText::FromString(TEXT("Craft the Pulse Lance"));
    Quest8->Objectives.Add(ObjLance);
    Quest8->RewardItems.Add(Stack(TEXT("Item_EnergyCell"), 8));
    Quest8->RewardItems.Add(Stack(TEXT("Item_DawnShard"), 8));
    Quest8->RewardResearchPoints = 20;
    Quest8->NextQuestId = NAME_None;
    Registry->RegisterQuest(Quest8);
}

// ---------------------------------------------------------------------------
// Loot tables (CODE_DEFAULT wave 3 — directive §6 loot)
// ---------------------------------------------------------------------------
void UAstrawildContentLibrary::BuildLootTables(UAstrawildItemRegistrySubsystem* Registry)
{
    UObject* Outer = Registry;

    UAstrawildLootTableDefinition* DungeonBoss = NewObject<UAstrawildLootTableDefinition>(Outer);
    DungeonBoss->LootTableId = TEXT("Loot_DungeonBoss");
    DungeonBoss->GuaranteedDrops = { Stack(TEXT("Item_AncientCore"), 1), Stack(TEXT("Item_CrystalShard"), 2), Stack(TEXT("Item_EmberAsh"), 2), Stack(TEXT("Item_DawnShard"), 3) }; // Batch 4 — M-11: +Dawn Shards.
    DungeonBoss->BonusRollChance = 0.75f;
    Registry->RegisterLootTable(DungeonBoss);

    // Batch 4 — M-11: Trader Tam's wares — the GuaranteedDrops list doubles as
    // the shop stock list (prices live on each item's VendorPrice in Dawn Shards).
    UAstrawildLootTableDefinition* VendorStarter = NewObject<UAstrawildLootTableDefinition>(Outer);
    VendorStarter->LootTableId = TEXT("Loot_VendorStarter");
    VendorStarter->GuaranteedDrops = { Stack(TEXT("Item_Berry"), 3), Stack(TEXT("Item_WaterFlask"), 1), Stack(TEXT("Item_Bandage"), 2), Stack(TEXT("Item_HerbalSalve"), 1), Stack(TEXT("Item_Resonator"), 1) };
    VendorStarter->BonusRollChance = 0.0f;
    Registry->RegisterLootTable(VendorStarter);
}

// ---------------------------------------------------------------------------
// NPCs (CODE_DEFAULT wave 3 — directive §26 quest hooks + vendor)
// ---------------------------------------------------------------------------
void UAstrawildContentLibrary::BuildNPCs(UAstrawildItemRegistrySubsystem* Registry)
{
    UObject* Outer = Registry;

    UAstrawildNPCDefinition* WardenMaren = NewObject<UAstrawildNPCDefinition>(Outer);
    WardenMaren->NpcId = TEXT("NPC_WardenMaren");
    WardenMaren->DisplayName = FText::FromString(TEXT("Warden Maren"));
    WardenMaren->OfferedQuestId = TEXT("Quest_FirstLight");
    Registry->RegisterNPC(WardenMaren);

    UAstrawildNPCDefinition* VendorTam = NewObject<UAstrawildNPCDefinition>(Outer);
    VendorTam->NpcId = TEXT("NPC_VendorTam");
    VendorTam->DisplayName = FText::FromString(TEXT("Trader Tam"));
    VendorTam->ShopLootTableId = TEXT("Loot_VendorStarter");
    VendorTam->CurrencyItemId = TEXT("Item_DawnShard"); // Batch 4 — M-11: live shop.
    Registry->RegisterNPC(VendorTam);
}

void UAstrawildContentLibrary::BuildDefaults(UAstrawildItemRegistrySubsystem* Registry)
{
    if (!Registry)
    {
        return;
    }

    BuildItems(Registry);
    BuildRecipes(Registry);
    BuildEchoes(Registry);
    BuildBuildings(Registry);
    BuildTechnologies(Registry);
    BuildQuests(Registry);
    BuildLootTables(Registry);
    BuildNPCs(Registry);

    UE_LOG(LogAstrawildEconomy, Log, TEXT("Content library defaults registered: 35 items, 26 recipes, 10 Echo species, 13 buildings, 10 technologies, 8 quests, 2 loot tables, 2 NPCs."));
}
