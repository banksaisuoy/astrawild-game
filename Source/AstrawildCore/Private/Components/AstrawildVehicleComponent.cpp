#include "Components/AstrawildVehicleComponent.h"

#include "GameFramework/Actor.h"
#include "Net/UnrealNetwork.h"

UAstrawildVehicleComponent::UAstrawildVehicleComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    SetIsReplicatedByDefault(true);
}

void UAstrawildVehicleComponent::BeginPlay()
{
    Super::BeginPlay();
    CurrentFuel = 1.0f;
    CurrentDurability = 1.0f;
    RuntimeState.FuelNormalized = CurrentFuel;
    RuntimeState.DurabilityNormalized = CurrentDurability;
    RuntimeState.Driver = nullptr;
}

void UAstrawildVehicleComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UAstrawildVehicleComponent, RuntimeState);
    DOREPLIFETIME(UAstrawildVehicleComponent, InstalledParts);
}

void UAstrawildVehicleComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (HasAuthorityForVehicle())
    {
        SimulateVehicle(FMath::Min(DeltaTime, 0.1f));
    }
}

bool UAstrawildVehicleComponent::HasAuthorityForVehicle() const
{
    const AActor* OwnerActor = GetOwner();
    return OwnerActor && OwnerActor->HasAuthority();
}

bool UAstrawildVehicleComponent::IsAutonomousVehicleOwner() const
{
    const AActor* OwnerActor = GetOwner();
    return OwnerActor && OwnerActor->GetLocalRole() == ROLE_AutonomousProxy;
}

bool UAstrawildVehicleComponent::EnterVehicle(AActor* NewDriver)
{
    return SetDriver(NewDriver);
}

bool UAstrawildVehicleComponent::ExitVehicle(AActor* ExpectedDriver)
{
    return ClearDriver(ExpectedDriver);
}

bool UAstrawildVehicleComponent::SetDriver(AActor* NewDriver)
{
    if (!HasAuthorityForVehicle())
    {
        if (IsAutonomousVehicleOwner())
        {
            ServerSetDriver(NewDriver);
        }
        return false;
    }
    SetDriverAuthority(NewDriver);
    return RuntimeState.Driver == NewDriver;
}

bool UAstrawildVehicleComponent::ClearDriver(AActor* ExpectedDriver)
{
    if (!HasAuthorityForVehicle())
    {
        if (IsAutonomousVehicleOwner())
        {
            ServerClearDriver(ExpectedDriver);
        }
        return false;
    }
    ClearDriverAuthority(ExpectedDriver);
    return RuntimeState.Driver == nullptr;
}

bool UAstrawildVehicleComponent::InstallPart(const EAstrawildVehicleSlot Slot, const FGameplayTag& PartTag)
{
    if (!HasAuthorityForVehicle())
    {
        if (IsAutonomousVehicleOwner())
        {
            ServerInstallPart(Slot, PartTag);
        }
        return false;
    }
    InstallPartAuthority(Slot, PartTag);
    return InstalledParts.ContainsByPredicate([Slot, PartTag](const FAstrawildVehicleInstalledPart& Part)
    {
        return Part.Slot == Slot && Part.PartTag == PartTag;
    });
}

bool UAstrawildVehicleComponent::RemovePart(const EAstrawildVehicleSlot Slot)
{
    if (!HasAuthorityForVehicle())
    {
        if (IsAutonomousVehicleOwner())
        {
            ServerRemovePart(Slot);
        }
        return false;
    }
    const int32 Before = InstalledParts.Num();
    RemovePartAuthority(Slot);
    return InstalledParts.Num() < Before;
}

bool UAstrawildVehicleComponent::ApplyThrottle(const float Throttle)
{
    FAstrawildVehicleControlInput Input = PendingInput;
    Input.Throttle = Throttle;
    return ApplyControlInput(Input);
}

bool UAstrawildVehicleComponent::ActivateNitroBoost(const bool bActivate)
{
    FAstrawildVehicleControlInput Input = PendingInput;
    Input.bBoost = bActivate;
    return ApplyControlInput(Input);
}

bool UAstrawildVehicleComponent::FireVehicleWeapon(const bool bSecondaryWeapon)
{
    FAstrawildVehicleControlInput Input = PendingInput;
    if (bSecondaryWeapon)
    {
        Input.bFireSecondary = true;
    }
    else
    {
        Input.bFirePrimary = true;
    }
    return ApplyControlInput(Input);
}

bool UAstrawildVehicleComponent::ApplyControlInput(const FAstrawildVehicleControlInput& Input)
{
    if (!HasAuthorityForVehicle())
    {
        if (IsAutonomousVehicleOwner())
        {
            ServerApplyControlInput(Input);
        }
        return false;
    }
    ApplyControlInputAuthority(Input);
    return true;
}

void UAstrawildVehicleComponent::ApplyDamage(const float Damage)
{
    if (!HasAuthorityForVehicle() || Damage <= 0.0f)
    {
        return;
    }
    CurrentDurability = FMath::Clamp(CurrentDurability - Damage, 0.0f, 1.0f);
    if (CurrentDurability <= 0.0f)
    {
        PendingInput = FAstrawildVehicleControlInput();
        RuntimeState.bEngineActive = false;
        RuntimeState.CurrentSpeedCentimetersPerSecond = 0.0f;
    }
    BroadcastState();
}

void UAstrawildVehicleComponent::Repair(const float AmountNormalized)
{
    if (!HasAuthorityForVehicle() || AmountNormalized <= 0.0f)
    {
        return;
    }
    CurrentDurability = FMath::Clamp(CurrentDurability + AmountNormalized, 0.0f, 1.0f);
    BroadcastState();
}

float UAstrawildVehicleComponent::GetFuelPercent() const
{
    return CurrentFuel * 100.0f;
}

float UAstrawildVehicleComponent::GetHealthPercent() const
{
    return CurrentDurability * 100.0f;
}

bool UAstrawildVehicleComponent::IsOperational() const
{
    return CurrentDurability > 0.0f && CurrentFuel > 0.0f;
}

float UAstrawildVehicleComponent::GetEffectiveMaxSpeed() const
{
    float Multiplier = 1.0f;
    for (const FAstrawildVehicleInstalledPart& Part : InstalledParts)
    {
        if (Part.Slot == EAstrawildVehicleSlot::Engine)
        {
            Multiplier *= FMath::Max(0.01f, Part.SpeedMultiplier);
        }
    }
    return FMath::Max(0.0f, MaxSpeedCentimetersPerSecond * Multiplier);
}

float UAstrawildVehicleComponent::GetEffectiveBoostSpeed() const
{
    float Multiplier = 1.0f;
    for (const FAstrawildVehicleInstalledPart& Part : InstalledParts)
    {
        if (Part.Slot == EAstrawildVehicleSlot::Engine || Part.Slot == EAstrawildVehicleSlot::Utility)
        {
            Multiplier *= FMath::Max(0.01f, Part.BoostMultiplier);
        }
    }
    return FMath::Max(0.0f, BoostSpeedCentimetersPerSecond * Multiplier);
}

void UAstrawildVehicleComponent::OnRepRuntimeState()
{
    OnVehicleStateChanged.Broadcast(RuntimeState);
}

void UAstrawildVehicleComponent::ServerSetDriver_Implementation(AActor* NewDriver)
{
    SetDriverAuthority(NewDriver);
}

void UAstrawildVehicleComponent::ServerClearDriver_Implementation(AActor* ExpectedDriver)
{
    ClearDriverAuthority(ExpectedDriver);
}

void UAstrawildVehicleComponent::ServerInstallPart_Implementation(const EAstrawildVehicleSlot Slot, const FGameplayTag PartTag)
{
    InstallPartAuthority(Slot, PartTag);
}

void UAstrawildVehicleComponent::ServerRemovePart_Implementation(const EAstrawildVehicleSlot Slot)
{
    RemovePartAuthority(Slot);
}

void UAstrawildVehicleComponent::ServerApplyControlInput_Implementation(const FAstrawildVehicleControlInput Input)
{
    ApplyControlInputAuthority(Input);
}

void UAstrawildVehicleComponent::SetDriverAuthority(AActor* NewDriver)
{
    if (!HasAuthorityForVehicle() || !IsValid(NewDriver) || RuntimeState.Driver == NewDriver)
    {
        return;
    }
    if (RuntimeState.Driver != nullptr)
    {
        return;
    }
    RuntimeState.Driver = NewDriver;
    BroadcastState();
}

void UAstrawildVehicleComponent::ClearDriverAuthority(AActor* ExpectedDriver)
{
    if (!HasAuthorityForVehicle() || (ExpectedDriver && RuntimeState.Driver != ExpectedDriver))
    {
        return;
    }
    RuntimeState.Driver = nullptr;
    PendingInput = FAstrawildVehicleControlInput();
    BroadcastState();
}

void UAstrawildVehicleComponent::InstallPartAuthority(const EAstrawildVehicleSlot Slot, const FGameplayTag& PartTag)
{
    if (!HasAuthorityForVehicle() || !PartTag.IsValid())
    {
        return;
    }
    for (const FAstrawildVehicleInstalledPart& Part : InstalledParts)
    {
        if (Part.Slot == Slot && Part.PartTag == PartTag)
        {
            return;
        }
    }
    RemovePartAuthority(Slot);
    FAstrawildVehicleInstalledPart NewPart;
    NewPart.Slot = Slot;
    NewPart.PartTag = PartTag;
    InstalledParts.Add(NewPart);
    OnVehiclePartChanged.Broadcast(Slot, PartTag);
}

void UAstrawildVehicleComponent::RemovePartAuthority(const EAstrawildVehicleSlot Slot)
{
    if (!HasAuthorityForVehicle())
    {
        return;
    }
    const int32 Removed = InstalledParts.RemoveAll([Slot](const FAstrawildVehicleInstalledPart& Part)
    {
        return Part.Slot == Slot;
    });
    if (Removed > 0)
    {
        OnVehiclePartChanged.Broadcast(Slot, FGameplayTag());
    }
}

void UAstrawildVehicleComponent::ApplyControlInputAuthority(const FAstrawildVehicleControlInput& Input)
{
    if (!HasAuthorityForVehicle())
    {
        return;
    }
    PendingInput = Input;
    PendingInput.Throttle = FMath::Clamp(PendingInput.Throttle, -1.0f, 1.0f);
    PendingInput.Steer = FMath::Clamp(PendingInput.Steer, -1.0f, 1.0f);
    PendingInput.Brake = FMath::Clamp(PendingInput.Brake, 0.0f, 1.0f);
    if (CurrentFuel <= 0.0f || CurrentDurability <= 0.0f)
    {
        PendingInput = FAstrawildVehicleControlInput();
    }
}

void UAstrawildVehicleComponent::SimulateVehicle(const float DeltaTime)
{
    if (!HasAuthorityForVehicle() || DeltaTime <= 0.0f)
    {
        return;
    }
    const bool bBoosting = PendingInput.bBoost && PendingInput.Throttle > 0.0f && CurrentFuel > 0.0f;
    const float TargetSpeed = PendingInput.Throttle * (bBoosting ? GetEffectiveBoostSpeed() : GetEffectiveMaxSpeed());
    const float Braking = FMath::Clamp(PendingInput.Brake + (PendingInput.bHandbrake ? 0.35f : 0.0f), 0.0f, 1.0f);
    RuntimeState.CurrentSpeedCentimetersPerSecond = FMath::FInterpTo(RuntimeState.CurrentSpeedCentimetersPerSecond, TargetSpeed * (1.0f - Braking), DeltaTime, 4.0f);
    if (bBoosting)
    {
        RuntimeState.BoostRemainingSeconds = FMath::Max(0.0f, RuntimeState.BoostRemainingSeconds - DeltaTime);
        if (RuntimeState.BoostRemainingSeconds <= 0.0f)
        {
            RuntimeState.BoostRemainingSeconds = BoostDurationSeconds;
        }
    }
    const float Consumption = FuelConsumptionPerSecond * (FMath::Abs(PendingInput.Throttle) + (bBoosting ? 1.5f : 0.0f)) * DeltaTime;
    CurrentFuel = FMath::Clamp(CurrentFuel - Consumption, 0.0f, 1.0f);
    RuntimeState.FuelNormalized = CurrentFuel;
    RuntimeState.DurabilityNormalized = CurrentDurability;
    RuntimeState.bEngineActive = IsOperational() && !FMath::IsNearlyZero(PendingInput.Throttle);
    if (AActor* OwnerActor = GetOwner())
    {
        const FVector ForwardMove = OwnerActor->GetActorForwardVector() * RuntimeState.CurrentSpeedCentimetersPerSecond * DeltaTime;
        OwnerActor->SetActorLocation(OwnerActor->GetActorLocation() + ForwardMove, true);
    }
    BroadcastState();
}

void UAstrawildVehicleComponent::BroadcastState()
{
    RuntimeState.FuelNormalized = CurrentFuel;
    RuntimeState.DurabilityNormalized = CurrentDurability;
    OnVehicleStateChanged.Broadcast(RuntimeState);
}
