#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildDamageTarget.generated.h"

class AAstrawildDamageTarget;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAstrawildDamageReceived, AAstrawildDamageTarget*, Target, float, DamageAmount, float, RemainingHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildTargetDefeated, AAstrawildDamageTarget*, Target);

UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildDamageTarget : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildDamageTarget();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Combat")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Combat", meta=(ClampMin="1.0"))
    float MaxHealth = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Combat", meta=(ClampMin="0.0"))
    float CurrentHealth = 100.0f;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Combat")
    FAstrawildDamageReceived OnDamageReceived;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Combat")
    FAstrawildTargetDefeated OnDefeated;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Combat")
    bool ApplyDamage(float DamageAmount);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Combat")
    void ResetTarget();

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Combat")
    bool IsDefeated() const { return CurrentHealth <= 0.0f; }

protected:
    virtual void BeginPlay() override;
};
