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

TArray<UAstrawildBuildingDefinition*> UAstrawildItemRegistrySubsystem::GetUnlockedBuildings(FName PlayerId) const
{
    TArray<UAstrawildBuildingDefinition*> Out;
    const UWorld* World = GetWorld();
    const UAstrawildResearchSubsystem* Research = World ? World->GetGameInstance()->GetSubsystem<UAstrawildResearchSubsystem>() : nullptr;

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
    Recipes.GenerateValueArray(Out);
    return Out;
}

TArray<UAstrawildEchoDefinition*> UAstrawildItemRegistrySubsystem::GetAllEchoDefinitions() const
{
    TArray<UAstrawildEchoDefinition*> Out;
    Echoes.GenerateValueArray(Out);
    return Out;
}
