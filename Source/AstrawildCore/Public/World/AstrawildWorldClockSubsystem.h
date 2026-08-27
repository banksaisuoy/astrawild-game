#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AstrawildWorldClockSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAstrawildTimeOfDayChangedSignature, float, NormalizedTime, bool, bIsNight);

UCLASS()
class ASTRAWILDCORE_API UAstrawildWorldClockSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UAstrawildWorldClockSubsystem();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|World Time", meta=(ClampMin="60.0"))
    float DayLengthSeconds = 1200.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|World Time")
    float WorldTimeSeconds = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ASTRAWILD|World Time")
    int32 DayIndex = 0;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|World Time|Events")
    FOnAstrawildTimeOfDayChangedSignature OnTimeOfDayChanged;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|World Time")
    void AdvanceTime(float DeltaSeconds);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|World Time")
    void SetWorldTime(float NewWorldTimeSeconds);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World Time")
    float GetNormalizedTime() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|World Time")
    bool IsNight() const;

private:
    FTimerHandle TimeTickHandle;
    bool bLastNightState = false;
    void HandleTimeTick();
    void BroadcastTimeStateIfChanged(bool bForce = false);
};
