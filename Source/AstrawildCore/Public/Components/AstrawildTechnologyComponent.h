#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/AstrawildTechnologyData.h"
#include "AstrawildTechnologyComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTechnologyUnlockedSignature, FGameplayTag, TechnologyTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTechnologyUnlockFailedSignature, const FText&, FailureReason);

UCLASS(ClassGroup=(Astrawild), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildTechnologyComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildTechnologyComponent();

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Technology")
    TObjectPtr<UDataTable> TechnologyTable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Technology", meta=(ClampMin="0"))
    int32 ResearchPoints = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Technology")
    TArray<FGameplayTag> UnlockedTechnologyTags;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Technology|Events")
    FOnTechnologyUnlockedSignature OnTechnologyUnlocked;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Technology|Events")
    FOnTechnologyUnlockFailedSignature OnTechnologyUnlockFailed;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Technology")
    void AddResearchPoints(int32 Amount);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Technology")
    bool IsTechnologyUnlocked(const FGameplayTag& TechnologyTag) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Technology")
    bool CanUnlockTechnology(const FGameplayTag& TechnologyTag, FText& OutFailureReason) const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Technology")
    bool TryUnlockTechnology(const FGameplayTag& TechnologyTag);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Technology")
    void LoadTechnologyState(const TArray<FGameplayTag>& InUnlockedTags, int32 InResearchPoints);

private:
    const FAstrawildTechnologyNodeRow* FindTechnologyRow(const FGameplayTag& TechnologyTag) const;
};
