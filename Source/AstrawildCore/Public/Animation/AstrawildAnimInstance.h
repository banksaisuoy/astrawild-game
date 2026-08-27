#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AstrawildTypes.h"
#include "Characters/AstrawildCharacter.h"
#include "AstrawildAnimInstance.generated.h"

class AAstrawildEchoBase;
class UAstrawildMechaComponent;

UCLASS(Blueprintable)
class ASTRAWILDCORE_API UAstrawildAnimInstance : public UAnimInstance
{
    GENERATED_BODY()

public:
    virtual void NativeInitializeAnimation() override;
    virtual void NativeUpdateAnimation(float DeltaSeconds) override;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Animation")
    float GroundSpeed = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Animation")
    float Direction = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Animation")
    bool bIsInAir = false;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Animation")
    bool bIsSprinting = false;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Animation")
    bool bIsDodging = false;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Animation")
    EAstrawildMovementState PlayerMovementState = EAstrawildMovementState::Idle;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Animation")
    EAstrawildEchoState EchoState = EAstrawildEchoState::WildPassive;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Animation")
    EAstrawildElement ElementalAffinity = EAstrawildElement::Neutral;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Animation")
    float HealthNormalized = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Animation|Mecha")
    bool bIsMechaActive = false;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Animation|Mecha")
    bool bIsMechaFlying = false;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Animation|Mecha")
    bool bIsMechaOverboosting = false;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Animation|Mecha")
    bool bIsMechaOverheated = false;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Animation|Mecha")
    float MechaEnergyNormalized = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Animation|Mecha")
    float MechaHeatNormalized = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Animation|Mecha")
    float MechaShieldNormalized = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Animation|Mecha")
    FGameplayTag MechaEquippedWeaponTag;

private:
    TWeakObjectPtr<APawn> CachedPawn;
    TWeakObjectPtr<AAstrawildCharacter> CachedPlayer;
    TWeakObjectPtr<AAstrawildEchoBase> CachedEcho;
};
