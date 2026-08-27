#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/AstrawildMechaVFXData.h"
#include "AstrawildMechaVFXComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnAstrawildMechaVFXRequestedSignature, FGameplayTag, EffectTag, FVector, Start, FVector, End, float, Intensity, FSoftObjectPath, NiagaraSystemPath);

UCLASS(ClassGroup=(Astrawild), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildMechaVFXComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildMechaVFXComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Mecha|VFX")
    TObjectPtr<UDataTable> VFXTable;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Mecha|VFX|Events")
    FOnAstrawildMechaVFXRequestedSignature OnVFXRequested;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Mecha|VFX")
    bool RequestEffect(FGameplayTag EffectTag, FVector Start, FVector End, float Intensity = 1.0f);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Mecha|VFX")
    bool GetEffectBinding(FGameplayTag EffectTag, FAstrawildMechaVFXBindingRow& OutBinding) const;

private:
    const FAstrawildMechaVFXBindingRow* FindBinding(FGameplayTag EffectTag) const;
};
