#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Data/AstrawildBreedingData.generated.h"

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildBreedingGroupRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Breeding")
    FName BreedingGroupId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Breeding")
    TArray<FGameplayTag> CompatibleSpeciesTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Breeding", meta=(ClampMin="1.0"))
    float IncubationDurationSeconds = 900.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Breeding", meta=(ClampMin="0.0", ClampMax="1.0"))
    float MutationChance = 0.02f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Breeding", meta=(ClampMin="0"))
    int32 MaxInheritedTraits = 3;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildEchoTraitRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trait")
    FGameplayTag TraitTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trait")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trait")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trait", meta=(ClampMin="0.0"))
    float HealthMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trait", meta=(ClampMin="0.0"))
    float AttackMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trait", meta=(ClampMin="0.0"))
    float DefenseMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trait", meta=(ClampMin="0.0"))
    float WorkSpeedMultiplier = 1.0f;
};
