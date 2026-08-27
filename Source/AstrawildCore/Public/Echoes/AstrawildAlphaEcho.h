#pragma once

#include "CoreMinimal.h"
#include "Echoes/AstrawildEchoBase.h"
#include "AstrawildAlphaEcho.generated.h"

UENUM(BlueprintType)
enum class EAstrawildBossPhase : uint8
{
    PhaseOne UMETA(DisplayName="Alpha Phase One"),
    PhaseTwo UMETA(DisplayName="Alpha Phase Two")
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildBossAttackPattern
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss Attack")
    FName PatternId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss Attack")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss Attack")
    int32 SpeciesAbilityIndex = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss Attack")
    EAstrawildElement Element = EAstrawildElement::Solar;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss Attack", meta=(ClampMin="0.0"))
    float TelegraphDuration = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss Attack", meta=(ClampMin="0.0"))
    float Cooldown = 4.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Boss Attack", meta=(ClampMin="0.1"))
    float DamageMultiplier = 1.0f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAlphaPhaseChangedSignature, EAstrawildBossPhase, NewPhase, float, HealthNormalized);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAlphaAttackTelegraphSignature, FName, PatternId);

UCLASS()
class ASTRAWILDCORE_API AAstrawildAlphaEcho : public AAstrawildEchoBase
{
    GENERATED_BODY()

public:
    AAstrawildAlphaEcho();

protected:
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alpha Echo|Encounter", meta=(ClampMin="0.05", ClampMax="0.95"))
    float PhaseTwoHealthThreshold = 0.5f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Alpha Echo|Encounter")
    EAstrawildBossPhase CurrentBossPhase = EAstrawildBossPhase::PhaseOne;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Alpha Echo|Encounter")
    bool bIsEncounterActive = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alpha Echo|Attacks")
    TArray<FAstrawildBossAttackPattern> PhaseOnePatterns;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Alpha Echo|Attacks")
    TArray<FAstrawildBossAttackPattern> PhaseTwoPatterns;

    UPROPERTY(BlueprintAssignable, Category="Alpha Echo|Events")
    FOnAlphaPhaseChangedSignature OnPhaseChanged;

    UPROPERTY(BlueprintAssignable, Category="Alpha Echo|Events")
    FOnAlphaAttackTelegraphSignature OnAttackTelegraph;

    UFUNCTION(BlueprintCallable, Category="Alpha Echo|Encounter")
    void StartEncounter();

    UFUNCTION(BlueprintCallable, Category="Alpha Echo|Encounter")
    void StopEncounter();

    UFUNCTION(BlueprintPure, Category="Alpha Echo|Encounter")
    float GetHealthNormalized() const;

    UFUNCTION(BlueprintPure, Category="Alpha Echo|Encounter")
    bool IsPhaseTwo() const { return CurrentBossPhase == EAstrawildBossPhase::PhaseTwo; }

    UFUNCTION(BlueprintCallable, Category="Alpha Echo|Attacks")
    bool ExecuteAttackPattern(int32 PatternIndex);

    UFUNCTION(BlueprintCallable, Category="Alpha Echo|Encounter")
    void EvaluatePhaseTransition();

private:
    TMap<FName, double> PatternCooldownEndTimes;
    const TArray<FAstrawildBossAttackPattern>& GetActivePatterns() const;
    void EnterPhaseTwo();
};
