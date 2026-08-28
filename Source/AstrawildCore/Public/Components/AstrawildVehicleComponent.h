#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/AstrawildVehicleData.h"
#include "AstrawildVehicleComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAstrawildVehicleStateChangedSignature, const FAstrawildVehicleRuntimeState&, State);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAstrawildVehiclePartChangedSignature, EAstrawildVehicleSlot, Slot, FGameplayTag, PartTag);

UCLASS(ClassGroup=(Astrawild), meta=(BlueprintSpawnableComponent))
class ASTRAWILDCORE_API UAstrawildVehicleComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAstrawildVehicleComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Vehicle|Setup")
    FGameplayTag VehicleTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Vehicle|Setup")
    bool bIsHoverbike = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Vehicle|Setup")
    bool bIsSubmersible = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Vehicle|Tuning", meta=(ClampMin="1.0"))
    float MaxSpeedCentimetersPerSecond = 2800.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Vehicle|Tuning", meta=(ClampMin="0.0"))
    float BoostSpeedCentimetersPerSecond = 4500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Vehicle|Tuning", meta=(ClampMin="0.0"))
    float FuelConsumptionPerSecond = 0.02f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Vehicle|Tuning", meta=(ClampMin="0.01"))
    float BoostDurationSeconds = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Vehicle|Tuning", meta=(ClampMin="0.0"))
    float HoverHeightCentimeters = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ASTRAWILD|Vehicle|Tuning", meta=(ClampMin="0.0"))
    float MaxDepthMeters = 1200.0f;

    UPROPERTY(ReplicatedUsing=OnRepRuntimeState, BlueprintReadOnly, Category="ASTRAWILD|Vehicle|State")
    FAstrawildVehicleRuntimeState RuntimeState;

    UPROPERTY(Replicated, BlueprintReadOnly, Category="ASTRAWILD|Vehicle|State")
    TArray<FAstrawildVehicleInstalledPart> InstalledParts;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Vehicle|Events")
    FOnAstrawildVehicleStateChangedSignature OnVehicleStateChanged;

    UPROPERTY(BlueprintAssignable, Category="ASTRAWILD|Vehicle|Events")
    FOnAstrawildVehiclePartChangedSignature OnVehiclePartChanged;

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Vehicle|Authority")
    bool EnterVehicle(AActor* NewDriver);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Vehicle|Authority")
    bool ExitVehicle(AActor* ExpectedDriver);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Vehicle|Authority")
    bool SetDriver(AActor* NewDriver);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Vehicle|Authority")
    bool ClearDriver(AActor* ExpectedDriver);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Vehicle|Authority")
    bool InstallPart(EAstrawildVehicleSlot Slot, const FGameplayTag& PartTag);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Vehicle|Authority")
    bool RemovePart(EAstrawildVehicleSlot Slot);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Vehicle|Authority")
    bool ApplyControlInput(const FAstrawildVehicleControlInput& Input);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Vehicle|Authority")
    bool ApplyThrottle(float Throttle);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Vehicle|Authority")
    bool ActivateNitroBoost(bool bActivate);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Vehicle|Authority")
    bool FireVehicleWeapon(bool bSecondaryWeapon);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Vehicle|Authority")
    void ApplyDamage(float Damage);

    UFUNCTION(BlueprintCallable, Category="ASTRAWILD|Vehicle|Authority")
    void Repair(float AmountNormalized);

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Vehicle")
    bool IsOperational() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Vehicle")
    float GetFuelPercent() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Vehicle")
    float GetHealthPercent() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Vehicle")
    float GetEffectiveMaxSpeed() const;

    UFUNCTION(BlueprintPure, Category="ASTRAWILD|Vehicle")
    float GetEffectiveBoostSpeed() const;

protected:
    UFUNCTION()
    void OnRepRuntimeState();

    UFUNCTION(Server, Reliable)
    void ServerSetDriver(AActor* NewDriver);

    UFUNCTION(Server, Reliable)
    void ServerClearDriver(AActor* ExpectedDriver);

    UFUNCTION(Server, Reliable)
    void ServerInstallPart(EAstrawildVehicleSlot Slot, FGameplayTag PartTag);

    UFUNCTION(Server, Reliable)
    void ServerRemovePart(EAstrawildVehicleSlot Slot);

    UFUNCTION(Server, Reliable)
    void ServerApplyControlInput(FAstrawildVehicleControlInput Input);

private:
    FAstrawildVehicleControlInput PendingInput;
    float CurrentFuel = 1.0f;
    float CurrentDurability = 1.0f;

    bool HasAuthorityForVehicle() const;
    bool IsAutonomousVehicleOwner() const;
    void SetDriverAuthority(AActor* NewDriver);
    void ClearDriverAuthority(AActor* ExpectedDriver);
    void InstallPartAuthority(EAstrawildVehicleSlot Slot, const FGameplayTag& PartTag);
    void RemovePartAuthority(EAstrawildVehicleSlot Slot);
    void ApplyControlInputAuthority(const FAstrawildVehicleControlInput& Input);
    void SimulateVehicle(float DeltaTime);
    void BroadcastState();
};
