#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/AstrawildSpaceFlightData.h"
#include "AstrawildSpaceFlightSubsystem.generated.h"

class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAstrawildLaunchStartedSignature, AActor*, Pilot, FGameplayTag, DestinationBiomeTag);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAstrawildFlightStateChangedSignature, AActor*, Pilot, EAstrawildFlightState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAstrawildVacuumEmergencySignature, AActor*, Pilot);

UCLASS()
class ASTRAWILDCORE_API UAstrawildSpaceFlightSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UAstrawildSpaceFlightSubsystem();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Space Flight|Rules", meta=(ClampMin="0.01"))
    float TickIntervalSeconds = 0.05f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Space Flight|Rules", meta=(ClampMin="0.0"))
    float LowGravityScale = 0.16f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Space Flight|Rules", meta=(ClampMin="0.0"))
    float VacuumPressureLossKPaPerSecond = 18.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Space Flight|Rules", meta=(ClampMin="0.0"))
    float MinimumSafeCabinPressureKPa = 40.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|Space Flight|State")
    int32 RegisteredLaunchPadCount = 0;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Space Flight|Events")
    FOnAstrawildLaunchStartedSignature OnLaunchStarted;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Space Flight|Events")
    FOnAstrawildFlightStateChangedSignature OnFlightStateChanged;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Space Flight|Events")
    FOnAstrawildVacuumEmergencySignature OnVacuumEmergency;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Space Flight|Authority")
    bool RegisterLaunchPad(const FAstrawildLaunchPadDefinition& Definition);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Space Flight|Authority")
    bool UnregisterLaunchPad(const FGameplayTag& PadTag);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Space Flight|Authority")
    bool RequestLaunch(AActor* Pilot, const FGameplayTag& PadTag);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Space Flight|Authority")
    bool UpdateFlightInput(AActor* Pilot, FVector InputAcceleration, float DeltaSeconds);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Space Flight|Authority")
    bool ReturnToSurface(AActor* Pilot);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Space Flight|Authority")
    void AdvanceFlight(float DeltaSeconds);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Space Flight")
    bool GetFlightState(AActor* Pilot, FAstrawildSpaceFlightState& OutState) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Space Flight")
    bool IsLaunchPadRegistered(const FGameplayTag& PadTag) const;

private:
    TMap<FGameplayTag, FAstrawildLaunchPadDefinition> LaunchPads;
    TMap<TWeakObjectPtr<AActor>, FAstrawildSpaceFlightState> PilotStates;
    TMap<TWeakObjectPtr<AActor>, float> LaunchTimers;
    FTimerHandle FlightTimerHandle;

    bool HasAuthorityForFlight() const;
    void HandleFlightTick();
    void RemoveInvalidPilots();
    void SetPilotState(AActor* Pilot, EAstrawildFlightState NewState);
};
