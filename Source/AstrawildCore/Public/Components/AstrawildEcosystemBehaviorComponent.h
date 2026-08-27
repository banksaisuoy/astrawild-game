#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/AstrawildEcosystemData.h"
#include "AstrawildEcosystemBehaviorComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAstrawildEcosystemStateChangedSignature, EAstrawildEcosystemState, NewState);

UCLASS(ClassGroup=(Astrawild), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildEcosystemBehaviorComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildEcosystemBehaviorComponent();

protected:
    virtual void BeginPlay() override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Ecosystem")
    TObjectPtr<UDataTable> BehaviorTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Ecosystem")
    FGameplayTag SpeciesTag;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Ecosystem")
    EAstrawildEcosystemState CurrentState = EAstrawildEcosystemState::Roam;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Ecosystem")
    float CurrentHealthNormalized = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Ecosystem")
    float CurrentFear = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Ecosystem")
    float HungerNormalized = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Ecosystem")
    FGameplayTag CurrentWorldEventTag;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Ecosystem|Events")
    FOnAstrawildEcosystemStateChangedSignature OnStateChanged;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Ecosystem")
    void SetPerceptionState(float HealthNormalized, float FearNormalized, float HungerNormalized, bool bThreatNearby);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Ecosystem")
    void SetWorldEvent(FGameplayTag EventTag, bool bMigrationActive);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Ecosystem")
    void ForceState(EAstrawildEcosystemState NewState);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Ecosystem")
    bool IsInCombatRelevantState() const;

private:
    float StateChangeCooldownRemaining = 0.0f;
    const FAstrawildEcosystemBehaviorRow* FindBehavior() const;
    void EvaluateState(bool bThreatNearby);
    void SetState(EAstrawildEcosystemState NewState);
    bool HasAuthorityForEcosystem() const;
};
