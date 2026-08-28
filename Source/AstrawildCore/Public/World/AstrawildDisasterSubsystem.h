#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/AstrawildDisasterData.h"
#include "AstrawildDisasterSubsystem.generated.h"

class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAstrawildDisasterStartedSignature, const FAstrawildDisasterState&, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAstrawildDisasterEndedSignature, FGameplayTag, DisasterTag);

UCLASS()
class ASTRAWILDCORE_API UAstrawildDisasterSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    UAstrawildDisasterSubsystem();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Disaster|Rules", meta=(ClampMin="0.1"))
    float TickIntervalSeconds = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Disaster|Rules", meta=(ClampMin="5.0"))
    float MinimumRandomIntervalSeconds = 60.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Disaster|Rules", meta=(ClampMin="5.0"))
    float MaximumRandomIntervalSeconds = 180.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Disaster|Rules")
    int32 RandomSeed = 19860417;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Disaster|Events")
    FOnAstrawildDisasterStartedSignature OnDisasterStarted;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Disaster|Events")
    FOnAstrawildDisasterEndedSignature OnDisasterEnded;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Disaster|Authority")
    bool RegisterDisasterDefinition(const FAstrawildDisasterDefinition& Definition);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Disaster|Authority")
    bool StartDisaster(const FGameplayTag& DisasterTag);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Disaster|Authority")
    bool StopDisaster(const FGameplayTag& DisasterTag);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Disaster|Authority")
    bool StartRandomDisaster(const FGameplayTag& BiomeTag);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Disaster|Authority")
    void AdvanceDisasters(float DeltaSeconds);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Disaster")
    bool GetActiveDisaster(FAstrawildDisasterState& OutState) const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Disaster")
    bool IsDisasterActive(const FGameplayTag& DisasterTag) const;

private:
    TMap<FGameplayTag, FAstrawildDisasterDefinition> Definitions;
    TMap<FGameplayTag, FAstrawildDisasterState> ActiveDisasters;
    TMap<FGameplayTag, float> Cooldowns;
    FRandomStream RandomStream;
    FTimerHandle DisasterTimerHandle;

    bool HasAuthorityForDisaster() const;
    void HandleDisasterTick();
    void ScheduleNextRandomEvent();
};
