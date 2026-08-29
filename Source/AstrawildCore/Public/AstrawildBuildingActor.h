#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildTypes.h"
#include "AstrawildBuildingActor.generated.h"

class UAstrawildBuildingDefinition;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAstrawildBuildingDamaged, class AAstrawildBuildingActor*, Building);

/**
 * A placed modular building piece (directive §16). Server-authoritative:
 * placement/rotation/removal flow through server RPCs from the placement component.
 * Power nodes register with the power subsystem automatically.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildBuildingActor : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildBuildingActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Building")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Building")
    FAstrawildBuildingDamaged OnBuildingDamaged;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Building")
    FGuid BuildingId;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Building")
    FName DefinitionId = NAME_None;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Building", Replicated)
    bool bIsSwitchedOn = true;

    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Building")
    FName OwnerPlayerId = NAME_None;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Building")
    bool InitializeFromDefinition(const UAstrawildBuildingDefinition* Definition, FName InOwnerPlayerId);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Building")
    const UAstrawildBuildingDefinition* GetBuildingDefinition() const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Building")
    void ApplyBuildingDamage(float DamageAmount);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Building")
    void SetSwitchedOn(bool bOn);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Building")
    float GetHealthFraction() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Building")
    bool IsDestroyed() const { return CurrentHealth <= 0.0f; }

    FAstrawildBuildingSaveData ToSaveData() const;
    bool FromSaveData(const FAstrawildBuildingSaveData& Data);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
    UPROPERTY(Replicated)
    float CurrentHealth = 100.0f;

    UPROPERTY()
    float MaxHealth = 100.0f;

    UPROPERTY(Replicated)
    float StoredCharge = 0.0f;

    void RegisterPower();
};
