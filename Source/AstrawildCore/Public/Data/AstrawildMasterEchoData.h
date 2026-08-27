#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AstrawildTypes.h"
#include "AstrawildMasterEchoData.generated.h"

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildMasterEchoSkillRow
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill")
    FName SkillId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill")
    FGameplayTag SkillTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill")
    FGameplayTag ElementTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill", meta=(ClampMin="0.0"))
    float CooldownSeconds = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill", meta=(ClampMin="0.0"))
    float DamageMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skill", meta=(ClampMin="0.0"))
    float TelegraphSeconds = 0.25f;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildMasterEchoRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
    int32 DexOrder = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
    FGameplayTag SpeciesTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
    FText SpeciesName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
    FText SpeciesTitle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Identity")
    FText AnatomyConcept;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ecology")
    FText Diet;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ecology")
    FText SocialBehavior;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ecology")
    FText Temperament;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ecology")
    FGameplayTag HabitatBiomeTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Ecology")
    FGameplayTag ActivityCycleTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Classification")
    EAstrawildElement PrimaryElement = EAstrawildElement::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Classification")
    TArray<EAstrawildElement> ElementalAffinities;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Classification")
    EAstrawildEchoRole Role = EAstrawildEchoRole::Combat;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats", meta=(ClampMin="1.0"))
    float BaseMaxHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats", meta=(ClampMin="1.0"))
    float BaseAttackPower = 20.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats", meta=(ClampMin="0.0"))
    float BaseDefensePower = 15.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats", meta=(ClampMin="0.0"))
    float BaseStamina = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats", meta=(ClampMin="0.0"))
    float BaseWalkSpeed = 250.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Stats", meta=(ClampMin="0.0"))
    float BaseRunSpeed = 550.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Capture", meta=(ClampMin="0.1"))
    float CaptureDifficultyModifier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Work")
    TArray<int32> WorkSuitabilityLevels;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Work")
    TArray<FGameplayTag> WorkSuitabilityTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Traits")
    TArray<FGameplayTag> PassiveTraitTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skills")
    TArray<FGameplayTag> ActiveSkillTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skills")
    TArray<FGameplayTag> ActiveSkillElementTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skills")
    TArray<float> ActiveSkillCooldowns;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skills")
    TArray<float> ActiveSkillDamageMultipliers;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Skills")
    TArray<float> ActiveSkillTelegraphs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Partner")
    FGameplayTag PartnerSkillTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Partner")
    FGameplayTag MountedWeaponTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drops")
    TArray<FGameplayTag> DropItemTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Drops")
    TArray<int32> DropItemQuantities;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fusion")
    FGameplayTag ParentSpeciesA;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fusion")
    FGameplayTag ParentSpeciesB;
};
