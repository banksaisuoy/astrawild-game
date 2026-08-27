#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AstrawildEcosystemData.generated.h"

UENUM(BlueprintType)
enum class EAstrawildEcosystemTemperament : uint8
{
    Docile,
    Curious,
    Territorial,
    PackHunter,
    SolitaryApex,
    NocturnalScavenger
};

UENUM(BlueprintType)
enum class EAstrawildEcosystemState : uint8
{
    Roam,
    Graze,
    Forage,
    Drink,
    Rest,
    Flee,
    Investigate,
    DefendTerritory,
    Hunt,
    Flock,
    Migrate,
    Stunned
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildEcosystemBehaviorRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ecosystem")
    FGameplayTag SpeciesTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ecosystem")
    EAstrawildEcosystemTemperament Temperament = EAstrawildEcosystemTemperament::Docile;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ecosystem")
    FGameplayTag DietTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ecosystem")
    FGameplayTag SocialGroupTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ecosystem", meta=(ClampMin="0.0"))
    float PerceptionRadius = 2500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ecosystem", meta=(ClampMin="0.0"))
    float TerritoryRadius = 1200.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ecosystem", meta=(ClampMin="0.0"))
    float HungerSecondsUntilForage = 900.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ecosystem", meta=(ClampMin="0.0"))
    float FleeHealthThreshold = 0.25f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ecosystem", meta=(ClampMin="0.0"))
    float DefendHealthThreshold = 0.55f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ecosystem")
    bool bCanMigrateDuringWorldEvents = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ecosystem")
    bool bFormsGroups = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ecosystem")
    bool bDefendsYoung = false;
};
