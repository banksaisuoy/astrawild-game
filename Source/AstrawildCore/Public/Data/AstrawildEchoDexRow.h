#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "AstrawildTypes.h"
#include "Data/AstrawildEchoDexRow.generated.h"

/**
 * Importable, presentation-agnostic EchoDex row. Binary DataAssets may reference this row after Editor import.
 */
USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildEchoDexRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Identity")
    FGameplayTag SpeciesTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Identity")
    FText SpeciesName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Identity")
    FText SpeciesTitle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Identity")
    FText LoreDescription;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Classification")
    EAstrawildElement PrimaryElement = EAstrawildElement::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Classification")
    TArray<EAstrawildElement> ElementalAffinities;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Classification")
    EAstrawildEchoRole Role = EAstrawildEchoRole::Combat;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Stats", meta=(ClampMin="1.0"))
    float BaseMaxHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Stats", meta=(ClampMin="1.0"))
    float BaseAttackPower = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Stats", meta=(ClampMin="0.0"))
    float BaseDefensePower = 15.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Stats", meta=(ClampMin="0.0"))
    float BaseWalkSpeed = 250.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Stats", meta=(ClampMin="0.0"))
    float BaseRunSpeed = 550.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Capture", meta=(ClampMin="0.1"))
    float CaptureDifficultyModifier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Traits")
    TArray<FGameplayTag> PassiveTraitTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Work")
    TArray<FGameplayTag> WorkSuitabilityTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Partner")
    FGameplayTag PartnerSkillTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Mount")
    FName MountProfileId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Mount")
    bool bCanBeMounted = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Breeding")
    FName BreedingGroupId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Evolution")
    FName EvolutionTargetId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Evolution", meta=(ClampMin="0"))
    int32 EvolutionLevel = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="EchoDex|Identity", meta=(ClampMin="1"))
    int32 DexOrder = 1;
};
