#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Data/AstrawildBreedingFusionData.generated.h"

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildBreedingFusionRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fusion")
    FGameplayTag ParentSpeciesA;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fusion")
    FGameplayTag ParentSpeciesB;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fusion")
    FGameplayTag OffspringSpeciesTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fusion")
    TArray<EAstrawildElement> OffspringElementalAffinities;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fusion")
    TArray<FGameplayTag> GuaranteedInheritedTraitTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fusion", meta=(ClampMin="0.0", ClampMax="1.0"))
    float TraitInheritanceChance = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fusion", meta=(ClampMin="0.0", ClampMax="1.0"))
    float HiddenPassiveUnlockChance = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fusion")
    FGameplayTag FusionGroupTag;
};
