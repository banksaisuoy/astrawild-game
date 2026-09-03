#include "AstrawildProductionContent.h"

#include "AstrawildDataAssets.h"
#include "AstrawildItemRegistrySubsystem.h"
#include "AstrawildLog.h"
#include "AstrawildArtPack.h"

// ---------------------------------------------------------------------------
// Local helpers — terse builders mirroring the ContentLibrary house style.
// ---------------------------------------------------------------------------

namespace
{
    FAstrawildItemStack Stack(const FName Id, const int32 Qty)
    {
        FAstrawildItemStack S;
        S.ItemId = Id;
        S.Quantity = Qty;
        return S;
    }

    UAstrawildItemDefinition* MakeItem(UAstrawildItemRegistrySubsystem* Registry, const FName Id, const FString& Name,
        const EAstrawildItemCategory Category, const float Weight, const int32 MaxStack)
    {
        UAstrawildItemDefinition* Item = NewObject<UAstrawildItemDefinition>(Registry);
        Item->ItemId = Id;
        Item->DisplayName = FText::FromString(Name);
        Item->Description = FText::FromString(FString::Printf(TEXT("CODE_DEFAULT — %s"), *Name));
        Item->Category = Category;
        Item->Weight = Weight;
        Item->MaxStackSize = MaxStack;
        Registry->RegisterItem(Item);
        return Item;
    }

    UAstrawildWeaponDefinition* MakeWeapon(UAstrawildItemRegistrySubsystem* Registry, const FName Id, const FString& Name,
        const EAstrawildWeaponFamily Family, const EAstrawildTechTier Tier, const EAstrawildRarity Rarity,
        const EAstrawildWeaponFireMode FireMode, const float Damage, const float FireInterval,
        const EAstrawildElementType Element, const FName AmmoId)
    {
        UAstrawildWeaponDefinition* Weapon = NewObject<UAstrawildWeaponDefinition>(Registry);
        Weapon->WeaponId = Id;
        Weapon->DisplayName = FText::FromString(Name);
        Weapon->Description = FText::FromString(FString::Printf(TEXT("CODE_DEFAULT weapon profile — %s"), *Name));
        Weapon->Family = Family;
        Weapon->Tier = Tier;
        Weapon->Rarity = Rarity;
        Weapon->FireMode = FireMode;
        Weapon->DamagePerHit = Damage;
        Weapon->FireIntervalSeconds = FireInterval;
        Weapon->Element = Element;
        Weapon->AmmoItemId = AmmoId;
        Weapon->MuzzleVfxId = Id;
        Weapon->TrailVfxId = Id;
        Weapon->ImpactVfxId = Id;
        Weapon->FireSoundId = Id;
        Registry->RegisterWeapon(Weapon);
        return Weapon;
    }

    UAstrawildRecipeDefinition* MakeRecipe(UAstrawildItemRegistrySubsystem* Registry, const FName Id, const FString& Name,
        const TArray<FAstrawildItemStack>& Inputs, const TArray<FAstrawildItemStack>& Outputs,
        const float Duration, const FName Tech, const FName Station)
    {
        UAstrawildRecipeDefinition* Recipe = NewObject<UAstrawildRecipeDefinition>(Registry);
        Recipe->RecipeId = Id;
        Recipe->DisplayName = FText::FromString(Name);
        Recipe->Ingredients = Inputs;
        Recipe->Outputs = Outputs;
        Recipe->CraftDurationSeconds = Duration;
        Recipe->RequiredTechId = Tech;
        Recipe->RequiredStationId = Station;
        Registry->RegisterRecipe(Recipe);
        return Recipe;
    }

    UAstrawildTechnologyDefinition* MakeTech(UAstrawildItemRegistrySubsystem* Registry, const FName Id, const FString& Name,
        const EAstrawildTechEra Era, const EAstrawildResearchBranch Branch, const int32 Cost,
        const TArray<FName>& Prereqs, const TArray<FName>& Recipes)
    {
        UAstrawildTechnologyDefinition* Tech = NewObject<UAstrawildTechnologyDefinition>(Registry);
        Tech->TechId = Id;
        Tech->DisplayName = FText::FromString(Name);
        Tech->Description = FText::FromString(FString::Printf(TEXT("CODE_DEFAULT tech — %s"), *Name));
        Tech->Era = Era;
        Tech->Branch = Branch;
        Tech->ResearchCost = Cost;
        Tech->PrerequisiteTechIds = Prereqs;
        Tech->UnlockedRecipeIds = Recipes;
        Registry->RegisterTechnology(Tech);
        return Tech;
    }

    UAstrawildResourceNodeDefinition* MakeNode(UAstrawildItemRegistrySubsystem* Registry, const FName Id, const FString& Name,
        const FName ItemId, const EAstrawildRarity Rarity, const int32 QtyPerHarvest, const int32 MaxQty,
        const float RespawnSeconds, const FLinearColor Tint, const bool bHidden = false)
    {
        UAstrawildResourceNodeDefinition* Node = NewObject<UAstrawildResourceNodeDefinition>(Registry);
        Node->NodeId = Id;
        Node->DisplayName = FText::FromString(Name);
        Node->ResourceItemId = ItemId;
        Node->Rarity = Rarity;
        Node->QuantityPerHarvest = QtyPerHarvest;
        Node->MaxQuantity = MaxQty;
        Node->RespawnDurationSeconds = RespawnSeconds;
        Node->NodeTint = Tint;
        Node->bRequiresScannerDetection = bHidden;
        Node->VisualScale = Rarity == EAstrawildRarity::Common ? 1.0f : (Rarity == EAstrawildRarity::Uncommon ? 1.15f : 1.3f);
        Registry->RegisterResourceNode(Node);
        return Node;
    }

    UAstrawildEchoDefinition* MakeProductionEcho(UAstrawildItemRegistrySubsystem* Registry, const FName Id, const FString& Name,
        const EAstrawildElementType Element, const EAstrawildEchoRole Role,
        const float HP, const float ATK, const float DEF, const float Speed,
        const EAstrawildPersonality Personality, const EAstrawildActivityPattern Pattern,
        const TArray<FName>& Food, const float CaptureDifficulty, const EAstrawildElementType Weakness,
        const EAstrawildEchoFamily Family, const EAstrawildBodyPlan BodyPlan, const EAstrawildSizeClass SizeClass,
        const EAstrawildZone HomeZone, const EAstrawildRarity Rarity, const EAstrawildEchoPassive Passive,
        const TArray<FAstrawildWorkAffinity>& WorkAffinities, const TArray<FAstrawildItemStack>& Loot)
    {
        UAstrawildEchoDefinition* Echo = NewObject<UAstrawildEchoDefinition>(Registry);
        Echo->DefinitionId = Id;
        Echo->DisplayName = FText::FromString(Name);
        Echo->Description = FText::FromString(FString::Printf(TEXT("CODE_DEFAULT production Echo — %s"), *Name));
        Echo->Element = Element;
        Echo->Role = Role;
        Echo->BaseStats.MaxHealth = HP;
        Echo->BaseStats.AttackPower = ATK;
        Echo->BaseStats.Defense = DEF;
        Echo->BaseStats.MoveSpeed = Speed;
        Echo->BaseStats.CaptureResilience = 0.35f;
        Echo->DominantPersonality = Personality;
        Echo->ActivityPattern = Pattern;
        Echo->PreferredFoodIds = Food;
        Echo->CaptureDifficulty = CaptureDifficulty;
        Echo->WeaknessElement = Weakness;
        Echo->Family = Family;
        Echo->BodyPlan = BodyPlan;
        Echo->SizeClass = SizeClass;
        Echo->HomeZone = HomeZone;
        Echo->Rarity = Rarity;
        Echo->Passive = Passive;
        Echo->WorkAffinities = WorkAffinities;
        Echo->DefeatLoot = Loot;
        Echo->bHostileToPlayers = false;
        Echo->PrimaryTint = FLinearColor(0.72f, 0.66f, 0.52f);
        Echo->SecondaryTint = FLinearColor(0.42f, 0.5f, 0.46f);
        Registry->RegisterEcho(Echo);
        return Echo;
    }

    UAstrawildPOIDefinition* MakePOI(UAstrawildItemRegistrySubsystem* Registry, const FName Id, const FString& Name,
        const FString& Lore, const EAstrawildPOIType Type, const EAstrawildZone Zone,
        const FVector2D Offset, const float DiscoveryRadius, const FName LootTableId, const int32 ResearchReward,
        const bool bRequiresSignalScanner = false)
    {
        UAstrawildPOIDefinition* POI = NewObject<UAstrawildPOIDefinition>(Registry);
        POI->PoiId = Id;
        POI->DisplayName = FText::FromString(Name);
        POI->Description = FText::FromString(FString::Printf(TEXT("CODE_DEFAULT POI — %s"), *Name));
        POI->LoreLine = FText::FromString(Lore);
        POI->Type = Type;
        POI->Zone = Zone;
        POI->OffsetFromZoneCenter = Offset;
        POI->DiscoveryRadius = DiscoveryRadius;
        POI->RewardLootTableId = LootTableId;
        POI->ResearchReward = ResearchReward;
        POI->bRequiresSignalScanner = bRequiresSignalScanner;
        POI->DressingSetId = Id;
        Registry->RegisterPOI(POI);
        return POI;
    }

    UAstrawildWorldEventDefinition* MakeWorldEvent(UAstrawildItemRegistrySubsystem* Registry, const FName Id, const FString& Name,
        const EAstrawildWorldEventKind Kind, const float Weight, const float CooldownHours, const int32 MinDay,
        const int32 DurationMinutes, const EAstrawildZone Zone, const bool bRequiresNight)
    {
        UAstrawildWorldEventDefinition* Event = NewObject<UAstrawildWorldEventDefinition>(Registry);
        Event->EventId = Id;
        Event->DisplayName = FText::FromString(Name);
        Event->Description = FText::FromString(FString::Printf(TEXT("CODE_DEFAULT world event — %s"), *Name));
        Event->Kind = Kind;
        Event->RarityWeight = Weight;
        Event->CooldownGameHours = CooldownHours;
        Event->MinDay = MinDay;
        Event->DurationGameMinutes = DurationMinutes;
        Event->Zone = Zone;
        Event->bRequiresNight = bRequiresNight;
        Registry->RegisterWorldEvent(Event);
        return Event;
    }

    UAstrawildLootTableDefinition* MakeLoot(UAstrawildItemRegistrySubsystem* Registry, const FName Id,
        const TArray<FAstrawildItemStack>& Drops, const float BonusChance)
    {
        UAstrawildLootTableDefinition* Loot = NewObject<UAstrawildLootTableDefinition>(Registry);
        Loot->LootTableId = Id;
        Loot->GuaranteedDrops = Drops;
        Loot->BonusRollChance = BonusChance;
        Registry->RegisterLootTable(Loot);
        return Loot;
    }
}

// ---------------------------------------------------------------------------
// Weapons — 8 families, each a distinct firing archetype (Master Plan §8)
// ---------------------------------------------------------------------------

void UAstrawildProductionContent::BuildWeapons(UAstrawildItemRegistrySubsystem* Registry)
{
    // --- Kinetic: the starter ranged option (scrap-shot slingshot). ---
    MakeWeapon(Registry, TEXT("Weapon_Scrapshot"), TEXT("Scrapshot"),
        EAstrawildWeaponFamily::Kinetic, EAstrawildTechTier::Field, EAstrawildRarity::Common,
        EAstrawildWeaponFireMode::Projectile, 14.0f, 0.5f, EAstrawildElementType::Ash, TEXT("Item_Stone"));

    // --- Pulse: the existing lance, now a data profile. ---
    MakeWeapon(Registry, TEXT("Weapon_PulseLance"), TEXT("Pulse Lance"),
        EAstrawildWeaponFamily::Pulse, EAstrawildTechTier::Mk1, EAstrawildRarity::Uncommon,
        EAstrawildWeaponFireMode::Projectile, 22.0f, 0.35f, EAstrawildElementType::Pulse, TEXT("Item_EnergyCell"));

    // --- Plasma: slow, heavy bolts. ---
    UAstrawildWeaponDefinition* Plasma = MakeWeapon(Registry, TEXT("Weapon_PlasmaCharger"), TEXT("Plasma Charger"),
        EAstrawildWeaponFamily::Plasma, EAstrawildTechTier::Mk2, EAstrawildRarity::Rare,
        EAstrawildWeaponFireMode::Projectile, 38.0f, 0.8f, EAstrawildElementType::Ember, TEXT("Item_EnergyCell"));
    Plasma->ProjectileSpeed = 3800.0f;
    Plasma->ProjectileVisualScale = 0.55f;

    // --- Laser: instant beam, pierces two targets. ---
    UAstrawildWeaponDefinition* Lumen = MakeWeapon(Registry, TEXT("Weapon_LumenBeam"), TEXT("Lumen Beam"),
        EAstrawildWeaponFamily::Laser, EAstrawildTechTier::Mk2, EAstrawildRarity::Rare,
        EAstrawildWeaponFireMode::Beam, 18.0f, 0.22f, EAstrawildElementType::Light, TEXT("Item_EnergyCell"));
    Lumen->BeamRange = 15000.0f;
    Lumen->PierceCount = 2;

    // --- Arc: hitscan that chains between packed enemies. ---
    UAstrawildWeaponDefinition* Arc = MakeWeapon(Registry, TEXT("Weapon_ArcCaster"), TEXT("Arc Caster"),
        EAstrawildWeaponFamily::Arc, EAstrawildTechTier::Mk2, EAstrawildRarity::Rare,
        EAstrawildWeaponFireMode::ArcChain, 26.0f, 0.6f, EAstrawildElementType::Pulse, TEXT("Item_EnergyCell"));
    Arc->ChainCount = 3;
    Arc->ChainRadius = 700.0f;
    Arc->ChainDamageFraction = 0.6f;

    // --- Rail: slow, devastating line-piercer. ---
    UAstrawildWeaponDefinition* Rail = MakeWeapon(Registry, TEXT("Weapon_MagrailDriver"), TEXT("Magrail Driver"),
        EAstrawildWeaponFamily::Rail, EAstrawildTechTier::Mk3, EAstrawildRarity::Epic,
        EAstrawildWeaponFireMode::Beam, 85.0f, 1.8f, EAstrawildElementType::Ash, TEXT("Item_RailSlug"));
    Rail->BeamRange = 24000.0f;
    Rail->PierceCount = 5;

    // --- Missile: lock-on homing. ---
    UAstrawildWeaponDefinition* Seeker = MakeWeapon(Registry, TEXT("Weapon_SkysingerLauncher"), TEXT("Skysinger Launcher"),
        EAstrawildWeaponFamily::Missile, EAstrawildTechTier::Mk3, EAstrawildRarity::Epic,
        EAstrawildWeaponFireMode::HomingProjectile, 62.0f, 1.4f, EAstrawildElementType::Ember, TEXT("Item_SeekerMissile"));
    Seeker->ProjectileSpeed = 3200.0f;
    Seeker->ProjectileVisualScale = 0.5f;
    Seeker->LockOnConeHalfAngle = 20.0f;
    Seeker->LockOnRange = 11000.0f;
    Seeker->HomingAcceleration = 2600.0f;

    // --- Experimental: the Nova line-annihilator (ancient-alloy gated). ---
    UAstrawildWeaponDefinition* Nova = MakeWeapon(Registry, TEXT("Weapon_StarlancePrototype"), TEXT("Starlance Prototype"),
        EAstrawildWeaponFamily::Experimental, EAstrawildTechTier::Experimental, EAstrawildRarity::Legendary,
        EAstrawildWeaponFireMode::Beam, 140.0f, 2.5f, EAstrawildElementType::Light, TEXT("Item_NovaCell"));
    Nova->BeamRange = 30000.0f;
    Nova->PierceCount = 6;

    // Art pack bindings (Batch 4 — Visual Vertical Slice): held meshes, hero FX
    // and audio soft paths from AstrawildArtPack. Every binding is a SOFT path:
    // until Antigravity imports the ArtSource pack via Content/Python/AwPipeline,
    // none of them resolve and the zero-asset fallbacks stay live (CP-00 rule 2).
    for (const AstrawildArtPack::FWeaponArt& Art : AstrawildArtPack::GetWeaponArt())
    {
        UAstrawildWeaponDefinition* WeaponDef = Registry->FindWeapon(Art.WeaponId);
        if (!WeaponDef)
        {
            continue;
        }
        if (!Art.MeshPath.IsEmpty())
        {
            WeaponDef->Mesh = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(Art.MeshPath));
        }
        if (!Art.MuzzleVfxPath.IsEmpty())
        {
            WeaponDef->MuzzleFlashVfx = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(Art.MuzzleVfxPath));
        }
        if (!Art.ImpactVfxPath.IsEmpty())
        {
            WeaponDef->ImpactVfx = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(Art.ImpactVfxPath));
        }
        if (!Art.TrailVfxPath.IsEmpty())
        {
            WeaponDef->ProjectileTrailVfx = TSoftObjectPtr<UNiagaraSystem>(FSoftObjectPath(Art.TrailVfxPath));
        }
        if (!Art.FireSoundPath.IsEmpty())
        {
            WeaponDef->FireSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(Art.FireSoundPath));
        }
        if (!Art.ImpactSoundPath.IsEmpty())
        {
            WeaponDef->ImpactSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(Art.ImpactSoundPath));
        }
    }

    // --- Weapon items (inventory entities carrying the profiles). ---
    UAstrawildItemDefinition* Scrapshot = MakeItem(Registry, TEXT("Item_Scrapshot"), TEXT("Scrapshot"),
        EAstrawildItemCategory::Equipment, 2.0f, 1);
    Scrapshot->AttackPower = 4.0f;
    Scrapshot->Element = EAstrawildElementType::Ash;
    Scrapshot->EquipmentSlot = EAstrawildEquipmentSlot::Weapon;
    Scrapshot->bIsRangedWeapon = true;
    Scrapshot->AmmoItemId = TEXT("Item_Stone");
    Scrapshot->WeaponDefinitionId = TEXT("Weapon_Scrapshot");
    Scrapshot->TechTier = EAstrawildTechTier::Field;
    Scrapshot->Description = FText::FromString(TEXT("A survivor's slug-thrower [LMB]. Burns cheap fieldstone as ammunition."));
    Scrapshot->VendorPrice = 8;

    UAstrawildItemDefinition* PlasmaItem = MakeItem(Registry, TEXT("Item_PlasmaCharger"), TEXT("Plasma Charger"),
        EAstrawildItemCategory::Equipment, 5.5f, 1);
    PlasmaItem->AttackPower = 10.0f;
    PlasmaItem->Element = EAstrawildElementType::Ember;
    PlasmaItem->EquipmentSlot = EAstrawildEquipmentSlot::Weapon;
    PlasmaItem->bIsRangedWeapon = true;
    PlasmaItem->AmmoItemId = TEXT("Item_EnergyCell");
    PlasmaItem->WeaponDefinitionId = TEXT("Weapon_PlasmaCharger");
    PlasmaItem->TechTier = EAstrawildTechTier::Mk2;
    PlasmaItem->Rarity = EAstrawildRarity::Rare;
    PlasmaItem->Description = FText::FromString(TEXT("Lobs slow plasma bolts that burn on impact [LMB]."));
    PlasmaItem->VendorPrice = 28;

    UAstrawildItemDefinition* LumenItem = MakeItem(Registry, TEXT("Item_LumenBeam"), TEXT("Lumen Beam"),
        EAstrawildItemCategory::Equipment, 4.5f, 1);
    LumenItem->AttackPower = 6.0f;
    LumenItem->Element = EAstrawildElementType::Light;
    LumenItem->EquipmentSlot = EAstrawildEquipmentSlot::Weapon;
    LumenItem->bIsRangedWeapon = true;
    LumenItem->AmmoItemId = TEXT("Item_EnergyCell");
    LumenItem->WeaponDefinitionId = TEXT("Weapon_LumenBeam");
    LumenItem->TechTier = EAstrawildTechTier::Mk2;
    LumenItem->Rarity = EAstrawildRarity::Rare;
    LumenItem->Description = FText::FromString(TEXT("A coherent light lance [LMB] — instant, pierces two bodies."));
    LumenItem->VendorPrice = 30;

    UAstrawildItemDefinition* ArcItem = MakeItem(Registry, TEXT("Item_ArcCaster"), TEXT("Arc Caster"),
        EAstrawildItemCategory::Equipment, 5.0f, 1);
    ArcItem->AttackPower = 8.0f;
    ArcItem->Element = EAstrawildElementType::Pulse;
    ArcItem->EquipmentSlot = EAstrawildEquipmentSlot::Weapon;
    ArcItem->bIsRangedWeapon = true;
    ArcItem->AmmoItemId = TEXT("Item_EnergyCell");
    ArcItem->WeaponDefinitionId = TEXT("Weapon_ArcCaster");
    ArcItem->TechTier = EAstrawildTechTier::Mk2;
    ArcItem->Rarity = EAstrawildRarity::Rare;
    ArcItem->Description = FText::FromString(TEXT("Chains a pulse arc through packed enemies [LMB] — crowd control energy."));
    ArcItem->VendorPrice = 32;

    UAstrawildItemDefinition* RailItem = MakeItem(Registry, TEXT("Item_MagrailDriver"), TEXT("Magrail Driver"),
        EAstrawildItemCategory::Equipment, 8.0f, 1);
    RailItem->AttackPower = 14.0f;
    RailItem->Element = EAstrawildElementType::Ash;
    RailItem->EquipmentSlot = EAstrawildEquipmentSlot::Weapon;
    RailItem->bIsRangedWeapon = true;
    RailItem->AmmoItemId = TEXT("Item_RailSlug");
    RailItem->WeaponDefinitionId = TEXT("Weapon_MagrailDriver");
    RailItem->TechTier = EAstrawildTechTier::Mk3;
    RailItem->Rarity = EAstrawildRarity::Epic;
    RailItem->Description = FText::FromString(TEXT("Hyper-velocity slug [LMB] — one line, five bodies."));
    RailItem->VendorPrice = 44;

    UAstrawildItemDefinition* SeekerItem = MakeItem(Registry, TEXT("Item_SkysingerLauncher"), TEXT("Skysinger Launcher"),
        EAstrawildItemCategory::Equipment, 9.0f, 1);
    SeekerItem->AttackPower = 12.0f;
    SeekerItem->Element = EAstrawildElementType::Ember;
    SeekerItem->EquipmentSlot = EAstrawildEquipmentSlot::Weapon;
    SeekerItem->bIsRangedWeapon = true;
    SeekerItem->AmmoItemId = TEXT("Item_SeekerMissile");
    SeekerItem->WeaponDefinitionId = TEXT("Weapon_SkysingerLauncher");
    SeekerItem->TechTier = EAstrawildTechTier::Mk3;
    SeekerItem->Rarity = EAstrawildRarity::Epic;
    SeekerItem->Description = FText::FromString(TEXT("Fire-and-forget homing rockets [LMB] — tracks what you face."));
    SeekerItem->VendorPrice = 46;

    UAstrawildItemDefinition* NovaItem = MakeItem(Registry, TEXT("Item_StarlancePrototype"), TEXT("Starlance Prototype"),
        EAstrawildItemCategory::Equipment, 11.0f, 1);
    NovaItem->AttackPower = 20.0f;
    NovaItem->Element = EAstrawildElementType::Light;
    NovaItem->EquipmentSlot = EAstrawildEquipmentSlot::Weapon;
    NovaItem->bIsRangedWeapon = true;
    NovaItem->AmmoItemId = TEXT("Item_NovaCell");
    NovaItem->WeaponDefinitionId = TEXT("Weapon_StarlancePrototype");
    NovaItem->TechTier = EAstrawildTechTier::Experimental;
    NovaItem->Rarity = EAstrawildRarity::Legendary;
    NovaItem->Description = FText::FromString(TEXT("Ancient resonance weaponized [LMB] — a dawn-light line that erases ranks."));
    NovaItem->VendorPrice = 90;

    // --- Ammunition (new families; Plasma/Lumen/Arc reuse Pulse Cells). ---
    UAstrawildItemDefinition* RailSlug = MakeItem(Registry, TEXT("Item_RailSlug"), TEXT("Rail Slug"),
        EAstrawildItemCategory::Material, 0.3f, 40);
    RailSlug->Description = FText::FromString(TEXT("Machined tungsten slug — Magrail ammunition."));
    RailSlug->VendorPrice = 2;

    UAstrawildItemDefinition* SeekerMissile = MakeItem(Registry, TEXT("Item_SeekerMissile"), TEXT("Seeker Missile"),
        EAstrawildItemCategory::Material, 0.6f, 20);
    SeekerMissile->Description = FText::FromString(TEXT("A self-guiding micro-missile — Skysinger ammunition."));
    SeekerMissile->VendorPrice = 3;

    UAstrawildItemDefinition* NovaCell = MakeItem(Registry, TEXT("Item_NovaCell"), TEXT("Nova Cell"),
        EAstrawildItemCategory::Material, 0.5f, 12);
    NovaCell->Rarity = EAstrawildRarity::Epic;
    NovaCell->Description = FText::FromString(TEXT("A stabilized starlight charge — Starlance ammunition."));
    NovaCell->VendorPrice = 8;

    // --- Ancient alloy: the hidden-vein material feeding the experimental tier. ---
    UAstrawildItemDefinition* AncientAlloy = MakeItem(Registry, TEXT("Item_AncientAlloy"), TEXT("Ancient Alloy"),
        EAstrawildItemCategory::Material, 1.2f, 20);
    AncientAlloy->Rarity = EAstrawildRarity::Epic;
    AncientAlloy->Description = FText::FromString(TEXT("Pre-collapse alloy. Found only where a scanner can read the old signals."));
    AncientAlloy->VendorPrice = 12;

    // --- Weapon recipes (tech ladder: Armory → WeaponSystems → AdvancedBallistics → ExperimentalArsenal). ---
    MakeRecipe(Registry, TEXT("Recipe_Scrapshot"), TEXT("Scrapshot"),
        { Stack(TEXT("Item_Wood"), 4), Stack(TEXT("Item_Stone"), 6), Stack(TEXT("Item_Fiber"), 3) },
        { Stack(TEXT("Item_Scrapshot"), 1) }, 6.0f, TEXT("Tech_Armory"), TEXT("Station_Workbench"));
    MakeRecipe(Registry, TEXT("Recipe_PlasmaCharger"), TEXT("Plasma Charger"),
        { Stack(TEXT("Item_CrystalShard"), 6), Stack(TEXT("Item_EnergyCell"), 4), Stack(TEXT("Item_ChitinPlate"), 2) },
        { Stack(TEXT("Item_PlasmaCharger"), 1) }, 12.0f, TEXT("Tech_WeaponSystems"), TEXT("Station_Workbench"));
    MakeRecipe(Registry, TEXT("Recipe_LumenBeam"), TEXT("Lumen Beam"),
        { Stack(TEXT("Item_CrystalShard"), 5), Stack(TEXT("Item_EnergyCell"), 3), Stack(TEXT("Item_VoltCore"), 1) },
        { Stack(TEXT("Item_LumenBeam"), 1) }, 10.0f, TEXT("Tech_WeaponSystems"), TEXT("Station_Workbench"));
    MakeRecipe(Registry, TEXT("Recipe_ArcCaster"), TEXT("Arc Caster"),
        { Stack(TEXT("Item_VoltCore"), 2), Stack(TEXT("Item_CrystalShard"), 4), Stack(TEXT("Item_StormSilver"), 2) },
        { Stack(TEXT("Item_ArcCaster"), 1) }, 12.0f, TEXT("Tech_WeaponSystems"), TEXT("Station_Workbench"));
    MakeRecipe(Registry, TEXT("Recipe_MagrailDriver"), TEXT("Magrail Driver"),
        { Stack(TEXT("Item_StormSilver"), 6), Stack(TEXT("Item_DuneGlass"), 4), Stack(TEXT("Item_RailSlug"), 2) },
        { Stack(TEXT("Item_MagrailDriver"), 1) }, 18.0f, TEXT("Tech_AdvancedBallistics"), TEXT("Station_Workbench"));
    MakeRecipe(Registry, TEXT("Recipe_SkysingerLauncher"), TEXT("Skysinger Launcher"),
        { Stack(TEXT("Item_DuneGlass"), 5), Stack(TEXT("Item_CoralShard"), 4), Stack(TEXT("Item_SeekerMissile"), 2) },
        { Stack(TEXT("Item_SkysingerLauncher"), 1) }, 18.0f, TEXT("Tech_AdvancedBallistics"), TEXT("Station_Workbench"));
    MakeRecipe(Registry, TEXT("Recipe_StarlancePrototype"), TEXT("Starlance Prototype"),
        { Stack(TEXT("Item_AncientAlloy"), 3), Stack(TEXT("Item_DawnShard"), 5), Stack(TEXT("Item_NovaCell"), 2) },
        { Stack(TEXT("Item_StarlancePrototype"), 1) }, 30.0f, TEXT("Tech_ExperimentalArsenal"), TEXT("Station_Workbench"));

    MakeRecipe(Registry, TEXT("Recipe_RailSlugBatch"), TEXT("Rail Slug Batch"),
        { Stack(TEXT("Item_StormSilver"), 1), Stack(TEXT("Item_Stone"), 2) },
        { Stack(TEXT("Item_RailSlug"), 4) }, 5.0f, TEXT("Tech_AdvancedBallistics"), TEXT("Station_Workbench"));
    MakeRecipe(Registry, TEXT("Recipe_SeekerMissileBatch"), TEXT("Seeker Missile Batch"),
        { Stack(TEXT("Item_DuneGlass"), 1), Stack(TEXT("Item_Fiber"), 2), Stack(TEXT("Item_CrystalShard"), 1) },
        { Stack(TEXT("Item_SeekerMissile"), 2) }, 6.0f, TEXT("Tech_AdvancedBallistics"), TEXT("Station_Workbench"));
    MakeRecipe(Registry, TEXT("Recipe_NovaCell"), TEXT("Nova Cell"),
        { Stack(TEXT("Item_AncientAlloy"), 1), Stack(TEXT("Item_EnergyCell"), 2) },
        { Stack(TEXT("Item_NovaCell"), 1) }, 8.0f, TEXT("Tech_ExperimentalArsenal"), TEXT("Station_Workbench"));
}

// ---------------------------------------------------------------------------
// Armor tiers + scanner tiers (Master Plan §9/§10)
// ---------------------------------------------------------------------------

void UAstrawildProductionContent::BuildArmorAndScanners(UAstrawildItemRegistrySubsystem* Registry)
{
    // --- Mk II "Vanguard" set (early-mid tech tier). ---
    UAstrawildItemDefinition* VanguardHelm = MakeItem(Registry, TEXT("Item_VanguardHelm"), TEXT("Vanguard Helm"),
        EAstrawildItemCategory::Equipment, 3.0f, 1);
    VanguardHelm->EquipmentSlot = EAstrawildEquipmentSlot::Helmet;
    VanguardHelm->ArmorRating = 45.0f;
    VanguardHelm->ColdInsulationRating = 6.0f;
    VanguardHelm->HeatInsulationRating = 4.0f;
    VanguardHelm->TechTier = EAstrawildTechTier::Mk2;
    VanguardHelm->Rarity = EAstrawildRarity::Uncommon;
    VanguardHelm->Description = FText::FromString(TEXT("Mk II field helm: layered plate with split thermal bands."));
    VanguardHelm->VendorPrice = 22;

    UAstrawildItemDefinition* VanguardVest = MakeItem(Registry, TEXT("Item_VanguardVest"), TEXT("Vanguard Vest"),
        EAstrawildItemCategory::Equipment, 6.0f, 1);
    VanguardVest->EquipmentSlot = EAstrawildEquipmentSlot::Torso;
    VanguardVest->ArmorRating = 60.0f;
    VanguardVest->ColdInsulationRating = 5.0f;
    VanguardVest->HeatInsulationRating = 5.0f;
    VanguardVest->TechTier = EAstrawildTechTier::Mk2;
    VanguardVest->Rarity = EAstrawildRarity::Uncommon;
    VanguardVest->Description = FText::FromString(TEXT("Mk II torso plating — the all-climate workhorse."));
    VanguardVest->VendorPrice = 24;

    // --- Mk III "Bastion" set (late tier, rare materials). ---
    UAstrawildItemDefinition* BastionHelm = MakeItem(Registry, TEXT("Item_BastionHelm"), TEXT("Bastion Helm"),
        EAstrawildItemCategory::Equipment, 4.0f, 1);
    BastionHelm->EquipmentSlot = EAstrawildEquipmentSlot::Helmet;
    BastionHelm->ArmorRating = 70.0f;
    BastionHelm->ColdInsulationRating = 10.0f;
    BastionHelm->HeatInsulationRating = 8.0f;
    BastionHelm->TechTier = EAstrawildTechTier::Mk3;
    BastionHelm->Rarity = EAstrawildRarity::Rare;
    BastionHelm->Description = FText::FromString(TEXT("Mk III siege helm — deep-winter and forge-heat both answer to it."));
    BastionHelm->VendorPrice = 40;

    UAstrawildItemDefinition* BastionPlate = MakeItem(Registry, TEXT("Item_BastionPlate"), TEXT("Bastion Plate"),
        EAstrawildItemCategory::Equipment, 9.0f, 1);
    BastionPlate->EquipmentSlot = EAstrawildEquipmentSlot::Torso;
    BastionPlate->ArmorRating = 95.0f;
    BastionPlate->ColdInsulationRating = 9.0f;
    BastionPlate->HeatInsulationRating = 9.0f;
    BastionPlate->TechTier = EAstrawildTechTier::Mk3;
    BastionPlate->Rarity = EAstrawildRarity::Rare;
    BastionPlate->Description = FText::FromString(TEXT("Mk III cuirass — the wall you wear."));
    BastionPlate->VendorPrice = 44;

    // --- Experimental "Astralforged" exosuit (ancient-alloy apex). ---
    UAstrawildItemDefinition* Astral = MakeItem(Registry, TEXT("Item_AstralforgedExosuit"), TEXT("Astralforged Exosuit"),
        EAstrawildItemCategory::Equipment, 12.0f, 1);
    Astral->EquipmentSlot = EAstrawildEquipmentSlot::Exosuit;
    Astral->ArmorRating = 55.0f;
    Astral->ColdInsulationRating = 12.0f;
    Astral->HeatInsulationRating = 12.0f;
    Astral->StaminaRegenBonus = 6.0f;
    Astral->CarryWeightBonus = 60.0f;
    Astral->MoveSpeedBonus = 0.20f;
    Astral->TechTier = EAstrawildTechTier::Experimental;
    Astral->Rarity = EAstrawildRarity::Epic;
    Astral->Description = FText::FromString(TEXT("An ancient frame rebuilt: +60kg carry, +20% speed, twelve degrees of calm on both thermometers."));
    Astral->VendorPrice = 85;

    // --- Scanner tier 2: hidden veins within reach. ---
    UAstrawildItemDefinition* ArrayScanner = MakeItem(Registry, TEXT("Item_ArrayScanner"), TEXT("Array Scanner"),
        EAstrawildItemCategory::Equipment, 2.0f, 1);
    ArrayScanner->EquipmentSlot = EAstrawildEquipmentSlot::Scanner;
    ArrayScanner->ScannerSpeedMultiplier = 4.0f;
    ArrayScanner->ScannerRangeMultiplier = 1.6f;
    ArrayScanner->bHiddenResourceDetection = true;
    ArrayScanner->TechTier = EAstrawildTechTier::Mk2;
    ArrayScanner->Rarity = EAstrawildRarity::Rare;
    ArrayScanner->Description = FText::FromString(TEXT("A phased array scanner: wider, faster, and it reads hidden mineral veins [V]."));
    ArrayScanner->VendorPrice = 36;

    // --- Scanner tier 3: the ancient-signal oracle (POI magnet). ---
    UAstrawildItemDefinition* OracleScanner = MakeItem(Registry, TEXT("Item_OracleScanner"), TEXT("Oracle Scanner"),
        EAstrawildItemCategory::Equipment, 2.5f, 1);
    OracleScanner->EquipmentSlot = EAstrawildEquipmentSlot::Scanner;
    OracleScanner->ScannerSpeedMultiplier = 5.0f;
    OracleScanner->ScannerRangeMultiplier = 2.5f;
    OracleScanner->bHiddenResourceDetection = true;
    OracleScanner->bAncientSignalTracking = true;
    OracleScanner->TechTier = EAstrawildTechTier::Experimental;
    OracleScanner->Rarity = EAstrawildRarity::Epic;
    OracleScanner->Description = FText::FromString(TEXT("Resonance oracle: doubles every discovery radius and tracks the ancient signals [V]."));
    OracleScanner->VendorPrice = 70;

    // --- Armor/scanner recipes (Armor + Scanner branches). ---
    MakeRecipe(Registry, TEXT("Recipe_VanguardHelm"), TEXT("Vanguard Helm"),
        { Stack(TEXT("Item_CrystalShard"), 3), Stack(TEXT("Item_ChitinPlate"), 2), Stack(TEXT("Item_Fiber"), 4) },
        { Stack(TEXT("Item_VanguardHelm"), 1) }, 10.0f, TEXT("Tech_ExosuitEngineering"), TEXT("Station_Workbench"));
    MakeRecipe(Registry, TEXT("Recipe_VanguardVest"), TEXT("Vanguard Vest"),
        { Stack(TEXT("Item_ChitinPlate"), 3), Stack(TEXT("Item_CrystalShard"), 4), Stack(TEXT("Item_EmberAsh"), 2) },
        { Stack(TEXT("Item_VanguardVest"), 1) }, 12.0f, TEXT("Tech_ExosuitEngineering"), TEXT("Station_Workbench"));
    MakeRecipe(Registry, TEXT("Recipe_BastionHelm"), TEXT("Bastion Helm"),
        { Stack(TEXT("Item_StormSilver"), 4), Stack(TEXT("Item_CrystalShard"), 6), Stack(TEXT("Item_Frostbloom"), 2) },
        { Stack(TEXT("Item_BastionHelm"), 1) }, 16.0f, TEXT("Tech_ExosuitEngineering"), TEXT("Station_Workbench"));
    MakeRecipe(Registry, TEXT("Recipe_BastionPlate"), TEXT("Bastion Plate"),
        { Stack(TEXT("Item_StormSilver"), 6), Stack(TEXT("Item_ChitinPlate"), 4), Stack(TEXT("Item_CoralShard"), 3) },
        { Stack(TEXT("Item_BastionPlate"), 1) }, 18.0f, TEXT("Tech_ExosuitEngineering"), TEXT("Station_Workbench"));
    MakeRecipe(Registry, TEXT("Recipe_AstralforgedExosuit"), TEXT("Astralforged Exosuit"),
        { Stack(TEXT("Item_AncientAlloy"), 2), Stack(TEXT("Item_DawnShard"), 4), Stack(TEXT("Item_CrystalShard"), 6) },
        { Stack(TEXT("Item_AstralforgedExosuit"), 1) }, 26.0f, TEXT("Tech_ExosuitEngineering"), TEXT("Station_Workbench"));
    MakeRecipe(Registry, TEXT("Recipe_ArrayScanner"), TEXT("Array Scanner"),
        { Stack(TEXT("Item_CrystalShard"), 4), Stack(TEXT("Item_VoltCore"), 1), Stack(TEXT("Item_EnergyCell"), 2) },
        { Stack(TEXT("Item_ArrayScanner"), 1) }, 12.0f, TEXT("Tech_ScannerArray"), TEXT("Station_Workbench"));
    MakeRecipe(Registry, TEXT("Recipe_OracleScanner"), TEXT("Oracle Scanner"),
        { Stack(TEXT("Item_AncientAlloy"), 1), Stack(TEXT("Item_CrystalShard"), 6), Stack(TEXT("Item_VoltCore"), 2) },
        { Stack(TEXT("Item_OracleScanner"), 1) }, 20.0f, TEXT("Tech_ScannerArray"), TEXT("Station_Workbench"));
}

// ---------------------------------------------------------------------------
// Robotics — drone modules + specialist robot chassis (Master Plan §11/§12)
// ---------------------------------------------------------------------------

void UAstrawildProductionContent::BuildRobotics(UAstrawildItemRegistrySubsystem* Registry)
{
    // --- Drone modules: items that auto-apply while carried (best per category). ---
    UAstrawildItemDefinition* CellExtender = MakeItem(Registry, TEXT("Item_DroneCellExtender"), TEXT("Drone Cell Extender"),
        EAstrawildItemCategory::Equipment, 1.5f, 1);
    CellExtender->DroneBatteryBonusSeconds = 600.0f;
    CellExtender->TechTier = EAstrawildTechTier::Mk2;
    CellExtender->Description = FText::FromString(TEXT("Fits any deployed drone: +10 minutes of flight per charge."));
    CellExtender->VendorPrice = 16;

    UAstrawildItemDefinition* FocusedArray = MakeItem(Registry, TEXT("Item_DroneFocusedArray"), TEXT("Drone Focused Array"),
        EAstrawildItemCategory::Equipment, 1.2f, 1);
    FocusedArray->DroneScanRadiusBonus = 600.0f;
    FocusedArray->DroneScanRateBonus = 2.0f;
    FocusedArray->TechTier = EAstrawildTechTier::Mk2;
    FocusedArray->Description = FText::FromString(TEXT("A sharper sensor head: +6m scan radius and faster journal progress."));
    FocusedArray->VendorPrice = 18;

    UAstrawildItemDefinition* SalvageClaw = MakeItem(Registry, TEXT("Item_DroneSalvageClaw"), TEXT("Drone Salvage Claw"),
        EAstrawildItemCategory::Equipment, 2.0f, 1);
    SalvageClaw->DroneHarvestRadiusBonus = 500.0f;
    SalvageClaw->TechTier = EAstrawildTechTier::Mk2;
    SalvageClaw->Description = FText::FromString(TEXT("Extends the drone's harvest reach by 5m — it gathers while you fight."));
    SalvageClaw->VendorPrice = 18;

    // --- Specialist robot chassis (data definitions). ---
    UAstrawildRobotDefinition* Borebot = NewObject<UAstrawildRobotDefinition>(Registry);
    Borebot->RobotId = TEXT("Robot_Borebot");
    Borebot->DisplayName = FText::FromString(TEXT("Borebot"));
    Borebot->Description = FText::FromString(TEXT("Mining chassis: 1.6x on mining sites, 0.5x anywhere else."));
    Borebot->PrimaryWorkType = EAstrawildWorkType::Mining;
    Borebot->SpecialistWorkRate = 1.6f;
    Borebot->GenericWorkRate = 0.5f;
    Borebot->MoveSpeedMultiplier = 0.8f;
    Borebot->PrimaryTint = FLinearColor(0.85f, 0.55f, 0.25f);
    Registry->RegisterRobot(Borebot);

    UAstrawildRobotDefinition* Cultivator = NewObject<UAstrawildRobotDefinition>(Registry);
    Cultivator->RobotId = TEXT("Robot_Cultivator");
    Cultivator->DisplayName = FText::FromString(TEXT("Cultivator Unit"));
    Cultivator->Description = FText::FromString(TEXT("Farming chassis: 1.5x on farm sites, 0.5x anywhere else."));
    Cultivator->PrimaryWorkType = EAstrawildWorkType::Farming;
    Cultivator->SpecialistWorkRate = 1.5f;
    Cultivator->GenericWorkRate = 0.5f;
    Cultivator->MoveSpeedMultiplier = 1.0f;
    Cultivator->PrimaryTint = FLinearColor(0.35f, 0.8f, 0.4f);
    Registry->RegisterRobot(Cultivator);

    UAstrawildRobotDefinition* Sentinel = NewObject<UAstrawildRobotDefinition>(Registry);
    Sentinel->RobotId = TEXT("Robot_Sentinel");
    Sentinel->DisplayName = FText::FromString(TEXT("Sentinel Frame"));
    Sentinel->Description = FText::FromString(TEXT("Defense chassis: mans defensive posts, 1.4x on defense sites."));
    Sentinel->PrimaryWorkType = EAstrawildWorkType::Defense;
    Sentinel->SpecialistWorkRate = 1.4f;
    Sentinel->GenericWorkRate = 0.6f;
    Sentinel->MoveSpeedMultiplier = 1.3f;
    Sentinel->PrimaryTint = FLinearColor(0.85f, 0.3f, 0.3f);
    Sentinel->Rarity = EAstrawildRarity::Rare;
    Registry->RegisterRobot(Sentinel);

    // --- Robot items (deployables carrying the chassis id). ---
    UAstrawildItemDefinition* BorebotItem = MakeItem(Registry, TEXT("Item_RobotBorebot"), TEXT("Borebot"),
        EAstrawildItemCategory::Equipment, 11.0f, 1);
    BorebotItem->EquipmentSlot = EAstrawildEquipmentSlot::Auto;
    BorebotItem->bDeploysRobot = true;
    BorebotItem->RobotDefinitionId = TEXT("Robot_Borebot");
    BorebotItem->TechTier = EAstrawildTechTier::Mk2;
    BorebotItem->Description = FText::FromString(TEXT("Deploy [J]: a mining chassis that eats rock sites alive."));
    BorebotItem->VendorPrice = 30;

    UAstrawildItemDefinition* CultivatorItem = MakeItem(Registry, TEXT("Item_RobotCultivator"), TEXT("Cultivator Unit"),
        EAstrawildItemCategory::Equipment, 10.0f, 1);
    CultivatorItem->EquipmentSlot = EAstrawildEquipmentSlot::Auto;
    CultivatorItem->bDeploysRobot = true;
    CultivatorItem->RobotDefinitionId = TEXT("Robot_Cultivator");
    CultivatorItem->TechTier = EAstrawildTechTier::Mk2;
    CultivatorItem->Description = FText::FromString(TEXT("Deploy [J]: gentle hands that never stop harvesting."));
    CultivatorItem->VendorPrice = 30;

    UAstrawildItemDefinition* SentinelItem = MakeItem(Registry, TEXT("Item_RobotSentinel"), TEXT("Sentinel Frame"),
        EAstrawildItemCategory::Equipment, 12.0f, 1);
    SentinelItem->EquipmentSlot = EAstrawildEquipmentSlot::Auto;
    SentinelItem->bDeploysRobot = true;
    SentinelItem->RobotDefinitionId = TEXT("Robot_Sentinel");
    SentinelItem->TechTier = EAstrawildTechTier::Mk2;
    SentinelItem->Rarity = EAstrawildRarity::Rare;
    SentinelItem->Description = FText::FromString(TEXT("Deploy [J]: a watchdog frame for the perimeter posts."));
    SentinelItem->VendorPrice = 34;

    // --- Robotics recipes (Automation branch tier 2). ---
    MakeRecipe(Registry, TEXT("Recipe_DroneCellExtender"), TEXT("Drone Cell Extender"),
        { Stack(TEXT("Item_EnergyCell"), 3), Stack(TEXT("Item_CrystalShard"), 2) },
        { Stack(TEXT("Item_DroneCellExtender"), 1) }, 8.0f, TEXT("Tech_AutomationII"), TEXT("Station_Workbench"));
    MakeRecipe(Registry, TEXT("Recipe_DroneFocusedArray"), TEXT("Drone Focused Array"),
        { Stack(TEXT("Item_CrystalShard"), 3), Stack(TEXT("Item_VoltCore"), 1) },
        { Stack(TEXT("Item_DroneFocusedArray"), 1) }, 8.0f, TEXT("Tech_AutomationII"), TEXT("Station_Workbench"));
    MakeRecipe(Registry, TEXT("Recipe_DroneSalvageClaw"), TEXT("Drone Salvage Claw"),
        { Stack(TEXT("Item_ChitinPlate"), 2), Stack(TEXT("Item_StormSilver"), 1) },
        { Stack(TEXT("Item_DroneSalvageClaw"), 1) }, 8.0f, TEXT("Tech_AutomationII"), TEXT("Station_Workbench"));
    MakeRecipe(Registry, TEXT("Recipe_RobotBorebot"), TEXT("Borebot"),
        { Stack(TEXT("Item_Stone"), 8), Stack(TEXT("Item_StormSilver"), 2), Stack(TEXT("Item_EnergyCell"), 3) },
        { Stack(TEXT("Item_RobotBorebot"), 1) }, 16.0f, TEXT("Tech_AutomationII"), TEXT("Station_Workbench"));
    MakeRecipe(Registry, TEXT("Recipe_RobotCultivator"), TEXT("Cultivator Unit"),
        { Stack(TEXT("Item_Fiber"), 8), Stack(TEXT("Item_Dawnbloom"), 2), Stack(TEXT("Item_EnergyCell"), 3) },
        { Stack(TEXT("Item_RobotCultivator"), 1) }, 16.0f, TEXT("Tech_AutomationII"), TEXT("Station_Workbench"));
    MakeRecipe(Registry, TEXT("Recipe_RobotSentinel"), TEXT("Sentinel Frame"),
        { Stack(TEXT("Item_ChitinPlate"), 3), Stack(TEXT("Item_StormSilver"), 3), Stack(TEXT("Item_EnergyCell"), 4) },
        { Stack(TEXT("Item_RobotSentinel"), 1) }, 18.0f, TEXT("Tech_AutomationII"), TEXT("Station_Workbench"));
}

// ---------------------------------------------------------------------------
// Resource nodes — deterministic identity (P0 fix, Master Plan §1/§5)
// ---------------------------------------------------------------------------

void UAstrawildProductionContent::BuildResourceNodes(UAstrawildItemRegistrySubsystem* Registry)
{
    MakeNode(Registry, TEXT("Node_Dawnwood"), TEXT("Dawnwood Stand"), TEXT("Item_Wood"),
        EAstrawildRarity::Common, 2, 3, 30.0f, FLinearColor(0.55f, 0.4f, 0.25f));
    MakeNode(Registry, TEXT("Node_Fieldstone"), TEXT("Fieldstone Outcrop"), TEXT("Item_Stone"),
        EAstrawildRarity::Common, 2, 3, 30.0f, FLinearColor(0.6f, 0.6f, 0.58f));
    MakeNode(Registry, TEXT("Node_Sunfiber"), TEXT("Sunfiber Thicket"), TEXT("Item_Fiber"),
        EAstrawildRarity::Common, 2, 3, 25.0f, FLinearColor(0.4f, 0.7f, 0.35f));
    MakeNode(Registry, TEXT("Node_DawnCrystal"), TEXT("Dawn Crystal Cluster"), TEXT("Item_CrystalShard"),
        EAstrawildRarity::Uncommon, 2, 3, 60.0f, FLinearColor(0.45f, 0.75f, 1.0f));
    MakeNode(Registry, TEXT("Node_EmberAsh"), TEXT("Ember Ash Vent"), TEXT("Item_EmberAsh"),
        EAstrawildRarity::Uncommon, 2, 3, 55.0f, FLinearColor(0.9f, 0.45f, 0.2f));
    MakeNode(Registry, TEXT("Node_SeaPearl"), TEXT("Sea Pearl Bed"), TEXT("Item_SeaPearl"),
        EAstrawildRarity::Uncommon, 1, 2, 70.0f, FLinearColor(0.85f, 0.9f, 1.0f));
    MakeNode(Registry, TEXT("Node_CoralShard"), TEXT("Coral Shard Reef"), TEXT("Item_CoralShard"),
        EAstrawildRarity::Uncommon, 2, 3, 60.0f, FLinearColor(1.0f, 0.55f, 0.6f));
    MakeNode(Registry, TEXT("Node_DuneGlass"), TEXT("Dune Glass Seam"), TEXT("Item_DuneGlass"),
        EAstrawildRarity::Rare, 1, 2, 90.0f, FLinearColor(0.95f, 0.85f, 0.5f));
    MakeNode(Registry, TEXT("Node_StormSilver"), TEXT("Storm Silver Vein"), TEXT("Item_StormSilver"),
        EAstrawildRarity::Rare, 1, 2, 100.0f, FLinearColor(0.7f, 0.75f, 0.95f));
    MakeNode(Registry, TEXT("Node_AncientVein"), TEXT("Hidden Alloy Vein"), TEXT("Item_AncientAlloy"),
        EAstrawildRarity::Epic, 1, 1, 480.0f, FLinearColor(0.6f, 0.95f, 0.9f), true);

    // Art pack bindings (Batch 4): crystal/rock cluster meshes per node type.
    // Soft paths — the rarity-shape placeholder stays active until import.
    for (const AstrawildArtPack::FNodeArt& Art : AstrawildArtPack::GetNodeArt())
    {
        if (UAstrawildResourceNodeDefinition* NodeDef = Registry->FindResourceNode(Art.NodeId))
        {
            NodeDef->MeshOverride = TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(Art.MeshPath));
        }
    }
}

// ---------------------------------------------------------------------------
// Work sites — data-driven consume→produce chains (Master Plan §7)
// ---------------------------------------------------------------------------

void UAstrawildProductionContent::BuildWorkSites(UAstrawildItemRegistrySubsystem* Registry)
{
    UAstrawildWorkSiteDefinition* CampGathering = NewObject<UAstrawildWorkSiteDefinition>(Registry);
    CampGathering->SiteId = TEXT("Site_CampGathering");
    CampGathering->DisplayName = FText::FromString(TEXT("Camp Gathering Post"));
    CampGathering->WorkType = EAstrawildWorkType::Gathering;
    CampGathering->OutputItemId = TEXT("Item_Fiber");
    CampGathering->OutputQuantity = 1;
    CampGathering->SecondsPerOutput = 10.0f;
    CampGathering->Zone = EAstrawildZone::DawnFields;
    CampGathering->OffsetFromZoneCenter = FVector2D(-600.0f, 900.0f);
    Registry->RegisterWorkSite(CampGathering);

    UAstrawildWorkSiteDefinition* CampFarm = NewObject<UAstrawildWorkSiteDefinition>(Registry);
    CampFarm->SiteId = TEXT("Site_CampFarm");
    CampFarm->DisplayName = FText::FromString(TEXT("Camp Berry Plot"));
    CampFarm->WorkType = EAstrawildWorkType::Farming;
    CampFarm->OutputItemId = TEXT("Item_Berry");
    CampFarm->OutputQuantity = 1;
    CampFarm->SecondsPerOutput = 14.0f;
    CampFarm->Zone = EAstrawildZone::DawnFields;
    CampFarm->OffsetFromZoneCenter = FVector2D(700.0f, 900.0f);
    Registry->RegisterWorkSite(CampFarm);

    // Powered mining station in Ember Ridge — the power grid finally FEEDS economy.
    UAstrawildWorkSiteDefinition* RidgeMining = NewObject<UAstrawildWorkSiteDefinition>(Registry);
    RidgeMining->SiteId = TEXT("Site_RidgeMining");
    RidgeMining->DisplayName = FText::FromString(TEXT("Ridge Breaker Rig"));
    RidgeMining->WorkType = EAstrawildWorkType::Mining;
    RidgeMining->OutputItemId = TEXT("Item_Stone");
    RidgeMining->OutputQuantity = 2;
    RidgeMining->SecondsPerOutput = 18.0f;
    RidgeMining->bRequiresPower = true;
    RidgeMining->Zone = EAstrawildZone::EmberRidge;
    RidgeMining->OffsetFromZoneCenter = FVector2D(-4000.0f, 3000.0f);
    Registry->RegisterWorkSite(RidgeMining);

    // The consume→produce showcase: raw meat goes in, seared meat comes out.
    UAstrawildWorkSiteDefinition* CampKitchen = NewObject<UAstrawildWorkSiteDefinition>(Registry);
    CampKitchen->SiteId = TEXT("Site_CampKitchen");
    CampKitchen->DisplayName = FText::FromString(TEXT("Camp Kitchen"));
    CampKitchen->WorkType = EAstrawildWorkType::Cooking;
    CampKitchen->OutputItemId = TEXT("Item_CookedMeat");
    CampKitchen->OutputQuantity = 1;
    CampKitchen->InputItems = { Stack(TEXT("Item_RawMeat"), 1) };
    CampKitchen->SecondsPerOutput = 8.0f;
    CampKitchen->Zone = EAstrawildZone::DawnFields;
    CampKitchen->OffsetFromZoneCenter = FVector2D(0.0f, 1400.0f);
    Registry->RegisterWorkSite(CampKitchen);
}

// ---------------------------------------------------------------------------
// World events — 9 data-driven archetypes (Master Plan §19)
// ---------------------------------------------------------------------------

void UAstrawildProductionContent::BuildWorldEvents(UAstrawildItemRegistrySubsystem* Registry)
{
    // Support loot tables first (referenced by events + POIs).
    MakeLoot(Registry, TEXT("Loot_EventSupplyDrop"),
        { Stack(TEXT("Item_EnergyCell"), 6), Stack(TEXT("Item_Bandage"), 2), Stack(TEXT("Item_DawnShard"), 1) }, 0.5f);
    MakeLoot(Registry, TEXT("Loot_EventMeteor"),
        { Stack(TEXT("Item_StormSilver"), 3), Stack(TEXT("Item_DuneGlass"), 2) }, 0.4f);
    MakeLoot(Registry, TEXT("Loot_POIAncient"),
        { Stack(TEXT("Item_AncientAlloy"), 1), Stack(TEXT("Item_DawnShard"), 2) }, 0.35f);
    MakeLoot(Registry, TEXT("Loot_POIRuin"),
        { Stack(TEXT("Item_CrystalShard"), 3), Stack(TEXT("Item_EnergyCell"), 2) }, 0.3f);
    MakeLoot(Registry, TEXT("Loot_POIWatchtower"),
        { Stack(TEXT("Item_Bandage"), 2), Stack(TEXT("Item_WaterFlask"), 2) }, 0.25f);

    UAstrawildWorldEventDefinition* StormSurge = MakeWorldEvent(Registry, TEXT("Event_StormSurge"), TEXT("Storm Surge"),
        EAstrawildWorldEventKind::StormSurge, 1.2f, 10.0f, 2, 90, EAstrawildZone::None, false);
    StormSurge->ForcedWeather = EAstrawildWeatherState::Storm;
    StormSurge->bForcesWeather = true;
    StormSurge->ResearchPointReward = 2;
    StormSurge->Description = FText::FromString(TEXT("A storm cell parks over the vale — visibility drops, pulse energy crackles."));

    UAstrawildWorldEventDefinition* Migration = MakeWorldEvent(Registry, TEXT("Event_GreatMigration"), TEXT("Great Migration"),
        EAstrawildWorldEventKind::Migration, 1.0f, 24.0f, 3, 120, EAstrawildZone::VerdantReach, false);
    Migration->SpeciesBoostId = TEXT("Echo_Sprigling");
    Migration->SpeciesBoostCount = 4;
    Migration->Description = FText::FromString(TEXT("Sprigling herds flow through the Verdant Reach."));

    UAstrawildWorldEventDefinition* ResourceSurge = MakeWorldEvent(Registry, TEXT("Event_ResourceSurge"), TEXT("Resource Surge"),
        EAstrawildWorldEventKind::ResourceSurge, 1.0f, 16.0f, 2, 0, EAstrawildZone::Glimmerwood, false);
    ResourceSurge->BonusNodeIds = { TEXT("Node_DawnCrystal"), TEXT("Node_DawnCrystal"), TEXT("Node_DawnCrystal") };
    ResourceSurge->ResearchPointReward = 1;
    ResourceSurge->Description = FText::FromString(TEXT("The Glimwood spits out crystal clusters overnight."));

    UAstrawildWorldEventDefinition* SupplyDrop = MakeWorldEvent(Registry, TEXT("Event_SupplyDrop"), TEXT("Supply Drop"),
        EAstrawildWorldEventKind::SupplyDrop, 0.8f, 20.0f, 2, 0, EAstrawildZone::DawnFields, false);
    SupplyDrop->RewardLootTableId = TEXT("Loot_EventSupplyDrop");
    SupplyDrop->Description = FText::FromString(TEXT("An orbital cache falls somewhere near camp — the crate's contents land in your pack."));

    UAstrawildWorldEventDefinition* AncientSignal = MakeWorldEvent(Registry, TEXT("Event_AncientSignal"), TEXT("Ancient Signal"),
        EAstrawildWorldEventKind::AncientSignal, 0.6f, 30.0f, 4, 0, EAstrawildZone::HollowApproach, false);
    AncientSignal->ResearchPointReward = 5;
    AncientSignal->Description = FText::FromString(TEXT("The old tower under the Hollow Approach hums — the journal fills itself."));

    UAstrawildWorldEventDefinition* NightRaid = MakeWorldEvent(Registry, TEXT("Event_NightRaid"), TEXT("Night Raid"),
        EAstrawildWorldEventKind::NightRaid, 0.9f, 18.0f, 3, 60, EAstrawildZone::None, true);
    NightRaid->RaidHostileCount = 3;
    NightRaid->Description = FText::FromString(TEXT("Gloomfangs test the camp perimeter — keep the fires high."));

    UAstrawildWorldEventDefinition* MeteorFall = MakeWorldEvent(Registry, TEXT("Event_MeteorFall"), TEXT("Meteor Fall"),
        EAstrawildWorldEventKind::MeteorFall, 0.7f, 26.0f, 4, 0, EAstrawildZone::FrostveilExpanse, false);
    MeteorFall->BonusNodeIds = { TEXT("Node_StormSilver"), TEXT("Node_StormSilver"), TEXT("Node_DuneGlass") };
    MeteorFall->RewardLootTableId = TEXT("Loot_EventMeteor");
    MeteorFall->Description = FText::FromString(TEXT("A star falls into the Frostveil — rare metals scatter with the impact."));

    UAstrawildWorldEventDefinition* RareBloom = MakeWorldEvent(Registry, TEXT("Event_RareEchoBloom"), TEXT("Rare Echo Bloom"),
        EAstrawildWorldEventKind::RareEchoBloom, 0.5f, 48.0f, 5, 90, EAstrawildZone::Glimmerwood, false);
    RareBloom->SpeciesBoostId = TEXT("Echo_Auroraling");
    RareBloom->SpeciesBoostCount = 1;
    RareBloom->ResearchPointReward = 3;
    RareBloom->Description = FText::FromString(TEXT("An Auroraling shows itself in the deep Glimmerwood — briefly."));

    UAstrawildWorldEventDefinition* BossStirring = MakeWorldEvent(Registry, TEXT("Event_BossStirring"), TEXT("Boss Stirring"),
        EAstrawildWorldEventKind::BossStirring, 0.5f, 36.0f, 6, 30, EAstrawildZone::HollowApproach, false);
    BossStirring->ResearchPointReward = 4;
    BossStirring->Description = FText::FromString(TEXT("The Underlight Warden shifts in its sleep — the whole zone holds its breath."));
}

// ---------------------------------------------------------------------------
// POIs — discovery content anchoring the Visual Vertical Slice (Master Plan §5/§31)
// ---------------------------------------------------------------------------

void UAstrawildProductionContent::BuildPOIs(UAstrawildItemRegistrySubsystem* Registry)
{
    MakePOI(Registry, TEXT("POI_FirstLightRuin"), TEXT("First Light Ruin"),
        TEXT("Where the first survey team camped — the walls remember."),
        EAstrawildPOIType::Ruin, EAstrawildZone::DawnFields, FVector2D(2000.0f, -1500.0f), 1400.0f,
        TEXT("Loot_POIRuin"), 2);

    MakePOI(Registry, TEXT("POI_DawnsteadWatchtower"), TEXT("Dawnstead Watchtower"),
        TEXT("The wardens still climb it at dusk to read the horizon."),
        EAstrawildPOIType::Watchtower, EAstrawildZone::DawnFields, FVector2D(-3000.0f, 2500.0f), 1400.0f,
        TEXT("Loot_POIWatchtower"), 2);

    MakePOI(Registry, TEXT("POI_SunkencollarCave"), TEXT("Sunkencollar Cave"),
        TEXT("Something breathes cold out of this hillside."),
        EAstrawildPOIType::CaveEntrance, EAstrawildZone::DuskMarsh, FVector2D(2500.0f, 1800.0f), 1200.0f,
        NAME_None, 3);

    MakePOI(Registry, TEXT("POI_GlimmerwoodMonolith"), TEXT("Glimmerwood Monolith"),
        TEXT("Older than the wood around it. The moss will not touch it."),
        EAstrawildPOIType::AncientTech, EAstrawildZone::Glimmerwood, FVector2D(-1200.0f, 2200.0f), 1300.0f,
        TEXT("Loot_POIAncient"), 5);

    MakePOI(Registry, TEXT("POI_EmberFoundry"), TEXT("Ember Foundry Husk"),
        TEXT("A pre-collapse forge, cracked open like an egg."),
        EAstrawildPOIType::Ruin, EAstrawildZone::EmberRidge, FVector2D(3200.0f, -2400.0f), 1400.0f,
        TEXT("Loot_POIRuin"), 4);

    MakePOI(Registry, TEXT("POI_FrostveilSignalSource"), TEXT("Frostveil Signal Source"),
        TEXT("The oracle scanner pins it under the ice shelf — it is transmitting."),
        EAstrawildPOIType::SignalSource, EAstrawildZone::FrostveilExpanse, FVector2D(1500.0f, -1500.0f), 1000.0f,
        TEXT("Loot_POIAncient"), 6, true);

    MakePOI(Registry, TEXT("POI_HollowApproachSpire"), TEXT("Hollow Spire"),
        TEXT("It was a doorway once. It is waiting to be one again."),
        EAstrawildPOIType::AncientTech, EAstrawildZone::HollowApproach, FVector2D(-2000.0f, 1000.0f), 1300.0f,
        TEXT("Loot_POIAncient"), 5);

    MakePOI(Registry, TEXT("POI_TidebreakerWreck"), TEXT("Tidebreaker Wreck"),
        TEXT("The hull name is gone; the cargo holds are not."),
        EAstrawildPOIType::Ruin, EAstrawildZone::TidebreakerIsles, FVector2D(1800.0f, -2200.0f), 1300.0f,
        TEXT("Loot_POIRuin"), 4);

    MakePOI(Registry, TEXT("POI_SunscarMirageStone"), TEXT("Mirage Stone"),
        TEXT("Walk to it and it is never quite where it was."),
        EAstrawildPOIType::Landmark, EAstrawildZone::SunscarDesert, FVector2D(-2500.0f, 2000.0f), 1500.0f,
        NAME_None, 3);

    MakePOI(Registry, TEXT("POI_StormcrestArray"), TEXT("Stormcrest Antenna Array"),
        TEXT("Lightning chose it; the surveyors did not."),
        EAstrawildPOIType::AncientTech, EAstrawildZone::StormcrestHighlands, FVector2D(2200.0f, 1400.0f), 1400.0f,
        TEXT("Loot_POIAncient"), 5);

    MakePOI(Registry, TEXT("POI_VerdantHeartTree"), TEXT("The Heart Tree"),
        TEXT("Every seed in the Reach rolled downhill from here."),
        EAstrawildPOIType::Landmark, EAstrawildZone::VerdantReach, FVector2D(0.0f, 2600.0f), 1600.0f,
        NAME_None, 3);

    MakePOI(Registry, TEXT("POI_PearlseaResonanceWell"), TEXT("Resonance Well"),
        TEXT("The reef grows in rings around whatever is at the bottom."),
        EAstrawildPOIType::SignalSource, EAstrawildZone::PearlseaReef, FVector2D(-1800.0f, -1800.0f), 1000.0f,
        TEXT("Loot_POIAncient"), 6, true);

    // Final Run (FR-10): the Azure Shallows finally gets its POI — the sextant
    // platform reads the tide races and explains why the isles' charts end mid-water.
    MakePOI(Registry, TEXT("POI_ShallowsSextant"), TEXT("The Shallows Sextant"),
        TEXT("A drowned surveyor's ring — it still tracks something deep under the race."),
        EAstrawildPOIType::Ruin, EAstrawildZone::AzureShallows, FVector2D(2400.0f, -1600.0f), 1200.0f,
        TEXT("Loot_VendorDriftwood"), 5);
}

// ---------------------------------------------------------------------------
// Biomes — Visual Vertical Slice asset contracts (Master Plan §31)
// ---------------------------------------------------------------------------

void UAstrawildProductionContent::BuildBiomes(UAstrawildItemRegistrySubsystem* Registry)
{
    struct FBiomeRow
    {
        FName Id;
        const TCHAR* Name;
        const TCHAR* Art;
        EAstrawildZone Zone;
        bool bStart;
        TArray<FName> Nodes;
        TArray<FName> Species;
        TArray<FName> PoiIds;
        // Production V2 Batch 2: explicit dressing tints (placeholder scatter reads
        // in each biome's art direction without any editor asset).
        FLinearColor CanopyTint;
        FLinearColor RockTint;
        FLinearColor GrassTint;
    };

    const TArray<FBiomeRow> Rows =
    {
        { TEXT("Zone_DawnFields"), TEXT("Dawn Fields"), TEXT("Rolling meadow, warm dawn light, starter safety (VVS P1 target)"),
          EAstrawildZone::DawnFields, true,
          { TEXT("Node_Dawnwood"), TEXT("Node_Fieldstone"), TEXT("Node_Sunfiber") },
          { TEXT("Echo_Lumewisp"), TEXT("Echo_Stonehide"), TEXT("Echo_Duskmoth"), TEXT("Echo_Sprigling") },
          { TEXT("POI_FirstLightRuin"), TEXT("POI_DawnsteadWatchtower") },
          FLinearColor(0.36f, 0.62f, 0.24f, 1.0f), FLinearColor(0.52f, 0.48f, 0.42f, 1.0f), FLinearColor(0.55f, 0.80f, 0.34f, 1.0f) },
        { TEXT("Zone_DuskMarsh"), TEXT("Dusk Marsh"), TEXT("Low fog, black water, bioluminescent reeds"),
          EAstrawildZone::DuskMarsh, false,
          { TEXT("Node_Sunfiber"), TEXT("Node_Dawnwood") },
          { TEXT("Echo_Duskmoth"), TEXT("Echo_Sprigling") },
          { TEXT("POI_SunkencollarCave") },
          FLinearColor(0.30f, 0.42f, 0.32f, 1.0f), FLinearColor(0.36f, 0.38f, 0.36f, 1.0f), FLinearColor(0.42f, 0.55f, 0.32f, 1.0f) },
        { TEXT("Zone_EmberRidge"), TEXT("Ember Ridge"), TEXT("Volcanic ridges, heat shimmer, obsidian spires"),
          EAstrawildZone::EmberRidge, false,
          { TEXT("Node_Fieldstone"), TEXT("Node_EmberAsh"), TEXT("Node_DawnCrystal") },
          { TEXT("Echo_Emberfang"), TEXT("Echo_Stonehide") },
          { TEXT("POI_EmberFoundry") },
          FLinearColor(0.85f, 0.32f, 0.18f, 1.0f), FLinearColor(0.22f, 0.18f, 0.18f, 1.0f), FLinearColor(0.45f, 0.35f, 0.22f, 1.0f) },
        { TEXT("Zone_FrostveilExpanse"), TEXT("Frostveil Expanse"), TEXT("Snowfields, blue shadows, aurora nights"),
          EAstrawildZone::FrostveilExpanse, false,
          { TEXT("Node_Fieldstone"), TEXT("Node_DawnCrystal") },
          { TEXT("Echo_Rimefang"), TEXT("Echo_Stonehide") },
          { TEXT("POI_FrostveilSignalSource") },
          FLinearColor(0.40f, 0.58f, 0.52f, 1.0f), FLinearColor(0.55f, 0.60f, 0.68f, 1.0f), FLinearColor(0.60f, 0.68f, 0.55f, 1.0f) },
        { TEXT("Zone_Glimmerwood"), TEXT("Glimmerwood"), TEXT("Violet canopy, drifting spores, crystal undergrowth"),
          EAstrawildZone::Glimmerwood, false,
          { TEXT("Node_Dawnwood"), TEXT("Node_DawnCrystal") },
          { TEXT("Echo_Voltmaw"), TEXT("Echo_Sprigling"), TEXT("Echo_Auroraling") },
          { TEXT("POI_GlimmerwoodMonolith") },
          FLinearColor(0.52f, 0.36f, 0.72f, 1.0f), FLinearColor(0.48f, 0.46f, 0.58f, 1.0f), FLinearColor(0.42f, 0.62f, 0.58f, 1.0f) },
        { TEXT("Zone_HollowApproach"), TEXT("Hollow Approach"), TEXT("Ash plain around a wound in the world"),
          EAstrawildZone::HollowApproach, false,
          { TEXT("Node_Fieldstone"), TEXT("Node_DawnCrystal"), TEXT("Node_AncientVein") },
          { TEXT("Echo_Gloomfang") },
          { TEXT("POI_HollowApproachSpire") },
          FLinearColor(0.38f, 0.32f, 0.30f, 1.0f), FLinearColor(0.28f, 0.25f, 0.26f, 1.0f), FLinearColor(0.42f, 0.38f, 0.28f, 1.0f) },
        { TEXT("Zone_AzureShallows"), TEXT("Azure Shallows"), TEXT("Knee-deep turquoise water, white sand, skiff country"),
          EAstrawildZone::AzureShallows, false,
          { TEXT("Node_SeaPearl"), TEXT("Node_Fieldstone") },
          { TEXT("Echo_Brinefin"), TEXT("Echo_Saltcrest") },
          {},
          FLinearColor(0.38f, 0.68f, 0.38f, 1.0f), FLinearColor(0.72f, 0.68f, 0.58f, 1.0f), FLinearColor(0.62f, 0.72f, 0.42f, 1.0f) },
        { TEXT("Zone_TidebreakerIsles"), TEXT("Tidebreaker Isles"), TEXT("Wave-carved stacks, driftwood hamlets, storm light"),
          EAstrawildZone::TidebreakerIsles, false,
          { TEXT("Node_SeaPearl"), TEXT("Node_Dawnwood") },
          { TEXT("Echo_Wavecrest"), TEXT("Echo_Mistwing") },
          { TEXT("POI_TidebreakerWreck") },
          FLinearColor(0.32f, 0.60f, 0.42f, 1.0f), FLinearColor(0.35f, 0.34f, 0.38f, 1.0f), FLinearColor(0.55f, 0.70f, 0.45f, 1.0f) },
        { TEXT("Zone_SunscarDesert"), TEXT("Sunscar Desert"), TEXT("Dune glass, white heat, buried machine bones"),
          EAstrawildZone::SunscarDesert, false,
          { TEXT("Node_DuneGlass"), TEXT("Node_Fieldstone") },
          { TEXT("Echo_Sunhide"), TEXT("Echo_Glimmerhornet") },
          { TEXT("POI_SunscarMirageStone") },
          FLinearColor(0.42f, 0.58f, 0.30f, 1.0f), FLinearColor(0.78f, 0.62f, 0.38f, 1.0f), FLinearColor(0.75f, 0.68f, 0.35f, 1.0f) },
        { TEXT("Zone_StormcrestHighlands"), TEXT("Stormcrest Highlands"), TEXT("Windswept tors, perpetual storm crown"),
          EAstrawildZone::StormcrestHighlands, false,
          { TEXT("Node_StormSilver"), TEXT("Node_Fieldstone") },
          { TEXT("Echo_Sunhorn"), TEXT("Echo_Geargolem") },
          { TEXT("POI_StormcrestArray") },
          FLinearColor(0.30f, 0.44f, 0.34f, 1.0f), FLinearColor(0.50f, 0.50f, 0.52f, 1.0f), FLinearColor(0.44f, 0.52f, 0.38f, 1.0f) },
        { TEXT("Zone_VerdantReach"), TEXT("Verdant Reach"), TEXT("Deep jungle green, canopy tunnels, hidden water"),
          EAstrawildZone::VerdantReach, false,
          { TEXT("Node_Sunfiber"), TEXT("Node_Dawnwood") },
          { TEXT("Echo_Verdantbloom"), TEXT("Echo_Fernthorn") },
          { TEXT("POI_VerdantHeartTree") },
          FLinearColor(0.22f, 0.55f, 0.20f, 1.0f), FLinearColor(0.42f, 0.44f, 0.36f, 1.0f), FLinearColor(0.35f, 0.72f, 0.30f, 1.0f) },
        { TEXT("Zone_PearlseaReef"), TEXT("Pearlsea Reef"), TEXT("Coral shelves in clear water, dangerous beauty"),
          EAstrawildZone::PearlseaReef, false,
          { TEXT("Node_CoralShard"), TEXT("Node_SeaPearl") },
          { TEXT("Echo_Coralray"), TEXT("Echo_Pearlcrest") },
          { TEXT("POI_PearlseaResonanceWell") },
          FLinearColor(0.40f, 0.66f, 0.46f, 1.0f), FLinearColor(0.80f, 0.72f, 0.66f, 1.0f), FLinearColor(0.50f, 0.75f, 0.55f, 1.0f) },
    };

    for (const FBiomeRow& Row : Rows)
    {
        UAstrawildBiomeDefinition* Biome = NewObject<UAstrawildBiomeDefinition>(Registry);
        Biome->BiomeId = Row.Id;
        Biome->DisplayName = FText::FromString(Row.Name);
        Biome->ArtDirection = FText::FromString(Row.Art);
        Biome->Zone = Row.Zone;
        Biome->bStartingBiome = Row.bStart;
        Biome->ResourceNodeIds = Row.Nodes;
        Biome->SignatureSpeciesIds = Row.Species;
        Biome->PoiIds = Row.PoiIds;
        Biome->TreeCanopyTint = Row.CanopyTint;
        Biome->RockTint = Row.RockTint;
        Biome->GrassTuftTint = Row.GrassTint;

        // Art pack bindings (Batch 4, CP-04): scatter mesh sets + landscape master
        // + ambience per biome. BiomeDressingActor's ISM path consumes these soft
        // refs and disables the PMC placeholders once they resolve.
        if (const AstrawildArtPack::FBiomeArt* Art = AstrawildArtPack::FindBiomeArt(Row.Id))
        {
            for (const FString& TreePath : Art->TreeMeshPaths)
            {
                Biome->TreeMeshes.Add(TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(TreePath)));
            }
            for (const FString& RockPath : Art->RockMeshPaths)
            {
                Biome->RockMeshes.Add(TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(RockPath)));
            }
            for (const FString& GrassPath : Art->GrassMeshPaths)
            {
                Biome->GrassMeshes.Add(TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(GrassPath)));
            }
            if (!Art->LandscapeMaterialPath.IsEmpty())
            {
                Biome->LandscapeMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(Art->LandscapeMaterialPath));
            }
            if (!Art->AmbientAudioPath.IsEmpty())
            {
                Biome->AmbientAudio = TSoftObjectPtr<USoundBase>(FSoftObjectPath(Art->AmbientAudioPath));
            }
        }
        Registry->RegisterBiome(Biome);
    }
}

// ---------------------------------------------------------------------------
// Production Echo roster — 6 role-differentiated species (Master Plan §6 STEP 5)
// ---------------------------------------------------------------------------

void UAstrawildProductionContent::BuildProductionEchoes(UAstrawildItemRegistrySubsystem* Registry)
{
    // Terraquill — the gathering specialist (fibers, berries, wood).
    MakeProductionEcho(Registry, TEXT("Echo_Terraquill"), TEXT("Terraquill"),
        EAstrawildElementType::Flora, EAstrawildEchoRole::Support, 120.0f, 14.0f, 18.0f, 420.0f,
        EAstrawildPersonality::Loyal, EAstrawildActivityPattern::Diurnal,
        { TEXT("Item_Berry"), TEXT("Item_Dawnbloom") }, 0.35f, EAstrawildElementType::Ember,
        EAstrawildEchoFamily::Beast, EAstrawildBodyPlan::Quadruped, EAstrawildSizeClass::Small,
        EAstrawildZone::DawnFields, EAstrawildRarity::Uncommon, EAstrawildEchoPassive::CarryBoost,
        { { EAstrawildWorkType::Gathering, 1.9f }, { EAstrawildWorkType::Farming, 1.2f } },
        { Stack(TEXT("Item_Fiber"), 2) });

    // Cindermule — the transport specialist (pack aura).
    MakeProductionEcho(Registry, TEXT("Echo_Cindermule"), TEXT("Cindermule"),
        EAstrawildElementType::Ember, EAstrawildEchoRole::Base, 180.0f, 18.0f, 26.0f, 380.0f,
        EAstrawildPersonality::Energetic, EAstrawildActivityPattern::Diurnal,
        { TEXT("Item_EmberAsh") }, 0.45f, EAstrawildElementType::Frost,
        EAstrawildEchoFamily::Beast, EAstrawildBodyPlan::Quadruped, EAstrawildSizeClass::Medium,
        EAstrawildZone::EmberRidge, EAstrawildRarity::Uncommon, EAstrawildEchoPassive::CarryBoost,
        { { EAstrawildWorkType::Transport, 1.9f }, { EAstrawildWorkType::Gathering, 1.0f } },
        { Stack(TEXT("Item_EmberAsh"), 2) });

    // Voltpylon — the power-plant Echo (generation affinity + stamina aura).
    MakeProductionEcho(Registry, TEXT("Echo_Voltpylon"), TEXT("Voltpylon"),
        EAstrawildElementType::Pulse, EAstrawildEchoRole::Base, 140.0f, 20.0f, 20.0f, 340.0f,
        EAstrawildPersonality::Energetic, EAstrawildActivityPattern::Nocturnal,
        { TEXT("Item_VoltCore") }, 0.5f, EAstrawildElementType::Flora,
        EAstrawildEchoFamily::Construct, EAstrawildBodyPlan::Biped, EAstrawildSizeClass::Medium,
        EAstrawildZone::Glimmerwood, EAstrawildRarity::Rare, EAstrawildEchoPassive::PlayerStamina,
        { { EAstrawildWorkType::PowerGeneration, 1.8f }, { EAstrawildWorkType::Crafting, 1.1f } },
        { Stack(TEXT("Item_VoltCore"), 1) });

    // Bastionbeetle — the base defense wall.
    MakeProductionEcho(Registry, TEXT("Echo_Bastionbeetle"), TEXT("Bastionbeetle"),
        EAstrawildElementType::Ash, EAstrawildEchoRole::Combat, 260.0f, 22.0f, 44.0f, 300.0f,
        EAstrawildPersonality::Protective, EAstrawildActivityPattern::Diurnal,
        { TEXT("Item_ChitinPlate") }, 0.55f, EAstrawildElementType::Pulse,
        EAstrawildEchoFamily::Insectoid, EAstrawildBodyPlan::Insectoid, EAstrawildSizeClass::Large,
        EAstrawildZone::VerdantReach, EAstrawildRarity::Rare, EAstrawildEchoPassive::ThreatDampener,
        { { EAstrawildWorkType::Defense, 1.9f }, { EAstrawildWorkType::Mining, 1.0f } },
        { Stack(TEXT("Item_ChitinPlate"), 2) });

    // Mistmender — the healing aura companion.
    MakeProductionEcho(Registry, TEXT("Echo_Mistmender"), TEXT("Mistmender"),
        EAstrawildElementType::Light, EAstrawildEchoRole::Support, 130.0f, 12.0f, 16.0f, 400.0f,
        EAstrawildPersonality::Social, EAstrawildActivityPattern::Crepuscular,
        { TEXT("Item_Dawnbloom"), TEXT("Item_HerbalSalve") }, 0.4f, EAstrawildElementType::Ash,
        EAstrawildEchoFamily::Spirit, EAstrawildBodyPlan::Floating, EAstrawildSizeClass::Small,
        EAstrawildZone::DuskMarsh, EAstrawildRarity::Rare, EAstrawildEchoPassive::PartyHeal,
        { { EAstrawildWorkType::Farming, 1.3f }, { EAstrawildWorkType::ResearchAssist, 1.5f } },
        { Stack(TEXT("Item_Dawnbloom"), 1) });

    // Deepdelver — the mining specialist.
    MakeProductionEcho(Registry, TEXT("Echo_Deepdelver"), TEXT("Deepdelver"),
        EAstrawildElementType::Ash, EAstrawildEchoRole::Base, 190.0f, 26.0f, 30.0f, 320.0f,
        EAstrawildPersonality::Lazy, EAstrawildActivityPattern::Nocturnal,
        { TEXT("Item_Stone") }, 0.5f, EAstrawildElementType::Light,
        EAstrawildEchoFamily::Elemental, EAstrawildBodyPlan::Amorphous, EAstrawildSizeClass::Medium,
        EAstrawildZone::StormcrestHighlands, EAstrawildRarity::Rare, EAstrawildEchoPassive::None,
        { { EAstrawildWorkType::Mining, 1.9f }, { EAstrawildWorkType::Construction, 1.3f } },
        { Stack(TEXT("Item_Stone"), 3), Stack(TEXT("Item_StormSilver"), 1) });

    // Art pack bindings (Batch 4, CP-02/CP-08): skeletal meshes + idle/move clips
    // per species. EchoCharacter swaps its PMC silhouette for the skinned body
    // the moment these resolve; unresolved species keep the procedural body.
    for (const AstrawildArtPack::FEchoArt& Art : AstrawildArtPack::GetEchoArt())
    {
        if (UAstrawildEchoDefinition* EchoDef = Registry->FindEcho(Art.EchoId))
        {
            EchoDef->SkeletalMesh = TSoftObjectPtr<USkeletalMesh>(FSoftObjectPath(Art.MeshPath));
            if (!Art.IdleAnimPath.IsEmpty())
            {
                EchoDef->IdleAnimation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Art.IdleAnimPath));
            }
            if (!Art.MoveAnimPath.IsEmpty())
            {
                EchoDef->MoveAnimation = TSoftObjectPtr<UAnimSequenceBase>(FSoftObjectPath(Art.MoveAnimPath));
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Content Pack CP-02 — evolution targets (the progression side of the roster).
// Each production Echo earns an evolved form: rarity + size class + stat bumps,
// same element/family/body-plan (identity reads), deeper work affinities.
// Gate discipline: level AND bond — a raised companion, not a ground one.
// ---------------------------------------------------------------------------

void UAstrawildProductionContent::BuildEvolutionTargets(UAstrawildItemRegistrySubsystem* Registry)
{
    struct FEvolutionSpec
    {
        FName BaseId;
        FName TargetId;
        FString TargetName;
        FName EvolveFromName; // For the description only.
        int32 LevelGate;
        float BondGate;
    };
    const FEvolutionSpec Specs[] = {
        { TEXT("Echo_Terraquill"),    TEXT("Echo_TerraquillVerdant"),   TEXT("Terraquill Verdant"),   TEXT("Terraquill"),    20, 35.0f },
        { TEXT("Echo_Cindermule"),    TEXT("Echo_CindermulePyre"),      TEXT("Cindermule Pyre"),      TEXT("Cindermule"),    22, 40.0f },
        { TEXT("Echo_Voltpylon"),     TEXT("Echo_VoltpylonTempest"),    TEXT("Voltpylon Tempest"),    TEXT("Voltpylon"),     25, 45.0f },
        { TEXT("Echo_Bastionbeetle"), TEXT("Echo_BastionbeetleBulwark"),TEXT("Bastionbeetle Bulwark"),TEXT("Bastionbeetle"), 28, 50.0f },
        { TEXT("Echo_Mistmender"),    TEXT("Echo_MistmenderRime"),      TEXT("Mistmender Rime"),      TEXT("Mistmender"),    24, 45.0f },
        { TEXT("Echo_Deepdelver"),    TEXT("Echo_DeepdelverAbyssal"),   TEXT("Deepdelver Abyssal"),   TEXT("Deepdelver"),    26, 40.0f },
    };

    for (const FEvolutionSpec& Spec : Specs)
    {
        UAstrawildEchoDefinition* Base = Registry->FindEcho(Spec.BaseId);
        if (!Base)
        {
            UE_LOG(LogAstrawildEconomy, Warning, TEXT("Evolution chain: base species %s not found."), *Spec.BaseId.ToString());
            continue;
        }

        // Evolved stat block: +35..40% health, +~30% attack/defense, slight speed trim.
        UAstrawildEchoDefinition* Evolved = NewObject<UAstrawildEchoDefinition>(Registry);
        Evolved->DefinitionId = Spec.TargetId;
        Evolved->DisplayName = FText::FromString(Spec.TargetName);
        Evolved->Description = FText::FromString(FString::Printf(
            TEXT("Evolved form of %s — earned through level and bond. The same companion, transformed."),
            *Spec.EvolveFromName.ToString()));
        Evolved->Element = Base->Element;
        Evolved->Role = Base->Role;
        Evolved->BaseStats.MaxHealth = FMath::RoundToFloat(Base->BaseStats.MaxHealth * 1.38f);
        Evolved->BaseStats.AttackPower = FMath::RoundToFloat(Base->BaseStats.AttackPower * 1.30f);
        Evolved->BaseStats.Defense = FMath::RoundToFloat(Base->BaseStats.Defense * 1.32f);
        Evolved->BaseStats.MoveSpeed = FMath::RoundToFloat(Base->BaseStats.MoveSpeed * 1.03f);
        Evolved->BaseStats.CaptureResilience = Base->BaseStats.CaptureResilience;
        Evolved->DominantPersonality = Base->DominantPersonality;
        Evolved->ActivityPattern = Base->ActivityPattern;
        Evolved->PreferredFoodIds = Base->PreferredFoodIds;
        Evolved->CaptureDifficulty = FMath::Clamp(Base->CaptureDifficulty + 0.2f, 0.1f, 0.95f);
        Evolved->WeaknessElement = Base->WeaknessElement;
        Evolved->Family = Base->Family;
        Evolved->BodyPlan = Base->BodyPlan;
        // Size grows one class (Small→Medium, Medium→Large; Large stays — the cap).
        Evolved->SizeClass = Base->SizeClass == EAstrawildSizeClass::Small
            ? EAstrawildSizeClass::Medium
            : EAstrawildSizeClass::Large;
        Evolved->HomeZone = Base->HomeZone;
        Evolved->Rarity = Base->Rarity == EAstrawildRarity::Uncommon
            ? EAstrawildRarity::Rare
            : EAstrawildRarity::Epic;
        Evolved->Passive = Base->Passive;
        // Work affinities deepen (+0.1 across the board, capped by the 0..2 contract).
        Evolved->WorkAffinities = Base->WorkAffinities;
        for (FAstrawildWorkAffinity& Affinity : Evolved->WorkAffinities)
        {
            Affinity.Affinity = FMath::Clamp(Affinity.Affinity + 0.1f, 0.0f, 2.0f);
        }
        Evolved->DefeatLoot = Base->DefeatLoot;
        Evolved->bHostileToPlayers = false;
        // Deeper, more saturated tints — the evolved form reads at a glance.
        Evolved->PrimaryTint = FLinearColor(
            FMath::Clamp(Base->PrimaryTint.R * 1.15f, 0.0f, 1.0f),
            FMath::Clamp(Base->PrimaryTint.G * 1.15f, 0.0f, 1.0f),
            FMath::Clamp(Base->PrimaryTint.B * 1.15f, 0.0f, 1.0f), 1.0f);
        Evolved->SecondaryTint = FLinearColor(
            FMath::Clamp(Base->SecondaryTint.R * 0.8f, 0.0f, 1.0f),
            FMath::Clamp(Base->SecondaryTint.G * 0.8f, 0.0f, 1.0f),
            FMath::Clamp(Base->SecondaryTint.B * 0.9f, 0.0f, 1.0f), 1.0f);
        Registry->RegisterEcho(Evolved);

        // Chain link + gates on the base species.
        Base->EvolveToDefinitionId = Spec.TargetId;
        Base->EvolveRequiredLevel = Spec.LevelGate;
        Base->EvolveRequiredBond = Spec.BondGate;
    }
}

// ---------------------------------------------------------------------------
// Production technologies — 6 new branch-tagged nodes (Master Plan §16)
// ---------------------------------------------------------------------------

void UAstrawildProductionContent::BuildProductionTechnologies(UAstrawildItemRegistrySubsystem* Registry)
{
    MakeTech(Registry, TEXT("Tech_WeaponSystems"), TEXT("Weapon Systems"), EAstrawildTechEra::Electrical,
        EAstrawildResearchBranch::Weapons, 20,
        { TEXT("Tech_Armory"), TEXT("Tech_Electrical") },
        { TEXT("Recipe_PlasmaCharger"), TEXT("Recipe_LumenBeam"), TEXT("Recipe_ArcCaster") });

    MakeTech(Registry, TEXT("Tech_AdvancedBallistics"), TEXT("Advanced Ballistics"), EAstrawildTechEra::AdvancedEnergy,
        EAstrawildResearchBranch::Weapons, 30,
        { TEXT("Tech_WeaponSystems") },
        { TEXT("Recipe_MagrailDriver"), TEXT("Recipe_SkysingerLauncher"), TEXT("Recipe_RailSlugBatch"), TEXT("Recipe_SeekerMissileBatch") });

    MakeTech(Registry, TEXT("Tech_ExperimentalArsenal"), TEXT("Experimental Arsenal"), EAstrawildTechEra::Ancient,
        EAstrawildResearchBranch::Weapons, 40,
        { TEXT("Tech_AdvancedBallistics"), TEXT("Tech_AncientResonance") },
        { TEXT("Recipe_StarlancePrototype"), TEXT("Recipe_NovaCell") });

    MakeTech(Registry, TEXT("Tech_ExosuitEngineering"), TEXT("Exosuit Engineering"), EAstrawildTechEra::AdvancedEnergy,
        EAstrawildResearchBranch::Armor, 25,
        { TEXT("Tech_AdvancedEnergy") },
        { TEXT("Recipe_VanguardHelm"), TEXT("Recipe_VanguardVest"), TEXT("Recipe_BastionHelm"), TEXT("Recipe_BastionPlate"), TEXT("Recipe_AstralforgedExosuit") });

    MakeTech(Registry, TEXT("Tech_ScannerArray"), TEXT("Scanner Array"), EAstrawildTechEra::AdvancedEnergy,
        EAstrawildResearchBranch::Scanner, 18,
        { TEXT("Tech_AdvancedEnergy") },
        { TEXT("Recipe_ArrayScanner"), TEXT("Recipe_OracleScanner") });

    MakeTech(Registry, TEXT("Tech_AutomationII"), TEXT("Automation II"), EAstrawildTechEra::AdvancedEnergy,
        EAstrawildResearchBranch::Automation, 22,
        { TEXT("Tech_Mechanics"), TEXT("Tech_AdvancedEnergy") },
        { TEXT("Recipe_DroneCellExtender"), TEXT("Recipe_DroneFocusedArray"), TEXT("Recipe_DroneSalvageClaw"),
          TEXT("Recipe_RobotBorebot"), TEXT("Recipe_RobotCultivator"), TEXT("Recipe_RobotSentinel") });
}

// ---------------------------------------------------------------------------
// Production quests — 2 chain extenders hooking the new systems (Master Plan §17)
// ---------------------------------------------------------------------------

void UAstrawildProductionContent::BuildProductionQuests(UAstrawildItemRegistrySubsystem* Registry)
{
    UAstrawildQuestDefinition* Signals = NewObject<UAstrawildQuestDefinition>(Registry);
    Signals->QuestId = TEXT("Quest_SignalsInTheStatic");
    Signals->Title = FText::FromString(TEXT("Signals in the Static"));
    Signals->Summary = FText::FromString(TEXT("The Field Scanner picks up shapes in the old places. Go read them."));
    {
        FAstrawildQuestObjective Obj;
        Obj.Type = EAstrawildQuestObjectiveType::CraftRecipe;
        Obj.TargetId = TEXT("Recipe_ArrayScanner");
        Obj.RequiredCount = 1;
        Obj.ObjectiveText = FText::FromString(TEXT("Craft the Array Scanner"));
        Signals->Objectives.Add(Obj);
    }
    {
        FAstrawildQuestObjective Obj;
        Obj.Type = EAstrawildQuestObjectiveType::DiscoverPOI;
        Obj.TargetId = TEXT("POI_FirstLightRuin");
        Obj.RequiredCount = 1;
        Obj.ObjectiveText = FText::FromString(TEXT("Discover the First Light Ruin"));
        Signals->Objectives.Add(Obj);
    }
    {
        FAstrawildQuestObjective Obj;
        Obj.Type = EAstrawildQuestObjectiveType::DiscoverPOI;
        Obj.TargetId = TEXT("POI_GlimmerwoodMonolith");
        Obj.RequiredCount = 1;
        Obj.ObjectiveText = FText::FromString(TEXT("Find the Glimmerwood Monolith"));
        Signals->Objectives.Add(Obj);
    }
    Signals->RewardItems = { Stack(TEXT("Item_EnergyCell"), 4), Stack(TEXT("Item_DawnShard"), 3) };
    Signals->RewardResearchPoints = 20;
    Signals->NextQuestId = TEXT("Quest_VanguardProtocol");
    Registry->RegisterQuest(Signals);

    UAstrawildQuestDefinition* Vanguard = NewObject<UAstrawildQuestDefinition>(Registry);
    Vanguard->QuestId = TEXT("Quest_VanguardProtocol");
    Vanguard->Title = FText::FromString(TEXT("The Vanguard Protocol"));
    Vanguard->Summary = FText::FromString(TEXT("Plate up, field the machines, and hold the line through a night raid."));
    {
        FAstrawildQuestObjective Obj;
        Obj.Type = EAstrawildQuestObjectiveType::CraftRecipe;
        Obj.TargetId = TEXT("Recipe_VanguardVest");
        Obj.RequiredCount = 1;
        Obj.ObjectiveText = FText::FromString(TEXT("Craft the Vanguard Vest (Mk II)"));
        Vanguard->Objectives.Add(Obj);
    }
    {
        FAstrawildQuestObjective Obj;
        Obj.Type = EAstrawildQuestObjectiveType::CraftRecipe;
        Obj.TargetId = TEXT("Recipe_RobotSentinel");
        Obj.RequiredCount = 1;
        Obj.ObjectiveText = FText::FromString(TEXT("Build a Sentinel Frame"));
        Vanguard->Objectives.Add(Obj);
    }
    {
        FAstrawildQuestObjective Obj;
        Obj.Type = EAstrawildQuestObjectiveType::SurviveTime;
        Obj.TargetId = NAME_None;
        Obj.RequiredCount = 300; // 5 in-world minutes of holding on.
        Obj.ObjectiveText = FText::FromString(TEXT("Survive 300 seconds in the field"));
        Vanguard->Objectives.Add(Obj);
    }
    Vanguard->RewardItems = { Stack(TEXT("Item_AncientAlloy"), 1), Stack(TEXT("Item_StormSilver"), 3) };
    Vanguard->RewardResearchPoints = 25;
    // Final Run (FR-5): Act 3 opens here — "The Storm Crown Stirs" takes over
    // the moment the Vanguard Protocol closes (validator: chain bridge).
    Vanguard->NextQuestId = TEXT("Quest_StormAnchors");
    Registry->RegisterQuest(Vanguard);
}

// ---------------------------------------------------------------------------
// Production V2 Batch 3 — dialogue trees (P12 Story/NPC, Master Plan §17).
// NPCs stop being quest-toast dispensers: quest offers migrate into choice
// consequences, vendor hand-off routes through bOpenShop, and one-time beats
// (tips, gifts) use story flags so they never repeat after a save/reload.
// ---------------------------------------------------------------------------

namespace
{
    FAstrawildDialogueLine Line(const TCHAR* Speaker, const TCHAR* Text)
    {
        FAstrawildDialogueLine Out;
        if (Speaker)
        {
            Out.SpeakerName = FText::FromString(Speaker);
        }
        Out.Text = FText::FromString(Text);
        return Out;
    }
}

void UAstrawildProductionContent::BuildDialogueTrees(UAstrawildItemRegistrySubsystem* Registry)
{
    // --- Warden Maren (Dawnstead) — First Light offer + Vale lore + report-back beat ---
    UAstrawildDialogueTreeDefinition* Maren = NewObject<UAstrawildDialogueTreeDefinition>(Registry);
    Maren->DialogueId = TEXT("Dialogue_WardenMaren");
    Maren->EntryNodeId = TEXT("hello");
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("hello");
        Node.Lines = { Line(nullptr, TEXT("The fields are calm — for now. Storms roll in off the Highlands and the wild Echoes get bold after dark.")) };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Ask about the Vale"));
            Choice.GotoNodeId = TEXT("vale");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Accept: First Light"));
            Choice.RequiredQuestActiveId = NAME_None; // Visible until taken.
            Choice.ForbiddenFlagId = TEXT("Maren_FirstLightAccepted");
            Choice.StartQuestId = TEXT("Quest_FirstLight");
            Choice.SetFlagId = TEXT("Maren_FirstLightAccepted");
            Choice.GotoNodeId = TEXT("accepted");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Report: First Light"));
            Choice.RequiredQuestCompletedId = TEXT("Quest_FirstLight");
            Choice.ForbiddenFlagId = TEXT("Maren_FirstLightReported");
            Choice.SetFlagId = TEXT("Maren_FirstLightReported");
            Choice.GiveResearchPoints = 15;
            Choice.GotoNodeId = TEXT("reported");
            Node.Choices.Add(Choice);
        }
        {
            // Final Run (FR-6): the crown conversation unlocks once the Drowned
            // Sovereign falls — the last story beat routes to the ending choice.
            // Final-audit G-2: canon (MASTER_CONTROL §3) gates Maren's final
            // dialogue on MQ-17 (homecoming) — NOT the Sovereign kill, or the
            // ending fires one quest early and strands MQ-17 active.
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("The crown — what do we do?"));
            Choice.RequiredQuestCompletedId = TEXT("Quest_FirstDawnAgain");
            Choice.ForbiddenFlagId = TEXT("Maren_EndingResolved");
            Choice.GotoNodeId = TEXT("crown");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Maren->Nodes.Add(Node);
    }
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("vale");
        Node.Lines = {
            Line(nullptr, TEXT("Dawn Fields were the first ground we ever fenced. Fertile, gentle — and everything hungry knows it.")),
            Line(nullptr, TEXT("East past the reef the isles start. Old Salt Perry will talk your ear off about what swims between them."))
        };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Back"));
            Choice.GotoNodeId = TEXT("hello");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Maren->Nodes.Add(Node);
    }
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("accepted");
        Node.Lines = { Line(nullptr, TEXT("Good. Mark three Echo signatures with the Field Scanner, then return. The dawn fields keep you.")) };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Maren->Nodes.Add(Node);
    }
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("reported");
        Node.Lines = {
            Line(nullptr, TEXT("Three clean marks. You've the patience for this work.")),
            Line(nullptr, TEXT("Take these field notes — the research bench will make better use of them than my shelf."))
        };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Maren->Nodes.Add(Node);
    }
    {
        // Final Run (FR-6) — Maren's final choice: the two endings. The crown
        // cage stands open; the Vale waits on one word. Both choices are one-way
        // (TriggerEndingId routes through the game state; the flag hides the
        // beat afterward; post-game free roam unlocks either way).
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("crown");
        Node.Lines = {
            Line(nullptr, TEXT("The cage is open. The core still hums — one pulse from the storm crown, one heartbeat of everything that drowned.")),
            Line(nullptr, TEXT("Break it, and the storms never return — but the Vale changes forever. Let it sleep, and we live beside the crown — guarded, watchful, free."))
        };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Break the cage — end the storms forever"));
            // Final-audit G-2: gated on MQ-17 (homecoming) per canon — see the
            // reveal choice comment above.
            Choice.RequiredQuestCompletedId = TEXT("Quest_FirstDawnAgain");
            Choice.ForbiddenFlagId = TEXT("Maren_EndingResolved");
            Choice.SetFlagId = TEXT("Maren_EndingResolved");
            Choice.TriggerEndingId = TEXT("Ending_BreakCage"); // → The Dawn That Stays.
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Let it sleep — the crown keeps its vigil"));
            // Final-audit G-2: gated on MQ-17 (homecoming) per canon.
            Choice.RequiredQuestCompletedId = TEXT("Quest_FirstDawnAgain");
            Choice.ForbiddenFlagId = TEXT("Maren_EndingResolved");
            Choice.SetFlagId = TEXT("Maren_EndingResolved");
            Choice.TriggerEndingId = TEXT("Ending_StormSleeps"); // → The Storm That Sleeps.
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Maren->Nodes.Add(Node);
    }
    Registry->RegisterDialogueTree(Maren);

    // --- Trader Tam (Dawnstead vendor) — shop hand-off + one-time Gloomfang tip ---
    UAstrawildDialogueTreeDefinition* Tam = NewObject<UAstrawildDialogueTreeDefinition>(Registry);
    Tam->DialogueId = TEXT("Dialogue_TraderTam");
    Tam->EntryNodeId = TEXT("hello");
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("hello");
        Node.Lines = { Line(nullptr, TEXT("Shards, friend. Shards for everything. What catches your eye?")) };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Browse wares"));
            Choice.bOpenShop = true;
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Ask about the Gloomfangs"));
            Choice.ForbiddenFlagId = TEXT("Tam_GloomfangTip");
            Choice.SetFlagId = TEXT("Tam_GloomfangTip");
            Choice.GiveResearchPoints = 10;
            Choice.GotoNodeId = TEXT("gloomfang");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Tam->Nodes.Add(Node);
    }
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("gloomfang");
        Node.Lines = {
            Line(nullptr, TEXT("Came in a pack of six last dark. Sela's watch dropped two before the torches caught.")),
            Line(nullptr, TEXT("They hate bright light and they hesitate before a shielded man. Worth knowing — here, I'll sketch the footfall pattern for your bench."))
        };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Browse wares"));
            Choice.bOpenShop = true;
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Tam->Nodes.Add(Node);
    }
    Registry->RegisterDialogueTree(Tam);

    // --- Elder Rowan (Dawnstead) — Wings over the Vale offer + old-world lore ---
    UAstrawildDialogueTreeDefinition* Rowan = NewObject<UAstrawildDialogueTreeDefinition>(Registry);
    Rowan->DialogueId = TEXT("Dialogue_ElderRowan");
    Rowan->EntryNodeId = TEXT("hello");
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("hello");
        Node.Lines = { Line(nullptr, TEXT("Sit. The Vale has grown wider while you slept — the skiff wardens chart isles now where our maps end.")) };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Ask about the old world"));
            Choice.GotoNodeId = TEXT("oldworld");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Accept: Wings over the Vale"));
            Choice.ForbiddenFlagId = TEXT("Rowan_WingsAccepted");
            Choice.StartQuestId = TEXT("Quest_WingsOverTheVale");
            Choice.SetFlagId = TEXT("Rowan_WingsAccepted");
            Choice.GotoNodeId = TEXT("accepted");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Rowan->Nodes.Add(Node);
    }
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("oldworld");
        Node.Lines = {
            Line(nullptr, TEXT("Before the quiet, they crossed the sky in machines that sang. What's left of their roads still hums at dusk.")),
            Line(nullptr, TEXT("We do not dig where it hums. You, I think, will."))
        };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Back"));
            Choice.GotoNodeId = TEXT("hello");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Rowan->Nodes.Add(Node);
    }
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("accepted");
        Node.Lines = { Line(nullptr, TEXT("Kael at Driftwood Landing will ready a skiff. Mind the reefs — and bring back a story worth a chair by the fire.")) };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Rowan->Nodes.Add(Node);
    }
    Registry->RegisterDialogueTree(Rowan);

    // --- Skiff Warden Kael (Driftwood Landing) — Sunken Vault offer + vault warning ---
    UAstrawildDialogueTreeDefinition* Kael = NewObject<UAstrawildDialogueTreeDefinition>(Registry);
    Kael->DialogueId = TEXT("Dialogue_SkiffWardenKael");
    Kael->EntryNodeId = TEXT("hello");
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("hello");
        Node.Lines = { Line(nullptr, TEXT("Skiff's fueled. The isles are yours now — just remember the tide decides when they aren't.")) };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Ask about the Sunken Vault"));
            Choice.GotoNodeId = TEXT("vault");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Accept: The Sunken Vault"));
            Choice.ForbiddenFlagId = TEXT("Kael_VaultAccepted");
            Choice.StartQuestId = TEXT("Quest_SunkenVault");
            Choice.SetFlagId = TEXT("Kael_VaultAccepted");
            Choice.GotoNodeId = TEXT("accepted");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Kael->Nodes.Add(Node);
    }
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("vault");
        Node.Lines = {
            Line(nullptr, TEXT("There's a door under the west isle that no fish will swim past. The old folk called it the Sunken Vault.")),
            Line(nullptr, TEXT("Pearl-divers went in two generations back. One came out — richer, quieter."))
        };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Back"));
            Choice.GotoNodeId = TEXT("hello");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Kael->Nodes.Add(Node);
    }
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("accepted");
        Node.Lines = { Line(nullptr, TEXT("Then take the portal marker on the jetty. Whatever built that door built the colossus inside it — scan before you shoot.")) };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Kael->Nodes.Add(Node);
    }
    Registry->RegisterDialogueTree(Kael);

    // --- Guard Captain Sela (Dawnstead) — night-raid survival lore + one-time watch advice ---
    UAstrawildDialogueTreeDefinition* Sela = NewObject<UAstrawildDialogueTreeDefinition>(Registry);
    Sela->DialogueId = TEXT("Dialogue_GuardSela");
    Sela->EntryNodeId = TEXT("hello");
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("hello");
        Node.Lines = { Line(nullptr, TEXT("Keep the fire behind you and the dark ahead. What's your business on my watch?")) };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Ask about the night raids"));
            Choice.GotoNodeId = TEXT("raids");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Ask for watch advice"));
            Choice.ForbiddenFlagId = TEXT("Sela_AdviceGiven");
            Choice.SetFlagId = TEXT("Sela_AdviceGiven");
            Choice.GiveResearchPoints = 10;
            Choice.GotoNodeId = TEXT("advice");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Sela->Nodes.Add(Node);
    }
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("raids");
        Node.Lines = {
            Line(nullptr, TEXT("When the sky goes amber, they come — Gloomfangs bold, Ashfangs hungry, worse behind them on the storm nights.")),
            Line(nullptr, TEXT("A fence slows them. A powered fence stops them. Get your generator humming before dusk, not after."))
        };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Back"));
            Choice.GotoNodeId = TEXT("hello");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Sela->Nodes.Add(Node);
    }
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("advice");
        Node.Lines = {
            Line(nullptr, TEXT("Watch the treeline, not the dark between the trees. Eyes catch movement — lanterns catch nothing.")),
            Line(nullptr, TEXT("I've written my patrol timing into the watch book. Your research bench can lift a thing or two from it."))
        };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Sela->Nodes.Add(Node);
    }
    Registry->RegisterDialogueTree(Sela);

    // --- Old Salt Perry (Driftwood Landing) — pure village color + one-time sea pearl gift ---
    UAstrawildDialogueTreeDefinition* Perry = NewObject<UAstrawildDialogueTreeDefinition>(Registry);
    Perry->DialogueId = TEXT("Dialogue_OldSaltPerry");
    Perry->EntryNodeId = TEXT("hello");
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("hello");
        Node.Lines = {
            Line(nullptr, TEXT("The tide took the old world. It can wait for you too.")),
            Line(nullptr, TEXT("Sit a while, or don't. The sea's not going anywhere — that's the whole trick of her."))
        };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Ask about the tide"));
            Choice.GotoNodeId = TEXT("tide");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Accept the sea pearl"));
            Choice.ForbiddenFlagId = TEXT("Perry_PearlGiven");
            Choice.RequiredFlagId = TEXT("Perry_TideHeard"); // He only gifts after sharing the tide story once.
            Choice.SetFlagId = TEXT("Perry_PearlGiven");
            Choice.GiveItemId = TEXT("Item_SeaPearl");
            Choice.GiveItemQuantity = 1;
            Choice.GotoNodeId = TEXT("pearl");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Perry->Nodes.Add(Node);
    }
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("tide");
        Node.Lines = {
            Line(nullptr, TEXT("Twice a day she swallows the reef road whole. The isles you want are the ones she only licks.")),
            Line(nullptr, TEXT("Nima sells charts. I sell the truth the charts leave off — free, today."))
        };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Back"));
            Choice.SetFlagId = TEXT("Perry_TideHeard");
            Choice.GotoNodeId = TEXT("hello");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Perry->Nodes.Add(Node);
    }
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("pearl");
        Node.Lines = { Line(nullptr, TEXT("Dived it the summer my knees still worked. It'll light a crystal circuit brighter than any shard — or sit pretty on a shelf. Your call.")) };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Perry->Nodes.Add(Node);
    }
    Registry->RegisterDialogueTree(Perry);

    // -------------------------------------------------------------------------
    // Final Run (FR-10) — the last five villagers get their trees. Every NPC in
    // the Vale now converses: vendor hand-offs route through bOpenShop, one-time
    // beats use story flags, and conditions gate on quest state + world facts.
    // -------------------------------------------------------------------------

    // --- Herbalist Wren (Dawnstead vendor) — salve recipe tip + shop ---
    UAstrawildDialogueTreeDefinition* Wren = NewObject<UAstrawildDialogueTreeDefinition>(Registry);
    Wren->DialogueId = TEXT("Dialogue_HerbalistWren");
    Wren->EntryNodeId = TEXT("hello");
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("hello");
        Node.Lines = { Line(nullptr, TEXT("Bark, root, bloom. The marsh gives everything if you know which parts to boil.")) };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Browse remedies"));
            Choice.bOpenShop = true;
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Ask about the salve recipe"));
            Choice.ForbiddenFlagId = TEXT("Wren_SalveTip");
            Choice.SetFlagId = TEXT("Wren_SalveTip");
            Choice.GiveResearchPoints = 8;
            Choice.GotoNodeId = TEXT("salve");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Wren->Nodes.Add(Node);
    }
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("salve");
        Node.Lines = {
            Line(nullptr, TEXT("Chitin plate, marsh bloom, a whisper of dawn shard. Grind, warm, wrap — the Herbal Salve writes itself.")),
            Line(nullptr, TEXT("Here, the proportions. Your research bench will make better ink of it than my chalk."))
        };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Browse remedies"));
            Choice.bOpenShop = true;
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Wren->Nodes.Add(Node);
    }
    Registry->RegisterDialogueTree(Wren);

    // --- Blacksmith Borin (Dawnstead vendor) — one-time whetstone gift + shop ---
    UAstrawildDialogueTreeDefinition* Borin = NewObject<UAstrawildDialogueTreeDefinition>(Registry);
    Borin->DialogueId = TEXT("Dialogue_BlacksmithBorin");
    Borin->EntryNodeId = TEXT("hello");
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("hello");
        Node.Lines = { Line(nullptr, TEXT("Steel today, story tomorrow. Which do you need?")) };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Browse the forge"));
            Choice.bOpenShop = true;
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Take the whetstone offer"));
            Choice.ForbiddenFlagId = TEXT("Borin_WhetstoneGiven");
            Choice.SetFlagId = TEXT("Borin_WhetstoneGiven");
            Choice.GiveItemId = TEXT("Item_Stone");
            Choice.GiveItemQuantity = 3;
            Choice.GotoNodeId = TEXT("whetstone");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Borin->Nodes.Add(Node);
    }
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("whetstone");
        Node.Lines = {
            Line(nullptr, TEXT("First-fielders get whetstones. Keep an edge on everything — the Vale doesn't forgive dull tools.")),
            Line(nullptr, TEXT("Three stones. Don't come back crying when you've ground them to sand."))
        };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Browse the forge"));
            Choice.bOpenShop = true;
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Borin->Nodes.Add(Node);
    }
    Registry->RegisterDialogueTree(Borin);

    // --- Guard Bram (Dawnstead) — night-watch intelligence + the Tyrant rumor ---
    UAstrawildDialogueTreeDefinition* Bram = NewObject<UAstrawildDialogueTreeDefinition>(Registry);
    Bram->DialogueId = TEXT("Dialogue_GuardBram");
    Bram->EntryNodeId = TEXT("hello");
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("hello");
        Node.Lines = { Line(nullptr, TEXT("Gloomfangs again. Always Gloomfangs. Third night running they test the south fence.")) };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("How do I survive a night raid?"));
            Choice.ForbiddenFlagId = TEXT("Bram_NightTip");
            Choice.SetFlagId = TEXT("Bram_NightTip");
            Choice.GiveResearchPoints = 8;
            Choice.GotoNodeId = TEXT("nighttip");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Anything stranger than Gloomfangs?"));
            Choice.GotoNodeId = TEXT("rumor");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Bram->Nodes.Add(Node);
    }
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("nighttip");
        Node.Lines = {
            Line(nullptr, TEXT("Fires. Lamplight. Don't chase them past the treeline — they pull you off the wall and swarm.")),
            Line(nullptr, TEXT("And keep a shield up. Here — the watch rotation notes, for your bench."))
        };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Back"));
            Choice.GotoNodeId = TEXT("hello");
            Node.Choices.Add(Choice);
        }
    }
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("rumor");
        Node.Lines = {
            Line(nullptr, TEXT("The Sunscar. Something made of glass has been walking the dunes at noon — flat, bright, wrong.")),
            Line(nullptr, TEXT("Kael's charts call it the Glass Tyrant. If you're headed that way... take the bright stuff. It hates light."))
        };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Back"));
            Choice.GotoNodeId = TEXT("hello");
            Node.Choices.Add(Choice);
        }
    }
    Registry->RegisterDialogueTree(Bram);

    // --- Farmer Jori (Dawnstead) — Sprigling husbandry tip ---
    UAstrawildDialogueTreeDefinition* Jori = NewObject<UAstrawildDialogueTreeDefinition>(Registry);
    Jori->DialogueId = TEXT("Dialogue_FarmerJori");
    Jori->EntryNodeId = TEXT("hello");
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("hello");
        Node.Lines = { Line(nullptr, TEXT("Spriglings turn the soil better than any hoe. I just follow behind with seeds and apologies.")) };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("How do you keep Spriglings happy?"));
            Choice.ForbiddenFlagId = TEXT("Jori_HusbandryTip");
            Choice.SetFlagId = TEXT("Jori_HusbandryTip");
            Choice.GiveResearchPoints = 6;
            Choice.GotoNodeId = TEXT("husbandry");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Jori->Nodes.Add(Node);
    }
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("husbandry");
        Node.Lines = {
            Line(nullptr, TEXT("Feed mix, a trough, and patience. They sulk without company — pen them near the fire, not the fence.")),
            Line(nullptr, TEXT("The old feeding schedule is in the shed ledger. Take it; my eyes are done with small print."))
        };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Jori->Nodes.Add(Node);
    }
    Registry->RegisterDialogueTree(Jori);

    // --- Fisher Nima (Driftwood Landing vendor) — tide table + shop ---
    UAstrawildDialogueTreeDefinition* Nima = NewObject<UAstrawildDialogueTreeDefinition>(Registry);
    Nima->DialogueId = TEXT("Dialogue_FisherNima");
    Nima->EntryNodeId = TEXT("hello");
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("hello");
        Node.Lines = { Line(nullptr, TEXT("Fresh catch, sea pearls, and gossip — the gossip's the expensive part.")) };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Browse the catch"));
            Choice.bOpenShop = true;
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("What's beyond the shallows?"));
            Choice.GotoNodeId = TEXT("beyond");
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Leave"));
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        Nima->Nodes.Add(Node);
    }
    {
        FAstrawildDialogueNode Node;
        Node.NodeId = TEXT("beyond");
        Node.Lines = {
            Line(nullptr, TEXT("There's a surveyor's ring out in the Azure Shallows — a sextant, still turning, still pointed at nothing.")),
            Line(nullptr, TEXT("Perry says the charts end there because the surveyor never came back to finish them. I say the sea keeps what it likes."))
        };
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Browse the catch"));
            Choice.bOpenShop = true;
            Choice.bEndDialogue = true;
            Node.Choices.Add(Choice);
        }
        {
            FAstrawildDialogueChoice Choice;
            Choice.Text = FText::FromString(TEXT("Back"));
            Choice.GotoNodeId = TEXT("hello");
            Node.Choices.Add(Choice);
        }
    }
    Registry->RegisterDialogueTree(Nima);
}

// ---------------------------------------------------------------------------
// Final Run (FR-5) — Act 3 "The Storm Crown" content pack.
//
// The game finally ENDS: MQ-13..17 walk the player from the three storm
// anchors, through the Glass Tyrant mini-boss and the Stratos Coil ceiling
// gate, into the Eye of the Maelstrom, against the Drowned Sovereign — then
// home to Dawnstead, where Warden Maren holds the last choice (two endings,
// wired through TriggerEndingId → the game state; see the Maren tree above).
// ---------------------------------------------------------------------------

void UAstrawildProductionContent::BuildFinalRunContent(UAstrawildItemRegistrySubsystem* Registry)
{
    if (!Registry)
    {
        return;
    }

    // --- Act 3 materials + key item ---

    UAstrawildItemDefinition* SovereignCore = MakeItem(Registry, TEXT("Item_SovereignCore"), TEXT("Sovereign Core"),
        EAstrawildItemCategory::QuestItem, 0.8f, 1);
    SovereignCore->Rarity = EAstrawildRarity::Legendary;
    SovereignCore->Description = FText::FromString(TEXT("The storm crown's heart, torn from the Drowned Sovereign. It still hums with the first pulse of the Vale."));
    SovereignCore->VendorPrice = 0; // Quest item — not for sale.

    UAstrawildItemDefinition* MaelstromGlass = MakeItem(Registry, TEXT("Item_MaelstromGlass"), TEXT("Maelstrom Glass"),
        EAstrawildItemCategory::Material, 0.5f, 20);
    MaelstromGlass->Rarity = EAstrawildRarity::Rare;
    MaelstromGlass->Description = FText::FromString(TEXT("Storm-fused glass. It rings when the crown stirs — and it coils resonance like wire."));
    MaelstromGlass->VendorPrice = 9;

    UAstrawildItemDefinition* StratosCoil = MakeItem(Registry, TEXT("Item_SkiffStratosCoil"), TEXT("Skiff Stratos Coil"),
        EAstrawildItemCategory::QuestItem, 2.0f, 1);
    StratosCoil->Rarity = EAstrawildRarity::Epic;
    StratosCoil->Description = FText::FromString(TEXT("A resonance coil that lifts a Dawn Skiff past the storm ceiling (120m → 160m). The sky gate opens."));
    StratosCoil->VendorPrice = 0; // Key item — not for sale.

    // --- Act 3 boss species roster ---

    // Glass Tyrant — the Sunscar's crystalline mini-boss (MQ-14). An encounter
    // DESIGN exception to the FR-3 matrix (Ash normally has no weakness): the
    // Tyrant's glass shatters under concentrated light — Bram's tip, the quest
    // text and the Dawn arsenal theme all agree.
    MakeProductionEcho(Registry, TEXT("Echo_GlassTyrant"), TEXT("Glass Tyrant"),
        EAstrawildElementType::Ash, EAstrawildEchoRole::Combat,
        260.0f, 38.0f, 22.0f, 520.0f,
        EAstrawildPersonality::Aggressive, EAstrawildActivityPattern::Diurnal,
        { }, 0.90f, EAstrawildElementType::Light,
        EAstrawildEchoFamily::Elemental, EAstrawildBodyPlan::Crystalline, EAstrawildSizeClass::Large,
        EAstrawildZone::SunscarDesert, EAstrawildRarity::Epic, EAstrawildEchoPassive::None,
        { }, { Stack(TEXT("Item_MaelstromGlass"), 2), Stack(TEXT("Item_DuneGlass"), 2) });

    // Eye Sentinel — the Sovereign's floating guards (dungeon adds + boss summons).
    // FR-3 matrix: Pulse falls to Light.
    MakeProductionEcho(Registry, TEXT("Echo_EyeSentinel"), TEXT("Eye Sentinel"),
        EAstrawildElementType::Pulse, EAstrawildEchoRole::Combat,
        90.0f, 22.0f, 12.0f, 480.0f,
        EAstrawildPersonality::Aggressive, EAstrawildActivityPattern::Nocturnal,
        { }, 0.85f, EAstrawildElementType::Light,
        EAstrawildEchoFamily::Construct, EAstrawildBodyPlan::Floating, EAstrawildSizeClass::Medium,
        EAstrawildZone::StormcrestHighlands, EAstrawildRarity::Rare, EAstrawildEchoPassive::None,
        { }, { Stack(TEXT("Item_MaelstromGlass"), 1) });

    // The Drowned Sovereign — the final boss (MQ-16). 400 HP base × the boss
    // health scale (5.0) = 2000 HP, three phases, weak to Dawn Light.
    MakeProductionEcho(Registry, TEXT("Echo_DrownedSovereign"), TEXT("The Drowned Sovereign"),
        EAstrawildElementType::Pulse, EAstrawildEchoRole::Combat,
        400.0f, 46.0f, 26.0f, 430.0f,
        EAstrawildPersonality::Aggressive, EAstrawildActivityPattern::Diurnal,
        { }, 0.95f, EAstrawildElementType::Light,
        EAstrawildEchoFamily::Ancient, EAstrawildBodyPlan::Serpent, EAstrawildSizeClass::Huge,
        EAstrawildZone::StormcrestHighlands, EAstrawildRarity::Legendary, EAstrawildEchoPassive::None,
        { }, { Stack(TEXT("Item_SovereignCore"), 1), Stack(TEXT("Item_MaelstromGlass"), 3) });

    // --- Skiff Engineering (FR-8 tech + recipe) ---

    MakeTech(Registry, TEXT("Tech_SkiffEngineering"), TEXT("Skiff Engineering"), EAstrawildTechEra::AdvancedEnergy,
        EAstrawildResearchBranch::Exploration, 25,
        { TEXT("Tech_AdvancedEnergy") },
        { TEXT("Recipe_SkiffStratosCoil") });

    MakeRecipe(Registry, TEXT("Recipe_SkiffStratosCoil"), TEXT("Skiff Stratos Coil"),
        { Stack(TEXT("Item_StormSilver"), 4), Stack(TEXT("Item_DuneGlass"), 3), Stack(TEXT("Item_MaelstromGlass"), 2) },
        { Stack(TEXT("Item_SkiffStratosCoil"), 1) }, 20.0f, TEXT("Tech_SkiffEngineering"), TEXT("Station_Workbench"));

    // --- The Sovereign's loot table (FR-7 boss-room override) ---

    MakeLoot(Registry, TEXT("Loot_EyeCore"),
        { Stack(TEXT("Item_SovereignCore"), 1), Stack(TEXT("Item_MaelstromGlass"), 2) }, 0.5f);

    // --- MQ-13 "The Storm Crown Stirs" ---
    UAstrawildQuestDefinition* StormAnchors = NewObject<UAstrawildQuestDefinition>(Registry);
    StormAnchors->QuestId = TEXT("Quest_StormAnchors");
    StormAnchors->Title = FText::FromString(TEXT("The Storm Crown Stirs"));
    StormAnchors->Summary = FText::FromString(TEXT("Three ancient anchors hum across the Vale — Frostveil, Sunscar, Stormcrest. Read them all; the crown is waking."));
    {
        FAstrawildQuestObjective Obj;
        Obj.Type = EAstrawildQuestObjectiveType::DiscoverPOI;
        Obj.TargetId = TEXT("POI_FrostveilSignalSource");
        Obj.RequiredCount = 1;
        Obj.ObjectiveText = FText::FromString(TEXT("Read the Frostveil signal source"));
        StormAnchors->Objectives.Add(Obj);
    }
    {
        FAstrawildQuestObjective Obj;
        Obj.Type = EAstrawildQuestObjectiveType::DiscoverPOI;
        Obj.TargetId = TEXT("POI_SunscarMirageStone");
        Obj.RequiredCount = 1;
        Obj.ObjectiveText = FText::FromString(TEXT("Read the Sunscar mirage stone"));
        StormAnchors->Objectives.Add(Obj);
    }
    {
        FAstrawildQuestObjective Obj;
        Obj.Type = EAstrawildQuestObjectiveType::DiscoverPOI;
        Obj.TargetId = TEXT("POI_StormcrestArray");
        Obj.RequiredCount = 1;
        Obj.ObjectiveText = FText::FromString(TEXT("Read the Stormcrest array"));
        StormAnchors->Objectives.Add(Obj);
    }
    StormAnchors->RewardItems = { Stack(TEXT("Item_MaelstromGlass"), 1) };
    StormAnchors->RewardResearchPoints = 25;
    StormAnchors->NextQuestId = TEXT("Quest_CrownRelay");
    Registry->RegisterQuest(StormAnchors);

    // --- MQ-14 "The Crown Relay" ---
    UAstrawildQuestDefinition* CrownRelay = NewObject<UAstrawildQuestDefinition>(Registry);
    CrownRelay->QuestId = TEXT("Quest_CrownRelay");
    CrownRelay->Title = FText::FromString(TEXT("The Crown Relay"));
    CrownRelay->Summary = FText::FromString(TEXT("A Glass Tyrant squats on the Sunscar relay, and the skiff needs a coil that sings above the storm ceiling."));
    {
        FAstrawildQuestObjective Obj;
        Obj.Type = EAstrawildQuestObjectiveType::DefeatCreature;
        Obj.TargetId = TEXT("Creature_GlassTyrant");
        Obj.RequiredCount = 1;
        Obj.ObjectiveText = FText::FromString(TEXT("Break the Glass Tyrant"));
        CrownRelay->Objectives.Add(Obj);
    }
    {
        FAstrawildQuestObjective Obj;
        Obj.Type = EAstrawildQuestObjectiveType::CraftRecipe;
        Obj.TargetId = TEXT("Recipe_SkiffStratosCoil");
        Obj.RequiredCount = 1;
        Obj.ObjectiveText = FText::FromString(TEXT("Craft the Skiff Stratos Coil"));
        CrownRelay->Objectives.Add(Obj);
    }
    CrownRelay->RewardItems = { Stack(TEXT("Item_DawnShard"), 6) };
    CrownRelay->RewardResearchPoints = 30;
    CrownRelay->NextQuestId = TEXT("Quest_EyeOfTheMaelstrom");
    Registry->RegisterQuest(CrownRelay);

    // --- MQ-15 "The Eye of the Maelstrom" ---
    UAstrawildQuestDefinition* EyeQuest = NewObject<UAstrawildQuestDefinition>(Registry);
    EyeQuest->QuestId = TEXT("Quest_EyeOfTheMaelstrom");
    EyeQuest->Title = FText::FromString(TEXT("The Eye of the Maelstrom"));
    EyeQuest->Summary = FText::FromString(TEXT("The eye opens above Stormcrest, past the 150-metre storm ceiling. Coil installed, climb, and enter."));
    {
        FAstrawildQuestObjective Obj;
        Obj.Type = EAstrawildQuestObjectiveType::ReachLocation;
        Obj.TargetId = TEXT("Location_EyeGate");
        Obj.RequiredCount = 1;
        Obj.ObjectiveText = FText::FromString(TEXT("Reach the Eye Gate above Stormcrest"));
        EyeQuest->Objectives.Add(Obj);
    }
    {
        FAstrawildQuestObjective Obj;
        Obj.Type = EAstrawildQuestObjectiveType::ReachLocation;
        Obj.TargetId = TEXT("Location_EyeOfTheMaelstrom");
        Obj.RequiredCount = 1;
        Obj.ObjectiveText = FText::FromString(TEXT("Enter the Eye of the Maelstrom"));
        EyeQuest->Objectives.Add(Obj);
    }
    EyeQuest->RewardItems = { Stack(TEXT("Item_EnergyCell"), 6) };
    EyeQuest->RewardResearchPoints = 30;
    EyeQuest->NextQuestId = TEXT("Quest_TheDrownedSovereign");
    Registry->RegisterQuest(EyeQuest);

    // --- MQ-16 "The Drowned Sovereign" ---
    UAstrawildQuestDefinition* Sovereign = NewObject<UAstrawildQuestDefinition>(Registry);
    Sovereign->QuestId = TEXT("Quest_TheDrownedSovereign");
    Sovereign->Title = FText::FromString(TEXT("The Drowned Sovereign"));
    Sovereign->Summary = FText::FromString(TEXT("Everything the storms drowned is still down there, wearing a crown. Dawn Light is its bane — three phases, one core."));
    {
        FAstrawildQuestObjective Obj;
        Obj.Type = EAstrawildQuestObjectiveType::DefeatCreature;
        Obj.TargetId = TEXT("Creature_DrownedSovereign");
        Obj.RequiredCount = 1;
        Obj.ObjectiveText = FText::FromString(TEXT("Defeat the Drowned Sovereign"));
        Sovereign->Objectives.Add(Obj);
    }
    Sovereign->RewardItems = { Stack(TEXT("Item_SovereignCore"), 1), Stack(TEXT("Item_MaelstromGlass"), 3) };
    Sovereign->RewardResearchPoints = 40;
    Sovereign->NextQuestId = TEXT("Quest_FirstDawnAgain");
    Registry->RegisterQuest(Sovereign);

    // --- MQ-17 "First Dawn Again" (terminus) ---
    UAstrawildQuestDefinition* FirstDawn = NewObject<UAstrawildQuestDefinition>(Registry);
    FirstDawn->QuestId = TEXT("Quest_FirstDawnAgain");
    FirstDawn->Title = FText::FromString(TEXT("First Dawn Again"));
    FirstDawn->Summary = FText::FromString(TEXT("Carry the core home to Dawnstead. Maren is waiting at the watch-fire — and the Vale is waiting on her word."));
    {
        FAstrawildQuestObjective Obj;
        Obj.Type = EAstrawildQuestObjectiveType::ReachLocation;
        Obj.TargetId = TEXT("Location_Dawnstead");
        Obj.RequiredCount = 1;
        Obj.ObjectiveText = FText::FromString(TEXT("Return to Dawnstead and speak with Warden Maren"));
        FirstDawn->Objectives.Add(Obj);
    }
    FirstDawn->RewardItems = { };
    FirstDawn->RewardResearchPoints = 50;
    FirstDawn->NextQuestId = NAME_None; // Chain terminus — the endings live in Maren's crown dialogue.
    Registry->RegisterQuest(FirstDawn);

    UE_LOG(LogAstrawildEconomy, Log,
        TEXT("Final Run content registered: Act 3 quest chain MQ-13..17 (StormAnchors→CrownRelay→EyeOfTheMaelstrom→DrownedSovereign→FirstDawnAgain), 3 boss species (GlassTyrant/EyeSentinel/DrownedSovereign), 3 items (SovereignCore/MaelstromGlass/SkiffStratosCoil), Tech_SkiffEngineering + coil recipe, Loot_EyeCore, Maren ending dialogue (2 endings)."));
}

// ---------------------------------------------------------------------------

void UAstrawildProductionContent::BuildAll(UAstrawildItemRegistrySubsystem* Registry)
{
    if (!Registry)
    {
        return;
    }
    BuildWeapons(Registry);
    BuildArmorAndScanners(Registry);
    BuildRobotics(Registry);
    BuildResourceNodes(Registry);
    BuildWorkSites(Registry);
    BuildWorldEvents(Registry);
    BuildPOIs(Registry);
    BuildBiomes(Registry);
    BuildProductionEchoes(Registry);
    BuildEvolutionTargets(Registry); // CP-02: evolved forms + chain links + gates.
    BuildProductionTechnologies(Registry);
    BuildProductionQuests(Registry);
    BuildDialogueTrees(Registry);
    BuildFinalRunContent(Registry); // FR-5: Act 3 "The Storm Crown" + the two endings.

    UE_LOG(LogAstrawildEconomy, Log,
        TEXT("Production V2 + Final Run content registered: 8 weapon profiles, 7 armor/scanner pieces, 6 robotics items, 10 resource nodes, 4 work sites, 9 world events, 13 POIs, 12 biomes, 6 production Echoes + 6 evolution targets + 3 Final Run bosses, 6 + 1 technologies, 2 + 5 quests (17 total), 6 + 5 dialogue trees (11 total — every NPC converses)."));
}
