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
    Registry->RegisterItem(WaterFlask);

    UAstrawildItemDefinition* Bandage = MakeItem(Outer, TEXT("Item_Bandage"), TEXT("Sunfiber Bandage"), EAstrawildItemCategory::Consumable, 0.2f, 30);
    Bandage->HealValue = 40.0f;
    Registry->RegisterItem(Bandage);

    Registry->RegisterItem(MakeItem(Outer, TEXT("Item_Resonator"), TEXT("Echo Resonator"), EAstrawildItemCategory::CreatureItem, 0.4f, 20));
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
    Registry->RegisterItem(HerbalSalve);

    // --- Equipment (CODE_DEFAULT wave 3): weapon + shield progression. ---

    UAstrawildItemDefinition* DawnwoodClub = MakeItem(Outer, TEXT("Item_DawnwoodClub"), TEXT("Dawnwood Club"), EAstrawildItemCategory::Equipment, 2.5f, 1);
    DawnwoodClub->AttackPower = 6.0f;
    Registry->RegisterItem(DawnwoodClub);

    UAstrawildItemDefinition* StonehideShield = MakeItem(Outer, TEXT("Item_StonehideShield"), TEXT("Stonehide Shield"), EAstrawildItemCategory::Equipment, 4.0f, 1);
    StonehideShield->BlockMitigation = 0.65f;
    Registry->RegisterItem(StonehideShield);

    UAstrawildItemDefinition* CrystalBlade = MakeItem(Outer, TEXT("Item_CrystalBlade"), TEXT("Dawn Crystal Blade"), EAstrawildItemCategory::Equipment, 3.0f, 1);
    CrystalBlade->AttackPower = 14.0f;
    Registry->RegisterItem(CrystalBlade);
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
        { TEXT("Tech_Electrical") }, {}, {}));

    // --- Content expansion (CODE_DEFAULT wave 2): the husbandry branch. ---

    Registry->RegisterTechnology(MakeTech(Outer, TEXT("Tech_Husbandry"), TEXT("Echo Husbandry"), EAstrawildTechEra::Primitive, 10,
        { TEXT("Tech_Cooking") }, { TEXT("Recipe_FeedMix"), TEXT("Recipe_HerbalSalve") }, { TEXT("Building_FeedTrough") }));

    // --- Content expansion (CODE_DEFAULT wave 3): the armory branch. ---

    Registry->RegisterTechnology(MakeTech(Outer, TEXT("Tech_Armory"), TEXT("Armory"), EAstrawildTechEra::Primitive, 8,
        { TEXT("Tech_BasicCrafting") }, { TEXT("Recipe_StonehideShield"), TEXT("Recipe_CrystalBlade") }, {}));
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
    Quest6->NextQuestId = NAME_None;
    Registry->RegisterQuest(Quest6);
}

// ---------------------------------------------------------------------------
// Loot tables (CODE_DEFAULT wave 3 — directive §6 loot)
// ---------------------------------------------------------------------------
void UAstrawildContentLibrary::BuildLootTables(UAstrawildItemRegistrySubsystem* Registry)
{
    UObject* Outer = Registry;

    UAstrawildLootTableDefinition* DungeonBoss = NewObject<UAstrawildLootTableDefinition>(Outer);
    DungeonBoss->LootTableId = TEXT("Loot_DungeonBoss");
    DungeonBoss->GuaranteedDrops = { Stack(TEXT("Item_AncientCore"), 1), Stack(TEXT("Item_CrystalShard"), 2), Stack(TEXT("Item_EmberAsh"), 2) };
    DungeonBoss->BonusRollChance = 0.75f;
    Registry->RegisterLootTable(DungeonBoss);

    UAstrawildLootTableDefinition* VendorStarter = NewObject<UAstrawildLootTableDefinition>(Outer);
    VendorStarter->LootTableId = TEXT("Loot_VendorStarter");
    VendorStarter->GuaranteedDrops = { Stack(TEXT("Item_Berry"), 3), Stack(TEXT("Item_WaterFlask"), 1), Stack(TEXT("Item_Bandage"), 2) };
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

    UE_LOG(LogAstrawildEconomy, Log, TEXT("Content library defaults registered: 19 items, 10 recipes, 7 Echo species, 10 buildings, 6 technologies, 6 quests, 2 loot tables, 2 NPCs."));
}
