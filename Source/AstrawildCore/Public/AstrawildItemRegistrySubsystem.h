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
class UAstrawildWeaponDefinition;
class UAstrawildResourceNodeDefinition;
class UAstrawildRobotDefinition;
class UAstrawildWorkSiteDefinition;
class UAstrawildWorldEventDefinition;
class UAstrawildPOIDefinition;
class UAstrawildBiomeDefinition;

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

    // --- Production V2: data-driven content foundation registries ---

    void RegisterWeapon(UAstrawildWeaponDefinition* Definition);
    void RegisterResourceNode(UAstrawildResourceNodeDefinition* Definition);
    void RegisterRobot(UAstrawildRobotDefinition* Definition);
    void RegisterWorkSite(UAstrawildWorkSiteDefinition* Definition);
    void RegisterWorldEvent(UAstrawildWorldEventDefinition* Definition);
    void RegisterPOI(UAstrawildPOIDefinition* Definition);
    void RegisterBiome(UAstrawildBiomeDefinition* Definition);

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

    // --- Production V2 lookups ---

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    UAstrawildWeaponDefinition* FindWeapon(FName WeaponId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    UAstrawildResourceNodeDefinition* FindResourceNode(FName NodeId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    UAstrawildRobotDefinition* FindRobot(FName RobotId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    UAstrawildWorkSiteDefinition* FindWorkSite(FName SiteId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    UAstrawildWorldEventDefinition* FindWorldEvent(FName EventId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    UAstrawildPOIDefinition* FindPOI(FName PoiId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    UAstrawildBiomeDefinition* FindBiome(FName BiomeId) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    TArray<UAstrawildWeaponDefinition*> GetAllWeapons() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    TArray<UAstrawildWorldEventDefinition*> GetAllWorldEvents() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    TArray<UAstrawildPOIDefinition*> GetAllPOIs() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    TArray<UAstrawildWorkSiteDefinition*> GetAllWorkSiteDefinitions() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    TArray<UAstrawildBiomeDefinition*> GetAllBiomes() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Registry")
    TArray<UAstrawildResourceNodeDefinition*> GetAllResourceNodeDefinitions() const;

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

    // --- Production V2 registries ---

    UPROPERTY()
    TMap<FName, TObjectPtr<UAstrawildWeaponDefinition>> Weapons;

    UPROPERTY()
    TMap<FName, TObjectPtr<UAstrawildResourceNodeDefinition>> ResourceNodes;

    UPROPERTY()
    TMap<FName, TObjectPtr<UAstrawildRobotDefinition>> Robots;

    UPROPERTY()
    TMap<FName, TObjectPtr<UAstrawildWorkSiteDefinition>> WorkSites;

    UPROPERTY()
    TMap<FName, TObjectPtr<UAstrawildWorldEventDefinition>> WorldEvents;

    UPROPERTY()
    TMap<FName, TObjectPtr<UAstrawildPOIDefinition>> POIs;

    UPROPERTY()
    TMap<FName, TObjectPtr<UAstrawildBiomeDefinition>> Biomes;

    void BuildContentDefaults();
};
