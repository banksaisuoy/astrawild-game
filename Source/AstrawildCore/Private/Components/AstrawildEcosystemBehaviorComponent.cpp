#include "Components/AstrawildEcosystemBehaviorComponent.h"

#include "Engine/DataTable.h"
#include "GameFramework/Actor.h"

UAstrawildEcosystemBehaviorComponent::UAstrawildEcosystemBehaviorComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UAstrawildEcosystemBehaviorComponent::BeginPlay()
{
    Super::BeginPlay();
    if (!SpeciesTag.IsValid())
    {
        SpeciesTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Echo.Unknown")), false);
    }
}

void UAstrawildEcosystemBehaviorComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    if (!HasAuthorityForEcosystem())
    {
        return;
    }
    StateChangeCooldownRemaining = FMath::Max(0.0f, StateChangeCooldownRemaining - DeltaTime);
    // AI movement/path following is owned by the pawn/controller. This component
    // only resolves high-level intent and avoids spawning work on every tick.
    if (StateChangeCooldownRemaining <= 0.0f)
    {
        EvaluateState(false);
    }
}

void UAstrawildEcosystemBehaviorComponent::SetPerceptionState(const float InHealthNormalized, const float InFearNormalized, const float InHungerNormalized, const bool bThreatNearby)
{
    if (!HasAuthorityForEcosystem())
    {
        return;
    }
    CurrentHealthNormalized = FMath::Clamp(InHealthNormalized, 0.0f, 1.0f);
    CurrentFear = FMath::Clamp(InFearNormalized, 0.0f, 1.0f);
    HungerNormalized = FMath::Clamp(InHungerNormalized, 0.0f, 1.0f);
    EvaluateState(bThreatNearby);
}

void UAstrawildEcosystemBehaviorComponent::SetWorldEvent(const FGameplayTag EventTag, const bool bMigrationActive)
{
    if (!HasAuthorityForEcosystem())
    {
        return;
    }
    CurrentWorldEventTag = EventTag;
    const FAstrawildEcosystemBehaviorRow* behavior = FindBehavior();
    if (bMigrationActive && behavior && behavior->bCanMigrateDuringWorldEvents)
    {
        SetState(EAstrawildEcosystemState::Migrate);
    }
}

void UAstrawildEcosystemBehaviorComponent::ForceState(const EAstrawildEcosystemState NewState)
{
    if (HasAuthorityForEcosystem())
    {
        SetState(NewState);
    }
}

bool UAstrawildEcosystemBehaviorComponent::IsInCombatRelevantState() const
{
    return CurrentState == EAstrawildEcosystemState::DefendTerritory || CurrentState == EAstrawildEcosystemState::Hunt || CurrentState == EAstrawildEcosystemState::Flee;
}

const FAstrawildEcosystemBehaviorRow* UAstrawildEcosystemBehaviorComponent::FindBehavior() const
{
    if (!BehaviorTable || !SpeciesTag.IsValid())
    {
        return nullptr;
    }
    TArray<FAstrawildEcosystemBehaviorRow*> rows;
    BehaviorTable->GetAllRows<FAstrawildEcosystemBehaviorRow>(TEXT("AstrawildEcosystemLookup"), rows);
    FAstrawildEcosystemBehaviorRow** Found = rows.FindByPredicate([this](const FAstrawildEcosystemBehaviorRow* row)
    {
        return row && row->SpeciesTag == SpeciesTag;
    });
    return Found ? *Found : nullptr;
}

void UAstrawildEcosystemBehaviorComponent::EvaluateState(const bool bThreatNearby)
{
    const FAstrawildEcosystemBehaviorRow* behavior = FindBehavior();
    if (!behavior)
    {
        SetState(bThreatNearby ? EAstrawildEcosystemState::Investigate : EAstrawildEcosystemState::Roam);
        return;
    }
    if (CurrentHealthNormalized <= behavior->FleeHealthThreshold || CurrentFear >= 0.85f)
    {
        SetState(EAstrawildEcosystemState::Flee);
        return;
    }
    if (bThreatNearby && behavior->Temperament == EAstrawildEcosystemTemperament::Territorial)
    {
        SetState(EAstrawildEcosystemState::DefendTerritory);
        return;
    }
    if (bThreatNearby && (behavior->Temperament == EAstrawildEcosystemTemperament::PackHunter || behavior->Temperament == EAstrawildEcosystemTemperament::SolitaryApex))
    {
        SetState(EAstrawildEcosystemState::Hunt);
        return;
    }
    if (HungerNormalized >= 0.70f)
    {
        SetState(behavior->DietTag.IsValid() ? EAstrawildEcosystemState::Forage : EAstrawildEcosystemState::Graze);
        return;
    }
    if (behavior->bFormsGroups)
    {
        SetState(EAstrawildEcosystemState::Flock);
        return;
    }
    SetState(EAstrawildEcosystemState::Roam);
}

void UAstrawildEcosystemBehaviorComponent::SetState(const EAstrawildEcosystemState NewState)
{
    if (CurrentState == NewState)
    {
        return;
    }
    CurrentState = NewState;
    StateChangeCooldownRemaining = 0.5f;
    OnStateChanged.Broadcast(NewState);
}

bool UAstrawildEcosystemBehaviorComponent::HasAuthorityForEcosystem() const
{
    return !GetOwner() || GetOwner()->HasAuthority();
}
