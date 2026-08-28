#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AstrawildDyeData.generated.h"

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildDyeRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dye")
    FGameplayTag DyeTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dye")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dye")
    FLinearColor PrimaryTint = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dye")
    FLinearColor SecondaryTint = FLinearColor::White;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dye")
    FName MaterialParameterName = TEXT("DyeTint");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dye")
    FGameplayTag UnlockRequirementTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dye", meta=(ClampMin="0"))
    int32 CraftCost = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dye")
    bool bUnlockedByDefault = false;
};
