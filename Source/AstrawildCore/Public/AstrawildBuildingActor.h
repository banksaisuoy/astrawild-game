#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildTypes.h"
#include "AstrawildInteractable.h"
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
class ASTRAWILDCORE_API AAstrawildBuildingActor : public AActor, public IAstrawildInteractable
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

    /**
     * Batch 2 — Item C: most-recent resolved power state for this building. Written
     * by PowerSubsystem::ResolveGrid (server) and replicated so clients see correct
     * lamp/visual state. Captured at save time in ToSaveData and restored at load
     * time in FromSaveData before the first natural Tick re-resolves it.
     */
    UPROPERTY(BlueprintReadOnly, Category="ASTRAWILD|Building", Replicated)
    bool bIsPowered = false;

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

    /** IAstrawildInteractable (audit C-2): Research Desk spends points on the next tech. */
    virtual void Interact_Implementation(AActor* InteractingActor) override;
    virtual FText GetInteractionPrompt_Implementation() const override;

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
