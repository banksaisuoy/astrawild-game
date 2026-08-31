#include "AstrawildItemRegistrySubsystem.h"

#include "AstrawildContentLibrary.h"
#include "AstrawildDataAssets.h"
#include "AstrawildLog.h"
#include "AstrawildResearchSubsystem.h"
#include "Engine/World.h"

bool UAstrawildItemRegistrySubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UAstrawildItemRegistrySubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);

    // Code-defined default content (replaceable by .uasset registrations later).
    if (InWorld.GetNetMode() != NM_Client)
    {
        UAstrawildContentLibrary::BuildDefaults(this);
    }
}

void UAstrawildItemRegistrySubsystem::RegisterItem(UAstrawildItemDefinition* Definition)
{
    if (IsValid(Definition) && !Definition->ItemId.IsNone())
    {
        Items.Add(Definition->ItemId, Definition);
    }
}

void UAstrawildItemRegistrySubsystem::RegisterRecipe(UAstrawildRecipeDefinition* Definition)
{
    if (IsValid(Definition) && !Definition->RecipeId.IsNone())
    {
        Recipes.Add(Definition->RecipeId, Definition);
    }
}

void UAstrawildItemRegistrySubsystem::RegisterEcho(UAstrawildEchoDefinition* Definition)
{
    if (IsValid(Definition) && !Definition->DefinitionId.IsNone())
    {
        Echoes.Add(Definition->DefinitionId, Definition);
    }
}

void UAstrawildItemRegistrySubsystem::RegisterBuilding(UAstrawildBuildingDefinition* Definition)
{
    if (IsValid(Definition) && !Definition->DefinitionId.IsNone())
    {
        Buildings.Add(Definition->DefinitionId, Definition);
    }
}

void UAstrawildItemRegistrySubsystem::RegisterTechnology(UAstrawildTechnologyDefinition* Definition)
{
    if (IsValid(Definition) && !Definition->TechId.IsNone())
    {
        Technologies.Add(Definition->TechId, Definition);
    }
}

void UAstrawildItemRegistrySubsystem::RegisterQuest(UAstrawildQuestDefinition* Definition)
{
    if (IsValid(Definition) && !Definition->QuestId.IsNone())
    {
        Quests.Add(Definition->QuestId, Definition);
    }
}

void UAstrawildItemRegistrySubsystem::RegisterLootTable(UAstrawildLootTableDefinition* Definition)
{
    if (IsValid(Definition) && !Definition->LootTableId.IsNone())
    {
        LootTables.Add(Definition->LootTableId, Definition);
    }
}

void UAstrawildItemRegistrySubsystem::RegisterNPC(UAstrawildNPCDefinition* Definition)
{
    if (IsValid(Definition) && !Definition->NpcId.IsNone())
    {
        NPCDefinitions.Add(Definition->NpcId, Definition);
    }
}

UAstrawildItemDefinition* UAstrawildItemRegistrySubsystem::FindItem(const FName ItemId) const
{
    return Items.FindRef(ItemId);
}

UAstrawildRecipeDefinition* UAstrawildItemRegistrySubsystem::FindRecipe(const FName RecipeId) const
{
    return Recipes.FindRef(RecipeId);
}

UAstrawildEchoDefinition* UAstrawildItemRegistrySubsystem::FindEcho(const FName DefinitionId) const
{
    return Echoes.FindRef(DefinitionId);
}

UAstrawildBuildingDefinition* UAstrawildItemRegistrySubsystem::FindBuilding(const FName DefinitionId) const
{
    return Buildings.FindRef(DefinitionId);
}

UAstrawildTechnologyDefinition* UAstrawildItemRegistrySubsystem::FindTechnology(const FName TechId) const
{
    return Technologies.FindRef(TechId);
}

UAstrawildQuestDefinition* UAstrawildItemRegistrySubsystem::FindQuest(const FName QuestId) const
{
    return Quests.FindRef(QuestId);
}

UAstrawildLootTableDefinition* UAstrawildItemRegistrySubsystem::FindLootTable(const FName LootTableId) const
{
    return LootTables.FindRef(LootTableId);
}

UAstrawildNPCDefinition* UAstrawildItemRegistrySubsystem::FindNPCDefinition(const FName NpcId) const
{
    return NPCDefinitions.FindRef(NpcId);
}

TArray<UAstrawildBuildingDefinition*> UAstrawildItemRegistrySubsystem::GetUnlockedBuildings(FName PlayerId) const
{
    TArray<UAstrawildBuildingDefinition*> Out;
    const UWorld* World = GetWorld();
    // Audit C-7 (final run): GetGameInstance() itself can be null in editor/preview
    // worlds — guard the whole chain instead of dereferencing blindly.
    const UAstrawildResearchSubsystem* Research = (World && World->GetGameInstance())
        ? World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>()
        : nullptr;

    for (const TPair<FName, TObjectPtr<UAstrawildBuildingDefinition>>& Pair : Buildings)
    {
        const UAstrawildBuildingDefinition* Def = Pair.Value;
        if (!Def)
        {
            continue;
        }
        if (Def->RequiredTechId.IsNone() || (Research && Research->IsTechUnlocked(Def->RequiredTechId)))
        {
            Out.Add(Pair.Value);
        }
    }
    return Out;
}

TArray<UAstrawildRecipeDefinition*> UAstrawildItemRegistrySubsystem::GetAllRecipes() const
{
    TArray<UAstrawildRecipeDefinition*> Out;
    Out.Reserve(Recipes.Num());
    for (const auto& Pair : Recipes)
    {
        if (Pair.Value)
        {
            Out.Add(Pair.Value);
        }
    }
    return Out;
}

TArray<UAstrawildTechnologyDefinition*> UAstrawildItemRegistrySubsystem::GetAllTechnologies() const
{
    // Audit C-2: research unlock path needs to enumerate the full tech list (auto-grant
    // of root techs + "next unlockable" selection at the Research Desk).
    TArray<UAstrawildTechnologyDefinition*> Out;
    Out.Reserve(Technologies.Num());
    for (const auto& Pair : Technologies)
    {
        if (Pair.Value)
        {
            Out.Add(Pair.Value);
        }
    }
    return Out;
}

TArray<UAstrawildEchoDefinition*> UAstrawildItemRegistrySubsystem::GetAllEchoDefinitions() const
{
    TArray<UAstrawildEchoDefinition*> Out;
    Out.Reserve(Echoes.Num());
    for (const auto& Pair : Echoes)
    {
        if (Pair.Value)
        {
            Out.Add(Pair.Value);
        }
    }
    return Out;
}

// --- Production V2: data-driven content foundation registries ---

void UAstrawildItemRegistrySubsystem::RegisterWeapon(UAstrawildWeaponDefinition* Definition)
{
    if (IsValid(Definition) && !Definition->WeaponId.IsNone())
    {
        Weapons.Add(Definition->WeaponId, Definition);
    }
}

void UAstrawildItemRegistrySubsystem::RegisterResourceNode(UAstrawildResourceNodeDefinition* Definition)
{
    if (IsValid(Definition) && !Definition->NodeId.IsNone() && !Definition->ResourceItemId.IsNone())
    {
        ResourceNodes.Add(Definition->NodeId, Definition);
    }
}

void UAstrawildItemRegistrySubsystem::RegisterRobot(UAstrawildRobotDefinition* Definition)
{
    if (IsValid(Definition) && !Definition->RobotId.IsNone())
    {
        Robots.Add(Definition->RobotId, Definition);
    }
}

void UAstrawildItemRegistrySubsystem::RegisterWorkSite(UAstrawildWorkSiteDefinition* Definition)
{
    if (IsValid(Definition) && !Definition->SiteId.IsNone() && !Definition->OutputItemId.IsNone())
    {
        WorkSites.Add(Definition->SiteId, Definition);
    }
}

void UAstrawildItemRegistrySubsystem::RegisterWorldEvent(UAstrawildWorldEventDefinition* Definition)
{
    if (IsValid(Definition) && !Definition->EventId.IsNone())
    {
        WorldEvents.Add(Definition->EventId, Definition);
    }
}

void UAstrawildItemRegistrySubsystem::RegisterPOI(UAstrawildPOIDefinition* Definition)
{
    if (IsValid(Definition) && !Definition->PoiId.IsNone())
    {
        POIs.Add(Definition->PoiId, Definition);
    }
}

void UAstrawildItemRegistrySubsystem::RegisterBiome(UAstrawildBiomeDefinition* Definition)
{
    if (IsValid(Definition) && !Definition->BiomeId.IsNone())
    {
        Biomes.Add(Definition->BiomeId, Definition);
    }
}

UAstrawildWeaponDefinition* UAstrawildItemRegistrySubsystem::FindWeapon(const FName WeaponId) const
{
    return Weapons.FindRef(WeaponId);
}

UAstrawildResourceNodeDefinition* UAstrawildItemRegistrySubsystem::FindResourceNode(const FName NodeId) const
{
    return ResourceNodes.FindRef(NodeId);
}

UAstrawildRobotDefinition* UAstrawildItemRegistrySubsystem::FindRobot(const FName RobotId) const
{
    return Robots.FindRef(RobotId);
}

UAstrawildWorkSiteDefinition* UAstrawildItemRegistrySubsystem::FindWorkSite(const FName SiteId) const
{
    return WorkSites.FindRef(SiteId);
}

UAstrawildWorldEventDefinition* UAstrawildItemRegistrySubsystem::FindWorldEvent(const FName EventId) const
{
    return WorldEvents.FindRef(EventId);
}

UAstrawildPOIDefinition* UAstrawildItemRegistrySubsystem::FindPOI(const FName PoiId) const
{
    return POIs.FindRef(PoiId);
}

UAstrawildBiomeDefinition* UAstrawildItemRegistrySubsystem::FindBiome(const FName BiomeId) const
{
    return Biomes.FindRef(BiomeId);
}

TArray<UAstrawildWeaponDefinition*> UAstrawildItemRegistrySubsystem::GetAllWeapons() const
{
    TArray<UAstrawildWeaponDefinition*> Out;
    Out.Reserve(Weapons.Num());
    for (const auto& Pair : Weapons)
    {
        if (Pair.Value)
        {
            Out.Add(Pair.Value);
        }
    }
    return Out;
}

TArray<UAstrawildWorldEventDefinition*> UAstrawildItemRegistrySubsystem::GetAllWorldEvents() const
{
    TArray<UAstrawildWorldEventDefinition*> Out;
    Out.Reserve(WorldEvents.Num());
    for (const auto& Pair : WorldEvents)
    {
        if (Pair.Value)
        {
            Out.Add(Pair.Value);
        }
    }
    return Out;
}

TArray<UAstrawildPOIDefinition*> UAstrawildItemRegistrySubsystem::GetAllPOIs() const
{
    TArray<UAstrawildPOIDefinition*> Out;
    Out.Reserve(POIs.Num());
    for (const auto& Pair : POIs)
    {
        if (Pair.Value)
        {
            Out.Add(Pair.Value);
        }
    }
    return Out;
}

TArray<UAstrawildWorkSiteDefinition*> UAstrawildItemRegistrySubsystem::GetAllWorkSiteDefinitions() const
{
    TArray<UAstrawildWorkSiteDefinition*> Out;
    Out.Reserve(WorkSites.Num());
    for (const auto& Pair : WorkSites)
    {
        if (Pair.Value)
        {
            Out.Add(Pair.Value);
        }
    }
    return Out;
}

TArray<UAstrawildBiomeDefinition*> UAstrawildItemRegistrySubsystem::GetAllBiomes() const
{
    TArray<UAstrawildBiomeDefinition*> Out;
    Out.Reserve(Biomes.Num());
    for (const auto& Pair : Biomes)
    {
        if (Pair.Value)
        {
            Out.Add(Pair.Value);
        }
    }
    return Out;
}

TArray<UAstrawildResourceNodeDefinition*> UAstrawildItemRegistrySubsystem::GetAllResourceNodeDefinitions() const
{
    TArray<UAstrawildResourceNodeDefinition*> Out;
    Out.Reserve(ResourceNodes.Num());
    for (const auto& Pair : ResourceNodes)
    {
        if (Pair.Value)
        {
            Out.Add(Pair.Value);
        }
    }
    return Out;
}
