#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AstrawildTypes.h"
#include "AstrawildEchoCharacter.generated.h"

class UAstrawildEchoDefinition;
class AAstrawildEchoCharacter;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildEchoCaptured, AAstrawildEchoCharacter*, Echo);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAstrawildEchoDamaged, AAstrawildEchoCharacter*, Echo, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildEchoDefeated, AAstrawildEchoCharacter*, Echo);

UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildEchoCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AAstrawildEchoCharacter();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Echo")
    TObjectPtr<UStaticMeshComponent> PlaceholderMesh;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Echo")
    FAstrawildEchoCaptured OnCaptured;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Echo")
    FAstrawildEchoDamaged OnDamaged;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Echo")
    FAstrawildEchoDefeated OnDefeated;

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="ASTRAWILD|Echo")
    TObjectPtr<UAstrawildEchoDefinition> EchoDefinition;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Echo", meta=(ClampMin="1"))
    int32 Level = 1;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Echo")
    FGuid InstanceId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Echo", meta=(ClampMin="0.0"))
    float Trust = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Echo")
    float CurrentHealth = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Echo")
    bool bCaptured = false;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo")
    bool InitializeFromDefinition(UAstrawildEchoDefinition* InDefinition, const FGuid& OptionalInstanceId = FGuid());

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo")
    bool ApplyDamage(float DamageAmount);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo")
    bool Capture(float InitialTrust = 0.0f);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Echo")
    void AddTrust(float Amount);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo")
    bool IsDefeated() const { return CurrentHealth <= 0.0f; }

    /** Health fraction in 0..1 range. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo")
    float GetHealthFraction() const;

    /**
     * Capture chance in 0..1 following the design rule:
     * capture succeeds by weakening the Echo first or by building trust,
     * never at full health with zero trust, and never once defeated.
     */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo")
    float ComputeCaptureChance() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Echo")
    FAstrawildEchoInstanceSaveData ToSaveData() const;

protected:
    virtual void BeginPlay() override;

private:
    FAstrawildEchoStats CachedStats;
};
