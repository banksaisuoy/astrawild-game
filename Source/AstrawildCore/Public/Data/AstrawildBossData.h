#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "AstrawildTypes.h"
#include "Data/AstrawildBossData.generated.h"

UENUM(BlueprintType)
enum class EAstrawildBossControllerState : uint8
{
    Idle,
    IntroCutscene,
    PhaseOneCombat,
    PhaseTwoEnrage,
    PhaseThreeDawn,
    SupermoveTelegraph,
    UltimateAoE,
    DefeatedLoot,
    Failed
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildBossEncounterRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss")
    FName EncounterId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss")
    FName DungeonId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss")
    FGameplayTag BossSpeciesTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss")
    EAstrawildElement PrimaryElement = EAstrawildElement::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss", meta=(ClampMin="1"))
    int32 RecommendedLevel = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss", meta=(ClampMin="1.0"))
    float MaxHealth = 30000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss", meta=(ClampMin="0.05", ClampMax="0.95"))
    float PhaseTwoHealthThreshold = 0.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss", meta=(ClampMin="0.0", ClampMax="0.95"))
    float PhaseThreeHealthThreshold = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss", meta=(ClampMin="1"))
    int32 PhaseCount = 2;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss", meta=(ClampMin="0.0"))
    float IntroDurationSeconds = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss", meta=(ClampMin="60.0"))
    float EncounterTimeLimitSeconds = 600.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss")
    bool bLockArena = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss", meta=(ClampMin="1", ClampMax="8"))
    int32 MaxParticipants = 4;
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildBossAttackRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss Attack")
    FName AttackId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss Attack")
    FName EncounterId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss Attack")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss Attack")
    int32 PhaseIndex = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss Attack")
    int32 SpeciesAbilityIndex = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss Attack")
    EAstrawildElement Element = EAstrawildElement::Neutral;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss Attack", meta=(ClampMin="0.0"))
    float TelegraphDurationSeconds = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss Attack", meta=(ClampMin="50.0"))
    float TelegraphRadius = 260.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss Attack", meta=(ClampMin="0.1"))
    float CooldownSeconds = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss Attack", meta=(ClampMin="0.1"))
    float DamageMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss Attack")
    bool bIsUltimate = false;
};
