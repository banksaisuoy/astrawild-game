#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/AstrawildEvolutionData.h"
#include "AstrawildEvolutionComponent.generated.h"

class AAstrawildEchoBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnEchoEvolvedSignature, AActor*, Echo, FGameplayTag, SourceSpeciesTag, FGameplayTag, TargetSpeciesTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEvolutionFailedSignature, AActor*, Echo, const FText&, FailureReason);

UCLASS(ClassGroup=(Astrawild), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildEvolutionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildEvolutionComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Evolution")
    TObjectPtr<UDataTable> EvolutionTable;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Evolution|Events")
    FOnEchoEvolvedSignature OnEchoEvolved;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Evolution|Events")
    FOnEvolutionFailedSignature OnEvolutionFailed;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Evolution")
    bool CanEvolve(AActor* Trainer, FText& OutFailureReason) const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Evolution")
    bool Evolve(AActor* Trainer);

private:
    const FAstrawildEvolutionRow* FindEvolutionRow(const AAstrawildEchoBase* Echo) const;
};
