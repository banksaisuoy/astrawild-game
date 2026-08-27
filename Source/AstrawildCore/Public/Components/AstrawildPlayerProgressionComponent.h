#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/AstrawildPlayerProgressionData.h"
#include "AstrawildPlayerProgressionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAstrawildPlayerLevelChangedSignature, int32, NewLevel, int32, UnspentStatPoints);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAstrawildPlayerPerkUnlockedSignature, FGameplayTag, PerkTag);

UCLASS(ClassGroup=(Astrawild), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildPlayerProgressionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildPlayerProgressionComponent();

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Progression")
    TObjectPtr<UDataTable> PerkTable;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Progression")
    int32 Level = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Progression")
    int32 UnspentStatPoints = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Progression")
    float Experience = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Progression", meta=(ClampMin="1"))
    int32 MaxLevel = 100;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Progression", meta=(ClampMin="1.0"))
    float ExperienceForFirstLevel = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Progression|Stats")
    TMap<EAstrawildPlayerStat, int32> AllocatedStatPoints;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Progression|Perks")
    TArray<FGameplayTag> UnlockedPerkTags;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Progression|Events")
    FOnAstrawildPlayerLevelChangedSignature OnLevelChanged;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Progression|Events")
    FOnAstrawildPlayerPerkUnlockedSignature OnPerkUnlocked;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Progression")
    int32 AddExperience(float ExperienceAmount);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Progression")
    bool AllocateStatPoint(EAstrawildPlayerStat StatType);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Progression")
    bool UnlockPerk(FGameplayTag PerkTag);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Progression")
    bool IsPerkUnlocked(FGameplayTag PerkTag) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Progression")
    int32 GetAllocatedStatPoints(EAstrawildPlayerStat StatType) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Progression")
    float GetPerkModifier(FGameplayTag PerkTag, FName ModifierName) const;

private:
    const FAstrawildPlayerPerkRow* FindPerk(FGameplayTag PerkTag) const;
    bool HasAuthorityForProgression() const;
};
