#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AstrawildTypes.h"
#include "AstrawildWorkSiteActor.generated.h"

class AAstrawildWorkSiteActor;
class AAstrawildEchoCharacter;
class UStaticMeshComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FAstrawildWorkProduced, AAstrawildWorkSiteActor*, Site, FName, ItemId, int32, Quantity);

/**
 * A base work site (directive §18): Echoes assigned here produce items over time.
 * Rate scales with species work affinity, personality and needs. Sites powered by
 * the power grid (workstations) produce faster.
 */
UCLASS(Blueprintable)
class ASTRAWILDCORE_API AAstrawildWorkSiteActor : public AActor
{
    GENERATED_BODY()

public:
    AAstrawildWorkSiteActor();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Work")
    TObjectPtr<UStaticMeshComponent> VisualMesh;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Work")
    FAstrawildWorkProduced OnWorkProduced;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Work")
    EAstrawildWorkType WorkType = EAstrawildWorkType::Gathering;

    /** Working range for assigned Echoes. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Work", meta=(ClampMin="100.0"))
    float WorkRange = 250.0f;

    /** Item produced by this site. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Work")
    FName OutputItemId = NAME_None;

    /** Seconds of effective work per produced item (base). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Work", meta=(ClampMin="0.1"))
    float SecondsPerOutput = 12.0f;

    /** Requires power from the grid to run at full speed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Work")
    bool bRequiresPower = false;

    virtual void Tick(float DeltaTime) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** Items accumulated and waiting for pickup. */
    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Work")
    int32 GetStoredOutput() const { return StoredOutput; }

    /** Player collects the accumulated output. */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Work")
    int32 CollectOutput();

    /** Attach an Echo worker (server). */
    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Work")
    bool AssignWorker(AAstrawildEchoCharacter* Echo);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Work")
    void RemoveWorker(AAstrawildEchoCharacter* Echo);

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(Replicated)
    int32 StoredOutput = 0;

    float WorkAccumulator = 0.0f;

    TArray<TWeakObjectPtr<AAstrawildEchoCharacter>> Workers;

    bool IsPowered() const;
    class UAstrawildPowerSubsystem* GetPower() const;
};
