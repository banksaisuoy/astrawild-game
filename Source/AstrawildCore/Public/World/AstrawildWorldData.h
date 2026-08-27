#pragma once

#include "CoreMinimal.h"
#include "AstrawildTypes.h"
#include "AstrawildWorldData.generated.h"

UENUM(BlueprintType)
enum class EAstrawildBiomeId : uint8
{
    DawnMeadows UMETA(DisplayName="Dawn Meadows"),
    SylvanRainforest UMETA(DisplayName="Sylvan Rainforest"),
    ScorchedObsidianCaldera UMETA(DisplayName="Scorched Obsidian Caldera"),
    GlacialZenith UMETA(DisplayName="Glacial Zenith")
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildBiomeDefinition : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Biome")
    FName BiomeId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Biome")
    EAstrawildBiomeId Biome = EAstrawildBiomeId::DawnMeadows;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Biome")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Biome")
    int32 MinLevel = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Biome")
    int32 MaxLevel = 15;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Biome", meta=(ClampMin="-5", ClampMax="5"))
    int32 TemperatureLevel = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Biome")
    TArray<EAstrawildElement> DominantElements;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Biome")
    TArray<FGameplayTag> ResourceTags;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildWorldSpawnRule : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
    FName SpawnRuleId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
    FGameplayTag SpeciesTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
    FName BiomeId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
    int32 MinLevel = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn")
    int32 MaxLevel = 5;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn", meta=(ClampMin="0.0"))
    float Weight = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn", meta=(ClampMin="0"))
    int32 MaxActive = 8;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildFastTravelSpire : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fast Travel")
    FName SpireId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fast Travel")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fast Travel")
    FName BiomeId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fast Travel")
    FTransform WorldTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fast Travel")
    bool bUnlockedByDefault = false;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildWorldCell
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="World Partition")
    FIntPoint Coordinates = FIntPoint::ZeroValue;

    UPROPERTY(BlueprintReadOnly, Category="World Partition")
    FBox2D Bounds = FBox2D(EForceInit::ForceInit);

    UPROPERTY(BlueprintReadOnly, Category="World Partition")
    int32 LinearIndex = INDEX_NONE;
};
