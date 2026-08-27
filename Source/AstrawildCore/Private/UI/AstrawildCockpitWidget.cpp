#include "UI/AstrawildCockpitWidget.h"

#include "Components/AstrawildMechaComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"

void UAstrawildCockpitWidget::NativeTick(const FGeometry& MyGeometry, const float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);
    RefreshAccumulator += InDeltaTime;
    if (RefreshAccumulator >= 0.10f)
    {
        RefreshAccumulator = 0.0f;
        RefreshCockpitState();
    }
}

void UAstrawildCockpitWidget::RefreshCockpitState()
{
    BindMechaIfNeeded();
    if (BoundMecha.IsValid())
    {
        const UAstrawildMechaComponent* Mecha = BoundMecha.Get();
        CockpitState.EnergyNormalized = Mecha->GetEnergyPercent();
        CockpitState.HeatNormalized = Mecha->GetHeatPercent();
        CockpitState.ShieldNormalized = Mecha->GetShieldPercent();
        CockpitState.bIsOverboosting = Mecha->IsOverboosting();
        CockpitState.bIsOverheated = Mecha->IsOverheated();
        CockpitState.bIsFlying = Mecha->IsFlightActive();
        CockpitState.EquippedWeaponTag = Mecha->EquippedWeaponTag;
    }
    else
    {
        CockpitState.EnergyNormalized = 0.0f;
        CockpitState.HeatNormalized = 0.0f;
        CockpitState.ShieldNormalized = 0.0f;
        CockpitState.bIsOverboosting = false;
        CockpitState.bIsOverheated = false;
        CockpitState.bIsFlying = false;
        CockpitState.EquippedWeaponTag = FGameplayTag::EmptyTag;
    }

    if (LockedTarget.IsValid() && GetOwningPlayerPawn())
    {
        const APawn* OwnerPawn = GetOwningPlayerPawn();
        CockpitState.TargetDistance = FVector::Distance(OwnerPawn->GetActorLocation(), LockedTarget->GetActorLocation());
        CockpitState.TargetName = FText::FromString(LockedTarget->GetName());
    }
    else
    {
        CockpitState.bHasTargetLock = false;
        CockpitState.TargetDistance = 0.0f;
        CockpitState.TargetName = FText::GetEmpty();
    }
    OnCockpitStateChanged.Broadcast(CockpitState);
}

void UAstrawildCockpitWidget::SetTargetLock(AActor* TargetActor, const bool bLocked)
{
    LockedTarget = bLocked ? TargetActor : nullptr;
    CockpitState.bHasTargetLock = bLocked && IsValid(TargetActor);
    RefreshCockpitState();
}

void UAstrawildCockpitWidget::BindMechaIfNeeded()
{
    if (BoundMecha.IsValid())
    {
        return;
    }
    if (APawn* OwnerPawn = GetOwningPlayerPawn())
    {
        BoundMecha = OwnerPawn->FindComponentByClass<UAstrawildMechaComponent>();
    }
}
