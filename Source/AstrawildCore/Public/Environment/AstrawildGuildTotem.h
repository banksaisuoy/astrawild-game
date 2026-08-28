#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "Environment/AstrawildGuildTotem.generated.h"

UCLASS()
class ASTRAWILDCORE_API AAstrawildGuildTotem : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildGuildTotem();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Guild")
    FGameplayTag TotemTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Guild")
    FGameplayTag InitialGuildTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Guild", meta=(ClampMin="1.0"))
    float TerritoryRadius = 1500.0f;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Guild")
    bool CaptureForGuild(const FGameplayTag& GuildTag, AActor* Instigator);
};
