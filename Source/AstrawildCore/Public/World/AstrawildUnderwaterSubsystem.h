#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/AstrawildUnderwaterData.h"
#include "AstrawildUnderwaterSubsystem.generated.h"

UENUM(BlueprintType)
enum class EAstrawildUnderwaterMovementMode : uint8
{
    Surface,
    Swimming,
    Diving,
    PressureEmergency
};

USTRUCT(BlueprintType)
struct ASTRAWILDCORE_API FAstrawildUnderwaterState
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category="Underwater")
    EAstrawildUnderwaterMovementMode MovementMode = EAstrawildUnderwaterMovementMode::Surface;

    UPROPERTY(BlueprintReadOnly, Category="Underwater")
    float DepthMeters = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Underwater")
    float OxygenRemainingSeconds = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Underwater")
    float OxygenNormalized = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Underwater")
    float PressureDamagePerSecond = 0.0f;

    UPROPERTY(BlueprintReadOnly, Category="Underwater")
    float BuoyancyMultiplier = 1.0f;

    UPROPERTY(BlueprintReadOnly, Category="Underwater")
    bool bIsSubmerged = false;

    UPROPERTY(BlueprintReadOnly, Category="Underwater")
    bool bHasPressureProtection = false;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAstrawildUnderwaterStateChangedSignature, const FAstrawildUnderwaterState&, State);

UCLASS()
class ASTRAWILDCORE_API UAstrawildUnderwaterSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UAstrawildUnderwaterSubsystem();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Underwater|Defaults", meta=(ClampMin="0.0"))
    float OxygenTankCapacitySeconds = 300.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Underwater|Defaults", meta=(ClampMin="0.0"))
    float SurfaceDepthMeters = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Underwater|Defaults", meta=(ClampMin="0.0"))
    float AbyssalTrenchMinDepthMeters = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Underwater|Defaults", meta=(ClampMin="0.0"))
    float AbyssalTrenchMaxDepthMeters = 1000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Underwater|Defaults", meta=(ClampMin="0.0"))
    float PressureResistanceDepthMeters = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Underwater|Defaults", meta=(ClampMin="0.0"))
    float BasePressureDamagePerSecond = 5.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Underwater|Defaults", meta=(ClampMin="0.0"))
    float BaseOxygenDrainPerSecond = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Underwater|Defaults", meta=(ClampMin="0.0"))
    float SurfaceOxygenRefillPerSecond = 20.0f;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Underwater|Events")
    FOnAstrawildUnderwaterStateChangedSignature OnUnderwaterStateChanged;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Underwater")
    bool IsAbyssalTrenchDepth(float DepthMeters) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Underwater")
    float CalculatePressureDamagePerSecond(float DepthMeters, bool bHasPressureProtection) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Underwater")
    float CalculateOxygenDrainPerSecond(float DepthMeters, bool bHasPressureProtection) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Underwater")
    float CalculateBuoyancyMultiplier(float DepthMeters) const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Underwater")
    FAstrawildUnderwaterState EvaluateDiverState(float DepthMeters, float OxygenRemainingSeconds, bool bHasPressureProtection, float DeltaSeconds) const;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Underwater")
    void SetActiveZoneTable(UDataTable* InZoneTable);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Underwater")
    bool GetActiveZoneRow(FName RowName, FAstrawildUnderwaterZoneRow& OutRow) const;

private:
    UPROPERTY(Transient)
    TObjectPtr<UDataTable> ActiveZoneTable = nullptr;
};
