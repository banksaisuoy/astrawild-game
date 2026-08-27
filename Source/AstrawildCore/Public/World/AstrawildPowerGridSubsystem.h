#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Data/AstrawildPowerGridData.h"
#include "AstrawildPowerGridSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAstrawildPowerGridStateChangedSignature, float, AvailableWatts, float, BatteryChargeWattHours);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAstrawildPowerConsumerStateChangedSignature, FGameplayTag, ConsumerTag);

UCLASS()
class ASTRAWILDCORE_API UAstrawildPowerGridSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Power Grid")
    TObjectPtr<UDataTable> PowerNodeTable;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Power Grid")
    TMap<FGameplayTag, float> GeneratorOutputWatts;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Power Grid")
    TMap<FGameplayTag, float> ConsumerDemandWatts;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Power Grid")
    TMap<FGameplayTag, bool> ConsumerPowered;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Power Grid")
    float BatteryChargeWattHours = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Power Grid", meta=(ClampMin="0.0"))
    float BatteryCapacityWattHours = 10000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Power Grid", meta=(ClampMin="0.0"))
    float BatteryChargeEfficiency = 0.92f;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Power Grid|Events")
    FOnAstrawildPowerGridStateChangedSignature OnPowerGridStateChanged;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Power Grid|Events")
    FOnAstrawildPowerConsumerStateChangedSignature OnConsumerStateChanged;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Power Grid")
    bool RegisterGenerator(FGameplayTag GeneratorTag, float OutputWatts);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Power Grid")
    bool RegisterConsumer(FGameplayTag ConsumerTag, float DemandWatts, float Priority = 1.0f);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Power Grid")
    bool UnregisterNode(FGameplayTag NodeTag);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Power Grid")
    void SimulatePowerStep(float DeltaSeconds);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Power Grid")
    float GetAvailableGenerationWatts() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Power Grid")
    float GetNetPowerWatts() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Power Grid")
    bool IsConsumerPowered(FGameplayTag ConsumerTag) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Power Grid")
    float GetBatteryNormalized() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Power Grid")
    bool IsDaytime() const;

private:
    TMap<FGameplayTag, float> ConsumerPriorities;
    FTimerHandle SimulationTimerHandle;
    void HandleSimulationTick();
    void RebuildDefaultsFromTable();
    bool HasAuthorityForGrid() const;
};
