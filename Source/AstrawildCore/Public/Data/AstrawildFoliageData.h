#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AstrawildFoliageData.generated.h"

UENUM(BlueprintType)
enum class EAstrawildFoliageRuleKind : uint8
{
    GroundCover,
    Tree,
    Shrub,
    ResourceNode,
    RockFormation
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildFoliageRuleRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Foliage")
    FName FoliageRuleId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Foliage")
    FName BiomeId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Foliage")
    EAstrawildFoliageRuleKind RuleKind = EAstrawildFoliageRuleKind::GroundCover;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Foliage")
    FName FoliageAssetId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Foliage")
    FGameplayTag ResourceTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Foliage", meta=(ClampMin="0.0"))
    float DensityScale = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Foliage", meta=(ClampMin="0.0", ClampMax="90.0"))
    float MinSlopeDegrees = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Foliage", meta=(ClampMin="0.0", ClampMax="90.0"))
    float MaxSlopeDegrees = 45.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Foliage")
    float MinHeightMeters = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Foliage")
    float MaxHeightMeters = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Foliage", meta=(ClampMin="0.0", ClampMax="1.0"))
    float WindResponse = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Foliage")
    bool bRespondsToCharacters = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Foliage")
    bool bUseNanite = false;
};
