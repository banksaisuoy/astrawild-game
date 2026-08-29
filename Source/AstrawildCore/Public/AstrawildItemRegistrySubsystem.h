#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstrawildItemRegistrySubsystem.generated.h"

class UAstrawildItemDefinition;
class UAstrawildRecipeDefinition;
class UAstrawildEchoDefinition;
class UAstrawildBuildingDefinition;
class UAstrawildTechnologyDefinition;
class UAstrawildQuestDefinition;
class UAstrawildLootTableDefinition;
class UAstrawildNPCDefinition;

/**
 * Central registry of gameplay definitions (directive §35).
 * The ContentLibrary registers code-defined defaults at world start; when real
 * .uasset data assets are authored they register through the same API and
 * (per-id) override the code defaults. Gameplay code resolves ids through this
 * registry and never through hard-coded object references.
 */
UCLASS()
class ASTRAWILDCORE_API UAstrawildItemRegistrySubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;
    virtual void OnWorldBeginPlay(UWorld& InWorld) override;

    void RegisterItem(UAstrawildItemDefinition* Definition);
    void RegisterRecipe(UAstrawildRecipeDefinition* Definition);
    void RegisterEcho(UAstrawildEchoDefinition* Definition);
    void RegisterBuilding(UAstrawildBuildingDefinition* Definition);
    void RegisterTechnology(UAstrawildTechnologyDefinition* Definition);
    void RegisterQuest(UAstrawildQuestDefinition* Definition);

    /** Wave 3: weighted loot tables resolve through the same registry contract. */
    void RegisterLootTable(UAstrawildLootTableDefinition* Definition);

    /** Wave 3: NPC definitions (dialogue/quest hooks) resolve through the registry. */
    void RegisterNPC(UAstrawildNPCDefinition* Definition);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    UAstrawildItemDefinition* FindItem(FName ItemId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    UAstrawildRecipeDefinition* FindRecipe(FName RecipeId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    UAstrawildEchoDefinition* FindEcho(FName DefinitionId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    UAstrawildBuildingDefinition* FindBuilding(FName DefinitionId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    UAstrawildTechnologyDefinition* FindTechnology(FName TechId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    UAstrawildQuestDefinition* FindQuest(FName QuestId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    UAstrawildLootTableDefinition* FindLootTable(FName LootTableId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    UAstrawildNPCDefinition* FindNPCDefinition(FName NpcId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    TArray<UAstrawildBuildingDefinition*> GetUnlockedBuildings(FName PlayerId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    TArray<UAstrawildRecipeDefinition*> GetAllRecipes() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    TArray<UAstrawildTechnologyDefinition*> GetAllTechnologies() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    TArray<UAstrawildEchoDefinition*> GetAllEchoDefinitions() const;

private:
    UPROPERTY()
    TMap<FName, TObjectPtr<UAstrawildItemDefinition>> Items;

    UPROPERTY()
    TMap<FName, TObjectPtr<UAstrawildRecipeDefinition>> Recipes;

    UPROPERTY()
    TMap<FName, TObjectPtr<UAstrawildEchoDefinition>> Echoes;

    UPROPERTY()
    TMap<FName, TObjectPtr<UAstrawildBuildingDefinition>> Buildings;

    UPROPERTY()
    TMap<FName, TObjectPtr<UAstrawildTechnologyDefinition>> Technologies;

    UPROPERTY()
    TMap<FName, TObjectPtr<UAstrawildQuestDefinition>> Quests;

    UPROPERTY()
    TMap<FName, TObjectPtr<UAstrawildLootTableDefinition>> LootTables;

    UPROPERTY()
    TMap<FName, TObjectPtr<UAstrawildNPCDefinition>> NPCDefinitions;

    void BuildContentDefaults();
};
