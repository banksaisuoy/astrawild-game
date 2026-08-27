#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AstrawildTypes.h"
#include "AstrawildBreedingComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEchoEggCreatedSignature, const FAstrawildEchoEggData&, EggData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEchoEggHatchedSignature, const FAstrawildEchoEggData&, EggData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEchoBreedingFailedSignature, const FText&, FailureReason);

UCLASS(ClassGroup=(Astrawild), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildBreedingComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildBreedingComponent();

protected:
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Breeding")
    TArray<FAstrawildEchoEggData> IncubatingEggs;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Breeding", meta=(ClampMin="1.0"))
    float DefaultIncubationDurationSeconds = 900.0f;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Breeding|Events")
    FOnEchoEggCreatedSignature OnEggCreated;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Breeding|Events")
    FOnEchoEggHatchedSignature OnEggHatched;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Breeding|Events")
    FOnEchoBreedingFailedSignature OnBreedingFailed;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Breeding")
    bool CanBreed(const FAstrawildCapturedEchoData& ParentA, const FAstrawildCapturedEchoData& ParentB, FText& OutFailureReason) const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Breeding")
    bool TryBreed(const FAstrawildCapturedEchoData& ParentA, const FAstrawildCapturedEchoData& ParentB, const FGameplayTag& OffspringSpeciesTag, FAstrawildEchoEggData& OutEgg);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Breeding")
    void AdvanceIncubation(float DeltaSeconds);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Breeding")
    void LoadIncubatingEggs(const TArray<FAstrawildEchoEggData>& InEggs);

private:
    static void AddUniqueAffinity(TArray<EAstrawildElement>& Affinities, EAstrawildElement Element);
    static void AddUniqueTraits(FGameplayTagContainer& Traits, const FGameplayTagContainer& Source);
    static void NormalizeEgg(FAstrawildEchoEggData& Egg);
};
